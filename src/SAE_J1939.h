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


#define GLOBAL_ADDR	255		// Destination only address. "Hey everyone!"
#define NULL_ADDR		254		// Source address only. "Hey, listen! But, I have no address."
#define REQ_MESSAGE	59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED	60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.

#define DEF_NUM_DATA_BYTES 8



//				----- ECU name -----


enum indGroup {

	Global,
	Highway,
	Agriculture,
	Construction,
	Marine,
	Industrial
};



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



class ECU :	public linkList,
				public idler,
				public ECUname {							

	public:
				ECU(byte inECUInst=0);
	virtual	~ECU(void);
		
				void     readHeader(uint32_t CANID, msgHeader* inHeader);
				uint32_t	makeHeader(uint32_t PGN, uint8_t priority);
	virtual  void		sendMessage(uint32_t PGN,byte priority,int numBytes,byte* data)=0;
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
				void	requestForAddressClaim(void);
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
				
				int	getNumBytes(void);				// These get and set the size of the
            void	setNumBytes(int inNumBytes);	// Data buffer. Default is 8.
				
	virtual  void	sendMessage(void)=0;			
				void	setSendInterval(float inMs);
            float	getSendInterval(void);
	virtual	void	idleTime(void);						// Same as idle, but called by the ECU.
				
				ECU*		ourECU;
				uint32_t	ourPGN;
            int		numBytes;
            byte*		dataBytes;
				timeObj	intervaTimer;
};


#endif