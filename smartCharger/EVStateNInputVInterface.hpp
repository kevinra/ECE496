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
  bool getIsVoltmeterOoS();
  void setIsVoltmeterOoS(bool isOoS);
  bool getIsRectNBatRelayOoS();
  void setIsRectNBatRelayOoS(bool isOoS);
  bool getIsErrorFromStateFile();
  void setIsErrorFromStateFile(bool isErr);
  int rectNbatRelayOn_withNonZeroInputVoltmeter();
  int rectNbatRelayOn_withInputVoltmeterOoS();
  int rectNbatRelayOff();

private:
  char m_dateNtimeStr[TIMESTRSIZE];


  int m_inputVolt;
  bool m_isVoltmeterOoS;
  bool m_isRectNBatRelayOoS;
  bool m_isErrorFromStateFile;
  pthread_mutex_t m_inputVoltMtx;
  pthread_mutex_t m_isVoltmeterOoSMtx;
  pthread_mutex_t m_isRectNBatRelayOoSMtx;
  pthread_mutex_t m_isErrorFromStateFileMtx;
  pthread_mutex_t m_recNbatMtx;
  GPIOWrapper rec1_PF_Enable;
  GPIOWrapper rec2_PF_Enable;
  GPIOWrapper rec1_LD_Enable;
  GPIOWrapper rec2_LD_Enable;
  GPIOWrapper batteryRelay;
};

#endif // EVStateNInputVInterface_HPP

