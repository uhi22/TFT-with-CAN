
#include "main.h"
#include "canbus.h"

/* The Hyundai Ioniq messages are taken from 
- (outdated) https://github.com/uhi22/IoniqMotorCAN/blob/master/Traces/hyundai_Ioniq28Motor.dbc
- newer: https://github.com/uhi22/Ioniq28Investigations/blob/main/CAN/hyundai_Ioniq28Motor.dbc
*/

int16_t wheelspeed_FL_kmh;
uint16_t acceleratorPedal_prc;
int16_t IBatt_0A1;
int16_t UBatt_0V1;
int32_t PBatt_W;
uint8_t blIoniqDetected=1;
uint8_t socDisp_0p5;
uint8_t TBattMin_C, TBattMax_C;
int32_t PIntegral_Wh;
int32_t IIntegral_0Ah01;
int32_t IIntegral_hiRes, PIntegral_hiRes;
uint8_t timeoutcounter_595;

/* BAT11 Battery Sensor of the 12V battery */
float   BAT11_BAT_SNSR_I;
uint8_t BAT11_BAT_SOC;
float   BAT11_BAT_SNSR_V;
float   BAT11_BAT_SNSR_Temp;
uint8_t BAT11_BAT_SOH;
uint8_t rawmessage678[8];

/* experimental data of ccs32clara, https://github.com/uhi22/ccs32clara */
uint32_t canRxDataUptime;
uint16_t canRxCheckpoint;
int16_t EVSEPresentVoltage;
uint8_t temperatureChannel_1_M40;
uint8_t temperatureChannel_2_M40;
uint8_t temperatureChannel_3_M40;
uint8_t temperatureCpu_M40;
int16_t canDebugValue1, canDebugValue2, canDebugValue3, canDebugValue4;
float uCcsInlet_V;


#define MESSAGE_ID_WHLSPD11 0x386 /* Hyundai Ioniq WHL_SPD11, see hyundai_Ioniq28Motor.dbc */
#define MESSAGE_ID_542 0x542 /* BMS SOC */
#define MESSAGE_ID_595 0x595 /* BMS */
#define MESSAGE_ID_596 0x596 /* BMS temperatures */
#define MESSAGE_ID_EMS20 0x200 /* Throttle */
#define MESSAGE_ID_BAT11 1353

#define MESSAGE_ID_EVSE_STATUS_678 0x678 /* status message of the Demo-EVSE */

CAN_RxHeaderTypeDef canRxMsgHdr;
uint8_t canRxData[8];

void canEvaluateReceivedMessage(void) {
	uint32_t tmp32;
    /* This is called in interrupt context. Keep it as short as possible. */
    if (canRxMsgHdr.StdId == MESSAGE_ID_EVSE_STATUS_678) {
    	uint8_t i;
		for (i=0; i<8; i++) rawmessage678[i] = canRxData[i];
        return;
    }
    
}
