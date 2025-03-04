#include <SAE_J1939.h>



// The byte order is not the same as Arduino. It could be different than whatever YOU are
// trying to use the for. So we have these two integer byte ordering routines to make life
// easier. Whatever byte you want as high byte, stuff in the highByte slot, low byte into
// lowByte slot etc.


uint16_t pack16(byte hiByte,byte lowByte) {
  
   struct int16 {
      byte lowByte;
      byte hiByte;
   };
   int16*	bytes;
   uint16_t	value;

   value = 0;                    // Shut up compiler.
   bytes = (int16*)&value;
   bytes->lowByte = lowByte;
   bytes->hiByte = hiByte;
   return(value);
}


uint32_t pack32(byte hiByte,byte byte2,byte byte1,byte lowByte) {
  
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



// ***************************************************************************************
//				----- message class -----
// ***************************************************************************************


bool	isBlank(uint8_t inVal)  { return inVal==0xFF; }
bool	isBlank(uint16_t inVal) { return inVal==0xFFFF; }
bool	isBlank(uint32_t inVal) { return inVal==0xFFFFFFFF; }


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


message::message(message* inMsg) {

	numBytes		= 0;										// Because now, it is.
	msgData = NULL;										// Default so we can use resizeBuff().
	setNumBytes(inMsg->getNumBytes());				// Set the default size.
	for (int i=0;i<numBytes;i++) {
		setDataByte(i,inMsg->getDataByte(i));
	}	
	setPriority(inMsg->getPriority());
	setR(inMsg->getR());
	setDP(inMsg->getDP());
	setPDUf(inMsg->getPDUf());
	setPDUs(inMsg->getPDUs());
	setSourceAddr(inMsg->getSourceAddr());
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


// This one passes the pointer to our data buffer to someone else and TRUSTS them to NOT
// MESS WITH IT. For educational purposes only! Actually, this is used in the commanded
// address stuff to see who the command is actually adressed to. Non-destructively.
byte* message::peekData(void) { return msgData; }


// Ok, this one passes the pointer to our data buffer to SOMEONE ELSE TO OWN. We give up
// ownership to it completely. This is used for multi packet transfers. We are basically
// pulled apart and our data is used to form a multi packet datas stream for trasmission.
// Really, it doesn't hurt, much.
byte* message::passData(void) {

	byte*	dataPtr;
	
	dataPtr = msgData;	// Grab the data's address.
	msgData = NULL;		// NULL out our pointer to it.
	numBytes = 0;			// zero out our amount of data.
	return dataPtr;		// Pass the buffer on to whomever is to take it.
}


// Someone built up some massive data buffer for us to hold. Recycle ours and grab theirs.
// We take ownership of it. Yes, this is scary but it does skip the doubling RAM footprint
// thing.
void message::acceptData(byte* inData,int inNumBytes) {

	setNumBytes(0);			// We recycle ours.
	msgData = inData;			// We point at theirs.
	numBytes = inNumBytes;	// And we patch our size to what we are TOLD is theirs.
}


// We are the message from some netItem. We have a netName built into us. (Messages have
// names of senders built in.) Something can grab us, as a message, and stuff in
// their name to see if our name is less than theirs. This is how net name battles are
// decided. The smaller value wins.
//
// So, that being said. During arbitration, our guy asks to claim an address. If someone
// already "owns" that address, they send back a message. That message will also carry
// their name. Our guy grabs the incoming message, makes this call with their own name
// passed in. This is basically asking if THEY win or not. This RETURNS TRUE IF THEY WON.
bool message::msgIsLessThanName(netName* inName) {
	
	netName	msgName;
	
	if (inName) {											// They gave us a non-null name pointer. Check
		if (getNumBytes()==8) {							// Our data is 8 bytes. Check
			msgName.setName(msgData);					// Setup a name out of our data bytes.
			return msgName.isLessThanName(inName);	// Return if the passed in name is less than ours.
		}														//
	}															//
	return false;											// Default to NOT less than.
}	


bool message::isBroadcast(void) {

	uint32_t	PGN;
	
	PGN = getPGN();
	if (PGN>=0x00F000 && PGN<=0x00FEFF) return true;	// If our PGN is between these values..
	if (PGN>=0x00FF00 && PGN<=0x00FFFF)	return true;	// Then they are known as PDU2 messages,
	if (PGN>=0x01F000 && PGN<=0x01FEFF) return true;	// and are brodcast messages.
	if (PGN>=0x01FF00 && PGN<=0x01FFFF) return true;	//  
	if (PDUs==GLOBAL_ADDR)					return true;	// If a PDU1 message has destination of 255? Also calling it a broadcast.
	return false;													// Everything else we'll treat as Peer to peer.
}


void message::showMessage(void) {
	
	Serial.print("PGN           : "); Serial.println(getPGN(),HEX);
	Serial.print("Priority      : "); Serial.println(priority);
	Serial.print("Reserve bit   : "); Serial.println(R);
	Serial.print("Data page bit : "); Serial.println(DP);
	Serial.print("PDU format    : "); Serial.println(PDUf);
	Serial.print("PDU specific  : "); Serial.println(PDUs);
	Serial.print("Source addr   : "); Serial.println(sourceAddr);
	Serial.println("Data as bytes");
	for (int i=0;i<numBytes;i++) {
		Serial.print("[ ");Serial.print(msgData[i]);Serial.print(" ]");Serial.print('\t');
	}
	Serial.println();
	Serial.println("Data as text");
	for (int i=0;i<numBytes;i++) {
		Serial.print((char)(msgData[i]));
	}
	Serial.println();
	
	/*
	for (int i=0;i<numBytes;i++) {
		Serial.print("[ 0x");Serial.print(msgData[i],HEX);Serial.print(" ]");Serial.print('\t');
	}
	Serial.println();
	*/
}



// ***************************************************************************************
//                       -----------  netName class  -----------
// ***************************************************************************************


byte nameBuff[8];		// Global name buffer for when people want to grab the 8 bytes from the netName class.


// Packed eight byte set of goodies. We'll preload this with a General purpose transducer.
// As an example.
netName::netName(void) {
	
	clearName(false);						// Shut up compiler!
	setID(0);								// Device ID. We make these up. You get 21 bits.
	setManufCode(0);						// This would be assigned to you by NMEA people.
	setECUInst(0);							// First netObj (Electronic control unit.)
	setFunctInst(0);						// First transducer.
	setFunction(DEV_FUNC_GP_TRANS);	// General purpose transducer.
												// Some spare bit here..
	setVehSys(DEV_CLASS_INST);			//	We are an instrument.
	setSystemInst(0);						// We are the first of our device class.
	setIndGroup(Marine);					// What kind of machine are we ridin' on?
	//setArbitraryAddrBit(?);			// Will be set when we choose our addressing mode.
}


// We allocate nothing, we need to recycle nothing.
netName::~netName(void) {  }


// Want to zero or max out our name? This'll do it. Why either? Some like it hot.. No
// really some like to set zero's as a starting point. But many actually like to set 255
// as a flag that this data is not being used and should be ignored. So this should be
// helpful for both.
void netName::clearName(bool hiLow) {

	byte value;
	
	if (hiLow) {
		value = 0xFF;
	} else {
		value = 0x00;
	}
	for(int i=0;i<8;i++) {
		name[i] = value;
	}
}


// We the same as that guy?
bool netName::sameName(netName* inName) {
	
	for(int i=0;i<8;i++) {
		if (name[i] != inName->name[i]) return false;
	}
	return true;
}


// Are we less than that guy? Meaning, we win arbitration of names.
bool netName::isLessThanName(netName* inName) {

	for(int i=7;i>=0;i++) {										// For each byte
		if (name[i] < inName->name[i]) return true;		// First non equal, we are less? Bail with true.
		if (name[i] > inName->name[i]) return false;		// First non equal, we are greater? Bail with false.
	}																	//
	return false;													// At this point we are equal, that is NOT less than.
}

				
// 64 bit - Pass back a copy of the 64 bits that this makes up as our name.
byte* netName::getName(void) {						
	for (int i=0;i<8;i++) {
		nameBuff[i] = name[i];
	}
	return nameBuff;
}


// If we want to decode one?
void netName::setName(byte* namePtr) {
	
	for (int i=0;i<8;i++) {
		name[i] = namePtr[i];
	}
}


// We want to be a copy of this one? Ok..
void netName::copyName(netName* inName) {

	if (inName) {
		setName(inName->getName());
	}
}


// Byte 8
// 1 bit - True, we CAN change our address. 128..247	
// False, we can't change our own address.
bool netName::getArbitraryAddrBit(void) { return name[7] >> 7; }    


void netName::setArbitraryAddrBit(bool AABit) { 
 
	name[7] &= ~(1<<7);								//Clear bit
	name[7] = name[7] | ((AABit & 0x1)<<7);
}


// 3 bit - Assigned by committee. Tractor, car, boat..
// See list of values in .h file.
indGroup netName::getIndGroup(void) { 
	
	byte group;
	
	group = name[7]>>4 & 0x7;
	return (indGroup) group;
}

// See list of values in .h file.
void netName::setIndGroup(indGroup inGroup) {
	
	byte group;
	
	group = byte(inGroup);
	name[7] &= ~(0x7<<4);								//Clear bits
	name[7] = name[7] | ((group & 0x7)<<4);
}


// 4 bit - System instance, like engine1 or engine2.
byte netName::getSystemInst(void) { return name[7] & 0xF; }


void netName::setSystemInst(byte sysInst) {
	
	name[7] &= ~(0xF);							//Clear bits
	name[7] = name[7] | (sysInst & 0xF);
}


//Byte7
// 7 bit - Assigned by committee.
// See list of values in .h file.
byte netName::getVehSys(void) { return (name[6] >> 1); }


// See list of values in .h file.
void netName::setVehSys(byte vehSys) {

	name[6] = 0;								//Clear bits
	name[6] = name[6] | (vehSys << 1);
}


//Byte6
// 8 bit - Assigned by committee.
// See list of values in .h file.
byte netName::getFunction(void) { return name[5]; }


// See list of values in .h file.
void netName::setFunction(byte funct) { name[5] = funct; }


//Byte5
// 5 bit - Instance of this function.
// See list of values in .h file.
byte netName::getFunctInst(void) { return name[4] >> 3; }

// See list of values in .h file.
void netName::setFunctInst(byte functInst) {

	name[4] &= ~(0x1F << 3);								//Clear bits
	name[4] = name[4] | ((functInst & 0x1F) << 3);
}


// 3 bit - What processor instance are we?
byte netName::getECUInst(void) { return name[4] & 0x7; }


void netName::setECUInst(byte inst) {

	name[4] &= ~(0x7);
	name[4] = name[4] | (inst & 0x7);
}


//Byte4/3
// 11 bit - Assigned by committee. Who made this thing?
// The list is long. See https://canboat.github.io/canboat/canboat.html
uint16_t netName::getManufCode(void) { return name[3]<<3 | (name[2] >> 5);  }


void netName::setManufCode(uint16_t manfCode) {

	/*
	name[2] &= ~(0x7<<5); //Clear bits byte 3
	name[3] = 0; //Clear bits byte 4
	name[2] = name[2] | (manfCode<<3 & 0xE0);
	name[3] = manfCode>>3;
	*/
	
	byte	b2;
	byte	b3;
	
	b2 = manfCode & 0b00000111;		// Grab the low three of the code
	b2 = b2 << 5;							// Shift these three to the top end of byte.
	b3 = manfCode & 0b11111000;		// Grab the high 5 bits of the code.
	b3 = b3 >> 3;							// Shift those five down to the bottom of byte.
	name[2] = name[2] & 0b00011111;	// Clear out the top three bits of byte 2.
	name[2] = name[2] | b2;				// Merge our 3 bits into that top place.
	name[3] = name[3] & 0b11100000;	// Clear out the bottom 5 bits of byte 3.
	name[3] = name[3] | b3;				// Merge our 5 bits into that bottom place.
}


//Byte3/2/1
// 21 bit - Unique Fixed value. Product ID & Serial number kinda' thing.
uint32_t netName::getID(void) { return (name[2] & 0x1F) << 16 | (name[1] << 8) | name[0]; }


void netName::setID(uint32_t value) {

	name[0] = value & 0xFF;
	name[1] = (value >> 8) & 0xFF;
	name[2] &= ~(0x1F); //Clear bits
	name[2] = name[2] | ((value >> 16) & 0x1F);
}


// There are zillions of manufactere's names. Put the ones you know and see a lot of in
// here for the printout.
void  netName::showManuf(int manuf) {
	
	switch(manuf) {
		case 35	: Serial.println("Left coast #1");			break;
		case 73	: Serial.println("Left coast #2");			break;
		case 135	: Serial.println("Airmar"); 					break;
		case 381 : Serial.println("B&G");						break;
		case 644	: Serial.println("Wema U.S.A dba KUS");	break;
		case 717	: Serial.println("Yacht Devices");			break;
		default	: Serial.println(manuf);
	}
}


void netName::showName(void) {
	
	Serial.print("ISO ID          : "); Serial.println(getID());			// Device ID. Serial #
	Serial.print("Manuf code      : "); showManuf(getManufCode());			// Who made you?					
	Serial.print("ECU Inst.       : "); Serial.println(getECUInst());		// What netObj# are you?
	Serial.print("Funct. Inst.    : "); Serial.println(getFunctInst());	// What # of your thing are you?
	Serial.print("Actual Funct.   : "); Serial.println(getFunction());	// Depth transducer. Or whatever.
																								// Some spare bit here..
	Serial.print("Kind of Funct.  : "); Serial.println(getVehSys());		//	We are an..?
	Serial.print("Item Inst.      : "); Serial.println(getSystemInst());	// We are the ? of our device class.
	Serial.print("Industry group  : "); 											// What kind of machine are we ridin' on?
	switch(getIndGroup()) {
		
		case Global			: Serial.println("Global");			break;
		case Highway		: Serial.println("Highway");			break;
		case Agriculture	: Serial.println("Agriculture");		break;
		case Construction	: Serial.println("Construction");	break;
		case Marine			: Serial.println("Marine");			break;
		case Industrial	: Serial.println("Industrial");		break;
	}				
	if (getArbitraryAddrBit()) {
		Serial.println("Addr bit        : Does auto addressing.");
	} else {
		Serial.println("Addr bit        : Does NOT do auto addressing.");
	}
	Serial.print("As bytes        : ");
	for (int i=0;i<8;i++) {
		Serial.print("[");Serial.print(name[i]);Serial.print("]\t");
	}
	Serial.println();
}



// ***************************************************************************************
//				           -----    addrList   &  addrNode    -----
// ***************************************************************************************
//				                -----    addrNode    -----


addrNode::addrNode(byte inAddr,netName* inName) 
	: linkListObj() {
	
	addr = inAddr;							// Copy the address.
	name.setName(inName->getName());	// Copy the name.
}


addrNode::~addrNode(void) {  }


// Are we greater than the obj being passed in?	
bool addrNode::isGreaterThan(linkListObj* compObj) { return ((addrNode*)compObj)->addr>addr; }


// Are we less than the obj being passed in?	
bool addrNode::isLessThan(linkListObj* compObj) { return ((addrNode*)compObj)->addr<addr; }



//				                -----    addrList    -----


// Create a new address list.
addrList::addrList(void) : linkList() {  }

// Recycle an address list.
addrList::~addrList(void) {  }


// Add an address name pair into our list. If it's not already there.
void addrList::addAddr(byte inAddr,netName* inName) {

	addrNode* newNode;
	
	if (inName) {												// Making sure. They think it's funny to slip in a NULL.
		if (findPair(inAddr,inName)==NULL) {			// If we can't find this pair in there..
			newNode = new addrNode(inAddr,inName);		// Have a go at creating a new node for them.
			if (newNode) {										// If we were able to allocate a new node..
				addToTop(newNode);							// Hook it to the list and we are done!
			}														//
		}
	}
}


// Return a pointer to this node, if we can find it by address.
addrNode* addrList::findAddr(byte inAddr) {

	addrNode*	trace;
	
	trace = (addrNode*) getFirst();
	while(trace) {
		if (trace->addr==inAddr) {
			return trace;
		}
		trace = (addrNode*) trace->getNext();
	}
	return trace;
}


// Return a pointer to this node, if we can find it by name.
addrNode* addrList::findName(netName* inName) {

	addrNode*	trace;
	
	trace = (addrNode*) getFirst();
	while(trace) {
		if (trace->name.sameName(inName)) {
			return trace;
		}
		trace = (addrNode*) trace->getNext();
	}
	return trace;
}


// Return a pointer to this node if we can find it by address,name pair.
addrNode* addrList::findPair(byte inAddr,netName* inName) {

	addrNode*	trace;
	
	trace = (addrNode*) getFirst();
	while(trace) {
		if (trace->name.sameName(inName) && trace->addr==inAddr) {
			return trace;
		}
		trace = (addrNode*) trace->getNext();
	}
	return trace;
}



// Let's see the list of addresses we got..
void addrList::showList(bool withNames) {

	addrNode*	trace;
	int			addr;
	
	Serial.println(   "----  Address list ----");
	Serial.print("Number items : ");
	Serial.println(getCount());
	trace = (addrNode*)getFirst();
	while(trace) {
		Serial.print("Address         : ");
		addr = trace->addr;
		if (addr==GLOBAL_ADDR) {
			Serial.println("Global address");
		} else if (addr==NULL_ADDR) {
			Serial.println("Null address");
		} else {
			Serial.println(addr);
		}
		if (withNames) {
			trace->name.showName();
			Serial.println();
			Serial.println(" - - - - - - - - - - - -");
		}
		trace = (addrNode*)trace->getNext();
	}
}


		
// ***************************************************************************************
//				          -----    xferList   &  xferNode    -----
// ***************************************************************************************



// Setting up the base goodies. A copy of the initial message. Setting the complete
// variable..
xferNode::xferNode(netObj* inNetObj,xferList* inList)
	: linkListObj() {
	
	success		= false;		// We start without success.
	complete		= true;		// Let the offspring set this.
	ourNetObj	= inNetObj;	// Save off our netObj pointer.
	ourList		= inList;	// Save our list manager thingy.
	msgData		= NULL;		// Start all pointers we may allocate to NULL
	byteTotal	= 0;			// None been sent. yet..
	packNum		= 1;			// The packet ID we'll be sending/expecting.
}
	

// outData may have been used. If not NULL, we'll need to delete it.
xferNode::~xferNode(void) {

	/*
	Serial.println("Transfer recycling report.");
	Serial.print("Was I successful : ");
	if (success)
		Serial.println("Success");
	else {
		Serial.println("Fail");
		Serial.print("With reason : ");
		switch(reason) {
			case notAbort			: Serial.println("notAbort");		break;
			case busyAbort			: Serial.println("busyAbort");		break;
			case resourceAbort	: Serial.println("resourceAbort");		break;
			case timoutAbort		: Serial.println("timoutAbort");		break;
			default 					: Serial.println("noReason");		break;
		}
	}
	*/
	if (msgData) {			// If someone set it..
		free(msgData);		// We release it.
		msgData = NULL;	// Flag it so no one else tries to release it.
	}
}


// In an attempt to shut up the compiler, use this to decode a data value to an abort
// reason.
abortReason	xferNode::valueToReason(byte value) {

	switch(value) {
		case notAbort			: return notAbort;		// Not because of a received abort. Mostly unexpected msg.
		case busyAbort			: return busyAbort;
		case resourceAbort	: return resourceAbort;
		case timoutAbort		: return timoutAbort;
		default 					: return noReason;
	}
}


// We get in a message, let us see if it is one sent specifically to us. We default to not ours.
bool xferNode::isOurMsg(message* inMsg) { return false; }


// If no one is listenting? Then pass back false.
bool xferNode::handleMsg(message* inMsg) { return false; }


// Start the timer with a time in ms somewhere between these two values.
void xferNode::startTimer(int lowMs,int hiMs) {
	
	long timeMs;
	
	timeMs = random(lowMs, hiMs+1);
	xFerTimer.setTime(timeMs,true);
}


// Add a completed incoming msg to the list of incoming messages.
void xferNode::addMsgToQ(message* msg) {

	msgObj*	newMsg;
	
	newMsg = new msgObj(msg);					// Make up a msgObj.
	if (newMsg) {									// Got one?
		ourNetObj->ourMsgQ.push(newMsg);		// Stuff it into the queue.
	}
}


// Send data message. All the info needed to do this should be in our local globals.
bool xferNode::sendDataMsg(void) {

	message	dataMsg;

	dataMsg.setPriority(DEF_TP_PRIORITY);						// Set up the standard bits..
	dataMsg.setR(0);													// Reserve bit.
	dataMsg.setDP(0);													// Data page.
	dataMsg.setPDUf(DATA_XFER_PF);								// Data xFer message.
	dataMsg.setPDUs(msgAddr);										// Broadcasting or peer to peer.
	dataMsg.setSourceAddr(ourNetObj->getAddr());				// From us.
	dataMsg.setDataByte(0,packNum++);							// Data packet ID.
	for(int i=1;i<8;i++) {											// For each byte..
		if (byteTotal>msgSize) {									// If we've run out of data..
			dataMsg.setDataByte(i,0xFF);							// Data byte is flagged as 255.
		} else {															// Else, we have data to send.
			dataMsg.setDataByte(i,msgData[byteTotal++]);		// Write the data byte.
		}																	//
	}																		//
	ourNetObj->outgoingingMsg(&dataMsg);						// And its on it's way!
	return byteTotal>=msgSize;										// Return if this was the last or not.
}


// We are either sending a oversize message or receiving one. In either case, there is an
// initial message that starts all of this. Either and oversized message we are sending or
// some sort of BAM message we are receiving. Lets use this and get the PGN so we can use
// it later. We'll need it.
void xferNode::getXferPGN(message* initMsg) {

	uint32_t	aPGN;
	
	if (initMsg) {														// Sanity, we actually got one.
		if (initMsg->getPDUf()==FLOW_CON_PF) {					// Ok, it's flow control. We can grab it from here.
			xferPGN = ourList->getPGNFromTPData(initMsg);	// We have this handy function for that.
		} else {															// Else its NOT a flow control. Assume it's from us.
			xferPGN = initMsg->getPGN();							// Messages know how to do this for themselves.
		}
		aPGN	= xferPGN;												// Copy the PGN, this is going to tear things apart..
		byte5	= aPGN & 0x000000FF;								// Lets put in the PGN, all are the same. Grab LSB
		aPGN	= aPGN >> 8;												// Slide over PGN.
		byte6	= aPGN & 0x000000FF;									// Grab LSB
		aPGN	= aPGN >> 8;												// Slide over PGN.
		byte7	= aPGN & 0x000000FF;									// Grab LSB
	} else {																// Else? We're getting nutty parameters. Pull the plug.
		success = false;												// This is NOT a success.
		complete = true;												// We're done.
		reason = noReason;											// We don't have a reason for the powers-that-be going bananas.
	}
}


// Send flowControl message.
void xferNode::sendflowControlMsg(flowContType msgType,abortReason reason) {

	message	flowContMsg;
	uint16_t	aWord;
	uint8_t	aByte;
	
	Serial.print("numBytes : ");
	Serial.println(flowContMsg.getNumBytes());
	flowContMsg.setPriority(7);								// Set up the standard bits..
	flowContMsg.setR(0);											// Reserve bit.
	flowContMsg.setDP(0);										// Data page.
	flowContMsg.setPDUf(FLOW_CON_PF);						// Yes, an FC message.
	flowContMsg.setPDUs(msgAddr);								// FCs are for doing peer to peer. OR.. BAM messages.
	flowContMsg.setSourceAddr(ourNetObj->getAddr());	// From us.
	flowContMsg.setDataByte(0,(int)msgType);				// Data section, first is a constant.
	switch(msgType) {
		case BAM				:										// If broadcast. outAddr == 255. Or.. 
		case reqToSend		:										// If peer to peer. outAddr != 255.
			aWord = msgSize;										// Next two are the number of bytes to come.
			aByte = aWord & 0x00FF;								// Low order byte.
			flowContMsg.setDataByte(1,aByte);				// Set it into data index 1.
			aWord = aWord >> 8;									// Slide over the high order byte.
			aByte = aWord & 0x00FF;								// Grab high order byte off end.
			flowContMsg.setDataByte(2,aByte);				// Set it into data index 2.
			flowContMsg.setDataByte(3,msgPacks);			// Set in num message packets.
			flowContMsg.setDataByte(4,0xFF);					// See BYTE 4 note below.
		break;						
		case clearToSend	:
			flowContMsg.setDataByte(1,msgPacks);			// Set in num message packets.
			flowContMsg.setDataByte(2,packNum);				// The expected packet number. (base 1)
			flowContMsg.setDataByte(3,0xFF);					// Fill with 0xFF. Ok..
			flowContMsg.setDataByte(4,0xFF);					// Same here.
		break;	
		case endOfMsg		:
			aWord = msgSize;										// Next two are the number of bytes to come.
			aByte = aWord & 0x00FF;								// Low order byte.
			flowContMsg.setDataByte(1,aByte);				// Set it into data index 1.
			aWord = aWord >> 8;									// Slide over the high order byte.
			aByte = aWord & 0x00FF;								// Grab high order byte off end.
			flowContMsg.setDataByte(2,aByte);				// Set it into data index 2.
			flowContMsg.setDataByte(3,msgPacks);			// Set in num message packets.
			flowContMsg.setDataByte(4,0xFF);					// Fill with 0xFF. Ok..
		break;	
		case abortMsg		:
			aByte = (int)reason;									// Read the abort reason as an integer.
			flowContMsg.setDataByte(1,aByte);				// Wants the abort reason.
			flowContMsg.setDataByte(2,0xFF);					// Fill with 0xFF. Ok..
			flowContMsg.setDataByte(3,0xFF);					// These next two as well
			flowContMsg.setDataByte(4,0xFF);					// There ya' go..
		break;
	}
	flowContMsg.setDataByte(5,byte5);						//	Stuff pre-calculated PGN bytes in place.
	flowContMsg.setDataByte(6,byte6);						// 
	flowContMsg.setDataByte(7,byte7);						//
	ourNetObj->outgoingingMsg(&flowContMsg);				// Off it goes!
}

// BYTE 4 : In a broadcast this is supposed to be set to 0xFF meaning unlimited amount of
// packets. But in response to a Clear-To-Send from a peer? It is to be set to the max
// number of packets to be sent. Or 0xFF for unlimited.. So what is this unlimited thing
// saying? Are we establishing a data stream? I think not! This is nonsense. We already,
// with BYTE 3, established the number of packets to be sent.
//
// Hence: I'll set it to 0xFF (unlimited) for now.



//				          -----    outgoingBroadcast    -----


outgoingBroadcast::outgoingBroadcast(message* inMsg,netObj* inNetObj,xferList* inList)
	: xferNode(inNetObj,inList) {
	
	success = false;								// We assume this will fail. Such a bad attitude.
	complete = true;								// We are done.
	reason = noReason;							// And it's because WE did something wrong.
	if (inMsg && inNetObj) {					// OK. As always, check sanity..
		msgSize = inMsg->getNumBytes();		// Save off the size. (used later)
		if (msgSize>8) {							// If its's too big..
			getXferPGN(inMsg);					// Save off the PGN for later.
			msgData = inMsg->passData();		// Hands over the actual data to us. (Yes messages can do this.)
			msgPacks = msgSize/7;				// Seven goes into num bytes.?.
			if (msgSize%7) {						//	We got leftovers?
				 msgPacks++;						// Then add one.
			}											// (msgPack is used later.)
			sendflowControlMsg(BAM);			// Send a BAM message.
			startTimer(TWMIN_MS,TWMAX_MS);	// We don't send another 'till the timer dings.
			complete = false;						// Successfully started. So, not complete.
		}
	}			
}


// If we grabbed the data buffer? We'll deal with it here..	
outgoingBroadcast::~outgoingBroadcast(void) {

	if (msgData) {			// If msgData is non NULL. (We used it.)
		free(msgData);		// Recycle the RAM.
		msgData = NULL;	// Flag it as recycled so no one else tries to free it.
	}
}


// Broadcasts do all their work blindly by timer in this idle routine.
void outgoingBroadcast::idleTime(void) {
	
	if (!complete) {									// If we're currently running..
		if (xFerTimer.ding()) {						// If our timer has expired..
			complete = sendDataMsg();				// Pack up and send a data message.
			if (complete) {							// If that was the last data packet..
				success = true;						// Then, as far as we know, this was a success.
			}
		}
	}
}



//				          -----    outgoingPeerToPeer    -----


// The outside world thinks this message should be broken into packets to be send out.
// We'll have a look and see if this is the case.
outgoingPeerToPeer::outgoingPeerToPeer(message* inMsg,netObj* inNetObj,xferList* inList)
	: xferNode(inNetObj,inList) {

	complete = true;											// Assume we are done here.
	success = false;											// Assume failure.
	if (inMsg) {												// Always check sanity..
		msgSize = inMsg->getNumBytes();					// Save off the size. Just in case..
		if (msgSize>8) {										// If its's too big..
			if (!inMsg->isBroadcast()) {					// If it's NOT to everyone.. (Peer to peer)
				getXferPGN(inMsg);							// Save off the PGN for later.
				msgData = inMsg->passData();				// Hand over the actual data to us. (messages can do this, very scary.)
				msgPacks = msgSize/7;						// Seven goes into num bytes?.
				if (msgSize%7) {								//	We got leftovers?
					 msgPacks++;								// Then add one.
				}													// 
				msgAddr = inMsg->getSourceAddr();		// Peer to peer to.. 
				sendflowControlMsg(reqToSend);			// Send a reqToSend message.					
				ourState = waitToSend;						// We don't send another 'till they say it's ok.							
				xFerTimer.setTime(TR_MS,true);			// We allow this much time for a clear to send to come in.
				complete = false;								// Ok. Meets all criteria. Do not kill us yet, we're still running.
			}
		}	
	}			
}
	

// If we grabbed the data buffer? We'll deal with it here..	
outgoingPeerToPeer::~outgoingPeerToPeer(void) {

	if (msgData) {			// If outData is non NULL. (We used it.)
		free(msgData);		// Recycle the RAM.
		msgData = NULL;	// Flag it as recycled so no one else tries to free it.
	}
}


// Filter out messages that we are not interested in or not even addressed to us. Or that
// it actually IS a message. Or, that we are actually still accepting messages. So many
// things to go wrong!
bool outgoingPeerToPeer::isOurMsg(message* inMsg) {

	if (!complete) {																// We're still running.
		if (inMsg) {																// We don't handle null messages.
			if (inMsg->getPDUf()==FLOW_CON_PF) {							// Only interested in flow control messages.
				if (inMsg->getPDUs()==ourNetObj->getAddr()) {			// Having our address.
					if (xferPGN==ourList->getPGNFromTPData(inMsg)) {	// And matching our transfer PGN.
						return true;												// In this case? Yeah, it's ours.
					}
				}
			}
		}
	}
	return false;
}


// Messages will be passed in for us to peruse. We'll filter out ones specifically for
// ourselves and deal with them here. We'll return true if we dealt with the message.
// Actually the only things we respond to are flow control messages. Anything else we'll
// ignore.
bool outgoingPeerToPeer::handleMsg(message* inMsg) {

	bool 	handled;
	bool	dataDone;
	
	handled = false;														// We've done nothing yet.
	if (isOurMsg(inMsg)) {												// Lets see if it's real and one we need to deal with.
		switch(inMsg->getDataByte(0)) {								// Lets take a look at the control byte..
			case  clearToSend	:											// We got a clear to send message.
				if (ourState==waitToSend) {							// If we were waiting for a clear to send..
					if (inMsg->getDataByte(1)==0) {					// If flagged "Need more time"..
						xFerTimer.setTime(TH_MS,true);				// Bump up the allowed time to this much. For clear or ACK.
					} else {													// Else it's a normal "clear to send".
						dataDone = sendDataMsg();						// We send a data packet. (Setting local complete flag)
						if (dataDone) {									// If that was the last data packet..
							ourState=waitForACK;							// We're now waiting for an ACK.
						}														//
						xFerTimer.setTime(T3_MS,true);				// We allow this much time for clear or ACK
					}															//
				} else if (ourState==waitForACK) {					// If we were waiting for an ACK.. {
					if (inMsg->getDataByte(1)==0) {					// If flagged "Need more time"..
						xFerTimer.setTime(TH_MS,true);				// Bump up the allowed time to this much. For clear or ACK.
					} else {													// Else this is messed up. We bail.
						success = false;									// This is a fail.
						complete = true;									// Crazy sauce stops the game.
						reason = notAbort;								// We didn't get an abort. We got nonsense.
					}															//
				}																//
			break;															// That should cover all those cases.
			case  endOfMsg		:											// We got an end of message.
				if (ourState==waitForACK) {							// If we were waiting for such a message..
					success = true;										// Then we are successful!
				}																//
				complete = true;											// In every case we are completely done.
			break;															// And that's it.
			case  abortMsg		:											// Got an abort?!
				complete = true;											// In every case we are done.
				reason = valueToReason(inMsg->getDataByte(1));	// Ask them why?
			break;															// Sigh, we failed.
			default				:											// If we hit a default we are seriously messed up.
				complete = true;											// In every other case we are done.
				reason = notAbort;										// We didn't get an abort. We got nonsense.
			break;															// Maybe we should take up knitting?
		}																		//
		handled = true;													// In all cases, it was our message. So we handled it.
	} 																			//
	return handled;														// We return our result.
}
						
					

	
// During break time, we'll check to see if the timer's run out. If so? The message failed
// to complete.
void outgoingPeerToPeer::idleTime(void) {
	
	if (!complete) {					// If we're still running..
		if (xFerTimer.ding()) {		// If the timer ran out, there was no response..
			reason = timoutAbort;	// We have a timeout failure.
			complete = true;			// I have failed! Kill me now!
		}
	}
}



//				          -----    incomingBroadcast    -----


// We are being created to handle an incoming broadcast. Let's get to it.
incomingBroadcast::incomingBroadcast(message* inMsg,netObj* inNetObj,xferList* inList)
	: xferNode(inNetObj,inList) {

	msgSize	= pack16(inMsg->getDataByte(2),inMsg->getDataByte(1));	// Grab the number of bytes.
	if (resizeBuff(msgSize,&msgData)) {											// If we got the RAM.
		getXferPGN(inMsg);															// Save off the PGN for later.
		msgPacks = inMsg->getDataByte(3);										// Grab the number of packets.
		msgAddr = inMsg->getSourceAddr();										// Grab source address.
		xFerTimer.setTime(BCAST_T1_MS);											// We start the timeout timer.
		complete = false;																// Clear the complete flag, were running!
	} else {																				// Oh ohh, ran outta' RAM.
		reason = resourceAbort;														// Failed to get the RAM.
		success = false;																// So we failed.
		complete = true;																// And we're done.
	}
}																					

	
// The only thing we allocated was part of a message that'll dissolve when we recycle. So
// nothing to do here.
incomingBroadcast::~incomingBroadcast(void) { }
	
	
// The only thing we can accept is a broadcast data packet from the guy we're liked to. We
// do need to be running and we don't bother with null messages either.
bool incomingBroadcast::isOurMsg(message* inMsg) {

	if (!complete) {																// We're still running.
		if (inMsg) {																// We don't handle null messages.
			if (inMsg->getPDUf()==DATA_XFER_PF) {							// We can only take in data packets.
				if (inMsg->isBroadcast()) {									// If it's a broadcast data packet..
					if (inMsg->getSourceAddr()==msgAddr) {					// If it's from our guy's source address..
						if (inMsg->getNumBytes()==8) {						// If it has exactly 8 bytes data..
							if (inMsg->getDataByte(0)==packNum) {			// Ok. If this matches our desired packet num..
								return true;										// It's all good!
							}
						}
					}
				}
			}
		}
	}
	return false;
}


// Broadcasts run completely on timers and there is no way to control them from this end.
bool incomingBroadcast::handleMsg(message* inMsg) {

	int		i;
	bool		handled;
	message	outMsg;
	
	handled = false;														// Not handled anything yet.
	if (isOurMsg(inMsg)) {												// If it's a broadcast data packet from our guy.															
		i = 1;																// Fine! We'll take it. Set up a counter.
		while(byteTotal<msgSize&&i<8) {								// While we have data to transfer and a place to store it.
			msgData[byteTotal] = inMsg->getDataByte(i);			// We transfer bytes.
			byteTotal++;													// Bump up the total transferred.
			i++;																// Bump local count.
		}																		// 
		packNum++;															// Bump up our packet ID num.
		if (byteTotal==msgSize) {										// If we got ALL the bytes?
			outMsg.acceptData(msgData,msgSize);						// We hand over ownership of our data buffer.
			msgData = NULL;												// And we flag that we no longer own that data.
			outMsg.setPGN(xferPGN);										// Set in our saved PGN.
			outMsg.setSourceAddr(msgAddr);							// Set in their address.
			addMsgToQ(&outMsg);											// Add what we built to the queue.
			success = true;												// A success!
			complete = true;												// Call for our recycling, we're done!
		} else {																// Else there's more? Of course there's more!
			xFerTimer.start();											// Restart the timeout timer.
		}																		//
		handled = true;													// Hey, we we handled that one!
	}																			//
	return handled;														// Wasn't what we were looking for. Not handled.
}


// If our timer ever expires we assume the sending node gave up.
void incomingBroadcast::idleTime(void) {

	if (!complete) {					// If we're still running..
		if (xFerTimer.ding()) {		// And our timeout timer expires?
			reason = timoutAbort;	// Then we ran outta' time!
			complete = true;			// Call for our recycling, we're done!
		}
	}
}



//				         -----    incomingPeerToPeer    -----


incomingPeerToPeer::incomingPeerToPeer(message* inMsg,netObj* inNetObj,xferList* inList)
	: xferNode(inNetObj,inList) {
	
	if (inMsg) {																					// Quick sanity.
		if (inMsg->getPDUf()==SEND_REQ) {													// Peer to peer, we only have the PDUf to go on.
			if (inMsg->getPDUs()==inNetObj->getAddr()) {									// It's ours.
				msgSize	= pack16(inMsg->getDataByte(2),inMsg->getDataByte(1));	// Grab the number of bytes.
				if (resizeBuff(msgSize,&msgData)) {											// See if we can get the RAM.
					getXferPGN(inMsg);															// Grab PGN to be used later.
					msgAddr = inMsg->getSourceAddr();										// Grab return addr.
					msgPacks = inMsg->getDataByte(3);										// Grab the number of packets.
					sendflowControlMsg(clearToSend);											// Tell 'em it's ok, send the data.
					xFerTimer.setTime(T2_MS);													// We start the timeout timer.
					complete = false;																// Clear the complete flag. We're running!
				} else {																				// Else we couldn't get the RAM?
					sendflowControlMsg(abortMsg,resourceAbort);							// Send an abort message.
					reason = resourceAbort;														// Note we ran outta' RAM.
					complete = true;																// And tell 'em to dump us off the list. Ain't going to work.
				}
			}
		}
	}
}

	

// We attempted to allocate the message buffer. If we failed at some point it might be
// laying about wasted RAM. If so, lets recycle it. If it's null, then either someone took
// it over or we never allocated it. Either way, this'll do the right thing.
incomingPeerToPeer::~incomingPeerToPeer(void) { resizeBuff(0,&msgData); }


// Is this a message for us? Is this a message at all? Is it messed up in any way we can
// tell? Are we actually still running?
bool incomingPeerToPeer::isOurMsg(message* inMsg) {

	if (!complete) {														// We're still running.
		if (inMsg) {														// We don't handle null messages.
			if (!inMsg->isBroadcast()) {								// If it's not a broadcast data packet..
				if (inMsg->getPDUs()==ourNetObj->getAddr()) {	// It's for us?! How exiting!
					if (inMsg->getSourceAddr()==msgAddr) {			// And, it's from our guy's source address..
						if (inMsg->getNumBytes()==8) {				// If it has exactly 8 bytes data..
							if (inMsg->getDataByte(0)==packNum) {	// Ok. If this matches our desired packet num..
								return true;								// It's all good!
							}
						}
					}
				}
			}
		}
	}
	return false;
}


// 
bool incomingPeerToPeer::handleMsg(message* inMsg) {

	int		i;
	bool		handled;
	message	outMsg;
	
	handled = false;															// Well, we haven't handled anything yet.
	if (isOurMsg(inMsg)) {													// Is this message ours ans in good shape?																			
		if (inMsg->getPDUf()==DATA_XFER_PF) {							// If it's a data packet..
			i = 1;																// Fine! We'll take it. Set up a counter.
			while(byteTotal<msgSize&&i<8) {								// While we have data to transfer and a place to store it.
				msgData[byteTotal] = inMsg->getDataByte(i);			// We transfer bytes.
				byteTotal++;													// Bump up the total transferred.
				i++;																// Bump local count.
			}																		// 
			packNum++;															// Bump up our packet ID num.
			if (byteTotal==msgSize) {										// If we got 'em all..
				outMsg.acceptData(msgData,msgSize);						// We hand over ownership of our data buffer.
				msgData = NULL;												// And we flag that we no longer own that data.
				outMsg.setPGN(xferPGN);										// Set in our saved PGN.
				outMsg.setSourceAddr(msgAddr);							// Set in their address.
				addMsgToQ(&outMsg);											// Add what we built to the queue.
				success = true;												// A success!
				complete = true;												// Call for our recycling, we're done!
				sendflowControlMsg(endOfMsg);								// Tell 'em we got it all.
			} else if (byteTotal<msgSize) {								// Else there's more coming..
				sendflowControlMsg(clearToSend);							// Tell 'em to send more.
				xFerTimer.setTime(T2_MS);									// We start the timeout timer.
			} else {
				reason = noReason;											// Missed the ending somehow.
				success = false;												// A fail.
				complete = true;												// We missed the data count. Abort!!
				sendflowControlMsg(abortMsg);								// Tell 'em
			}													
			handled = true;													// We handled this message.
		} else if (inMsg->getPDUf()==FLOW_CON_PF) {					// Or, if it's a flow control msg..
			switch(inMsg->getDataByte(0)) {								// Let's see what they sent us.
				case reqToSend		:											// Umm, we're receiving, not sending.
				case clearToSend	:											// Same as above.
				case BAM				:											// Whoa! This is peer to peer..
					complete = true;											// Each case is crazy sauce. Pull the plug!
					reason = notAbort;										// We didn't get an abort. They went nuts!
				break;															// Enough!
				case abortMsg		:											// Abort is valid.
					reason = valueToReason(inMsg->getDataByte(1));	// We'll save their reason.
					success = false;											// A fail.
					complete = true;											// And we're done.
				break;															// Jump out.
				default				:											// We got something else?!
					complete = true;											// Again it's crazy sauce. Pull the plug!
					reason = notAbort;										// We didn't get an abort, they sent gibberish.
				break;															//
			}																		//
			handled = true;													// We handled this message.				   
		}
	}		
	return handled;
}


// Idle in this case is basically a deadman switch. If the timer expires, the connection was lost.
void incomingPeerToPeer::idleTime(void) {

	if (!complete && xFerTimer.ding()) {	// If the timer expires while still working..
		reason = timoutAbort;
		complete = true;							// Give up. The other side dropped connection.
	}
}



//				            -----    xferList    -----


xferList::xferList(void)
	: linkList(), idler() { ourNetObj = NULL; }
	
	
xferList::~xferList(void) {  }


void xferList::begin(netObj* inNetObj) { ourNetObj = inNetObj; }


// Most transfer portocal message types have the multi packet PGN number stored in the
// data section. This will decode that value for you.
uint32_t xferList::getPGNFromTPData(message* inMsg) {

	uint32_t aPGN;
	
	aPGN = 0;
	aPGN = aPGN | inMsg->getDataByte(7);
	aPGN = aPGN << 8;
	aPGN = aPGN | inMsg->getDataByte(6);
	aPGN = aPGN << 8;
	aPGN = aPGN | inMsg->getDataByte(5);
	return aPGN;
}


// Either we create a new outgoing extended message. Or, we received from the net a new
// incoming extended message. Create the suitable handler node with the initial message
// that started it. Then, add this new node to the xferNode list.
void xferList::addXfer(message* ioMsg,xferTypes xferType) {

	xferNode*	newXferNode;
	
	newXferNode = NULL;
	switch(xferType) {
		case broadcastIn		:	// We received a "BAM message".
			newXferNode = (xferNode*) new incomingBroadcast(ioMsg,ourNetObj,this);
		break;
		case broadcastOut		:	// We created a "BAM message".
			newXferNode = (xferNode*) new outgoingBroadcast(ioMsg,ourNetObj,this);
		break;
		case peerToPeerIn		:	// We received a "request to send" from a peer.
			newXferNode = (xferNode*) new incomingPeerToPeer(ioMsg,ourNetObj,this);
		break;
		case peerToPeerOut	:	// We created a "request to send" for a peer.
			newXferNode = (xferNode*) new outgoingPeerToPeer(ioMsg,ourNetObj,this);
		break;
	}
	if (newXferNode) {
		addToTop(newXferNode);
	}
}



bool xferList::checkList(message* ioMsg) {

	bool			handled;
	xferNode*	trace;
	
	handled = false;													// Assume we don't handle this.
	trace = (xferNode*)getFirst();								// Grab first handler node from the list.
	while(trace && !handled) {										// While we have non NULL node. AND message has not been handled..
		handled = trace->handleMsg(ioMsg);						// Ask each node if they want/need to handle this message.
		trace = (xferNode*)trace->getNext();					// Then grab the next node regardless of the answer.
	}
	return handled;
}	

// Ok, a message has come in from either the net, OR, we wrote it. It could be a start of
// a transfer we need to deal with. It could be part of a message we are already dealing
// with. Most likely it's nothing that concerns us. But, we get first right of refusal. So
// lets have a look at it.
bool xferList::handleMsg(message* ioMsg,bool received) {

	
	bool			handled;
	uint32_t		PGN;
	message		tempMsg;
	
	handled = false;															// Assume we don't handle this.
	if (ioMsg && ourNetObj) {												// Sanity first, is it not NULL? Is netObj not NULL?
		if (received) {														// If its from the net..
			if (ioMsg->getPDUf()==FLOW_CON_PF) {						// If we have an incoming BAM message..
				PGN = getPGNFromTPData(ioMsg);							// Lets see what this BAM is all about. Could be broadcast or peer to peer.
				tempMsg.setPGN(PGN);											// Drop this PGN into our temp message so we can..
				if (tempMsg.isBroadcast()) {								// See if the multi packet message will be a broadcast..
					addXfer(ioMsg,broadcastIn);							// Setup a brodcast transfer.
					handled = true;											//	And this message has been handled!
				} else {															// Else, it's peer to peer..
					if (tempMsg.getPDUs()==ourNetObj->getAddr()) {	// Using tempMsg, see if it's actually to us..
						addXfer(ioMsg,peerToPeerIn);						// Setup an incoming brodcast transfer.
						handled = true;										// We handled it.
					}																//
				}																	//
			} else {																// Else it's not a BAM, so..
				handled = checkList(ioMsg);								// See if anyone is looking for this.
			}																		// 
		} else {																	// Else we just wrote this? Yeech!
			if (ioMsg->getNumBytes()>8) {									// If message is oversized..
				Serial.println("incoming has more than 8 bytes");
				Serial.print("And has PGN of : 0x");
				Serial.println(ioMsg->getPGN(),HEX);
				if (ioMsg->isBroadcast()) {								// If the message itself is a broadcast..
					Serial.println("broadcast");
					addXfer(ioMsg,broadcastOut);							// Setup a multi packet brodcast transfer.
				} else {															// Else it's NOT a broadcast..
					Serial.println("It's a peer to peer");
					addXfer(ioMsg,peerToPeerOut);							// Set up a multi packet peer to peer transfer.
				}																	//
				handled = true;												// In any case, it's been handled.
			}																		//
		}																			// 
	}																				//
	return handled;															// Return the final result.
}


// Basic garbage collection. Any transfer message nodes completed get marked as complete
// and need to be recycled. Actually we only need to kill off one. This will be called
// over and over so, if there are more, the'll get hit soon.
void  xferList::listCleanup(void) {

	xferNode*	trace;
	
	trace = (xferNode*)getFirst();					// Grab the pointer to the top of the list.
	while(trace) {											// While we don't have a null pointer..
		if (trace->complete) {							// If this node is complete..
			unlinkObj(trace);								// Unlink the node.
			delete(trace);									// Recycle the node.
			trace = NULL;									// One node killing is enough. Null out the pointer.
		} else {												// If this node was NOT complete..
			trace = (xferNode*)trace->getNext();	// We jump to the next. (Till we hit a null pointer.)
		}
	}	
}


// Maintain the list and let all the current transfers do their thing.
void  xferList::idle(void) {

	xferNode*	trace;
	
	listCleanup();										// If we can find a completed node, we'll recycle it.
	trace = (xferNode*)getFirst();				// Grab pointer to top of list.
	while(trace) {										// While we don't have a null pointer..
		trace->idleTime();							// Give each node some time to do stuff.
		trace = (xferNode*)trace->getNext();	// We jump to the next node. (Till we hit a null pointer.)
	}
}



// ***************************************************************************************
//				              ----- mesgQ. Hold 'em in here. -----
// ***************************************************************************************


msgObj::msgObj(message* inMsg)
	: linkListObj(),
	message(inMsg) {  }


msgObj::~msgObj(void) { }					
					

msgQ::msgQ(void) {  }


msgQ::~msgQ(void) {  }



// ***************************************************************************************
//		----- netObj. Base class for allowing navigation of SAE J1939 networks -----
// ***************************************************************************************

						
netObj::netObj(void)
	: linkList(), idler(), netName() {
	
	ourState	= config;		// We arrive in config mode.
	addr		= NULL_ADDR;	// No address.
	holdTimer.reset();		// Shut down the timers so we don't get false triggers.
	arbitTimer.reset();		//
	claimTimer.reset();		//
}


// Destructor. I don't think there's anything that's not handled automatically.
netObj::~netObj(void) {  }


// Things we can do to set up shop once we are actually post globals and running in code.
void netObj::begin(netName* inName,byte inAddr,addrCat inAddCat) {

	ourXferList.begin(this);				// The xferList needs a pointer to us. Here 'tis.
	copyName(inName);							// We get our name info and copy it to ourselves.
	setAddr(inAddr);							// Our initial address.
	setAddrCat(inAddCat);					// Our method of handling address issues.
	hookup();									// We are guaranteed to be in code section, so hookup.
	ourAddrList.hookup();					// And hook up these guys as well.
	ourXferList.hookup();					// That should do it..
}


// Add the handlers of the messages you would like to send/receive.
void netObj::addMsgHandler(msgHandler* inHanldler) {

	addToTop(inHanldler);	// Hope it's a good one. NULL pointers will be filtered out.
}


// When a message comes in from the net, pass it in here. -(8 or less data bytes)- For now
// we just stuff it into the incoming message queue. During idle time we'll grab messages
// out of that queue and deal with them or pass them on to the user's handlers.
void netObj::incomingMsg(message* inMsg) {

	msgObj*	newMsg;
	
	if (inMsg) {												// First sanity. Did they slip us a NULL?
		if (!ourXferList.handleMsg(inMsg,true)) {		// Not NULL. Ok, if the xfer list doesn't want it..
			newMsg = new msgObj(inMsg);					// Make up a msgObj..
			if (newMsg) {										// Got one?
				ourMsgQ.push(newMsg);						// Stuff it into the queue.
			}
		}
	}
}


// When we want a message sent out, it's passed in here. If the message's data section is
// greater than 8 bytes, this will automatically send it to the transfer list to be broken
// into a set of multi packet messages. -(Can have > 8 data bytes)-
void netObj::outgoingingMsg(message* outMsg) {

	if (outMsg) {											// First sanity. Always check for NULL.
		if (outMsg->getNumBytes()>8) {				// Ok, If we have more than 8 databytes..
			ourXferList.handleMsg(outMsg,false);	// Pass the message over to the xfer list.
		} else {												// Else, we are within 8 data bytes limit..
			sendMsg(outMsg);								// Shove the message out the wire.
		}
	}
}


// This is called to first clear the address list, then make the
// sendRequestForAddressClaim() function to tell everyone to broadcast in their name and
// addresses. NOTE : This makes the call, but you must wait at least 750 ms before reading
// the list so everyone has time to respond.
void netObj::refreshAddrList(void) {

	ourAddrList.dumpList();							// Clear out list of addresses.
	ourAddrList.addAddr(addr,this);				// Stuff ourselves in first. We can't hear our own broadcasts.
	sendRequestForAddressClaim(GLOBAL_ADDR);	// Start gathering addresses again.
	claimTimer.setTime(BCAST_T1_MS);				// Start the claim timer for how long we allow them to come in.
}


// This is where we actually handle the vetted incoming messages. The multi packet
// messages are already assembled as messages with >8 byte data blocks. First we see if
// it's a network task. These we have to handle ourselves. Then, if not, we ask each the
// handlers if one of them can handle it. Once a msgHandler handles it, or
// none will. We are done.	-(Can have > 8 data bytes, see above)-
void netObj::checkMessages(void) {

	msgObj*			aMsg;
	msgHandler*		trace;
	bool				done;
	
	aMsg = (msgObj*)ourMsgQ.pop();										// Pop off the next message object.
	if (aMsg) {																	// If we got one..
		if (isReqAddrClaim(aMsg)) {										// Is it an request address claim? "I want your address and name".
			handleReqAdderClaim(aMsg);										// Do the request address claim dance.
		}																			//
		else if (isAddrClaim(aMsg)) {										// Else if it's an address claim? "I'm going to use this address. You ok with that?"
			handleAdderClaim(aMsg);											// Check to see if they are trying to take our address. Deal with this!
		}																			// 
		else if (isCantClaim(aMsg)) {										// Else if it's it's a can not claim an address?
			handleCantClaim(aMsg);											// We.. Well, I donno'. I guess it may have been ours, and this is a confirmation we won?
		}																			//
		else if (isCommandedAddr(aMsg)) {								// Someone, or something is trying to change our address.
			handleComAddr(aMsg);												// If this is all legal, in order, and makes sense. We'll do it.
		}
		else {																	// Else this is not something we handle..
			if (ourState==running) {										// If we are in a running state.
				done = false;													// Note we're not done.
				trace = (msgHandler*)getFirst();							// We go through our user's message handlers. Let them have a whack at it.
				while(!done) {													// For ever handler..
					if (trace) {												// If non-NULL..
						if (trace->handleMsg(aMsg)) {						// Can you haled this?
							done = true;										// If so? We are done.
						} else {													// Else can't handle it?
							trace = (msgHandler*)trace->getNext();		//	We grab the next handler on the list.
						}															// And start all over.
					} else {														// Else we hit a NULL?
						done = true;											// In this case we are also done.
					}																//
				}																	//
			}																		//
		}																			//
		delete(aMsg);															// And in the end of it all, we recycle the message object.
	}
}
			
					
// Calculate and start the startup time delay. Function of address.
void netObj::startHoldTimer(void) {
	
	if (addr>=0 && addr <=127) {		// If our address is in this range..
		holdTimer.setTime(.1);			// We set the timer to this.
		return;								// All done.
	}											// 
	if (addr>=248 && addr <=253) {	// No? Well if our address is in this range..
		holdTimer.setTime(.1);			// We set the timer to this.
		return;								// All done.
	}											//
	holdTimer.setTime(250);				// If not sett it to this.
}


// Assuming that in some way YOU fixed the error. This will clear the address error and
// restart the process. By YOU I mean let's say you had an address collision? YOU set a
// unclaimed address by calling setAddress() and now it's time to run on that new address.
void netObj::clearErr(void) {

	ourState = config;	// Next pass though idle() will kick off the machine.
}


// We need to change states. This is kinda' the flowchart of the program. What steps do we
// do, if even possible, to get from one state to another? List 'em here.
void netObj::changeState(netObjState newState) {
	
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
		case startHold :													// **   We are in startHold state   **
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
					ourState = running;									// Ok, we're running.
				break;														//
				default			: 								break;	// No other path to take here.
			}
		break;																//													//
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
		default					:								break;	// Other states will be switched elsewhere.
	}
}

	
// For debug prints. Outputs name for each state.
void netObj::stateName(netObjState aState) {
	
	switch(aState) {
		case config		: Serial.print("Configuration");		break;
		case startHold	: Serial.print("Start wait");			break;
		case arbit		: Serial.print("Arbitration");		break;
		case running	: Serial.print("Running");				break;
		case addrErr	: Serial.print("Address error");		break;
	}
}


