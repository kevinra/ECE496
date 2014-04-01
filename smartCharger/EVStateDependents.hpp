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
#include "GPIOWrapper.hpp"

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
  int m_driveBeginTimeRowID;
  int m_nextDriving_time;
  int m_nextDriving_day;
  float m_lastTravelledDist;
  float m_distPerSoC;
  bool m_isFaultFromStateFile;
  unsigned int m_inputCurrent;
  #ifdef SIM_IN_CUR
  unsigned int m_simulatedCurrent;
  #endif
  GPIOWrapper* m_pGPIO_bOut_fIn_2;
  GPIOWrapper* m_pGPIO_bOut_fIn_1;
  GPIOWrapper* m_pGPIO_bOut_fIn_0;

  // m_nextDrivingTime;
  chronoTP m_tp_curTime;

  // methods
  void vclmvmRecordHandle();
  void fpgaCtrl();
  void piggybackInfoNRenameFileWithVclMvm();

  int sql_findCorrespRowID();
  static int sql_callBack_findCorrespRowID(void* data, int argc, char **argv, char **azColName);
  void sql_updateCorrespRowID(float td);
  void sql_findNextDrivingInfo();
  static int sql_callBack_findNextDrivingInfo(void* data, int argc, char **argv, char **azColName);

  int calculateInputCurrent(int currentTime, int dayCurrentTime);
  int fpgaSend(int value);
  void getCurTimeInfo(int& day, int& hour, int& min);
};

#endif // EVSTATEDEPENDENTS_HPP
