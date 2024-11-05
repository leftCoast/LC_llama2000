#ifndef LC_llama_NMEA200_h
#define LC_llama_NMEA200_h


#include <idlers.h>
#include <mapper.h>
#include <timeObj.h>
#include <CAN.h>
#include <SAE_J1939.h>


#define DEF_2515_RST_PIN   8
#define DEF_2515_INT_PIN   2
#define DEF_NUM_DATA_BYTES 8


// Basically what are called parameter group numbers. Or PGN(s)
enum msgTypes {
  noType,
  waterSpeed,  // 0x1F503
  waterDepth,  // 0x1F50B
  waterTemp,   // 0x1FD08
  fluidLevel   // 0x1F211
};


enum tankType {
   fuel,
   water,
   grayWater,
   liveWell,
   oil,
   blackWater
};


// Decoding the 39 bit CAN header.
struct msg_t {
  uint32_t  pgn;			// Type of data (Parameter group number)
  uint8_t   sa;			// Source address. -NMEA 2000 address-
  uint8_t   ps;			// Part of PGN
  uint8_t   dp;			// Part of PGN
  uint8_t   priority;	// CAN priority bits.
};


class CANMsgObj;


// ************ llama_NMEA2000 ************


class llama_NMEA2000 :   public ECU {
   public:
            llama_NMEA2000(byte inECUInst=0,int inResetPin=DEF_2515_RST_PIN,int inIntPin=DEF_2515_INT_PIN);
            ~llama_NMEA2000(void);

            bool        begin(int inCSPin);
            bool        addMsgObj(msgTypes inType,int inInstance=0);
            CANMsgObj*  getMsgObj(uint32_t inPGN,int inInstance=0);
            CANMsgObj*  getMsgObj(msgTypes inType,int inInstance=0);
   virtual  void        idle(void); // Watches for new data.
    
   protected:
            void        readAddress(uint32_t can_id, msg_t * msg);
    
            int   resetPin;
            int   intPin;
            msg_t msg;
};



// ************* CANMsgObj *************


class CANMsgObj : public CA {

   public:
            CANMsgObj(ECU* inECU);
            ~CANMsgObj(void);

            msgTypes getType(void);
            uint32_t getPGN(void);
            
            int      getNumBytes(void);
            void		setNumBytes(int inNumBytes);
   virtual  void     decodeMessage(void)=0;
            void     showDataBytes(void);
            void     setSendInterval(float inMs);
            float    getSendInterval(void);
   virtual  void     sendMessage(void)=0;
   virtual	void		idleTime(void);		// Same as idle, but called by the ECU.
            
            byte*    dataBytes;
            int      numBytes;
            msgTypes msgType;
            uint32_t msgPGN;
            //byte     instance;
            timeObj  intervaTimer;
};



// ************* waterSpeedObj *************


class waterSpeedObj  : public CANMsgObj {

   public:
          waterSpeedObj(ECU* inECU);
          ~waterSpeedObj(void);
          
            float getSpeed(void);
   virtual  void  sendMessage(void);
   
   protected:
   virtual  void  decodeMessage(void);
             
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
  virtual   void  decodeMessage(void);
  
  
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
   virtual  void  decodeMessage(void);
   
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
   virtual  void     decodeMessage(void);
   
            tankType fluidType;
            float    level;
            float    capacity;
            int      tankID;
};


#endif
