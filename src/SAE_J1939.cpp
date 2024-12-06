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
	/*
	for (int i=0;i<numBytes;i++) {
		Serial.print("[ 0x");Serial.print(msgData[i],HEX);Serial.print(" ]");Serial.print('\t');
	}
	Serial.println();
	*/
}



//				-----            BAMmsg            -----


// [32] [Size LSB] [Size MSB] [numPacks] [0xFF] [PGN LSB] [PGN2] [PGN MSB]

// I hope I did this right.


BAMmsg::BAMmsg(void) {  }

BAMmsg::BAMmsg(message* inBAMMsg) {  

	if (getPDUf()==BAM_PF && getNumBytes()==8) {
		setCANID(inBAMMsg->getCANID());
		for(int i=0;i<8;i++) {
			msgData[i] = inBAMMsg->getDataByte(i);
		}
	}	
}


BAMmsg::~BAMmsg(void){  }
	
	
void BAMmsg::setupBAM(byte destAddr,int numBytes,byte numPacks,uint32_t PGN) {
	
	setPGN(BAM_COMMAND);
	setPDUs(destAddr);
	msgData[0] = 32;						// As per doc.
	msgData[1] = numBytes & 0xFF;		// Byte one LSB
	numBytes = numBytes >> 8;			// Scroll off the LSB.
	msgData[2] = numBytes & 0xFF;		// Leaving MSB for byte 2.
	msgData[3] = numPacks;				// Byte 3 is just a byte.
	msgData[4] = 0xFF;					// As per doc.
	msgData[5] = PGN & 0xFF;			// Byte 5, LSB
	PGN = PGN >> 8;						// Clock it over.
	msgData[6] = PGN & 0xFF;			// Byte 6, Medium SB?
	PGN = PGN >> 8;						// Clock it over.
	msgData[7] = PGN & 0xFF;			// Byte 7, MSB. And we're done?
}


int BAMmsg::getBAMNumBytes(void) {

	int	numBytes;
	
	numBytes = 0;
	numBytes = numBytes |  msgData[2];
	numBytes = numBytes << 8;
	numBytes = numBytes |  msgData[1];
	return numBytes;
}


byte BAMmsg::getBAMNumPacks(void) { return msgData[3]; }


uint32_t BAMmsg::getBAMPGN(void) {

	uint32_t PGN;
    
	PGN = 0;
	PGN = PGN | msgData[7];
	PGN = PGN << 8;
	PGN = PGN | msgData[6];
	PGN = PGN << 8;
	PGN = PGN | msgData[5];
	return PGN;
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

	for(int i=0;i<8;i++) {
		name[i] = 0;
	}
}


