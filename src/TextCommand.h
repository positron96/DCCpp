/**********************************************************************

SerialCommand.h
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#ifdef ARDUINO_ARCH_AVR
#ifndef TextCommand_h
#define TextCommand_h

#include "DCCpp.h"

/**	\defgroup commandsGroup Text Commands Syntax
*/

#ifdef USE_TEXTCOMMAND

#include "PacketRegister.h"
#include "CurrentMonitor.h"

#define  MAX_COMMAND_LENGTH         30

/** DCC++ BASE STATION COMMUNICATES VIA THE SERIAL PORT USING SINGLE-CHARACTER TEXT COMMANDS
WITH OPTIONAL PARAMETERS, AND BRACKETED BY < AND > SYMBOLS.  SPACES BETWEEN PARAMETERS
ARE REQUIRED.  SPACES ANYWHERE ELSE ARE IGNORED.  A SPACE BETWEEN THE SINGLE-CHARACTER
COMMAND AND THE FIRST PARAMETER IS ALSO NOT REQUIRED.*/
struct TextCommand{
	static char commandString[MAX_COMMAND_LENGTH+1];
	static void init(volatile RegisterList *, volatile RegisterList *, CurrentMonitor *);
	static void parse(char *);
	static void process();

private:
	static void setThrottle(char *);
	static void setFunction(char *);
	static void setAccessory(char *);
	static void writeTextPacket(volatile RegisterList& rl, char *);

	static void readCV(volatile RegisterList& rl, char *);
	static void writeVerifyCVByteProg(char *);
	static void writeVerifyCVBitProg(char *);

	static void writeCVByteMain(char *);
	static void writeCVBitMain(char *s);
}; // TextCommand
  
#endif
#endif
#endif
