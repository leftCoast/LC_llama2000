#ifndef SAE_J1939_h
#define SAE_J1939_h

#include <lists.h>
#include <idlers.h>
#include <timeObj.h>
#include <resizeBuff.h>

// If you are reading this and wondering what the heck it all means? Buy this book.

// A Comprehensible Guide to J1939 
// By Wilfred Voss.
//
// This is the book I bought used on Amazon. I thought it was a pretty rough learning
// curve. But pretty much it's all there. Most websites I've seen that say they are a
// guide to this stuff are just exact copies of that book.
//
//                     +     +     +     +     +     +     +  
//
// This (SAE J1939 is all built on CAN Bus. Each message has a 39 bit header. Pared down
// to 29 bits of message ID info. (Classic CAN has only 11 bits). Followed by 0..8 bytes
// of data.
// 
// Ok, here's what we end up reading from the CAN BUS chip after what we don't need is
// stripped out for us. Well, and if we are sending a message, these are what we end up
// sending as well.
//
//  [                       29 bits of CAN ID                            ]       Data
//  ----------------------------------------------------------------------  -------------
// |  3 bits  |  6 bits  |   1 bit  |   1 bit   |   1 byte   |   1 byte   | 0 to 8 bytes |
// | Priority | all zero | reserved | Data page | PF or PDUf | PS or PDUs | message data |
//             ----------------------------------------------------------
//                          This underlined section is your PGN
//
// This is what we decode into your message. Oh and notice there is 0 to 8 bytes of data
// coming along with this. The message gets that as well.
//
// People go on about the PGN. But, as you can see, that value overlaps (Is a function of)
// other values and can vary. Not always the best value to use as a message ID. For
// example the PS part can mean Global address, or specific address, changing the value of
// the PGN.
//
// So the message class here only holds the different parts. When you ask it for a PGN, it
// calculates one for you from those parts. If you set in a new PGN, it will calculate the
// different parts that match that PGN back into the message.
//
// And, along with all of this, there are some very common control values. Most of the
// heavy hitters are set out here for use in the code.


#define GLOBAL_ADDR		255		// Destination only address. "Hey everyone!"
#define NULL_ADDR			254		// Source address only. "Hey, I have no address."
#define REQ_MESSAGE		59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED		60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.
#define COMMAND_ADDR		65240		// We were told to use this address.
#define BAM_COMMAND		60416		// Big load coming! Make room!

#define REQ_ADDR_CLAIM_PF	234	// 0xEA, PS = Destination addr.
#define BAM_PF					236	// PS = 255 or Destination addr.
#define ADDR_CLAIMED_PF		238	// 0xEE, PS = 255. Works for both (ACK) & (NACK)
#define COMMAND_ADDR_PF		254	// PS = 216. Giving PGN of COMMAND_ADDR above.


#define DEF_NUM_BYTES	8			// Remember data is 0..8 bytes? Most are 8 bytes. We default to that.
#define DEF_PRIORITY		6			// Seems that 6 is the preferred default priority.
#define DEF_R				false		// R (reserved bit) All the doc.s say to leave it as 0.
#define DEF_DP				false		// DP (Data page) All doc.s say to leave it  as 0. But nearly ALL NMEA 2000 sets it as high order PGN bit (1).

class netName;							// Forward class thing. Don't worry about it.
class msgHandler;						// And another one. Just look the other way. Maybe hum a little.


// Typically unused data bytes are set to 0xFF. These can be used as quick and easy way to
// test if a field read in from a message, is all set to ones IE : unused.

bool	isBlank(uint8_t inVal);
bool	isBlank(uint16_t inVal);
bool	isBlank(uint32_t inVal);



//				----- message -----


class message {

	public:
				message(int inNumBytes=DEF_NUM_BYTES);
				message(message* inMsg);

	virtual	~message(void);
	
