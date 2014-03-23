/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  Almost entirely based on Software by RidgeRun:
  Copyright (c) 2011, RidgeRun
  All rights reserved.

  GPIOWrapper.cpp


*/

#include "GPIOWrapper.hpp"
#include <fcntl.h>
#include <poll.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT -1  // -1 means no timeout


#define STRLEN_OUT 4
#define STRLEN_IN 3

GPIOWrapper::GPIOWrapper()
{
  m_gpioNum = 0;
  m_fd = 0;
  m_isInputPin = TRUE;
  m_isInterruptSetup = FALSE;
}

GPIOWrapper::GPIOWrapper(int gpioNum)
{
  m_gpioNum = gpioNum;
  m_fd = 0;
  m_isInputPin = TRUE;
  m_isInterruptSetup = FALSE;
}


GPIOWrapper::~GPIOWrapper()
{
  // No error handling here.
  int fd, len;
  char buf[MAX_BUF]; 
  fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
  len = snprintf(buf, sizeof(buf), "%d", m_gpioNum);
  write(fd, buf, len);
  close(fd);
}


void GPIOWrapper::setGPIONum(int gpioNum)
{
  m_gpioNum = gpioNum;
  return;
}


int GPIOWrapper::init(PIN_DIRECTION pinDir, char* edge, bool isNonBlocking)
{
  int fd, len, retVal;
  char buf[MAX_BUF];

  // Argument validity check: if edge is given, the pin must be in the input direction
  if (edge != NULL && pinDir != INPUT_PIN)
  {
    DBG_ERR_MSG("If edge is specified, pin direction cannot be output!");
    return -1;
  }
 
  // Export the pin
  fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
  if (fd < 0)
  {
    DBG_ERR_MSG("gpio/export file opening on pin " << m_gpioNum < "failed!");
    return fd;
  }
  len = snprintf(buf, sizeof(buf), "%d", m_gpioNum);
  if (len < 0)
  {
    DBG_ERR_MSG("gpio/export snprintf on pin" << m_gpioNum < "failed!");
    return len;
  }
  retVal = write(fd, buf, len);
  if (retVal < 0)
  {
    DBG_ERR_MSG("gpio/export file writing on pin " << m_gpioNum < "failed!");
    return retVal;
  }
  revVal = close(fd);
  if (retVal < 0)
  {
    DBG_ERR_MSG("gpio/export file closing on pin " << m_gpioNum < "failed!");
    return retVal;
  }
 
  // We are going to use the same buffer over and over so flush it everytime
  // after the use.
  // Set direction of the pin
  memset((void*) buf, 0, MAX_BUF);
  len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", m_gpioNum);
  if (len < 0)
  {
    DBG_ERR_MSG("gpio/direction snprintf on pin" << m_gpioNum < "failed!");
    return len;
  }
  fd = open(buf, O_WRONLY);
  if (fd < 0)
  {
    DBG_ERR_MSG("gpio/direction file opening on pin " << m_gpioNum < "failed!");
    return fd;
  }
  if (pinDir)
  {
    retVal = write(fd, "out", STRLEN_OUT);
    m_isInputPin = FALSE;
  }
  else
  {
    retVal = write(fd, "in", STRLEN_IN);
  }
  if (retVal < 0)
  {
    DBG_ERR_MSG("gpio/direction file writing on pin " << m_gpioNum < "failed!");
    return retVal;
  }
  revVal = close(fd);
  if (retVal < 0)
  {
    DBG_ERR_MSG("gpio/direction file closing on pin " << m_gpioNum < "failed!");
    return retVal;
  }

  // Set the interrupt edge if specified
  if (edge != NULL)
  {
    memset((void*) buf, 0, MAX_BUF);
    len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", m_gpioNum);
    if (len < 0)
    {
      DBG_ERR_MSG("gpio/edge snprintf on pin" << m_gpioNum < "failed!");
      return len;
    }
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
      DBG_ERR_MSG("gpio/edge file opening on pin " << m_gpioNum < "failed!");
      return fd;
    }
   
    retVal = write(fd, edge, strlen(edge) + 1);
    if (retVal < 0)
    {
      DBG_ERR_MSG("gpio/edge file writing on pin " << m_gpioNum < "failed!");
      return retVal;
    }
    retVal = close(fd);
    if (retVal < 0)
    {
      DBG_ERR_MSG("gpio/edge file closing on pin " << m_gpioNum < "failed!");
      return retVal;
    }
    m_isInterruptSetup = TRUE;
  }

  // Open and save the file desriptor of the pin on memory
  memset((void*) buf, 0, MAX_BUF);
  len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", m_gpioNum);
  if (len < 0)
  {
    DBG_ERR_MSG("gpio/value snprintf on pin" << m_gpioNum < "failed!");
    return len;
  }
  if (edge != NULL)
  {
    fd = open(buf, O_RDONLY | O_NONBLOCK );
  }
  // Not sure if below fd thing works...
  else if (pinDir == INPUT_PIN)
  {
    fd = open(buf, O_RDONLY);  
  }
  else if (isNonBlocking)
  {
    fd = open(buf, O_WRONLY | O_NONBLOCK );
  }
  else
  {
    fd = open(buf, O_WRONLY);
  }
  if (fd < 0)
  {
    DBG_ERR_MSG("gpio/value file opening on pin " << m_gpioNum < "failed!");
    return fd;
  }

  m_fd = fd;
  return 0;
}


int GPIOWrapper::gpioSet(PIN_VALUE val)
{
  int retVal;
  if (m_isInputPin)
  {
    DBG_ERR_MSG("Cannot set GPIO value on input pin!");
    return -1;
  }

  if (val)
  {
    retVal = write(m_fd, "1", 2);
  }
  else
  {
    retVal = write(m_fd, "0", 2);
  }
  if (retVal < 0)
  {
    DBG_ERR_MSG("file writing on pin " << m_gpioNum < "failed!");
    return retVal;
  }

  return 0;
}

// Not sure this will work properlly when file descriptor is opened with non-block option
int GPIOWrapper::gpioGet(int* pValue)
{
  int retVal;
  char ch;

  retVal = read(m_fd, &ch, 1);
  if (retVal < 0)
  {
    DBG_ERR_MSG("File reading on pin " << m_gpioNum < "failed!");
    return retVal;
  }

  if (ch != '0')
  {
    *pValue = 1;
  }
  else
  {
    *pValue = 0;
  }

  return 0;
}


int GPIOWrapper::getGPIOfd()
{
  return m_fd;
}


int GPIOWrapper::gpioWaitForInterrupt()
{
  struct pollfd fdset[2];
  int nfds = 2; // Size of struct pollfd array
  int len;
  char* buf[MAX_BUF];

  memset((void*)fdset, 0, nfds * sizeof(fdset));
  fdset[0].fd = STDIN_FILENO;
  fdset[0].events = POLLIN;
  fdset[1].fd = m_fd;
  fdset[1].events = POLLPRI;

  if ( poll(fdset, nfds, POLL_TIMEOUT) < 0 )
  {
    DBG_ERR_MSG("poll() on pin " << m_gpioNum << " failed!");
    return -1;
  }
  if (fdset[1].revents & POLLPRI)
  {
    // Not sure if read step below is necessary.
    DBG_OUT_MSG("poll() on pin " << m_gpioNum << " interrupt occurred.");
    len = read(fdset[1].fd, buf, MAX_BUF);
  }
  if (fdset[0].revents & POLLIN)
  {
    // Not sure if read step below is necessary.
    (void)read(fdset[0].fd, buf, 1);
  }

  return 0;
}

