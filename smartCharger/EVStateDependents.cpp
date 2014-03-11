/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.cpp

  
*/

// <T1>

#include "EVStateDependents.hpp"
#include "EVStateNInputVInterface.hpp"
#include "VehicleState.hpp"
#include "I2CWrapper.hpp"
#include "GPIOWrapper.hpp"

#define EVSTATEPOLLINGPERIOD_PARKED 300
#define HEATEROFF_SOCTHRESHOLD 80

enum heaterState
{
  S0_IDLE = 0,
  S1_QUERIEDTEMP,
  S2_GOT_TEMP,
  S3_TEMPLIMIT_RAISED
}

enum vclmvmState
{
  S0_PARKED_NOT_CHARGING = 0,
  S1_PARKED_CHARGING,
  S2_CRUSING,
}


EVStateDependents::EVStateDependents()
{
  m_evState = new VehicleState();
  m_vmState = S0_PARKED_NOT_CHARGING;
  m_vmRecordID = 0;
  m_heaterState = S0_IDLE;
  m_isHeaterOoS = FALSE;
  m_isHeaterOn = FALSE;
  m_isFaultFromStateFile = FALSE;
  m_tempthrshld1; = 5;
  m_tempthrshld2; = 10;
  m_fpgaI2C = new I2CWrapper();
  m_heaterGPIO = new GPIOWrapper(GPIO_Heater);
}

EVStateDependents::~EVStateDependents()
{
  delete m_evState;
  delete m_fpgaI2C;
  delete m_heaterGPIO;
}


int EVStateDependents::init()
{
  if ( m_evState.init() )
  {
    DBG_ERR_MSG("VehicleState object initialization failed!");
    return 1;
  }
  if ( m_fpgaI2C.init() )
  {
    DBG_ERR_MSG("FPGA I2C initialization failed!");
    return 1;
  }
  if ( m_heaterGPIO.init() )
  {
    DBG_ERR_MSG("Heater GPIO initialization failed!");
    return 1;
  }
  return 0;
}


void* EVStateDependents::run()
{
  bool isIdle = TRUE;
  bool isHeaterOn = FALSE;
  int retVal;

  VehicleState evState
    
  UserHabit uh = UserHabit();

  while(1)
  {
    // If state file is successfully obtained
    if ( m_evState.getCurDateNStateFile(m_rawCurTime) )
    {
      if ( m_evState.extractData(m_rawCurTime) )
      {
        cv_signal(gettingEVStateDone);

        if ( ( m_isFaultFromStateFile = m_evState.getIsFaultPresent() ) )
        {
          DBG_OUT_MSG("State.json file has fault.")
          if ( m_evState.isDifferentError() )
          {
            char errorStr[ERRORSTRSIZE];
            m_evState.generateErrorStr(errorStr);
            g_errQueue.addErrStr(errorStr);
          }
        }
        vclmvmRecordHandle();
        fpgaCtrl();
        heaterCtrl();

        cv_wait(isEveyoneDoneWithEVStateFile)
      }
    }
  }
}


void vclmvmRecordHandle()
{
  // FSM logic
  switch (m_vmState)
  {
    case S0_PARKED_NOT_CHARGING:
      DBG_OUT_MSG("VM Record State: S0_PARKED_NOT_CHARGING");
      if (g_EVStateNInputVInterface.getShouldFPGAon() )
      {
        m_vmState = S1_PARKED_CHARGING;
      }
      else if ( !m_evState.getIsParked() )
      {
        struct rm* timeInfo = localtime(&m_rawCurTime);
        // Record current time into BBB DB
        m_vmRecordID = 
        m_vmState = S2_CRUSING;
      }
      break;
    case S1_PARKED_CHARGING:
      DBG_OUT_MSG("VM Record State: S1_PARKED_CHARGING");
      if ( !g_EVStateNInputVInterface.getShouldFPGAon() || m_isFaultFromStateFile)
      {
        m_vmState = S1_PARKED_CHARGING;
      }
      break;
    case S2_CRUSING:
      DBG_OUT_MSG("VM Record State: S2_CRUSING");
      if ( g_EVStateNInputVInterface.getIsParked() )
      {
        double td = m_evState.getTravelledDist();
        // update a tuple with m_vmRecordID with td
        m_evState.resetTravelledDist();
        m_vmState = S0_PARKED_NOT_CHARGING;
      }
      break;

    default:
      DBG_ERR_MSG("Unknown VM Record State");
  }
}


// FPGA has timeout mechanism so that it will not send signals to LLCs if it hasn't
// received any signals for 1 second.
void EVStateDependents::fpgaCtrl()
{
  if ( !g_EVStateNInputVInterface.getShouldFPGAon() ||
       m_isFaultFromStateFile ||
       g_EVStateNInputVInterface.getIsChargingHWOoS()
     )
  {
    return;
  }

  int inputCurrent = calculateInputCurrent();
  if ( FPGAI2C(InputCurrent) )
  {
    g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE)
  }
}


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
      if ( g_EVStateNInputVInterface.getShouldFPGAon() )
      {
        // If temperature data is outdated
        if (g_currentDate - m_lastTempQueryTime > TEMPQUERYINTERVAL)
        {
          t1t4Interface.setIsTempReady(FALSE);
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
      else if (! g_EVStateNInputVInterface.getShouldFPGAon() )
      {
        m_heaterState = S0;
      }
      break;
    case S2_GOT_TEMP:
      DBG_OUT_MSG("HeaterState: S2");
      if (! g_EVStateNInputVInterface.getShouldFPGAon() )
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
      if (! g_EVStateNInputVInterface.getShouldFPGAon() )
      {
        setTempLevel(NORMAL);
        m_heaterState = S0;
      }
      break;
    default:
      m_isHeaterOoS = TRUE;
      DBG_ERR_MSG("Unknown heater State!");
  }

  if ( g_EVStateNInputVInterface.getShouldFPGAon() ) // charging mode
  {
    if ( !m_isHeaterOn && m_evState.getMinTemp() < TEMPTHRSHLD_1 )
    {
      if ( m_heaterGPIO.gpioSet(HIGH) )
      {
        DBG_ERR_MSG("Heater GPIO set(HIGH) failed!");
        m_isHeaterOoS = TRUE;
      }
      else
      {
        DBG_OUT_MSG("HEATER: ON");
        m_isHeaterOn = TRUE;
      }

    } // Else if the heater is on and it is above the second threshold value
    else if ( m_isHeaterOn && m_evState.getMinTemp() > TEMPTHRSHLD_2 )
    {
      if ( m_heaterGPIO.gpioSet(LOW) )
      {
        DBG_ERR_MSG("Heater GPIO set(LOW) failed!");
        m_isHeaterOoS = TRUE;
      }
      else
      {
        DBG_OUT_MSG("HEATER: OFF");
        m_isHeaterOn = FALSE;
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