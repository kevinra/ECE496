/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.hpp

  
*/


#ifndef COMPRESSNUPLOADSTATEFILE_HPP
#define COMPRESSNUPLOADSTATEFILE_HPP

#include "common.hpp"
#include "Thread.hpp"
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
  unsigned int m_num7ZipCreated;
  unsigned int m_numFileAdded;

  bool is7zFile(char fileName[]);
  void compressNUploadGroup(char fileListStr[]);
  void upload7zFile(char name7z[]);
  void forkNRunCmd(cmdMode cm, char arg1[], char arg2[]);
};

#endif // COMPRESSNUPLOADSTATEFILE_HPP
