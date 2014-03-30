#include "EVStateNInputVInterface.hpp"
#include "EVStateDependents.hpp"
// #include "InputVoltDependents.hpp"
#include "CompressNUploadStateFile.hpp"
#include <curl/curl.h>

int main()
{
  if ( g_EVStateNInputVInterface.init() )
  {
    ERR_MSG("Global object for evState and inputV interface failed!");
    return 1;
  }

  // InputVoltDependents ivdThread;
  EVStateDependents evdThread;
  CompressNUploadStateFile cnuThread;

  CURLcode curlRetv;
  if ( (curlRetv = curl_global_init(CURL_GLOBAL_DEFAULT)) != CURLE_OK )
  {
    ERR_MSG("Global initialization of CURL API failed!");
    return 1;
  }
  
  // if ( ivdThread.init() )
  // {
  //   ERR_MSG("InputVoltDependents thread initialization failed!");
  //   return 1;
  // }
  if ( evdThread.init() )
  {
    ERR_MSG("EVStateDependents thread initialization failed!");
    return 1;
  }
  if ( cnuThread.init() )
  {
    ERR_MSG("EVStateDependents thread initialization failed!");
    return 1;
  }

  // ivdThread.start();
  // evdThread.start();
  cnuThread.start();

  // ivdThread.join();
  // evdThread.join();
  cnuThread.join();

  curl_global_cleanup();
  return 0;
}
