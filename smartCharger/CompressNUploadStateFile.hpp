/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.hpp

  
*/


#ifndef COMPRESSNUPLOADSTATEFILE_HPP
#define COMPRESSNUPLOADSTATEFILE_HPP

#include "common.hpp"
#include "thread/Thread.hpp"

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

  is7zFile(char fileName[])
  compressNUploadGroup(char fileListStr[]);
  upload7zFile(char 7zName[])
};

#endif // COMPRESSNUPLOADSTATEFILE_HPP
