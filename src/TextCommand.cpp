/**********************************************************************

TextCommand.cpp
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/

#include "Arduino.h"
#ifdef ARDUINO_ARCH_AVR
// See TextCommand::parse() below for defined text commands.

#include "TextCommand.h"
#ifdef USE_TEXTCOMMAND

#ifdef VISUALSTUDIO
#include "string.h"
#else
extern unsigned int __heap_start;
extern void *__brkval;
#endif

///////////////////////////////////////////////////////////////////////////////

char TextCommand::commandString[MAX_COMMAND_LENGTH+1];

///////////////////////////////////////////////////////////////////////////////

void TextCommand::init(volatile RegisterList *_mRegs, volatile RegisterList *_pRegs, CurrentMonitor *_mMonitor){
  commandString[0] = 0;
} // TextCommand:TextCommand

///////////////////////////////////////////////////////////////////////////////

void TextCommand::process(){
  char c;
	
  #if defined(USE_ETHERNET)

	EthernetClient client= DCCPP_INTERFACE.available();

	if (client) {

		if (DCCppConfig::Protocol == EthernetProtocol::HTTP) {
			DCCPP_INTERFACE.println("HTTP/1.1 200 OK");
			DCCPP_INTERFACE.println("Content-Type: text/html");
			DCCPP_INTERFACE.println("Access-Control-Allow-Origin: *");
			DCCPP_INTERFACE.println("Connection: close");
			DCCPP_INTERFACE.println("");
		}

		while (client.connected() && client.available()) {        // while there is data on the network
			c = client.read();
			if (c == '<')                    // start of new command
				commandString[0] = 0;
			else if (c == '>')               // end of new command
				parse(commandString);
			else if (strlen(commandString) < MAX_COMMAND_LENGTH)    // if comandString still has space, append character just read from network
				sprintf(commandString, "%s%c", commandString, c);     // otherwise, character is ignored (but continue to look for '<' or '>')
		} // while

		if (DCCppConfig::Protocol == EthernetProtocol::HTTP)
			client.stop();
	}

  #else  // SERIAL case

	while (DCCPP_INTERFACE.available()>0) {    // while there is data on the serial line
		c = DCCPP_INTERFACE.read();
		if (c == '<')                    // start of new command
			commandString[0] = 0;
		else if (c == '>')               // end of new command
			parse(commandString);
		else if (strlen(commandString) < MAX_COMMAND_LENGTH)	// if commandString still has space, append character just read from serial line
			sprintf(commandString, "%s%c", commandString, c);	// otherwise, character is ignored (but continue to look for '<' or '>')
	} // while
  
  #endif

} // TextCommand:process
   
///////////////////////////////////////////////////////////////////////////////

void TextCommand::parse(char *com){


#ifdef DCCPP_DEBUG_MODE
	Serial.print(com[0]);
	Serial.println(F(" command"));
#endif

  switch(com[0]){

	case 't':       
		/**	\addtogroup commandsGroup
		SET ENGINE THROTTLES USING 128-STEP SPEED CONTROL
		-------------------------------------------------

		<b>
		\verbatim
		<t REGISTER CAB SPEED DIRECTION>
		\endverbatim
		</b>

		sets the throttle for a given register/cab combination 

		- <b>REGISTE%R</b>: an internal register number, from 1 through MAX_MAIN_REGISTERS (inclusive), to store the DCC packet used to control this throttle setting
		- <b>CAB</b>:  the short (1-127) or long (128-10293) address of the engine decoder
		- <b>SPEED</b>: throttle speed from 0-126, or -1 for emergency stop (resets SPEED to 0)
		- <b>DIRECTION</b>: 1=forward, 0=reverse.  Setting direction when speed=0 or speed=-1 only effects directionality of cab lighting for a stopped train

		returns: <b>\<T REGISTE%R SPEED DIRECTION\></b>
		*/

		setThrottle(com+1);
		break;

	case 'f':       
		/**	\addtogroup commandsGroup
		OPERATE ENGINE DECODER FUNCTIONS F0-F28
		---------------------------------------
		
		<b>
		\verbatim
		<f CAB BYTE1 [BYTE2]>
		\endverbatim
		</b>

		turns on and off engine decoder functions F0-F28 (F0 is sometimes called FL)  
		NOTE: setting requests transmitted directly to mobile engine decoder --- current state of engine functions is not stored by this program
   
		- <b>CAB</b>:  the short (1-127) or long (128-10293) address of the engine decoder
   
		To set functions F0-F4 on (=1) or off (=0):
     
		- <b>BYTE1</b>:  128 + F1*1 + F2*2 + F3*4 + F4*8 + F0*16
		- <b>BYTE2</b>:  omitted
  
		To set functions F5-F8 on (=1) or off (=0):
  
		- <b>BYTE1</b>:  176 + F5*1 + F6*2 + F7*4 + F8*8
		- <b>BYTE2</b>:  omitted
  
		To set functions F9-F12 on (=1) or off (=0):
  
		- <b>BYTE1</b>:  160 + F9*1 +F10*2 + F11*4 + F12*8
		- <b>BYTE2</b>:  omitted
  
		To set functions F13-F20 on (=1) or off (=0):
  
		- <b>BYTE1</b>: 222 
		- <b>BYTE2</b>: F13*1 + F14*2 + F15*4 + F16*8 + F17*16 + F18*32 + F19*64 + F20*128
  
		To set functions F21-F28 on (=1) of off (=0):
  
		- <b>BYTE1</b>: 223
		- <b>BYTE2</b>: F21*1 + F22*2 + F23*4 + F24*8 + F25*16 + F26*32 + F27*64 + F28*128
  
		returns: NONE
		*/


		setFunction(com+1);
		break;

	case 'a':       
		/**	\addtogroup commandsGroup
		OPERATE STATIONARY ACCESSORY DECODERS 
		-------------------------------------
		
		<b>
		\verbatim
		<a ADDRESS SUBADDRESS ACTIVATE>
		\endverbatim
		</b>

		turns an accessory (stationary) decoder on or off
   
		- <b>ADDRESS</b>:  the primary address of the decoder (0-511)
		- <b>SUBADDRESS</b>: the sub-address of the decoder (0-3)
		- <b>ACTIVATE</b>: 1=on (set), 0=off (clear)
   
		Note that many decoders and controllers combine the ADDRESS and SUBADDRESS into a single number, N,
		from  1 through a max of 2044, where
   
		N = (ADDRESS - 1) * 4 + SUBADDRESS + 1, for all ADDRESS>0
   
		OR
   
		- <b>ADDRESS</b> = INT((N - 1) / 4) + 1
		- <b>SUBADDRESS</b> = (N - 1) % 4
   
		However, this general command simply sends the appropriate DCC instruction packet to the main tracks
		to operate connected accessories.  It does not store or retain any information regarding the current
		status of that accessory. To have this sketch store and retain the direction of DCC-connected turnouts, as 
		well as automatically invoke the required <b>\<a\></b> command as needed, first define/edit/delete turnouts 
		using the following variations of the "T" command.

		returns: NONE
		*/

		setAccessory(com+1);
		break;

