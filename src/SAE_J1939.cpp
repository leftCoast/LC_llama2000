#include <SAE_J1939.h>


//				----- message class -----

/*
// Decoding the 29 bit CAN/J1939 header.
struct msgHeader {
	uint32_t  PGN;				// Type of data (Parameter group number)
	uint8_t   priority;		// CAN priority bits.
	bool      R;				// Reserve bit.
	bool      DP;				// Data page.
	uint8_t   PDUf;			// PDU format
	uint8_t   PDUs;			// PDU specific.
	uint8_t   sourceAddr;	// Who sent this?
};
*/


message::message(int inNumBytes) {
		
		priority		= DEF_PRIORITY;	// Something to get us going.
		R				= DEF_R;				// Reserve bit.
		DP				= DEF_DP;			// Data page.
		PDUf			= 0;					// PDU format
		PDUs			= 0;					// PDU specific.
		sourceAddr	= 0;					// Who sent this?
		numBytes		= 0;					// Because now, it is.
		msgData		= NULL;				// Default so we can use resizeBuff().
		setNumBytes(inNumBytes);		// Set the default size.
}


message::~message(void) { setNumBytes(0); }

		
void message::setNumBytes(int inNumBytes) {

	if (inNumBytes != numBytes) {
		if (resizeBuff(inNumBytes,&msgData)) {
			numBytes = inNumBytes;
		} else {
			numBytes = 0;
		}
	}
}


int message::getNumBytes(void) { return numBytes; }

		
void message::setCANID(uint32_t CANID) {
  
  sourceAddr	= CANID & 0xFF;
  CANID			= CANID >> 8;
  PDUs			= CANID & 0xFF;
  CANID			= CANID >> 8;
  PDUf			= CANID & 0xFF;
  CANID			= CANID >> 8;
  DP				= CANID & 0x01;
  CANID			= CANID >> 1;
  R				= CANID & 0x01;
  CANID			= CANID >> 1;
  priority		= CANID &0x07;
}


uint32_t message::getCANID(void) {

	uint32_t PGN;
	
	PGN = getPGN();
	return ((PGN << 8) | priority << 26) | sourceAddr;
}

			
void message::setPGN(uint32_t PGN) {

	PDUs	= PGN & 0x0FF;
	PGN	= PGN >> 8;
	PDUf	= PGN & 0x0FF;
	PGN	= PGN >> 8;
	DP		= PGN & 0x01;
	PGN	= PGN >> 1;
	R		= PGN & 0x01;
}

	
uint32_t message::getPGN(void) {

	uint32_t PGN;
    
	PGN = 0;
	if (R) {
		bitWrite(PGN, 0, 1);
	}
	PGN = PGN << 1;
	if (DP) {
		bitWrite(PGN, 0, 1);
	}
	PGN = PGN << 8;
	PGN = PGN | PDUf;
	PGN = PGN << 8;
	PGN = PGN | PDUs;
	return PGN;
}


void message::setPriority(byte inPriority) { priority = inPriority; }


byte message::getPriority(void) { return priority; }


void message::setR(bool inR) { R = inR; }


bool message::getR(void) { return R; }


void message::setDP(bool inDP) { DP = inDP; }


bool message::getDP(void) { return DP; }


void message::setPDUf(byte inPDUf) { PDUf = inPDUf; }


byte message::getPDUf(void) { return PDUf; }


void message::setPDUs(byte inPDUs) { PDUs = inPDUs; } 


byte message::getPDUs(void) { return PDUs; }


void message::setSourceAddr(byte inSourceAddr) { sourceAddr = inSourceAddr; }


byte message::getSourceAddr(void) { return sourceAddr; }


void message::setData(int index,byte inByte) { msgData[index] = inByte; }


