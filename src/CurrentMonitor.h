/**********************************************************************

CurrentMonitor.h
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#ifdef ARDUINO_ARCH_AVR
#ifndef CurrentMonitor_h
#define CurrentMonitor_h

/** Factor to smooth the result...*/
#define  CURRENT_SAMPLE_SMOOTHING   0.02

/** Maximum current value in ADC numbers. */
#define  CURRENT_MAX   30

/** Time between two measurements. */
#define  CURRENT_SAMPLE_TIME        5


/** This structure/class describes a current monitor.*/

struct CurrentMonitor{
  static long int sampleTime; /**< time elapsed since last measurement. This delay is common to all monitors. */
	int pin;	/**< Attached check pin.*/
  float currentSampleMax;	/**< Value of the maximum current accepted without problem.*/
  float current; /**< Value of the last measured current.*/
  /** begin function.
	@param pin	Attached pin. UNEFINED_PIN to inactivate this monitor.
	@param inSignalPin	Pin to set LOW if a shortcut is detectexd.
  @param msg	Message to send to console when a smoothed current value greater than maximum is detected.
  @param inSampleMax	Maximum value of the current. 
  */
  void begin(int pin, float inSampleMax = CURRENT_MAX);
  /** Checks if sufficient time has elapsed since last update. Common to all monitors.
  */
  static boolean checkTime();
  /** Checks the current on this monitor.
  */
  void check();
};

#endif
#endif
