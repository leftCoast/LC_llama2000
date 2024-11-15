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


enum tankType {
   fuel,
   water,
   grayWater,
   liveWell,
   oil,
   blackWater
};


class CANMsgObj;


// ************ llama_NMEA2000 ************


class llama_NMEA2000 :   public ECU {
   public:
            llama_NMEA2000(byte inECUInst=0,int inResetPin=DEF_2515_RST_PIN,int inIntPin=DEF_2515_INT_PIN);
            ~llama_NMEA2000(void);

            bool        begin(int inCSPin);
            bool        addMsgObj(uint32_t inPGN,int inInstance=0);
            CANMsgObj*  getMsgObj(uint32_t inPGN,int inInstance=0);
	virtual  void			sendMessage(uint32_t PGN,byte priority,byte address,int numBytes,byte* data);
   virtual  void			handlePacket(void);
    
   protected:
    
            int			resetPin;
            int			intPin;
            msgHeader	header;
};



// ************* CANMsgObj *************


class CANMsgObj : public CA {

   public:
            CANMsgObj(ECU* inECU);
            ~CANMsgObj(void);

            uint32_t getPGN(void);
            
   virtual  void     handleMsg(void)=0;
            void     showDataBytes(void);
   virtual  void     sendMessage(void);
            
            uint32_t	ourPGN;
};



// ************* waterSpeedObj *************


class waterSpeedObj  : public CANMsgObj {

   public:
          waterSpeedObj(ECU* inECU);
          ~waterSpeedObj(void);
          
            float getSpeed(void);
   virtual  void  sendMessage(void);
   
   protected:
   virtual  void  handleMsg(void);
             
          mapper  speedMap;
          float   knots;
};



// ************* waterDepthObj *************


class waterDepthObj  : public CANMsgObj {

   public:
          waterDepthObj(ECU* inECU);
          ~waterDepthObj(void);
          
            float getDepth(void);
   virtual  void  sendMessage(void);
   
   protected:
  virtual   void  handleMsg(void);
  
  
            float feet;
          
};


// ************* waterTempObj *************


class waterTempObj  : public CANMsgObj {

   public:
            waterTempObj(ECU* inECU);
            ~waterTempObj(void);
          
            float getTemp(void);
   virtual  void  sendMessage(void);
   
   protected:
   virtual  void  handleMsg(void);
   
            float   degF;
};



// ************* fluidLevelObj *************


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
   virtual  void     sendMessage(void);
          
   protected:
   virtual  void     handleMsg(void);
   
            tankType fluidType;
            float    level;
            float    capacity;
            int      tankID;
};


#endif
