/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.cpp

  
*/

// <T1>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sqlite3.h>
#include "EVStateDependents.hpp"
#include "EVStateNInputVInterface.hpp"
#include "VehicleState.hpp"
// #include "I2CWrapper.hpp"
// #include "GPIOWrapper.hpp"

// #define TEMP_1 5
// #define TEMP_2 10
// #define HEATEROFF_SOCTHRESHOLD 80
#define EVSTATEPOLLINGPERIOD_PARKED 300
#define DISTPERSOC_INIT 20
#define ALPHA 0.875
#define BINARYLOCATION_MOVE "/bin/mv"
#define DBNAME "sm.db"
#define ROUNDINGTHRSHD 4 


EVStateDependents::EVStateDependents()
{
  m_pVS_evState = new VehicleState();
  m_vmState = S0_PARKED_NOT_CHARGING;
  m_vmRowID = 0;
  m_lastTravelledDist = 0;
  m_distPerSoC = DISTPERSOC_INIT;
  m_isFaultFromStateFile = false;
  // m_pI2C_fpga = new I2CWrapper();

  // m_heaterState = S0_IDLE;
  // m_isHeaterOoS = false;
  // m_isHeaterOn = false;
  // m_tempthrshld1; = TEMP_1;
  // m_tempthrshld2; = TEMP_2;
  // m_pGPIO_heater = new GPIOWrapper(GPIO_Heater);
}

EVStateDependents::~EVStateDependents()
{
  delete m_pVS_evState;
  // delete m_pI2C_fpga;
  // delete m_pGPIO_heater;
}


int EVStateDependents::init()
{
  if ( g_EVStateNInputVInterface.init() )
  {
    DBG_ERR_MSG("Global object for evState and inputV interface failed!");
    return 1;
  }
  if ( m_pVS_evState->init() )
  {
    DBG_ERR_MSG("VehicleState object initialization failed!");
    return 1;
  }
  // if ( m_pI2C_fpga->init() )
  // {
  //   DBG_ERR_MSG("FPGA I2C initialization failed!");
  //   return 1;
  // }
  // if ( m_pGPIO_heater->init() )
  // {
  //   DBG_ERR_MSG("Heater GPIO initialization failed!");
  //   return 1;
  // }
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
        if ( ( m_isFaultFromStateFile = m_pVS_evState->getIsFaultPresent() ) )
        {
          DBG_OUT_MSG("State.json file has a fault.");
          if ( m_pVS_evState->isDifferentError() )
          {
            // char errorStr[ERRORSTRSIZE];
            // m_pVS_evState->generateErrorStr(errorStr);
            // g_errQueue.addErrStr(errorStr);
          }
        }
        vclmvmRecordHandle();

        // fpgaCtrl();
        
        // heaterCtrl();
        piggybackInfoNRenameFileWithVclMvm();
      }
    }
  }
}


