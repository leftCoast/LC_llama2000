#ifndef SAE_J1939_h
#define SAE_J1939_h

#include <lists.h>
#include <idlers.h>
#include <timeObj.h>
#include <resizeBuff.h>				// Need to objectify this to a base class. Then any chip should work.


// CAN Header. This is all built on CAN Bus. Each message has a 39 bit header. Pared down
// to 29 bits of info. Followed by 0..8 bytes data.
// 
// PGN - Parameter Group Number. Basically what is this message about.
class ECUname;

#define GLOBAL_ADDR		255		// Destination only address. "Hey everyone!"
#define NULL_ADDR			254		// Source address only. "Hey, listen! But, I have no address."
#define REQ_MESSAGE		59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED		60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.
#define COMMAND_ADDR		65240		// We were told to use this one.
#define BAM_COMMAND		60416		// Big load coming! Make room!

#define REQ_ADDR_CLAIM_PF	234	// PS = Destination addr.
#define BAM_PF					236	// PS = 255 or Destination addr.
#define ADDR_CLAIMED_PF		238	// PS = 255. Works for both (ACK) & (NACK)
#define COMMAND_ADDR_PF		254	// PS = 216. Giving PGN of COMMAND_ADDR above.


#define DEF_NUM_BYTES	8
#define DEF_PRIORITY		6
#define DEF_R				false
#define DEF_DP				false

//				----- message -----

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


class message {

	public:
				message(int inNumBytes=DEF_NUM_BYTES);
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
				bool		msgIsLessThanName(ECUname* inName);
				
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


	
//				----- ECU name -----


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
class ECUname {
	
	public:
					ECUname(void);
	virtual		~ECUname(void);
		
		void		clearName(void);							// Want to zero our name out? This'll do it.
		bool		sameName(ECUname* inName);				// We the same as that guy?
		bool		isLessThanName(ECUname* inName);		// Are we less than that guy?
		byte*		getName(void);								// 64 bit - Pass back the packed up 64 bits that this makes up as our name.
		void		setName(byte* namePtr);					// Make this 64 bits our name.
		void		copyName(ECUname* namePtr);			// Make us a clone of that.
		
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
		byte		getECUInst(void);							// 3 bit - What processor instance are we?
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


class addrNode :	public linkListObj {

	public:
				addrNode(byte inAddr);
	virtual	~addrNode(void);
	
	virtual	bool	isGreaterThan(linkListObj* compObj);	// Are we greater than the obj being passed in?
	virtual	bool	isLessThan(linkListObj* compObj);		// Are we less than the obj being passed in?
	
				byte	addr;
};



class addrList :	public linkList,
						public idler {

public:
				addrList(void);
	virtual	~addrList(void);
	
				addrNode*	findAddr(byte inAddr);
	virtual	void			addAddr(byte inAddr);
};



//				-----    xferList   &  xferNode    -----



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



//				----- ECU Electronic control unit. -----


// Addressing categories for ECU's. Choose one.
enum addrCat {

	nonConfig,			// Address is hard coded.
	serviceConfig,		// You can hook up.. Something. To change our address.
	commandConfig,		// We can respond to address change messages.
	selfConfig,			// We set our own depending on how the network is set up.
	arbitraryConfig,	// We can do the arbitrary addressing dance.
	noAddress			// We have no address, just a listener, or broadcaster.
};


// What in the world is going on in there? The "ourState" variable can give a hint.
enum		ECUState {
	
	config,				// Still on the bench being assembled.
	startWait,			// Certain addresses call for a start wait.
	arbit,				// Doing address arbitration.
	addrErr,				// Had an address error that we can't fix alone.
	running				// Everything seems fine. We're running.
};


class ECU :	public linkList,
				public idler,
				public ECUname {							

	public:
				ECU(void);
	virtual	~ECU(void);
		
	virtual	void		begin(ECUname* inName,byte inAddr,addrCat inAddCat);	// Initial setup.
				void		changeState(ECUState newState);								// Keeping track of what we are up to.
	virtual  void		sendMsg(message* outMsg)=0;									// You have to fill this one out.
	virtual  void		handleMsg(message* inMsg);										// When a message comes in, pass it into here.
				bool		isReqAddrClaim(message* inMsg);								// Is this a request for address claimed msg?
				bool		isAddrClaim(message* inMsg);									// Is this an address claimed msg?
				bool		isCantClaim(message* inMsg);									// Is this a fail to claim address msg?
				bool		isCommandedAddr(message* inMsg);								// Is this a commanded address msg?
				void		handleReqAdderClaim(message* inMsg);						// Handle a request for address claimed msg.
				void		handleAdderClaim(message* inMsg);							// Handle an address claimed msg.
				void		handleCantClaim(message* inMsg);								// Handle a failed to claim an address msg.				
				void		handleComAddr(message* inMsg);								// Handle a commanded address message.
				void		setAddrCat(addrCat inAddrCat);								// How we deal with addressing.
				addrCat	getAddrCat(void);													// See how we deal with addressing.
				byte		getAddr(void);														// Here's our current address.
				void		setAddr(byte inAddr);											// Set a new address.
				void		clearErr(void);													// This will clear the address error and restart the process.
				void		startStartTimer(void);											// Calculate and start the startup time delay. Function of address.
				void		startClaimTimer(void);											// Calculate and start the claim time delay. Random function.
				// arbitraryConfig
				void		sendRequestForAddressClaim(byte inAddr);					// Tell us your name and address.
				void		sendAddressClaimed(bool tryFail=true);						// This is our name and address.
				void		sendCannotClaimAddress(void);									// We can't find an address!
				void		sendCommandedAddress(byte comAddr);							// HEY YOU! Set this as your address!

	virtual	void		idle(void);
				
				ECUState	ourState;
				addrCat	ourAddrCat;
				byte		addr;
				addrList	ourAddrList;
				xferList	ourXferList;
				timeObj	startupTimer;
				timeObj	claimTimer;
				bool		waitForClaim;
};



//				----- CA Controller application -----


// Controller application.
class CA :	public linkListObj {	

	public:
				CA(ECU* inECU);
				~CA(void);
				
				int	getNumBytes(void);				// These only set the value. No buffer here.
            void	setNumBytes(int inNumBytes);	// The message class reads  and uses this.
	virtual  bool	handleMsg(message* inMsg);		// Both of these are stubs.
	virtual  void	sendMsg(void);						// 
				void	setSendInterval(float inMs);	// Used for broadcasting.
            float	getSendInterval(void);			//
	virtual	void	idleTime(void);					// Same as idle, but called by the ECU.
				
				ECU*		ourECU;			// Pointer to our boss!
				uint32_t	ourPGN;			// Typically, this is the type of info we understand.
            int		numBytes;		// How many bytes will the data buffer need?
				timeObj	intervaTimer;	// If broadcasting, how often do we broadcast? (Ms)
};


#endif