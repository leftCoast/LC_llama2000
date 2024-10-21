#include <LC_lama_NMEA200.h>
#include <resizeBuff.h>



int bytesTo16Int(byte hiByte,int lowByte) {
  
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

int bytesTo32int(byte hiByte,byte byte2,byte byte1,byte lowByte) {
  
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


CANMessage* createMsgObj(msgTypes inType) {

   switch(inType) {
      case noType       : return NULL;
      case waterSpeed   : return (CANMessage*) new waterSpeedObj;
      case waterDepth   : return (CANMessage*) new waterDepthObj;
      case waterTemp    : return (CANMessage*) new waterTempObj;
   }
   return NULL;
}



// ************ lama_NMEA200 ************


lama_NMEA200::lama_NMEA200(int inResetPin,int inIntPin)
  : linkList(),
  idler() {

  resetPin = inResetPin;
  intPin   = inIntPin;
}


lama_NMEA200::~lama_NMEA200(void) {  }


bool lama_NMEA200::begin(int inCSPin) {

  hookup();
  pinMode(resetPin, OUTPUT);
  delay(50);
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  return CAN.begin(500E3);
}


CANMessage* lama_NMEA200::getMsgObj(uint32_t inPGN) {

  CANMessage* trace;
  
  trace = (CANMessage*)getFirst();
  while(trace) {
    if (trace->getPGN()==inPGN) {
      return trace;
    }
    trace = (CANMessage*)trace->getNext();
  }
  return NULL;
}


CANMessage* lama_NMEA200::getMsgObj(msgTypes inType) {

  CANMessage* trace;
  
  trace = (CANMessage*)getFirst();
  while(trace) {
    if (trace->getType()==inType) {
      return trace;
    }
    trace = (CANMessage*)trace->getNext();
  }
  return NULL;
}


bool lama_NMEA200::addMsgObj(msgTypes inType) {

   CANMessage* newMsgObj;
  
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


void lama_NMEA200::idle(void) {

   int         packetSize;
   uint32_t    theID;
   msg_t       msg;
   CANMessage* decodeObj;
   int         i;
   int          numBytes;

   packetSize = CAN.parsePacket();
   if (packetSize) {
      theID = CAN.packetId();
      readAddress (theID, &msg);
      decodeObj = (CANMessage*)getMsgObj(msg.pgn);
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


void lama_NMEA200::readAddress (uint32_t can_id, msg_t* msg) {
  
  uint32_t buffer = can_id;
  
  msg->sa = buffer & 0xFF;
  buffer = buffer >> 8;
  msg->pgn = buffer & 0x3FFFF;
  msg->ps = buffer & 0xFF;
  msg->dp = (buffer & 0xFF00) >> 8;
  msg->priority = (buffer & 0x1C0000) >> 18;
}



// ************* CANMessage *************


CANMessage::CANMessage(int inNumBytes)
   : linkListObj() {

   msgType = noType;
   msgPGN  = 0;
   numBytes = 0;
   dataBytes = NULL;
   Serial.print("Num data bytes : ");
   Serial.println(inNumBytes);
   if (resizeBuff(inNumBytes,&dataBytes)) {
   numBytes = inNumBytes;
  }
}


CANMessage::~CANMessage(void) { resizeBuff(0,&dataBytes); }

msgTypes CANMessage::getType(void) { return msgType; }

uint32_t CANMessage::getPGN(void) { return msgPGN; }

int CANMessage::getNumBytes(void) { return numBytes; }


void CANMessage::showDataBytes(void) {
   
   for (int i=0;i<numBytes;i++) {
      Serial.print(dataBytes[i],DEC);
      Serial.print("\t");
   }
   Serial.println();
}
 
 
 
// ************* waterSpeedObj *************


waterSpeedObj::waterSpeedObj(void)
   : CANMessage() {

  msgType = waterSpeed;
  msgPGN  = 0x1F503;
  knots   = 0;
  speedMap.setValues(0,1023,0,(1023*1.943844)*0.01);
}


waterSpeedObj::~waterSpeedObj(void) {  }


void waterSpeedObj::decodeMessage(void) {

  unsigned int rawSpeed;
  
  rawSpeed = (unsigned int) bytesTo16Int(dataBytes[2],dataBytes[1]);
  knots = speedMap.map(rawSpeed);
}

  
float waterSpeedObj::getSpeed(void) { return knots; }



// ************* waterDepthObj *************


waterDepthObj::waterDepthObj(void)
   : CANMessage() {

  msgType = waterDepth;
  msgPGN  = 0x1F50B;
  feet   = 0;
}


waterDepthObj::~waterDepthObj(void) {  }


void waterDepthObj::decodeMessage(void) {

  unsigned int rawDepth;
  
  rawDepth = (unsigned int) bytesTo32int(dataBytes[4],dataBytes[3],dataBytes[2],dataBytes[1]);
  rawDepth = rawDepth / 100.0;     // Give meters.
  feet = rawDepth * 3.28084;     // Give feet
}

  
float waterDepthObj::getDepth(void) { return feet; }



// ************* waterTempObj *************


waterTempObj::waterTempObj(void)
   : CANMessage() {

   msgType  = waterTemp;
   msgPGN   = 0x1FD08;
   degF     = 0;
}


waterTempObj::~waterTempObj(void) {  }


void waterTempObj::decodeMessage(void) {

   unsigned int rawTemp;

   rawTemp  = (unsigned int) (unsigned int) bytesTo16Int(dataBytes[4],dataBytes[3]);
   degF  = rawTemp / 100.0;         // Give kelvan.
   degF  = (degF * 1.8) - 459.67;   // Give degF.
}

  
float waterTempObj::getTemp(void) { return degF; }
          
