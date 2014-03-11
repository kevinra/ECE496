/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  VehicleState.cpp

  
*/


#include "VehicleState.hpp"
#include <string.h>


VehicleState::VehicleState()
{
  m_speedCur = 0;
  m_speedOld = 0;
  m_eStop = FALSE;
  m_pBrake = FALSE;

  m_soc = 0;
  m_vCellMin = 0;
  m_vCellMax = 0;
  m_current = 0;
  m_tempMin = 0;
  m_tempMax = 0;

  m_gearPos = 0;
  m_accelPos = 0;

  m_prev_faultMap = 0;
  m_faultMap = 0;
  m_prev_driveWarning = FALSE;
  m_driveWarning = FALSE;
  m_prev_driveOverTemp = FALSE;
  m_driveOverTemp = FALSE;
  m_prev_driverFault = FALSE;
  m_driverFault = FALSE;
}
VehicleState::~VehicleState()
{
  if (m_ftpHandle)
  {
    curl_easy_cleanup(curl);
    
  }
}


int VehicleState::init()
{
  m_ftpHandle = curl_easy_init();
  if (m_ftpHandle)
  {
    curl_easy_setopt(m_ftpHandle, CURLOPT_URL,
                     "http://192.168.3.201/cgi-bin/state.json");

    // curl_easy_setopt(curl, CURLOPT_URL,
    //                  "http://192.168.1.147/state.json");

    // Define our callback to get called when there's data to be written
    curl_easy_setopt(m_ftpHandle, CURLOPT_WRITEFUNCTION, VehicleState::my_fwrite);

    // Switch on full protocol/debug output
    #ifdef DEBUG
    curl_easy_setopt(m_ftpHandle, CURLOPT_VERBOSE, 1L);
    #endif

    // Set a pointer to our struct to pass to the callback
    curl_easy_setopt(m_ftpHandle, CURLOPT_WRITEDATA, &outFile);
  }
  else
  {
    DBG_ERR_MSG("curl ftp handler initialization failed");
    return 1;
  }
  return 0;
}


int VehicleState::getCurDateNStateFile()
{
  getCurrentTimeStr(timeStr);
  if (timeStr == NULL)
  {
    fprintf(stderr, "snprintf Failed!\n");
    return -1;
  }

  // outFile.fileName = "read.txt";
  outFile.fileName = timeStr;
  outFile.stream = NULL;

  res = curl_easy_perform(curl);

  if(outFile.stream)
  {
    fclose(outFile.stream);
  }
}


int VehicleState::extractData()
{

}


bool VehicleState::getIsParked()
{
  
}


bool VehicleState::getIsErrorPresent()
{
  
}


bool isDifferentError()
{

}


int VehicleState::generateErrorStr(char* pDest)
{
  
}


void printExtractedAttribs()
{

}


// Private methods

void VehicleState::convertStrToBoolean(char* strBoolean)
{
  
}


static size_t VehicleState::my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out=(struct FtpFile *)stream;

  if(out && !out->stream)
  {
    // open file for writing
    out->stream = fopen(out->fileName, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }

  return fwrite(buffer, size, nmemb, out->stream);
}


static void EVStateNInputVInterface::setCurrentTimeStr()
{
  time_t rawtime;
  struct tm* timeInfo;

  time(&rawtime);
  timeInfo = localtime(&rawtime);

  if ( snprintf(m_dateNtimeStr, TIMESTRSIZE, "%d|%.2d|%.2d_%.2d|%.2d|%.2d.txt", 
                timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, 
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec) < 0)
  {
    DBG_ERR_MSG("snprintf error occured!");
    timeStr = NULL;
  }

  return;
}