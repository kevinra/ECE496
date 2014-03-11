/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.hpp

  
*/

#ifndef EVSTATEDEPENDENTS_HPP
#define EVSTATEDEPENDENTS_HPP

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
  VehicleState m_evState;

  m_nextDrivingTime;
  heaterState m_heaterState;
  bool m_isHeaterOoS;
  bool m_isHeaterOn;
  int m_tempthrshld1 = 5;
  int m_tempthrshld2 = 10;
  GPIOWrapper m_heaterGPIO

  void fpgaCtrl();
  void heaterCtrl();
  void setTempLvl(int isPreHeat);
};


#endif // EVSTATEDEPENDENTS_HPP