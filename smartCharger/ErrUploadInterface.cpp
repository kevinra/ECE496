#include "ErrUploadInterface.hpp"

// Global object
ErrUploadInterface g_ErrUploadInterface;

ErrUploadInterface::ErrUploadInterface()
{
  m_fileInUseStr = NULL;
  m_fileInUseStrMtx = NULL;
}


ErrUploadInterface::~ErrUploadInterface()
{
  pthread_mutex_destroy(&m_fileInUseStrMtx);
}


int ErrUploadInterface::init()
{
  if ( pthread_mutex_init(&m_fileInUseStrMtx, NULL) )
  {
    ERR_MSG("m_fileInUseStrMtx initialization failed!");
    return 1;
  }
  return 0;
}


void ErrUploadInterface::setFileInUseStr(char* fileName)
{
  pthread_mutex_lock(&m_fileInUseStrMtx);
  m_fileInUseStr = fileName;
  pthread_mutex_unlock(&m_fileInUseStrMtx);
  return;
}


void ErrUploadInterface::getFileInUseStr()
{
  pthread_mutex_lock(&m_fileInUseStrMtx);
  char* retVal = m_fileInUseStr;
  pthread_mutex_unlock(&m_fileInUseStrMtx);
  retVal;
}

