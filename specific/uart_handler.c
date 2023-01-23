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
#include "pico_main.h"


#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
#define UART_UC_IF UART_NUM_1
#define UART_UC_RX 16
#define UART_UC_TX 17
static QueueHandle_t uart1_queue;

static const char *TAG = "uart_events";


char starting_delimiter = '#';
char ending_delimiter = '-';
int UARTProDataLength = 70;
xSemaphoreHandle temp;
volatile bool completedReceiving = false;
volatile bool dali_long_command_receiving = false;



char getXOR(struct protocolData *toBeSendData)
{
	char xor = (char)(toBeSendData->device_id);
	xor = xor ^ (char)(toBeSendData->rc_code);
	xor = xor ^ (char)(toBeSendData->id);
	int i=0;
	for(;i<toBeSendData->length;i++)
	{
		xor = xor ^ (toBeSendData->data)[i];
	}
	return xor;
}
void protocolUARTSend(struct protocolData *toBeSendData) {
	uart_write_bytes(UART_UC_IF, &starting_delimiter, 1);
	uart_write_bytes(UART_UC_IF, (char *)&(toBeSendData->length), 1);
	uart_write_bytes(UART_UC_IF, (char *)&(toBeSendData->device_id), 1);
	uart_write_bytes(UART_UC_IF, (char *)&(toBeSendData->rc_code), 1);
	uart_write_bytes(UART_UC_IF, (char *)&(toBeSendData->id), 1);
	uart_write_bytes(UART_UC_IF, toBeSendData->data, toBeSendData->length);
	char a = getXOR(toBeSendData);
	uart_write_bytes(UART_UC_IF, &a, 1);
	uart_write_bytes(UART_UC_IF, &ending_delimiter, 1);
	uart_wait_tx_done(UART_UC_IF, 1000);
	printf("Data Sent\n");
}

int SendAndReceiveUartData(struct protocolData *toBeSendData,
		struct protocolData *dataRcvd,int timeOut) {

	if(dali_long_command_receiving == true)
	{
		return 0;
	}

	if (temp == NULL) {
		temp = xSemaphoreCreateMutex();
	}

	if( xSemaphoreTake( temp, 1000 ) != pdTRUE )
	{
		return 0;
	}
	xQueueReset(Uart_message_queue);
	protocolUARTSend(toBeSendData);

	uint8_t received_message;
	// int count = 5;
	int ATCommand = 0;
	int runningCount = -1;
	int64_t xTime1;
	xTime1 = esp_timer_get_time();
	while (esp_timer_get_time() - xTime1 < timeOut * 100000) {

		if(xQueueReceive(Uart_message_queue, (void *)&received_message, 10)){

			ExtrateUARTData(received_message,dataRcvd,&ATCommand,&runningCount);

			//printf("b : %d", received_message);

			if(ATCommand == 1)
			{
				xSemaphoreGive(temp);
				return 1;
			}
		}
	}
	ESP_LOGE(TAG, "UART Receive Timeout");
	xSemaphoreGive(temp);

	//printf("limiter = %s\n","rcvdCharater");
	return 0;
}

struct protocolData *rcvdInterruptData = NULL;
void intiatercvdStructureData()
{
	rcvdInterruptData = calloc(1,sizeof(struct protocolData));
	rcvdInterruptData->length =0;
	rcvdInterruptData->rc_code =0;
	rcvdInterruptData->id =0;
	// rcvdInterruptData->data =calloc(1,UARTProDataLength+1);
}
int InterruptrunningCount = -1;

void ReceiveInterruptUartData() {
	uint8_t received_message;
	int ATCommand = 0;
	int64_t xTime1;
	xTime1 = esp_timer_get_time();
	while (true) {
		if(xQueueReceive(stm_interrupt_message_queue, &received_message, 10)){
			ExtrateUARTData(received_message, rcvdInterruptData, &ATCommand, &InterruptrunningCount);
			if(ATCommand == 2)
			{
				processUartInterrupt(rcvdInterruptData);
				break;
			}
			else if(esp_timer_get_time() - xTime1 > 100000){
				break;
			}
		}
		else{
			return;
		}
	}
}


// function - ExtrateUARTData
// free the data variable of rcvdUratData struct after using of rcvdUratData
void ExtrateUARTData(uint8_t rcvdCharater,struct protocolData *rcvdUratData,int *insideATCommand,int *runningCount) {
	//  printf("received = %d	\n", rcvdCharater);
	if (rcvdCharater == starting_delimiter) {
		*runningCount = 0;
	}else if(*runningCount == -1){
		return;
	}
	switch (*runningCount) {
	case 0:
		break;
	case 1:
		rcvdUratData->length = rcvdCharater;
		if (rcvdCharater > UARTProDataLength){
			*insideATCommand = -1;
			//printf("%s\n", "insideATCommand false length error");
		}
		break;
	case 2:
		rcvdUratData->device_id = rcvdCharater;
		break;
	case 3:
		rcvdUratData->rc_code = rcvdCharater;
		break;
	case 4:
		rcvdUratData->id = rcvdCharater;
		break;
	default:
		if ((*runningCount - 5) >= rcvdUratData->length) {
			*insideATCommand = -1;
		}
		if ((*runningCount - 5) < rcvdUratData->length) {
			rcvdUratData->data[*runningCount - 5] = rcvdCharater;
		}
		if ((*runningCount - 5) == (rcvdUratData->length) && rcvdCharater == ending_delimiter) {
			*insideATCommand = rcvdUratData->rc_code;
			// printf("A");
		}
		else if((*runningCount - 5) == (rcvdUratData->length))
		{
			// printf("B");
			uint8_t xorcod = getXOR(rcvdUratData);
			if(rcvdCharater == xorcod)
			{
				// printf("C");
				*insideATCommand = 0;
			}
			// printf("D - %d -\n",xorcod);
			

		}
		else if((*runningCount - 5) == (rcvdUratData->length+1) && rcvdCharater == ending_delimiter)
		{
			// printf("F");
			*insideATCommand = rcvdUratData->rc_code;
		}
		
		
		break;
	}
	++(*runningCount);
}

/*
inline void handleUARTData(uint8 rcvdCharater) {
	// printf("received = %d	\n", rcvdCharater);
	xQueueSendFromISR(stm_interrupt_message_queue, &rcvdCharater, 0);
	xQueueSendFromISR(Uart_message_queue, &rcvdCharater, 0);
}
*/

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
					ESP_LOGD(TAG, "[UART DATA]: %d", event.size);
					uart_byte_count = uart_read_bytes(UART_UC_IF, dtmp, event.size, 0);
					for(size_t i=0; i<uart_byte_count; i++) {
						//printf("a : %d\n", dtmp[i]);
						xQueueSendToBack(stm_interrupt_message_queue, &dtmp[i], 0);
						xQueueSendToBack(Uart_message_queue, &dtmp[i], 0);
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
	intiatercvdStructureData();

	stm_interrupt_message_queue = xQueueCreate(sizeof(uint8_t)* 100 ,sizeof(uint8_t));
	Uart_message_queue = xQueueCreate(sizeof(uint8_t)* 100, sizeof(uint8_t));

	uart_config_t uart_uc_config = {
		.baud_rate = 19200,
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