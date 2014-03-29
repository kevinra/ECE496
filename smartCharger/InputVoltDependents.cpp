/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  InputVoltDependents.cpp

  
*/

// <T2>

#include <poll.h>
#include "InputVoltDependents.hpp"
#include "EVStateNInputVInterface.hpp"

#define NUMOFINTERRUPT 4
#define NUMOFPOLLSTRUCT (NUMOFPOLLSTRUCT + 1)
#define INTERRUPTNUM_REC1PFM 1
#define INTERRUPTNUM_BBB_IN_FPGA_OUT_2 2
#define INTERRUPTNUM_BBB_IN_FPGA_OUT_1 3
#define INTERRUPTNUM_BBB_IN_FPGA_OUT_0 4

InputVoltDependents::InputVoltDependents()
{
  m_isInputVMeterOoS = false;
  m_pGPIO_rec1PfEn = new GPIOWrapper(GPIO_REC1_PF_ENABLE);
  m_pGPIO_rec1LdEn = new GPIOWrapper(GPIO_REC1_LD_ENABLE);
  m_pGPIO_rec1PFM = new GPIOWrapper(GPIO_REC1_PFM);
  m_pGPIO_bIn_fOut_2 = new GPIOWrapper(GPIO_BBB_IN_FPGA_OUT_2);
  m_pGPIO_bIn_fOut_1 = new GPIOWrapper(GPIO_BBB_IN_FPGA_OUT_1);
  m_pGPIO_bIn_fOut_0 = new GPIOWrapper(GPIO_BBB_IN_FPGA_OUT_0);
}


InputVoltDependents::~InputVoltDependents()
{
  recNBatRelayOff();
  delete m_pGPIO_rec1PfEn;
  delete m_pGPIO_rec1LdEn;
  delete m_pGPIO_rec1PFM;
  delete m_pGPIO_bIn_fOut_2;
  delete m_pGPIO_bIn_fOut_1;
  delete m_pGPIO_bIn_fOut_0;
}