byte message::getData(int index) { return msgData[index]; }

	
void message::showMessage(void) {

	Serial.print("PGN           : "); Serial.println(getPGN(),HEX);
	Serial.print("Priority      : "); Serial.println(priority);
	Serial.print("Reserve bit   : "); Serial.println(R);
	Serial.print("Data page bit : "); Serial.println(DP);
	Serial.print("PDU format    : "); Serial.println(PDUf);
	Serial.print("PDU specific  : "); Serial.println(PDUs);
	Serial.print("Source addr   : "); Serial.println(sourceAddr);
	Serial.println("Data");
	for (int i=0;i<numBytes;i++) {
		Serial.print("[ ");Serial.print(msgData[i]);Serial.print(" ]");Serial.print('\t');
	}
	Serial.println();
	for (int i=0;i<numBytes;i++) {
		Serial.print("[ 0x");Serial.print(msgData[i],HEX);Serial.print(" ]");Serial.print('\t');
	}
}


//  -----------  ECUname class  -----------

byte nameBuff[8];		// Global name buffer for when people want to grab the 8 bytes from the ECUname class.


// Packed eight byte set of goodies.
ECUname::ECUname(void) { clearName(); }

ECUname::~ECUname(void) {  }


// Just in case you need to clear it out.
void ECUname::clearName(void) {

	for(int i=0;i<7;i++) {
		name[i] = 0;
	}
}


// We the same as that guy?
bool ECUname::sameName(ECUname* inName) {
	
	for(int i=0;i<7;i++) {
		if (name[i] != inName->name[i]) return false;
	}
}

			
// 64 bit - Pass back ta copy of the 64 bits that this makes up as our name.
byte* ECUname::getName(void) {						
	for (int i=0;i<7;i++) {
		nameBuff[i] = name[i];
	}
	return nameBuff;
}


// If we want to decode one?
void ECUname::setName(byte* namePtr) {
	
	for (int i=0;i<7;i++) {
		name[i] = namePtr[i];
	}
}


// Byte 8
// 1 bit - True, we CAN change our address. 128..247	
// False, we can't change our own address.
bool ECUname::getArbitratyAddrBit(void) { return name[7] >> 7; }    


void ECUname::setArbitratyAddrBit(bool AABit) { 
 
	name[7] &= ~(1<<7);								//Clear bit
	name[7] = name[7] | ((AABit & 0x1)<<7);
}


// 3 bit - Assigned by committee. Tractor, car, boat..
indGroup ECUname::getIndGroup(void) { 
	
	byte group;
	
	group = name[7]>>4 & 0x7;
	return (indGroup) group;
}


void ECUname::setIndGroup(indGroup inGroup) {
	
	byte group;
	
	group = byte(inGroup);
	name[7] &= ~(0x7<<4);								//Clear bits
	name[7] = name[7] | ((group & 0x7)<<4);
}


// 4 bit - System instance, like engine1 or engine2.
byte ECUname::getSystemInst(void) { return name[7] & 0xF; }


void ECUname::setSystemInst(byte sysInst) {
	
	name[7] &= ~(0xF);							//Clear bits
	name[7] = name[7] | (sysInst & 0xF);
}


//Byte7
// 7 bit - Assigned by committee.
byte ECUname::getVehSys(void) { return (name[6] >> 1); }


void ECUname::setVehSys(byte vehSys) {

	name[6] = 0;								//Clear bits
	name[6] = name[6] | (vehSys << 1);
}


//Byte6
// 8 bit - Assigned by committee. 0..127 absolute definition.
// 128+ need other fields for definition.
byte ECUname::getFunction(void) { return name[5]; }


void ECUname::setFunction(byte funct) { name[5] = funct; }


//Byte5
// 5 bit - Instance of this function. (Fuel level?)
byte ECUname::getFunctInst(void) { return name[4] >> 3; }


void ECUname::setFunctInst(byte functInst) {

	name[4] &= ~(0x1F << 3);								//Clear bits
	name[4] = name[4] | ((functInst & 0x1F) << 3);
}


// 3 bit - What processor instance are we?
byte ECUname::getECUInst(void) { return name[4] & 0x7; }

void ECUname::setECUInst(byte inst) {

	name[4] &= ~(0x7);
	name[4] = name[4] | (inst & 0x7);
}


