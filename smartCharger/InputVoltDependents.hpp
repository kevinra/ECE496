/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  InputVoltDependents.hpp

  
*/

#ifndef INPUTVOLTDEPENDENTS_HPP
#define INPUTVOLTDEPENDENTS_HPP

#include "common.hpp"
#include "Thread.hpp"
#include "GPIOWrapper.hpp"

class InputVoltDependents: public Thread
{
public:
  InputVoltDependents();
  ~InputVoltDependents();
  int init();
  void* run();

private:
  // Members
  bool m_isInputVMeterOoS;
  GPIOWrapper* m_pGPIO_rec1PfEn;
  GPIOWrapper* m_pGPIO_rec1LdEn;
  GPIOWrapper* m_pGPIO_rec1PFM;
  GPIOWrapper* m_pGPIO_bIn_fOut_2;
  GPIOWrapper* m_pGPIO_bIn_fOut_1;
  GPIOWrapper* m_pGPIO_bIn_fOut_0;

  // Methods
  int recNBatRelayOn();
  int recNBatRelayOff();
  bool isBothLdEnReady();
  void waitForInterrupt();
};

#endif // INPUTVOLTDEPENDENTS_HPP
