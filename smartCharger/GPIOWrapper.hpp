/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  Almost entirely based on Software by RidgeRun:
  Copyright (c) 2011, RidgeRun
  All rights reserved.
 
  GPIOWrapper.hpp

*/
#ifndef GPIOWRAPPER_HPP
#define GPIOWRAPPER_HPP

#include "common.hpp"

#define MAX_BUF 64
#define STR_RISING_EDGE "rising"
#define STR_FALLING_EDGE "falling"
#define STR_BOTH_EDGE "both"

enum PIN_VALUE
{
	LOW = 0,
	HIGH = 1
};

enum PIN_DIRECTION{
  INPUT_PIN = 0,
  OUTPUT_PIN
};

class GPIOWrapper
{
public:
  GPIOWrapper();
  GPIOWrapper(int gpioNum);
  ~GPIOWrapper();

  void setGPIONum(int gpioNum);
  int gpioInit(PIN_DIRECTION pinDir, char* edge, bool isNonBlocking);
  int gpioSet(PIN_VALUE val);
  int gpioGet(int* pValue);
  int gpioWaitForInterrupt();

private:
  int m_gpioNum;
  int m_fd;
  bool m_isInputPin;
  bool m_isInterruptSetup;
};


#endif GPIOWRAPPER_HPP