// We the same as that guy?
bool ECUname::sameName(ECUname* inName) {
	
	for(int i=0;i<8;i++) {
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
	for (int i=0;i<8;i++) {
		nameBuff[i] = name[i];
	}
	return nameBuff;
}


// If we want to decode one?
void ECUname::setName(byte* namePtr) {
	
	for (int i=0;i<8;i++) {
		name[i] = namePtr[i];
	}
}


// We want to be a copy of this one? Ok..
void ECUname::copyName(ECUname* inName) {

	if (inName) {
		setName(inName->getName());
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



//				-----    addrList   &  addrNode    -----


addrNode::addrNode(byte inAddr) 
	: linkListObj()
	{ addr = inAddr; }


addrNode::~addrNode(void) {  }


// Are we greater than the obj being passed in?	
bool addrNode::isGreaterThan(linkListObj* compObj) { return ((addrNode*)compObj)->addr>addr; }


// Are we less than the obj being passed in?	
bool addrNode::isLessThan(linkListObj* compObj) { return ((addrNode*)compObj)->addr<addr; }



// Create a new address list.
addrList::addrList(void) : linkList() {  }

// Recycle an address list.
addrList::~addrList(void) {  }


// Return a pointer to this node if we can find it.
addrNode* addrList::findAddr(byte inAddr) {

	addrNode*	trace;
	
	trace = (addrNode*) getFirst();
	while(trace) {
		if (trace->addr==inAddr) return trace;
		trace = (addrNode*) trace->getNext();
	}
	return trace;
}


// Add an address into our list.	IF it's not already there.
void addrList::addAddr(byte inAddr) {

	addrNode* newNode;
	
	if (findAddr(inAddr)==NULL) {
		newNode = new addrNode(inAddr);
		if (newNode) addToTop(newNode);
	}
}


// Let's see the list of addresses we got..
void addrList::showList(void) {

	addrNode* trace;
	
	Serial.println("----  Address list ----");
	Serial.print("Number items : ");
	Serial.println(getCount());
	trace = (addrNode*)getFirst();
	while(trace) {
		Serial.print("Address : ");
		Serial.println(trace->addr);
		trace = (addrNode*)trace->getNext();
	}
	Serial.println();
}
		

//				-----    xferList   &  xferNode    -----


// As this is currently written, once we receive a BAM message, we own it. We will delete
// it when we are done with it.
xferNode::xferNode(BAMmsg* inMsg,bool inRecieve)
	: linkListObj() {
	
	msg = inMsg;
	recieve = inRecieve;
	buff = NULL;
	numBytes = 0;
	complete = true;
	if (msg) {
		if (resizeBuff(msg->getBAMNumBytes(),&buff)) {
			numBytes = msg->getBAMNumBytes();
			complete = false;
		}
	}
}
	

// Here is where we delete the BAM message. Just so you know.	
xferNode::~xferNode(void) {

	resizeBuff(0,&buff);			// Recycle our buffer.
	if (msg) delete(msg);		// Recycle the BAM message.
}


// If we have things we need to do at certain times? They happend here.
void xferNode::idleTime(void) {


}
	



xferList::xferList(void)
	: linkList(), idler() {  }
	
	
xferList::~xferList(void) {  }


// Either we create an outgoing BAM message or we receive in incoming BAM message. Toss it
// in here.
void xferList::addXfer(BAMmsg* inMsg,bool inRecieve) {  }


// Basic garbage collection. Any transfer message nodes completed get marked as complete
// and need to be recycled. Actually we only need to kill off one. This will be called
// over and over so, if there are more, the'll get hit soon.
void  xferList::listCleanup(void) {

	xferNode*	trace;
	
	trace = (xferNode*)getFirst();
	while(trace) {
		if (trace->complete) {
			unlinkObj(trace);
			delete(trace);
			trace = NULL;
		} else {
			trace = (xferNode*)trace->getNext();
		}
	}	
}


// Maintain the list and let all the current transfers do their thing.
void  xferList::idle(void) {

	xferNode*	trace;
	
	listCleanup();
	trace = (xferNode*)getFirst();
	while(trace) {
		trace->idleTime();
		trace = (xferNode*)trace->getNext();
	}
}



//				----- ECU Electronic control unit. -----


// Electronic control unit.						
ECU::ECU(void)
	: linkList(), idler() {
	
	ourState			= config;		// We arrive in config mode.
	addr				= NULL_ADDR;	// No address.
}


// Destructor. I don't think there's anything that's not handled automatically.
ECU::~ECU(void) {  }


// Things we can do to set up shop once we are actually post globals and running in code.
void ECU::begin(ECUname* inName,byte inAddr,addrCat inAddCat) {

	copyName(inName);							// We get our name info and copy it to ourselves.
	setAddr(inAddr);							// Our initial address.
	setAddrCat(inAddCat);					// Our method of handling address issues.
	hookup();									// We are guaranteed to be in code section, so hookup.
	ourAddrList.hookup();					// And hook up these guys as well.
	ourXferList.hookup();					// That should do it..
}


void ECU::stateName(ECUState aState) {
	
	switch(aState) {
		case config		: Serial.print("Configuration");	break;
		case startHold	: Serial.print("Start wait");		break;
		case arbit		: Serial.print("Arbitration");	break;
		case running	: Serial.print("Running");			break;
		case addrErr	: Serial.print("Address error");	break;
	}
}


// We need to change states. This is kinda' the flowchart of the program. What steps do we
// do, if even possible, to get from one state to another? List 'em here.
void ECU::changeState(ECUState newState) {

	Serial.print("*** State change from ");
	stateName(ourState);
	Serial.print(" to ");
	stateName(newState);
	Serial.println(" with result of.. ");
	
	switch(ourState) {
		case config		:													// **    We are in config state     **
			switch(newState) {											// **   And we want to switch to..  **
				case startHold	:											// Start holding state.
					startHoldTimer();										// Calculate and fire up the timer.
					ourState = startHold;								// Holding!		
				break;														//
				default	: 										break;	// No other path to take here.
			}
		break;
		case startHold :													// **    We are in startHold state     **
			switch(newState) {											// **   And we want to switch to..  **
				case arbit		:											// Shift into arbitration?
					if (ourAddrCat==arbitraryConfig) {				// If this is a legal state for us..
						startArbit();										// Start up arbitration.
						ourState = arbit;									// Ok, switch to arbitration.
					}															// Otherwise there is no change.
				break;														//
				case running	:											// Config to running? That's ok.
					sendAddressClaimed(true);							// Tell everyone where we plan to sit.
					ourState = running;									// Get busy running..
				break;														// 
				default			: 								break;	// No other path to take here.
			}
		break;																//
		case arbit		:													// **     We are in arbit state     **
			switch(newState) {											// **   And we want to switch to..  **
				case addrErr	: ourState = addrErr;	break;	// How? Ran out of addresses? (Actually yes)
				case running	:											// Arbit to running.
					ourAddrList.dumpList();								// Clear out address list, done with it.
					ourState = running;									// Ok, we're running.
				break;														//
				default			: 								break;	// No other path to take here.
			}
		break;																	//
		case running	:													// **   We are in running state     **
			switch(newState) {											// **   And we want to switch to..  **
				case arbit		:											// Arbitration mode? We'll see..
					if (ourAddrCat==arbitraryConfig) {				// If we are an arbitration type..
						startArbit();										// Start up arbitration.
						ourState = arbit;									// Back in arbitration again.
					}															//
				break;														//
				case addrErr	:											// Address error, uugh! Address conflict I guess.
					ourXferList.dumpList();								// Clear out xferList, done with it.
					addr = NULL_ADDR;										// We give up our address.
					ourState = addrErr;									// And were in error mode.
				break;														//
				default			: 								break;	// No other path to take here.
			}																	//
		break;																//
		case addrErr	:													// ** We are in address error state **
			switch(newState) {											// **   And we want to switch to..  **
				case arbit		:											// Arbitration mode? We'll see..
					if (ourAddrCat==arbitraryConfig) {				// If we are an arbitration type..
						startArbit();										// Start up arbitration.
						ourState = arbit;									// Back in arbitration again.
					}															//
				break;														//
				default			:								break;	// We just stay in error state.
			}																	//
		break;																//
		default					:								break;	// Other states will be switch elsewhere.
	}
	Serial.print("*** ");
	stateName(ourState);
	Serial.println();
}

void msgOut(message* inMsg) {
	
	Serial.println("-------------------------");
	inMsg->showMessage();
	Serial.println();
	Serial.println();
}


// We are passed in a message to handle. (From our progeny, or ourselves, if assembled.)
// First  we see if it's a network task that we have to handle ourselves. Then, if not, we
// ask each the CAs if one of them can handl it. Once a CA handles it, or none will. We
// are done.
void ECU::handleMsg(message* inMsg) {

	CA*	trace;
	bool	done;
	
	msgOut(inMsg);
	if (isReqAddrClaim(inMsg)) {
		Serial.println("Is REQ Claim");
		handleReqAdderClaim(inMsg);
	}
	else if (isAddrClaim(inMsg)) {
		Serial.println("Is addr Claim");
		handleAdderClaim(inMsg);
	}
	else if (isCantClaim(inMsg)) {
		Serial.println("Is CAN'T Claim");
		handleCantClaim(inMsg);
	}
	else if (isCommandedAddr(inMsg)) {
		Serial.println("Is COMMAND addr");
		handleComAddr(inMsg);
	}
	else {
		Serial.println("All returned false.");
		if (ourState==running) {
			
			done = false;
			trace = (CA*)getFirst();
			while(!done) {
				if (trace) {
					if (trace->handleMsg(inMsg)) {
						done = true;
					} else {
						trace = (CA*)trace->getNext();
					}
				} else {
					done = true;
				}
			}
		}
	}
}


// Request address claimed. Someone is asking for everyone, or us, to show who they are
// and what address they are holding at this moment.
bool ECU::isReqAddrClaim(message* inMsg) {
	
	Serial.println("isReqAddrClaim?");
	if (inMsg->getPDUf()==REQ_ADDR_CLAIM_PF && inMsg->getNumBytes()==3) {
		return true;
	}
	return false;
}


// Address Claimed. Someone is telling the world that this is the address they are going
// to use. If this conflicts with yours? Deal with that.
bool ECU::isAddrClaim(message* inMsg) {
	
	Serial.println("isAddrClaim?");
	if (inMsg->getPDUf()==ADDR_CLAIMED_PF && inMsg->getPDUs()==GLOBAL_ADDR && inMsg->getNumBytes()==8) {
		return true;
	}
	return false;
}


// Someone is telling the network that they can not claim an address at all.
bool ECU::isCantClaim(message* inMsg) {
	
	Serial.println("isCantClaim?");
	if (inMsg->getPDUf()==ADDR_CLAIMED_PF && inMsg->getPDUs()==GLOBAL_ADDR && inMsg->getSourceAddr()==NULL_ADDR && inMsg->getNumBytes()==8) {
		return true;
	}
	return false;
}


// Someone is telling somebody to change their address to a given new value. (The 9th byte)
bool ECU::isCommandedAddr(message* inMsg) {
	
	if (inMsg->getPGN()==COMMAND_ADDR && inMsg->getNumBytes()==9) return true;
	return false;
}


// We have a new message asking us or everyone what address we are and what our name is.
void ECU::handleReqAdderClaim(message* inMsg) {
	
	switch(ourState) {
		case arbit		:																		// Arbitrating
		case running	:																		// Or running..
			if (inMsg->getPDUs()==GLOBAL_ADDR || inMsg->getPDUs()==addr) {		// If sent to everyone or just us..
				sendAddressClaimed(true);													// We send our address & name.
			}
		break;
		case addrErr	:																		// We failed to get an address.
			if (inMsg->getPDUs()==GLOBAL_ADDR || inMsg->getPDUs()==addr) {		// If sent to everyone or just us..
				sendCannotClaimAddress();													// We send null address & name.
			}
		break;
		default : break;
	}
}


// We have a message telling us that someone claimed an address. Let's see if it's our
// address they claimed. If so? We can send back and address claimed?
void ECU::handleAdderClaim(message* inMsg) {
	Serial.println("***** ADDRESS CLAIM *****");
	ourAddrList.addAddr(inMsg->getSourceAddr());						// In all cases we add this address to our list.
	switch(ourState) {														// Now, lets see what's what..
		case arbit		:														// Arbitrating. (We can arbitrate then!)
			if (inMsg->getSourceAddr()==addr) {							// Claiming our address!?
				if (waitingForClaim) {										// If we are waiting for a contesting claim..
					if (inMsg->msgIsLessThanName(this)) {				// If they win the arbitration..
						Serial.println("***** WE LOOSE ADDRESS CLAIM *****");
						sendAddressClaimed(false);							// Let them know, that we know, that they won.
						addr = NULL_ADDR;										// We give up the address. This flags it for us as well.
					} else {														// Else, we win the name fight.
						Serial.println("***** WE WIN ADDRESS CLAIM *****");
						sendAddressClaimed(true);							// Rub in face!
					}																//
				}																	// Otherwise, all we want is the list of addresses.
			}																		//
		break;																	//
		case running	:														// Running.											
			if (inMsg->getSourceAddr()==addr) {							// Claiming our address!?
				if (inMsg->msgIsLessThanName(this)) {					// If they win the arbitration..
					sendAddressClaimed(false);								// Let them know, that we know, that they won.
					addr = NULL_ADDR;											// We give up the address.
					if (ourAddrCat==arbitraryConfig) {					// If we do we do arbitration..
						changeState(arbit);									// We go back to arbitration.
					} else {														// Else, we don't do arbitration..
						changeState(addrErr);								// We go to address error state.
					}																//
				} else {															// Else WE WON the name game!
					sendAddressClaimed(true);								// Rub in face!
				}																	//
			}																		//
		break;																	// 
		default : break;														// In any other state, we have nothing else to do.
	}
}


// We have a message telling someone, or all, that they can not claim an address. I don't
// have any response to this. "Gee too bad"? For now I guess, it's handled. 
void ECU::handleCantClaim(message* inMsg) { }



// There is a command that sends ECU's new addresses to switch to. We deal with these
// here.
void ECU::handleComAddr(message* inMsg) {

	if (ourAddrCat==commandConfig) {				// Only commandConfig addressing can do this.
		switch(ourState) {							// If our state is..
			case running	:							// Running is the only state we COULD see it in.
				addr = inMsg->getDataByte(8);		// Grab the address and plug it in.
				sendAddressClaimed(true);			// Tell the neighborhood.
			break;										// And we're done.
			default			: break;					// Else we just ignore it. They are crazy. Or power hungry.
		}
	}
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


// Fine, our address is now inAddr.
void ECU::setAddr(byte inAddr) { addr		= inAddr; }

// Assuming that in some way YOU fixed the error. This will clear the address error and
// restart the process. By YOU I mean let's say you had an address collision? YOU set a
// unclaimed address by calling setAddress() and now it's time to run on that new address.
void ECU::clearErr(void) {

	ourState = config;	// Next pass though idle() will kick off the machine.
}
	
	
// Calculate and start the startup time delay. Function of address.
void ECU::startHoldTimer(void) {
	
	if (addr>=0 && addr <=127) {
		holdTimer.setTime(.1);
		return;
	}
	if (addr>=248 && addr <=253) {
		holdTimer.setTime(.1);
		return;
	}
	holdTimer.setTime(250);
}

																				
// arbitraryConfig
	
// Calculate and start the claim time delay. Random function.	
void ECU::startClaimTimer(void) {

	long	rVal;
	
	rVal = random(0, 256);
	arbitTimer.setTime(rVal * 0.6);
}

		
// This sends the request to inAddr to send back their name. inAdder can be specific or
// GLOBAL_ADDR to hit everyone. The hope is that whomever called this cleared out the
// address list. Not the end of the world if not.
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
	}													//
	ourMsg.setPGN(ADDR_CLAIMED);				// Set the PGN..
	ourMsg.setPDUs(GLOBAL_ADDR);				// Then set destination address as lower bits of PGN.
	sendMsg(&ourMsg);								// Off it goes!													
}


void ECU::sendCannotClaimAddress(void) { sendAddressClaimed(false); }


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


// From whatever state we are in now, start arbitration.
void ECU::startArbit(void) {
	
	Serial.println("**** startArbit() ****");
	
	if (addr==NULL_ADDR) {								// If we have no current address..
		Serial.println("**** No Addr, start a list.. ****");
		ourAddrList.dumpList();							// Clear out list of addresses.
		ourXferList.dumpList();							// Clear out transfer list as well. Can't finish any.
		sendRequestForAddressClaim(GLOBAL_ADDR);	// Start gathering addresses again.
		arbitTimer.setTime(250);						// Set the timer.
		ourArbitState = waitingForAddrs;				// Note that we are gathering addresses again.
	} else {													// Else we DO have an address..
		Serial.println("**** Have Addr, see if it works. ****");
		sendAddressClaimed();							// See if we can claim what we got.
		arbitTimer.setTime(250);						// Set the timer.
		ourArbitState=waitingForClaim;
	}
}


// RE:ISO 11783 - Arbitators that do not have an assigned preferred address or cannot
// claim their preferred address shall claim an address in the range of 128 to 247.

// Ok, we need an address. Go through the allowed list checking with our generated list of
// claimed addresses. If you can't find a value? Grab it as our own and pass it back! If
// we find ALL of them? Pass back a NULL_ADDR as a fail.
byte ECU::chooseAddr(void) {

	int i;
	
	for(i=128;i<248;i++) {
		if (!ourAddrList.findAddr(i)) {
			return i;
		}
	}
	return NULL_ADDR;
}

// Arbitration is all about timers. That and adress lists..
void ECU::checkArbit(void) {
	
	if (ourArbitState==waitingForAddrs) {			// If gathering addresses, to choose a new one..
		if (arbitTimer.ding()) {						// If gathering's over..
			arbitTimer.reset();							// Shut off the timer.
			ourAddrList.showList();						// Well let's see 'em. DEBUGGING
			addr = chooseAddr();							// Choose an address using the list to compare.
			Serial.print("***** CHOOSING NEW ADDRESS : ");
			Serial.print(addr);
			Serial.println(" *****");
			if (addr!=NULL_ADDR) {						// If we found an unclaimed one..
				sendAddressClaimed(true);				// Send address claim on new address.
				startClaimTimer();						// Start the claim timer.
				ourArbitState = waitingForClaim;		// And we wait..
			} else {											// Else ALL are claimed..
				changeState(addrErr);					// We have failed.
			}
		}														//
	}															//
	else if (ourArbitState==waitingForClaim) {	// Waiting to see if our address claim is challenged.
		if (arbitTimer.ding()) {						// If waiting for challenge is over..
			if (addr==NULL_ADDR) {						// If we are back to NULL_ADDR, we were challenged and lost!
				startArbit();								// We start all over again.
			} else {											// Else, we have a unchallenged address!
				Serial.println("***** SUCCESS! No one wanted the new address! *****");
				changeState(running);					// Whoo hoo! Go to running state.
			}
		}
	}
}

		
// Deal with timers, See if any CA's need to output messages of their own. Or other chores
// we know nothing about.
void ECU::idle(void) {

	CA*			trace;
	
	switch(ourState) {
		case config		:									// We're in config state. Time to start up!
			changeState(startHold);						// First pass through idle in config state. Start holding.
		break;
		case startHold	:									// In startHold state.
			if (holdTimer.ding()) {						// If the time is up..
				holdTimer.reset();						// Shut off the timer.
				if (ourAddrCat==arbitraryConfig) {	// If we do arbitration..
					changeState(arbit);					// Slide into arbitration gear.
				} else {										// Else, we don't do arbitration?
					changeState(running);				// Then we start running.
				}												//
			}													//
		break;												//
		case arbit		:	checkArbit();	break;	// In arbitration state. We check what's up.
		case running	:									// We're in running state. Let the CAs have some runtime.
			trace = (CA*)getFirst();					// Well start at the beginning and let 'em all have a go.
			while(trace) {									// While we got something..
				trace->idleTime();						// Give 'em some time to do things.
				trace = (CA*)trace->getNext();		// Grab the next one.
			}													//
		break;												//
		default			:						break;	// Anything else? Basically do nothing.
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


bool CA::handleMsg(message* inMsg) { return false; }


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



		
				