				void		setNumBytes(int inNumBytes);
				int		getNumBytes(void);	
				void		setCANID(uint32_t CANID);
				uint32_t	getCANID(void);	
				void		setPGN(uint32_t PGN);
				uint32_t	getPGN(void);
				void		setPriority(byte inPriority);
				byte		getPriority(void);
				void		setR(bool inR);
				bool		getR(void);
				void		setDP(bool inDP);
				bool		getDP(void);
				void		setPDUf(byte inPDUf);
				byte		getPDUf(void);
				void		setPDUs(byte inPDUs);
				byte		getPDUs(void);
				void		setSourceAddr(byte inSourceAddr);
				byte		getSourceAddr(void);
				void		setDataByte(int index,byte inByte);
				byte		getDataByte(int index);
				void		showMessage(void);
				bool		msgIsLessThanName(netName* inName);
				
	protected:
				int		numBytes;	// Size of our data buffer.
				byte*		msgData;		// The data buffer itself!
				uint8_t	priority;	// CAN priority bits.
				bool		R;				// Reserve bit.
				bool		DP;			// Data page.
				uint8_t	PDUf;			// PDU format
				uint8_t	PDUs;			// PDU specific.
				uint8_t	sourceAddr;	// Who sent this?
};



//				-----            BAMmsg            -----


// [32] [Size LSB] [Size MSB] [numPacks] [0xFF] [PGN LSB] [PGN2] [PGN MSB]
// NOT FINISHED YET

class BAMmsg :	public message {

	public:
				BAMmsg(void);
				BAMmsg(message* inBAMMsg);
	virtual	~BAMmsg(void);
	
				void		setupBAM(byte destAddr,int numBytes,byte numPacks,uint32_t PGN);
				int		getBAMNumBytes(void);
				byte		getBAMNumPacks(void);
				uint32_t	getBAMPGN(void);
};


	
//				----- netObj name -----

// Another rule of network control is that each object on the network has an address and, 
// needs to have what they call, it's name. The name is 64 bits long. (Just fits into 8
// bytes of data) It has 10 fields of information. Some fields are bought from the SAE
// people for massive amount of cash. Some you choose. Some are configuration values. When
// one is asked for their address they also hand back in the data bytes their name.
//
//                 SAE list               SAE list  SAE   SAE list                        SAE list
// |    1 bit    |  3 bits  |   4 bits   |7 bits|  1 bit | 8 bits |  5 bits   | 3 bits  | 11 bits  |21 bits|
// |Can arbitrate|Ind. group|System inst.|System|Reserved|Function|Funct inst.|ECU inst.|Manu. code|ID num |
//
// The following is a list of stuff that can go into sections of these names.

// Industry group. These fit an enum well.

enum indGroup {

	Global,
	Highway,
	Agriculture,
	Construction,
	Marine,
	Industrial
};


// deviceClass values. These values are NOT uniform. So, we'll do the #define thing.

#define	DEV_CLASS_RES			0		// Reserved for 2000 Use
#define	DEV_CLASS_SYS_TOOLS	10		// System tools
#define	DEV_CLASS_SAFETY		20		// Safety systems
#define	DEV_CLASS_NETWRK		25		// Internetwork device
#define	DEV_CLASS_ELEC_DIST	30		// Electrical Distribution
#define	DEV_CLASS_ELEC_GEN	35		// Electrical Generation
#define	DEV_CLASS_CONTROL		40		// Steering and Control surfaces
#define	DEV_CLASS_PROPEL		50		// Propulsion
#define	DEV_CLASS_NAV			60		// Navigation
#define	DEV_CLASS_COMS			70		// Communication
#define	DEV_CLASS_SENSE_COM  75		// Sensor Communication Interface
#define	DEV_CLASS_INST			80		// Instrumentation/general systems
#define	DEV_CLASS_EXT_ENV		85		// External Environment
#define	DEV_CLASS_INT_ENV		90		// Internal Environment
#define	DEV_CLASS_DECK_EQP	100	// Deck + cargo + fishing equipment systems
#define	DEV_CLASS_UI			110	// User Interface
#define	DEV_CLASS_DISP			120	// Display
#define	DEV_CLASS_ENT			125	// Entertainment


