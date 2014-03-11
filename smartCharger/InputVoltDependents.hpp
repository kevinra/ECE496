/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  InputVoltDependents.hpp

  
*/

#ifndef INPUTVOLTDEPENDENTS_HPP
#define INPUTVOLTDEPENDENTS_HPP

#include "common.hpp"
#include "thread/Thread.hpp"

class InputVoltDependents: public Thread
{
public:
  InputVoltDependents();
  ~InputVoltDependents();
  int init();
  int recNBatRelayOn();
  int recNBatRelayOff();
  void* run();

private:
	bool m_isInputVMeterOoS;
  I2CWrapper m_inputVMeterI2C;
  GPIOWrapper m_fpgaInterruptGPIO = GPIOWrapper(GPIO_FPGAINTERRUPT);
  GPIOWrapper m_rec1LdEnGPIO = GPIOWrapper(GPIO_REC1_LD_ENABLE);
  GPIOWrapper m_rec2LdEnGPIO = GPIOWrapper(GPIO_REC2_LD_ENABLE);
  GPIOWrapper m_rec1PFMGPIO = GPIOWrapper(GPIO_REC1_PFM);
  GPIOWrapper m_rec2PFMGPIO = GPIOWrapper(GPIO_REC2_PFM);

  bool isBothLdEnReady();
  int processInputVMeter();
  inputV translateInputV();
  void handlePFM();
};


#endif // INPUTVOLTDEPENDENTS_HPP