#ifdef USE_TURNOUT
	case 'T':
/*
* *** SEE TURNOUT.CPP FOR COMPLETE INFO ON THE DIFFERENT VARIATIONS OF THE "T" COMMAND
* USED TO CREATE/EDIT/REMOVE/SHOW TURNOUT DEFINITIONS
*/

		Turnout::parse(com+1);
		break;
#endif

#ifdef USE_OUTPUT

	case 'Z':
/**** SEE OUTPUT.CPP FOR COMPLETE INFO ON THE DIFFERENT VARIATIONS OF THE "Z" COMMAND
*   USED TO CREATE / EDIT / REMOVE / SHOW OUTPUT DEFINITIONS
*/

		Output::parse(com+1);
		break;
#endif

#ifdef USE_SENSOR

	case 'S': 
/*   
 *   *** SEE SENSOR.CPP FOR COMPLETE INFO ON THE DIFFERENT VARIATIONS OF THE "S" COMMAND
 *   USED TO CREATE/EDIT/REMOVE/SHOW SENSOR DEFINITIONS
 */
		Sensor::parse(com+1);
		break;

#ifdef DCCPP_PRINT_DCCPP
	case 'Q':
		/**	\addtogroup commandsGroup
		SHOW STATUS OF ALL SENSORS
		--------------------------

		<b>
		\verbatim
		<Q>
		\endverbatim
		</b>

		returns: the status of each sensor ID in the form <b>\<Q ID\></b> (active) or <b>\<q ID\></b> (not active)
		*/

		Sensor::status();
		break;
