#ifndef LC_llama_NMEA200_h
#define LC_llama_NMEA200_h


#include <idlers.h>
#include <mapper.h>
#include <CAN.h>
#include <SAE_J1939.h>


#define DEF_2515_RST_PIN   8
#define DEF_2515_INT_PIN   2

/*
Basically what are called parameter group numbers. Or PGN(s)

waterSpeed,  // 0x1F503
waterDepth,  // 0x1F50B
waterTemp,   // 0x1FD08
fluidLevel   // 0x1F211
*/


class CANMsgObj;


// ************ llama2000 ************


/*
When you plug into the network, other devices will want to know what/who you are. These
are the setting that tell your network network neighbors this information. What you see
here is the default set put in place as an example. Change to match what you actually are
by doing these calls in your program's setup() function before letting this thing start
running.

setID(0);								// Device ID. We make these up. You get 21 bits.
setManufCode(0);						// This would be assigned to you by NMEA people.
setECUInst(0);							// First ECU (Electronic control unit.)
setFunctInst(0);						// First depth transducer.
setFunction(DEV_FUNC_GP_TRANS);	// Depth transducer.
											// Some spare bit here..
setVehSys(DEV_CLASS_INST);			//	We are an instrument.
setSystemInst(0);						// We are the first of our device class.
setIndGroup(Marine);					// What kind of machine are we ridin' on?
setArbitraryAddrBit(?);				// Will be set when we choose our addressing mode.

*/


class llama2000 :   public ECU {

   public:
            llama2000(byte inECUInst=0,int inResetPin=DEF_2515_RST_PIN,int inIntPin=DEF_2515_INT_PIN);
            ~llama2000(void);

	virtual  bool        begin(int inCSPin);
            bool        addMsgObj(uint32_t inPGN);
            CANMsgObj*  getMsgObj(uint32_t inPGN);
	virtual  void			sendMsg(message* outMsg);
	virtual	void			recieveMsg(void);
    
   protected:
            int			resetPin;
            int			intPin;
};



// ************* CANMsgObj *************


class CANMsgObj : public CA {

   public:
            CANMsgObj(ECU* inECU);
            ~CANMsgObj(void);

            uint32_t getPGN(void);
            
   virtual  void	sendMsg(message* outMsg);
            
            uint32_t	ourPGN;
};



// ************* waterSpeedObj *************


class waterSpeedObj  : public CANMsgObj {

   public:
				waterSpeedObj(ECU* inECU);
				~waterSpeedObj(void);

            float getSpeed(void);
   virtual  bool  handleMsg(message* inMsg);
             
          mapper  speedMap;
          float   knots;
};



// ************* waterDepthObj *************


class waterDepthObj  : public CANMsgObj {

   public:
				waterDepthObj(ECU* inECU);
				~waterDepthObj(void);
          
            float getDepth(void);
	virtual	bool  handleMsg(message* inMsg);
  
  
            float feet;
          
};


// ************* waterTempObj *************


class waterTempObj  : public CANMsgObj {

   public:
            waterTempObj(ECU* inECU);
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


class fluidLevelObj  : public CANMsgObj {

   public:
            fluidLevelObj(ECU* inECU);
            ~fluidLevelObj(void);
          
            tankType getTankType(void);
            void     setTankType(tankType inType);
            float    getLevel(void);
            void     setLevel(float inLevel);
            float    getCapacity(void);
            void     setCapacity(float inCapacity);
   virtual  void     sendMsg(void);
   
            tankType fluidType;
            float    level;
            float    capacity;
            int      tankID;
};


#endif
