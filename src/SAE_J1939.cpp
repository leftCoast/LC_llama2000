#include <SAE_J1939.h>


//				----- message class -----

byte dataCopy[8];


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


void message::setDataByte(int index,byte inByte) { msgData[index] = inByte; }


byte message::getDataByte(int index) { return msgData[index]; }


// This takes our data bytes, MUST be eight in this case. Converts it to a ECUname and
// compares it to the passed in name. If the message's name is less in value than the
// passed in name, message owner's name wins the battle.
bool message::msgIsLessThanName(ECUname* inName) {
	
	ECUname	msgName;
	
	if (inName) {											// They gave us a non-null name pointer. Check
		if (getNumBytes()==8) {							// Our data is 8 bytes. Check
			msgName.setName(msgData);					// Setup a name out of our data bytes.
			return msgName.isLessThanName(inName);	// Return if the passed in name is less than ours.
		}														//
	}															//
	return false;											// Default to NOT less than.
}	


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


// Packed eight byte set of goodies. We'll preload this with a depth sounder. As an example.
ECUname::ECUname(void) {

	setID(0);								// Device ID. We make these up. You get 21 bits.
	setManufCode(0);						// This would be assigned to you by NMEA people.
	setECUInst(0);							// First ECU (Electronic control unit.)
	setFunctInst(0);						// First depth transducer.
	setFunction(DEV_FUNC_GP_TRANS);	// Depth transducer.
												// Some spare bit here..
	setVehSys(DEV_CLASS_INST);			//	We are an instrument.
	setSystemInst(0);						// We are the first of our device class.
	setIndGroup(Marine);					// What kind of machine are we ridin' on?
	//setArbitraryAddrBit(?);			// Will be set when we choose our addressing mode.
}

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
	return true;
}


