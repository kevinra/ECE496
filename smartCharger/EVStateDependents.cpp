/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.cpp

  
*/

// <T1>
#include <stdio.h>
#include <fstream>
#include <unistd.h>
#include <sqlite3.h>
#include <cstdlib> // for abs()
#include <signal.h>
#include "EVStateDependents.hpp"
#include "EVStateNInputVInterface.hpp"
#include "VehicleState.hpp"
#include "GPIOWrapper.hpp"

#define NEXTDRIVING_INIT -1
#define NEXTDRIVING_DAY_DONTCARE 100
#define DISTPERSOC_INIT 20
#define ALPHA 0.875
#define BINARYLOCATION_MOVE "/bin/mv"
#define DBNAME "sm.db"
#define ROUNDINGTHRSHD 4 


#define SIMULATION_INPUTCURRENT_MAX 7

struct sqlRetVal
{
  int day;
  int starthour;
  int numOfAttrib;
};

EVStateDependents::EVStateDependents()
{
  m_pVS_evState = new VehicleState();
  m_vmState = S0_PARKED_NOT_CHARGING;
  m_driveBeginTimeRowID = 0;
  m_nextDriving_time = NEXTDRIVING_INIT;
  m_nextDriving_day = NEXTDRIVING_INIT;
  m_lastTravelledDist = 0;
  m_distPerSoC = DISTPERSOC_INIT;
  m_isFaultFromStateFile = false;
  m_inputCurrent = 0;
  #ifdef SIM_IN_CUR
  m_simulatedCurrent = 0;
  #endif
  m_pGPIO_bOut_fIn_2 = new GPIOWrapper(GPIO_BBB_OUT_FPGA_IN_2);
  m_pGPIO_bOut_fIn_1 = new GPIOWrapper(GPIO_BBB_OUT_FPGA_IN_1);
  m_pGPIO_bOut_fIn_0 = new GPIOWrapper(GPIO_BBB_OUT_FPGA_IN_0);
}


EVStateDependents::~EVStateDependents()
{
  delete m_pVS_evState;
  delete m_pGPIO_bOut_fIn_2;
  delete m_pGPIO_bOut_fIn_1;
  delete m_pGPIO_bOut_fIn_0;
}


