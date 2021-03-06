/**********************************************************************

PacketRegister.cpp
COPYRIGHT (c) 2013-2016 Gregg E. Berman
Adapted by Thierry PARIS

Part of DCC++ BASE STATION for the Arduino
**********************************************************************/

#include "Arduino.h"
#ifdef ARDUINO_ARCH_AVR

#include "DCCpp.h"
//#include "DCCpp_Uno.h"
//#include "PacketRegister.h"
//#include "Comm.h"

#ifdef USE_ETHERNET
uint8_t DCCppConfig::EthernetIp[4];
uint8_t DCCppConfig::EthernetMac[6];
int DCCppConfig::EthernetPort = 0;

EthernetProtocol DCCppConfig::Protocol = EthernetProtocol::TCP;
#endif

byte DCCppConfig::SignalEnablePinMain = UNDEFINED_PIN;
byte DCCppConfig::CurrentMonitorMain = UNDEFINED_PIN;
					
byte DCCppConfig::SignalEnablePinProg = UNDEFINED_PIN;
byte DCCppConfig::CurrentMonitorProg = UNDEFINED_PIN;
					
uint8_t	DCCppConfig::SignalPortMaskMain = 0;
uint8_t	DCCppConfig::SignalPortMaskProg = 0;

volatile uint8_t *DCCppConfig::SignalPortInMain = 0;
volatile uint8_t *DCCppConfig::SignalPortInProg = 0;

///////////////////////////////////////////////////////////////////////////////

void Register::initPackets(){
  activePacket=packet;
  updatePacket=packet+1;
} // Register::initPackets

///////////////////////////////////////////////////////////////////////////////
	
RegisterList::RegisterList(int maxNumRegs){
  this->maxNumRegs=maxNumRegs;
  reg=(Register *)calloc((maxNumRegs+1),sizeof(Register));
  for(int i=0;i<=maxNumRegs;i++)
	reg[i].initPackets();
  regMap=(Register **)calloc((maxNumRegs+1),sizeof(Register *));
  speedTable=(int *)calloc((maxNumRegs+1),sizeof(int *));
  currentReg=reg;
  regMap[0]=reg;
  maxLoadedReg=reg;
  nextReg=NULL;
  currentBit=0;
  nRepeat=0;
} // RegisterList::RegisterList
  
///////////////////////////////////////////////////////////////////////////////

// LOAD DCC PACKET INTO TEMPORARY REGISTER 0, OR PERMANENT REGISTERS 1 THROUGH DCC_PACKET_QUEUE_MAX (INCLUSIVE)
// CONVERTS 2, 3, 4, OR 5 BYTES INTO A DCC BIT STREAM WITH PREAMBLE, CHECKSUM, AND PROPER BYTE SEPARATORS
// BITSTREAM IS STORED IN UP TO A 10-BYTE ARRAY (USING AT MOST 76 OF 80 BITS)

