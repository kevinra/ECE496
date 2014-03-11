/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  VehicleState.cpp

  
*/


#include "VehicleState.hpp"
#include <string.h>

#define POS_SPEED 3
#define POS_ESTOP 6
#define POS_PBRAKE 7
#define POS_SOC 15
#define POS_FAULTMAP 18
#define POS_VCELLMIN 21
#define POS_VCELLMAX 22
#define POS_CURRENT 25
#define POS_TEMP_1 29
#define POS_TEMP_2 30
#define POS_TEMP_3 31
#define POS_TEMP_4 32
#define POS_DRIVEWARNING 231
#define POS_DRIVEOVERTEMP 232
#define POS_DRIVEFAULT 233
#define POS_GEARPOS 238
#define POS_ACCELPOS 245

#define INITIALMINTEMPVAL 100
#define INITIALMAXTEMPVAL -90

#define LINESIZE 70
#define ATTRIBSIZE 20
#define BOOLSTRSIZE 8

#define GEARPOS_NEUTRAL 2

#define BOOLEANSTR_TRUE "true,"
#define BOOLEANSTR_FALSE "false,"

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
  m_isParked = TRUE;
  m_isFaultPresent = FALSE;
  m_travelledDist = 0;
}


VehicleState::~VehicleState()
{
  if (m_ftpHandle)
  {
    curl_easy_cleanup(m_ftpHandle); 
  }
}


int VehicleState::init()
{
  DBG_OUT_MSG("");
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
    curl_easy_setopt(m_ftpHandle, CURLOPT_WRITEDATA, &m_outFile);
  }
  else
  {
    DBG_ERR_MSG("curl ftp handler initialization failed");
    return 1;
  }
  return 0;
}


int VehicleState::getCurDateNStateFile(time_t& rRawTime)
{
  DBG_OUT_MSG("");
  if ( setCurrentTimeStr(rRawTime) )
  {
    return 1;
  }
  outFile.fileName = m_dateNtimeStr;
  outFile.stream = NULL;

  res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    DBG_ERR_MSG("Getting state.json failed. curl told us %d\n", res);
    return 1;
  }

  // If there is content to write to the file
  if (outFile.stream)
  {
    fclose(outFile.stream);
  }
  else
  {
    DBG_ERR_MSG("ERROR: state.json content is empty");
    return 1;
  }
}


int VehicleState::extractData()
{
  FILE *stateFile;
  char line[LINESIZE];
  m_tempMin = INITIALMINTEMPVAL;
  m_tempMax = INITIALMAXTEMPVAL;

  stateFile = fopen("state.json" , "r");
  if(stateFile == NULL)
  {
    DBG_ERR_MSG("Cannot open state.json file\n");
    return -1;
  }

  for (int i = 0; i <= POS_ACCELPOS; i++)
  {
    if ( fgets (line, LINESIZE, stateFile) == NULL )
    {
      DBG_ERR_MSG("The line is NULL where it should not be!!\n");
      return -1;
    }

    char attribute[ATTRIBSIZE];
    char boolStr[BOOLSTRSIZE]
    int value;
    switch(i)
    {
      case POS_SPEED:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_speedOld = m_speedCur;
        m_speedCur = value / 100;
        break;
      case POS_ESTOP:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_eStop = convertStrToBoolean(boolStr);
        break;
      case POS_PBRAKE:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, boolStr);
        m_pBrake = convertStrToBoolean(boolStr);
        break;
      case POS_SOC:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_soc = value;
        break;
      case POS_FAULTMAP:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_prev_faultMap - m_faultMap;
        m_faultMap = value;
        break;
      case POS_VCELLMIN:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_vCellMin = value;
        break;
      case POS_VCELLMAX:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_vCellMax = value;
        break;
      case POS_CURRENT:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_current = value;
        break;
      case POS_TEMP_1:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_2:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_3:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_4:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_DRIVEWARNING:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, boolStr);
        m_prev_ = m_driveWarning;
        m_driveWarning = convertStrToBoolean(boolStr);
        break;
      case POS_DRIVEOVERTEMP:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, boolStr);
        m_prev_driveOverTemp = m_driveOverTemp;
        m_driveOverTemp = convertStrToBoolean(boolStr);
        break;
      case POS_DRIVEFAULT:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, boolStr);
        m_prev_driverFault = m_driverFault;
        m_driverFault = convertStrToBoolean(boolStr);
        break;
      case POS_GEARPOS:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_gearPos = value;
        break;
      case POS_ACCELPOS:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_accelPos = value;
        break;
    }
  }

  fclose(stateFile);

  if (m_speedCur == 0 && m_pBrake == TRUE && m_gearPos == GEARPOS_NEUTRAL && m_accelPos == 0)
  {
    DBG_OUT_MSG("Vehicle is currently parked.");
    m_isParked = TRUE;
  }
  else
  {
    DBG_OUT_MSG("Vehicle is not parked.");
    m_isParked FALSE;
  }
  if (m_faultMap != 0 || m_driveWarning || m_driveOverTemp || m_driverFault)
  {
    DBG_OUT_MSG("Vehicle fault is reported!");
    m_isFaultPresent =  TRUE;
  }
  else
  {
    DBG_OUT_MSG("No vehicle fault is reported!");
    m_isFaultPresent =  FALSE;
  }

  // Trapezoid area formula. Note that speed is in the unit of km/h
  travelledDist += (m_speedOld + m_speedCur) * difftime() / 7200
  return 0;
}


