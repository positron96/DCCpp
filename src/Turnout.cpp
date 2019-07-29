/**********************************************************************

Turnout.cpp, renamed from Accessories.cpp
COPYRIGHT (c) 2013-2016 Gregg E. Berman

Part of DCC++ BASE STATION for the Arduino

**********************************************************************/
#include "Arduino.h"
#ifdef ARDUINO_ARCH_AVR

#include "DCCpp.h"

#ifdef USE_TURNOUT

#ifdef VISUALSTUDIO
#include "string.h"
#endif

#include "Turnout.h"
#include "DCCpp_Uno.h"
//#include "Comm.h"



#ifdef USE_EEPROM
#include "EEStore.h"
#include "EEPROM.h"
#endif

///////////////////////////////////////////////////////////////////////////////

bool Turnout::begin(int id, int add, int subAdd) {
#if defined(USE_EEPROM)	|| defined(USE_TEXTCOMMAND)
#if defined(USE_EEPROM)	&& defined(DCCPP_DEBUG_MODE)
	if (strncmp(EEStore::data.id, EESTORE_ID, sizeof(EESTORE_ID)) != 0) {    // check to see that eeStore contains valid DCC++ ID
		DCCPP_INTERFACE.println(F("Turnout::begin() must be called BEFORE DCCpp.begin() !"));
	}
#endif
	if (firstTurnout == NULL) {
		firstTurnout = this;
	}
	else if (get(id) == NULL) {
		Turnout *tt = firstTurnout;
		while (tt->nextTurnout != NULL)
			tt = tt->nextTurnout;
		tt->nextTurnout = this;
	} else {
		// this ID already present
		return false;
	}
#endif

	this->set(id, add, subAdd);
	return true;

#endif
}

///////////////////////////////////////////////////////////////////////////////

void Turnout::set(int id, int add, int subAdd) {
	this->data.id = id;
	this->data.address = add;
	this->data.subAddress = subAdd;
	this->data.tStatus = 0;
}

///////////////////////////////////////////////////////////////////////////////

void Turnout::activate(int s) {
	data.tStatus = (s>0);                                    // if s>0 set turnout=ON, else if zero or negative set turnout=OFF
	DCCpp::mainRegs.setAccessory(this->data.address, this->data.subAddress, this->data.tStatus);
#ifdef USE_EEPROM
	if (this->eepromPos>0)
#ifdef VISUALSTUDIO
		EEPROM.put(this->eepromPos, (void *) &(this->data.tStatus), sizeof(int));	// ArduiEmulator version...
#else
		EEPROM.put(this->eepromPos, this->data.tStatus);
#endif
#endif

}

#if defined(USE_EEPROM)	|| defined(USE_TEXTCOMMAND)
///////////////////////////////////////////////////////////////////////////////

Turnout* Turnout::get(int id) {
	Turnout *tt;
	for (tt = firstTurnout; tt != NULL && tt->data.id != id; tt = tt->nextTurnout)
		;
	return(tt);
}

///////////////////////////////////////////////////////////////////////////////
bool Turnout::remove(int id) {
	Turnout *tt, *pp;

	for (tt = firstTurnout, pp = NULL; tt != NULL && tt->data.id != id; pp = tt, tt = tt->nextTurnout)
		;

	if (tt == NULL) {
		return false;
	}

	if (tt == firstTurnout)
		firstTurnout = tt->nextTurnout;
	else
		pp->nextTurnout = tt->nextTurnout;

	free(tt);

	return true;
}

///////////////////////////////////////////////////////////////////////////////

int Turnout::count() {
	int count = 0;
	Turnout *tt;
	for (tt = firstTurnout; tt != NULL; tt = tt->nextTurnout)
		count++;
	return count;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef USE_EEPROM
void Turnout::load() {
	struct TurnoutData data;
	Turnout *tt;

	// #ifdef DCCPP_DEBUG_MODE
	// Serial.println("Loading "+String(EEStore::data.nTurnouts)+" turnouts; addr="+EEStore::pointer() );	
	// #endif

	for (int i = 0; i<EEStore::data.nTurnouts; i++) {
#ifdef VISUALSTUDIO
		EEPROM.get(EEStore::pointer(), (void *)&data, sizeof(TurnoutData));
#else
		EEPROM.get(EEStore::pointer(), data);
#endif
#if defined(USE_TEXTCOMMAND)
		tt = create(data.id, data.address, data.subAddress);
#else
		tt = get(data.id);
#ifdef DCCPP_DEBUG_MODE
		if (tt == NULL)
			Serial.println(F("Turnout::begin() must be called BEFORE Turnout::load() !"));
		else
#endif
			tt->set(data.id, data.address, data.subAddress);
#endif
		// #ifdef DCCPP_DEBUG_MODE
		// Serial.println("Loaded id="+String(data.id)+"; "+EEStore::pointer() );	
		// #endif
		tt->data.tStatus = data.tStatus;
		tt->eepromPos = EEStore::pointer();
		EEStore::advance(sizeof(tt->data));
	}
}

///////////////////////////////////////////////////////////////////////////////

void Turnout::store() {
	Turnout *tt;

	tt = firstTurnout;
	EEStore::data.nTurnouts = 0;

	while (tt != NULL) {
		tt->eepromPos = EEStore::pointer();
#ifdef VISUALSTUDIO
		EEPROM.put(EEStore::pointer(), (void *) &(tt->data), sizeof(TurnoutData));	// ArduiEmulator version...
#else
		EEPROM.put(EEStore::pointer(), tt->data);
#endif
		// #ifdef DCCPP_DEBUG_MODE
		// Serial.println("Storing id="+String(tt->data.id)+"; addr="+EEStore::pointer() );
		// #endif
		EEStore::advance(sizeof(tt->data));
		tt = tt->nextTurnout;
		EEStore::data.nTurnouts++;
	}
}
#endif


Turnout *Turnout::create(int id, int add, int subAdd) {
	Turnout *tt = new Turnout();

	if (tt == NULL) {       // problem allocating memory
		return(tt);
	}

	if( !tt->begin(id, add, subAdd) ) {
		delete tt;
		return(NULL);
	}

	return(tt);
}


#if defined(USE_EEPROM)	|| defined(USE_TEXTCOMMAND)

///////////////////////////////////////////////////////////////////////////////


Turnout *Turnout::firstTurnout = NULL;
#endif //defined(USE_EEPROM)	|| defined(USE_TEXTCOMMAND)

#endif //USE_TURNOUT
#endif