void RegisterList::loadPacket(int nReg, byte *b, int nBytes, int nRepeat, int printFlag) volatile 
{  
#ifdef VISUALSTUDIO
  return;
#endif
  nReg=nReg%((maxNumRegs+1));          // force nReg to be between 0 and maxNumRegs, inclusive

  while(nextReg!=NULL);              // pause while there is a Register already waiting to be updated -- nextReg will be reset to NULL by interrupt when prior Register updated fully processed
 
  if(regMap[nReg]==NULL)              // first time this Register Number has been called
   regMap[nReg]=maxLoadedReg+1;       // set Register Pointer for this Register Number to next available Register
 
  Register *r=regMap[nReg];           // set Register to be updated
  Packet *p=r->updatePacket;          // set Packet in the Register to be updated
  byte *buf=p->buf;                   // set byte buffer in the Packet to be updated
		  
  b[nBytes]=b[0];                        // copy first byte into what will become the checksum byte  
  for(int i=1;i<nBytes;i++)              // XOR remaining bytes into checksum byte
	b[nBytes]^=b[i];
  nBytes++;                              // increment number of bytes in packet to include checksum byte
	  
  buf[0]=0xFF;                        // first 8 bytes of 22-byte preamble
  buf[1]=0xFF;                        // second 8 bytes of 22-byte preamble
  buf[2]=0xFC + bitRead(b[0],7);      // last 6 bytes of 22-byte preamble + data start bit + b[0], bit 7
  buf[3]=b[0]<<1;                     // b[0], bits 6-0 + data start bit
  buf[4]=b[1];                        // b[1], all bits
  buf[5]=b[2]>>1;                     // b[2], bits 7-1
  buf[6]=b[2]<<7;                     // b[2], bit 0
  
  if(nBytes==3){
	p->nBits=49;
  } else{
	buf[6]+=b[3]>>2;                  // b[3], bits 7-2
	buf[7]=b[3]<<6;                   // b[3], bit 1-0
	if(nBytes==4){
	  p->nBits=58;
	} else{
	  buf[7]+=b[4]>>3;                // b[4], bits 7-3
	  buf[8]=b[4]<<5;                 // b[4], bits 2-0
	  if(nBytes==5){
		p->nBits=67;
	  } else{
		buf[8]+=b[5]>>4;              // b[5], bits 7-4
		buf[9]=b[5]<<4;               // b[5], bits 3-0
		p->nBits=76;
	  } // >5 bytes
	} // >4 bytes
  } // >3 bytes
  
  nextReg=r;
  this->nRepeat=nRepeat;
  maxLoadedReg=max(maxLoadedReg,nextReg);
  
#ifdef DCCPP_DEBUG_MODE
  if(printFlag)       // for debugging purposes
		printPacket(nReg,b,nBytes,nRepeat);  
#endif

} // RegisterList::loadPacket

///////////////////////////////////////////////////////////////////////////////

void RegisterList::setThrottle(int nReg, int cab, int tSpeed, int tDirection) volatile 
{
	byte b[5];                      // save space for checksum byte
	byte nB = 0;

	if (cab>127)
		b[nB++] = highByte(cab) | 0xC0;      // convert train number into a two-byte address

	b[nB++] = lowByte(cab);
	b[nB++] = 0x3F;                        // 128-step speed control byte
	if (tSpeed >= 0)
		b[nB++] = tSpeed + (tSpeed>0) + tDirection * 128;   // max speed is 126, but speed codes range from 2-127 (0=stop, 1=emergency stop)
	else {
		b[nB++] = 1;
		tSpeed = 0;
	}

	loadPacket(nReg, b, nB, 0, 1);

	speedTable[nReg] = tDirection == 1 ? tSpeed : -tSpeed;

} // RegisterList::setThrottle(ints)


///////////////////////////////////////////////////////////////////////////////

void RegisterList::setFunction(int nReg, int cab, int fByte, int eByte) volatile
{
	byte b[5];                      // save space for checksum byte
	byte nB = 0;

	if (cab>127)
		b[nB++] = highByte(cab) | 0xC0;      // convert train number into a two-byte address

	b[nB++] = lowByte(cab);

	if (eByte < 0) {                      // this is a request for functions FL,F1-F12  
		b[nB++] = (fByte | 0x80) & 0xBF;     // for safety this guarantees that first nibble of function byte will always be of binary form 10XX which should always be the case for FL,F1-F12  
	}
	else {                             // this is a request for functions F13-F28
		b[nB++] = (fByte | 0xDE) & 0xDF;     // for safety this guarantees that first byte will either be 0xDE (for F13-F20) or 0xDF (for F21-F28)
		b[nB++] = eByte;
	}

	/* NMRA DCC norm ask for two DCC packets instead of only one:
	"Command Stations that generate these packets, and which are not periodically refreshing these functions,
	must send at least two repetitions of these commands when any function state is changed."
	https://www.nmra.org/sites/default/files/s-9.2.1_2012_07.pdf
	*/
	loadPacket(nReg, b, nB, 4, 1);
} // RegisterList::setFunction(ints)


  ///////////////////////////////////////////////////////////////////////////////