// SET how we deal with addressing.
void netObj::setAddrCat(addrCat inAddrCat) {

	ourAddrCat = inAddrCat;
	setArbitraryAddrBit(ourAddrCat==arbitraryConfig);
}


// SEE how we deal with addressing.
addrCat netObj::getAddrCat(void) { return ourAddrCat; }


// Fine, our address is now inAddr.
void netObj::setAddr(byte inAddr) { addr = inAddr; }


// Here's our address.
byte netObj::getAddr(void) { return addr; }


// If we have a device's netName, see if we can find it's address.
byte netObj::findAddr(netName* inName) {

	addrNode*	aDevice;
	
	aDevice = ourAddrList.findName(inName);
	if (aDevice) {
		return aDevice->addr;
	}
	return NULL_ADDR;
}


// If we have a device's address, see if we can find it's name.
netName netObj::findName(byte inAddr) {

	addrNode*	aDevice;
	netName		aName;
	
	aName.clearName(false);
	aDevice = ourAddrList.findAddr(inAddr);
	if (aDevice) {
		aName = aDevice->name;
	}
	return aName;
}


// Another human readable printout.
void netObj::showAddrList(bool showNames) {
	
	ourAddrList.showList(showNames);
	Serial.println();
}


// Request address claimed. Someone is asking for everyone, or us, to show who they are
// and what address they are holding at this moment.
bool netObj::isReqAddrClaim(message* inMsg) {
	
	if (inMsg->getPDUf()==REQ_ADDR_CLAIM_PF && inMsg->getNumBytes()==3) {
		return true;
	}
	return false;
}