#endif
#endif

	case 'w':      
		
		/**	\addtogroup commandsGroup
		WRITE CONFIGURATION VARIABLE BYTE TO ENGINE DECODER ON MAIN OPERATIONS TRACK
		----------------------------------------------------------------------------

		<b>
		\verbatim
		<w CAB CV VALUE>
		\endverbatim
		</b>

		writes, without any verification, a Configuration Variable to the decoder of an engine on the main operations track
    
		- <b>CAB</b>:  the short (1-127) or long (128-10293) address of the engine decoder 
		- <b>CV</b>: the number of the Configuration Variable memory location in the decoder to write to (1-1024)
		- <b>VALUE</b>: the value to be written to the Configuration Variable memory location (0-255)
    
		returns: NONE
		*/    

		writeCVByteMain(com+1);
		break;      


	case 'b':      
		/**	\addtogroup commandsGroup
		WRITE CONFIGURATION VARIABLE BIT TO ENGINE DECODER ON MAIN OPERATIONS TRACK
		---------------------------------------------------------------------------

		<b>
		\verbatim
		<b CAB CV BIT VALUE>
		\endverbatim
		</b>

		writes, without any verification, a single bit within a Configuration Variable to the decoder of an engine on the main operations track
    
		- <b>CAB</b>:  the short (1-127) or long (128-10293) address of the engine decoder 
		- <b>CV</b>: the number of the Configuration Variable memory location in the decoder to write to (1-1024)
		- <b>BIT</b>: the bit number of the Configuration Variable register to write (0-7)
		- <b>VALUE</b>: the value of the bit to be written (0-1)
    
		returns: NONE
		*/        

		writeCVBitMain(com+1);
		break;      

	case 'W':      
		/**	\addtogroup commandsGroup
		WRITE CONFIGURATION VARIABLE BYTE TO ENGINE DECODER ON PROGRAMMING TRACK
		------------------------------------------------------------------------

		<b>
		\verbatim
		<W CV VALUE CALLBACKNUM CALLBACKSUB>
		\endverbatim
		</b>

		writes, and then verifies, a Configuration Variable to the decoder of an engine on the programming track
    
		- <b>CV</b>: the number of the Configuration Variable memory location in the decoder to write to (1-1024)
		- <b>VALUE</b>: the value to be written to the Configuration Variable memory location (0-255) 
		- <b>CALLBACKNUM</b>: an arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs that call this function
		- <b>CALLBACKSUB</b>: a second arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs (e.g. DCC++ Interface) that call this function
    
		returns: <b>\<r CALLBACKNUM|CALLBACKSUB|CV Value\></b>
		where VALUE is a number from 0-255 as read from the requested CV, or -1 if verification read fails
		*/    

	  writeVerifyCVByteProg(com+1);
	  break;      

	case 'B':      
		/**	\addtogroup commandsGroup
		WRITE CONFIGURATION VARIABLE BIT TO ENGINE DECODER ON PROGRAMMING TRACK
		-----------------------------------------------------------------------

		<b>
		\verbatim
		<B CV BIT VALUE CALLBACKNUM CALLBACKSUB>
		\endverbatim
		</b>

		writes, and then verifies, a single bit within a Configuration Variable to the decoder of an engine on the programming track
		
		- <b>CV</b>: the number of the Configuration Variable memory location in the decoder to write to (1-1024)
		- <b>BIT</b>: the bit number of the Configuration Variable memory location to write (0-7)
		- <b>VALUE</b>: the value of the bit to be written (0-1)
		- <b>CALLBACKNUM</b>: an arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs that call this function
		- <b>CALLBACKSUB</b>: a second arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs (e.g. DCC++ Interface) that call this function
		
		returns: <b>\<r CALLBACKNUM|CALLBACKSUB|CV BIT VALUE\></b>
		where VALUE is a number from 0-1 as read from the requested CV bit, or -1 if verification read fails
		*/

	  writeVerifyCVBitProg(com+1);
	  break;      

	case 'R':     
		/**	\addtogroup commandsGroup
		READ CONFIGURATION VARIABLE BYTE FROM ENGINE DECODER ON PROGRAMMING TRACK
		-------------------------------------------------------------------------

		<b>
		\verbatim
		<R CV CALLBACKNUM CALLBACKSUB>
		\endverbatim
		</b>

		reads a Configuration Variable from the decoder of an engine on the programming track
		
		- <b>CV</b>: the number of the Configuration Variable memory location in the decoder to read from (1-1024)
		- <b>CALLBACKNUM</b>: an arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs that call this function
		- <b>CALLBACKSUB</b>: a second arbitrary integer (0-32767) that is ignored by the Base Station and is simply echoed back in the output - useful for external programs (e.g. DCC++ Interface) that call this function
		
		returns: <b>\<r CALLBACKNUM|CALLBACKSUB|CV VALUE\></b>
		where VALUE is a number from 0-255 as read from the requested CV, or -1 if read could not be verified
		*/

	  readCV(DCCpp::progRegs, com+1);
	  break;

	case '1':      
		/**	\addtogroup commandsGroup
		TURN ON POWER FROM MOTOR SHIELD TO TRACKS
		-----------------------------------------

		<b>
		\verbatim
		<1>
		\endverbatim
		</b>

		enables power from the motor shield to the main operations and programming tracks
    
		returns: <b>\<p1\></b>
		*/    
		DCCpp::powerOn();
		break;
		  
	case '0':     
		/**	\addtogroup commandsGroup
		TURN OFF POWER FROM MOTOR SHIELD TO TRACKS
		------------------------------------------

		<b>
		\verbatim
		<0>
		\endverbatim
		</b>

		disables power from the motor shield to the main operations and programming tracks
    
		returns: <b>\<p0\></b>
		*/
		DCCpp::powerOff();
		break;

	case 'c':     
		/**	\addtogroup commandsGroup
		READ MAIN OPERATIONS TRACK CURRENT
		----------------------------------

		<b>
		\verbatim
		<c>
		\endverbatim
		</b>

		reads current being drawn on main operations track
    
		returns: <b>\<a CURRENT\> </b>
		where CURRENT = 0-1024, based on exponentially-smoothed weighting scheme
		*/

		DCCPP_INTERFACE.print("<a");
		DCCPP_INTERFACE.print(int(DCCpp::getCurrentMain()));
		DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
		break;

	case 's':
		/**	\addtogroup commandsGroup
		STATUS OF DCC++ BASE STATION
		----------------------------

		<b>
		\verbatim
		<s>
		\endverbatim
		</b>

		returns status messages containing track power status, throttle status, turn-out status, and a version number
		NOTE: this is very useful as a first command for an interface to send to this sketch in order to verify connectivity and update any GUI to reflect actual throttle and turn-out settings
    
		returns: series of status messages that can be read by an interface to determine status of DCC++ Base Station and important settings
		*/

		if(digitalRead(DCCppConfig::SignalEnablePinProg)==LOW)      // could check either PROG or MAIN
			DCCPP_INTERFACE.print("<p0>");
		else
			DCCPP_INTERFACE.print("<p1>");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif

		for(int i=1;i<=MAX_MAIN_REGISTERS;i++) {
			if(DCCpp::mainRegs.speedTable[i]==0)
				continue;
			DCCPP_INTERFACE.print("<T");
			DCCPP_INTERFACE.print(i); DCCPP_INTERFACE.print(" ");
			if(DCCpp::mainRegs.speedTable[i]>0){
				DCCPP_INTERFACE.print(DCCpp::mainRegs.speedTable[i]);
				DCCPP_INTERFACE.print(" 1>");
			} else {
				DCCPP_INTERFACE.print(- DCCpp::mainRegs.speedTable[i]);
				DCCPP_INTERFACE.print(" 0>");
			}          
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
	  	}
		DCCPP_INTERFACE.print(F("<iDCC++ LIBRARY BASE STATION FOR ARDUINO "));
		//DCCPP_INTERFACE.print(ARDUINO_TYPE);
		//DCCPP_INTERFACE.print(" / ");
		//DCCPP_INTERFACE.print(MOTOR_SHIELD_NAME);
		DCCPP_INTERFACE.print(F(": V-"));
		DCCPP_INTERFACE.print(F(VERSION));
		DCCPP_INTERFACE.print(" / ");
		DCCPP_INTERFACE.print(F(__DATE__));
		DCCPP_INTERFACE.print(" ");
		DCCPP_INTERFACE.print(F(__TIME__));
		DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif

		DCCPP_INTERFACE.print("<N ");
