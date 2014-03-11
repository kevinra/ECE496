/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.hpp

  
*/

#ifndef EVSTATEDEPENDENTS_HPP
#define EVSTATEDEPENDENTS_HPP

#include <time.h>
#include "common.hpp"
#include "thread/Thread.hpp"

#define NORMAL 0
#define PREHEAT 1
#define PREHEATINGTEMPADJUSTMENT


class EVStateDependents: public Thread
{
public:
  EVStateDependents();
  ~EVStateDependents();
  int init();
  void* run();

private:
  // Members
  VehicleState m_evState;
  vclmvmState m_vmState;
  int m_vmRecordID;
  heaterState m_heaterState;
  bool m_isHeaterOoS;
  bool m_isHeaterOn;
  bool m_isFaultFromStateFile;
  int m_tempthrshld1;
  int m_tempthrshld2;
  I2CWrapper m_fpgaI2C;
  GPIOWrapper m_heaterGPIO;

  m_nextDrivingTime;
  time_t m_rawCurTime;
  time_t m_lastTempQueryTime;


  // methods
  void vclmvmRecordHandle();
  void fpgaCtrl();
  void heaterCtrl();
  void setTempLvl(int isPreHeat);
};

#endif // EVSTATEDEPENDENTS_HPP
