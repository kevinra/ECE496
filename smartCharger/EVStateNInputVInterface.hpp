/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.hpp


*/

#ifndef EVStateNInputVInterface_HPP
#define EVStateNInputVInterface_HPP

#include "GPIOWrapper.hpp"
#include <pthread.h>

#define TIMESTRSIZE 30

extern EVStateNInputVInterface g_EVStateNInputVInterface;

enum inputV
{
  inputV_dontCare = 0,
  inputV_120V,
  inputV_240V
}

class EVStateNInputVInterface: public Thread
{
public:
  EVStateNInputVInterface();
  ~EVStateNInputVInterface();

  int init();
  int getInputVolt();
  void setInputVolt(int inputV);
  bool getIsChargingHWOoS();
  void setIsChargingHWOoS(bool isOoS);
  bool getShouldFPGAOn();
  void setShouldFPGAOn(bool shouldOn);

private:
  inputV m_inputVolt;
  bool m_isChargingHwErr;
  bool m_shouldFPGAOn;
  pthread_mutex_t m_inputVoltMtx;
  pthread_mutex_t m_isChargingHWOoSMtx;
  pthread_mutex_t m_shouldFPGAOnMtx;
};

#endif // EVStateNInputVInterface_HPP