#if defined(USE_ETHERNET)
		DCCPP_INTERFACE.print("ETHERNET :");
		DCCPP_INTERFACE.print(Ethernet.localIP());
		DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
#else
		DCCPP_INTERFACE.println("SERIAL>");
#endif

#ifdef DCCPP_PRINT_DCCPP
#ifdef USE_TURNOUT
		Turnout::show();
#endif
#ifdef USE_OUTPUT
		Output::show();
#endif
#ifdef USE_SENSOR
		Sensor::show();
#endif
#endif
		break;

#ifdef USE_EEPROM
	case 'E':     
		/**	\addtogroup commandsGroup
		STORE SETTINGS IN EEPROM
		------------------------

		<b>
		\verbatim
		<E>
		\endverbatim
		</b>

		Stores settings for turnouts and sensors EEPROM
    
		returns: <b>\<e nTurnouts nSensors\></b>
		*/
	 
		EEStore::store();
		DCCPP_INTERFACE.print("<e ");
		DCCPP_INTERFACE.print(EEStore::data.nTurnouts);
		DCCPP_INTERFACE.print(" ");
		DCCPP_INTERFACE.print(EEStore::data.nSensors);
		DCCPP_INTERFACE.print(" ");
		DCCPP_INTERFACE.print(EEStore::data.nOutputs);
		DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
		break;

	case 'e':     
		/**	\addtogroup commandsGroup
		CLEAR SETTINGS IN EEPROM
		------------------------

		<b>
		\verbatim
		<e>
		\endverbatim
		</b>

		clears settings for Turnouts in EEPROM
    
		returns: <b>\<O\></b>
		*/
	 
		EEStore::clear();
		DCCPP_INTERFACE.print("<O>");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
	break;
