#include<string.h>
#include<inttypes.h>

#include "uart_handler.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"


#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
#define UART_UC_IF UART_NUM_1
#define UART_UC_RX 16
#define UART_UC_TX 17
#define delayTime 200
static QueueHandle_t uart1_queue;

static const char *TAG = "uart_events";


char starting_delimiter = '#';
char ending_delimiter = '-';
int UARTProDataLength = 70;
xSemaphoreHandle temp;
volatile bool completedReceiving = false;
uint8_t byteVal[2];

uint8_t savAndDochcksm(uint8_t brigtness,uint16_t datacolor){
	byteVal[1] = datacolor & 0x00FF; // low byte (0x34)
byteVal[0] = datacolor >> 8;     // high byte (0x12)

uint8_t chksum=byteVal[0]+byteVal[1]+brigtness;
chksum=~chksum;
return chksum;
}
void uartWrite(uint8_t chksum,uint8_t brigtness){
uint8_t starting_delimiter = byteVal[1];
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	 starting_delimiter = byteVal[0];
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	 starting_delimiter = brigtness;
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	 starting_delimiter = 0x00;
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	 starting_delimiter =chksum ;
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
}
void protocolUARTSend() {
	uint8_t starting_delimiter = 0x0D;
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	printf("Data Sent\n");
}
void protocolUARTSend2(uint8_t delay) {
	uint8_t starting_delimiter = delay;
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	// starting_delimiter = 0x1C;
	// uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	//  starting_delimiter = 0x0C;
	// uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	//  starting_delimiter = 0x64;
	// uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	//  starting_delimiter = 0x00;
	// uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	//  starting_delimiter = 0x4B;
	// uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	byteVal[1]=0x1C;
	byteVal[0]=0x0C;
uint8_t chksum=byteVal[0]+byteVal[1]+0x64+delay;
chksum=~chksum;
	uartWrite(chksum,0x64);
	printf("light transaction 2 Sended\n");
}
void protocolUARTSend3(uint8_t colortemp,uint8_t brigtness) {
	uint16_t datacolor=3000 + colortemp;
	uint8_t cheksum=savAndDochcksm(brigtness,datacolor);

// uint8_t starting_delimiter = byteVal[1];
// 	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
// 	 starting_delimiter = byteVal[0];
// 	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
// 	 starting_delimiter = brigtness;
// 	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
// 	 starting_delimiter = 0x00;
// 	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
// 	 starting_delimiter =chksum ;
// 	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
uartWrite(cheksum,brigtness);
	printf("light transaction 3 Sended and chksum is %d\n",cheksum);
}
void provision_blink(){
	protocolUARTSend();
	    vTaskDelay(250);
	protocolUARTSend2(0x01);
	  vTaskDelay(100);
uint8_t cheksum=savAndDochcksm(100,3000);
uartWrite(cheksum,100);
vTaskDelay(50);
cheksum=savAndDochcksm(0,3000);
uartWrite(cheksum,0);
}
static void uart_event_task(void *pvParameters)
{
	uart_event_t event;
	uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);
	int uart_byte_count = 0;

	for (;;) {
		// Waiting for UART event.

		if (xQueueReceive(uart1_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
			bzero(dtmp, RD_BUF_SIZE);
			ESP_LOGD(TAG, "uart[%d] event:", UART_UC_IF);

			switch (event.type) {
				case UART_DATA:
					ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
					uart_byte_count = uart_read_bytes(UART_UC_IF, dtmp, event.size, 0);
					for(size_t i=0; i<uart_byte_count; i++) {
						//printf("a : %d\n", dtmp[i]);
						ESP_LOGI(TAG,"%d ",dtmp[i]);
					}
					break;

				// Event of HW FIFO overflow detected
				case UART_FIFO_OVF:
					ESP_LOGE(TAG, "hw fifo overflow");
					// If fifo overflow happened, you should consider adding flow control for your application.
					// The ISR has already reset the rx FIFO,
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(UART_UC_IF);
					xQueueReset(uart1_queue);
					break;

				// Event of UART ring buffer full
				case UART_BUFFER_FULL:
					ESP_LOGE(TAG, "ring buffer full");
					// If buffer full happened, you should consider encreasing your buffer size
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(UART_UC_IF);
					xQueueReset(uart1_queue);
					break;

				case UART_PARITY_ERR:
					ESP_LOGE(TAG, "uart parity error");
					break;

				// Event of UART frame error
				case UART_FRAME_ERR:
					ESP_LOGE(TAG, "uart frame error");
					break;

				// Others
				default:
					ESP_LOGD(TAG, "uart event type: %d", event.type);
					break;
			}
		}
	}

	free(dtmp);
	dtmp = NULL;
	vTaskDelete(NULL);
}

void uart_setup() {
	// intiatercvdStructureData();

	uart_config_t uart_uc_config = {
		.baud_rate = 9600,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE
	};
	uart_param_config(UART_UC_IF, &uart_uc_config);
	uart_set_pin(UART_UC_IF, UART_UC_TX, UART_UC_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	// Install UART driver, and get the queue.
	uart_driver_install(UART_UC_IF, BUF_SIZE * 2, BUF_SIZE * 2, 100, &uart1_queue, 0);

	xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}