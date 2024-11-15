#ifndef SAE_J1939_h
#define SAE_J1939_h

#include <lists.h>
#include <idlers.h>
#include <timeObj.h>
#include <resizeBuff.h>
#include <CAN.h>				// Need to objectify this to a base class. Then any chip should work.

// CAN Header. This is all built on CAN Bus. Each message has a 39 bit header. Followed by 0..8 bytes data.
// PGN - Parameter Group Number. Basically what is this message about.


#define GLOBAL_ADDR	255		// Destination only address. "Hey everyone!"
#define NULL_ADDR		254		// Source address only. "Hey, listen! But, I have no address."
#define REQ_MESSAGE	59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED	60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.

#define DEF_NUM_DATA_BYTES 8

//				----- ECU Electronic control unit. -----

/*
// Decoding the 29 bit CAN header.
struct msgHeader {
  uint32_t  PGN;			// Type of data (Parameter group number)
  uint8_t   sourceAddr;	// Source address. -NMEA 2000 address-
  uint8_t   ps;			// Part of PGN
  uint8_t   dp;			// Part of PGN
  uint8_t   priority;	// CAN priority bits.
};
*/


// Decoding the 29 bit CAN header.
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
				public idler {							

	public:
				ECU(byte inECUInst=0);
	virtual	~ECU(void);
		
				byte		getECUInst(void);
				void		setECUInst(byte inInst);
				void     readHeader(uint32_t CANID, msgHeader* inHeader);
				uint32_t	makeHeader(uint32_t PGN, uint8_t priority, uint8_t sourceAddr);
	virtual  void		sendMessage(uint32_t PGN,byte priority,byte address,int numBytes,byte* data)=0;
	virtual  void			handlePacket(void)=0;
	virtual	void		idle(void);	
				
	protected:
		byte	ECUInst;
};



//				----- 29 bit CAN header pack & unpack -----

class extCANHeader {

	public:
					extCANHeader(void);
	virtual		~extCANHeader(void);
	
	void		setHeader(uint32_t inHeader);
	uint32_t	getHeader(void);
	void		setPriority(byte inPriority);
	byte		getPriority(void);
	void		setDataPage(bool inDataPage);
	bool		getDataPage(void);
	void		setPUDFormat(byte inFormat);
	byte		getPUDFormat(void);
	void		setPUDSpecific(byte inSpecific);
	byte		getPUDSpecific(void);
	void		setSRRBit(bool inSRR);
	bool		getSRRBit(void);
	void		setExtBit(bool inExt);
	bool		getExtBit(void);
	void		setSourceAddr(byte inSourceAddr);
	byte		getSourceAddr(void);
	void		setDataLen(byte inDataLen);
	byte		getDataLen(void);
	
	uint32_t	header;
	byte		priority;
	bool		dataPage;
	byte		PDUFormat;
	byte		PDUSpecific;
	bool		subRemoteReq;
	bool		IDExtBit;
	byte		sourceAddr;
	byte		dataLen;
};



//				----- Controller name -----


// Packed eight byte set of goodies.
class CAName {
	
	public:
					CAName(void);
	virtual		~CAName(void);
		
		void		clearName(void);							// Want to zero our name out? This'll do it.
		
		uint32_t	getPGN(void);								// This is one of the importante bits to decode.
		void		setPGN(uint32_t PGN);					// And 
		
		bool		getArbitratyAddrBit(void);				// 1 bit - True, we CAN change our address. 128..247
		void		setArbitratyAddrBit(bool AABit);		// False, we can't change our own address.
		byte		getIndGroup(void);						// 3 bit - Assigned by committee. Tractor, car, boat..
		void		setIndGroup(byte indGroup);
		byte		getSystemInst(void);						// 4 bit - System instance, like engine1 or engine2.
		void		setSystemInst(byte sysInst);
		byte		getVehSys(void);							// 7 bit - Assigned by committee. 
		void		setVehSys(byte vehSys);					//
																	// One bit reserved field. Set to zero. 
		byte		getFunction(void);						// 8 bit - Assigned by committee. 0..127 absolute definition.
		void		setFunction(byte funct);				// 128+ need other fields for definition.
		byte		getFunctInst(void);						// 5 bit - Instance of this function. (Fuel level?)
		void		setFunctInst(byte functInst);			//
		byte		getECUInst(void);							// 3 bit - What processor instance are we?
		void		setECUInst(byte inst);					// 
		uint16_t	getManufCode(void);						// 11 bit - Assigned by committee. Who made this thing?
		void		setManufCode(uint16_t manufCode);	//
		uint32_t	getID(void);								// 21 bit - Unique Fixed value. Product ID & Serial number kinda' thing.
		void		setID(uint32_t inID);
		byte*		getName(void);								// 64 bit - Pass back the packed up 64 bits that this makes up as our name.
		void		setName(byte* namePtr);					// If we want to decode one?
		
	protected:
		
		byte			name[8];
};



//				----- CA Controller application -----


// Addressing categories for CA's. Choose one.
enum adderCat {

	nonConfig,			// Address is hard coded.
	serviceConfig,		// You can hook up equipment to change our address.
	commandConfig,		// We can respond to address change messages.
	selfConfig,			// We set our own depending on how the network is set up.
	arbitraryConfig,	// We can do the arbitrary addressing dance.
	noAddress			// We have no address, just a listener, or broadcaster.
};


// Controller application.
class CA :	public linkListObj,
				public CAName {	

	public:
				CA(ECU* inECU);
				~CA(void);
				
				void		setAddrCat(adderCat inAddrCat);	// How we deal with addressing.
				adderCat	getAddrCat(void);						// See how we deal with addressing.
				int      getNumBytes(void);					// These get and set the size of the
            void		setNumBytes(int inNumBytes);		// Data buffer. Default is 8.
            
				//	nonConfig
				void	setNonConfigAddr(byte inAddr);
				
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
				
	virtual  void	sendMessage(void)=0;			
				void	setSendInterval(float inMs);
            float	getSendInterval(void);
	virtual	void	idleTime(void);					// Same as idle, but called by the ECU.
				
				
				enum	CAState { preStart, claiming, working };
				
				ECU*			ourECU;
				uint32_t		ourPGN;
				adderCat		ourAddrCat;
				byte			defAddress;
				byte			address;
            int			numBytes;
            byte*			dataBytes;
				CAState		ourState;
				timeObj		intervaTimer;
};


#endif