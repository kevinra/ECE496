/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.cpp

  
*/

#include "SimpleGPIO.h"
#include "EVStateNInputVInterface.hpp"

// global object
EVStateNInputVInterface g_EVStateNInputVInterface;

EVStateNInputVInterface::EVStateNInputVInterface()
{
  m_inputVolt = 0;
  m_isVoltmeterOoS = FALSE;
  m_isErrorFromStateFile = FALSE;
  pthread_mutex_init(&m_inputVoltMtx, NULL);
  pthread_mutex_init(&m_isVoltmeterOoSMtx, NULL);
  pthread_mutex_init(&m_isErrorFromStateFil  Mtx, NULL);
  pthread_mutex_init(&m_recNbatMtx, NULL);

  gpio_export(GPIO_REC1_PF_ENABLE);
  gpio_set_dir(GPIO_REC1_PF_ENABLE, OUTPUT_PIN);
  gpio_export(GPIO_REC1_LD_ENABLE);
  gpio_set_dir(GPIO_REC1_LD_ENABLE, INPUT_PIN);
  gpio_export(GPIO_REC1_PFM);
  gpio_set_dir(GPIO_REC1_PFM, INPUT_PIN);
  gpio_export(GPIO_REC2_PF_ENABLE);
  gpio_set_dir(GPIO_REC2_PF_ENABLE, OUTPUT_PIN);
  gpio_export(GPIO_REC2_LD_ENABLE);
  gpio_set_dir(GPIO_REC2_LD_ENABLE, INPUT_PIN);
  gpio_export(GPIO_REC2_PFM);
  gpio_set_dir(GPIO_REC2_PFM, INPUT_PIN);
  gpio_export(GPIO_BATRELAY);
  gpio_set_dir(GPIO_BATRELAY, OUTPUT_PIN);
  gpio_export(GPIO_FPGAINTERRUPT);
  gpio_set_dir(GPIO_FPGAINTERRUPT, INPUT_PIN);
  gpio_set_edge(GPIO_FPGAINTERRUPT, "rising");
}


EVStateNInputVInterface::~EVStateNInputVInterface();
{
  pthread_mutex_destroy(&m_inputVoltMtx, NULL);
  pthread_mutex_destroy(&m_isVoltmeterOoSMtx, NULL);
  pthread_mutex_destroy(&m_isErrorFromStateFileMtx, NULL);
  pthread_mutex_destroy(&m_recNbatMtx, NULL);
  gpio_unexport(GPIO_REC1_PF_ENABLE);
  gpio_unexport(GPIO_REC1_LD_ENABLE);
  gpio_unexport(GPIO_REC1_PFM);
  gpio_unexport(GPIO_REC2_PF_ENABLE);
  gpio_unexport(GPIO_REC2_LD_ENABLE);
  gpio_unexport(GPIO_REC2_PFM);
  gpio_unexport(GPIO_HEATER);
  gpio_unexport(GPIO_BATRELAY);
  gpio_unexport(GPIO_FPGAINTERRUPT);
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
}


int EVStateNInputVInterface::rectNbatRelayOn()
{
  pthread_mutex_lock(&m_recNbatMtx);

  gpio_set_value(GPIO_REC1_PF_ENABLE, HIGH);
  gpio_set_value(GPIO_REC2_PF_ENABLE, HIGH);
  while( gpio_get_value(GPIO_REC1_LD_ENABLE) ) {}
  while( gpio_get_value(GPIO_REC12_LD_ENABLE) ) {}
  gpio_set_value(GPIO_BATRELAY, HIGH);

  pthread_mutex_unlock(&m_recNbatMtx);
}


int EVStateNInputVInterface::rectNbatRelayOff()
{
  pthread_mutex_lock(&m_recNbatMtx);

  gpio_set_value(GPIO_REC1_PF_ENABLE, LOW);
  gpio_set_value(GPIO_REC2_PF_ENABLE, LOW);
  gpio_set_value(GPIO_BATRELAY, LOW);

  pthread_mutex_unlock(&m_recNbatMtx);
}