#endif

	case ' ':
		/**	\addtogroup commandsGroup
		PRINT CARRIAGE RETURN IN SERIAL MONITOR WINDOW
		----------------------------------------------

		<b>
		\verbatim
		< >
		\endverbatim
		</b>

		simply prints a carriage return - useful when interacting with ArduIno through serial monitor window
    
		returns: a carriage return
		*/

		DCCPP_INTERFACE.println("");
		break;  

///          
/// THE FOLLOWING COMMANDS ARE NOT NEEDED FOR NORMAL OPERATIONS AND ARE ONLY USED FOR TESTING AND DEBUGGING PURPOSES
/// PLEASE SEE SPECIFIC WARNINGS IN EACH COMMAND BELOW
///

	case 'D':       
		/**	\addtogroup commandsGroup
		ENTER DIAGNOSTIC MODE
		---------------------

		<b>
		\verbatim
		<D>
		\endverbatim
		</b>

		changes the clock speed of the chip and the pre-scaler for the timers so that you can visually see the DCC signals flickering with an LED
		SERIAL COMMUNICATION WILL BE INTERUPTED ONCE THIS COMMAND IS ISSUED - MUST RESET BOARD OR RE-OPEN SERIAL WINDOW TO RE-ESTABLISH COMMS
		*/

		Serial.println("\nEntering Diagnostic Mode...");
		delay(1000);
		
		bitClear(TCCR1B,CS12);    // set Timer 1 prescale=8 - SLOWS NORMAL SPEED BY FACTOR OF 8
		bitSet(TCCR1B,CS11);
		bitClear(TCCR1B,CS10);

		#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)      // Configuration for UNO

		  bitSet(TCCR0B,CS02);    // set Timer 0 prescale=256 - SLOWS NORMAL SPEED BY A FACTOR OF 4
		  bitClear(TCCR0B,CS01);
		  bitClear(TCCR0B,CS00);
		  
		#else                     // Configuration for MEGA

		  bitClear(TCCR3B,CS32);    // set Timer 3 prescale=8 - SLOWS NORMAL SPEED BY A FACTOR OF 8
		  bitSet(TCCR3B,CS31);
		  bitClear(TCCR3B,CS30);

		#endif

		CLKPR = 0x80;           // THIS SLOWS DOWN SYSYEM CLOCK BY FACTOR OF 256
		CLKPR = 0x08;           // BOARD MUST BE RESET TO RESUME NORMAL OPERATIONS

		break;

	case 'M':       
		/**	\addtogroup commandsGroup
		WRITE A DCC PACKE%T TO ONE OF THE REGISTERS DRIVING THE MAIN OPERATIONS TRACK
		-----------------------------------------------------------------------------

		<b>
		\verbatim
		<M REGISTER BYTE1 BYTE2 [BYTE3] [BYTE4] [BYTE5]>
		\endverbatim
		</b>

		writes a DCC packet of two, three, four, or five hexadecimal bytes to a register driving the main operations track
		fOR DEBUGGING AND TESTING PURPOSES ONLY.  DO NOT USE UNLESS YOU KNOW HOW TO CONSTRUCT NMRA DCC PACKETS - YOU CAN INADVERTENTLY RE-PROGRAM YOUR ENGINE DECODER
   
		- <b>REGISTE%R</b>: an internal register number, from 0 through MAX_MAIN_REGISTERS (inclusive), to write (if REGISTE%R=0) or write and store (if REGISTE%R>0) the packet 
		- <b>BYTE1</b>:  first hexadecimal byte in the packet
		- <b>BYTE2</b>:  second hexadecimal byte in the packet
		- <b>BYTE3</b>:  optional third hexadecimal byte in the packet
		- <b>BYTE4</b>:  optional fourth hexadecimal byte in the packet
		- <b>BYTE5</b>:  optional fifth hexadecimal byte in the packet
  
		returns: NONE   
		*/

		writeTextPacket(DCCpp::mainRegs, com+1);
		break;

	case 'P':       
		/**	\addtogroup commandsGroup
		WRITE A DCC PACKE%T TO ONE OF THE REGISTERS DRIVING THE MAIN OPERATIONS TRACK
		-----------------------------------------------------------------------------

		<b>
		\verbatim
		<P REGISTER BYTE1 BYTE2 [BYTE3] [BYTE4] [BYTE5]>
		\endverbatim
		</b>

		writes a DCC packet of two, three, four, or five hexadecimal bytes to a register driving the programming track
		FOR DEBUGGING AND TESTING PURPOSES ONLY.  DO NOT USE UNLESS YOU KNOW HOW TO CONSTRUCT NMRA DCC PACKETS - YOU CAN INADVERTENTLY RE-PROGRAM YOUR ENGINE DECODER
   
		- <b>REGISTE%R</b>: an internal register number, from 0 through MAX_MAIN_REGISTERS (inclusive), to write (if REGISTE%R=0) or write and store (if REGISTE%R>0) the packet 
		- <b>BYTE1</b>:  first hexadecimal byte in the packet
		- <b>BYTE2</b>:  second hexadecimal byte in the packet
		- <b>BYTE3</b>:  optional third hexadecimal byte in the packet
		- <b>BYTE4</b>:  optional fourth hexadecimal byte in the packet
		- <b>BYTE5</b>:  optional fifth hexadecimal byte in the packet
   
		returns: NONE   
		*/

		writeTextPacket(DCCpp::progRegs, com+1);
		break;
			
