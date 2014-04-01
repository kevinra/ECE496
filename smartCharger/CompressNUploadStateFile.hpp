/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.hpp

  
*/


#ifndef COMPRESSNUPLOADSTATEFILE_HPP
#define COMPRESSNUPLOADSTATEFILE_HPP

#include "common.hpp"
#include "Thread.hpp"
#include <string>
#include <curl/curl.h>

enum cmdMode
{
  CM_REMOVE = 0,
  CM_COMPRESS = 1
};

class CompressNUploadStateFile: public Thread
{
public:
  CompressNUploadStateFile();
  ~CompressNUploadStateFile();
  int init();
  void* run();

private:
  CURL* m_curl;
  unsigned int m_numTarCreated;
  unsigned int m_numFileAdded;

  bool isTarFile(char fileName[]);
  void compressNUploadGroup(std::string fileNameArry[]);
  void uploadTarBall(char nameTarBall[]);
  int forkNRunCmd(cmdMode cm, char arg1[], std::string arg2[]);
};

#endif // COMPRESSNUPLOADSTATEFILE_HPP
