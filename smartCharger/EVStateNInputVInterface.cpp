/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.cpp

  
*/

#include "EVStateNInputVInterface.hpp"

#define TIMEOUTCOUNTERLIMIT 1000

// global object
EVStateNInputVInterface g_EVStateNInputVInterface;

EVStateNInputVInterface::EVStateNInputVInterface()
{
  m_inputVolt = 0;
  m_isVoltmeterOoS = FALSE;
  m_isRectNBatRelayOoS = FALSE;
  m_isErrorFromStateFile = FALSE;
  rec1_PF_Enable.setGPIONum(GPIO_REC1_PF_ENABLE);
  rec2_PF_Enable.setGPIONum(GPIO_REC2_PF_ENABLE);
  rec1_LD_Enable.setGPIONum(GPIO_REC1_LD_ENABLE);
  rec2_LD_Enable.setGPIONum(GPIO_REC2_LD_ENABLE);
  batteryRelay.setGPIONum(GPIO_BATRELAY);
}


EVStateNInputVInterface::~EVStateNInputVInterface();
{
  pthread_mutex_destroy(&m_inputVoltMtx, NULL);
  pthread_mutex_destroy(&m_isVoltmeterOoSMtx, NULL);
  pthread_mutex_destroy(&m_isRectNBatRelayOoSMtx, NULL);
  pthread_mutex_destroy(&m_isErrorFromStateFileMtx, NULL);
  pthread_mutex_destroy(&m_recNbatMtx, NULL);
}


