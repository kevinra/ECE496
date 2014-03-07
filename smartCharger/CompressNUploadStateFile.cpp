/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  CompressNUploadStateFile.cpp

  
*/

// <T3>

#include "CompressNUploadStateFile.hpp"


CompressNUploadStateFile::CompressNUploadStateFile()
{

}

CompressNUploadStateFile::~CompressNUploadStateFile()
{

}

void* CompressNUploadStateFile::run()
{
	while()
	{
	  cv_wait(gettingEVStateDone)
	  <currentDate>.7z = 7zcompress(<currentDate>.json)
	  ftpUpload(<currentDate>.7z)
	}
}