// Device function.. OH lord! These are listed by.. deviceClass.

//  DEV_CLASS_SYS_TOOLS
#define	DEV_FUNC_DIAG			130	// Diagnostic.
#define	DEV_FUNC_LOGGER		140	// Bus Traffic Logger.

// DEV_CLASS_SAFETY
#define	DEV_FUNC_ENC			110	// Alarm Enunciator.
#define	DEV_FUNC_EPIRB			130	// Emergency Position Indicating Radio Beacon (EPIRB)
#define	DEV_FUNC_OVERBOARD	135	// Man Overboard
#define	DEV_FUNC_DATA_LOG		140	// Voyage Data Recorder
#define	DEV_FUNC_CAMERA		150	// Camera

// DEV_CLASS_NETWRK
#define	DEV_FUNC_GATE			130	// PC Gateway
#define	DEV_FUNC_NMEA_ALOG	131	// NMEA 2000 to Analog Gateway
#define	DEV_FUNC_ALOG_NMEA	132	// Analog to NMEA 2000 Gateway
#define	DEV_FUNC_NMEA_SER		133	// NMEA 2000 to Serial Gateway
#define	DEV_FUNC_NMEA_0183	135	// NMEA 0183 Gateway
#define	DEV_FUNC_NMEA_NET		136	// NMEA Network Gateway
#define	DEV_FUNC_NMEA_LAN		137	// NMEA 2000 Wireless Gateway
#define	DEV_FUNC_ROUTER		140	// Router
#define	DEV_FUNC_BRIDGE		150	// Bridge
#define	DEV_FUNC_REPEAT		160	// Repeater

// DEV_CLASS_ELEC_DIST
#define	DEV_FUNC_BINARY		130	// Binary Event Monitor
#define	DEV_FUNC_LOAD_CONT	140	// Load Controller
#define	DEV_FUNC_PWR_INP		141	// AC/DC Input
#define	DEV_FUNC_CONTROL		150	// Function Controller

// DEV_CLASS_ELEC_GEN
#define	DEV_FUNC_ENGINE		140	// Engine
#define	DEV_FUNC_ALT			141	// DC Generator/Alternator
#define	DEV_FUNC_SOLAR			142	// Solar Panel (Solar Array)
#define	DEV_FUNC_WIND			143	// Wind Generator (DC)
#define	DEV_FUNC_AC_BUS		152	// AC Bus
#define	DEV_FUNC_MAIN			153	// AC Mains (Utility/Shore)
#define	DEV_FUNC_AC_OUT		154	// AC Output
#define	DEV_FUNC_PC_CHRG		160	//Power Converter - Battery Charger
#define	DEV_FUNC_PC_CHRG_INV	161	//Power Converter - Battery Charger+Inverter
#define	DEV_FUNC_PC_INV		162	//Power Converter - Inverter
#define	DEV_FUNC_PC_DC_DC		163	//Power Converter - DC
#define	DEV_FUNC_BAT			170	// Battery
#define	DEV_FUNC_ENG_GATE		180	// Engine Gateway

// DEV_CLASS_CONTROL
#define	DEV_FUNC_FOLLOW		130	// Follow-up Controller
#define	DEV_FUNC_MODE			140	// Mode Controller
#define	DEV_FUNC_AUTOPILOT	150	// Autopilot
#define	DEV_FUNC_RUDDER		155	// Rudder
#define	DEV_FUNC_HEADING		160	// Heading Sensors
#define	DEV_FUNC_TRIM			170	//Trim (Tabs)/Interceptors
#define	DEV_FUNC_PITCH_ROLL	180	//Attitude (Pitch, Roll, Yaw) Control