int EVStateDependents::init()
{
  if ( m_pVS_evState->init() )
  {
    DBG_ERR_MSG("VehicleState object initialization failed!");
    return 1;
  }

  #ifdef BBB
  // We might be able to initialize them as non-blocking, but it's not tested
  if ( m_pGPIO_bOut_fIn_2->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Beaglebone output pin #2 to FPGA input  initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_bOut_fIn_1->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Beaglebone output pin #1 to FPGA input  initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_bOut_fIn_0->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Beaglebone output pin #0 to FPGA input  initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  #endif

  return 0;
}


void* EVStateDependents::run()
{
  while(1)
  {
    // If state file is successfully obtained
    if ( !m_pVS_evState->getCurDateNStateFile(m_tp_curTime) )
    {
      if ( !m_pVS_evState->extractData() )
      {

        #ifdef DEBUG
        m_pVS_evState->printExtractedAttribs();
        #endif

        if ( ( m_isFaultFromStateFile = m_pVS_evState->getIsFaultPresent() ) )
        {
          if ( m_pVS_evState->isNewError() )
          {
            char errorStr[ERRORSTRSIZE];
            m_pVS_evState->generateErrorStr(errorStr);
            // g_errQueue.addErrStr(errorStr);
            DBG_OUT_VAR(errorStr);
          }
        }
        vclmvmRecordHandle();
        fpgaCtrl();
        piggybackInfoNRenameFileWithVclMvm();
      }
    }
    #ifdef DEBUG
    sleep(3);
    #endif
  }
  // Must not reach here!
  DBG_ERR_MSG("Infinite while loop stopped!");
}


void EVStateDependents::vclmvmRecordHandle()
{
  // FSM logic
  switch (m_vmState)
  {
    case S0_PARKED_NOT_CHARGING:
      DBG_OUT_MSG("VM Record State: S0_PARKED_NOT_CHARGING");
      #ifdef NO_RECT
      if ( m_pVS_evState->getCurrentFlow() > 0)
      #else
      if (g_EVStateNInputVInterface.getShouldLLCOn() )
      #endif
      {
        sql_findNextDrivingInfo();
        m_vmState = S1_PARKED_CHARGING;
      }

      else if ( !m_pVS_evState->getIsParked() )
      {
        // Find rowID of current time in BBB DB so that the row can be udated
        // with travelled distance after the trip is over.
        m_driveBeginTimeRowID = sql_findCorrespRowID();
        m_lastTravelledDist = 0;
        m_vmState = S2_CRUSING;
      }
      break;
    case S1_PARKED_CHARGING:
      DBG_OUT_MSG("VM Record State: S1_PARKED_CHARGING");
      #ifdef NO_RECT
      if ( m_pVS_evState->getCurrentFlow() == 0 || m_isFaultFromStateFile )
      #else
      if ( !g_EVStateNInputVInterface.getShouldLLCOn() || m_isFaultFromStateFile )
      #endif
      {
        m_inputCurrent = 0;
        m_nextDriving_time = NEXTDRIVING_INIT;
        m_nextDriving_day = NEXTDRIVING_INIT;
        m_vmState = S0_PARKED_NOT_CHARGING;
      }
      break;
    case S2_CRUSING:
      DBG_OUT_MSG("VM Record State: S2_CRUSING");
      if ( m_pVS_evState->getIsSoCDecreased() )
      {
        // Instead of calculating average, it uses how TCP calculates timeout period.
        // There is room for improvement, of course.
        float curTotalDist = m_pVS_evState->getTravelledDist();
        m_distPerSoC = ALPHA * m_distPerSoC + (1 - ALPHA) * (curTotalDist - m_lastTravelledDist);
        m_lastTravelledDist = curTotalDist;
        DBG_OUT_MSG("SOC decreased. m_distPerSoC is updated to " << m_distPerSoC);
      }
      if ( m_pVS_evState->getIsParked() )
      {
        DBG_OUT_MSG("Current trip is over.");
        // If rowID obtained is zero, it means that the program failed
        // to obtain a tuple that matches criteria
        if (m_driveBeginTimeRowID)
        {
          float td = m_pVS_evState->getTravelledDist();
          sql_updateCorrespRowID(td);
          DBG_OUT_MSG("Total distance travelled for this trip: " << td << "m");
        }
        m_pVS_evState->resetTravelledDist();
        m_vmState = S0_PARKED_NOT_CHARGING;
      }
      break;

    default:
      DBG_ERR_MSG("Unknown VM Record State");
  }
  return;
}


void EVStateDependents::fpgaCtrl()
{
  #ifdef NO_RECT
  if ( m_pVS_evState->getCurrentFlow() == 0 ||
  #else
  if ( !g_EVStateNInputVInterface.getShouldLLCOn() ||
  #endif
       m_vmState != S1_PARKED_CHARGING ||
       m_isFaultFromStateFile ||
       g_EVStateNInputVInterface.getIsChargingHWOoS()
     )
  {
    DBG_OUT_MSG("Beaglebone doesn't need to communicate with FPGA now.");
    return;
  }

  int curDay, curHour, curMin;
  getCurTimeInfo(curDay, curHour, curMin); // Note that minutes are rounded to nearest 10.

  // If SQL query to find next driving time and day failed,
  // the values below will be the same as the initialized value.
  if ( m_nextDriving_time != NEXTDRIVING_INIT && m_nextDriving_day != NEXTDRIVING_INIT)
  {
    m_inputCurrent = calculateInputCurrent(curHour, curDay);
    DBG_OUT_MSG("calculated input current: " << m_inputCurrent);
  }
  
  #ifdef SIM_IN_CUR
  if (m_simulatedCurrent > SIMULATION_INPUTCURRENT_MAX)
  {
    m_simulatedCurrent = 0;
  }
  if ( fpgaSend(m_simulatedCurrent) )
  #else
  if ( fpgaSend(m_inputCurrent) )
  #endif
  {
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
  }

  #ifdef SIM_IN_CUR
  m_simulatedCurrent++;
  #endif

  return;
}


void EVStateDependents::piggybackInfoNRenameFileWithVclMvm()
{
  // Piggyback derived information into state file.
  char origFilePath[STATEFILE_FULLPATHSIZE];
  char newFilePath[STATEFILE_FULLPATHSIZE];
  char* origStateFileName = m_pVS_evState->getStateFileName();
  snprintf(origFilePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", origStateFileName);

  std::ofstream fs;
  fs.open(origFilePath, std::ios::out | std::ios::app);
  fs << "\n\"travelledDistance\":\t" << m_pVS_evState->getTravelledDist() << ",\n";
  fs << "\"distancePerSoC\":\t" << m_distPerSoC << "\n";
  fs << "\"calculatedInputCurrent\":\t" << m_inputCurrent << "\n";
  int curDay, curHour, curMin;
  getCurTimeInfo(curDay, curHour, curMin); // Note that minutes are rounded to nearest 10.
  int remainingHour = abs(curDay - m_nextDriving_day) * 24 + (m_nextDriving_time - curHour);
  fs << "\"hourTillNextDriving\":\t" << remainingHour << "\n";

  fs.close();

  // Rename the file
  if (m_vmState == S0_PARKED_NOT_CHARGING)
  {
    snprintf(newFilePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "S0_%s", origStateFileName);
  }
  else if (m_vmState == S1_PARKED_CHARGING)
  {
    snprintf(newFilePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "S1_%s", origStateFileName);
  }
  else if (m_vmState == S2_CRUSING)
  {
    snprintf(newFilePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "S2_%s", origStateFileName);
  }

  pid_t pID = fork();
  if (pID == 0) // Child process
  {
    // If exec is successful, it would not return a value, it will trigger the if statement
    // only if there is some error. However, not all the errors are handled by this.
    if ( execl(BINARYLOCATION_MOVE, BINARYLOCATION_MOVE, origFilePath, newFilePath, NULL) == -1 )
    {
      ERR_MSG("renaming state file failed!");
    }
    exit(1);
  }
  else if (pID < 0)
  {
    ERR_MSG("fork() to rename " << origFilePath << " to " << newFilePath << " failed!");
  }
  waitpid(pID, NULL, 0);
  kill(pID, SIGINT);
  return;
}


// Note that if no match is found on the database, it would
// return 0, which is actually an error this is case.
int EVStateDependents::sql_findCorrespRowID()
{
  sqlite3 *db;
  char *zErrMsg = 0;
  int rowID = 0;

  // Open database
  if( sqlite3_open(DBNAME, &db) )
  {
    ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return rowID;
  }

  int curDay, curHour, curMin;
  getCurTimeInfo(curDay, curHour, curMin); // Note that minutes are rounded to nearest 10.

  char sql[STATEFILE_FULLPATHSIZE];
  snprintf(sql, STATEFILE_FULLPATHSIZE,
           "SELECT rowID FROM dc WHERE day=%d AND starthour=%d AND startmin=%d",
           curDay, curHour, curMin);

  if( sqlite3_exec(db, sql, EVStateDependents::sql_callBack_findCorrespRowID,
      (void*)&rowID, &zErrMsg) != SQLITE_OK )
  {
    ERR_MSG("SQL SELECT error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  DBG_OUT_MSG("returning rowID = " << rowID);
  return rowID;
}


int EVStateDependents::sql_callBack_findCorrespRowID(void* data, int argc, char **argv, char **azColName)
{
   *(int*)data = atoi(argv[0]);
   return 0;
}


void EVStateDependents::sql_updateCorrespRowID(float td)
{
  sqlite3 *db;
  char *zErrMsg = 0;

  if( sqlite3_open(DBNAME, &db) )
  {
    ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return;
  }
  char sql[STATEFILE_FULLPATHSIZE];
  snprintf(sql, STATEFILE_FULLPATHSIZE,
           "UPDATE dc SET distance=%f WHERE rowID=%d",
           td, m_driveBeginTimeRowID);

  if( sqlite3_exec(db, sql, NULL, NULL, &zErrMsg) != SQLITE_OK )
  {
    ERR_MSG("SQL UPDATE error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  return;
}


void EVStateDependents::sql_findNextDrivingInfo()
{
  sqlite3 *db;
  char *zErrMsg = 0;

  // Open database
  if( sqlite3_open(DBNAME, &db) )
  {
    ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return;
  }

  int curDay, curHour, curMin;
  getCurTimeInfo(curDay, curHour, curMin);
  float availableDist = m_distPerSoC * m_pVS_evState->getSoC();
  DBG_OUT_MSG("Looking for a row that is later than " << curDay << " day " << curHour << ":" << curMin <<
              " and distance greater than " << availableDist);

  char sql[ERRORSTRSIZE]; // Note that this string length has nothing to do with the error string.
  snprintf(sql, ERRORSTRSIZE,
           "SELECT day, starthour FROM dc WHERE rowid = (SELECT MIN(rowid) FROM DC WHERE day>=%d AND starthour>=%d AND startmin>%d AND distance>=%f)",
           curDay, curHour, curMin, availableDist);

  struct sqlRetVal srv;
  srv.numOfAttrib = 0;
  if( sqlite3_exec(db, sql, EVStateDependents::sql_callBack_findNextDrivingInfo,
     (void*)&srv, &zErrMsg) != SQLITE_OK )
  {
    ERR_MSG("SQL SELECT error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);

  if (srv.numOfAttrib) //Checking whether SQL actually returned something.
  {
    DBG_OUT_MSG("returning next driving infos: " << srv.day << "day " << srv.starthour << "hour");
    m_nextDriving_day = srv.day;
    m_nextDriving_time = srv.starthour;
  }
  else
  {
    DBG_OUT_MSG("SQL SELECT found no matching tuple");
    m_nextDriving_day = NEXTDRIVING_DAY_DONTCARE;
    m_nextDriving_time = 0;
  }

  return;
}


int EVStateDependents::sql_callBack_findNextDrivingInfo(void* data, int argc, char **argv, char **azColName)
{
  struct sqlRetVal* srv = (sqlRetVal*) data;
  srv -> day = atoi(argv[0]);
  srv -> starthour = atoi(argv[1]);
  srv -> numOfAttrib = argc;
  return 0;
}


//code for input current calculation
/*
  int currentTime; //from somewhere** 24-hr clock with hour, no minutes
  int dayCurrentTime; //from somewhere** day of the current time
  
  int nextDrivingTime; //from somewhere** 24-hr clock with hour, no minutes 
  int dayNextDrivingTime; //from somewhere** day of the next driving time
  
  int SOC_remaining; //from somewhere** remaining SOC value in %  
*/

int EVStateDependents::calculateInputCurrent(int currentTime, int dayCurrentTime)
{
  int zeroCurrent = 0; //no current
  int trickleCurrent = 1; //trickle charge
  int maxCurrent = 8; //in number of phases
  
  int currentPricingPhase;
  int drivingPricingPhase;
  int lowPricing = 72; //price during low
  int medPricing = 109; //price during med
  int highPricing = 129; //price during high
  
  //determining price at current time
  if (currentTime >= 0 || currentTime < 7) {currentPricingPhase = 1;} //first low phase
  else if (currentTime >= 7 || currentTime < 11) {currentPricingPhase = 2;} //first med phase
  else if (currentTime >= 11 || currentTime < 17) {currentPricingPhase = 3;} //high phase
  else if (currentTime >= 17 || currentTime < 19) {currentPricingPhase = 4;} //second med phase
  else if (currentTime >= 19 || currentTime < 24) {currentPricingPhase = 5;} //second low phase
  
  //determining price at next driving time
  if (m_nextDriving_time >= 0 || m_nextDriving_time < 7) {drivingPricingPhase = 1;} //first low phase
  else if (m_nextDriving_time >= 7 || m_nextDriving_time < 11) {drivingPricingPhase = 2;} //first med phase
  else if (m_nextDriving_time >= 11 || m_nextDriving_time < 17) {drivingPricingPhase = 3;} //high phase
  else if (m_nextDriving_time >= 17 || m_nextDriving_time < 19) {drivingPricingPhase = 4;} //second med phase
  else if (m_nextDriving_time >= 19 || m_nextDriving_time < 24) {drivingPricingPhase = 5;} //second low phase
  
  int chargingTime = m_nextDriving_time + 24 * (m_nextDriving_day - dayCurrentTime) - currentTime; //total charging hours
  
  int low_chargingTime; //hours remaining until next driving cycle
  int med_chargingTime; //hours remaining until next driving cycle
  int hi_chargingTime; //hours remaining until next driving cycle
  
  //determining charging time we have
  if (m_nextDriving_day == dayCurrentTime) //if next driving time is today
  {
    if (drivingPricingPhase == 1 && currentPricingPhase == 1)
    {
      low_chargingTime = m_nextDriving_time - currentTime;
      med_chargingTime = 0;
      hi_chargingTime = 0;
    }
    else if (drivingPricingPhase == 2 && currentPricingPhase == 1)
    {
      low_chargingTime = 7 - currentTime;
      med_chargingTime = m_nextDriving_time - 7;
      hi_chargingTime = 0;
    }
    else if (drivingPricingPhase == 2 && currentPricingPhase == 2)
    {
      low_chargingTime = 0;
      med_chargingTime = m_nextDriving_time - currentTime;
      hi_chargingTime = 0;
    }
    else if (drivingPricingPhase == 3 && currentPricingPhase == 1)
    {
      low_chargingTime = 7 - currentTime;
      med_chargingTime = 11 - 7;
      hi_chargingTime = m_nextDriving_time - 11;
    }
    else if (drivingPricingPhase == 3 && currentPricingPhase == 2)
    {
      low_chargingTime = 0;
      med_chargingTime = 11 - currentTime;
      hi_chargingTime = m_nextDriving_time - 11;
    }
    else if (drivingPricingPhase == 3 && currentPricingPhase == 3)
    {
      low_chargingTime = 0;
      med_chargingTime = 0;
      hi_chargingTime = m_nextDriving_time - currentTime;
    }
    else if (drivingPricingPhase == 4 && currentPricingPhase == 1)
    {
      low_chargingTime = 7 - currentTime;
      med_chargingTime = 11 - 7 + m_nextDriving_time - 17;
      hi_chargingTime = 17 - 11;
    }
    else if (drivingPricingPhase == 4 && currentPricingPhase == 2)
    {
      low_chargingTime = 0;
      med_chargingTime = 11 - currentTime + m_nextDriving_time - 17;
      hi_chargingTime = 17 - 11;
    }
    else if (drivingPricingPhase == 4 && currentPricingPhase == 3)
    {
      low_chargingTime = 0;
      med_chargingTime = m_nextDriving_time - 17;
      hi_chargingTime = 17 - currentTime;
    }
    else if (drivingPricingPhase == 4 && currentPricingPhase == 4)
    {
      low_chargingTime = 0;
      med_chargingTime = m_nextDriving_time - currentTime;
      hi_chargingTime = 0;
    }
    else if (drivingPricingPhase == 5 && currentPricingPhase == 1)
    {
      low_chargingTime = 7 - currentTime + m_nextDriving_time - 19;
      med_chargingTime = 19 - 17 + 11 - 7;
      hi_chargingTime = 17 - 11;
    }
    else if (drivingPricingPhase == 5 && currentPricingPhase == 2)
    {
      low_chargingTime = m_nextDriving_time - 19;
      med_chargingTime = 19 - 17 + 11 - currentTime;
      hi_chargingTime = 17 - 11;
    }
    else if (drivingPricingPhase == 5 && currentPricingPhase == 3)
    {
      low_chargingTime = m_nextDriving_time - 19;
      med_chargingTime = 19 - 17;
      hi_chargingTime = 17 - currentTime;
    }
    else if (drivingPricingPhase == 5 && currentPricingPhase == 4)
    {
      low_chargingTime = m_nextDriving_time - 19;
      med_chargingTime = 19 - currentTime;
      hi_chargingTime = 0;
    }
    else if (drivingPricingPhase == 5 && currentPricingPhase == 5)
    {
      low_chargingTime = m_nextDriving_time - currentTime;
      med_chargingTime = 0;
      hi_chargingTime = 0;
    }
  }
  else if (m_nextDriving_day > dayCurrentTime) //if next driving time is not today
  {
    if (drivingPricingPhase == 1 && currentPricingPhase == 5)
    {
      low_chargingTime = 24 - currentTime + 7 - m_nextDriving_time;
      med_chargingTime = 0;
      hi_chargingTime = 0;
    }
    else
    {
      low_chargingTime = 8;
      med_chargingTime = 0;
      hi_chargingTime = 0;
    }
  }
    
  double idealCurrent;
  double idealCurrentL;
  double idealCurrentM;
  double idealCurrentH;
  
  int inputCurrentL = 0; //input current during low price phases
  int inputCurrentM = 0; //input current during medium price phases
  int inputCurrentH = 0; //input current during high price phases
  
  int SOC_90 = 90; //90% SOC value
  int SOC_remaining = m_pVS_evState->getSoC(); //from somewhere** remaining SOC value in %
  
  //determining the ideal current for full charge
  if (low_chargingTime != 0)
  {
    idealCurrentL = (((SOC_90 - SOC_remaining) * 110) / low_chargingTime) / 1.2;
    //SOC's in %, 110 in Amp*hour (110 is full charge capacity), 1.2 in Amp, therefore idealCurrent in # of phases
  }
    
  //determining the ideal current including the price of electricity
  if (idealCurrentL > maxCurrent) //must charge at full power regardless of price
  {
    inputCurrentL = maxCurrent;
    if (med_chargingTime != 0)
    {
      idealCurrentM = ((((SOC_90 - SOC_remaining) * 110) - (low_chargingTime * inputCurrentL * 1.2)) / med_chargingTime) / 1.2;
      if (idealCurrentM > maxCurrent)
      {
        inputCurrentM = maxCurrent;
        if (hi_chargingTime != 0)
        {
          idealCurrentH = (((SOC_90 - SOC_remaining) * 110) - (low_chargingTime * inputCurrentL * 1.2) - (med_chargingTime * inputCurrentM * 1.2) / hi_chargingTime) / 1.2;
          if (idealCurrentH > maxCurrent)  
          {
            inputCurrentH = maxCurrent;
          }
          else 
          {
            inputCurrentH = idealCurrentH;
          }
        }
      }
    }
  } else {inputCurrentL = idealCurrentL;}
  
  if (SOC_remaining <= 90)
  {
    if (currentPricingPhase == 1 || currentPricingPhase == 5)
      return inputCurrentL;
    else if (currentPricingPhase == 2 || currentPricingPhase == 4)
      return inputCurrentM;
    else if (currentPricingPhase ==3)
      return inputCurrentH;
  }
  else if (SOC_remaining > 90 && SOC_remaining < 95)
  {
    return trickleCurrent;
  }
  return zeroCurrent;
}


int EVStateDependents::fpgaSend(int value)
{
  // Workaround for having only 3 bits
  if (value > SIMULATION_INPUTCURRENT_MAX)
  {
    value = SIMULATION_INPUTCURRENT_MAX;
  }

  unsigned int bit_2 = (value & 0x00000004) >> 2;
  unsigned int bit_1 = (value & 0x00000002) >> 1;
  unsigned int bit_0 = value & 0x00000001;
  DBG_OUT_MSG("Sending " << value << "(" << bit_2 << bit_1 << bit_0 << ") to FPGA");
  
  #ifdef BBB
  if ( m_pGPIO_bOut_fIn_2->gpioSet( static_cast<PIN_VALUE>(bit_2) ) )
  {
    // g_errQueue.addErrStr("GPIO pins for the BBB output #2 to FPGA input failed!");
    return 1;
  }
  if ( m_pGPIO_bOut_fIn_1->gpioSet( static_cast<PIN_VALUE>(bit_1) ) )
  {
    // g_errQueue.addErrStr("GPIO pins for the BBB output #1 to FPGA input failed!");
    return 1;
  }
  if ( m_pGPIO_bOut_fIn_0->gpioSet( static_cast<PIN_VALUE>(bit_0) ) )
  {
    // g_errQueue.addErrStr("GPIO pins for the BBB output #0 to FPGA input failed!");
    return 1;
  }
  #endif

  return 0;
}

void EVStateDependents::getCurTimeInfo(int& day, int& hour, int& min)
{
  using namespace std::chrono;
  std::time_t tt_curTime = system_clock::to_time_t(m_tp_curTime);
  struct std::tm* pTimeInfo = std::localtime(&tt_curTime);
  // Round the minute to nearest 10 (current granularity of the DB)
  int roundedMin = pTimeInfo->tm_min;
  if (roundedMin % 10 > ROUNDINGTHRSHD)
  {
    roundedMin = (roundedMin + 10) / 10 * 10;
  }
  else
  {
    roundedMin = roundedMin / 10 * 10;
  }

  day = pTimeInfo->tm_wday;
  hour = pTimeInfo->tm_hour;
  min = roundedMin;
  return;
}
