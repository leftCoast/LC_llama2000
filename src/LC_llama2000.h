#ifndef LC_llama_NMEA200_h
#define LC_llama_NMEA200_h


#include <idlers.h>
#include <mapper.h>
#include <runningAvg.h>
#include <CAN.h>
#include <SAE_J1939.h>


#define DEF_2515_RST_PIN   8
#define DEF_2515_INT_PIN   2


//#define SECOND_LLAMA	// Comment out for first llama, uncomment for second llama.

#ifdef SECOND_LLAMA

// Second llama. The "other device" for testing.
#define DEVICE_ID	1706					// 1706 Mike. 
#define MANF_CODE	73						// PT 73
#define DEF_ADDR	44						// Also easy to spot.
#define ADDR_CAT	commandConfig		// Can be told where to go.
#define OTHER_ADDR	45					// The other guy's address. (For testing with two)

#else

// First llama.
#define DEVICE_ID	6387					// 6387 Foxtrot. 
#define MANF_CODE	35						// J/35
#define DEF_ADDR	45						// Easy to spot.
#define ADDR_CAT	arbitraryConfig	// Can do the address dance.
#define OTHER_ADDR	44					// The other guy's address. (For testing with two)

#endif

// ************ llama2000 ************


/*
When you plug into the network, other devices will want to know what/who you are. These
are the setting that tell your network network neighbors this information. What you see
here is the default set put in place as an example. Change to match what you actually are
by doing these calls in your program's setup() function before letting this thing start
running.

setID(0);								// Device ID. We make these up. You get 21 bits.
setManufCode(0);						// This would be assigned to you by NMEA people.
setECUInst(0);							// First netObj (Electronic control unit.)
setFunctInst(0);						// First depth transducer.
setFunction(DEV_FUNC_GP_TRANS);	// Depth transducer.
											// Some spare bit here..
setVehSys(DEV_CLASS_INST);			//	We are an instrument.
setSystemInst(0);						// We are the first of our device class.
setIndGroup(Marine);					// What kind of machine are we ridin' on?
setArbitraryAddrBit(?);				// Will be set when we choose our addressing mode.

*/


class llama2000 :   public netObj {

   public:
            llama2000(byte inECUInst=0,int inResetPin=DEF_2515_RST_PIN,int inIntPin=DEF_2515_INT_PIN);
            ~llama2000(void);

	virtual  bool        begin(int inCSPin);
	virtual  void			sendMsg(message* outMsg);
	virtual	void			recieveMsg(void);
   virtual	void			idle(void);
   protected:
            int			resetPin;
            int			intPin;
};


#endif
