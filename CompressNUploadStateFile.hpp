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
  void* run();

private:
  CURL* curl;

};

#endif // COMPRESSNUPLOADSTATEFILE_HPP