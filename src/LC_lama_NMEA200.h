#ifndef LC_lama_NMEA200_h
#define LC_lama_NMEA200_h

#include  <idlers.h>
#include  <mapper.h>
#include  <CAN.h>

#define DEF_2515_RST_PIN  8
#define DEF_2515_INT_PIN  2
#define NUM_DATA_BYTES    8

enum msgTypes { 
  noType,
  waterSpeed,
  waterDepth
};


struct msg_t {
  uint32_t  pgn;
  uint8_t   sa;
  uint8_t   ps;
  uint8_t   dp;
  uint8_t   priority;
};


class CANMessage;



// ************ lama_NMEA200 ************


class lama_NMEA200 : public linkList,
                     public idler {
   public:
            lama_NMEA200(int inResetPin=DEF_2515_RST_PIN,int inIntPin=DEF_2515_INT_PIN);
            ~lama_NMEA200(void);

            bool        begin(int inCSPin);
            bool        addMsgObj(msgTypes inType);
            CANMessage* getMsgObj(uint32_t inType);
            CANMessage* getMsgObj(msgTypes inType);
   virtual  void        idle(void);
    
   protected:
            void        readAddress(uint32_t can_id, msg_t * msg);
    
            int   resetPin;
            int   intPin;
            msg_t msg;
};



// ************* CANMessage *************


class CANMessage  : public linkListObj {

   public:
            CANMessage(void);
            ~CANMessage(void);

            msgTypes  getType(void);
            uint32_t  getPGN(void);
            void      readMessage(void);
   virtual  void      decodeMessage(void)=0;
            
            byte     dataBytes[NUM_DATA_BYTES];
            
   protected:
            msgTypes msgType;
            uint32_t msgPGN;
};



// ************* waterSpeedObj *************


class waterSpeedObj  : public CANMessage {

  public:
          waterSpeedObj(void);
          ~waterSpeedObj(void);
          
          float getSpeed(void);
          
  protected:
  virtual void  decodeMessage(void);
             
          mapper  speedMap;
          float   knots;
};



// ************* waterDepthObj *************


class waterDepthObj  : public CANMessage {

  public:
          waterDepthObj(void);
          ~waterDepthObj(void);
          
          float getDepth(void);
          
  protected:
  virtual void  decodeMessage(void);
             
          mapper  depthMap;
          float   feet;
};

#endif
