/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  InputVoltDependents.cpp

  
*/

// <T2>

#include <poll.h>
#include "GPIOWrapper.hpp"
#include "InputVoltDependents.hpp"
#include "EVStateNInputVInterface.hpp"

#define INPUTVOLT_120VRANGE_1 
#define INPUTVOLT_120VRANGE_2 
#define INPUTVOLT_240VRANGE_1 
#define INPUTVOLT_240VRANGE_2 

InputVoltDependents::InputVoltDependents()
{
  m_isInputVMeterOoS = FALSE;
  m_inputVMeterI2C = new I2CWrapper();
  m_fpgaInterruptGPIO = new GPIOWrapper(GPIO_FPGAINTERRUPT);
  m_rec1PfEnGPIO = new GPIOWrapper(GPIO_REC1_PF_ENABLE);
  m_rec2PfEnGPIO = new GPIOWrapper(GPIO_REC2_PF_ENABLE);
  m_rec1LdEnGPIO = new GPIOWrapper(GPIO_REC1_LD_ENABLE);
  m_rec2LdEnGPIO = new GPIOWrapper(GPIO_REC2_LD_ENABLE);
  m_rec1PFMGPIO = new GPIOWrapper(GPIO_REC1_PFM);
  m_rec2PFMGPIO = new GPIOWrapper(GPIO_REC2_PFM);
  m_batRelayGPIO = new GPIOWrapper(GPIO_BATRELAY);
}
InputVoltDependents::~InputVoltDependents()
{
  recNBatRelayOff();
  delete m_inputVMeterI2C;
  delete m_fpgaInterruptGPIO;
  delete m_rec1PfEnGPIO;
  delete m_rec2PfEnGPIO;
  delete m_rec1LdEnGPIO;
  delete m_rec2LdEnGPIO;
  delete m_rec1PFMGPIO;
  delete m_rec2PFMGPIO;
  delete m_batRelayGPIO;
}

