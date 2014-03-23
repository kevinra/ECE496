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
  // Members
	bool m_isInputVMeterOoS;
  I2CWrapper* m_pI2C_inputVMeter;
  GPIOWrapper* m_pGPIO_fpgaInterrupt;
  GPIOWrapper* m_pGPIO_rec1PfEn;
  GPIOWrapper* m_pGPIO_rec2PfEn;
  GPIOWrapper* m_pGPIO_rec1LdEn;
  GPIOWrapper* m_pGPIO_rec2LdEn;
  GPIOWrapper* m_pGPIO_rec1PFM;
  GPIOWrapper* m_pGPIO_rec2PFM;
  GPIOWrapper* m_pGPIO_batRelay;

  // Methods
  bool isBothLdEnReady();
  int processInputVMeter();
  inputV translateInputV();
  void handlePFM();
};


#endif // INPUTVOLTDEPENDENTS_HPP
