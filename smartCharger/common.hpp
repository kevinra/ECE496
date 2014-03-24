#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>

// DEBUG print format obtained from:
// http://latedev.wordpress.com/2012/08/09/c-debug-macros/

#ifdef DEBUG
  #define DBG_OUT_VAR(var) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ")::"\
         << __func__ << "> " << #var << " = [" << (var) << "]" << std::endl
  #define DBG_ERR_VAR(var) \
    std::cerr << "ERR: " << __FILE__ << "(" << __LINE__ << ")::"\
         << __func__ << "> " << #var << " = [" << (var) << "]" << std::endl
  #define DBG_OUT_MSG(msg) \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ")::" \
         << __func__ << "> " << msg << std::endl
  #define DBG_ERR_MSG(msg) \
    std::cerr << "ERR: " << __FILE__ << "(" << __LINE__ << ")::" \
         << __func__ << "> " << msg << std::endl
#else
  #define DBG_OUT_VAR(var)
  #define DBG_ERR_VAR(var)
  #define DBG_OUT_MSG(msg)
  #define DBG_ERR_MSG(msg)
#endif

#define ERR_MSG(msg) \
    std::cerr << "ERROR: " << __FILE__ << "(" << __LINE__ << ")::" \
         << __func__ << "> " << msg << std::endl

#define ERRORSTRSIZE 150
#define STATEFILE_STRSIZE 35
#define STATEFILE_LOCATION "/home/root/stateFiles/"
#define STATEFILE_FULLPATHSIZE 70

#define GPIO_REC1_PF_ENABLE 60     // P9_12 = GPIO1_28
#define GPIO_REC1_LD_ENABLE 48     // P9_15 = GPIO1_16
#define GPIO_REC1_PFM 3            // P9_21 = GPIO0_03
#define GPIO_REC2_PF_ENABLE 2      // P9_22 = GPIO2_02
#define GPIO_REC2_LD_ENABLE 49     // P9_23 = GPIO1_28
#define GPIO_REC2_PFM 15           // P9_24 = GPIO0_15
#define GPIO_Heater 14             // P9_26 = GPIO0_14
#define GPIO_BATRELAY 115          // P9_27 = GPIO3_19
#define GPIO_FPGAINTERRUPT 112     // P9_30 = GPIO3_16

// extern <type> g_errQueue;

#endif // COMMON_HPP