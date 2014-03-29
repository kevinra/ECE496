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
#include <signal.h>
#include "EVStateDependents.hpp"
#include "EVStateNInputVInterface.hpp"
#include "VehicleState.hpp"
#include "GPIOWrapper.hpp"

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
          if ( m_pVS_evState->isDifferentError() )
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
    sleep(3);
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
      #ifdef NOFPGA
      if ( m_pVS_evState->getCurrentFlow() > 0)
      {
        m_vmState = S1_PARKED_CHARGING;
      }
      #else
      if (g_EVStateNInputVInterface.getShouldFPGAOn() )
      {
        m_vmState = S1_PARKED_CHARGING;
      }
      #endif

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
        DBG_OUT_MSG("Current trip is over.");
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


// FPGA has timeout mechanism so that it will not send signals to LLCs if it hasn't
// received any signals for 1 second.
void EVStateDependents::fpgaCtrl()
{
  #ifdef NOFPGA
  if ( m_pVS_evState->getCurrentFlow() == 0 ||
       m_isFaultFromStateFile ||
       g_EVStateNInputVInterface.getIsChargingHWOoS()
     )
  {
    return;
  }
  #else
  if ( !g_EVStateNInputVInterface.getShouldFPGAOn() ||
       m_isFaultFromStateFile ||
       g_EVStateNInputVInterface.getIsChargingHWOoS()
     )
  {
    return;
  }
  #endif

  // int inputCurrent = calculateInputCurrent();
  // if ( FPGAI2C(InputCurrent) )
  // {
  //   g_EVStateNInputVInterface.setIsChargingHWOoS(true)
  // }
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