#ifndef VISUALSTUDIO
	case 'F':     
		/**	\addtogroup commandsGroup
		ATTEMPTS TO DETERMINE HOW MUCH FREE SRAM IS AVAILABLE IN ARDUINO
		----------------------------------------------------------------

		<b>
		\verbatim
		<F>
		\endverbatim
		</b>

		measure amount of free SRAM memory left on the Arduino based on trick found on the Internet.
		Useful when setting dynamic array sizes, considering the Uno only has 2048 bytes of dynamic SRAM.
		Unfortunately not very reliable --- would be great to find a better method
     
		returns: <b>\<f MEM\></b>
		where MEM is the number of free bytes remaining in the Arduino's SRAM
		*/

		int v; 
		DCCPP_INTERFACE.print("<f");
		DCCPP_INTERFACE.print((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
		DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
		break;
#endif

	case 'L':     
		/**	\addtogroup commandsGroup
		LISTS BIT CONTENTS OF ALL INTERNAL DCC PACKE%T REGISTERS
		-------------------------------------------------------

		<b>
		\verbatim
		<L>
		\endverbatim
		</b>

		lists the packet contents of the main operations track registers and the programming track registers
		FOR DIAGNOSTIC AND TESTING USE ONLY
		*/

	  DCCPP_INTERFACE.println("");
	  for(Register *p = DCCpp::mainRegs.reg; p <= DCCpp::mainRegs.maxLoadedReg;p++){
		DCCPP_INTERFACE.print("M"); DCCPP_INTERFACE.print((int)(p - DCCpp::mainRegs.reg)); DCCPP_INTERFACE.print(":\t");
		DCCPP_INTERFACE.print((int)p); DCCPP_INTERFACE.print("\t");
		DCCPP_INTERFACE.print((int)(p->activePacket)); DCCPP_INTERFACE.print("\t");
		DCCPP_INTERFACE.print(p->activePacket->nBits); DCCPP_INTERFACE.print("\t");
		for(int i=0;i<10;i++){
		  DCCPP_INTERFACE.print(p->activePacket->buf[i],HEX); DCCPP_INTERFACE.print("\t");
		}
		DCCPP_INTERFACE.println("");
	  }
	  for(Register *p = DCCpp::progRegs.reg; p <= DCCpp::progRegs.maxLoadedReg;p++){
		DCCPP_INTERFACE.print("P"); DCCPP_INTERFACE.print((int)(p - DCCpp::progRegs.reg)); DCCPP_INTERFACE.print(":\t");
		DCCPP_INTERFACE.print((int)p); DCCPP_INTERFACE.print("\t");
		DCCPP_INTERFACE.print((int)p->activePacket); DCCPP_INTERFACE.print("\t");
		DCCPP_INTERFACE.print(p->activePacket->nBits); DCCPP_INTERFACE.print("\t");
		for(int i=0;i<10;i++){
		  DCCPP_INTERFACE.print(p->activePacket->buf[i],HEX); DCCPP_INTERFACE.print("\t");
		}
		DCCPP_INTERFACE.println("");
	  }
	  DCCPP_INTERFACE.println("");
	  break;
	default:
		DCCPP_INTERFACE.println(F("Unknown command"));

  } // switch
}; // SerialCommand::parse

