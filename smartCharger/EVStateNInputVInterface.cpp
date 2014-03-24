/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.cpp

  
*/

#include "EVStateNInputVInterface.hpp"

// global object
EVStateNInputVInterface g_EVStateNInputVInterface;

EVStateNInputVInterface::EVStateNInputVInterface()
{
  m_inputVolt = inputV_dontCare;
  m_isChargingHWOoS = false;
  m_shouldFPGAOn = false;
}

EVStateNInputVInterface::~EVStateNInputVInterface()
{
  pthread_mutex_destroy(&m_inputVoltMtx);
  pthread_mutex_destroy(&m_isChargingHWOoSMtx);
  pthread_mutex_destroy(&m_shouldFPGAOnMtx);
}


int EVStateNInputVInterface::init()
{
  // Initialize mutexs
  if ( pthread_mutex_init(&m_inputVoltMtx, NULL) )
  {
    DBG_ERR_MSG("m_inputVoltMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_isChargingHWOoSMtx, NULL) )
  {
    DBG_ERR_MSG("m_isVoltmeterOoSMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_shouldFPGAOnMtx, NULL) )
  {
    DBG_ERR_MSG("m_isVoltmeterOoSMtx initialization failed!");
    return 1;
  }
  return 0;
}


inputV EVStateNInputVInterface::getInputVolt()
{
  pthread_mutex_lock(&m_inputVoltMtx);
  inputV iv = m_inputVolt;
  pthread_mutex_unlock(&m_inputVoltMtx);
  return iv;
}


void EVStateNInputVInterface::setInputVolt(inputV iv)
{
  pthread_mutex_lock(&m_inputVoltMtx);
  m_inputVolt = iv;
  pthread_mutex_unlock(&m_inputVoltMtx);
  return;
}


bool EVStateNInputVInterface::getIsChargingHWOoS()
{
  pthread_mutex_lock(&m_isChargingHWOoSMtx);
  bool isOoS = m_isChargingHWOoS;
  pthread_mutex_unlock(&m_isChargingHWOoSMtx);
  return isOoS;
}


void EVStateNInputVInterface::setIsChargingHWOoS(bool isOoS)
{
  pthread_mutex_lock(&m_isChargingHWOoSMtx);
  m_isChargingHWOoS = isOoS;
  pthread_mutex_unlock(&m_isChargingHWOoSMtx);
  return;
}


bool EVStateNInputVInterface::getShouldFPGAOn()
{
  pthread_mutex_lock(&m_shouldFPGAOnMtx);
  bool shouldOn = m_shouldFPGAOn;
  pthread_mutex_unlock(&m_shouldFPGAOnMtx);
  return shouldOn;
}


void EVStateNInputVInterface::setShouldFPGAOn(bool shouldOn)
{
  pthread_mutex_lock(&m_shouldFPGAOnMtx);
  m_shouldFPGAOn = shouldOn;
  pthread_mutex_unlock(&m_shouldFPGAOnMtx);
  return;
}
