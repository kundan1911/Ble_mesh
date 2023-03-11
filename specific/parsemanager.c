#include<stdint.h>
#include<string.h>

#include "commJSON.h"
#include "genericUdpTask.h"
// #include "httpclient.h"
#include "httpProtocol.h"
#include "httpUtils.h"
#include "nvs_basic.h"
#include "nvs_common.h"
#include "ota.h"
#include "parsemanager.h"
#include "pico_main.h"
#include "scenes.h"
#include "uart_protocol.h"

#include "mqtt_publish.h"
#include "mqtt_protocol.h"

#include "cJSON.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_wifi_types.h"
#include "rmt_tx.h"
#include "ir_decode.h"
#include "ex_eeprom.h"
#include "bldc_fan.h"

# include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "ble_mesh.h"
#include "ble_mesh_protocol.h"
static const char *TAG = "parsemanager";



device_status_t prevDeviceSetting;



void report_error()
{
	if(device_status.update == true)
	{
		device_status.update = false;
		cJSON *devices = cJSON_CreateArray();
		cJSON* device = cJSON_CreateObject();
		for (int i = 0; i < device_status.no_of_rs485_device; i++)
		{
			if(device_status.basic[i].active == false)
			{
				continue;
			}
			cJSON_AddItemToObject(device, "subDeviceId",cJSON_CreateNumber(i));
			cJSON_AddItemToObject(device, "serial_time", cJSON_CreateNumber(device_status.basic[i].serial_update_time));
			cJSON_AddItemToObject(device, "active", cJSON_CreateNumber(device_status.basic[i].active));
		}
		if(device_status.dali[0].active == true)
		{
			cJSON_AddItemToObject(device, "subDeviceId",cJSON_CreateNumber(33));
			cJSON_AddItemToObject(device, "serial_time", cJSON_CreateNumber(device_status.dali[0].serial_update_time));
			cJSON_AddItemToObject(device, "active", cJSON_CreateNumber(device_status.dali[0].active));
		}
		cJSON_AddItemToArray(devices, device);
		char *temp = cJSON_Print(devices);
		cJSON_Delete(devices);
		int leng = strlen(temp);
		printf("length - %d\n",leng);
		printf("data - %s\n",temp);
		int ret = 0;
		if(leng > 2)
			ret = postDiagnostic(temp);
		free(temp);
	}
}
bool report_basic_state()
{
	cJSON *devices = cJSON_CreateArray();
	for (int i = 0; i < device_status.no_of_rs485_device; i++)
	{
		if(device_status.basic[i].active == false || device_status.basic[i].update == false)
		{
			continue;
		}
		
		cJSON* device = cJSON_CreateObject();

		char arc_level_str[device_status.basic[i].no_of_switch * 2];
		// printf("no_of_control_gear - %d \n",device_status.no_of_control_gear);
		for (int j = 0; j < device_status.basic[i].no_of_switch; j++)
		{
			// printf("port - %d, arc_level - %d \n",i,control_gear[i].arc_level);
			sprintf(arc_level_str+j*2,"%02x",device_status.basic[i].switch_config[j].arc_level);
		}
		cJSON_AddItemToObject(device, "arcLevel", cJSON_CreateString(arc_level_str));


		cJSON_AddItemToObject(device, "deviceType",cJSON_CreateNumber(device_status.basic[i].device_type));
		cJSON_AddItemToObject(device, "subDeviceId",cJSON_CreateNumber(i));

		cJSON_AddItemToArray(devices, device);
	}
	char *temp = cJSON_Print(devices);
	cJSON_Delete(devices);
	int leng = strlen(temp);
	printf("length - %d\n",leng);
	printf("data - %s\n",temp);
	int ret = 0;
	if(leng > 2)
		ret = postStatistic(temp);
	free(temp);
	if(ret == 1)
	{
		for (int i = 0; i < device_status.no_of_rs485_device; i++)
		{
			device_status.basic[i].update = false;
		}
	}
	return true;
}


bool report_dali_state()
{
	// printf("AAAAAA");
	printf("update dali - %d %d",device_status.dali[0].update,device_status.dali[0].active);
	if(device_status.dali[0].update == false || device_status.dali[0].active == false)
	{
		return false;
	}
	// printf("BBBB");
	cJSON *root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "deviceType",cJSON_CreateNumber(device_status.dali[0].device_type));
	cJSON_AddItemToObject(root, "subDeviceId",cJSON_CreateNumber(33));
	cJSON_AddItemToObject(root, "controlGearStatus", cJSON_CreateNumber(device_status.dali[0].control_gear_status));
	char arc_level_str[DALI_NUMBER_OF_MAX_CONTROL_GEAR*2];
	// printf("no_of_control_gear - %d \n",device_status.no_of_control_gear);
	for (int i = 0; i < device_status.dali[0].no_of_control_gear; i++)
	{
		// printf("port - %d, arc_level - %d \n",i,control_gear[i].arc_level);
		sprintf(arc_level_str+i*2,"%02x",device_status.dali[0].control_gear[i].arc_level);
	}
	cJSON_AddItemToObject(root, "arcLevel", cJSON_CreateString(arc_level_str));
	char color_temp_str[DALI_NUMBER_OF_MAX_CONTROL_GEAR*4];
	// printf("no_of_control_gear - %d \n",device_status.no_of_control_gear);
	for (int i = 0; i < device_status.dali[0].no_of_control_gear; i++)
	{
		// printf("port - %d, color_temp - %d \n",i,control_gear[i].color_temp);
		sprintf(color_temp_str+i*4,"%04x",device_status.dali[0].control_gear[i].color_temp);
	}
	cJSON_AddItemToObject(root, "colorTemperature", cJSON_CreateString(color_temp_str));

	char *temp = cJSON_Print(root);
	cJSON_Delete(root);
	int ret = postStatistic(temp);
	free(temp);
	if(ret == 1)
	{
		device_status.update_dali = false;
		device_status.dali[0].update = false;
	}
	return true;
}