///////////////////////////////////////////////////////////////////////////////


void TextCommand::setThrottle(char *s) 
{
	int nReg;
	int cab;
	int tSpeed;
	int tDirection;
  
	if (sscanf(s, "%d %d %d %d", &nReg, &cab, &tSpeed, &tDirection) != 4)
	{
#ifdef DCCPP_DEBUG_MODE
    	Serial.println(F("t Syntax error"));
#endif
		return;
	}

	DCCpp::mainRegs.setThrottle(nReg, cab, tSpeed, tDirection);

	DCCPP_INTERFACE.print("<T");
	DCCPP_INTERFACE.print(nReg); DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(cab); DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(tSpeed); DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(tDirection);
	DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
	DCCPP_INTERFACE.println("");
#endif
} // TextCommand::setThrottle(string)


void TextCommand::setFunction(char *s)
{
	int cab;
	int fByte, eByte;
	int nParams;

	nParams = sscanf(s, "%d %d %d", &cab, &fByte, &eByte);
	if (nParams<2)
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("f Syntax error"));
#endif
		return;
	}

	if (nParams == 2)                       // this is a request for functions FL,F1-F12  
		eByte = -1;

	DCCpp::mainRegs.setFunction(0, cab, fByte, eByte);	// TODO : nReg 0 is not valid !

#ifdef DCCPP_DEBUG_MODE
	Serial.print(F("{func addr="));
	Serial.print(cab); Serial.print(F("; fbyte="));
	Serial.print(fByte); Serial.print(F("; ebyte="));
	Serial.print(eByte);
	Serial.println("}");
#endif


} // TextCommand::setFunction(string)



void TextCommand::setAccessory(char *s) 
{
	int aAdd;                       // the accessory address (0-511 = 9 bits) 
	int aNum;                       // the accessory number within that address (0-3)
	int activate;                   // flag indicated whether accessory should be activated (1) or deactivated (0) following NMRA recommended convention

	if (sscanf(s, "%d %d %d", &aAdd, &aNum, &activate) != 3)
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("a Syntax error"));
#endif
		return;
	}

		DCCpp::mainRegs.setAccessory(aAdd, aNum, activate);

#ifdef DCCPP_DEBUG_MODE
	Serial.print(F("{acc id="));
	Serial.print(aAdd); Serial.print(F(" num="));
	Serial.print(aNum); Serial.print(F(" <- "));
	Serial.print(activate);
	Serial.println("}");
#endif

} // TextCommand::setAccessory(string)


void TextCommand::writeTextPacket(volatile RegisterList& rl, char *s)
{
	int nReg;
	byte b[6];
	int nBytes;

	nBytes = sscanf(s, "%d %hhx %hhx %hhx %hhx %hhx", &nReg, b, b+1, b+2, b+3, b+4) - 1;

	rl.writeTextPacket(nReg, b, nBytes);

#ifdef DCCPP_DEBUG_MODE
	Serial.print(F("{raw nreg="));
	Serial.print(nReg); Serial.print(" ln=");
	Serial.print(nBytes); Serial.print(": [");
	Serial.print(b[0]); Serial.print(",");
	Serial.print(b[1]); Serial.print(",");
	Serial.print(b[2]); Serial.print(",");
	Serial.print(b[3]); Serial.print(",");
	Serial.print(b[4]); Serial.print("] ");
	Serial.println("}");
#endif

} // TextCommand::writeTextPacket(string)


