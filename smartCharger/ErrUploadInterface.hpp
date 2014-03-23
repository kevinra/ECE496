#ifndef ERRUPLOADINTERFACE_HPP
#define ERRUPLOADINTERFACE_HPP

#include <pthread.h>

extern ErrUploadInterface g_ErrUploadInterface;

class ErrUploadInterface
{
public:
  ErrUploadInterface();
  ~ErrUploadInterface();
  int init();
  void addErrStr(char* fileName);
  char* getFileInUseStr();

private:
  char* m_fileInUseStr;
  pthread_mutex_t m_fileInUseStrMtx;
  pthread_cond_t m_
};

#endif // ERRUPLOADINTERFACE_HPP
