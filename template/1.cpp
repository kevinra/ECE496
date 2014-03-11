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

		fpgaCtrl();
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

S0: parked, not charging
S1: parked, charging
S2: crusing

S0:
  if (getShouldFPGAon)
  	record start driving time
    state = S1
  else if (! isParked)
  	state = S2

S1:
	if (! getShouldFPGAon)
		state = S0
S2:
	if (isParked)
		record travalled distance
	  state = S0