// DEV_CLASS_PROPEL
#define	DEV_FUNC_ENG_MON		30		// Engineroom Monitoring
#define	DEV_FUNC_ENGINE		140	// Engine
#define	DEV_FUNC_DC_ALT		141	// DC Generator/Alternator
#define	DEV_FUNC_ENG_CONT		150	// Engine Controller
#define	DEV_FUNC_AC_ALT		151	// AC Generator
#define	DEV_FUNC_MOTOR			155	// Motor
#define	DEV_FUNC_ENG_GATEWAY 160	// Engine Gateway
#define	DEV_FUNC_TRANS			165	// Transmission
#define	DEV_FUNC_THROTTLE		170	// Throttle/Shift Control
#define	DEV_FUNC_ACT			180	// Actuator
#define	DEV_FUNC_GAUGE			190	// Gauge Interface
#define	DEV_FUNC_GAUGE_LRG	200	// Gauge Large
#define	DEV_FUNC_GAUGE_SM		210	// Gauge Small

// DEV_CLASS_NAV
#define	DEV_FUNC_DEPTH			130	// Bottom Depth
#define	DEV_FUNC_SP_DEPTH		135	// Bottom Depth/Speed
#define	DEV_FUNC_SP_DP_TEMP	136	// Bottom Depth/Speed/Temperature
#define	DEV_FUNC_ATTITUDE		140	// Ownship Attitude
#define	DEV_FUNC_GNSS			145	// Ownship Position (GNSS)
#define	DEV_FUNC_LORAN 		150	// Ownship Position (Loran C)
#define	DEV_FUNC_SPEED			155	// Speed
#define	DEV_FUNC_TURN_RATE	160	// Turn Rate Indicator
#define	DEV_FUNC_NAV_A			170	// Integrated Navigation
#define	DEV_FUNC_NAV_B			175	// Integrated Navigation System
#define	DEV_FUNC_NAV_C			190	// Navigation Management
#define	DEV_FUNC_AUTO_ID		195	// Automatic Identification System (AIS)
#define	DEV_FUNC_RADAR			200	// Radar
#define	DEV_FUNC_IR				201	// Infrared Imaging
#define	DEV_FUNC_ECDIS			205	// ECDIS
#define	DEV_FUNC_ECS			210	// ECS
#define	DEV_FUNC_DIR			220	// Direction Finder
#define	DEV_FUNC_V_STAT		230	// Voyage Status (What the hell does this mean?)

// DEV_CLASS_COMS
#define	DEV_FUNC_EPIRB			130	// EPIRB
#define	DEV_FUNC_AIS			140	// AIS
#define	DEV_FUNC_DSC			150	// DSC
#define	DEV_FUNC_TRANSCIEVE	160	// Data Receiver/Transceiver
#define	DEV_FUNC_SAT			170	// Satellite
#define	DEV_FUNC_MF_HF			180	// Radio-telephone (MF/HF)
#define	DEV_FUNC_PHONE			190	// Radiotelephone

// DEV_CLASS_SENSE_COM
#define	DEV_FUNC_TEMP			130	// Temperature
#define	DEV_FUNC_PRESSURE		140	// Pressure
#define	DEV_FUNC_LEVEL			150	// Fluid Level
#define	DEV_FUNC_FLOW			160	// Flow
#define	DEV_FUNC_HUMIDITY		170	// Humidity

// DEV_CLASS_INST
#define	DEV_FUNC_DATE_TIME	130	// Time/Date Systems
#define	DEV_FUNC_VDR			140	// VDR
#define	DEV_FUNC_INST			150	// Integrated Instrumentation
#define	DEV_FUNC_GP_DISP		160	// General Purpose Displays
#define	DEV_FUNC_GP_SENSE		170	// General Sensor Box
#define	DEV_FUNC_WEATHER		180	// Weather Instruments
#define	DEV_FUNC_GP_TRANS		190	// Transducer/General
#define	DEV_FUNC_NMEA_CONV	200	// NMEA 0183 Converter