void RegisterList::setAccessory(int aAdd, int aNum, int activate) volatile 
{
	byte b[3];                      // save space for checksum byte

	b[0] = aAdd % 64 + 128;         // first byte is of the form 10AAAAAA, where AAAAAA represent 6 least significant bits of accessory address  
	b[1] = ((((aAdd / 64) % 8) << 4) + (aNum % 4 << 1) + activate % 2) ^ 0xF8;      // second byte is of the form 1AAACDDD, where C should be 1, and the least significant D represent activate/deactivate

	loadPacket(0, b, 2, 4, 1);

} // RegisterList::setAccessory(ints)


  ///////////////////////////////////////////////////////////////////////////////

void RegisterList::writeTextPacket(int nReg, byte *b, int nBytes) volatile 
{

	if (nBytes<2 || nBytes>5) {    // invalid valid packet
		DCCPP_INTERFACE.print("<mInvalid Packet>");
#if !defined(USE_ETHERNET)
		DCCPP_INTERFACE.println("");
#endif
		return;
	}

	loadPacket(nReg, b, nBytes, 0, 1);

} // RegisterList::writeTextPacket(bytes)


  ///////////////////////////////////////////////////////////////////////////////

int RegisterList::readCV(int cv, int callBack, int callBackSub) volatile
{
	byte bRead[4];
	int bValue;
	int c, d, base;

	cv--;                              // actual CV addresses are cv-1 (0-1023)

	byte monitorPin = DCCppConfig::CurrentMonitorProg;
	if (DCCpp::IsMainTrack(this))
		monitorPin = DCCppConfig::CurrentMonitorMain;

	if (monitorPin == UNDEFINED_PIN)
		return -1;

	bRead[0] = 0x78 + (highByte(cv) & 0x03);   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
	bRead[1] = lowByte(cv);

	bValue = 0;

	int v;

	for (int i = 0; i<8; i++) {

		c = 0;
		d = 0;
		base = 0;

		//Serial.print("bit "+String(i) );


		for (int j = 0; j < ACK_BASE_COUNT; j++)
		{
			v = (int)analogRead(monitorPin);
			base += v;
		}
		base /= ACK_BASE_COUNT;


		bRead[2] = 0xE8 + i;

		loadPacket(0, resetPacket, 2, 3);          // NMRA recommends starting with 3 reset packets
		loadPacket(0, bRead, 3, 5);                // NMRA recommends 5 verify packets
		loadPacket(0, resetPacket, 2, 1);          // forces code to wait until all repeats of bRead are completed (and decoder begins to respond)

		for (int j = 0; j<ACK_SAMPLE_COUNT; j++)
		{
			v = (int)analogRead(monitorPin);
			c = (int)(ACK_MUL*(v - base)*ACK_SAMPLE_SMOOTHING + c*(1.0 - ACK_SAMPLE_SMOOTHING));
			//if(v!= 0) Serial.print(String(v)+"("+c+") ");
			if (c>ACK_SAMPLE_THRESHOLD*ACK_MUL) {
				d = 1;
				//Serial.print(" ### ");
			}
		}

		bitWrite(bValue, i, d);
		//Serial.println("");
	}

	c = 0;
	d = 0;
	base = 0;

	for (int j = 0; j<ACK_BASE_COUNT; j++)
		base += analogRead(monitorPin);
	base /= ACK_BASE_COUNT;

	bRead[0] = 0x74 + (highByte(cv) & 0x03);   // set-up to re-verify entire byte
	bRead[2] = bValue;

	loadPacket(0, resetPacket, 2, 3);          // NMRA recommends starting with 3 reset packets
	loadPacket(0, bRead, 3, 5);                // NMRA recommends 5 verify packets
	loadPacket(0, resetPacket, 2, 1);          // forces code to wait until all repeats of bRead are completed (and decoder begins to respond)

	//Serial.print("verify ");
	for (int j = 0; j<ACK_SAMPLE_COUNT; j++) {
		v = analogRead(monitorPin);
		c = (int)(ACK_MUL*(v - base)*ACK_SAMPLE_SMOOTHING + c*(1.0 - ACK_SAMPLE_SMOOTHING));
		//if(v!= 0) Serial.print(String(v)+"("+c+") ");
		if (c>ACK_SAMPLE_THRESHOLD*ACK_MUL) {
			d = 1;
			//Serial.print("+ ");
		}
	}

	//Serial.println("");

	if (d == 0)    // verify unsuccessful
		bValue = -1;

	return bValue;
}


