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

extern ElecPrice g_pricingInfo;

#define EVSTATEPOLLINGPERIOD_PARKED 300
#define HEATEROFF_SOCTHRESHOLD 80

enum heaterState
{
  S0_IDLE = 0,
  S1_QueriedTemp,
  S2_GotTemp
  S3_TempLimitRaised
}


EVStateDependents::EVStateDependents()
{

};
EVStateDependents::~EVStateDependents() {};

void* EVStateDependents::run()
{
  bool isIdle = TRUE;
  bool isHeaterOn = FALSE;
  int retVal;

  VehicleState evState = VehicleState();
  retVal = evState.init();
  if (retVal < 0)
    
  UserHabit uh = UserHabit();

  // constructor opens file and sets unblocking
  I2CWrapper fpgaI2C = I2CWrapper();
  GPIOWrapper heaterGPIO = GPIOWrapper();

  while(1)
  {
    // Also updates g_currentDate
    evState.getNextract();
    cv_signal(gettingEVStateDone);

    if ( evState.isErrorPresent() )
    {
      if ( evState.isDifferentError() )
      {
        char errorStr[ERRORSTRSIZE];
        evState.generateErrorStr(errorStr);
        ftphdlr.uploadError(errorStr);

      }
    }
    // If currently driving
    if ( !(isIdle = evState.isCurrentlyParked) )
    {

    }




    cv_wait(isEveyoneDoneWithEVStateFile)
    sleep(isIdle * EVSTATEPOLLINGPERIOD_PARKED)
    
  }
}


void EVStateDependents::fpgaCtrl()
{
  if ( !g_EVStateNInputVInterface.getShouldFPGAon() || g_EVStateNInputVInterface.getIsChargingHWErr() )
  {
    return;
  }

  int inputCurrent = calculateInputCurrent();
  if ( FPGAI2C(InputCurrent) )
  {
    t1t2Interface.setShouldFPGAon(FALSE)
    t1t2Interface.setIsChargingHWErr(TRUE)
  }
}


void EVStateDependents::heaterCtrl()
{
  if (m_isHeaterOoS)
  {
    return;
  }

  switch (m_heaterState)
  {
    case S0:
      DBG_OUT_MSG("HeaterState: S0");
      if ( t1t2Interface.getShouldFPGAon() )
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
    case S1:
      DBG_OUT_MSG("HeaterState: S1");
      if (t1t4Interface.getIsTempReady())
      {
        m_lastTempQueryTime = g_currentDate;      
        m_heaterState = S2;
      }
      else if (! t1t2Interface.getShouldFPGAon() )
      {
        m_heaterState = S0;
      }
      break;
    case S2:
      DBG_OUT_MSG("HeaterState: S2");
      if (! t1t2Interface.getShouldFPGAon() )
      {
        m_heaterState = S0;
      }
      else if (nextDrivingTime - g_currentDate <= PREHEATINGPERIOD)
      {
        setTempLevel(PREHEAT);
        m_heaterState = S3;
      }
      break;
    case S3:
      DBG_OUT_MSG("HeaterState: S3");
      if (! t1t2Interface.getShouldFPGAon() )
      {
        setTempLevel(NORMAL);
        m_heaterState = S0;
      }
      break;
    default:
      m_isHeaterOoS = TRUE;
      DBG_ERR_MSG("Unknown heater State!");
  }

  if ( t1t2Interface.getShouldFPGAon() ) // charging mode
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