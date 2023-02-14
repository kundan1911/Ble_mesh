#ifndef PICO_MAIN_H
#define PICO_MAIN_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define ESP_FIRMWARE_CODENAME "Shannon"   // To Be Changed Only After Major Changes, Not For Bug Fixes 
#define ESP_FIRMWARE_BUILD_DATE __DATE__
#define ESP_FIRMWARE_BUILD_TIME __TIME__

extern const char* ESP_FIRMWARE_VERSION;
extern const uint8_t DEVICE_TYPE;
extern char* getName(char* name);

char DEVICE_ID[17];
char DEVICE_MAC_STR[18];
char RUNNING_PARTITION[6];

QueueHandle_t stm_interrupt_message_queue;
QueueHandle_t Uart_message_queue;

#endif
