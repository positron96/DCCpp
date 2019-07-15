#include "RF24Acc.h"

RF24 RF24Acc::radio(9,10);
RF24Network RF24Acc::network(radio);
RF24Mesh RF24Acc::mesh(radio,network);

	
int RF24Acc::begin() {
  bool ret = radio.begin();
  if (!ret) {
    return -1;
  }
  radio.setPALevel(RF24_PA_MIN);
  mesh.setNodeID(0);
  ret = mesh.begin();
  if(!ret) {
    return -2;
  }
  return 0;
}

void RF24Acc::loop() {
  mesh.update();
  mesh.DHCP();
  if(network.available()){
    RF24NetworkHeader header;
    char msg[60];
    int ln = network.read(header,msg,30); 
    Serial.print( (char)header.type);
    Serial.print(": ");
    msg[ln]=0;
    Serial.println( msg );
  }
}

bool RF24Acc::sendAccessory(int aAddr, int aNum, bool activate) {
   
  //Serial.println( String("accessory to ")+aAddr+"; Ch:"+aNum+"; V="+activate );

  if(aAddr>255) 
    return false;

  // twin coil modification
  aNum *= 2;
  if(activate) {
    aNum += 1;
  }
  activate = 1;

  uint8_t payload[] = { (uint8_t)aNum, (uint8_t)activate };
  bool ret = mesh.write( &payload[0], 'O', 2,  aAddr);
  if(!ret) {
    Serial.println( F("RF24 send failed") );
    return false;
  }
  return true;
  
}
 

