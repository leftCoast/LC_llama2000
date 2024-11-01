#ifndef SAE_J1939_h
#define SAE_J1939_h

#include <lists.h>
#include <idlers.h>

#define GLOBAL_ADDR	255		// Destination only address. "Hey everyone!"
#define NULL_ADDR		254		// Source address only. "Hey, listen! But, I have no address."
#define REQ_MESSAGE	59904		// Request PGN. "Hey EVERYONE send out your name and address."
#define ADDR_CLAIMED	60928		// Claimed PGN. "Hey EVERYONE this is my name and address." Or can't claim one.

// Electronic control unit.
class ECU :	public linkList,
				public idler {							

	public:
				ECU(byte inECUInst=0);
	virtual	~ECU(void);
		
				byte	getECUInst(void);
	virtual	void idle(void);	
	
	protected:
		byte	ECUInst;
};



// Addressing categories for CA's. Choose one.
enum adderCat {

	nonConfig,			// Address is hard coded.
	serviceConfig,		// You can hook up equipment to change our address.
	commandConfig,		// We can respond to address change messages.
	selfConfig,			// We set our own depending on how the network is set up.
	arbitraryConfig,	// We can do the arbitrary addressing dance.
	noAddress			// We have no address, just a listener, or broadcaster.
};



// Packed eight byte set of goodies.
class CAName {
	
	public:
					CAName(void);
	virtual		~CAName(void);
		
		bool		getArbitratyAddrBit(void);			// 1 bit - True, we CAN change our address. 128..247
		void		setArbitratyAddrBit(bool AABit);	// False, we can't change our own address.
		byte		getIndGroup(void);					// 3 bit - Assigned by committee. Tractor, car, boat..
		void		setIndGroup(byte indGroup);
		byte		getSystemInst(void);					// 4 bit - System instance, like engine1 or engine2.
		void		setSystemInst(byte sysInst);
		byte		getVehSys(void);						// 7 bit - Assigned by committee. 
		void		setVehSys(byte vehSys);				//
																// One bit reserved field. Set to zero. 
		byte		getFunction(void);					// 8 bit - Assigned by committee. 0..127 absolute definition.
		void		setFunction(byte funct);			// 128+ need other fields for definition.
		byte		getFunctInst(void);					// 5 bit - Instance of this function. (Fuel level?)
		void		getFunctInst(byte functInst);		//
		byte		getECUInst(void);						// 3 bit - What processor instance are we?
		void		setECUInst(byte inst);				// 
		uint16_t	getManufCode(void);					// 11 bit - Assigned by committee. Who made this thing?
		void		setManufCode(int16_t manfCode);	//
		uint32_t	getID(void);							// 21 bit - Unique Fixed value. Product ID & Serial number kinda' thing.
		void		setID(uint32_t inID);
		byte*		getName(void);							// 64 bit - Pass back the packed up 64 bits that this makes up as our name.
		void		setName(byte* namePtr);				// If we want to decode one?
		
	
	protected:
		
		byte			name[8];
};



// Controller application.
class CA :	public linkListObj,
				public CAName {	

	public:
				CA(ECU* inECU,CACat inAddrCat,byte inDefAddress);
				~CA(void);
				
				void		getAddrCat(void);						// How we deal with addressing.
				adderCat	setAddrCat(adderCat inAddrCat);	//
				
				//	nonConfig
				void	setNonConfigAddr(void);
				
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
				
	protected:
				enum	CAState { inital, claiming, working };
				
	virtual	void idleTime(void);		// Same as idle, but called by the ECU.
		
				ECU*			ourECU;
				CAAdderCat	ourAddrCat;
				byte			defAddress;
				byte			address;
				CAState		ourState;
};


#endif