// Address Claimed. Someone is telling the world that this is the address they are going
// to use. If this conflicts with yours? Deal with that.
bool netObj::isAddrClaim(message* inMsg) {
	
	if (inMsg->getPDUf()==ADDR_CLAIMED_PF && inMsg->getPDUs()==GLOBAL_ADDR && inMsg->getNumBytes()==8) {
		return true;
	}
	return false;
}


// Someone is telling the network that they can not claim an address at all.
bool netObj::isCantClaim(message* inMsg) {
	
	if (inMsg->getPDUf()==ADDR_CLAIMED_PF && inMsg->getPDUs()==GLOBAL_ADDR && inMsg->getSourceAddr()==NULL_ADDR && inMsg->getNumBytes()==8) {
		return true;
	}
	return false;
}


// Someone is telling somebody to change their address to a given new value. (The 9th byte)
bool netObj::isCommandedAddr(message* inMsg) {
	
	if (inMsg->getPGN()==COMMAND_ADDR && inMsg->getNumBytes()==9) return true;
	return false;
}


// We have a new message asking us, or everyone, what address we are and what our name is.
void netObj::handleReqAdderClaim(message* inMsg) {
	
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
void netObj::handleAdderClaim(message* inMsg) {

	netName	aName;
	byte		buff[8];
	
	for(int i=0;i<8;i++) {													// ASSUMING this is addr claim message AND it has it's address..
		buff[i] = inMsg->getDataByte(i);									// Copy each data byte to our local buffer.
	}																				//
	aName.setName(buff);														// netNames can set their values from a byte buffer.
	ourAddrList.addAddr(inMsg->getSourceAddr(),&aName);			// In all cases we add this address/name pair to our list.
	switch(ourState) {														// Now, lets see what's what..
		case arbit		:														// Arbitrating. (We can arbitrate then!)
			if (inMsg->getSourceAddr()==addr) {							// Claiming our address!?
				if (waitingForClaim) {										// If we are waiting for a contesting claim..
					if (inMsg->msgIsLessThanName(this)) {				// If they win the arbitration..
						sendAddressClaimed(false);							// Let them know, that we know, that they won.
						addr = NULL_ADDR;										// We give up the address. This flags it for us as well.
					} else {														// Else, we win the name fight.
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
void netObj::handleCantClaim(message* inMsg) { }


// There is a command that sends netObj's new addresses to switch to. We deal with these
// here.
void netObj::handleComAddr(message* inMsg) {
	
	bool sameName;
	
	if (ourAddrCat==commandConfig) {								// Only commandConfig addressing can do this.
		if (inMsg->getNumBytes()==9) {							// These messages MUST have 9 byte data sections.
			sameName = true;											// Ok assuming it's the same name as us..
			for(int i=0;i<8;i++) {									// We'll check to make sure.
				if (name[i] != inMsg->getDataByte(i)) {		// If any bytes don't match..
					sameName = false;									// We fail the test.
				}															//
			}																//
			if (sameName) {											// Is this message carrying our name in it? (Is it for us?)
				if (ourState==running) {							// If our state is running. The only state we COULD see it in.
					setAddr(inMsg->getDataByte(8));				// Grab the address and plug it in.
					sendAddressClaimed(true);						// Tell the neighborhood.
				}
			}
		}
	}
}

	
// Calculate and start the arbitration time delay. Random function.	
void netObj::startArbitTimer(void) {

	long	rVal;
	
	rVal = random(TWMIN_MS, TWMAX_MS);
	arbitTimer.setTime(rVal * 0.6);
}


// From whatever state we are in now, start arbitration.
void netObj::startArbit(void) {
		
	if (addr==NULL_ADDR) {								// If we have no current address..
		ourXferList.dumpList();							// Clear out transfer list. Can't finish any.
		refreshAddrList();								// Makes the call and starts the timer.
		ourArbitState = waitingForAddrs;				// Note that we are gathering addresses again.
	} else {													// Else we DO have an address..
		sendAddressClaimed();							// See if we can claim what we got.
		arbitTimer.setTime(250);						// Set the timer.
		ourArbitState=waitingForClaim;				// Note that we are waiting for a claim.
	}
}


// RE:ISO 11783 - Arbitators that do not have an assigned preferred address or cannot
// claim their preferred address shall claim an address in the range of 128 to 247.

// Ok, we need an address. Go through the allowed list checking with our generated list of
// claimed addresses. If you can't find a value? Grab it as our own and pass it back! If
// we find ALL of them? Pass back a NULL_ADDR as a fail.
byte netObj::chooseAddr(void) {

	int i;
	
	for(i=128;i<248;i++) {
		if (!ourAddrList.findAddr(i)) {
			return i;
		}
	}
	return NULL_ADDR;
}
	
	
// Arbitration is all about timers. That and adress lists..
void netObj::checkArbit(void) {
	
	if (ourArbitState==waitingForAddrs) {			// If gathering addresses, to choose a new one..
		if (arbitTimer.ding()) {						// If gathering's over..
			arbitTimer.reset();							// Shut off the timer.
			addr = chooseAddr();							// Choose an address using the list to compare.
			if (addr!=NULL_ADDR) {						// If we found an unclaimed one..
				sendAddressClaimed(true);				// Send address claim on new address.
				startArbitTimer();						// Start the claim timer.
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
				changeState(running);					// Whoo hoo! Go to running state.
			}
		}
	}
}
		
		
// This sends the request to inAddr to send back their name. inAdder can be specific or
// GLOBAL_ADDR to hit everyone. The hope is that whomever called this cleared out the
// address list. Not the end of the world if not.
void netObj::sendRequestForAddressClaim(byte inAddr) {

	message	ourMsg(3);						// Create a message with 3 byte data buffer.
	
	ourMsg.setDataByte(0,0);				// Byte zero, gets zero.
	ourMsg.setDataByte(1,0xEE);			// Byte one, gets 0xEE.
	ourMsg.setDataByte(2,0);				// Byte 2 gets zero. These three are saying "Send your name!"
	ourMsg.setSourceAddr(getAddr());		// Set our address.
	ourMsg.setPGN(REQ_MESSAGE);			// Set the PGN..
	ourMsg.setPDUs(inAddr);					// Then set destination address as lower bits of PGN.
	outgoingingMsg(&ourMsg);				// Off it goes!
}


void netObj::sendAddressClaimed(bool tryFail) {
	
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
	outgoingingMsg(&ourMsg);					// Off it goes!													
}


void netObj::sendCannotClaimAddress(void) { sendAddressClaimed(false); }


// Address command :
// There is a myriad ways for this command to fail. The documentation for it, in the book
// I used was scrambled up by an uncaught editing error. I was able to piece together what
// I think is how it works? Basically, you broadcast a name with the 9th data byte set the
// new address that the named item should move to. Of all the values for PDU format &
// specific etc. That I came up with, I choose the ones that turned up the most, and that
// the different pieces matched up mathematically. I still don't know what the value 216
// has to do with anything.
//
// So to use this, find the name of the item you would like to move to a new address. Drop
// the name and new address in here and it'll send out the message. You will need to
// update the address list by calling sendRequestForAddressClaim(GLOBAL_ADDR), wait for a
// few ms, 750? Then check your list to see if your guy actually moved or not.
void netObj::addrCom(netName* nameObj,byte newAddr) {
	
	message	comMsg;
	byte*		namePtr;
											
	if (nameObj) {														// Sanity, make sure they actually sent a name.
		comMsg.setNumBytes(9);										// Extra byte needed for this one.
		comMsg.setPGN(COMMAND_ADDR);								// Set in the command PGN.
		comMsg.setPriority(DEF_PRIORITY);						// And this.
		comMsg.setSourceAddr(addr);								// Our current address.		
		namePtr = nameObj->getName();								// Ok, we get a pointer to their name data.
		for (int i=0;i<8;i++) {										// For each byte..
			comMsg.setDataByte(i,namePtr[i]);					// Plunk it into our comMsg data buffer.
		}																	//
		comMsg.setDataByte(8,newAddr);							// In the (9th) byte, stuff in the new address.
		outgoingingMsg(&comMsg);									// Send it on it's way!
	}														
}

	
// Deal with timers, See if any msgHandler's need to output messages of their own. Or other chores
// we know nothing about.
void netObj::idle(void) {

	msgHandler*			trace;
	
	switch(ourState) {
		case config		:										// We're in config state. Time to start up!
			changeState(startHold);							// First pass through idle in config state. Start holding.
		break;													//
		case startHold	:										// In startHold state.
			if (holdTimer.ding()) {							// If the time is up..
				holdTimer.reset();							// Shut off the timer.
				if (ourAddrCat==arbitraryConfig) {		// If we do arbitration..
					changeState(arbit);						// Slide into arbitration gear.
				} else {											// Else, we don't do arbitration?
					changeState(running);					// Then we start running.
				}													//
			}														//
		break;													//
		case arbit		:										// In arbitration state. We'll check what's up.
			checkMessages();									// First see if there's a message waiting for us.
			checkArbit();										// Then check the state of our arbitration.
		break;													//
		case running	:										// We're in running state. Let the CAs have some runtime.
			checkMessages();									// First see if there's a message waiting for us.
			trace = (msgHandler*)getFirst();				// Well start at the beginning and let 'em all have a go.
			while(trace) {										// While we got something..
				trace->idleTime();							// Give 'em some time to do things.
				trace = (msgHandler*)trace->getNext();	// Grab the next one.
			}														//
			if (claimTimer.ding()) {						// If the claim timer dings. Means it was running..
				claimTimer.reset();							// We shut it off.
			}
		break;													//
		default			:						break;		// Anything else? Basically do nothing.
	}		
}



// ***************************************************************************************		
//                     -----------  msgHandler class  -----------
// ***************************************************************************************


msgHandler::msgHandler(netObj* inNetObj)
	: linkListObj() {
	
	ourNetObj	= inNetObj;		// Pointer back to our "boss".
   intervaTimer.reset();		// Default to off.
}


msgHandler::~msgHandler(void) { }


// Fill this in if you'd like to read messages.
bool msgHandler::handleMsg(message* inMsg) { return false; }


// Fill this in if you'd like to create and send messages.
void msgHandler::newMsg(void) { }


// The created messages are sent by this guy.
void msgHandler::sendMsg(message* inMsg) { ourNetObj->outgoingingMsg(inMsg); }


// Broadcasting typically is done on a clock. Set the time interval with this call.
void msgHandler::setSendInterval(float inMs) {

   if (inMs>0) {
      intervaTimer.setTime(inMs);
   } else {
      intervaTimer.reset();
   }
}
 
 
// To be complete, you can read out the time interval with this call.
float msgHandler::getSendInterval(void) {  return intervaTimer.getTime(); }
 

// Same as idle, but called by netContorl.	 
void  msgHandler::idleTime(void) {

   if (intervaTimer.ding()) {
      newMsg();
      intervaTimer.stepTime();
   }
}
			