// DEV_CLASS_EXT_ENV
#define	DEV_FUNC_ENV_AIR		130	// Atmospheric
#define	DEV_FUNC_ENV_WATER	160	// Aquatic

// DEV_CLASS_INT_ENV
#define	DEV_FUNC_HVAC			130	// HVAC

// DEV_CLASS_DECK_EQP
#define	DEV_FUNC_FISH_SCALE	130	// Scale (Catch)

// DEV_CLASS_UI
#define	DEV_FUNC_BTN			130	// Button Interface
#define	DEV_FUNC_SWITCH		135	// Switch Interface
#define	DEV_FUNC_ANALOG		140	// Analog Interface

// DEV_CLASS_DISP
#define	DEV_FUNC_DISP			130	// Display
#define	DEV_FUNC_ALARM			40	// Alarm Enunciator

// DEV_CLASS_ENT
#define	DEV_FUNC_PLAYER		130	// Multimedia Player
#define	DEV_FUNC_MEDIA_CONT	140	// Multimedia Controller


// Packed eight byte set of goodies.

class netName {
	
	public:
					netName(void);
	virtual		~netName(void);
		
		void		clearName(void);							// Want to zero our name out? This'll do it.
		bool		sameName(netName* inName);				// We the same as that guy?
		bool		isLessThanName(netName* inName);		// Is our name numerically less than that guy?
		byte*		getName(void);								// 64 bit - Pass back the packed up 64 bits that this makes up as our name.
		void		setName(byte* namePtr);					// Make this 64 bits, our name.
		void		copyName(netName* namePtr);			// Make us a clone of that.
		
		bool		getArbitraryAddrBit(void);				// 1 bit - True, we CAN change our address. 128..247
		void		setArbitraryAddrBit(bool AABit);		// False, we can't change our own address.
		indGroup	getIndGroup(void);						// 3 bit - Assigned by committee. Tractor, car, boat..
		void		setIndGroup(indGroup inGroup);
		byte		getSystemInst(void);						// 4 bit - System instance, like engine1 or engine2.
		void		setSystemInst(byte sysInst);
		byte		getVehSys(void);							// 7 bit - Assigned by committee. 
		void		setVehSys(byte vehSys);					//
																	// One bit reserved field. Set to zero. 
		byte		getFunction(void);						// 8 bit - Assigned by committee. 0..127 absolute definition.
		void		setFunction(byte funct);				// 128+ need other fields for definition.
		byte		getFunctInst(void);						// 5 bit - Instance of this function. Multi clone ECIUs?
		void		setFunctInst(byte functInst);			//
		byte		getECUInst(void);							// 3 bit - What controller instance are we?
		void		setECUInst(byte inst);					// 
		uint16_t	getManufCode(void);						// 11 bit - Assigned by committee. Who made this thing?
		void		setManufCode(uint16_t manufCode);	//
		uint32_t	getID(void);								// 21 bit - Unique Fixed value. Product ID & Serial number kinda' thing.
		void		setID(uint32_t inID);
		
		void		showName(void);							// Human readable printout.

	protected:
		
		byte		name[8];										// The stored 8 byte. 64 bit name.
};



//				-----    addrList   &  addrNode    -----

// When wondering if an address is unused, who serves up the best data, where something
// should be sent.. This is our internal list that show's everyone's address. for now it's
// used for searching for unused address values. Later we can add the names, and then it'll
// be a catalog.

class addrNode :	public linkListObj {

	public:
				addrNode(byte inAddr,netName* inName);
	virtual	~addrNode(void);
	
	virtual	bool	isGreaterThan(linkListObj* compObj);	// Are we greater than the obj being passed in?
	virtual	bool	isLessThan(linkListObj* compObj);		// Are we less than the obj being passed in?
	
				byte		addr;
				netName	name;
};



class addrList :	public linkList,
						public idler {

public:
				addrList(void);
	virtual	~addrList(void);
	
				void			addAddr(byte inAddr,netName* inName);
				addrNode*	findAddr(byte inAddr);
				addrNode*	findName(netName* inName);
				addrNode*	findPair(byte inAddr,netName* inName);
		
				void			showList(bool withNames=false);
};



