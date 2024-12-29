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


// This takes our data bytes, MUST be eight in this case. Converts it to a netName and
// compares it to the passed in name. If the message's name is less in value than the
// passed in name, message owner's name wins the battle.
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

	name[2] &= ~(0x7<<5); //Clear bits byte 3
	name[3] = 0; //Clear bits byte 4
	name[2] = name[2] | (manfCode<<3 & 0xE0);
	name[3] = manfCode>>3;
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


void netName::showName(void) {
	
	Serial.print("ISO ID          : "); Serial.println(getID());			// Device ID. Serial #
	Serial.print("Manuf code      : "); Serial.println(getManufCode());	// Who made you?					
	Serial.print("ECU Inst.       : "); Serial.println(getECUInst());		// What netObj# are you?
	Serial.print("Funct. Inst.    : "); Serial.println(getFunctInst());	// What # of your thing are you?
	Serial.print("Actual Funct.   : "); Serial.println(getFunction());	// Depth transducer. Or whatever.
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


// ***************************************************************************************
//				           -----    addrList   &  addrNode    -----
// ***************************************************************************************

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

	addrNode* trace;
	
	Serial.println(   "----  Address list ----");
	Serial.print("Number items : ");
	Serial.println(getCount());
	trace = (addrNode*)getFirst();
	while(trace) {
		Serial.print("Address         : ");
		Serial.println(trace->addr);
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
xferNode::xferNode(message* inMsg,netObj* inNetObj)
	: linkListObj() {
	
	complete = true;					// Default to an error. Delete me!
	msg = new message(inMsg);		// Create our copy of the initial message.
	if (msg && inNetObj) {			// If we got the message copied. And non NULL netObj..
		complete = false;				// Clear the kill order.
		ourNetObj = inNetObj;		// Save off our netObj pointer.
	}										//
}
	

// Here is where we delete our copy of the initial message. Just so you know.	
xferNode::~xferNode(void) { if (msg) delete(msg); } // Recycle the initial message.


void xferNode::addMsgToQ(void) {

	msgObj*	newMsg;
	
	newMsg = new msgObj(msg);					// Make up a msgObj and stuff it in there.
	if (newMsg) {									// Got one?
		ourNetObj->ourMsgQ.push(newMsg);		// Stuff it into the queue.
	}
}



//				          -----    outgoingBroadcast    -----


outgoingBroadcast::outgoingBroadcast(message* inMsg,netObj* inNetObj)
	: xferNode(inMsg,inNetObj) {  }
	
	
outgoingBroadcast::~outgoingBroadcast(void) {  }
	

bool outgoingBroadcast::handleMsg(message* inMsg) {
	
	return false;
}

		
void outgoingBroadcast::idleTime(void) {
	
	
}



//				          -----    outgoingPeerToPeer    -----


outgoingPeerToPeer::outgoingPeerToPeer(message* inMsg,netObj* inNetObj)
	: xferNode(inMsg,inNetObj) {  }
	
	
outgoingPeerToPeer::~outgoingPeerToPeer(void) {  }

	
bool outgoingPeerToPeer::handleMsg(message* inMsg) {

	return false;
}

	
void outgoingPeerToPeer::idleTime(void) {

}



//				          -----    incomingBroadcast    -----


incomingBroadcast::incomingBroadcast(message* inMsg,netObj* inNetObj)
	: xferNode(inMsg,inNetObj) {
	
	if (msg) {																			// If we got the initial message..
		msgSize	= pack16(msg->getDataByte(2),msg->getDataByte(1));		// Copy out the message size.
		msgPacks	= msg->getDataByte(3);											// Grab the number of packets.
		msgPGN	= pack16(msg->getDataByte(7),msg->getDataByte(6));		// Grab the top two bytes..
		msgPGN 	= msgPGN << 8;														// Shift 'em up by a byte.
		msgPGN 	= msgPGN & 0x00ffff00;											// Zero out everything else int our PGN.
		msgPGN 	= msgPGN | msg->getDataByte(5);								// Stuff in the least significant byte.
		msgAddr	= NULL_ADDR;														// Group broadcasts have no source address.
		delete(msg);																	// Done with incoming message. recycle it.
		msg = new message(msgSize);												// Have a shot at creating the new larger message.
		if (msg) {																		// If we were able to allocate the new message..
			msg->setPGN(msgPGN);														// We set it's PGN.
			msgPackNum = 1;															// Set the packet number.
			byteTotal = 0;																// We ain't transferred any yet.
			xFerTimout.setTime(750);												// We start the timeout timer.
		} else {																			// Else? We couldn't get the RAM?
			complete = true;															// Call for our recycling, we're done.
		}
	}																						//
}
	
	
incomingBroadcast::~incomingBroadcast(void) { }
	

bool incomingBroadcast::handleMsg(message* inMsg) {

	int	i;
	
	if (inMsg && !complete) {															// Sanity! Non NULL message and NOT complete?
		if (inMsg->getPDUf()==DATA_XFER_PF) {										// If it's a data packet..
			if (inMsg->getPDUs()==GLOBAL_ADDR) {									// If it's a GLOBAL data packet..
				if (inMsg->getNumBytes()==8) {										// AND if it has exactly 8 bytes data..
					if (inMsg->getDataByte(0)==msgPackNum) {						// Ok. If this matches our desired packet num..
						i = 1;																// Fine! We'll take it. Set up a counter.
						while(byteTotal<msgSize&&i<8) {								// While we have data to transfer and a place to store it.
							msg->setDataByte(byteTotal,inMsg->getDataByte(i));	// We transfer bytes.
							byteTotal++;													// Bump up the total transferred.
							i++;																// Bump local count.
						}																		// 
						msgPackNum++;														// Bump up our packet ID num.
						xFerTimout.start();												// Restart the timeout timer.
						if (byteTotal==msgSize) {										// If we got ALL the bytes?
							addMsgToQ();													// All done, add what we built to the queue.
							complete = true;												// Call for our recycling, we're done!
						}																		//
						return true;														// Hey, we we handled that one!
					}																			//
				}																				//
			}																					//
		}																						//
	}																							//
	return false;																			// Wasn't what we were looking for, not handled.
}	


void incomingBroadcast::idleTime(void) {

	if (xFerTimout.ding()) {	// If our timeout timer expires?
		complete = true;			// Call for our recycling, we're done!
	}
}



//				         -----    incomingPeerToPeer    -----


incomingPeerToPeer::incomingPeerToPeer(message* inMsg,netObj* inNetObj)
	: xferNode(inMsg,inNetObj) { }
	
	
incomingPeerToPeer::~incomingPeerToPeer(void) { }

	
bool incomingPeerToPeer::handleMsg(message* inMsg) {

	
	return false;
}


void incomingPeerToPeer::idleTime(void) {

}



//				            -----    xferList    -----


xferList::xferList(void)
	: linkList(), idler() { ourNetObj = NULL; }
	
	
xferList::~xferList(void) {  }


void xferList::begin(netObj* inNetObj) { ourNetObj = inNetObj; }


// Either we create a new outgoing extended message. Or, we received from the net a new
// incoming extended message. Create the suitable handler node with the initial message
// that started it. Then, add this new node to the xferNode list.
void xferList::addXfer(message* ioMsg,xferTypes xferType) {

	xferNode*	newXferNode;
	
	newXferNode = NULL;
	switch(xferType) {
		case broadcastIn		:	// We received a "BAM message".
			newXferNode = (xferNode*) new incomingBroadcast(ioMsg,ourNetObj);
		break;
		case broadcastOut		:	// We created a "BAM message".
			newXferNode = (xferNode*) new outgoingBroadcast(ioMsg,ourNetObj);
		break;
		case peerToPeerIn		:	// We received a "request to send" from a peer.
			newXferNode = (xferNode*) new incomingPeerToPeer(ioMsg,ourNetObj);
		break;
		case peerToPeerOut	:	// We created a "request to send" for a peer.
			newXferNode = (xferNode*) new outgoingPeerToPeer(ioMsg,ourNetObj);
		break;
	}
	if (newXferNode) {
		addToTop(newXferNode);
	}
}


// Ok, a message has come in from either the net, OR, we wrote it. It could be a start of
// a transfer we need to deal with. It could be part of a message we are already dealing
// with. Most likely it's nothing that concerns us. But, we get first right of refusal. So
// lets have a look at it.
bool  xferList::handledMsg(message* ioMsg,bool received) {

	xferNode*	trace;
	bool			handled;
	
	handled = false;															// Assume we don't handle this.
	if (ioMsg && ourNetObj) {												// Sanity first, is it not NULL? Is netObj not NULL?
		if (received) {														// If its from the net..
			if (ioMsg->getPDUf()==BAM_PF) {								// If we have an incoming BAM message..
				if (ioMsg->getPDUs()==GLOBAL_ADDR) {					// If the message is a broadcast..
					addXfer(ioMsg,broadcastIn);							// Setup a brodcast transfer.
					handled = true;											//
				} else {															// Else it's peer to peer..
					if (ioMsg->getPDUs()==ourNetObj->getAddr()) {	// If it's actually to us..
						addXfer(ioMsg,peerToPeerIn);						// Setup a brodcast transfer.
						handled = true;										//
					}																//
				}																	//
			}																		//
		} else {																	// Else we wrote it!
			if (ioMsg->getNumBytes()>8) {									// If message is oversized..
				if (ioMsg->getPDUs()==GLOBAL_ADDR) {					// If the message is a broadcast..
					addXfer(ioMsg,broadcastOut);							// Setup a brodcast transfer.
				} else {															// Else it's NOT a broadcast..
					addXfer(ioMsg,peerToPeerOut);							// Set up a peer to peer transfer.
				}																	//
				handled = true;												// In any case, it's been handled.
			}																		//
		}																			//
		if (!handled) {														// If the message has NOT been handled..
			trace = (xferNode*)getFirst();								// Grab first handler node from the list.
			while(trace && !handled) {										// While we have non NULL node AND message has not been handled..
				handled = trace->handleMsg(ioMsg);						// Ask each node if they want/need to handle this message.
				trace = (xferNode*)trace->getNext();					// Then jump to the next node regardless of the answer.
			}																		// That's it. either the message was handled or not.
		}																			// 
	}																				//
	return handled;															// Return the final result.
}


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


// When a message comes in from the net, pass it in here. -(8 or less data bytes)-
void netObj::incomingMsg(message* inMsg) {

	msgObj*	newMsg;
	
	if (inMsg) {												// First sanity. Did they slip us a NULL?
		if (!ourXferList.handledMsg(inMsg,true)) {	// Not NULL. Ok, if the xfer list doesn't want it..
			newMsg = new msgObj(inMsg);					// Make up a msgObj and stuff it in there.
			if (newMsg) {										// Got one?
				ourMsgQ.push(newMsg);						// Stuff it into the queue.
			}
		}
	}
}


// When we want a message sent out, pass it in here. -(Can have > 8 data bytes)-
void netObj::outgoingingMsg(message* outMsg) {

	if (outMsg) {											// First sanity. Always check for NULL.
		if (outMsg->getNumBytes()>8) {				// Ok, If we have more than 8 databytes..
			ourXferList.handledMsg(outMsg,false);	// Pass the message over to the xfer list.
		} else {												// Else, we are within 8 data bytes limit..
			sendMsg(outMsg);								// Shove the message out the wire.
		}
	}
}


// First  we see if it's a network task that we have to handle ourselves. Then, if not, we
// ask each the handlers if one of them can handle it. Once a msgHandler handles it, or
// none will. We are done.	-(Can have > 8 data bytes)-
void netObj::checkMessages(void) {

	msgObj*			aMsg;
	msgHandler*		trace;
	bool				done;
	
	aMsg = (msgObj*)ourMsgQ.pop();
	if (aMsg) {
		if (isReqAddrClaim(aMsg)) {
			handleReqAdderClaim(aMsg);
		}
		else if (isAddrClaim(aMsg)) {
			handleAdderClaim(aMsg);
		}
		else if (isCantClaim(aMsg)) {
			handleCantClaim(aMsg);
		}
		else if (isCommandedAddr(aMsg)) {
			handleComAddr(aMsg);
		}
		else {
			if (ourState==running) {
				done = false;
				trace = (msgHandler*)getFirst();
				while(!done) {
					if (trace) {
						if (trace->handleMsg(aMsg)) {
							done = true;
						} else {
							trace = (msgHandler*)trace->getNext();
						}
					} else {
						done = true;
					}
				}
			}
		}
		delete(aMsg);
	}
}
			
					
// Calculate and start the startup time delay. Function of address.
void netObj::startHoldTimer(void) {
	
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
}

	
// For debug prints. Outputs name for each state.
void netObj::stateName(netObjState aState) {
	
	switch(aState) {
		case config		: Serial.print("Configuration");	break;
		case startHold	: Serial.print("Start wait");		break;
		case arbit		: Serial.print("Arbitration");	break;
		case running	: Serial.print("Running");			break;
		case addrErr	: Serial.print("Address error");	break;
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
	Serial.print(  "OUR Addr & name : ");
	Serial.println(addr);
	showName();
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

	
// Calculate and start the claim time delay. Random function.	
void netObj::startClaimTimer(void) {

	long	rVal;
	
	rVal = random(0, 256);
	arbitTimer.setTime(rVal * 0.6);
}

	
// From whatever state we are in now, start arbitration.
void netObj::startArbit(void) {
		
	if (addr==NULL_ADDR) {								// If we have no current address..
		ourAddrList.dumpList();							// Clear out list of addresses.
		ourXferList.dumpList();							// Clear out transfer list as well. Can't finish any.
		sendRequestForAddressClaim(GLOBAL_ADDR);	// Start gathering addresses again.
		arbitTimer.setTime(250);						// Set the timer.
		ourArbitState = waitingForAddrs;				// Note that we are gathering addresses again.
	} else {													// Else we DO have an address..
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
	sendMsg(&ourMsg);							// Off it goes!
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
	sendMsg(&ourMsg);								// Off it goes!													
}


void netObj::sendCannotClaimAddress(void) { sendAddressClaimed(false); }


void netObj::sendCommandedAddress(byte comAddr) {
	
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
			