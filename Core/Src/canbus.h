
/* interface for canbus.c */

extern int16_t wheelspeed_FL_kmh;
extern uint16_t acceleratorPedal_prc;
extern int16_t IBatt_0A1;
extern int16_t UBatt_0V1;
extern int32_t PBatt_W;
extern uint8_t blIoniqDetected;


extern uint32_t canRxDataUptime;
extern uint16_t canRxCheckpoint;
extern int16_t EVSEPresentVoltage, uCcsInlet_V;
extern uint8_t temperatureChannel_1_M40;
extern uint8_t temperatureChannel_2_M40;
extern uint8_t temperatureChannel_3_M40;
extern uint8_t temperatureCpu_M40;

extern CAN_RxHeaderTypeDef canRxMsgHdr;
extern uint8_t canRxData[8];

extern void canEvaluateReceivedMessage(void);

