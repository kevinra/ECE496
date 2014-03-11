/*
Questions to ask

<General>
Q. How do we support disconnection of the smart charger? It can be done through the web but it
   might be slow and unreliable. Need for hardware support on the box.

A. It's safe to remove once its disconnected from the grid.


Q. Which one should be turned on first? LLC or Rectifiers?
1. Battery Relay
2. Recifiers
3. LLC (FPGA)
1 & 2 are interchangable

Q. Which errors should we be aware of?

FPGA
LLC Errors from FPGA: LLC not responding
too high current

Voltmeter cannot be read --> Error


Q. How to know whether it is currently charging or not? i.e. when to set PF_Enable high?
   Is it always on high?
based on voltmeter input voltage
turn off when no input voltage
also when the battery full


Q. How to detect disconnection?
FPGA tells that LLCs have zero current
verification:Read voltmeter value



<Rectifiers>
Q. Before turning rectifiers on, it should be check whether everything is good to go.
   How do we check this? What should we check?

read voltmeter


Q. What to do PFM goes low?
Error, shutdown and display

Q. Any case when PF_Enable should be set low other then disconnecting? Shutdown on error?
   If so, define the error.
Nope, shutdown only


<FPGA>
Q. Other than communicating current to send to the battery, which information
   shoule be communicated between BBB and FPGA? Errors? If so, define the error
   along with actions to be taken for each corresponding error.

Since it's the master, it waits for any error from FPGA. Upon the error, just open
the relay

Q. Should I use Interrupt?
Better use it


<Battery Relay>
Q. When should it be turned on? Before FPGA will enable the LLC or after?
First 

Q. When should be turned off? Whenever there is disconnection from the grid?
ERROR or disconnection


<Input Voltmeter (I2C)>
Q. What actions should be taken for each input voltage? 110V/220V? Any formula?

around 2V 120V: only running 4LLC, not change in any signal from FPGA
around 4V 240V: Run normally

Q. Any errors that BBB should consider?
Error: out of expected range.
Warning: voltmeter out of service? --> 4 LLCs

Q. Can BBB read this voltmeter value before enabling rectifiers?
Yes. 

*/


/*
Questions to be think about

Q. What to do if no Internet connection?
A. Save locally. Make a queue of data to be updated to the server

Q. What if corruption on server side database?
A. Find a way to enfore the relation integrity on the DBs

Q. Where to store user habit?
A. BBB. Try to keep data on BBB up to date

Q. Data pushing from the server? such as user schedule. Polling from the BBB can be the waste of
   the bandwidth
A. Find a way to access BBB DB directly from the server

Q. Multithreading??
A. Probably. Better responsiveness for each threads.


*/


#include <iostream.h>

using namespace std;

#define REC1_PF_Enable P9_12
#define REC2_PF_Enable P9_22
#define REC1_LD_Enable P9_15
#define REC2_LD_Enable P9_23
#define REC1_PFM P9_21
#define REC2_PFM P9_24
#define HEATER_RELAY P9_26
#define DC_RELAY P9_27
#define FPGA_INTERRUPT P9_30  // Potential: not verified yet!


parsedStatusData;


int main(int argc, char *argv[])
{
  createThread(thread_rectifiers())

  stateFile = get("state.json")
  parsedStatusData = parse(stateFile)
  
  createThread(thread_heater())

  createThread(thread_FPGA())
  createThread(thread_inputVoltage())

  createThread(thread_upload())



}


thread_rectifiers()
{
  while (true)
  {
    if (input voltage != 0)
    {
      if (isRectifiersReady = false)
      {
        setGPIO(REC1_PF_Enable, HIGH)
        setGPIO(REC2_PF_Enable, HIGH)

        while (readGPIO(REC1_LD_Enable) != HIGH && readGPIO(REC2_LD_Enable) != HIGH) {}

        isRectifiersReady = true;
      }
      else
      {
        if ( readGPIO(REC1_PFM) == LOW || readGPIO(REC2_PFM) == LOW )
        {
          broadcast error
          setGPIO(REC1_PF_Enable, HIGH)
          setGPIO(REC2_PF_Enable, HIGH)
          display error
        }
      }
    }
    // input voltage = 0 but also process interrupt from FPGA (zero current)
    else if (isDisconnectedFromGrid)
    {
      setGPIO(REC1_PF_Enable, HIGH)
      setGPIO(REC2_PF_Enable, HIGH)
    }
  }
}


thread_batteryRelay()
{
  while (true)
  {
    if (input voltage != 0)
    {
      close battery relay
      isBatteryRelayReady = true;
    }
    else
    {
      open battery relay
    }
  }
}



thread_stateFileProcessing()
{

}



thread_heater()
{
  if (parsedStatusData.ready)
  {
    if sufficient energy for heater
      turn on the heater
    else
      turn off the heater
  }
}

thread_relay()
{

}

thread_FPGA()
{
  I2C_setValue(FPGA, currentToSend)
}

thread_upload()
{
  if (parsedStatusData.ready)
  {
    if (internet connection)
    {
      if (data from BBB DB which should be uploaded to the server)
      {
        dataToBeUploaded = queryFromBBB_DB + parsedStatusData
      }
      upload to server DB(dataToBeUploaded)
    }
    else
    {
      update beaglebone DB
    }
  }
}
// upload to the server frequently when driving and less frequently when parked.