int EVStateNInputVInterface::init()
{
  // Initialize mutexs
  if ( pthread_mutex_init(&m_inputVoltMtx, NULL) )
  {
    ERR_MSG("m_inputVoltMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_isVoltmeterOoSMtx, NULL) )
  {
    ERR_MSG("m_isVoltmeterOoSMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_isRectNBatRelayOoSMtx, NULL) )
  {
    ERR_MSG("m_isVoltmeterOoSMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_isErrorFromStateFileMtx, NULL) )
  {
    ERR_MSG("m_isErrorFromStateFileMtx initialization failed!");
    return 1;
  }
  if ( pthread_mutex_init(&m_recNbatMtx, NULL) )
  {
    ERR_MSG("m_recNbatMtx initialization failed!");
    return 1;
  }

  // Initialize GPIOWrappers
  if ( rec1_PF_Enable.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    ERR_MSG("Rectifier_1 PF_Enable GPIO initialization failed!");
    return 1;
  }
  if ( rec2_PF_Enable.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    ERR_MSG("Rectifier_2 PF_Enable GPIO initialization failed!");
    return 1;
  }
  if ( rec1_LD_Enable.gpioInit(INPUT_PIN, NULL, FALSE) )
  {
    ERR_MSG("Rectifier_1 LD_Enable GPIO initialization failed!");
    return 1;
  }
  if ( rec2_LD_Enable.gpioInit(INPUT_PIN, NULL, FALSE) )
  {
    ERR_MSG("Rectifier_2 LD_Enable GPIO initialization failed!");
    return 1;
  }
  if ( batteryRelay.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    ERR_MSG("Battery Relay GPIO initialization failed!");
    return 1;
  }

  return 0;
}


int EVStateNInputVInterface::getInputVolt()
{
  pthread_mutex_lock(&m_inputVoltMtx);
  int inputV = m_inputVolt;
  pthread_mutex_unlock(&m_inputVoltMtx);
  return inputV;
}


void EVStateNInputVInterface::setInputVolt(int inputV)
{
  pthread_mutex_lock(&m_inputVoltMtx);
  m_inputVolt = inputV;
  pthread_mutex_unlock(&m_inputVoltMtx);
  return;
}


bool EVStateNInputVInterface::getIsVoltmeterOoS()
{
  pthread_mutex_lock(&m_isVoltmeterOoSMtx);
  bool isOoS = m_isVoltmeterOoS;
  pthread_mutex_unlock(&m_isVoltmeterOoSMtx);
  return isOoS;
}


void EVStateNInputVInterface::setIsVoltmeterOoS(bool isOoS)
{
  pthread_mutex_lock(&m_isVoltmeterOoSMtx);
  m_isVoltmeterOoS = isOoS;
  pthread_mutex_unlock(&m_isVoltmeterOoSMtx);
  return;
}


bool EVStateNInputVInterface::getIsRectNBatRelayOoS()
{
  pthread_mutex_lock(&m_isRectNBatRelayOoSMtx);
  bool isOoS = m_isRectNBatRelayOoS;
  pthread_mutex_unlock(&m_isRectNBatRelayOoSMtx);
  return isOoS;
}


void EVStateNInputVInterface::setIsRectNBatRelayOoS(bool isOoS)
{
  pthread_mutex_lock(&m_isRectNBatRelayOoSMtx);
  m_isRectNBatRelayOoS = isOoS;
  pthread_mutex_unlock(&m_isRectNBatRelayOoSMtx);
  return;
}


bool EVStateNInputVInterface::getIsErrorFromStateFile()
{
  pthread_mutex_lock(&m_isErrorFromStateFileMtx);
  bool isErr = m_isErrorFromStateFile;
  pthread_mutex_unlock(&m_isErrorFromStateFileMtx);
  return isErr;
}


void EVStateNInputVInterface::setIsErrorFromStateFile()
{
  pthread_mutex_lock(&m_isErrorFromStateFileMtx);
  bool isErr = m_isErrorFromStateFile;
  pthread_mutex_unlock(&m_isErrorFromStateFileMtx);
  return;
}


int EVStateNInputVInterface::rectNbatRelayOn_withNonZeroInputVoltmeter()
{
  pthread_mutex_lock(&m_recNbatMtx);
  int gpioVal = 0;

  if ( rec1_PF_Enable.gpioSet(HIGH) )
  {
    ERR_MSG("Rectifier_1 PF_Enable GPIO set failed!");
    return 1;
  }
  if ( rec2_PF_Enable.gpioSet(HIGH) )
  {
    ERR_MSG("Rectifier_2 PF_Enable GPIO set failed!");
    return 1;
  }
  for (int timeOutCounter = 0; (gpioVal == 0 || timeOutCounter < TIMEOUTCOUNTERLIMIT); timeOutCounter++)
  {
    if ( rec1_LD_Enable.gpioGet(&gpioVal) )
    {
      ERR_MSG("Rectifier_1 LD_Enable GPIO get failed!");
      return 1;   
    }
    DBG_OUT_MSG("Polling on Rectifier_1 LD_Enable...");
  }

  gpioVal = 0;
  for (int timeOutCounter = 0; (gpioVal == 0 || timeOutCounter < TIMEOUTCOUNTERLIMIT); timeOutCounter++)
  {
    if ( rec2_LD_Enable.gpioGet(&gpioVal) )
    {
      ERR_MSG("Rectifier_2 LD_Enable GPIO get failed!");
      return 1;
    }
    DBG_OUT_MSG("Polling on Rectifier_2 LD_Enable...");
  }
  if ( batteryRelay.gpioSet(HIGH) )
  {
    ERR_MSG("Battery Relay GPIO set failed!");
    return 1;
  }

  pthread_mutex_unlock(&m_recNbatMtx);
  return 0;
}


int EVStateNInputVInterface::rectNbatRelayOn_withInputVoltmeterOoS()
{
  pthread_mutex_lock(&m_recNbatMtx);
  int gpioVal = 0;

  if ( rec1_PF_Enable.gpioSet(HIGH) )
  {
    ERR_MSG("Rectifier_1 PF_Enable GPIO set failed!");
    return 1;
  }
  if ( rec2_PF_Enable.gpioSet(HIGH) )
  {
    ERR_MSG("Rectifier_2 PF_Enable GPIO set failed!");
    return 1;
  }
  for (int timeOutCounter = 0; (gpioVal == 0 || timeOutCounter < TIMEOUTCOUNTERLIMIT); timeOutCounter++)
  {
    if ( rec1_LD_Enable.gpioGet(&gpioVal) )
    {
      ERR_MSG("Rectifier_1 LD_Enable GPIO get failed!");
      return 1;   
    }
    DBG_OUT_MSG("Polling on Rectifier_1 LD_Enable...");
  }

  gpioVal = 0;
  for (int timeOutCounter = 0; (gpioVal == 0 || timeOutCounter < TIMEOUTCOUNTERLIMIT); timeOutCounter++)
  {
    if ( rec2_LD_Enable.gpioGet(&gpioVal) )
    {
      ERR_MSG("Rectifier_2 LD_Enable GPIO get failed!");
      return 1;
    }
    DBG_OUT_MSG("Polling on Rectifier_2 LD_Enable...");
  }
  if ( batteryRelay.gpioSet(HIGH) )
  {
    ERR_MSG("Battery Relay GPIO set failed!");
    return 1;
  }

  pthread_mutex_unlock(&m_recNbatMtx);
  return 0;
}

int EVStateNInputVInterface::rectNbatRelayOff()
{
  pthread_mutex_lock(&m_recNbatMtx);

  gpio_set_value(GPIO_REC1_PF_ENABLE, LOW);
  gpio_set_value(GPIO_REC2_PF_ENABLE, LOW);
  gpio_set_value(GPIO_BATRELAY, LOW);

  pthread_mutex_unlock(&m_recNbatMtx);
}





