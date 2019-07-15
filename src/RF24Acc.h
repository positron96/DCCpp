#pragma once

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"



class RF24Acc {
	private:  
	static RF24 radio;
	static RF24Network network;
	static RF24Mesh mesh;

	public:
	
	static int begin();
	static void loop();
	static bool sendAccessory(int aAddr, int aNum, bool activate);

};