///////////////////////////////////////////////////////////////////////////////

bool RegisterList::writeVerifyCVByte(int cv, int bValue, int callBack, int callBackSub) volatile 
{
	byte bWrite[4];
	int c, d, base, v;

	cv--;                              // actual CV addresses are cv-1 (0-1023)

	bWrite[0] = 0x7C + (highByte(cv) & 0x03);   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
	bWrite[1] = lowByte(cv);
	bWrite[2] = bValue;

	byte monitorPin = DCCppConfig::CurrentMonitorProg;
	if (DCCpp::IsMainTrack(this))
		monitorPin = DCCppConfig::CurrentMonitorMain;

	if (monitorPin != UNDEFINED_PIN)
	{
		base = 0;
		for (int j = 0; j < ACK_BASE_COUNT; j++) {
			v = analogRead(monitorPin);
			//Serial.println(v);
			base += v;
		}
		base /= ACK_BASE_COUNT;
	}

	loadPacket(0, resetPacket, 2, 1);
	loadPacket(0, bWrite, 3, 4);
	loadPacket(0, resetPacket, 2, 1);
	loadPacket(0, idlePacket, 2, 10);

	// If monitor pin undefined, write cv without any confirmation...
	if (monitorPin != UNDEFINED_PIN)
	{
		c = 0;
		d = 0;

		//Serial.println("verify; base="+String(base) );

		bWrite[0] = 0x74 + (highByte(cv) & 0x03);   // set-up to re-verify entire byte

		loadPacket(0, resetPacket, 2, 3);          // NMRA recommends starting with 3 reset packets
		loadPacket(0, bWrite, 3, 5);               // NMRA recommends 5 verify packets
		loadPacket(0, resetPacket, 2, 1);          // forces code to wait until all repeats of bRead are completed (and decoder begins to respond)

		for (int j = 0; j < ACK_SAMPLE_COUNT; j++) {
			v = analogRead(monitorPin);
			c = (int)(ACK_MUL*(v - base)*ACK_SAMPLE_SMOOTHING + c*(1.0 - ACK_SAMPLE_SMOOTHING));
			//if(v!= 0) Serial.print(String(v)+"("+c+") ");
			if (c > ACK_SAMPLE_THRESHOLD*ACK_MUL) {
				d = 1;
				//Serial.print("+ ");
			}
		}

		return d == 1;    // verify successful
	}

	return false;

} // RegisterList::writeCVByte(ints)


  ///////////////////////////////////////////////////////////////////////////////

