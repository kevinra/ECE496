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
  m_isInputVMeterOoS = false;
  m_pI2C_inputVMeter = new I2CWrapper();
  m_pGPIO_fpgaInterrupt = new GPIOWrapper(GPIO_FPGAINTERRUPT);
  m_pGPIO_rec1PfEn = new GPIOWrapper(GPIO_REC1_PF_ENABLE);
  m_pGPIO_rec2PfEn = new GPIOWrapper(GPIO_REC2_PF_ENABLE);
  m_pGPIO_rec1LdEn = new GPIOWrapper(GPIO_REC1_LD_ENABLE);
  m_pGPIO_rec2LdEn = new GPIOWrapper(GPIO_REC2_LD_ENABLE);
  m_pGPIO_rec1PFM = new GPIOWrapper(GPIO_REC1_PFM);
  m_pGPIO_rec2PFM = new GPIOWrapper(GPIO_REC2_PFM);
  m_pGPIO_batRelay = new GPIOWrapper(GPIO_BATRELAY);
}
InputVoltDependents::~InputVoltDependents()
{
  recNBatRelayOff();
  delete m_pI2C_inputVMeter;
  delete m_pGPIO_fpgaInterrupt;
  delete m_pGPIO_rec1PfEn;
  delete m_pGPIO_rec2PfEn;
  delete m_pGPIO_rec1LdEn;
  delete m_pGPIO_rec2LdEn;
  delete m_pGPIO_rec1PFM;
  delete m_pGPIO_rec2PFM;
  delete m_pGPIO_batRelay;
}

// Note that g_EVStateNInputVInterface is initialized by EVState.init() so no need to 
// initialize it here.
int InputVoltDependents::init()
{
  if ( m_pI2C_inputVMeter->init() )
  {
    DBG_ERR_MSG("Input voltmeter I2C Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_fpgaInterrupt->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("FPGA GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec1PfEn->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec2PfEn->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec1LdEn->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("Rectifier_1 LD_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec2LdEn->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("Rectifier_2 LD_Enable GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec1PFM->init(INPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Rectifier_1 PFM GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec2PFM->init(INPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Rectifier_2 PFM GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_batRelay->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Battery relay GPIO Initializing failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  return 0;
}


int InputVoltDependents::recNBatRelayOn()
{
  if ( m_pGPIO_rec1PfEn->gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec2PfEn->gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_batRelay->gpioSet(HIGH) )
  {
    DBG_ERR_MSG("Battery Relay GPIO set HIGH failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  return 0;
}


int InputVoltDependents::recNBatRelayOff()
{
  if ( m_pGPIO_rec1PfEn->gpioSet(LOW) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO set LOW failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec2PfEn->gpioSet(LOW) )
  {
    DBG_ERR_MSG("Rectifier_2 PF_Enable GPIO set LOW failed!");
    g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_batRelay->gpioSet(LOW) )
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
      g_EVStateNInputVInterface.setShouldFPGAon(true);
      m_pGPIO_fpgaInterrupt->gpioWaitForInterrupt();

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
  if ( m_pGPIO_rec1LdEn->gpioGet(&rec1_ldEn) )
  {
    g_errQueue.addErrStr("Reading Rectifier_1 LD_Enable GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
    return false;
  }
  if (rec1_ldEn == 0)
  {
    DBG_OUT_MSG("Rectifier_1 LD_Enable is Low. Waiting for an interrupt");
    if ( m_pGPIO_rec1LdEn->gpioWaitForInterrupt() )
    {
      g_errQueue.addErrStr("Waiting for interrupt on Rectifier_1 LD_Enable GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(true);
      return false;     
    }
  }
  if ( m_pGPIO_rec2LdEn->gpioGet(&rec2_ldEn) )
  {
    g_errQueue.addErrStr("Reading Rectifier_2 LD_Enable GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
    return false;
  }
  if (rec2_ldEn == 0)
  {
    DBG_OUT_MSG("Rectifier_2 LD_Enable is Low. Waiting for an interrupt");
    if ( m_pGPIO_rec2LdEn->gpioWaitForInterrupt() )
    {
      g_errQueue.addErrStr("Waiting for interrupt on Rectifier_2 LD_Enable GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(true);
      return false;
    }
  }
  return true;
}


InputV InputVoltDependents::processInputVMeter()
{
  if ( m_isInputVMeterOoS )
  {
    DBG_OUT_MSG("Input voltmeter is already out of service so just exit.");
    return inputV_120V;
  }

  int readValue;
  if ( m_pI2C_inputVMeter->i2cGet(&readValue) )
  {
    DBG_ERR_MSG("Input voltmeter I2C read failed!");
    g_errQueue.addErrStr("Warning: Input voltmeter is out of service! - cannot be read");
    m_isInputVMeterOoS = true;
    return inputV_120V;
  }

  inputV iv =  translateInputV(readValue);
  if (iv == inputV_dontCare)
  {
    DBG_ERR_MSG("Input voltmeter reading out of expected range!");
    g_errQueue.addErrStr("Warning: Input voltmeter is out of service! - gives wrong reading");
    m_isInputVMeterOoS = true;
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
  if ( m_pGPIO_rec1PFM->gpioGet(&rec1) )
  {
    g_errQueue.addErrStr("Reading Rectifier_1 PFM GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
    return;
  }
  if (rec1 == 0)
  {
    if ( m_pGPIO_rec2PFM->gpioGet(&rec2) )
    {
      g_errQueue.addErrStr("Reading Rectifier_2 PFM GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(true);
      return;
    }
    if (rec2 == 0)
    {
      return;
    }
    else
    {
      g_errQueue.addErrStr("FPGA Error Detected!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(true);
      return;
    }
  }
  else
  {
    g_errQueue.addErrStr("FPGA Error Detected!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
    return;
  }
  return;
}