#ifdef DEBUG
void printExtractedAttribs()
{
  DBG_OUT_MSG("m_speedOld:" << m_speedOld);
  DBG_OUT_MSG("m_speedCur:" << m_speedCur);
  DBG_OUT_MSG("m_eStop:" << m_eStop);
  DBG_OUT_MSG("m_pBrake:" << m_pBrake);
  DBG_OUT_MSG("m_soc:" << m_soc);
  DBG_OUT_MSG("m_vCellMin:" << m_vCellMin);
  DBG_OUT_MSG("m_vCellMax:" << m_vCellMax);
  DBG_OUT_MSG("m_current:" << m_current);
  DBG_OUT_MSG("m_tempMin" << m_tempMin);
  DBG_OUT_MSG("m_tempMax:" << m_tempMax);
  DBG_OUT_MSG("m_gearPos:" << m_gearPos);
  DBG_OUT_MSG("m_accelPos:" << m_accelPos);
  DBG_OUT_MSG("m_prev_faultMap" << m_prev_faultMap);
  DBG_OUT_MSG("m_faultMap" << m_faultMap);
  DBG_OUT_MSG("m_prev_driveWarning:" << m_prev_driveWarning);
  DBG_OUT_MSG("m_driveWarning:" << m_driveWarning);
  DBG_OUT_MSG("m_prev_driveOverTemp:" << m_prev_driveOverTemp);
  DBG_OUT_MSG("m_driveOverTemp:" << m_driveOverTemp);
  DBG_OUT_MSG("m_prev_driverFault:" << m_prev_driverFault);
  DBG_OUT_MSG("m_driverFault:" << m_driverFault);
  return;
}
#endif


bool VehicleState::getIsParked()
{
  return m_isParked;
}


bool VehicleState::getIsFaultPresent()
{
  return m_isFaultPresent;
}


bool VehicleState::isDifferentError()
{
  if (m_prev_faultMap != m_faultMap)
    return TRUE;
  if (m_prev_driveWarning != m_driveWarning)
    return TRUE;
  if (m_prev_driveOvertemp != m_driveOvertemp)
    return TRUE;
  if (m_prev_driveFault != m_driveFault)
    return TRUE;
  return FALSE;
}


void VehicleState::generateErrorStr(char* pDest)
{
  memset((void*)pDest, 0, ERRORSTRSIZE * sizeof(char));
  if (m_faultMap != 0)
  {
    strncat(errStr, "Battery faultmap is non-zero!\n", 32);
  }
  if (m_driveWarning)
  {
    strncat(errStr, "Drive warning is reported!\n", 30);
  }
  if (m_driveOvertemp)
  {
    strncat(errStr, "Drive over temperature is reported!\n", 40);
  }
  if (m_driveFault)
  {
    strncat(errStr, "Drive fault is reported!\n", 30);
  }
  return;
}


// Private methods

bool VehicleState::convertStrToBoolean(char* strBoolean)
{
  if ( strncmp(strBoolean, BOOLEANSTR_TRUE, BOOLSTRSIZE) )
  {
    return TRUE;
  }
  if ( strncmp(strBoolean, BOOLEANSTR_FALSE, BOOLSTRSIZE) )
  {
    return FALSE;
  }
  DBG_ERR_MSG("Boolean string is neither true or false!");
  return FALSE;
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


static int EVStateNInputVInterface::setCurrentTimeStr(time_t& rRawTime)
{
  struct tm* timeInfo;

  m_rawPrevTime = rRawTime;

  time(&rRawTime);
  timeInfo = localtime(&rRawTime);

  if ( snprintf(m_dateNtimeStr, TIMESTRSIZE, "%d|%.2d|%.2d_%.2d|%.2d|%.2d.txt", 
                timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, 
                timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec) < 0)
  {
    DBG_ERR_MSG("snprintf error occured!");
    return 1;
  }

  return 0;
}