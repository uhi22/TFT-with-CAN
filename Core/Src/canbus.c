
#include "main.h"
#include "canbus.h"

uint8_t rawmessage678[8];
uint8_t rawmessage679[8];
uint8_t rawmessage67A[8];

/* experimental data of ccs32clara, https://github.com/uhi22/ccs32clara */

#define MESSAGE_ID_EVSE_STATUS_678 0x678 /* status message of the Demo-EVSE */
#define MESSAGE_ID_EVSE_STATUS_679 0x679 /* status message of the Demo-EVSE */
#define MESSAGE_ID_EVSE_STATUS_67A 0x67A /* status message of the Demo-EVSE */

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
    if (canRxMsgHdr.StdId == MESSAGE_ID_EVSE_STATUS_679) {
    	uint8_t i;
		for (i=0; i<8; i++) rawmessage679[i] = canRxData[i];
        return;
    }
    if (canRxMsgHdr.StdId == MESSAGE_ID_EVSE_STATUS_67A) {
    	uint8_t i;
		for (i=0; i<8; i++) rawmessage67A[i] = canRxData[i];
        return;
    }
    
}
