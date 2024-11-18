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


#define GLOBAL_ADDR		255		// Destination only address. "Hey everyone!"
#define NULL_ADDR			254		// Source address only. "Hey, listen! But, I have no address."
#define REQ_MESSAGE		59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED		60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.

#define DEF_NUM_BYTES	8
#define DEF_PRIORITY		6
#define DEF_R				true
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
				void		setData(int index,byte inByte);
				byte		getData(int index);
				void		showMessage(void);
		
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
#define	DEV_CLASS_ENV_EXT		85		// External Environment
#define	DEV_CLASS__ENV_INT	90		// Internal Environment
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

/*
50	130	Engineroom Monitoring
50	140	Engine
50	141	DC Generator/Alternator
50	150	Engine Controller
50	151	AC Generator
50	155	Motor
50	160	Engine Gateway
50	165	Transmission
50	170	Throttle/Shift Control
50	180	Actuator
50	190	Gauge Interface
50	200	Gauge Large
50	210	Gauge Small
60	130	Bottom Depth
60	135	Bottom Depth/Speed
60	136	Bottom Depth/Speed/Temperature
60	140	Ownship Attitude
60	145	Ownship Position (GNSS)
60	150	Ownship Position (Loran C)
60	155	Speed
60	160	Turn Rate Indicator
60	170	Integrated Navigation
60	175	Integrated Navigation System
60	190	Navigation Management
60	195	Automatic Identification System (AIS)
60	200	Radar
60	201	Infrared Imaging
60	205	ECDIS
60	210	ECS
60	220	Direction Finder
60	230	Voyage Status
70	130	EPIRB
70	140	AIS
70	150	DSC
70	160	Data Receiver/Transceiver
70	170	Satellite
70	180	Radio-telephone (MF/HF)
70	190	Radiotelephone
75	130	Temperature
75	140	Pressure
75	150	Fluid Level
75	160	Flow
75	170	Humidity
80	130	Time/Date Systems
80	140	VDR
80	150	Integrated Instrumentation
80	160	General Purpose Displays
80	170	General Sensor Box
80	180	Weather Instruments
80	190	Transducer/General
80	200	NMEA 0183 Converter
85	130	Atmospheric
85	160	Aquatic
90	130	HVAC
100	130	Scale (Catch)
110	130	Button Interface
110	135	Switch Interface
110	140	Analog Interface
120	130	Display
120	140	Alarm Enunciator
125	130	Multimedia Player
125	140	Multimedia Controller
*/

// Packed eight byte set of goodies.
class ECUname {
	
	public:
					ECUname(void);
	virtual		~ECUname(void);
		
		void		clearName(void);							// Want to zero our name out? This'll do it.
		bool		sameName(ECUname* inName);				// We the same as that guy?
		byte*		getName(void);								// 64 bit - Pass back the packed up 64 bits that this makes up as our name.
		void		setName(byte* namePtr);					// Make this 64 bits our name.
		
		bool		getArbitratyAddrBit(void);				// 1 bit - True, we CAN change our address. 128..247
		void		setArbitratyAddrBit(bool AABit);		// False, we can't change our own address.
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
		
	protected:
		
		byte			name[8];									// The stored 8 byte. 64 bit name.
};



//				----- ECU Electronic control unit. -----


// Addressing categories for ECU's. Choose one.
enum adderCat {

	nonConfig,			// Address is hard coded.
	serviceConfig,		// You can hook up equipment to change our address.
	commandConfig,		// We can respond to address change messages.
	selfConfig,			// We set our own depending on how the network is set up.
	arbitraryConfig,	// We can do the arbitrary addressing dance.
	noAddress			// We have no address, just a listener, or broadcaster.
};


class ECU :	public linkList,
				public idler,
				public ECUname {							

	public:
				ECU(byte inECUInst=0);
	virtual	~ECU(void);
		
	virtual  void		sendMsg(message* outMsg)=0;
	virtual  void		handlePacket(void)=0;
	
				void		setAddrCat(adderCat inAddrCat);		// How we deal with addressing.
							adderCat	getAddrCat(void);				// See how we deal with addressing.
				
				// serviceConfig
				// commandConfig
				// selfConfig
				byte	getAddr(void);
				void	setAddr(byte inAddr);
				byte	getDefAddr(void);
				void	setDefAddr(byte inAddr);
				
				// arbitraryConfig
				void	requestForAddressClaim(byte inAddr);
				void	addressClaimed(void);
				void	cannotClaimAddress(void);
				void	commandedAddress(void);
				
	virtual	void		idle(void);
	
				enum	ECUState { preStart, claiming, working };
				
				adderCat	ourAddrCat;
				byte		defAddr;
				byte		addr;	
};



//				----- CA Controller application -----


// Controller application.
class CA :	public linkListObj {	

	public:
				CA(ECU* inECU);
				~CA(void);
				
				int	getNumBytes(void);				// These only set the value. No buffer here.
            void	setNumBytes(int inNumBytes);	// The message class reads  and uses this.
	virtual  void	handleMsg(message* inMsg);		// Both of these are stubs.
	virtual  void	sendMsg(void);						// 
				void	setSendInterval(float inMs);	// Used for broadcasting.
            float	getSendInterval(void);			//
	virtual	void	idleTime(void);					// Same as idle, but called by the ECU.
				
				ECU*		ourECU;			// Pointer to our boss!
				uint32_t	ourPGN;			// Used as our internal ID.
            int		numBytes;		// How many bytes will the data buffer need?
				timeObj	intervaTimer;	// If broadcasting, how often?
};


#endif