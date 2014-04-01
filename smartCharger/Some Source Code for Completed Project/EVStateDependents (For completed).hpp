/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  EVStateDependents.hpp

  
*/

#ifndef EVSTATEDEPENDENTS_HPP
#define EVSTATEDEPENDENTS_HPP

#include "VehicleState.hpp"
#include "common.hpp"
#include "Thread.hpp"

#define NORMAL 0
#define PREHEAT 1
#define PREHEATINGTEMPADJUSTMENT

enum heaterState
{
  S0_IDLE = 0,
  S1_QUERIEDTEMP,
  S2_GOT_TEMP,
  S3_TEMPLIMIT_RAISED
}

enum vclMvmState
{
  S0_PARKED_NOT_CHARGING = 0,
  S1_PARKED_CHARGING,
  S2_CRUSING,
};


class EVStateDependents: public Thread
{
public:
  EVStateDependents();
  ~EVStateDependents();
  int init();
  void* run();

private:
  // Members
  VehicleState* m_pVS_evState;
  vclMvmState m_vmState;
  int m_vmRowID;
  float m_lastTravelledDist;
  float m_distPerSoC;
  bool m_isFaultFromStateFile;
  I2CWrapper* m_pI2C_fpga;

  GPIOWrapper* m_pGPIO_heater;
  heaterState m_heaterState;
  bool m_isHeaterOoS;
  bool m_isHeaterOn;
  int m_tempthrshld1;
  int m_tempthrshld2;
  time_t m_lastTempQueryTime;

  m_nextDrivingTime;
  chronoTP m_tp_curTime;

  // methods
  void vclmvmRecordHandle();
  void fpgaCtrl();
  void piggybackInfoNRenameFileWithVclMvm();
  int sqlFindCorrespRowID();
  static int selectCallback(void* data, int argc, char **argv, char **azColName);
  void sqlUpdateCorrespRowID(float td);

  
  void heaterCtrl();
  void setTempLvl(int isPreHeat);
};

#endif // EVSTATEDEPENDENTS_HPP