void TextCommand::readCV(volatile RegisterList& rl, char *s)
{
	int cv, callBack, callBackSub;
	int bValue;

	if (sscanf(s, "%d %d %d", &cv, &callBack, &callBackSub) != 3)          // cv = 1-1024
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("R Syntax error"));
#endif
		bValue = -1;
	} else {
		bValue = rl.readCV(cv, callBack, callBackSub);
	}
	
	DCCPP_INTERFACE.print("<r");
	DCCPP_INTERFACE.print(callBack);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(callBackSub);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(cv);
	DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(bValue);
	DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
	DCCPP_INTERFACE.println("");
#endif

} // TextCommand::readCV(string)



void TextCommand::writeVerifyCVByteProg(char *s)
{
	int bValue, cv, callBack, callBackSub;

	if (sscanf(s, "%d %d %d %d", &cv, &bValue, &callBack, &callBackSub) != 4)          // cv = 1-1024
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("W Syntax error"));
#endif
		return;
	}

	bool ret = DCCpp::progRegs.writeVerifyCVByte(cv, bValue, callBack, callBackSub);

	DCCPP_INTERFACE.print("<r");
	DCCPP_INTERFACE.print(callBack);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(callBackSub);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(cv);
	DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(ret ? bValue : -1);
	DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
	DCCPP_INTERFACE.println("");
#endif

} // TextCommand::writeCVByte(string)


void TextCommand::writeVerifyCVBitProg(char *s)
{
	int bNum, bValue, cv, callBack, callBackSub;

	if(sscanf(s,"%d %d %d %d %d",&cv,&bNum,&bValue,&callBack,&callBackSub) != 5)      // cv = 1-1024
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("W Syntax error"));
#endif
		return;
	}

	bool ret = DCCpp::progRegs.writeVerifyCVBit(cv, bNum, bValue, callBack, callBackSub);

	DCCPP_INTERFACE.print("<r");
	DCCPP_INTERFACE.print(callBack);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(callBackSub);
	DCCPP_INTERFACE.print("|");
	DCCPP_INTERFACE.print(cv);
	DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(bNum);
	DCCPP_INTERFACE.print(" ");
	DCCPP_INTERFACE.print(ret ? bValue : -1);
	DCCPP_INTERFACE.print(">");
#if !defined(USE_ETHERNET)
	DCCPP_INTERFACE.println("");
#endif


} // TextCommand::writeCVBit(string)


void TextCommand::writeCVByteMain(char *s) 
{
	int cab;
	int cv;
	int bValue;

	if (sscanf(s, "%d %d %d", &cab, &cv, &bValue) != 3)
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("w Syntax error"));
#endif
		return;
	}

	DCCpp::mainRegs.writeCVByteMain(cab, cv, bValue);

#ifdef DCCPP_DEBUG_MODE
	Serial.print(F("{wr byte main addr="));
	Serial.print(cab); Serial.print(" CV");
	Serial.print(cv); Serial.print(" <- ");
	Serial.print(bValue); Serial.print(" ");
	Serial.println("}");
#endif
} // TextCommand::writeCVByteMain(string)


void TextCommand::writeCVBitMain(char *s)
{
	int cab;
	int cv;
	int bNum;
	int bValue;

	if (sscanf(s, "%d %d %d %d", &cab, &cv, &bNum, &bValue) != 4)
	{
#ifdef DCCPP_DEBUG_MODE
		Serial.println(F("w Syntax error"));
#endif
		return;
	}

	DCCpp::mainRegs.writeCVBitMain(cab, cv, bNum, bValue);

#ifdef DCCPP_DEBUG_MODE
	Serial.print(F("{wr bit main addr="));
	Serial.print(cab); Serial.print(" CV");
	Serial.print(cv); Serial.print("[");
	Serial.print(bNum); Serial.print("] <- ");
	Serial.print(bValue); Serial.print(" ");
	Serial.println("}");
#endif

} // TextCommand::writeCVBitMain(string)






#endif
#endif