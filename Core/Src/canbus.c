
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
uint8_t blIoniqDetected=1;
uint8_t socDisp_0p5;
uint8_t TBattMin_C, TBattMax_C;
int32_t PIntegral_Wh;
int32_t IIntegral_0Ah01;
int32_t IIntegral_hiRes, PIntegral_hiRes;
uint8_t timeoutcounter_595;

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
#define MESSAGE_ID_542 0x542 /* BMS SOC */
#define MESSAGE_ID_595 0x595 /* BMS */
#define MESSAGE_ID_596 0x596 /* BMS temperatures */
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
    if (canRxMsgHdr.StdId == MESSAGE_ID_542) {
        /* battery SOC */
        socDisp_0p5 = canRxData[0];
        return;
    }
    if (canRxMsgHdr.StdId == MESSAGE_ID_595) {
        #define PINTEGRAL_HIRES_SCALER (10*(int32_t)3600) /* Scale to Wh */
        /* battery voltage and current */ 
        IBatt_0A1 = canRxData[5];
        IBatt_0A1<<=8;
        IBatt_0A1 |= canRxData[4];
        UBatt_0V1 = canRxData[7];
        UBatt_0V1<<=8;
        UBatt_0V1 |= canRxData[6];
        PBatt_W = ((int32_t)UBatt_0V1 * (int32_t)IBatt_0A1)/100;
        timeoutcounter_595=15; /* 15*100ms = 1.5s */
        /* the energy */
        PIntegral_hiRes += PBatt_W; /* integrate the power. Resolution 1W per 100ms */
        while (PIntegral_hiRes > PINTEGRAL_HIRES_SCALER) {
          PIntegral_hiRes -= PINTEGRAL_HIRES_SCALER;
          PIntegral_Wh++;
        }
        while (PIntegral_hiRes < -PINTEGRAL_HIRES_SCALER) {
          PIntegral_hiRes += PINTEGRAL_HIRES_SCALER;
          PIntegral_Wh--;
        }
        /* Integrate the current to get the charge capacity */
        #define IINTEGRAL_HIRES_SCALER ((int32_t)3600) /* Scale to 0.01Ah */
        IIntegral_hiRes += IBatt_0A1; /* resolution 0.1A per 100ms, or 0.01As */
        while (IIntegral_hiRes > IINTEGRAL_HIRES_SCALER) {
        	IIntegral_hiRes -= IINTEGRAL_HIRES_SCALER;
        	IIntegral_0Ah01++;
        }
        while (IIntegral_hiRes < -IINTEGRAL_HIRES_SCALER) {
        	IIntegral_hiRes += IINTEGRAL_HIRES_SCALER;
        	IIntegral_0Ah01--;
        }

        return;
    }
    if (canRxMsgHdr.StdId == MESSAGE_ID_596) {
        /* battery temperatures */
        TBattMin_C = canRxData[6]; /* battery min temperature in 1°C */
        TBattMax_C = canRxData[7]; /* battery max temperature in 1°C */
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
}