// Are we less than that guy?
bool ECUname::isLessThanName(ECUname* inName) {

	for(int i=0;i<8;i++) {											// For each byte
		if (name[i] < inName->name[i]) return true;		// First non equal, we are less? Bail with true.
		if (name[i] > inName->name[i]) return false;		// First non equal, we are greater? Bail with false.
	}																	//
	return false;													// At this point we are equal, that is NOT less than.
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
bool ECUname::getArbitraryAddrBit(void) { return name[7] >> 7; }    


void ECUname::setArbitraryAddrBit(bool AABit) { 
 
	name[7] &= ~(1<<7);								//Clear bit
	name[7] = name[7] | ((AABit & 0x1)<<7);
}


// 3 bit - Assigned by committee. Tractor, car, boat..
// See list of values in .h file.
indGroup ECUname::getIndGroup(void) { 
	
	byte group;
	
	group = name[7]>>4 & 0x7;
	return (indGroup) group;
}

// See list of values in .h file.
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
// See list of values in .h file.
byte ECUname::getVehSys(void) { return (name[6] >> 1); }


// See list of values in .h file.
void ECUname::setVehSys(byte vehSys) {

	name[6] = 0;								//Clear bits
	name[6] = name[6] | (vehSys << 1);
}


//Byte6
// 8 bit - Assigned by committee.
// See list of values in .h file.
byte ECUname::getFunction(void) { return name[5]; }


// See list of values in .h file.
void ECUname::setFunction(byte funct) { name[5] = funct; }


//Byte5
// 5 bit - Instance of this function.
// See list of values in .h file.
byte ECUname::getFunctInst(void) { return name[4] >> 3; }

// See list of values in .h file.
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
// The list is long. See https://canboat.github.io/canboat/canboat.html
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


void ECUname::showName(void) {
	
	Serial.print("ISO ID          : "); Serial.println(getID());			// Device ID. Serial #
	Serial.print("Manuf code      : "); Serial.println(getManufCode());	// Who made you?					
	Serial.print("ECU Inst.       : "); Serial.println(getECUInst());		// What ECU# are you?
	Serial.print("Funct. Inst.    : "); Serial.println(getFunctInst());	// What # of your thing are you?
	Serial.print("Actual Funct.   : "); Serial.println(getFunction());	// Depth transducer.
																								// Some spare bit here..
	Serial.print("Kind of Funct.  : "); Serial.println(getVehSys());		//	We are an..?
	Serial.print("Item Inst.      : "); Serial.println(getSystemInst());	// We are the ? of our device class.
	Serial.print("Industry group  : "); 											// What kind of machine are we ridin' on?
	switch(getIndGroup()) {
		
		case Global			: Serial.println("Global"); break;
		case Highway		: Serial.println("Highway"); break;
		case Agriculture	: Serial.println("Agriculture"); break;
		case Construction	: Serial.println("Construction"); break;
		case Marine			: Serial.println("Marine"); break;
		case Industrial	: Serial.println("Industrial"); break;
	}				
	if (getArbitraryAddrBit()) {
		Serial.print("Addr bit        : Does auto addressing.");
	} else {
		Serial.print("Addr bit        : Does NOT do auto addressing.");
	} 
}




//				----- ECU Electronic control unit. -----


// Electronic control unit.						
ECU::ECU(void)
	: linkList(), idler() {
	
	ourState	= config;		// We arrive in config mode.
	addr		= NULL_ADDR;	// No address.
}


ECU::~ECU(void) {  }


void ECU::begin(ECUname* inName,byte inAddr,addrCat inAddCat) {

	copyName(inName);
	setAddr(inAddr);
	setAddrCat(inAddCat);
}


void ECU::setState(ECUState newState) {

	switch(ourState) {
		case config		:
			switch(newState) {
				case config		:
																	// Whatever.
				break;	
				case arbit		:
					ourState = preStart;						// No, you go here first.
				break;
				case addrErr	:
					ourState = addrErr;						// Can't see how, but. Whatever.
				case running	:
					ourState = preStart;						// No, you go here first.
				break;
			}
		break;
		case arbit		:
			switch(newState) {
				case config		:
					ourState = config;						// I guess going back is ok?
				break;	
				case arbit		:
																	// Whatever.
				break;
				case addrErr	:
					ourState = addrErr;						// Can't see how, but. Whatever.
				case running	:
					ourState = running;						// Ok, we're running.
				break;
			}
		}
		case running	:
			switch(newState) {
				case config		:
					ourState = config;						// I guess going back is ok?
				break;	
				case arbit		:
					if (ourAddrCat==arbitraryConfig) {	// If we are an arbitration type..
						ourState = arbit;						// We can go back and do it again.
					}
				break;
				case addrErr	:
					ourState = addrErr;						// Oh ohh, address conflict I guess.
				case running	:
					ourState = running;						// Ok, we're still running.
				break;
			}
		break;
		case addrErr	:
			switch(newState) {
				case config		:
					ourState = config;						// Just about the only thing to clear the error.
				break;	
				case arbit		:
				case addrErr	:
				case running	:
					ourState = addrErr;						// None of these is going back to fix the error. So..
				break;
			}
		break;
	}
}


