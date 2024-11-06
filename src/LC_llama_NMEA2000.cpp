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


CANMsgObj* createMsgObj(ECU* inECU,msgTypes inType) {

   switch(inType) {
      case noType       : return NULL;
      case waterSpeed   : return (CANMsgObj*) new waterSpeedObj(inECU);
      case waterDepth   : return (CANMsgObj*) new waterDepthObj(inECU);
      case waterTemp    : return (CANMsgObj*) new waterTempObj(inECU);
      case fluidLevel    : return (CANMsgObj*) new fluidLevelObj(inECU);
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
  Serial.print("Looking for : ");Serial.print(inPGN);Serial.print("  instance ");Serial.println(inInstance);
  while(trace) {
    if (trace->getPGN()==inPGN && trace->getFunctInst()==inInstance) {
      return trace;
    }
    trace = (CANMsgObj*)trace->getNext();
  }
  return NULL;
}


CANMsgObj* llama_NMEA2000::getMsgObj(msgTypes inType,int inInstance) {

  CANMsgObj* trace;
  
  trace = (CANMsgObj*)getFirst();
  while(trace) {
    if (trace->getType()==inType && trace->getFunctInst()==inInstance) {
      return trace;
    }
    trace = (CANMsgObj*)trace->getNext();
  }
  return NULL;
}


bool llama_NMEA2000::addMsgObj(msgTypes inType,int inInstance) {

   CANMsgObj* newMsgObj;
  
   if (!getMsgObj(inType,inInstance)) {
      newMsgObj = createMsgObj(this,inType);
      if (!newMsgObj) {
          return false;
      } else {
      	newMsgObj->setFunctInst(inInstance);
         addToTop(newMsgObj);
      }
   }
   return true;
}


void llama_NMEA2000::handleMsg(uint32_t PGN) {

	CANMsgObj*	messageObj;
	int         i;
   int			numBytes;
   
	messageObj = (CANMsgObj*)getMsgObj(PGN);			// See we can find a messageObj (CA) that will handle this..
	if (messageObj) {											// If we do have a messageObj to handle it..
		numBytes = messageObj->getNumBytes();			// Read the size of it's storage buffer.
		i = 0;													// Starting at zero..
		while (CAN.available()&&i<numBytes) {			// While we have a byte to read and a place to put it..
			messageObj->dataBytes[i] = CAN.read();		// Read and store the byte into the messageObj.
			i++;													// Bump of the storage index.
		}															//
		messageObj->decodeMessage();						// All stored, let the messageObj deal with it.
	}
}


/*
void llama_NMEA2000::idle(void) {

   int         packetSize;
	uint32_t    theID;
   msg_t       msg;
   CANMsgObj*	messageObj;
   int         i;
   int			numBytes;

	ECU::idle();													// Let all the CANMsgObj(s) have time to do things.
   packetSize = CAN.parsePacket();							// Check to see if a packet came though.
   if (packetSize) {												// If we got a packet..
      theID = CAN.packetId();									// Read it's ID (PGN + ADDRESS).
      readAddress (theID, &msg);								// Decode the ID.
      messageObj = (CANMsgObj*)getMsgObj(msg.pgn);		// See we can find a messageObj (CA) that will handle this..
      if (messageObj) {											// If we do have a messageObj to handle it..
         numBytes = messageObj->getNumBytes();			// Read the size of it's storage buffer.
         i = 0;													// Starting at zero..
         while (CAN.available()&&i<numBytes) {			// While we have a byte to read and a place to put it..
            messageObj->dataBytes[i] = CAN.read();		// Read and store the byte into the messageObj.
            i++;													// Bump of the storage index.
         }															//
         messageObj->decodeMessage();						// All stored, let the messageObj deal with it.
      }
   }
}
*/

/*
// Decoding the 39 bit CAN header.
void llama_NMEA2000::readAddress (uint32_t can_id, msg_t* msg) {
  
  uint32_t buffer = can_id;
  
  msg->sa = buffer & 0xFF;
  buffer = buffer >> 8;
  msg->pgn = buffer & 0x3FFFF;
  msg->ps = buffer & 0xFF;
  msg->dp = (buffer & 0xFF00) >> 8;
  msg->priority = (buffer & 0x1C0000) >> 18;
}
*/


// ************* CANMsgObj *************


CANMsgObj::CANMsgObj(ECU* inECU)
   : CA(inECU) {

   msgType		= noType;				// No idea what type we'll be.
   msgPGN		= 0;						// Goes along with that.
   dataBytes	= NULL;					// So we can use resizeBuff().
   setNumBytes(DEF_NUM_DATA_BYTES);	// Set the default size.
   intervaTimer.reset();   			// Default to off.
}


CANMsgObj::~CANMsgObj(void) { resizeBuff(0,&dataBytes); }


msgTypes CANMsgObj::getType(void) { return msgType; }


uint32_t CANMsgObj::getPGN(void) { return msgPGN; }


int CANMsgObj::getNumBytes(void) { return numBytes; }


void CANMsgObj::setNumBytes(int inNumBytes) {

	if (resizeBuff(inNumBytes,&dataBytes)) {
      numBytes = inNumBytes;
   } else {
   	numBytes = 0;
   }
}


void CANMsgObj::showDataBytes(void) {
   
   for (int i=0;i<numBytes;i++) {
      Serial.print(dataBytes[i],DEC);
      Serial.print("\t");
   }
   Serial.println();
}
 
 
void CANMsgObj::setSendInterval(float inMs) {

   if (inMs>0) {
      intervaTimer.setTime(inMs);
   } else {
      intervaTimer.reset();
   }
}
 

float CANMsgObj::getSendInterval(void) {  return intervaTimer.getTime(); }
 
 
void  CANMsgObj::idleTime(void) {

   if (intervaTimer.ding()) {
      sendMessage();
      intervaTimer.stepTime();
   }
}
 


// ************* waterSpeedObj *************


waterSpeedObj::waterSpeedObj(ECU* inECU)
   : CANMsgObj(inECU) {

  msgType = waterSpeed;
  msgPGN  = 0x1F503;
  knots   = 0;
  speedMap.setValues(0,1023,0,(1023*1.943844)*0.01);
}


waterSpeedObj::~waterSpeedObj(void) {  }


void waterSpeedObj::decodeMessage(void) {

  unsigned int rawSpeed;
  
  rawSpeed = (unsigned int) pack16(dataBytes[2],dataBytes[1]);
  knots = speedMap.map(rawSpeed);
}

  
float waterSpeedObj::getSpeed(void) { return knots; }


void waterSpeedObj::sendMessage(void) { }



// ************* waterDepthObj *************


waterDepthObj::waterDepthObj(ECU* inECU)
   : CANMsgObj(inECU) {

  msgType = waterDepth;
  msgPGN  = 0x1F50B;
  feet   = 0;
}


waterDepthObj::~waterDepthObj(void) {  }


void waterDepthObj::decodeMessage(void) {

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

   msgType  = waterTemp;
   msgPGN   = 0x1FD08;
   degF     = 0;
}


waterTempObj::~waterTempObj(void) {  }


void waterTempObj::decodeMessage(void) {

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
   
   msgType		= fluidLevel;
   msgPGN		= 0x1F211;
	fluidType	= fuel;  // This is 0.
   level       = 0;
   setSendInterval(2500);	// Only set this if we send the stuff.
}
 

fluidLevelObj::~fluidLevelObj(void) {  }

tankType fluidLevelObj::getTankType() { return fluidType; }

void fluidLevelObj::setTankType(tankType inType) { fluidType = inType; }

float fluidLevelObj::getLevel(void) { return level; }

void fluidLevelObj::setLevel(float inLevel) { level = inLevel; }

float fluidLevelObj::getCapacity(void) { return capacity; }

void fluidLevelObj::setCapacity(float inCapacity) { capacity = inCapacity; }
          
void fluidLevelObj::decodeMessage(void) {

}


void fluidLevelObj::sendMessage(void) {
   
   int16_t  tempInt;
   int32_t  tempLong;
   uint32_t address;
   
   address = ourECU->makeAddress(msgPGN,6,0);
   
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
   //Serial.println("Data set we are going to send");
   //showDataBytes();
   CAN.beginExtendedPacket(address);
   for (int i=0;i<7;i++) {
      CAN.write(dataBytes[i]);
   }
   CAN.endPacket();
}
