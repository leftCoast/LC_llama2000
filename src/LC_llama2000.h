#ifndef LC_llama_NMEA200_h
#define LC_llama_NMEA200_h


#include <idlers.h>
#include <mapper.h>
#include <runningAvg.h>
#include <CAN.h>
#include <SAE_J1939.h>


#define DEF_2515_RST_PIN   8
#define DEF_2515_INT_PIN   2


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



// ************* msgHandler *************
//   Below are example message handlers.
// *************************************



// ************* waterSpeedObj *************


class waterSpeedObj  : public msgHandler {

   public:
				waterSpeedObj(netObj* inNetObj);
				~waterSpeedObj(void);

            float getSpeed(void);
   virtual  bool  handleMsg(message* inMsg);
             
          mapper  speedMap;
          float   knots;
};



// ************* waterDepthObj *************


class waterDepthObj  : public msgHandler {

   public:
				waterDepthObj(netObj* inNetObj);
				~waterDepthObj(void);
          
            float getDepth(void);
	virtual	bool  handleMsg(message* inMsg);
  
            float feet;
          
};


// ************* waterTempObj *************


class waterTempObj  : public msgHandler {

   public:
            waterTempObj(netObj* inNetObj);
            ~waterTempObj(void);
          
            float getTemp(void);
   virtual  bool  handleMsg(message* inMsg);
   
            float   degF;
};



// ************* fluidLevelObj *************

enum tankType {
   fuel,
   water,
   grayWater,
   liveWell,
   oil,
   blackWater
};


class fluidLevelObj  : public msgHandler {

   public:
            fluidLevelObj(netObj* inNetObj);
            ~fluidLevelObj(void);
          
            tankType getTankType(void);
            void     setTankType(tankType inType);
            float    getLevel(void);
            void     setLevel(float inLevel);
            float    getCapacity(void);
            void     setCapacity(float inCapacity);
   virtual  bool  	handleMsg(message* inMsg);
   virtual  void     newMsg(void);
   
            tankType fluidType;
            float    level;
            float    capacity;
            int      tankID;
};



// ************* airTempBarometer *************

//runningAvg inHgSmooth(6);

class airTempBarometer  : public msgHandler {

   public:
            airTempBarometer(netObj* inNetObj);
            ~airTempBarometer(void);
          
   virtual  bool  handleMsg(message* inMsg);
   			float getAirTemp(void);
   			float getInHg(void);
   
   			float   		degF;
   			runningAvg*	inHgSmooth;
            float   		inHg;
};


#endif