uint8_t report_count = 0;
int reportCurrentState() {
	switch (report_count)
	{
	case 0:
		report_basic_state();
		report_count = 1;
		break;
	case 1:
		report_dali_state();
		report_count = 2;
		break;
	case 2:
		report_error();
		report_count = 0;
		break;
	
	default:
		break;
	}
	return 0;
}


int addToChangeLog(int subDeviceId ,int port , int newstate , int source, int field) {

	int works = 0;
	ESP_LOGD(TAG, "subDeviceId : %d, Port : %d, NewState : %d, Source : %d", subDeviceId, port, newstate, source);

	cJSON *devices = cJSON_CreateArray();
	cJSON* device = cJSON_CreateObject();
	cJSON_AddItemToObject(device, "port",
			cJSON_CreateNumber(port));
	cJSON_AddItemToObject(device, "subDeviceId",
			cJSON_CreateNumber(subDeviceId));

	if 	(field == 0){
		cJSON_AddItemToObject(device, "state",
				cJSON_CreateNumber(newstate));
	}
	if 	(field == 1){
		cJSON_AddItemToObject(device, "colorTemperature",
				cJSON_CreateNumber(newstate));
	}
	if 	(field == 2){
		cJSON_AddItemToObject(device, "fanspeed",
				cJSON_CreateNumber(newstate));
	}
	
	if (source == LOCAL_SOURCE_SWITCHBOARD) {
		cJSON_AddItemToObject(device, "source",
				cJSON_CreateString("switchboard"));
	}
	else if (source == LOCAL_SOURCE_SMARTPHONE) {
		cJSON_AddItemToObject(device, "source",
				cJSON_CreateString("mobile"));
	}
	else if (source == LOCAL_SOURCE_SCENE_SELF) {
		cJSON_AddItemToObject(device, "source",
				cJSON_CreateString("scene_self"));
	}
	else if (source == LOCAL_SOURCE_SCENE_EXT) {
		cJSON_AddItemToObject(device, "source",
				cJSON_CreateString("scene_ext"));
	}
	else {
		// cJSON_AddItemToObject(device, "source",
		// 		cJSON_CreateString("unknown"));
	}
	
	cJSON_AddItemToArray(devices, device);

	char *temp = cJSON_Print(devices);
	cJSON_Delete(devices);

	int ret = postStatistic(temp);
	free(temp);

	return 0;
}



int firstTimeLoggedIn(){
	int ret =reportCurrentState(true);
	if(ret<0)
		{
			//Printwarning("calling SOFT:restart ");
			// restart();
			return -1 ;
		}
	ret = reportDeviceDetail();
	if(ret<0)
		{
			//Printwarning("calling SOFT:restart ");
			// restart();
			return -1 ;
		}
	return 1;
}



bool get_port_device_id(int body,int *device_id,int *port_number)
{
	printf("get_port_device_id");
	char data_r[10];
	memset(data_r,0,10);
	sprintf(data_r,"%d",body);

	printf("data_r - %c %c",data_r[0],data_r[1]);

	if(strlen(data_r) < 2)
	{
		return -1;
	}

	char id[3],port[3];
	memset(id,0,3);

	for(int i = 0; i < strlen(data_r) - 1; i++)
	{
		id[i] = data_r[i];
	}

	printf("data_r - %s",data_r);
	printf("id - %s",id);
	
	int len = strlen(data_r);
	printf("strlen - %d",len);
	port[0] = data_r[len-1];
	port[1] = 0;
	port[2] = 0;




	*device_id = atoi(id);
	*port_number = atoi(port);
	return 1;

}

