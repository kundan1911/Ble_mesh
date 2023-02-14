#ifndef PICO_UART_HANDLER_H
#define PICO_UART_HANDLER_H

#include<stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


extern xSemaphoreHandle temp;

extern int UARTProDataLength;
// extern volatile bool dali_long_command_receiving;

void protocolUARTSend();
void protocolUARTSend2();
void protocolUARTSend3(uint8_t,u_int8_t);
void uart_setup();


#endif /* INCLUDE_UART_HANDLER_H_ */