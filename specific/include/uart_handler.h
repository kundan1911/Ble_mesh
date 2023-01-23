#ifndef PICO_UART_HANDLER_H
#define PICO_UART_HANDLER_H

#include<stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

struct protocolData
{
	uint8_t length;
	uint8_t device_id;	
	uint8_t rc_code;
	uint8_t id;
	char data[70];
};

struct stm_interrupt_message {
	uint8_t port;
	uint8_t state;
};

extern xSemaphoreHandle temp;

extern int UARTProDataLength;
extern volatile bool dali_long_command_receiving;

void ExtrateUARTData(uint8_t rcvdCharater,struct protocolData *rcvdUratData,int *insideATCommand,int *runningCount);
void intiatercvdStructureData();
void processUartInterrupt(struct protocolData *rcvdUratData);
int SendAndReceiveUartData(struct protocolData *toBeSendData, struct protocolData *dataRcvd,int timeOut);
void ReceiveInterruptUartData();

void uart_setup();


#endif /* INCLUDE_UART_HANDLER_H_ */