// Note that g_EVStateNInputVInterface is initialized by the main
// so no need to initialize it here.
int InputVoltDependents::init()
{
  if ( m_pGPIO_rec1PfEn->init(OUTPUT_PIN, NULL, false) )
  {
    DBG_ERR_MSG("Rectifier_1 PF_Enable GPIO Initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec1LdEn->init(INPUT_PIN, STR_RISING_EDGE, false) )
  {
    DBG_ERR_MSG("Rectifier_1 LD_Enable GPIO Initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_rec1PFM->init(INPUT_PIN, STR_FALLING_EDGE, true) )
  {
    DBG_ERR_MSG("Rectifier_1 PFM GPIO Initializing failed!");
    // g_errQueue.addErrStr("");
    return 1;
  }
  if ( m_pGPIO_bIn_fOut_2->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("Beaglebone input from FPGA out pin #2 initializing failed!");
     // g_errQueue.addErrStr("");
  }
  if ( m_pGPIO_bIn_fOut_1->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("Beaglebone input from FPGA out pin #1 initializing failed!");
     // g_errQueue.addErrStr("");
  }
  if ( m_pGPIO_bIn_fOut_0->init(INPUT_PIN, STR_RISING_EDGE, true) )
  {
    DBG_ERR_MSG("Beaglebone input from FPGA out pin #0 initializing failed!");
     // g_errQueue.addErrStr("");
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
  return 0;
}


void* InputVoltDependents::run()
{
  recNBatRelayOn();
  while ( !g_EVStateNInputVInterface.getIsChargingHWOoS() )
  {
    if ( isBothLdEnReady() )
    {
      g_EVStateNInputVInterface.setInputVolt( processInputVMeter() );
      g_EVStateNInputVInterface.setShouldFPGAon(true);
      waitForInterrupt();
    }
  }
  DBG_ERR_MSG("Error occurred, so thread(InputVoltDependents) returns!");
  recNBatRelayOff();
  return NULL;
}


bool InputVoltDependents::isBothLdEnReady()
{
  int rec1_ldEn;
  if ( m_pGPIO_rec1LdEn->gpioRead(&rec1_ldEn) )
  {
    DBG_ERR_MSG("Reading Rectifier_1 LD_Enable GPIO failed!");
    // g_errQueue.addErrStr("Reading Rectifier_1 LD_Enable GPIO failed!");
    g_EVStateNInputVInterface.setIsChargingHWOoS(true);
    return false;
  }
  if (rec1_ldEn == 0)
  {
    DBG_OUT_MSG("Rectifier_1 LD_Enable is Low. Waiting for an interrupt");
    if ( m_pGPIO_rec1LdEn->gpioWaitForInterrupt() )
    {
      DBG_ERR_MSG("Waiting for interrupt on Rectifier_1 LD_Enable GPIO failed!");
      // g_errQueue.addErrStr("Waiting for interrupt on Rectifier_1 LD_Enable GPIO failed!");
      g_EVStateNInputVInterface.setIsChargingHWOoS(true);
      return false;     
    }
  }
  return true;
}


void InputVoltDependents::waitForInterrupt()
{
  struct pollfd fdset[NUMOFPOLLSTRUCT];
  int rc;
  char *buf[MAX_BUF];
 
  memset( (void*)fdset, 0, sizeof(fdset) );

  fdset[0].fd = STDIN_FILENO;
  fdset[0].events = POLLIN;
  fdset[1].fd = m_pGPIO_rec1PFM->gpioGetFd();
  fdset[1].events = POLLPRI;
  fdset[2].fd = m_pGPIO_bIn_fOut_2->gpioGetFd();
  fdset[2].events = POLLPRI;
  fdset[3].fd = m_pGPIO_bIn_fOut_1->gpioGetFd();
  fdset[3].events = POLLPRI;
  fdset[4].fd = m_pGPIO_bIn_fOut_0->gpioGetFd();
  fdset[4].events = POLLPRI;

  rc = poll(fdset, NUMOFPOLLSTRUCT, POLL_TIMEOUT);
  if (rc <= 0)
  {
    DBG_ERR_MSG("poll() on InputVoltDependents GPIO pins failed!");
    // g_errQueue.addErrStr("");
    return -1;
  }
  // 3 bit lines 0'bXXX is used to identify failing LLC.
  int failedLLCNum = 0;
  if (fdset[INTERRUPTNUM_REC1PFM].revents & POLLPRI)
  {
    int len = read(fdset[INTERRUPTNUM_REC1PFM].fd, buf, MAX_BUF);
    DBG_OUT_MSG("Interrupt on Rectifier_1 PFM occurred.");
    g_EVStateNInputVInterface.setShouldFPGAon(false);
  }
  if (fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_2].revents & POLLPRI)
  {
    int len = read(fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_2].fd, buf, MAX_BUF);
    DBG_OUT_MSG("Interrupt on Beaglebone input FPGA out #2 in occurred.");
    failedLLCNum += 4;
    g_EVStateNInputVInterface.setShouldFPGAon(false);
  }
  if (fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_1].revents & POLLPRI)
  {
    int len = read(fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_1].fd, buf, MAX_BUF);
    DBG_OUT_MSG("Interrupt on Beaglebone input FPGA out #1 in occurred.");
    failedLLCNum += 2;
    g_EVStateNInputVInterface.setShouldFPGAon(false);
  }
  if (fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_0].revents & POLLPRI)
  {
    int len = read(fdset[INTERRUPTNUM_BBB_IN_FPGA_OUT_0].fd, buf, MAX_BUF);
    DBG_OUT_MSG("Interrupt on Beaglebone input FPGA out #0 in occurred!");
    failedLLCNum += 1;
    g_EVStateNInputVInterface.setShouldFPGAon(false);
  }
  DBG_OUT_MSG(failedLLCNum);
  // If any of LLC is failed, upload to the server.
  if (failedLLCNum)
  {
    // g_errQueue.addErrStr("LLC #" << failedLLCNum << "failed.\n");
  }
  return 0;
}


