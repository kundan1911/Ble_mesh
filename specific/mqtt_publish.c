#include<string.h>
#include<stdint.h>
#include <stdio.h>

#include "mqtt_publish.h"
#include "mqtt_protocol.h"
#include "genericUdpTask.h"


#include "pico_main.h"

static const char *TAG = "MQTT_PUBLISH";


int postDiagnostic(char * publish_data) {
	if (publish_data == NULL)
	{
		return -2;
	}
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char mqtt_topic[80];
	memset(mqtt_topic,0,80);

	// sprintf(stats,"port=%d&state=%d&timer=%d&interLock=%d&dimmingStrategy=%d&portType=%d&switchType=%d&token=%s",port,state,timer,interLock,dimmingStrategy,portType,switchType,sessionToken);
	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));
	// printf("postStatistic = %s\n",stats);


	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"diagnostic");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 1;
}

int postBleDeviceInfo(char * publish_data) {
	if (publish_data == NULL)
	{
		return -2;
	}
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char mqtt_topic[80];
	memset(mqtt_topic,0,80);

	// sprintf(stats,"port=%d&state=%d&timer=%d&interLock=%d&dimmingStrategy=%d&portType=%d&switchType=%d&token=%s",port,state,timer,interLock,dimmingStrategy,portType,switchType,sessionToken);
	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));
	// printf("postStatistic = %s\n",stats);


	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"bleDeviceInfo");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 1;
}
//POST /switch scene
// 
int publish_switch_scene(uint8_t sub_device_id,uint8_t scene_id, uint8_t switch_no,uint8_t tap) {
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char publish_data[100];
	memset(publish_data,0,100);
	char mqtt_topic[60];
	memset(mqtt_topic,0,60);
	//printf("%s","localcommand sending ");
	// sprintf(stats,"port=%d&state=%d&source=%s&token=%s",port,newstate,source);

	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));

	sprintf(publish_data,"{\"subDeviceId\":%d,\"scene\":%d,\"switch\":%d,\"tap\":%d}",sub_device_id,scene_id,switch_no,tap);

	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"scene");

	ESP_LOGE(TAG,"data to be published: %s ",publish_data);
	
	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 0;
}


//POST /updatestats
//port , state , type , token
//200
// int postStatistic(uint8_t port,uint8_t state,uint8_t timer,uint8_t interLock,uint8_t dimmingStrategy,uint8_t portType,uint8_t switchType) {
int postStatistic(char * publish_data) {
	if (publish_data == NULL)
	{
		return -2;
	}
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char mqtt_topic[80];
	memset(mqtt_topic,0,80);

	// sprintf(stats,"port=%d&state=%d&timer=%d&interLock=%d&dimmingStrategy=%d&portType=%d&switchType=%d&token=%s",port,state,timer,interLock,dimmingStrategy,portType,switchType,sessionToken);
	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));
	// printf("postStatistic = %s\n",stats);


	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"updatestats");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 1;
}

int8_t http_post_device_status(char * publish_data) {
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	// char publish_data[170];
	// memset(publish_data,0,170);
	char mqtt_topic[60];
	memset(mqtt_topic,0,60);

	// sprintf(stats,"retainState=%d&deviceTemperature=%d&devicePower=%d&fadeTime=%d&errorCode=%d&UCFirmwareVersion=%d&token=%s",device_state_recovery,device_temperature,device_power,device_fade_time,device_error,deviceInfo.UCFirmwareVersion,sessionToken);
	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));

	// sprintf(publish_data,"{\"retainState\":%d,\"fadeTime\":%d,\"UCFirmwareVersion\":\"%d\"}",device_state_recovery,device_fade_time,deviceInfo.UCFirmwareVersion);

	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"device_detail");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);
	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 0;
}


//POST /dali control gear detail
// 
int publish_control_gear_detail(control_gear_details_t details) {
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char publish_data[100];
	memset(publish_data,0,100);
	char mqtt_topic[60];
	memset(mqtt_topic,0,60);
	//printf("%s","localcommand sending ");
	// sprintf(stats,"port=%d&state=%d&source=%s&token=%s",port,newstate,source);

	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));

	sprintf(publish_data,"{\"subDeviceId\":33,\"infoType\":%d,\"addr\":%d,\"deviceType\":%d,\"colorType\":%d}",\
		details.info_type,details.addr,details.device_type,details.color_type);

	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"controlGearInfo");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 0;
}

//POST /dali control gear long addr
// 
int publish_control_gear_long_addr(uint8_t info_type,uint32_t long_addr) {
	if (mqtt_get_connected_status() == false)
	{
		return -3;
	}
	char publish_data[100];
	memset(publish_data,0,100);
	char mqtt_topic[60];
	memset(mqtt_topic,0,60);
	//printf("%s","localcommand sending ");
	// sprintf(stats,"port=%d&state=%d&source=%s&token=%s",port,newstate,source);

	// sprintf(header,"Content-Length: %d\r\n",strlen(stats));

	sprintf(publish_data,"{\"subDeviceId\":33,\"infoType\":%d,\"longAddr\":%d}",info_type,long_addr);

	sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"controlGearInfo");

	ESP_LOGD(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 2, 0);
    ESP_LOGD(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 0;
}