bool RegisterList::writeVerifyCVBit(int cv, int bNum, int bValue, int callBack, int callBackSub) volatile 
{
	byte bWrite[4];
	int c, d, base, v;

	cv--;                              // actual CV addresses are cv-1 (0-1023)
	bValue = bValue % 2;
	bNum = bNum % 8;

	bWrite[0] = 0x78 + (highByte(cv) & 0x03);   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
	bWrite[1] = lowByte(cv);
	bWrite[2] = 0xF0 + bValue * 8 + bNum;

	byte monitorPin = DCCppConfig::CurrentMonitorProg;
	if (DCCpp::IsMainTrack(this))
		monitorPin = DCCppConfig::CurrentMonitorMain;

	if (monitorPin != UNDEFINED_PIN)
	{
		base = 0;

		for (int j = 0; j < ACK_BASE_COUNT; j++) {
			v = analogRead(monitorPin);
			base += v;
		}
		base /= ACK_BASE_COUNT;
	}

	loadPacket(0, resetPacket, 2, 1);
	loadPacket(0, bWrite, 3, 4);
	loadPacket(0, resetPacket, 2, 1);
	loadPacket(0, idlePacket, 2, 10);

	// If monitor pin undefined, write cv without any confirmation...
	if (monitorPin != UNDEFINED_PIN)
	{
		c = 0;
		d = 0;

		bitClear(bWrite[2], 4);              // change instruction code from Write Bit to Verify Bit

		loadPacket(0, resetPacket, 2, 3);          // NMRA recommends starting with 3 reset packets
		loadPacket(0, bWrite, 3, 5);               // NMRA recommends 5 verfy packets
		loadPacket(0, resetPacket, 2, 1);          // forces code to wait until all repeats of bRead are completed (and decoder begins to respond)

		for (int j = 0; j < ACK_SAMPLE_COUNT; j++) {
			v = analogRead(monitorPin);
			c = (int)((ACK_MUL*(v - base)*ACK_SAMPLE_SMOOTHING + c*(1.0 - ACK_SAMPLE_SMOOTHING)));
			if (c > ACK_SAMPLE_THRESHOLD*ACK_MUL)
				d = 1;
		}

		return d == 1;    // verify unsuccessful
	}

	return false;

} // RegisterList::writeCVBit(ints)



///////////////////////////////////////////////////////////////////////////////

void RegisterList::writeCVByteMain(int cab, int cv, int bValue) volatile 
{
	byte b[6];                      // save space for checksum byte
	byte nB = 0;

	cv--;

	if (cab>127)
		b[nB++] = highByte(cab) | 0xC0;      // convert train number into a two-byte address

	b[nB++] = lowByte(cab);
	b[nB++] = 0xEC + (highByte(cv) & 0x03);   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
	b[nB++] = lowByte(cv);
	b[nB++] = bValue;

	loadPacket(0, b, nB, 4);

} // RegisterList::writeCVByteMain(ints)


  ///////////////////////////////////////////////////////////////////////////////

void RegisterList::writeCVBitMain(int cab, int cv, int bNum, int bValue) volatile 
{
	byte b[6];                      // save space for checksum byte
	byte nB = 0;

	cv--;

	bValue = bValue % 2;
	bNum = bNum % 8;

	if (cab>127)
		b[nB++] = highByte(cab) | 0xC0;      // convert train number into a two-byte address

	b[nB++] = lowByte(cab);
	b[nB++] = 0xE8 + (highByte(cv) & 0x03);   // any CV>1023 will become modulus(1024) due to bit-mask of 0x03
	b[nB++] = lowByte(cv);
	b[nB++] = 0xF0 + bValue * 8 + bNum;

	loadPacket(0, b, nB, 4);

} // RegisterList::writeCVBitMain(ints)


///////////////////////////////////////////////////////////////////////////////

#ifdef DCCPP_DEBUG_MODE
void RegisterList::printPacket(int nReg, byte *b, int nBytes, int nRepeat) volatile 
{  
	DCCPP_INTERFACE.print(F("{packet nreg="));
	DCCPP_INTERFACE.print(nReg);
	DCCPP_INTERFACE.print(": [");
  for(int i=0;i<nBytes;i++){
	  DCCPP_INTERFACE.print(" ");
	  DCCPP_INTERFACE.print(b[i],HEX);
  }
  DCCPP_INTERFACE.print(F("] repeat="));
  DCCPP_INTERFACE.print(nRepeat);
  DCCPP_INTERFACE.print("}");
#if !defined(USE_ETHERNET)
  DCCPP_INTERFACE.println("");
#endif
} // RegisterList::printPacket()
#endif

///////////////////////////////////////////////////////////////////////////////

byte RegisterList::idlePacket[3]={0xFF,0x00,0};                 // always leave extra byte for checksum computation
byte RegisterList::resetPacket[3]={0x00,0x00,0};

byte RegisterList::bitMask[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};         // masks used in interrupt routine to speed the query of a single bit in a Packet
#endif