void EVStateDependents::vclmvmRecordHandle()
{
  // FSM logic
  switch (m_vmState)
  {
    case S0_PARKED_NOT_CHARGING:
      DBG_OUT_MSG("VM Record State: S0_PARKED_NOT_CHARGING");
      if (g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        m_vmState = S1_PARKED_CHARGING;
      }
      else if ( !m_pVS_evState->getIsParked() )
      {
        // Record current time into BBB DB
        m_vmRowID = sqlFindCorrespRowID();
        m_vmState = S2_CRUSING;
        m_lastTravelledDist = 0;
      }
      break;
    case S1_PARKED_CHARGING:
      DBG_OUT_MSG("VM Record State: S1_PARKED_CHARGING");
      if ( !g_EVStateNInputVInterface.getShouldFPGAOn() || m_isFaultFromStateFile)
      {
        m_vmState = S1_PARKED_CHARGING;
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
        // If rowID obtained is zero, it means that the program failed
        // to obtain a tuple that matches criteria
        if (m_vmRowID)
        {
          float td = m_pVS_evState->getTravelledDist();
          sqlUpdateCorrespRowID(td);
          DBG_OUT_MSG("Total distance travelled for this trip: " << td);
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


/*
// FPGA has timeout mechanism so that it will not send signals to LLCs if it hasn't
// received any signals for 1 second.
void EVStateDependents::fpgaCtrl()
{
  if ( !g_EVStateNInputVInterface.getShouldFPGAOn() ||
       m_isFaultFromStateFile ||
       g_EVStateNInputVInterface.getIsChargingHWOoS()
     )
  {
    return;
  }

  int inputCurrent = calculateInputCurrent();
  if ( FPGAI2C(InputCurrent) )
  {
    g_EVStateNInputVInterface.setIsChargingHWOoS(true)
  }
}
*/


void EVStateDependents::piggybackInfoNRenameFileWithVclMvm()
{
  // Piggyback derived information into state file.
  char origFilePath[STATEFILE_FULLPATHSIZE];
  char newFilePath[STATEFILE_FULLPATHSIZE];
  char* origStateFileName = m_pVS_evState->getStateFileName();
  snprintf(origFilePath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", origStateFileName);

  std::ofstream fs;
  fs.open(origFilePath, std::ios::out | std::ios::app);
  fs << "\n\"travelledDistance\": " << m_pVS_evState->getTravelledDist() << ",\n";
  fs << "\n\"distancePerSoC\": " << m_distPerSoC << "\n\n";
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
  #ifdef DEBUG
  if ( execl(BINARYLOCATION_MOVE, BINARYLOCATION_MOVE, origFilePath, newFilePath, NULL) )
  {
    ERR_MSG("Renaming " << origFilePath << " to " << newFilePath << " failed!");
  }
  #else
  execl(BINARYLOCATION_MOVE, BINARYLOCATION_MOVE, origFilePath, newFilePath, NULL);
  #endif

  return;
}


// Note that if no match is found on the database, it would
// return 0, which is actually an error this is case.
int EVStateDependents::sqlFindCorrespRowID()
{
  sqlite3 *db;
  char *zErrMsg = 0;
  int rowID = 0;

  using namespace std::chrono;
  std::time_t m_tt_curTime = system_clock::to_time_t(m_tp_curTime);
  struct std::tm* pTimeInfo = std::localtime(&m_tt_curTime);

  // Open database
  if( sqlite3_open(DBNAME, &db) )
  {
    DBG_ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return rowID;
  }

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

  char sql[STATEFILE_FULLPATHSIZE];
  snprintf(sql, STATEFILE_FULLPATHSIZE,
           "SELECT rowID FROM dc WHERE day=%d AND starthour=%d AND startmin=%d",
           pTimeInfo->tm_wday, pTimeInfo->tm_hour, roundedMin);

  if( sqlite3_exec(db, sql, EVStateDependents::selectCallback, (void*)&rowID, &zErrMsg) != SQLITE_OK )
  {
    DBG_ERR_MSG("SQL SELECT error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  DBG_OUT_MSG("returning rowID = " << rowID);
  return rowID;
}


int EVStateDependents::selectCallback(void* data, int argc, char **argv, char **azColName)
{
   *(int*)data = atoi(argv[0]);
   return 0;
}


void EVStateDependents::sqlUpdateCorrespRowID(float td)
{
  sqlite3 *db;
  char *zErrMsg = 0;

  // Open database
  if( sqlite3_open(DBNAME, &db) )
  {
    DBG_ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return;
  }
  char sql[STATEFILE_FULLPATHSIZE];
  snprintf(sql, STATEFILE_FULLPATHSIZE,
           "UPDATE dc SET distance=%f WHERE rowID=%d",
           td, m_vmRowID);

  if( sqlite3_exec(db, sql, NULL, NULL, &zErrMsg) != SQLITE_OK )
  {
    DBG_ERR_MSG("SQL UPDATE error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  return;
}


/*
void EVStateDependents::heaterCtrl()
{
  if (m_isHeaterOoS)
  {
    return;
  }

  // FSM logic
  switch (m_heaterState)
  {
    case S0_IDLE:
      DBG_OUT_MSG("HeaterState: S0");
      if ( g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        // If temperature data is outdated
        if (g_currentDate - m_lastTempQueryTime > TEMPQUERYINTERVAL)
        {
          t1t4Interface.setIsTempReady(false);
          t1t4Interface.queryOutsideTemp; // like cvSignal; wake up T4
          m_heaterState = S1;
        }
        else
        {
          m_heaterState = S2;
        }
      }
      break;
    case S1_QUERIEDTEMP:
      DBG_OUT_MSG("HeaterState: S1");
      if (t1t4Interface.getIsTempReady())
      {
        m_lastTempQueryTime = g_currentDate;
        m_heaterState = S2;
      }
      else if (! g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        m_heaterState = S0;
      }
      break;
    case S2_GOT_TEMP:
      DBG_OUT_MSG("HeaterState: S2");
      if (! g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        m_heaterState = S0;
      }
      else if (nextDrivingTime - g_currentDate <= PREHEATINGPERIOD)
      {
        setTempLevel(PREHEAT);
        m_heaterState = S3;
      }
      break;
    case S3_TEMPLIMIT_RAISED:
      DBG_OUT_MSG("HeaterState: S3");
      if (! g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        setTempLevel(NORMAL);
        m_heaterState = S0;
      }
      break;
    default:
      m_isHeaterOoS = true;
      DBG_ERR_MSG("Unknown heater State!");
  }

  if ( g_EVStateNInputVInterface.getShouldFPGAOn() ) // charging mode
  {
    if ( !m_isHeaterOn && m_pVS_evState->getMinTemp() < TEMPTHRSHLD_1 )
    {
      if ( m_pGPIO_heater->gpioSet(HIGH) )
      {
        DBG_ERR_MSG("Heater GPIO set(HIGH) failed!");
        m_isHeaterOoS = true;
      }
      else
      {
        DBG_OUT_MSG("HEATER: ON");
        m_isHeaterOn = true;
      }

    } // Else if the heater is on and it is above the second threshold value
    else if ( m_isHeaterOn && m_pVS_evState->getMinTemp() > TEMPTHRSHLD_2 )
    {
      if ( m_pGPIO_heater->gpioSet(LOW) )
      {
        DBG_ERR_MSG("Heater GPIO set(LOW) failed!");
        m_isHeaterOoS = true;
      }
      else
      {
        DBG_OUT_MSG("HEATER: OFF");
        m_isHeaterOn = false;
      }
    }
  }
  else // Discharging mode
  {

  }

  return;
}

void setTempLevel(int isPreHeat)
{
  tempThreshold1 = baseTemp + PREHEATINGTEMPADJUSTMENT * isPreHeat;
  tempThreshold2 = baseTemp + PREHEATINGTEMPADJUSTMENT * isPreHeat;
  return;
}

*/

