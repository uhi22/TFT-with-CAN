

extern char strUartPower[10];
extern uint8_t serial_rx_byte;
extern uint32_t nUartRxCallbacks;
extern uint32_t nUartRxCounterNewline;
//extern uint32_t nUartTest1, nUartTest2, nUartTest3;

#define UART_MAX_DATA_LEN 10
extern char strUartVoltage[UART_MAX_DATA_LEN];
extern char strUartCurrent[UART_MAX_DATA_LEN];
extern char strUartPower[UART_MAX_DATA_LEN];
extern char strUartCharge[UART_MAX_DATA_LEN];
extern char strUartEnergy[UART_MAX_DATA_LEN];
extern char strUartTime[UART_MAX_DATA_LEN];

extern void serialComm_evaluateReceivedByte(void);
