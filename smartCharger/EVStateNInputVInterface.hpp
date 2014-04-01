/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.hpp


*/

#ifndef EVStateNInputVInterface_HPP
#define EVStateNInputVInterface_HPP

#include "GPIOWrapper.hpp"
#include <pthread.h>

class EVStateNInputVInterface
{
public:
  EVStateNInputVInterface();
  ~EVStateNInputVInterface();

  int init();
  bool getIsChargingHWOoS();
  void setIsChargingHWOoS(bool isOoS);
  bool getShouldLLCOn();
  void setShouldLLCOn(bool shouldOn);

private:
  bool m_isChargingHWOoS;
  bool m_shouldFPGAOn;
  pthread_mutex_t m_isChargingHWOoSMtx;
  pthread_mutex_t m_shouldFPGAOnMtx;
};

#endif // EVStateNInputVInterface_HPP

extern EVStateNInputVInterface g_EVStateNInputVInterface;