//				-----    xferList   &  xferNode    -----

// This is work in progress. When needed to send more than eight bytes one needs to do a
// multi message transmission. This list is to track what multi part messages we are
// currently active. Either sending or receiving.

class xferNode :	public linkListObj {

	public:
				xferNode(BAMmsg* inMsg,bool inRecieve);
	virtual	~xferNode(void);
	
	virtual	void	idleTime(void);
	
	BAMmsg*	msg;
	byte*		buff;
	int		numBytes;
	bool		complete;
	bool		recieve;
};



class xferList :	public linkList,
						public idler {

public:
				xferList(void);
	virtual	~xferList(void);
	
	virtual	void	addXfer(BAMmsg* inMsg,bool inRecieve);
				void	listCleanup(void);
	virtual	void  idle(void);
};



//				----- msgQ. Get 'em and hold 'em in here. -----

// The idea is that messages can come in in bursts. Instead of handling each one as it
// comes in, we just copy them to this queue to be handled as we have time.

class msgObj :	public linkListObj,
					public message {
	public:
				msgObj(message* inMsg);
	virtual	~msgObj(void);
};					
					

class msgQ :	public queue {

public:
				msgQ(void);
	virtual	~msgQ(void);
};



//		----- netObj. Base class for allowing navigation of SAE J1939 networks -----

// This is the part that holds all the logic for the network. You'r controller as it were.
// It's purely virtual in that you can not create one of these. You have to inherit it as
// the base of your own class. And the class you create, from this, is responsible for
// understanding how to work your hardware. How to read CAN messages from it and send CAN
// messages out of it. The combination of the two give you something that can actively
// navigate the network.
//
// This netObj has the logic to navigate the addressing and maintenance communication. But
// the actual data messages YOU want to send or receive, need handlers to be written by
// inheriting msgHandler (below) as their base classes and attached to this netObj class
// during runtime by calling addMsgHandler() for each.


// Addressing categories for netObj's. Choose one.
enum addrCat {

	nonConfig,			// Address is hard coded.
	serviceConfig,		// You can hook up.. Something. To change our address.
	commandConfig,		// We can respond to address change messages.
	selfConfig,			// We set our own depending on how the network is set up.
	arbitraryConfig,	// We can do the arbitrary addressing dance.
	noAddress			// We have no address, just a listener?
};



// This is the base class that holds all the logic for navigating an SAE J1939 network.
// For those of you that are boaters out there. This is the actual network that NMEA 2000
// uses. Inherit this, create a class that can read in, and send out CAN BUS messages. And
// you will have an object that lets you read and write stuff on NMEA 2000 networks.
// 
// Although the learning curve will most likely give you massive headaches.


