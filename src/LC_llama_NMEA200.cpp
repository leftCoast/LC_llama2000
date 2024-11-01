#include <LC_llama_NMEA200.h>
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


CANMsgObj* createMsgObj(msgTypes inType) {

   switch(inType) {
      case noType       : return NULL;
      case waterSpeed   : return (CANMsgObj*) new waterSpeedObj;
      case waterDepth   : return (CANMsgObj*) new waterDepthObj;
      case waterTemp    : return (CANMsgObj*) new waterTempObj;
      case fluidLevel    : return (CANMsgObj*) new fluidLevelObj;
   }
   return NULL;
}

uint32_t makeAddress (uint32_t pgn, uint8_t priority, uint8_t source) {
  return ((pgn << 8) | priority << 26) | source;
}

// ************ llama_NMEA200 ************


llama_NMEA200::llama_NMEA200(int inResetPin,int inIntPin)
  : linkList(), idler() {

  resetPin = inResetPin;
  intPin   = inIntPin;
}


llama_NMEA200::~llama_NMEA200(void) {  }


bool llama_NMEA200::begin(int inCSPin) {

  hookup();
  pinMode(resetPin, OUTPUT);
  delay(50);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  return CAN.begin(500E3);
}


CANMsgObj* llama_NMEA200::getMsgObj(uint32_t inPGN) {

  CANMsgObj* trace;
  
  trace = (CANMsgObj*)getFirst();
  while(trace) {
    if (trace->getPGN()==inPGN) {
      return trace;
    }
    trace = (CANMsgObj*)trace->getNext();
  }
  return NULL;
}


CANMsgObj* llama_NMEA200::getMsgObj(msgTypes inType) {

  CANMsgObj* trace;
  
  trace = (CANMsgObj*)getFirst();
  while(trace) {
    if (trace->getType()==inType) {
      return trace;
    }
    trace = (CANMsgObj*)trace->getNext();
  }
  return NULL;
}


bool llama_NMEA200::addMsgObj(msgTypes inType) {

   CANMsgObj* newMsgObj;
  
   if (!getMsgObj(inType)) {
      newMsgObj = createMsgObj(inType);
      if (!newMsgObj) {
          return false;
      } else {
         addToTop(newMsgObj);
      }
   }
   return true;
}


void llama_NMEA200::idle(void) {

   int         packetSize;
   uint32_t    theID;
   msg_t       msg;
   CANMsgObj* decodeObj;
   int         i;
   int          numBytes;

   packetSize = CAN.parsePacket();
   if (packetSize) {
      theID = CAN.packetId();
      readAddress (theID, &msg);
      decodeObj = (CANMsgObj*)getMsgObj(msg.pgn);
      if (decodeObj) {
         numBytes = decodeObj->getNumBytes();
         i = 0;
         while (CAN.available()&&i<numBytes) {
            decodeObj->dataBytes[i] = CAN.read();
            i++;
         }
         decodeObj->decodeMessage();
      }
   }
}


void llama_NMEA200::readAddress (uint32_t can_id, msg_t* msg) {
  
  uint32_t buffer = can_id;
  
  msg->sa = buffer & 0xFF;
  buffer = buffer >> 8;
  msg->pgn = buffer & 0x3FFFF;
  msg->ps = buffer & 0xFF;
  msg->dp = (buffer & 0xFF00) >> 8;
  msg->priority = (buffer & 0x1C0000) >> 18;
}



// ************* CANMsgObj *************


CANMsgObj::CANMsgObj(int inNumBytes)
   : linkListObj(), idler() {

   msgType = noType;
   msgPGN  = 0;
   numBytes = 0;
   dataBytes = NULL;
   Serial.print("Num data bytes : ");
   Serial.println(inNumBytes);
   if (resizeBuff(inNumBytes,&dataBytes)) {
      numBytes = inNumBytes;
   }
   intervaTimer.reset();   // Default to off.
}


CANMsgObj::~CANMsgObj(void) { resizeBuff(0,&dataBytes); }

msgTypes CANMsgObj::getType(void) { return msgType; }

uint32_t CANMsgObj::getPGN(void) { return msgPGN; }

int CANMsgObj::getNumBytes(void) { return numBytes; }


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
      hookup();
   } else {
      intervaTimer.reset();
   }
}
 

float CANMsgObj::getSendInterval(void) {  intervaTimer.getTime(); }
 
 
void  CANMsgObj::idle(void) {

   if (intervaTimer.ding()) {
      sendMessage();
      intervaTimer.stepTime();
   }
}
 
 
// ************* waterSpeedObj *************


waterSpeedObj::waterSpeedObj(void)
   : CANMsgObj() {

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


waterDepthObj::waterDepthObj(void)
   : CANMsgObj() {

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


waterTempObj::waterTempObj(void)
   : CANMsgObj() {

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


 fluidLevelObj::fluidLevelObj(void) 
   : CANMsgObj() {
   
   msgType  = fluidLevel;
   msgPGN   = 0x1F211;
   
   instance    = 0;
   fluidType   = fuel;  // This is 0.
   level       = 0;
   capacity    = 0;
   //periodTimer.setTime(2500);
}
 

fluidLevelObj::~fluidLevelObj(void) {  }


byte  fluidLevelObj::getInstance(void) { return instance; }

void  fluidLevelObj::setInstance(byte inInstance) { instance = inInstance; }

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
   
   address = makeAddress(msgPGN,6,0);
   
   dataBytes[0] = instance & 0b00001111;
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
