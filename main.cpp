// queryServer(electricity_Price) - as future support

#include "EVStateNInputVInterface.hpp"
#include "EVStateDependents.hpp"
#include "InputVoltDependents.hpp"
#include "CompressNUploadStatFile.hpp"

int main()
{
	if (g_EVStateNInputVInterface.init() < 0)
	{
		ERR_MSG("global object g_EVStateNInputVInterface initialization failed!");
	  return 0;
	}

  InputVoltDependents ivdThread;
  EVStateDependents evdThread;
  if (ivdThread.init() )
  {
  	DBG_ERR_MSG("InputVoltDependents thread initialization failed!");
  	return 0;
  }
  if (evdThread.init() )
  {
  	DBG_ERR_MSG("EVStateDependents thread initialization failed!");
  	return 0;
  }


	curl_global_init(CURL_GLOBAL_DEFAULT);

	evdThread.start();
  ivdThread.start();



  if (! g_errQueue.isEmpty() )
  {
  	upload data
  }

	curl_global_cleanup();
	return 0;
	
}