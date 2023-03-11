
#include <stdbool.h>
#include<stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ble_mesh_protocol.h"
#include "esp_timer.h"
#include <math.h>
// #include "esp_ble_mesh_defs.h"

// #include "esp_ble_mesh_lighting_model_api.h"
#include "ble_mesh.h"
#include "pico_main.h"

 xSemaphoreHandle temp;
#define ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI 0x1305



set_t set_state;
void assignMssgParameterValues(uint16_t data,uint8_t type){
	
set_state.tid=tid++;

if(type){
  set_state.ctl_lightness=255;
set_state.ctl_temperatrue=800+(int)floor((float)data/4);
printf("temperature: %d",set_state.ctl_temperatrue);
}
else{
    set_state.ctl_lightness=(int)floor((float)data*5/3);
    set_state.ctl_temperatrue=950;
	printf("ctl_lightness: %d",set_state.ctl_lightness);
}
set_state.op_en=true;
set_state.trans_time=0;
set_state.delay=0;

    return;
}
void pushAckToQueue(nodeAck ackw){
	xQueueSendToBack(Ble_message_ack_queue,( void * )&ackw,( TickType_t ) 0);
return;
}
nodeAck recvAck;
uint8_t sendAndReceiveBleData(uint8_t deviceId,uint16_t data,uint8_t type){

if (temp == NULL) {
		temp = xSemaphoreCreateMutex();
	}

if( xSemaphoreTake( temp, 1000 ) != pdTRUE )
	{
		return 0;
	}

assignMssgParameterValues(data,type);
xQueueReset(Ble_message_ack_queue);
	sendMessageToNode(ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI,deviceId,(void *)&set_state);

	int64_t xTime1;
	xTime1 = esp_timer_get_time();
	while (esp_timer_get_time() - xTime1 < 600000) {

		if(xQueueReceive(Ble_message_ack_queue, (void *)&recvAck,( TickType_t ) 10)){

			if(recvAck.desAddr == deviceId)
			{
				printf("received ack");
				xSemaphoreGive(temp);
				return recvAck.err;
			}
			else{
				printf("received ack of other node");
			}
		
	}
	}
	printf( "Ble Receive Timeout");
	xSemaphoreGive(temp);
	return 0;
  }


  void ble_setup() {
	Ble_message_ack_queue = xQueueCreate(5, sizeof( struct nodeAck * ));
	if(Ble_message_ack_queue == 0){
		printf("queue not created");
	}
	else{
		printf("queue  created");
	}
	// xTaskCreate(ble_event_task, "ble_event_task", 2048, NULL, 12, NULL);
}

