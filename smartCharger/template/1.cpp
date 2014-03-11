#define TEMPQUERYINTERVAL
#define NOTINITIALIAZED
#define PREHEATINGPERIOD

time_t m_lastTempQueryTime
int outsideTemp = TEMP_UNDEF

void* run()
{
	isHeaterOoS
	g_currentDate (shared with this & EVState)
	distancePerSoC
	time_t estimatedChargeStopT


	while (TRUE)
	{
		if (evState.getNExtract())
		{
		  continue
		}

		cvSignal(gettingEVStateDone)  // wakeup T3

	  if (evState.getIsError())
	  {
	    t1t2Interface.setIsErrorFromState(TRUE)
	  }
	  else
	  {
	    t1t2Interface.setIsErrorFromState(FALSE)
	  }

		// FPGA
	  if ( t1t2Interface.getShouldFPGAon() && !t1t2Interface.getIsChargingHWErr() )
	  {
	    int inputCurrent = calculateInputCurrent();
	    if ( FPGAI2C(InputCurrent) )
	    {
	    	t1t2Interface.setShouldFPGAon(FALSE)
	      t1t2Interface.setIsChargingHWErr(TRUE)
	    }
	  }

		// Heater Control
    heaterCtrl();

		// Recording user pattern (time when stops charging)
	  ()

	  if (! g_errQueue.getIsQueueEmpty() )
	  {
	  	curl_multi_perform(...)
	  }

	  cvWait()

	}
}

