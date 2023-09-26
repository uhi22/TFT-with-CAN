
#include "main.h"
#include "canbus.h"

/* The Hyundai Ioniq messages are taken from 
https://github.com/uhi22/IoniqMotorCAN/blob/master/Traces/hyundai_Ioniq28Motor.dbc
*/

int16_t wheelspeed_FL_kmh;
uint16_t acceleratorPedal_prc;
int16_t IBatt_0A1;
int16_t UBatt_0V1;
int32_t PBatt_W;
uint8_t blIoniqDetected=0;

/* experimental data of ccs32clara, https://github.com/uhi22/ccs32clara */
uint32_t canRxDataUptime;
uint16_t canRxCheckpoint;
int16_t EVSEPresentVoltage, uCcsInlet_V;
uint8_t temperatureChannel_1_M40;
uint8_t temperatureChannel_2_M40;
uint8_t temperatureChannel_3_M40;
uint8_t temperatureCpu_M40;
int16_t canDebugValue1, canDebugValue2, canDebugValue3, canDebugValue4;


#define MESSAGE_ID_WHLSPD11 0x386 /* Hyundai Ioniq WHL_SPD11, see hyundai_Ioniq28Motor.dbc */
#define MESSAGE_ID_595 0x595 /* BMS */
#define MESSAGE_ID_EMS20 0x200 /* Throttle */

CAN_RxHeaderTypeDef canRxMsgHdr;
uint8_t canRxData[8];

void canEvaluateReceivedMessage(void) {
	uint32_t tmp32;
    /* This is called in interrupt context. Keep it as short as possible. */
    if (canRxMsgHdr.StdId == MESSAGE_ID_WHLSPD11) {
        /* wheel speed front left, in 0.03125 km/h */
    	tmp32 = canRxData[1] & 0x3F;
    	tmp32<<=8;
    	tmp32 += canRxData[0];
    	tmp32 *= 3125;
    	tmp32 /= 100000uL;
		wheelspeed_FL_kmh = tmp32;
        return;
    }
    if (canRxMsgHdr.StdId == MESSAGE_ID_595) {
        /* battery voltage and current */ 
        IBatt_0A1 = canRxData[5];
        IBatt_0A1<<=8;
        IBatt_0A1 |= canRxData[4];
        UBatt_0V1 = canRxData[7];
        UBatt_0V1<<=8;
        UBatt_0V1 |= canRxData[6];
        PBatt_W = ((int32_t)UBatt_0V1 * (int32_t)IBatt_0A1)/100;
        return;
    }
    
    if (canRxMsgHdr.StdId == MESSAGE_ID_EMS20) {
        /* accelerator pedal */
        acceleratorPedal_prc = canRxData[4]; /* 0 to 0xFE */
        acceleratorPedal_prc *= 100;
        acceleratorPedal_prc /= 0xFE; /* now it is scaled 0 to 100% */
        blIoniqDetected = 1;
        return;
    }
    
    if (canRxMsgHdr.StdId == 0x567) {
    	canRxDataUptime = canRxData[0];
    	canRxDataUptime <<=8;
    	canRxDataUptime |= canRxData[1];
    	canRxDataUptime <<=8;
    	canRxDataUptime |= canRxData[2];
    	canRxCheckpoint = canRxData[3];
    	canRxCheckpoint <<=8;
    	canRxCheckpoint |= canRxData[4];
        return;
    }
    if (canRxMsgHdr.StdId == 0x568) {
    	EVSEPresentVoltage = canRxData[0];
    	EVSEPresentVoltage <<=8;
    	EVSEPresentVoltage |= canRxData[1];
    	uCcsInlet_V = canRxData[2];
    	uCcsInlet_V <<=8;
    	uCcsInlet_V |= canRxData[3];
        return;
    }
    if (canRxMsgHdr.StdId == 0x569) {
    	temperatureCpu_M40 = canRxData[0];
    	temperatureChannel_1_M40 = canRxData[1];
    	temperatureChannel_2_M40 = canRxData[2];
    	temperatureChannel_3_M40 = canRxData[3];
        return;
    }
    if (canRxMsgHdr.StdId == 0x56A) {
    	canDebugValue1 = canRxData[0];
    	canDebugValue1 <<=8;
    	canDebugValue1 |= canRxData[1];
    	canDebugValue2 = canRxData[2];
    	canDebugValue2 <<=8;
    	canDebugValue2 |= canRxData[3];
    	canDebugValue3 = canRxData[4];
    	canDebugValue3 <<=8;
    	canDebugValue3 |= canRxData[5];
    	canDebugValue4 = canRxData[6];
    	canDebugValue4 <<=8;
    	canDebugValue4 |= canRxData[7];
        return;
    }

}
