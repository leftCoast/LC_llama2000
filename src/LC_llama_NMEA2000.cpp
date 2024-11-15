#include <LC_llama_NMEA2000.h>
#include <resizeBuff.h>



int pack16(byte hiByte,int lowByte) {
  
   struct int16 {
      byte lowByte;
      byte hiByte;
   };
   int16*   bytes;
   int      value;

   value = 0;                    // Shut up compiler.
   bytes = (int16*)&value;
   bytes->lowByte = lowByte;
   bytes->hiByte = hiByte;
   return(value);
}

int pack32(byte hiByte,byte byte2,byte byte1,byte lowByte) {
  
   struct int32 {
      byte byte0;
      byte byte1;
      byte byte2;
      byte byte3;
   };
   int32*   bytes;
   uint32_t value;

   value = 0;                    // Shut up compiler.
   bytes = (int32*)&value;
   bytes->byte3 = hiByte;
   bytes->byte2 = byte2;
   bytes->byte1 = byte1;
   bytes->byte0 = lowByte;
   return(value);
}


CANMsgObj* createMsgObj(ECU* inECU,uint32_t PGN) {

	switch(PGN) {
      
      case 0x1F503	: return (CANMsgObj*) new waterSpeedObj(inECU);
      case 0x1F50B   : return (CANMsgObj*) new waterDepthObj(inECU);
      case 0x1FD08	: return (CANMsgObj*) new waterTempObj(inECU);
      case 0x1F211   : return (CANMsgObj*) new fluidLevelObj(inECU);
      default			: return NULL;
   }
   return NULL;
}



// ************ llama_NMEA2000 ************


llama_NMEA2000::llama_NMEA2000(byte inECUInst,int inResetPin,int inIntPin)
  : ECU(inECUInst) {

  resetPin = inResetPin;
  intPin   = inIntPin;
}


llama_NMEA2000::~llama_NMEA2000(void) {  }


bool llama_NMEA2000::begin(int inCSPin) {

  hookup();
  pinMode(resetPin, OUTPUT);
  delay(50);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  return CAN.begin(500E3);
}


CANMsgObj* llama_NMEA2000::getMsgObj(uint32_t inPGN,int inInstance) {

  CANMsgObj* trace;
  
  trace = (CANMsgObj*)getFirst();
  while(trace) {
    if (trace->getPGN()==inPGN && trace->getFunctInst()==inInstance) {
      return trace;
    }
    trace = (CANMsgObj*)trace->getNext();
  }
  return NULL;
}


bool llama_NMEA2000::addMsgObj(uint32_t inPGN,int inInstance) {

   CANMsgObj* newMsgObj;
  
   if (!getMsgObj(inPGN,inInstance)) {
      newMsgObj = createMsgObj(this,inPGN);
      if (!newMsgObj) {
          return false;
      } else {
      	newMsgObj->setFunctInst(inInstance);
         addToTop(newMsgObj);
      }
   }
   return true;
}


void llama_NMEA2000::sendMessage(uint32_t PGN,byte priority,byte address,int numBytes,byte* data) {
	
   uint32_t header;
   
   Serial.println("sendMessage()");
   header = makeHeader(PGN,priority,address);
   Serial.print("writing\t");Serial.print(PGN,HEX);Serial.print('\t');
   Serial.print(priority); Serial.print('\t');
   Serial.print(address); Serial.print('\t');
   Serial.println(numBytes);
   CAN.beginExtendedPacket(header);
   for (int i=0;i<numBytes;i++) {
      CAN.write(data[i]);
   }
   CAN.endPacket();
}



void llama_NMEA2000::handlePacket(void) {

	CANMsgObj*	messageObj;
	int         i;
   int			numBytes;
   int         packetSize;
	uint32_t    theCANID;
   msgHeader	header;
	
	packetSize = CAN.parsePacket();							// Check to see if a packet came though.
   if (packetSize) {												// If we got a packet..
      theCANID = CAN.packetId();								// Read it's ID (PGN + ADDRESS).
      readHeader(theCANID, &header);						// Decode the ID.
		messageObj = (CANMsgObj*)getMsgObj(header.PGN);	// See if we can find a messageObj (CA) that will handle this..
		if (messageObj) {											// If we do have a messageObj to handle it..
			numBytes = messageObj->getNumBytes();			// Read the size of it's storage buffer.
			i = 0;													// Starting at zero..
			Serial.print(header.PGN,HEX);Serial.print('\t');
			while (CAN.available()&&i<numBytes) {			// While we have a byte to read and a place to put it..
				messageObj->dataBytes[i] = CAN.read();		// Read and store the byte into the messageObj.
				Serial.print(messageObj->dataBytes[i]);
				Serial.print('\t');
				i++;													// Bump of the storage index.
			}															//
			Serial.println();
			messageObj->handleMsg();							// All stored, let the messageObj deal with it.
		}
	}
}



