/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  VehicleState.hpp

  
*/

#ifndef VEHICLESTATE_HPP
#define VEHICLESTATE_HPP

#include "common.hpp"
#include <curl/curl.h>
#include <chrono>
#include <ctime>

typedef std::chrono::system_clock::time_point chronoTP;
struct FtpFile
{
  char* fileName;
  FILE* stream;
};

class VehicleState
{
public:
  VehicleState();
  ~VehicleState();

  int init();
  int getCurDateNStateFile(chronoTP& pRawTime);
  int extractData();
  // Acessor to extracted variables
  #ifdef NO_RECT
  int getCurrentFlow();
  #endif
  #ifdef DEBUG
  void printExtractedAttribs();
  #endif
  int getSoC();
  // Accessor to derived variables
  bool getIsParked();
  bool getIsFaultPresent();
  bool getIsSoCDecreased();
  float getTravelledDist();
  char* getStateFileName();
  bool isNewError();
  void generateErrorStr(char* pDest);
  void resetTravelledDist();

private:
  // "general"
  float m_speedOld;
  float m_speedCur;
  bool m_eStop;
  bool m_pBrake;

  // "battery"
  int m_soc;
  int m_vCellMin;
  int m_vCellMax;
  int m_current;
  // These are just mins and maxs of all 4 temperature attributes
  int m_tempMin;
  int m_tempMax;

  // "drive"
  int m_gearPos;
  int m_accelPos;

  // attributes related to errors
  int m_prev_faultMap;
  int m_faultMap;
  bool m_prev_driveWarning;
  bool m_driveWarning;
  bool m_prev_driveOverTemp;
  bool m_driveOverTemp;
  bool m_prev_driveFault;
  bool m_driveFault;

  bool m_isParked;
  bool m_isFaultPresent;
  bool m_isSoCDecreased;
  float m_travelledDist; // in meters

  // FTP related members
  CURL* m_ftpHandle;
  struct FtpFile m_outFile;

  char m_stateFileName[STATEFILE_STRSIZE];
  float m_timeDiff; // in ms 
  chronoTP m_tp_PrevTime;

  bool convertStrToBoolean(char strBoolean[]);
  static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);
  bool setCurrentTimeStr(chronoTP& rChronoTP_curTime);
};

#endif // VEHICLESTATE_HPP
