/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateNInputVInterface.hpp


*/

#ifndef EVStateNInputVInterface_HPP
#define EVStateNInputVInterface_HPP

#define GPIO_REC1_PF_ENABLE 60     // P9_12 = GPIO1_28
#define GPIO_REC1_LD_ENABLE 48     // P9_15 = GPIO1_16
#define GPIO_REC1_PFM 3            // P9_21 = GPIO0_03
#define GPIO_REC2_PF_ENABLE 2      // P9_22 = GPIO2_02
#define GPIO_REC2_LD_ENABLE 49     // P9_23 = GPIO1_28
#define GPIO_REC2_PFM 15           // P9_24 = GPIO0_15
#define GPIO_BATRELAY 115          // P9_27 = GPIO3_19
#define GPIO_FPGAINTERRUPT 112     // P9_30 = GPIO3_16

//#include "GPIOWrapper.hpp"
#include "simpleGPIO.h"
#include <pthread.h>

extern EVStateNInputVInterface g_EVStateNInputVInterface;

class EVStateNInputVInterface: public Thread
{
public:
  EVStateNInputVInterface();
  ~EVStateNInputVInterface();

  int getInputVolt();
  void setInputVolt(int inputV);
  bool getIsVoltmeterOoS();
  void setIsVoltmeterOoS(bool isOoS);
  bool getIsErrorFromStateFile();
  void setIsErrorFromStateFile(bool isErr);
  int rectNbatRelayOn();
  int rectNbatRelayOff();

private:
  int m_inputVolt;
  bool m_isVoltmeterOoS;
  bool m_isErrorFromStateFile;
  pthread_mutex_t m_inputVoltMtx;
  pthread_mutex_t m_isVoltmeterOoSMtx;
  pthread_mutex_t m_isErrorFromStateFileMtx;
  pthread_mutex_t m_recNbatMtx;
  /*
  GPIOWrapper Rec1_PF_Enable;
  GPIOWrapper Rec2_PF_Enable;
  GPIOWrapper Rec1_LD_Enable;
  GPIOWrapper Rec2_LD_Enable;
  GPIOWrapper BatteryRelay;
  */
};

#endif // EVStateNInputVInterface_HPP