//////////////////////////////////////////////////////////////// Command Type 1
// Change the state of single port
int cloud_change_states(cJSON *body) {
	


	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * state = NULL;
	state = cJSON_GetObjectItem(body, "state");
	cJSON * colorTemperature= NULL;
	colorTemperature = cJSON_GetObjectItem(body, "colorTemperature");
	cJSON * fanSpeed = NULL;
	fanSpeed = cJSON_GetObjectItem(body, "fanspeed");

	if(port == NULL)
	{
		return -2;
	}
	uint8_t port_number = port->valueint-10;
	// port_number=26;
	// uint8_t sub_device_id = subDeviceId->valueint;

	if(state != NULL && fanSpeed == NULL)
	{
		uint8_t port_state = state->valueint;
		printf("\nport_number - %d port_state - %d\n",port_number,port_state);

		snd_mssg_to_vnd_srv(port_number);
		// sendAndReceiveBleData(port_number,port_state,0);
		// ble_send_onoff_command(port_number,port_state);
		//if(ae_change_states(sub_device_id,port_number, port_state)) {
		//	return 1;
		//}
	}
	else if(colorTemperature != NULL)
	{
		uint16_t port_color_temp = colorTemperature->valueint;
		printf("\nport_number - %d port_color_temp - %d\n",port_number,port_color_temp);
		// ble_send_command_color(port_number,port_color_temp);
		sendAndReceiveBleData(port_number,port_color_temp,1);
		// if(ae_set_color_temp(sub_device_id,port_number, port_color_temp)) {
		// 	return 1;
		// }
	}
	else if(fanSpeed != NULL)
	{
		cJSON * pulseTime = NULL;
		pulseTime = cJSON_GetObjectItem(body, "timer");

		uint8_t port_number = port->valueint;
		uint16_t fan_Speed = fanSpeed->valueint;
		uint16_t pulse_time = pulseTime->valueint;

		ESP_LOGE(TAG, "port_number id - %d fan_Speed - %d", port_number,fan_Speed);

		// int ret = set_bldc_fan_speed(sub_device_id,port_number, fan_Speed,pulse_time);
		// return ret;

		// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
		// if(set_bldc_fan_speed(sub_device_id,port_number, fan_Speed,pulse_time) == 1) {
		// 	return 1;
		// }	
	}
	return 0;	

	
}
//////////////////////////////////////////////////////////////// Command Type 2
//
int cloudSetEncryptionKey(char *body) {
	char objid[33];
	memset(objid,0,33);
	int length;
	if(extractData(body,"key",objid,&length))
	{
		if (nvs_set_udp_password(objid)!=ESP_OK) {
			return -2;
		}
		printf("Password updated len %d\n",length);
		if (nvs_get_udp_password(UDP_PASSWORD)!=ESP_OK) {
			return -3;
		}
		//If Key Was Successfully Obtained, Set devicePrivate To True
		devicePrivate = true;
		return 1;
	}
	return -1;
}
//////////////////////////////////////////////////////////////// Command Type 3
//
//////////////////////////////////////////////////////////////// Command Type 4
//
int cloud_set_switch_type(cJSON *body) {
	// int device_id,port,switch_type;
	// if(extractNumber(body,"port",&port) < 0) return -1;
	// if(extractNumber(body,"switch_type",&switch_type) < 0) return -1;

	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");


	cJSON * switchType = NULL;
	switchType = cJSON_GetObjectItem(body, "switchType");

	cJSON * tapNo = NULL;
	tapNo = cJSON_GetObjectItem(body, "tapNo");
	cJSON * sceneId = NULL;
	sceneId = cJSON_GetObjectItem(body, "sceneId");
	cJSON * sceneAddress = NULL;
	sceneAddress = cJSON_GetObjectItem(body, "sceneAddress");

	if(port == NULL)
	{
		return -2;
	}
	int port_number = port->valueint;
	uint8_t sub_device_id = subDeviceId->valueint;

	if(switchType != NULL && tapNo != NULL && sceneId != NULL && sceneAddress != NULL)
	{
		uint8_t switch_type = switchType->valueint;
		uint8_t tap_no = tapNo->valueint;
		uint8_t scene_id = sceneId->valueint;
		uint8_t scene_address = sceneAddress->valueint;
		// ESP_LOGI(TAG, "port - %d , switch type - %d , tap_no - %d , scene_id - %d , scene_address - %d", __func__, port_number,switch_type,tap_no,scene_id,scene_address);
		if(ae_set_switch_type(sub_device_id,port_number,switch_type,tap_no,scene_id,scene_address)) {
			return 1;
		}
	}
	return 0;
}
//////////////////////////////////////////////////////////////// Command Type 5
//
//////////////////////////////////////////////////////////////// Command Type 6
//
int cloudSetstateRetain(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * stateRetain = NULL;
	stateRetain = cJSON_GetObjectItem(body, "stateRetain");

	

	if (stateRetain == NULL) return -1;

	uint8_t state_retain = stateRetain->valueint;
	uint8_t sub_device_id = subDeviceId->valueint;

	if(ae_set_state_recovery_type(sub_device_id,state_retain)) {
		return 1;
	}
	else {
		return -1;
	}
	return -1;

}
//////////////////////////////////////////////////////////////// Command Type 7
//
//////////////////////////////////////////////////////////////// Command Type 8
//
//////////////////////////////////////////////////////////////// Command Type 9
//
int cloud_scene_store_uc(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * grpupAddr = NULL;
	grpupAddr = cJSON_GetObjectItem(body, "groupAddr");
	cJSON * sceneId = NULL;
	sceneId = cJSON_GetObjectItem(body, "sceneId");
	cJSON * sceneType = NULL;
	sceneType = cJSON_GetObjectItem(body, "sceneType");
	cJSON * daliSceneId = NULL;
	daliSceneId = cJSON_GetObjectItem(body, "daliSceneId");
	cJSON * gearType = NULL;
	gearType = cJSON_GetObjectItem(body, "gearType");
	cJSON * colorType = NULL;
	colorType = cJSON_GetObjectItem(body, "colorType");
	cJSON * state = NULL;
	state = cJSON_GetObjectItem(body, "state");
	cJSON * color_temp = NULL;
	color_temp = cJSON_GetObjectItem(body, "colorTemp");

	if(daliSceneId != NULL)
	{
		uint8_t sub_device_id = subDeviceId->valueint;
		uint8_t port_number = port->valueint;
		uint8_t grpup_addr = grpupAddr->valueint;
		uint8_t gear_type = gearType->valueint;
		uint8_t color_type = colorType->valueint;
		uint8_t scene_number = sceneId->valueint;
		uint8_t scene_type = sceneType->valueint;
		uint8_t dali_scene_id = daliSceneId->valueint;
		uint8_t arc_level = state->valueint;

		uint16_t color_temp_level = 0;

		if (color_temp != NULL) 
		{
			color_temp_level = color_temp->valueint;
		}
		printf("\port_number id - %d scene_number - %d arc_level - %d color_temp_level - %d",port_number,scene_number,arc_level,color_temp_level);

		// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
		if(ae_scene_store_dali(sub_device_id,port_number,grpup_addr, gear_type, color_type,scene_number, scene_type,dali_scene_id, arc_level,color_temp_level)) {
			return 1;
		}
	}
	else
	{
		uint8_t sub_device_id = subDeviceId->valueint;
		uint8_t scene_number = sceneId->valueint;
		uint8_t scene_type = sceneType->valueint;
		
		if(sub_device_id > NUMBER_OF_MAX_RS485_DEVICE && sub_device_id != 255)
		{
			return 0;
		}
		if(sub_device_id == 255){
					uint8_t load_state[4];
		for(uint8_t i=0;i<4;i++)
		{
			cJSON * load = NULL;
			char data_r[10];
			memset(data_r,0,10);
			sprintf(data_r,"load%d",(uint8_t)(i+1));
			load = cJSON_GetObjectItem(body, data_r);
			if(load != NULL)
			{
				load_state[i] = load->valueint;
			}
			else
			{
				load_state[i] = 0xFF;
			}
			printf("\port_number id - %d scene_number - %d",i,load_state[i]);

		}
		printf("\ndevice_id id - %d scene_number - %d",sub_device_id,scene_number);

		// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
		if(ae_scene_store(sub_device_id,scene_number, scene_type,&load_state)) {
			return 1;
		}

		}else{
		uint8_t load_state[device_status.basic[sub_device_id].no_of_switch];
		for(uint8_t i=0;i<device_status.basic[sub_device_id].no_of_switch;i++)
		{
			cJSON * load = NULL;
			char data_r[10];
			memset(data_r,0,10);
			sprintf(data_r,"load%d",(uint8_t)(i+1));
			load = cJSON_GetObjectItem(body, data_r);
			if(load != NULL)
			{
				load_state[i] = load->valueint;
			}
			else
			{
				load_state[i] = 0xFF;
			}
			printf("\port_number id - %d scene_number - %d",i,load_state[i]);

		}
		printf("\ndevice_id id - %d scene_number - %d",sub_device_id,scene_number);

		// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
		if(ae_scene_store(sub_device_id,scene_number, scene_type,&load_state)) {
			return 1;
		}
		}
	}

	
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 10
//
int cloudSetDimmingStrategies(char *body) {
	int port,dimmingStrategies,subDeviceId;
	if(extractNumber(body,"subDeviceId",&subDeviceId) < 0) return -1;
	if(extractNumber(body,"port",&port) < 0) return -1;
	if(extractNumber(body,"dimmingStrategies",&dimmingStrategies) < 0) return -1;
	// if (port > NUMBER_OF_MAX_SWITCHES || port < 0) return -1;
	if (dimmingStrategies > 3 || dimmingStrategies < 0) return -1;
	if(ae_set_dimming_strategy(subDeviceId,port,dimmingStrategies)) {
		return 1;
	}
	else {
		return -1;
	}
	return -1;
}
//////////////////////////////////////////////////////////////// Command Type 11
//
int cloudSetWiFiSSIDInMemory(char *body) {
	wifi_config_t wifi_sta_cfg;
	esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_sta_cfg);
	if(wifi_sta_cfg.sta.ssid[0] == 0) {
		return 0;
	}
	if (nvs_set_sta_credentials((char *)wifi_sta_cfg.sta.ssid, (char *)wifi_sta_cfg.sta.password) == ESP_OK) {
		return 1;
	}
	else {
		return -1;
	}

}
//////////////////////////////////////////////////////////////// Command Type 12
//
//////////////////////////////////////////////////////////////// Command Type 13
//
int cloud_set_fade_time(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * fadeTime = NULL;
	fadeTime = cJSON_GetObjectItem(body, "fadeTime");
	cJSON * fadeRate = NULL;
	fadeRate = cJSON_GetObjectItem(body, "fadeRate");
	cJSON * portType = NULL;
	portType = cJSON_GetObjectItem(body, "portType");

	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_number = port->valueint;
	uint8_t fade_time = fadeTime->valueint;


	if ( portType != NULL ){
		uint8_t port_type = portType->valueint;
		printf("\port_number id - %d portType - %d",port_number,port_type);
		if(ae_fade_time_set(sub_device_id,port_number,port_type, fade_time)) {return 1;}
	}
	
	if ( fadeRate != NULL ){
		uint8_t fade_rate = fadeRate->valueint;
		printf("\port_number id - %d fadeRate - %d",port_number,fade_rate);
		if(ae_fade_time_set_dali(sub_device_id,port_number, fade_time, fade_rate)) {return 1;}
	}

	return 0;
	
}
//////////////////////////////////////////////////////////////// Command Type 14
//
int cloudSetRelayControl(char *body) {

	return -1;
}
//////////////////////////////////////////////////////////////// Command Type 15
//
int cloud_scene_remove(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * scene = NULL;
	scene = cJSON_GetObjectItem(body, "scene");


	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_number = port->valueint;
	uint8_t scene_number = scene->valueint;

	printf("\port_number id - %d scene_number - %d",port_number,scene_number);
	erase_scene(scene_number);

	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(ae_scene_remove(sub_device_id,port_number, scene_number)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 16
//
int cloud_scene_execuate(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * scene = NULL;
	scene = cJSON_GetObjectItem(body, "sceneId");


	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_number = scene->valueint;
	uint16_t scene_number = scene->valueint;

	printf("\port_number id - %d scene_number - %d",port_number,scene_number);
	add_to_scene_queue(scene_number);

	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(ae_scene_execuate(sub_device_id,port_number, scene_number)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 20
// Change the state of single port with port timeout
int cloudChangeStates_time(cJSON *body) {

	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");

	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * state = NULL;
	state = cJSON_GetObjectItem(body, "state");
	cJSON * timer= NULL;
	timer = cJSON_GetObjectItem(body, "timer");

	if(port == NULL || subDeviceId == NULL || state == NULL || timer == NULL)
	{
		return -2;
	}
	uint8_t port_number = port->valueint;
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_state = state->valueint;
	uint8_t port_time = timer->valueint;

	if(ae_change_states_timer(sub_device_id,port_number, port_state,port_time)) {
		return 1;
	}

	return 0;
}


//////////////////////////////////////////////////////////////// Command Type 21
//
int cloud_set_group_address(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * groupId = NULL;
	groupId = cJSON_GetObjectItem(body, "groupId");
	cJSON * active = NULL;
	active = cJSON_GetObjectItem(body, "active");

	if(subDeviceId == NULL || port == NULL || groupId == NULL)
	{
		return -2;
	}

	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_number = port->valueint;
	uint8_t group_number = groupId->valueint;

	uint8_t active_state = 1;
	if(active != NULL)
	{
		active_state = active->valueint;
	}
	printf("\nport_number id - %d group_number - %d",port_number,group_number);

	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(ae_group_addr_set(sub_device_id,port_number, group_number, active_state)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 22
// dali add new control gear
int cloud_control_gear_add(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * present_control_gear = NULL;
	present_control_gear = cJSON_GetObjectItem(body, "addr");
	cJSON * longAddr = NULL;
	longAddr = cJSON_GetObjectItem(body, "longAddr");


	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t present_control_gear_number = present_control_gear->valueint;


	// printf("\port_number id - %d port_state - %d",port_number,port_state);

	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(longAddr != NULL)
	{
		uint32_t long_addr = longAddr->valueint;
		if(ae_add_control_gear_using_long_addr(sub_device_id,present_control_gear_number,long_addr)) {
			return 1;
		}
	}
	else
	{
		if(ae_control_gear_add(sub_device_id,present_control_gear_number)) {
			return 1;
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////// Command Type 23
// dali  new control gear scerch
int cloud_control_gear_scerch(cJSON *body) {

	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");

	uint8_t sub_device_id = subDeviceId->valueint;

	if(ae_new_control_gear_scerch(sub_device_id)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 24
// dali reset control gear
int cloud_reset_control_gear(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * rst_type = NULL;
	rst_type = cJSON_GetObjectItem(body, "rstType");


	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t control_gear_addr = port->valueint;
	uint8_t control_gear_rst_type = rst_type->valueint;

	if(ae_reset_control_gear(sub_device_id,control_gear_addr,control_gear_rst_type)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 25
// cloud_update_control_gear_info
int cloud_update_control_gear_info(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");


	uint8_t sub_device_id = subDeviceId->valueint;

	if(ae_update_control_gear_info_dali(sub_device_id)) {
		return 1;
	}
	return 0;
	
}
//////////////////////////////////////////////////////////////// Command Type 26
// cloud_blink_control_gear_using_long_addr
int cloud_blink_control_gear_using_long_addr(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * longAddr = NULL;
	longAddr = cJSON_GetObjectItem(body, "longAddr");


	uint8_t control_gear_addr = 255;

	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint32_t control_gear_long_addr = longAddr->valueint;

	if(ae_add_control_gear_using_long_addr(sub_device_id,control_gear_addr,control_gear_long_addr)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 27
// dali reset control gear
int cloud_dali_command(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * type = NULL;
	type = cJSON_GetObjectItem(body, "commandType");
	cJSON * command = NULL;
	command = cJSON_GetObjectItem(body, "command");
	cJSON * data = NULL;
	data = cJSON_GetObjectItem(body, "data");


	
	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t dali_command_type = type->valueint;
	uint8_t dali_command_command = command->valueint;
	uint8_t dali_command_data = data->valueint;

	int ret = ae_dali_raw_command(sub_device_id,dali_command_type,dali_command_command,dali_command_data);

	return ret;
	
}

//////////////////////////////////////////////////////////////// Command Type 29
// step up down
int cloud_step_up_down(cJSON *body) {
	
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * colorType = NULL;
	colorType = cJSON_GetObjectItem(body, "colorType");
	cJSON * stepUp = NULL;
	stepUp = cJSON_GetObjectItem(body, "stepUp");
	cJSON * stepDown = NULL;
	stepDown = cJSON_GetObjectItem(body, "stepDown");
	int ret = -1;

	if(port == NULL)
	{
		return -2;
	}
	uint8_t port_number = port->valueint;

	uint8_t color_type = 0;
	if(colorType != NULL)
	{
		color_type = colorType->valueint;
	}
	uint8_t sub_device_id = subDeviceId->valueint;
	

	if(stepUp != NULL)
	{
		uint8_t step_up_count = stepUp->valueint;
		ret = ae_dali_step_up_down(sub_device_id,port_number,1,color_type,step_up_count);
	}
	else if(stepDown != NULL)
	{
		uint8_t step_up_count = stepDown->valueint;
		ret = ae_dali_step_up_down(sub_device_id,port_number,2,color_type,step_up_count);
	}

	

	return ret;
	
}

//////////////////////////////////////////////////////////////// Command Type 30
// cloud_rs485_sync
int cloud_rs485_sync(cJSON *body) {
	


	get_device_on_bus_enable_call(2);

	return true;
	
}

//////////////////////////////////////////////////////////////// Command Type 31
// cloud_scene_store_esp
int cloud_scene_store_esp(cJSON *body) {
	ESP_LOGI(TAG, "cloud_scene_store_esp");
	cJSON * sceneId = NULL;
	sceneId = cJSON_GetObjectItem(body, "sceneId");
	cJSON * sceneType = NULL;
	sceneType = cJSON_GetObjectItem(body, "sceneType");
	cJSON * commandId = NULL;
	commandId = cJSON_GetObjectItem(body, "commandId");

	scene_entry_ir_t scene_entry_ir;
	memset(&scene_entry_ir, 0, sizeof(scene_entry_ir_t));
	

	if(sceneId == NULL || sceneType == NULL)
	{
		return -2;
	}
	
	scene_entry_ir.scene_id = sceneId->valueint;
	scene_entry_ir.scene_type = sceneType->valueint;
	scene_entry_ir.command_id = commandId->valueint;
	
	// ESP_LOGI(TAG, "scene_id:%d", scene_id);
	// ESP_LOGI(TAG, "scene_type:%d", scene_type);
	// ESP_LOGI(TAG, "command_id:%d",command_id);
	
	printf("scene_id id - %d command_id - %d\n",scene_entry_ir.scene_id,scene_entry_ir.command_id);
	

	if(scene_entry_ir.scene_type == 1)
	{
		cJSON * irData = NULL;
		irData = cJSON_GetObjectItem(body, "IRDATA");
		cJSON * irFrequency = NULL;
		irFrequency = cJSON_GetObjectItem(body, "freq");

		cJSON * nodeId = NULL;
		nodeId = cJSON_GetObjectItem(body, "nodeId");


		cJSON * multiplier = NULL;
		multiplier = cJSON_GetObjectItem(body, "multiplier");

		cJSON * beforeDelay = NULL;
		beforeDelay = cJSON_GetObjectItem(body, "beforeDelay");
		cJSON * afterDelay = NULL;
		afterDelay = cJSON_GetObjectItem(body, "afterDelay");

		


		scene_entry_ir.ir_frequency = irFrequency->valueint;
		scene_entry_ir.ir_data_multiplier = multiplier->valueint;
		scene_entry_ir.after_delay = afterDelay->valueint;
		scene_entry_ir.before_delay = beforeDelay->valueint;

		

		printf("ir_frequency - %d \n",scene_entry_ir.ir_frequency);
		

		char *json_ir_data = irData->valuestring;


		char *token;
		// /* get the first token */
		token = strtok(json_ir_data, ",");


		/* walk through other tokens */
		int j=0;
		while( token != NULL ) {
			if(j > 1400)
			{
				return -2;
			}

			for (int i = 0; i < strlen(token); i++) {
				scene_entry_ir.ir_data[j] = (scene_entry_ir.ir_data[j] * 16) + getHexToNum(token[i]);
			}
			
			token = strtok(NULL, ",");
			j++;
			
		}
		scene_entry_ir.ir_data_len = j;
		printf( "ir data length = %d\n", scene_entry_ir.ir_data_len );


		memcpy(scene_entry_ir.node_id, nodeId->valuestring, 5);
		printf("nodeID - %s \n",scene_entry_ir.node_id);
		

		// ex_eeprom_store_scene_ir(scene_entry_ir);
		int ret = write_command_to_eeprom(&scene_entry_ir);
		if(ret < 0)
		{
			return ret;
		}

	}
	return true;
}

//////////////////////////////////////////////////////////////// Command Type 33
//
int cloud_remove_group_address(cJSON *body) {
	cJSON * subDeviceId = NULL;
	subDeviceId = cJSON_GetObjectItem(body, "subDeviceId");
	cJSON * port = NULL;
	port = cJSON_GetObjectItem(body, "port");
	cJSON * groupId = NULL;
	groupId = cJSON_GetObjectItem(body, "groupId");


	if(subDeviceId == NULL || port == NULL || groupId == NULL)
	{
		return -2;
	}

	uint8_t sub_device_id = subDeviceId->valueint;
	uint8_t port_number = port->valueint;
	uint8_t group_number = groupId->valueint;

	printf("\port_number id - %d group_number - %d",port_number,group_number);

	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(ae_group_addr_set(sub_device_id,port_number, group_number, 0)) {
		return 1;
	}
	return 0;
	
}

//////////////////////////////////////////////////////////////// Command Type 39
//
int cloud_control_gear_search_ble(cJSON *body) {
	clear_all_recv_pkg_arr();
		// start_provisioner_provisioning();
		
return 0;
}
//////////////////////////////////////////////////////////////// Command Type 40
//
int cloud_control_gear_add_ble(cJSON *body) {
// esp_ble_mesh_provisioner_prov_enable(1);
	cJSON * cjson_dev_id = NULL;
	cJSON * cjson_unicast_addr = NULL;
	cJSON * cjson_addr = NULL;
	cJSON* cjson_addrType=NULL;
	cJSON* cjson_bearer=NULL;

	esp_err_t err;
	cjson_dev_id = cJSON_GetObjectItem(body, "deviceUuid");
	cjson_unicast_addr = cJSON_GetObjectItem(body, "unicastAddr");
	cjson_addr = cJSON_GetObjectItem(body, "address");
cjson_addrType=cJSON_GetObjectItem(body, "addrType");
cjson_bearer=cJSON_GetObjectItem(body, "bearer");

	if(cjson_dev_id == NULL || cjson_unicast_addr == NULL || cjson_addr == NULL || cjson_bearer== NULL || cjson_addrType == NULL)
	{
		return -2;
	}

	char* hex_dev_id = cjson_dev_id->valuestring;
	ESP_LOGI(TAG,"%s",hex_dev_id);
	char* hex_addr = cjson_addr->valuestring;
		ESP_LOGI(TAG,"%s",hex_addr);
	uint16_t custom_node_uni_addr=cjson_unicast_addr->valueint;
	// uint8_t advType=cjson_advType->valueint;
	uint8_t bearer=cjson_bearer->valueint;
	uint8_t addrType=cjson_addrType->valueint;
	ESP_LOGI(TAG,"bearer %d",bearer);
	ESP_LOGI(TAG,"addrType %d",addrType);
	ESP_LOGI(TAG,"unicast %d",custom_node_uni_addr);
    uint8_t num_dev_id[16];
	uint8_t num_addr[6];
	dev_id_decode_hex(hex_dev_id,num_dev_id);
	// dev_id_decode_hex(hex_addr,num_addr);
	
	for(int i=0;i<16;i++){
		ESP_LOGI(TAG,"dev_id from mqtt:- %d",num_dev_id[i]);
	}
	for(int i=0;i<6;i++){
		num_addr[i]=num_dev_id[i+2];
	}
	// for(int i=0;i<6;i++){
	// 	ESP_LOGI(TAG,"addr from mqtt:- %d",num_addr[i]);
	// }
	// ESP_LOGI(TAG,"received_unicast:- %d",(custom_node_uni_addr));
	// esp_ble_mesh_device_delete_t prev_node={
	// .uuid={num_dev_id},
	// 	.flag=1
	// };
	// esp_ble_mesh_provisioner_delete_dev(&prev_node);
	// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
	if(num_dev_id[0]==210 && num_dev_id[1]==196){
	esp_ble_mesh_unprov_dev_add_t add_dev = {0};
    memcpy(add_dev.addr, num_addr, BD_ADDR_LEN);
    add_dev.addr_type = 1;
    memcpy(add_dev.uuid, num_dev_id, 16);
    add_dev.oob_info = 0;
    add_dev.bearer = 2;
    err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
            ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
	if (err) {
        ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);

    }
	else{
		ESP_LOGE(TAG, "provisioned %d",err);
		// esp_ble_mesh_provisioner_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
	}
	return 0;
	}
	 err=esp_ble_mesh_provisioner_prov_device_with_addr(num_dev_id,num_addr,addrType,1,0,custom_node_uni_addr);
	if (err) {
        ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
    }
	else{
		ESP_LOGE(TAG, "not prov:%d",err);
		// esp_ble_mesh_provisioner_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
	}
	clear_all_recv_pkg_arr();
	return 0;
	
}

/*
//////////////////////////////////////////////////////////////// Command Type 
//
int cloud_send_dest_addr_to_node(cJSON *body) {
	cJSON * cjson_clt_addr = NULL;
	cjson_clt_addr = cJSON_GetObjectItem(body, "clt_addr");
	cJSON * cjson_srv_addr = NULL;
	cjson_srv_addr = cJSON_GetObjectItem(body, "srv_addr");
	uint16_t clt_addr=cjson_clt_addr->valueint;
	uint16_t srv_addr=cjson_srv_addr->valueint;
	esp_err_t err=snd_mssg_to_clt(clt_addr,srv_addr);
	if(err==ESP_OK){
		ESP_LOGI(TAG,"mssg sended successfully to clt ");
	}
	else{
		ESP_LOGI(TAG,"mssg failed to send");

	}
	return 0;
	
}
*/
//////////////////////////////////////////////////////////////// Command Type 43
//
int cloud_control_gear_info_ble(cJSON *body) {

	// get_device_on_bus_enable_call(2);
	get_node_on_bus_enable_call(1);

	return true;
	
}
void customFree(void *ptr) {
	if (ptr != NULL)
	{
		free(ptr);
	}
}

int process_commands(char *body) {
	ESP_LOGI(TAG, "data - %s",body);
	//=============================SPC===========
	char *jtoken = ":\"}";
	char *key = "action";
	char *action = getJSONvalue(body, key, jtoken);

	int ACTION =0;
	if(action!=NULL) {ACTION = (int)strtol(action, (char **)NULL, 10);
	free(action);
	}
	char  nodeRes[64];
	memset(&nodeRes,'\0',64);
	char *t1, *t2, *t3,*t4;
	switch (ACTION)	{
		case 5656:
			t1 = getJSONvalue(body, "updateFile", jtoken);
			t2 = getJSONvalue(body, "ipAdrress", jtoken);
			t3 = getJSONvalue(body, "serverPort", jtoken);
			ota_main(t2, (int)strtol(t3, (char **)NULL, 10), t1);

			return 0;
		case CMD_IRDATA:
			t1 = getJSONvalue(body, "nodeId",jtoken);
			t2 = getJSONvalue(body, "pipeAddr",jtoken);
			t3 = getJSONvalue(body, "IRDATA",jtoken);
			t4 = getJSONvalue(body, "broadcastMsg",jtoken);

			if(t1!=NULL && t2 !=NULL && t3 !=NULL) {
				char *nrf_data;
				uint16_t nrf_size = 0;
				int nrf_timer = 0;
				ir_decode_single(t3, &nrf_data, &nrf_size);
				int rc = send_ir_data(t1, t2, nrf_data, nrf_size, &nrf_timer, nodeRes);
				nodeRes[0] = '1';
				nodeRes[1] = '\0';
				free(nrf_data);
				// if (t4 != NULL) {
				// 	if (nodeRes != NULL && (nodeRes[0] == '\0' || nodeRes[0] == '1')) {
				// 		setReportChangeLog();
				// 	}
				// }
				
				free(t1);
				free(t2);
				free(t3);
				// if(rc == 1) {
				// 	if(nrf_timer>200) {
				// 		vTaskDelay((nrf_timer-200)/10);
				// 	}
				// }

				return 1;
			}
			customFree(t1);
			customFree(t2);
			customFree(t3);
			customFree(t4);
			break;

		default:
			break;
		}
		//=============================SPC===========

	cJSON * received_msg = cJSON_Parse(body);
	if (received_msg == NULL) {
		return false;
	}
	cJSON *  type = cJSON_GetObjectItem(received_msg, "type");
	if (type == NULL) 
	{
		cJSON_Delete(received_msg);
		return false;
	}
	
	int commandType = type->valueint;
	printf("Command Type = %d\n",commandType);
	//printf("Command Type = %d\n",commandType);
	int ret = -1;
	switch(commandType)
	{
		case 1:
			ret = cloud_change_states(received_msg);
			break;
		case 2:
			ret = cloudSetEncryptionKey(body);
			break;
		case 4:
			ret = cloud_set_switch_type(received_msg);
			break;
		case 6:
			ret = cloudSetstateRetain(received_msg);
			break;
		case 9:
			ret = cloud_scene_store_uc(received_msg);
			break;
		case 10:
			ret = cloudSetDimmingStrategies(body);
			break;
		case 11:
			ret = cloudSetWiFiSSIDInMemory(body);
			break;
		case 12:
			ret = cloudSetDimmingStrategies(body);
			break;
		case 13:
			ret = cloud_set_fade_time(received_msg);
			break;
		case 14:
			ret = cloudSetRelayControl(body);
			break;
		case 15:
			ret = cloud_scene_remove(received_msg);
			break;
		case 16:
			ret = cloud_scene_execuate(received_msg);
			break;
		case 17:
			break;
		case 18:
			break;
		case 20:
			ret = cloudChangeStates_time(received_msg);
			break;
		case 21:
			ret = cloud_set_group_address(received_msg);
			break;
		case 22:
			ret = cloud_control_gear_add(received_msg);
			break;
		case 23:
			ret = cloud_control_gear_scerch(received_msg);
			break;
		case 24:
			ret = cloud_reset_control_gear(received_msg);
			break;
		case 25:
			ret = cloud_update_control_gear_info(received_msg);
			break;
		case 26:
			ret = cloud_blink_control_gear_using_long_addr(received_msg);
			break;
		case 27:
			ret = cloud_dali_command(received_msg);
			break;
		case 28:
			break;
		case 29:
			ret = cloud_step_up_down(received_msg);
			break;
		case 30:
			ret = cloud_rs485_sync(received_msg);
			break;
		case 31:
			ret = cloud_scene_store_esp(received_msg);
			break;
		case 33:
			ret = cloud_remove_group_address(received_msg);
			break;
		case 39:
			ret = cloud_control_gear_search_ble(received_msg);
			break;
		case 40:
			ret = cloud_control_gear_add_ble(received_msg);
			break;
		case 43:
			ret = cloud_control_gear_info_ble(received_msg);
			break;
		default:
			break;
	}
	cJSON_Delete(received_msg);
	return ret;
}


int reportDeviceDetail() {

	return 0;

}

int reportCommand()
{
	int ret = reportCurrentState();
	return ret;
}

void initParsemanager() {
	memset(&prevDeviceSetting, 0, sizeof(device_status_t));

	mqtt_init(firstTimeLoggedIn, process_commands);
	return;
}