// ************* CANMsgObj *************


CANMsgObj::CANMsgObj(ECU* inECU)
   : CA(inECU) { }


CANMsgObj::~CANMsgObj(void) { resizeBuff(0,&dataBytes); }


uint32_t CANMsgObj::getPGN(void) { return ourPGN; }


void CANMsgObj::showDataBytes(void) {
   
   for (int i=0;i<numBytes;i++) {
      Serial.print(dataBytes[i],DEC);
      Serial.print("\t");
   }
   Serial.println();
}
 
void CANMsgObj::sendMessage(void) { Serial.println("WHAT?!"); }


// ************* waterSpeedObj *************


waterSpeedObj::waterSpeedObj(ECU* inECU)
   : CANMsgObj(inECU) {

  //msgType = waterSpeed;
  ourPGN  = 0x1F503;
  knots   = 0;
  speedMap.setValues(0,1023,0,(1023*1.943844)*0.01);
}


waterSpeedObj::~waterSpeedObj(void) {  }


void waterSpeedObj::handleMsg(void) {

  unsigned int rawSpeed;
  
  rawSpeed = (unsigned int) pack16(dataBytes[2],dataBytes[1]);
  knots = speedMap.map(rawSpeed);
}

  
float waterSpeedObj::getSpeed(void) { return knots; }


void waterSpeedObj::sendMessage(void) { }



// ************* waterDepthObj *************


waterDepthObj::waterDepthObj(ECU* inECU)
   : CANMsgObj(inECU) {

  //msgType = waterDepth;
  ourPGN  = 0x1F50B;
  feet   = 0;
}


waterDepthObj::~waterDepthObj(void) {  }


void waterDepthObj::handleMsg(void) {

  unsigned int rawDepth;
  
  rawDepth = (unsigned int) pack32(dataBytes[4],dataBytes[3],dataBytes[2],dataBytes[1]);
  rawDepth = rawDepth / 100.0;     // Give meters.
  feet = rawDepth * 3.28084;     // Give feet
}

  
float waterDepthObj::getDepth(void) { return feet; }


void waterDepthObj::sendMessage(void) { }



// ************* waterTempObj *************


waterTempObj::waterTempObj(ECU* inECU)
   : CANMsgObj(inECU) {

   //msgType  = waterTemp;
   ourPGN   = 0x1FD08;
   degF     = 0;
}


waterTempObj::~waterTempObj(void) {  }


void waterTempObj::handleMsg(void) {

   unsigned int rawTemp;

   rawTemp  = (unsigned int) (unsigned int) pack16(dataBytes[4],dataBytes[3]);
   degF  = rawTemp / 100.0;         // Give kelvan.
   degF  = (degF * 1.8) - 459.67;   // Give degF.
}

  
float waterTempObj::getTemp(void) { return degF; }
          

void waterTempObj::sendMessage(void) { }



// ************* fluidLevelObj *************


 fluidLevelObj::fluidLevelObj(ECU* inECU) 
   : CANMsgObj(inECU) {
   
   ourPGN		= 0x1F211;
	fluidType	= fuel;  // This is 0.
   level       = 0;
}
 

fluidLevelObj::~fluidLevelObj(void) {  }

tankType fluidLevelObj::getTankType() { return fluidType; }

void fluidLevelObj::setTankType(tankType inType) { fluidType = inType; }

float fluidLevelObj::getLevel(void) { return level; }

void fluidLevelObj::setLevel(float inLevel) { level = inLevel; }

float fluidLevelObj::getCapacity(void) { return capacity; }

void fluidLevelObj::setCapacity(float inCapacity) { capacity = inCapacity; }
          
void fluidLevelObj::handleMsg(void) {

}


void fluidLevelObj::sendMessage(void) {
   
   int16_t  tempInt;
   int32_t  tempLong;

   dataBytes[0] = getFunctInst() & 0b00001111;
   dataBytes[0] = dataBytes[0]<<4;
   dataBytes[0] = dataBytes[0] | ((byte)fluidType & 0b00001111);
   
   tempInt = round(level*250);
   dataBytes[1] = tempInt & 0x00FF;
   dataBytes[2] = (tempInt & 0xFF00)>>8;
   tempLong = round(capacity * 10);
   dataBytes[3] = tempLong & 0x000000FF;
   dataBytes[4] = (tempLong & 0x0000FF00)>>8;
   dataBytes[5] = (tempLong & 0x00FF0000)>>16;
   dataBytes[6] = (tempLong & 0xFF000000)>>24;
   dataBytes[7] = 0xFF;                            // Reserved, so..
   ourECU->sendMessage(ourPGN,6,0,8,dataBytes);
}