void ECU::handleMsg(message* inMsg) {

	CA*	trace;
	bool	done;
	
	if (isReqAddrClaim(inMsg)) handleReqAdderClaim(inMsg);
	else if (isAddrClaim(inMsg)) handleAdderClaim(inMsg);
	else if (isCantClaim(inMsg)) handleCantClaim(inMsg);
	else if (isCommandedAddr(inMsg)) handleComAddr(inMsg);
	else if (ourState==running) {
		done = false;
		trace = (CA*)getFirst();
		while(!done) {
			if (trace) {
				if (trace->handleMsg(inMsg) {
					done = true;
				} else {
					trace = (CA*)trace->next();
				}
			} else {
				done = true;
			}
		}
	}
}


// Request address claimed. Someone is asking for everyone, or you to show who you are
// and what address you are holding at this moment.
bool ECU::isReqAddrClaim(inMsg) {

	if (inMsg->PGN==REQ_MESSAGE && inMsg->getNumBytes()==3) return true;
	if (inMsg->PDUf==REQ_ADDR_CLAIM_PF && inMsg->R==0 && inMsg->DP==0 && inMsg->getNumBytes()==3) return true;
	return false;
	
}


// Address Claimed. Someone is telling the world that this is the address they are going
// to use. If this conflicts with yours? Deal with that.
bool ECU::isAddrClaim(inMsg) {
	
	if (inMsg->PGN==ADDR_CLAIMED && inMsg->sourceAddr==GLOBAL_ADDR && inMsg->getNumBytes()==8) return true;
	return false;
}

// Someone is telling the network that they can not claim an address at all.
bool ECU::isCantClaim(inMsg) {
	
	if (inMsg->PGN==ADDR_CLAIMED && inMsg->sourceAddr==NULL_ADDR && inMsg->getNumBytes()==8) return true;
	return false;
}


// Someone is telling somebody to change their address to a given new value. (The 9th byte)
bool ECU::isCommandedAddr(inMsg) {
	
	if (inMsg->PGN==COMMAND_ADDR && inMsg->getNumBytes()==9) return true;
	return false;
}


// We have a new message asking us or everyone what address we are and what our name is.
void ECU::handleReqAdderClaim(inMsg) {
	
	switch(ourState) {
		case arbit		:														// Arbitrating
		case running	:														// Or running..
			if (inMsg->PDUs==GLOBAL_ADDR || inMsg->PDUs==addr) {	// If sent to everyone or just us..
				sendAddressClaimed(true);									// We send our address & name.
			}
		break;
		case addrErr	:														// We failed to get an address.
			if (inMsg->PDUs==GLOBAL_ADDR || inMsg->PDUs==addr) {	// If sent to everyone or just us..
				sendCannotClaimAddress();									// We send null address & name.
			}
		break;
		default : break;
	}
}


// We have a message telling us that someone claimed an address. Let's see if it's our
// address they claimed. If so? We can send back and address claimed?
void ECU::handleAdderClaim(inMsg) {
	
	switch(ourState) {
		case arbit		:														// Arbitrating. (We can arbitrate then!)
			if (inMsg->sourceAddr==addr) {								// Claiming our address!?
				if (inMsg->isLessThanName(this)) {						// If they win the arbitration..
					addr = NULL_ADDR;											// We give up the address.
				} else {															// Else, we win the name fight.
					sendAddressClaimed();									// Rub in face!
				}																	//
			}																		//
		break;
		case running	:														// Running.											
			if (inMsg->sourceAddr==addr) {								// Claiming our address!?
				if (inMsg->isLessThanName(this)) {						// If they win the arbitration..
					addr = NULL_ADDR;											// We give up the address.
					if (ourAddrCat==arbitraryConfig) {					// If we do we do arbitration..
						setState(arbit);										// We go back to arbitration.
					} else {
						sendCannotClaimAddress();							// We send we are stuck;
						setState(addrErr);									// We go to address error state.
					}																//
				} else {															// Else WE WON the name game!
					sendAddressClaimed();									// Rub in face!
				}																	//
			}																		//
		break;	
		default : break;
	}
}


// We have a message telling someone or all that they can not claim an address. I don't
// have any response to this. "Gee too bad"? For now I guess it's handled. 
void ECU::handleCantClaim(inMsg) { }



void ECU::handleComAddr(inMsg) {

	if (ourAddrCat==commandConfig) {				// Only commandConfig addressing can do this.
		switch(ourState) {							// If our state is..
			case	addrErr	:							// Address error. But how can it receive this?
				changeState(config);					// Say it does, set to config.
				changeState(running);				// Then running..
			case running	:							// Or already running. State we SHOULD be in.
				addr = inMsg->getDataByte(8);		// Grab the address and plug it in.
				sendAddressClaimed();				// Tell the neighborhood.
			break;										// And we're done.
		}
	}
}


void setupAddrTable(void) {
	
}

// See how we deal with addressing.
addrCat ECU::getAddrCat(void) { return ourAddrCat; }


// Set how we deal with addressing.
void ECU::setAddrCat(addrCat inAddrCat) {

	ourAddrCat = inAddrCat;
	setArbitraryAddrBit(ourAddrCat==arbitraryConfig);
}


// serviceConfig
// commandConfig
// selfConfig
byte ECU::getAddr(void) { return addr; }


void ECU::setAddr(byte inAddr) {

	addr		= inAddr;	// Fine, our address is now inAddr.
	defAddr	= addr;		// And defAddr always tracks last addr.
}
	

byte ECU::getDefAddr(void) { return defAddr; }


void ECU::setDefAddr(byte inAddr) {

	defAddr = inAddr;				// Set the default address.
	if (ourState==config) {		// If we're in config state..
		addr = inAddr;				// We set our working address as well.
	}
}


// arbitraryConfig

// This sends the request to inAddr to send back their name. inAdder can be specific or
// GLOBAL_ADDR to hit everyone.
void ECU::sendRequestForAddressClaim(byte inAddr) {
	
	message	ourMsg(3);						// Create a message with 3 byte data buffer.
	
	ourMsg.setDataByte(0,0);				// Byte zero, gets zero.
	ourMsg.setDataByte(1,0xEE);			// Byte one, gets 0xEE.
	ourMsg.setDataByte(2,0);				// Byte 2 gets zero. These three are saying "Send your name!"
	ourMsg.setSourceAddr(getAddr());		// Set our address.
	ourMsg.setPGN(REQ_MESSAGE);			// Set the PGN..
	ourMsg.setPDUs(inAddr);					// Then set destination address as lower bits of PGN.
	sendMsg(&ourMsg);							// Off it goes!
}


void ECU::sendAddressClaimed(bool tryFail) {
	
	message	ourMsg;								// Create a message. (Default buffer size.)
	byte*		ourName;								// Pointer for our name's data.
	
	ourName = getName();							// Get a fresh copy of our name.
	for(int i=0;i<8;i++) {						// For each byte in our name..
		ourMsg.setDataByte(i,ourName[i]);	// Set our name byte into the message data buffer.
	}													// 
	if (tryFail) {									// If we're trying..
		ourMsg.setSourceAddr(getAddr());		// Set our address. (ACK)
	} else {											// Else we failed to get one..
		ourMsg.setSourceAddr(NULL_ADDR);		// Set NULL address. (NACK)
	}
	ourMsg.setPGN(ADDR_CLAIMED);				// Set the PGN..
	ourMsg.setPDUs(GLOBAL_ADDR);				// Then set destination address as lower bits of PGN.
	sendMsg(&ourMsg);								// Off it goes!
}


void ECU::sendCannotClaimAddress(void) { addressClaimed(false); }


void ECU::sendCommandedAddress(byte comAddr) {
	
	message	ourMsg(9);							// Create a 9 byte message.
	byte*		ourName;								// Pointer for our name's data.
	
	ourName = getName();							// Get a fresh copy of our name.
	for(int i=0;i<8;i++) {						// For each byte in our name..
		ourMsg.setDataByte(i,ourName[i]);	// Set our name byte into the message data buffer.
	}													//
	ourMsg.setDataByte(8,comAddr);			// Set our commanded address byte into the message.
	ourMsg.setSourceAddr(getAddr());			// Set our address.
	ourMsg.setPGN(COMMAND_ADDR);				// Set the PGN..
	sendMsg(&ourMsg);								
}

		
// First thing is to check for and handle incoming messages.
// Next is to see if any CA's need to output messages of their own.
void ECU::idle(void) {

	CA*			trace;

	handlePacket();								// If polling, and that's what's up now. We see if a message has arrived.
	if (running) {									// If we're in running state? We'll run the CAs.
		trace = (CA*)getFirst();				// Well start at the beginning and let 'em all have a go.
		while(trace) {								// While we got something..
			trace->idleTime();					// Give 'em some time to do things.
			trace = (CA*)trace->getNext();	// Grab the next one.
		}
	}
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



		
				