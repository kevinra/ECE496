/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  VehicleState.hpp

  
*/

#ifndef VEHICLESTATE_HPP
#define VEHICLESTATE_HPP

#include "common.h"
#include <curl/curl.h>
#include <time.h>

class VehicleState
{
public:
  VehicleState();
  ~VehicleState();

  int init();
  int getCurDateNStateFile(time_t* pRawTime);
  int extractData();
  bool getIsParked();
  bool getIsErrorPresent();
  bool isDifferentError();
  void generateErrorStr(char* pDest);
  void resetTravelledDist();
  #ifdef DEBUG
  void printExtractedAttribs();
  #endif

private:
  // "general"
  double m_speedOld;
  double m_speedCur;
  bool m_eStop;
  bool m_pBrake;

  // "battery"
  int m_socOld;
  int m_socCur;
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
  bool m_prev_driverFault;
  bool m_driverFault;

  int m_isParked;
  bool m_isFaultPresent;
  double m_travelledDist;

  // FTP related members
  CURL* m_ftpHandle;
  CURLcode m_res;
  struct FtpFile m_outFile;

  char m_dateNtimeStr[TIMESTRSIZE];
  time_t m_rawPrevTim;

  bool convertStrToBoolean(char* strBoolean);
  static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream);
  static void setCurrentTimeStr();
};

#endif // VEHICLESTATE_HPP