int InputVoltDependents::init()
{
  if ( m_inputVMeterI2C.i2cInit() )
  {
    DBG_ERR_MSG("Input voltmeter I2C Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_fpgaInterruptGPIO.gpioInit(INPUT_PIN, STR_RISING_EDGE, TRUE) )
  {
    DBG_ERR_MSG("FPGA GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec1PfEnGPIO.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec2PfEnGPIO.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec1LdEnGPIO.gpioInit(INPUT_PIN, STR_RISING_EDGE, TRUE) )
  {
    DBG_ERR_MSG("Rectifier_1 LD_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec2LdEnGPIO.gpioInit(INPUT_PIN, STR_RISING_EDGE, TRUE) )
  {
    DBG_ERR_MSG("Rectifier_2 LD_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec1PFMGPIO.gpioInit(INPUT_PIN, NULL, FALSE) )
  {
    DBG_ERR_MSG("Rectifier_1 PFM GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_rec2PFMGPIO.gpioInit(INPUT_PIN, NULL, FALSE) )
  {
    DBG_ERR_MSG("Rectifier_2 PFM GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_batRelayGPIO.gpioInit(OUTPUT_PIN, NULL, FALSE) )
  {
    DBG_ERR_MSG("Battery relay GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  return 0;
}


int InputVoltDependents::recNBatRelayOn()
{
  if ( rec1_PF_Enable.gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( rec2_PF_Enable.gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( batteryRelay.gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Battery Relay GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  return 0;
}


int InputVoltDependents::recNBatRelayOff()
{
  if ( rec1_PF_Enable.gpioSet(LOW) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO set LOW failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( rec2_PF_Enable.gpioSet(LOW) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO set LOW failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( batteryRelay.gpioSet(LOW) )
  {
    DBG_ERR_MSG("Battery Relay GPIO set LOW failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  return 0;
}


void* InputVoltDependents::run()
{
  while ( !g_EVStateNInputVInterface.getIsChargingHWOoS() )
  {
    if ( isBothLdEnReady() )
    {
      g_EVStateNInputVInterface.setInputVolt( processInputVMeter() );
      g_EVStateNInputVInterface.setShouldFPGAon(TRUE);
      m_fpgaInterruptGPIO.gpioWaitForInterrupt();

      // Upon FPGA interrupt, check whether it's disconnection from
      // the grid or actual error from FPGA. If both PFM are LOW,
      // it's disconnection from the grid. Otherwise, it's an error.
      // Alternatively, we can read both LD_Enable
      handlePFM();
    }
  }
  DBG_ERR_MSG("Error occurred, so thread(InputVoltDependents) returns!");
  recNBatRelayOff();
  return NULL;
}


bool InputVoltDependents::isBothLdEnReady()
{
  int rec1_ldEn, rec2_ldEn;
  if ( m_rec1LdEnGPIO.gpioGet(&rec1_ldEn) )
  {
    g_errQueue.addErrStr("Reading Rectifier_1 LD_Enable GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
    return FALSE;
  }
  if (rec1_ldEn == 0)
  {
    if ( m_rec1LdEnGPIO.gpioWaitForInterrupt() )
    {
      g_errQueue.addErrStr("Waiting for interrupt on Rectifier_1 LD_Enable GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
      return FALSE;     
    }
  }
  if ( m_rec2LdEnGPIO.gpioGet(&rec2_ldEn) )
  {
    g_errQueue.addErrStr("Reading Rectifier_2 LD_Enable GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
    return FALSE;
  }
  if (rec2_ldEn == 0)
  {
    if ( m_rec2LdEnGPIO.gpioWaitForInterrupt() )
    {
      g_errQueue.addErrStr("Waiting for interrupt on Rectifier_2 LD_Enable GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
      return FALSE;
    }
  }
  return TRUE;
}


InputV InputVoltDependents::processInputVMeter()
{
  if ( m_isInputVMeterOoS )
  {
    DBG_OUT_MSG("Input voltmeter is already out of service so just exit.");
    return inputV_120V;
  }

  int readValue;
  if ( m_inputVMeterI2C.i2cGet(&readValue) )
  {
    DBG_ERR_MSG("Input voltmeter I2C read failed!");
    g_errQueue.addErrStr("Warning: Input voltmeter is out of service! - cannot be read");
    m_isInputVMeterOoS = TRUE;
    return inputV_120V;
  }

  inputV iv =  translateInputV(readValue);
  if (iv == inputV_dontCare)
  {
    DBG_ERR_MSG("Input voltmeter reading out of expected range!");
    g_errQueue.addErrStr("Warning: Input voltmeter is out of service! - gives wrong reading");
    m_isInputVMeterOoS = TRUE;
    return inputV_120V;
  }
  return iv;
}


inputV InputVoltDependents::translateInputV(int readVal)
{
  if (readVal > INPUTVOLT_120VRANGE_1 && readVal < INPUTVOLT_120VRANGE_2)
    return inputV_120V;
  else if (readVal > INPUTVOLT_240VRANGE_1 && readVal < INPUTVOLT_240VRANGE_2)
    return inputV_240V;
  return inputV_dontCare;
}


void InputVoltDependents::handlePFM()
{
  if ( m_rec1PFMGPIO.gpioGet(&rec1) )
  {
    g_errQueue.addErrStr("Reading Rectifier_1 PFM GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
    return;
  }
  if (rec1 == 0)
  {
    if ( m_rec2PFMGPIO.gpioGet(&rec2) )
    {
      g_errQueue.addErrStr("Reading Rectifier_2 PFM GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
      return;
    }
    if (rec2 == 0)
    {
      return;
    }
    else
    {
      g_errQueue.addErrStr("FPGA Error Detected!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
      return;
    }
  }
  else
  {
    g_errQueue.addErrStr("FPGA Error Detected!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(TRUE);
    return;
  }
  return;
}




