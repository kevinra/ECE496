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
  int m_vmRowID;
  float m_lastTravelledDist;
  float m_distPerSoC;
  bool m_isFaultFromStateFile;
  GPIOWrapper* m_pGPIO_bOut_fIn_2;
  GPIOWrapper* m_pGPIO_bOut_fIn_1;
  GPIOWrapper* m_pGPIO_bOut_fIn_0;

  // m_nextDrivingTime;
  chronoTP m_tp_curTime;

  // methods
  void vclmvmRecordHandle();
  void fpgaCtrl();
  void piggybackInfoNRenameFileWithVclMvm();
  int sqlFindCorrespRowID();
  static int selectCallback(void* data, int argc, char **argv, char **azColName);
  void sqlUpdateCorrespRowID(float td);
};

#endif // EVSTATEDEPENDENTS_HPP