//Byte4/3
// 11 bit - Assigned by committee. Who made this thing?
uint16_t ECUname::getManufCode(void) { return name[3]<<3 | (name[2] >> 5);  }


void ECUname::setManufCode(uint16_t manfCode) {

	name[2] &= ~(0x7<<5); //Clear bits byte 3
	name[3] = 0; //Clear bits byte 4
	name[2] = name[2] | (manfCode<<3 & 0xE0);
	name[3] = manfCode>>3;
}


//Byte3/2/1
// 21 bit - Unique Fixed value. Product ID & Serial number kinda' thing.
uint32_t ECUname::getID(void) { return (name[2] & 0x1F) << 16 | (name[1] << 8) | name[0]; }


void ECUname::setID(uint32_t value) {

	name[0] = value & 0xFF;
	name[1] = (value >> 8) & 0xFF;
	name[2] &= ~(0x1F); //Clear bits
	name[2] = name[2] | ((value >> 16) & 0x1F);
}



//				----- ECU Electronic control unit. -----


// Electronic control unit.						
ECU::ECU(byte inECUInst)
	: linkList(), idler() {
	
	setECUInst(inECUInst);
}


ECU::~ECU(void) {  }


// First thing is to check for and handle incoming messages.
// Next is to see if any CA's need to output messages of their own.
void ECU::idle(void) {

	CA*			trace;

   handlePacket();							// If polling, and that's what's up now. We see if a message has arrived.
	trace = (CA*)getFirst();				// Well start at the beginning and let 'em all have a go.
	while(trace) {								// While we got something..
		trace->idleTime();					// Give 'em some time to do things.
		trace = (CA*)trace->getNext();	// Grab the next one.
	}
}


// See how we deal with addressing.
adderCat ECU::getAddrCat(void) { return ourAddrCat; }


// Set how we deal with addressing.
void ECU::setAddrCat(adderCat inAddrCat) {

	ourAddrCat = inAddrCat;
	setArbitratyAddrBit(ourAddrCat==arbitraryConfig);
}


// serviceConfig
// commandConfig
// selfConfig
byte ECU::getAddr(void) { return addr; }



void ECU::setAddr(byte inAddr) { addr = inAddr; }
	


byte ECU::getDefAddr(void) { return defAddr; }



void ECU::setDefAddr(byte inAddr) {

	defAddr = inAddr;
}


// arbitraryConfig

// This sends the request to inAddr to send back their name. inAdder can be specific or
// GLOBAL_ADDR to hit everyone.
void ECU::requestForAddressClaim(byte inAddr) {
	
	message	ourMsg(3);
	
	ourMsg.setData(0,0);
	ourMsg.setData(1,0xEE);
	ourMsg.setData(2,0);
	ourMsg.setSourceAddr(getAddr());
	ourMsg.setPGN(REQ_MESSAGE);
	ourMsg.setPDUs(inAddr);
	sendMsg(&ourMsg);
}


void ECU::addressClaimed(void) {

}


void ECU::cannotClaimAddress(void) {

}


void ECU::commandedAddress(void) {

}

			
		
//  -----------  CA class  -----------


CA::CA(ECU* inECU)
	: linkListObj() {
	
	ourECU		= inECU;					// Pointer back to our "boss".
	ourPGN		= 0;						// Default to zero.
   numBytes		= DEF_NUM_BYTES;		// Set the default size.
   intervaTimer.reset();				// Default to off.
}


CA::~CA(void) { }


int CA::getNumBytes(void) { return numBytes; }


void CA::setNumBytes(int inNumBytes) { numBytes = inNumBytes; }


void CA::handleMsg(message* inMsg) {  }


void CA::sendMsg(void) {  }


void CA::setSendInterval(float inMs) {

   if (inMs>0) {
      intervaTimer.setTime(inMs);
   } else {
      intervaTimer.reset();
   }
}
 

float CA::getSendInterval(void) {  return intervaTimer.getTime(); }
 

// Same as idle, but called by the ECU.	 
void  CA::idleTime(void) {

   if (intervaTimer.ding()) {
      sendMsg();
      intervaTimer.stepTime();
   }
}



		
				