class netObj :	public linkList,
					public idler,
					public netName {							

	public:
				enum netObjState {															// Different states we can be in.
					config,																		// Still on the bench being assembled.
					startHold,																	// Certain addresses call for a start hold before beginning.
					arbit,																		// Doing address arbitration.
					addrErr,																		// Had an address error that we can't fix alone.
					running																		// Everything seems fine. We're running.
				};
				
				enum arbitState {																// Arbitration has it's own set of states.
					waitingForAddrs,															// Send us your addresses and names has been called. Gather them.
					waitingForClaim,															// Our address claim has been sent. Wait to see if it is challenged.
				};	
				
				netObj(void);
	virtual	~netObj(void);
	
	virtual	void		begin(netName* inName,byte inAddr,addrCat inAddCat);	// Initial setup.
	virtual	void		addMsgHandler(msgHandler* inCA);								// Add the handlers of the messages you would like to send/receive.
	virtual  void		sendMsg(message* outMsg)=0;									// You have to fill this one out.
	virtual  void		handleMsg(message* inMsg);										// When a message comes in, pass it into here.
				void		checkMessages(void);												// If we have one we'll grab it and dela with it.
				byte		findAddr(netName* inName);										// If we have a device's netName, see if we can find it's address.
				netName	findName(byte inAddr);											// If we have a device's address, see if we can find it's name.
				void		startHoldTimer(void);											// Calculate and start the address holding time delay. Function of address.
				void		clearErr(void);													// This will clear the address error and restart the process.
				void		changeState(netObjState newState);							// Keeping track of what we are up to.
				void		stateName(netObjState aState);								// state -> "state" kinda' thing for debugging.
														
				void		setAddrCat(addrCat inAddrCat);								// How we deal with addressing.
				addrCat	getAddrCat(void);													// See how we deal with addressing.
				void		setAddr(byte inAddr);											// Set a new address.
				byte		getAddr(void);														// Here's our current address.
				void		showAddrList(bool showNames);									// Another human readable printout.
				
				bool		isReqAddrClaim(message* inMsg);								// Is this a request for address claimed msg?
				bool		isAddrClaim(message* inMsg);									// Is this an address claimed msg?
				bool		isCantClaim(message* inMsg);									// Is this a fail to claim address msg?
				bool		isCommandedAddr(message* inMsg);								// Is this a commanded address msg?
				void		handleReqAdderClaim(message* inMsg);						// Handle a request for address claimed msg.
				void		handleAdderClaim(message* inMsg);							// Handle an address claimed msg.
				void		handleCantClaim(message* inMsg);								// Handle a failed to claim an address msg.				
				void		handleComAddr(message* inMsg);								// Handle a commanded address message.
				void		startClaimTimer(void);											// Calculate and start the claim time delay. Random function.
				void		startArbit(void);													// From whatever state we are in now, start arbitration.
				byte		chooseAddr(void);													// We have a list of claimed addresses and a range of allowed addresses. Find one.
				void		checkArbit(void);													// Arbitration is all about wait states.
				void		sendRequestForAddressClaim(byte inAddr);					// Tell us your name and address.
				void		sendAddressClaimed(bool tryFail=true);						// This is our name and address.
				void		sendCannotClaimAddress(void);									// We can't find an address!
				void		sendCommandedAddress(byte comAddr);							// HEY YOU! Set this as your address!
				
	virtual	void			idle(void);
	
				msgQ			ourMsgQ;															// A place to store incoming messages.
				
				netObjState	ourState;														// What state we are in now.
				timeObj		holdTimer;														// Our timer for startup holding.
																								
				addrCat		ourAddrCat;														// How we deal with addressing.
				byte			addr;																// Our current network address.
				arbitState	ourArbitState;													// Arbitration has a couple wait states.
				timeObj		arbitTimer;														// Arbitration timer.
				addrList		ourAddrList;													// List of used addresses from the network.
				
				xferList		ourXferList;													// The transport protocol list. (UNFINISHED)
};



//				----- msgHandler -----


// Base class for handling and creation of SAE J1939 network messages. Inherit this create
// your handler object and add them using the netObj call addMsgHandler().
class msgHandler :	public linkListObj {	

	public:
				msgHandler(netObj* inNetObj);
				~msgHandler(void);
				
				int	getNumBytes(void);				// These only set the value. No buffer here.
            void	setNumBytes(int inNumBytes);	// The message class reads  and uses this.
	virtual  bool	handleMsg(message* inMsg);		// Fill in to handle messages.
	virtual  void	newMsg(void);						// Fill in to create messages.
	virtual  void	sendMsg(message* inMsg);		// This one just sends messages on their way.
	
				void	setSendInterval(float inMs);	// Used for broadcasting.
            float	getSendInterval(void);			//
	virtual	void	idleTime(void);					// Same as idle, but called by the netObj.
				
				netObj*	ourNetObj;						// Pointer to our boss!
				timeObj	intervaTimer;					// If broadcasting, how often do we broadcast? (Ms)
};


#endif