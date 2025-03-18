
#include "main.h"
#include "serialComm.h"

char strUartVoltage[UART_MAX_DATA_LEN];
char strUartCurrent[UART_MAX_DATA_LEN];
char strUartPower[UART_MAX_DATA_LEN];
char strUartCharge[UART_MAX_DATA_LEN];
char strUartEnergy[UART_MAX_DATA_LEN];
char strUartTime[UART_MAX_DATA_LEN];

uint8_t serial_rx_byte;
uint32_t nUartRxCallbacks;
uint32_t nUartRxCounterNewline;
//uint32_t nUartTest1, nUartTest2, nUartTest3;


#define UART_LINE_BUFFER_LENGTH 20
uint8_t uart_rxlinebufferIndex;
uint8_t uart_rxlinebuffer[UART_LINE_BUFFER_LENGTH];

void uart_copyRxDataIntoString(char *destination) {
	/* this runs in interrupt context */
	uint8_t i,k;
	k=0;
	for (i=2; i<uart_rxlinebufferIndex; i++) {
	  if (k<UART_MAX_DATA_LEN-2) {
		  destination[k] = uart_rxlinebuffer[i];
		  k++;
	  }
	}
	destination[k] = ' '; k++; /* add space */
	destination[k] = 0;

}

void uart_evaluateReceivedLine(void) {
	/* this runs in interrupt context */
	if (uart_rxlinebufferIndex>=3) { /* we should have at least three characters, e.g. "P=5" */
		if (uart_rxlinebuffer[0]=='U') { uart_copyRxDataIntoString(strUartVoltage); }
		if (uart_rxlinebuffer[0]=='I') { uart_copyRxDataIntoString(strUartCurrent); }
		if (uart_rxlinebuffer[0]=='P') { uart_copyRxDataIntoString(strUartPower); }
		if (uart_rxlinebuffer[0]=='C') { uart_copyRxDataIntoString(strUartCharge); }
		if (uart_rxlinebuffer[0]=='E') { uart_copyRxDataIntoString(strUartEnergy); }
		if (uart_rxlinebuffer[0]=='t') { uart_copyRxDataIntoString(strUartTime); }
	}
}

void serialComm_evaluateReceivedByte(void) {
	/* this runs in interrupt context */
	nUartRxCallbacks++;
	if (serial_rx_byte==0x0A) {
		nUartRxCounterNewline++;
		uart_evaluateReceivedLine();
		uart_rxlinebufferIndex=0;
	} else if (serial_rx_byte>=0x20) { /* it was a printable character, so just put it into the line buffer */
		if (uart_rxlinebufferIndex<UART_LINE_BUFFER_LENGTH) {
			uart_rxlinebuffer[uart_rxlinebufferIndex] = serial_rx_byte;
			uart_rxlinebufferIndex++;
		}
	}
}

