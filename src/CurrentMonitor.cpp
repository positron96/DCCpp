/**********************************************************************

CurrentMonitor.cpp
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#include "Arduino.h"

#ifdef ARDUINO_ARCH_AVR

#include "DCCpp_Uno.h"
#include "CurrentMonitor.h"
#include "Comm.h"

///////////////////////////////////////////////////////////////////////////////

void CurrentMonitor::begin(int pin, float inSampleMax)
{
	this->pin = pin;
	this->current = 0;
	this->currentSampleMax = inSampleMax;
} // CurrentMonitor::begin
  
boolean CurrentMonitor::checkTime()
{
	if(millis( ) - sampleTime < CURRENT_SAMPLE_TIME)   // no need to check current yet
		return(false);
	sampleTime = millis();  
	return(true);  
} // CurrentMonitor::checkTime
  
void CurrentMonitor::check()
{
	if (this->pin == UNDEFINED_PIN)
		return;

	float v = analogRead(this->pin) * 1024 / this->currentSampleMax;
	this->current = (float)(v * CURRENT_SAMPLE_SMOOTHING + this->current * (1.0 - CURRENT_SAMPLE_SMOOTHING));      // compute new exponentially-smoothed current
	//Serial.println(current);

	// current overload and Signal is on
	if (DCCpp::isPowerOn() && this->current > 1024)
	{
		DCCpp::powerOff();
	}
} // CurrentMonitor::check  

long int CurrentMonitor::sampleTime=0;

#endif
