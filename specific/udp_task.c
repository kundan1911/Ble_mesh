#include<string.h>

#include "pico_main.h"
#include "uart_protocol.h"
#include "genericUdpTask.h"
#include "nvs_common.h"
#include "ota.h"
#include "parsemanager.h"
#include "udp_task.h"
#include "wifi.h"

#include "cJSON.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "error_manager.h"
#include "scenes.h"
#include "commJSON.h"

#include "polar_status_codes.h"
#include "ir_decode.h"
#include "rmt_tx.h"
#include "bldc_fan.h"


#define RECEIVER_PORT_NUM 10000
#define RECEIVER_IP_ADDR "0.0.0.0"

#define MAXNODE_SIZE 65
#define UDP_MAX_PACK_SIZE 512


mpack_rx_t mpack_rx;
mpack_tx_t mpack_tx;

// #define debug_super
// #define DEBUG

static const char *TAG = "udp_task";

void udp_broadcast_quark(void) {
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("255.255.255.255");
	sa.sin_port = htons(10000);
	char quark_message[] = "Quark";
	sendto(udp_socket_id, quark_message, strlen(quark_message), 0, (struct sockaddr *)(&sa), sizeof(struct sockaddr));
}

void broadcastUDPMessage(char *message, int len){
		struct sockaddr_in sa;
		int socket_debug = socket(PF_INET, SOCK_DGRAM, 0);
		sa.sin_family = AF_INET;
		// sa.sin_addr.s_addr = esp_ipinfo.ip.addr | ~esp_ipinfo.netmask.addr;
		sa.sin_addr.s_addr = inet_addr("255.255.255.255");
		sa.sin_port = htons(10001);
		#ifdef DEBUG
		// printf("to IP address %s\n", ipstr);
		#endif
		sendMessageToUdpSocket(socket_debug, message, len, (struct sockaddr *)&sa, NULL);
		close(socket_debug);
}

void CreateMessageAndBroadcast(uint8_t portNumber, uint8_t changeState){
	// printf("creating message port-%d state-%d\n", portNumber, changeState);
	cJSON* root = cJSON_CreateObject();
	int msg_id = 1001; //some randome number
	cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msg_id));
	cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(DEVICE_ID));
	cJSON *devices = cJSON_CreateArray();
	cJSON* device = cJSON_CreateObject();
	cJSON_AddItemToObject(device, "port", cJSON_CreateNumber(portNumber));
	cJSON_AddItemToObject(device, "state",cJSON_CreateNumber(changeState));
	cJSON_AddItemToArray(devices, device);
	cJSON_AddItemToObject(root, "devices", devices);
	char *temp = cJSON_Print(root);
	int leng = strlen(temp);
	cJSON_Delete(root);
	broadcastUDPMessage(temp, leng);
	free(temp);
}


void udp_nodes_discover(cJSON *message, int socket_fd, struct sockaddr* sa) {
	ESP_LOGI(TAG, "udp_nodes_discover ");
	cJSON *msgid = cJSON_GetObjectItem(message, "message_id");
	int leng;

	cJSON *root = cJSON_CreateObject();
	if (msgid != NULL)
		cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
	cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FIRST_RESPONSE));
	char *temp = cJSON_Print(root);
	leng = strlen(temp);
	sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
	cJSON_Delete(root);
	free(temp);

	char nodes[MAXNODE_SIZE];
	memset(nodes, 0, MAXNODE_SIZE);
	int j = 0;
	char a = '1';
	for (int i = 0;i<8;i++)
	{
		nodes[j++] =  DEVICE_ID[12];
		nodes[j++] =  DEVICE_ID[13];
		nodes[j++] =  DEVICE_ID[14];
		nodes[j++] =  DEVICE_ID[15];
		nodes[j++] =  a++;

	}

	// nrf_nodes_discover(nodes);

	root = cJSON_CreateObject();
	if (msgid != NULL)
		cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
	cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
	cJSON_AddItemToObject(root, "res", cJSON_CreateString(nodes));
	temp = cJSON_Print(root);
	leng = strlen(temp);
	sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
	cJSON_Delete(root);
	free(temp);
	return;
}

void udp_node_pair(cJSON *message, int socket_fd, struct sockaddr* sa) {
	ESP_LOGI(TAG, "udp_node_pair :");
	cJSON *msgid = cJSON_GetObjectItem(message, "message_id");
	cJSON *nodeId = cJSON_GetObjectItem(message, "nodeId");
	cJSON *nodeAddr = cJSON_GetObjectItem(message, "pipeAddr");

	cJSON *root = cJSON_CreateObject();
	if (msgid != NULL)
		cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
	cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));

	if (nodeId != NULL && nodeAddr != NULL) {
		char res[MAXNODE_SIZE];
		memset(res,0,MAXNODE_SIZE);
		res[0] = '1';
		// nrf_node_pair(nodeId->valuestring, nodeAddr->valuestring, res);
		cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
		cJSON_AddItemToObject(root, "nodeId", cJSON_CreateString(nodeId->valuestring));
	}
	else {
		char res[6];
		sprintf(res, "%d", POLAR_STAT_PARAMS_MISSING);
		cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
		cJSON *missing_params = cJSON_CreateArray();
		if (nodeId == NULL) {
			cJSON_AddItemToArray(missing_params, cJSON_CreateString("nodeId"));
		}
		if (nodeAddr == NULL) {
			cJSON_AddItemToArray(missing_params, cJSON_CreateString("pipeAddr"));
		}
		cJSON_AddItemToObject(root, "missing", missing_params);
	}

	char *temp = cJSON_Print(root);
	int leng = strlen(temp);
	sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
	cJSON_Delete(root);
	free(temp);
	return;
}



void udp_process_mpack(cJSON *message, int socket_fd, struct sockaddr* sa) {
	ESP_LOGI(TAG, "udp_process_mpack :");
	cJSON *msgid = cJSON_GetObjectItem(message, "message_id");

	cJSON *packet_index_rx = cJSON_GetObjectItem(message, "PACKID");
	cJSON *packet_index_tx = cJSON_GetObjectItem(message, "NEXTPACK");
	cJSON *subcall = cJSON_GetObjectItem(message, "scall");

	//If Recorded Data is Being Requested
	if (packet_index_tx != NULL && packet_index_rx == NULL) {
		cJSON *root = cJSON_CreateObject();
		if (msgid != NULL) {
			cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
		}
		cJSON_AddItemToObject(root, "call", cJSON_CreateString("MPACK"));
		cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
		cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_tx->valueint));

		//If there is data to be transmitted
		uint16_t start_count = (packet_index_tx->valueint) * UDP_MAX_PACK_SIZE;
		if (mpack_tx.total_size > start_count) {
			char packet_data[UDP_MAX_PACK_SIZE+1];
			uint16_t packet_size = 0;
			//If Last Packet Has To Be Sent
			if (mpack_tx.total_size - start_count < UDP_MAX_PACK_SIZE) {
				packet_size = mpack_tx.total_size - start_count;
				memcpy(packet_data, mpack_tx.data + start_count, packet_size);
				free(mpack_tx.data);
			}
			else {
				packet_size = UDP_MAX_PACK_SIZE;
				memcpy(packet_data, mpack_tx.data + start_count, packet_size);
			}
			packet_data[packet_size] = '\0';
			cJSON_AddItemToObject(root, "DATA", cJSON_CreateString(packet_data));
			cJSON_AddItemToObject(root, "PACKNUM", cJSON_CreateNumber(mpack_tx.total_size / UDP_MAX_PACK_SIZE));
		}
		else {
			cJSON_AddItemToObject(root, "code", cJSON_CreateString("0,0"));
			cJSON_AddItemToObject(root, "PACKNUM", cJSON_CreateNumber(0));
			cJSON_AddItemToObject(root, "ERROR", cJSON_CreateString("NO DATA"));
		}
		char *temp = cJSON_Print(root);
		int leng = strlen(temp);
		sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
		cJSON_Delete(root);
		free(temp);
		return;
	}

	//If Data Is Being Sent To Shoot
	else if (packet_index_tx == NULL && packet_index_rx != NULL) {
		if (subcall == NULL) {
			char res[6];
			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
			sprintf(res, "%d", POLAR_STAT_PARAMS_MISSING);
			cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
			cJSON *missing_params = cJSON_CreateArray();
			cJSON_AddItemToArray(missing_params, cJSON_CreateString("scall"));
			cJSON_AddItemToObject(root, "missing", missing_params);
			char *temp = cJSON_Print(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			return;
		}
		else if (strcmp(subcall->valuestring, "IRDA")) {
			char res[6];
			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
			sprintf(res, "%d", POLAR_STAT_PARAMS_INCORRECT);
			cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
			cJSON *incorrect_params = cJSON_CreateArray();
			cJSON_AddItemToArray(incorrect_params, cJSON_CreateString("scall"));
			cJSON_AddItemToObject(root, "incorrect", incorrect_params);
			char *temp = cJSON_Print(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			return;
		}

		cJSON *packet_count = cJSON_GetObjectItem(message, "PACKNUM");
		cJSON *packet_size = cJSON_GetObjectItem(message, "PACK_SIZE");
		cJSON *node_id = cJSON_GetObjectItem(message, "nodeId");
		cJSON *node_addr = cJSON_GetObjectItem(message, "pipeAddr");
		cJSON *multiplier = cJSON_GetObjectItem(message, "multiplier");
		if (packet_count == NULL || packet_size == NULL || \
			node_id == NULL || node_addr == NULL || msgid == NULL) {
			char res[6];
			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
			sprintf(res, "%d", POLAR_STAT_PARAMS_MISSING);
			cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
			cJSON *missing_params = cJSON_CreateArray();
			if (packet_count == NULL) {
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("PACKNUM"));
			}
			if (packet_size == NULL) {
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("PACK_SIZE"));
			}
			if (node_id == NULL) {
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("nodeId"));
			}
			if (node_addr == NULL) {
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("pipeAddr"));
			}
			if (msgid == NULL) {
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("message_id"));
			}
			cJSON_AddItemToObject(root, "missing", missing_params);
			char *temp = cJSON_Print(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			return;
		}

		//If Initial Packet Is Received
		if (packet_index_rx->valueint == 0) {
			cJSON *total_size = cJSON_GetObjectItem(message, "DATASIZE");
			memset(&mpack_rx, 0, sizeof(mpack_rx));
			mpack_rx.udp_start_time = esp_timer_get_time();
			mpack_rx.source_ip = (((struct sockaddr_in *)sa)->sin_addr).s_addr;
			mpack_rx.total_size = total_size->valueint;
			mpack_rx.packet_size = packet_size->valueint;
			mpack_rx.msg_id = msgid->valueint;
			mpack_rx.total_packets_count = packet_count->valueint;
			mpack_rx.recd_packets_count = 0;
			mpack_rx.multiplier = 50;
			if(multiplier != NULL)
			{
				mpack_rx.multiplier = multiplier->valueint;
			}
			memcpy(mpack_rx.node_id, node_id->valuestring, strlen(node_id->valuestring));
			memcpy(mpack_rx.node_addr, node_addr->valuestring, strlen(node_addr->valuestring));
			if (mpack_rx.data != NULL) {
				free(mpack_rx.data);
			}
			mpack_rx.data = (char *)calloc(mpack_rx.total_size, sizeof(char));

			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
			cJSON_AddItemToObject(root, "res", cJSON_CreateString("1"));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
			char *temp = cJSON_Print(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			return;
		}
		//If Any Subsequent Packets Are Received, Proceed Only If Parameters In This Message
		// Match Those Stored in MPACK Struct And Packet Index Is Increased By 1
		//For Any Packet Apart From Last Packet, Check If Its Size Matches Previous Packet Sizes
		 //For Last Packet, Check If Total Size Received Earlier Is Fulfilled.
		else if (mpack_rx.source_ip == (((struct sockaddr_in *)sa)->sin_addr).s_addr && \
				mpack_rx.total_packets_count == packet_count->valueint && \
				mpack_rx.recd_packets_count == (packet_index_rx->valueint - 1) && \
				((mpack_rx.total_packets_count-1 != packet_index_rx->valueint && mpack_rx.packet_size == packet_size->valueint) || \
				(mpack_rx.total_packets_count-1 == packet_index_rx->valueint && mpack_rx.total_size == (mpack_rx.recd_packets_count*mpack_rx.packet_size) + packet_size->valueint)) && \
				mpack_rx.msg_id == msgid->valueint && \
				(!strcmp(mpack_rx.node_id, node_id->valuestring)) && \
				(!strcmp(mpack_rx.node_addr, node_addr->valuestring))) {

			cJSON *ir_data = cJSON_GetObjectItem(message, "DATA");
			if (ir_data == NULL) {
				cJSON *root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
				cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
				
				char res[6];
				sprintf(res, "%d", POLAR_STAT_PARAMS_MISSING);
				cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
				cJSON *missing_params = cJSON_CreateArray();
				cJSON_AddItemToArray(missing_params, cJSON_CreateString("DATA"));
				cJSON_AddItemToObject(root, "missing", missing_params);

				char *temp = cJSON_Print(root);
				int leng = strlen(temp);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);

				free(mpack_rx.data);
				mpack_rx.data = NULL;
				memset(&mpack_rx, 0, sizeof(mpack_rx));
				return;
			}
			memcpy(mpack_rx.data + (mpack_rx.recd_packets_count * mpack_rx.packet_size), \
				ir_data->valuestring, strlen(ir_data->valuestring));
			mpack_rx.recd_packets_count = packet_index_rx->valueint;
			//If Last Packet Is Received
			if (mpack_rx.recd_packets_count == mpack_rx.total_packets_count-1) {
				cJSON *root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
				cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FIRST_RESPONSE));
				char *temp = cJSON_Print(root);
				int leng = strlen(temp);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);
				char res[6];
				mpack_rx.nrf_start_time = esp_timer_get_time();
				// char *nrf_data;
				uint16_t nrf_size = 0;
				int nrf_timer = 0;
				// ir_decode_single(mpack_rx.data, &nrf_data, &nrf_size);

				char *json_ir_data = mpack_rx.data;
				printf("code string %s\n",json_ir_data);
				uint16_t ir_data_udp[1400];
				memset(ir_data_udp,0,2800);
				uint16_t ir_data_len = 0;
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
						ir_data_udp[j] = (ir_data_udp[j] * 16) + getHexToNum(token[i]);
					}
					
					token = strtok(NULL, ",");
					j++;
					
				}
				ir_data_len = j;
				printf( "ir data length = %d\n", ir_data_len );


				res[0] = '1';
				res[1] = '\0';
				// send_ir_data(mpack_rx.node_id, mpack_rx.node_addr, nrf_data, nrf_size, &nrf_timer, res);
				int retu = send_ir_data_new(mpack_rx.node_id, ir_data_udp, ir_data_len, mpack_rx.multiplier);
				res[0] = '1';
				res[1] = '\0';
				// free(nrf_data);
				cJSON *extras = cJSON_CreateObject();
				cJSON_AddItemToObject(extras, "mpacRTime", \
					cJSON_CreateNumber((esp_timer_get_time() - mpack_rx.udp_start_time) / 1000));
				cJSON_AddItemToObject(extras, "sendIRDATATime", \
					cJSON_CreateNumber((esp_timer_get_time() - mpack_rx.nrf_start_time) / 1000));

				root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
				cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
				cJSON_AddItemToObject(root, "res", cJSON_CreateString(res));
				cJSON_AddItemToObject(root, "extras", extras);
				temp = cJSON_Print(root);
				leng = strlen(temp);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);

				free(mpack_rx.data);
				mpack_rx.data = NULL;
				memset(&mpack_rx, 0, sizeof(mpack_rx));
				return;
			}
			//For All Other Packets Except Last Packet
			else {
				cJSON *root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
				cJSON_AddItemToObject(root, "res", cJSON_CreateString("1"));
				cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_FINAL_RESPONSE));
				char *temp = cJSON_Print(root);
				int leng = strlen(temp);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);
				return;
			}
		}
		//If Unexpected Packet Id Received
		else {
			cJSON *extras = cJSON_CreateObject();
			cJSON_AddItemToObject(extras, "expected", cJSON_CreateNumber(mpack_rx.recd_packets_count + 1));
			cJSON_AddItemToObject(extras, "from", cJSON_CreateNumber(mpack_rx.source_ip));
			cJSON_AddItemToObject(extras, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));

			cJSON *root = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "PACKID", cJSON_CreateNumber(packet_index_rx->valueint));
			cJSON_AddItemToObject(root, "res", cJSON_CreateString("1"));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(POLAR_STAT_RECORDING_COUNT_ERROR));
			cJSON_AddItemToObject(root, "extras", extras);

			char *temp = cJSON_Print(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			return;
		}
	}
	// char *temp = cJSON_Print(root);
	// int leng = strlen(temp);
	// sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
	// cJSON_Delete(root);
	// free(temp);
	return;
}




bool processReceivedDataUdp(int socket_fd, char *data, int length , struct sockaddr* sa) {
	cJSON * receivedMsg = cJSON_Parse(data);
	cJSON * call = NULL;
	if (receivedMsg == NULL) {
		return false;
	}
	call = cJSON_GetObjectItem(receivedMsg, "call");
	if (call != NULL) {
		ESP_LOGI(TAG, "Data : %s", data);
		
		if (strcmp(call->valuestring, "query") == 0) {
			cJSON *root = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg, "message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(DEVICE_ID));
			cJSON_AddItemToObject(root, "fv", cJSON_CreateString(ESP_FIRMWARE_VERSION));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(1));

			cJSON *devices = cJSON_CreateArray();
			for (int i = 0; i < device_status.no_of_rs485_device; i++)
			{
				if(device_status.basic[i].active == false)
				{
					continue;
				}
				
				cJSON* device = cJSON_CreateObject();

				cJSON_AddItemToObject(device, "deviceType",cJSON_CreateNumber(device_status.basic[i].device_type));
				cJSON_AddItemToObject(device, "subDeviceId",cJSON_CreateNumber(i));

				char arc_level_str[device_status.basic[i].no_of_switch * 2];
				// printf("no_of_control_gear - %d \n",device_status.no_of_control_gear);
				for (int j = 0; j < device_status.basic[i].no_of_switch; j++)
				{
					// printf("port - %d, arc_level - %d \n",i,control_gear[i].arc_level);
					sprintf(arc_level_str+j*2,"%02x",device_status.basic[i].switch_config[j].arc_level);
				}
				cJSON_AddItemToObject(device, "arcLevel", cJSON_CreateString(arc_level_str));


				cJSON_AddItemToArray(devices, device);
			}
			cJSON_AddItemToObject(root, "devices", devices);
			
			char *temp = cJSON_Print(root);
			cJSON_Delete(root);

			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			free(temp);
		}
		else if (strcmp(call->valuestring, "queryDali") == 0) {
			cJSON *root = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg, "message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(DEVICE_ID));
			cJSON_AddItemToObject(root, "fv", cJSON_CreateString(ESP_FIRMWARE_VERSION));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(1));

			cJSON_AddItemToObject(root, "controlGearStatus", cJSON_CreateNumber(device_status.dali[0].control_gear_status));
			char arc_level_str[device_status.dali[0].no_of_control_gear*2];
			memset(&arc_level_str,0,device_status.dali[0].no_of_control_gear*2);
			printf("no_of_control_gear - %d \n",device_status.dali[0].no_of_control_gear);
			for (int i = 0; i < device_status.dali[0].no_of_control_gear; i++)
			{
				// printf("port - %d, arc_level - %d \n",i,control_gear[i].arc_level);
				sprintf(arc_level_str+i*2,"%02x",device_status.dali[0].control_gear[i].arc_level);
			}
			cJSON_AddItemToObject(root, "arcLevel", cJSON_CreateString(arc_level_str));

			char color_temperature_str[device_status.dali[0].no_of_control_gear*4];
			memset(&color_temperature_str,0,device_status.dali[0].no_of_control_gear*4);
			for (int i = 0; i < device_status.dali[0].no_of_control_gear; i++)
			{
				// printf("port - %d, color Temp - %d \n",i,control_gear[i].color_temp);
				sprintf(color_temperature_str+i*4,"%04x",device_status.dali[0].control_gear[i].color_temp);
			}
			cJSON_AddItemToObject(root, "colorTemperature", cJSON_CreateString(color_temperature_str));
			
			char *temp = cJSON_Print(root);
			cJSON_Delete(root);

			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			free(temp);
		}
		else if(strstr(call->valuestring, "deviceInfo") != NULL){
			cJSON* root = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));

			wifi_ap_record_t wifi_connected_ap_info;
			char bssid[18];
			memset(bssid,0,18);
			esp_wifi_sta_get_ap_info(&wifi_connected_ap_info);
			sprintf(bssid,MACSTR,MAC2STR(wifi_connected_ap_info.bssid));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(1));
			cJSON_AddItemToObject(root, "deviceId", cJSON_CreateString(DEVICE_ID));
			cJSON_AddItemToObject(root, "errorCode", cJSON_CreateNumber(ERROR_CODE_NO));
			cJSON_AddItemToObject(root, "MAC", cJSON_CreateString(DEVICE_MAC_STR));
			cJSON_AddItemToObject(root, "WiFi_ssid", cJSON_CreateString((char *)wifi_connected_ap_info.ssid));
			cJSON_AddItemToObject(root, "WiFi_bssid", cJSON_CreateString(bssid));
			cJSON_AddItemToObject(root, "WiFi_rssi", cJSON_CreateNumber(wifi_connected_ap_info.rssi));
			cJSON_AddItemToObject(root, "WiFi_channel", cJSON_CreateNumber(wifi_connected_ap_info.primary));
			cJSON_AddItemToObject(root, "WiFi_rssi", cJSON_CreateNumber(wifi_connected_ap_info.rssi));
			cJSON_AddItemToObject(root, "WiFi_channel", cJSON_CreateNumber(wifi_connected_ap_info.primary));
			cJSON_AddItemToObject(root, "uptime", cJSON_CreateNumber(esp_timer_get_time()/1000000));
			// cJSON_AddItemToObject(root, "rst_reason", cJSON_CreateNumber(rst_get_reason()));

			//cJSON_AddItemToObject(root, "HardwareID ",
			//cJSON_CreateString(HardwareID));

			char *temp = cJSON_Print(root);
			cJSON_Delete(root);
#ifdef debug_super
			// printf(temp);
#endif
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			free(temp);


		}
		else if(strstr(call->valuestring, "ErrorInfo") != NULL){
			cJSON* root = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg, "message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
			cJSON_AddItemToObject(root, "rc", cJSON_CreateNumber(1));
			creatErrorMessage(ERROR_CODE_NO,root);


			char *temp = cJSON_Print(root);
			cJSON_Delete(root);
			int leng = strlen(temp);
			sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
			free(temp);

		}
		else if (strstr(call->valuestring, "action") != NULL) {
			cJSON * subDeviceId = NULL;
			subDeviceId = cJSON_GetObjectItem(receivedMsg, "subDeviceId");
			cJSON * port = NULL;
			port = cJSON_GetObjectItem(receivedMsg, "port");
			cJSON * state = NULL;
			state = cJSON_GetObjectItem(receivedMsg, "state");
			cJSON * colorTemperature= NULL;
			colorTemperature = cJSON_GetObjectItem(receivedMsg, "colorTemperature");

			cJSON* ret_msg = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(ret_msg, "message_id", cJSON_CreateNumber(msgid->valueint));

			cJSON_AddItemToObject(ret_msg, "device_id", cJSON_CreateString(DEVICE_MAC_STR));
			cJSON *devices = cJSON_CreateArray();
			cJSON *device = cJSON_CreateObject();
			
			
			cJSON_AddItemToArray(devices, device);
			cJSON_AddItemToObject(ret_msg, "devices", devices);

			if(port == NULL || subDeviceId == NULL)
			{
				cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("-2"));
			}
			cJSON_AddItemToObject(device, "port", cJSON_CreateNumber(port->valueint));
			cJSON_AddItemToObject(device, "subDeviceId", cJSON_CreateNumber(subDeviceId->valueint));

			uint8_t sub_device_id = subDeviceId->valueint;
			uint8_t port_number = port->valueint;
			if(state != NULL)
			{
				uint8_t port_state = state->valueint;
				printf("\nport_number - %d port_state - %d\n",port_number,port_state);
				if(ae_change_states(sub_device_id,port_number, port_state)) {
					cJSON *source = NULL;
					source = cJSON_GetObjectItem(receivedMsg, "source");
					if (source != NULL && (!strcmp(source->valuestring, "scene"))) {
						addToChangeLog(sub_device_id, port_number, port_state, LOCAL_SOURCE_SCENE_EXT,0);
					}
					else {
						addToChangeLog(sub_device_id, port_number, port_state, LOCAL_SOURCE_SMARTPHONE,0);
					}
					cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("1"));
				}
				else
				{
					cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("0"));
				}
				cJSON_AddItemToObject(device, "state",cJSON_CreateNumber(state->valueint));
				
			}
			else if(colorTemperature != NULL)
			{
				uint16_t port_color_temp = colorTemperature->valueint;
				printf("\nport_number - %d port_color_temp - %d\n",port_number,port_color_temp);
				if(ae_set_color_temp(sub_device_id,port_number, port_color_temp)) {
				cJSON *source = NULL;
					source = cJSON_GetObjectItem(receivedMsg, "source");
					if (source != NULL && (!strcmp(source->valuestring, "scene"))) {
						addToChangeLog(sub_device_id, port_number, port_color_temp, LOCAL_SOURCE_SCENE_EXT,1);
					}
					else {
						addToChangeLog(sub_device_id, port_number, port_color_temp, LOCAL_SOURCE_SMARTPHONE,1);
					}
					cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("1"));
				}
				else
				{
					cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("0"));
				}
			}



			char *temp = cJSON_Print(ret_msg);
			int len = strlen(temp);

			// CreateMessageAndBroadcast(port_number,changeState);
			sendMessageToUdpSocket(socket_fd, temp, len, sa, NULL);
			cJSON_Delete(ret_msg);
			free(temp);
			temp=NULL;
		}

		else if (strstr(call->valuestring, "pulse") != NULL) {  
			cJSON * subDeviceId = NULL;
			subDeviceId = cJSON_GetObjectItem(receivedMsg, "subDeviceId");
			cJSON * port = NULL;
			port = cJSON_GetObjectItem(receivedMsg, "port");
			cJSON * fanSpeed = NULL;
			fanSpeed = cJSON_GetObjectItem(receivedMsg, "fanspeed");
			cJSON * pulseTimer = NULL;
			pulseTimer = cJSON_GetObjectItem(receivedMsg, "timer");	
			uint8_t sub_device_id = subDeviceId->valueint;
			uint8_t portNumber = port->valueint;
			uint8_t fan_speed = fanSpeed->valueint;
			uint16_t pulseTime = pulseTimer->valueint;
			ESP_LOGE("UDP", "port_number id - %d pulse_count - %d - pulseTime - %d", portNumber,fan_speed,pulseTime);
			int result=set_bldc_fan_speed(sub_device_id,portNumber, fan_speed, pulseTime);

			cJSON* root = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));

			cJSON_AddItemToObject(root, "device_id", cJSON_CreateString(DEVICE_MAC_STR));
			cJSON *devices = cJSON_CreateArray();
			cJSON *device = cJSON_CreateObject();
			cJSON_AddItemToObject(device, "port", cJSON_CreateNumber(portNumber));
			cJSON_AddItemToObject(device, "state",cJSON_CreateNumber(60));
			cJSON_AddItemToArray(devices, device);
			cJSON_AddItemToObject(root, "devices", devices);
			if (result == 1) {	
				cJSON *source = NULL;
					source = cJSON_GetObjectItem(receivedMsg, "source");
					if (source != NULL && (!strcmp(source->valuestring, "scene"))) {
						addToChangeLog(sub_device_id, portNumber, fan_speed-1, LOCAL_SOURCE_SCENE_EXT,2);
					}
					else {
						addToChangeLog(sub_device_id, portNumber, fan_speed, LOCAL_SOURCE_SMARTPHONE,2);
					}
					cJSON_AddItemToObject(root, "rc", cJSON_CreateString("1"));
			}

			else
				cJSON_AddItemToObject(root, "rc", cJSON_CreateString("0"));

			char *temp = cJSON_Print(root);
			int len = strlen(temp);
			//broadcastUDPMessage(temp, len);
			// CreateMessageAndBroadcast(portNumber,fan_speed);
			sendMessageToUdpSocket(socket_fd, temp, len, sa, NULL);
			cJSON_Delete(root);
			free(temp);
			temp=NULL;
		}


		else if (strstr(call->valuestring, "scene") != NULL) {
			cJSON * subDeviceId = NULL;
			subDeviceId = cJSON_GetObjectItem(receivedMsg, "subDeviceId");
			cJSON * port = NULL;
			port = cJSON_GetObjectItem(receivedMsg, "port");
			cJSON * scene = NULL;
			scene = cJSON_GetObjectItem(receivedMsg, "sceneId");

			


			uint8_t sub_device_id = subDeviceId->valueint;
			uint8_t port_number = scene->valueint;
			uint16_t scene_number = scene->valueint;

			printf("\port_number id - %d scene_number - %d",port_number,scene_number);

			cJSON* ret_msg = cJSON_CreateObject();
			cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
			if (msgid != NULL)
				cJSON_AddItemToObject(ret_msg, "message_id", cJSON_CreateNumber(msgid->valueint));

			// if (port_number > NUMBER_OF_MAX_SWITCHES || port_number < 0) return -1;
			add_to_scene_queue(scene_number);
			if(ae_scene_execuate(sub_device_id,port_number, scene_number)) {
				cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("1"));
			}
			else
			{
				cJSON_AddItemToObject(ret_msg, "rc", cJSON_CreateString("0"));
			}

			char *temp = cJSON_Print(ret_msg);
			int len = strlen(temp);

			// CreateMessageAndBroadcast(port_number,changeState);
			sendMessageToUdpSocket(socket_fd, temp, len, sa, NULL);
			cJSON_Delete(ret_msg);
			free(temp);
			temp=NULL;
			
		}
		else if (strstr(call->valuestring, "mction") != NULL) {

			cJSON *port1 = NULL;
			cJSON *port2 = NULL;
			cJSON *port3 = NULL;
			cJSON *port4 = NULL;

			port1 = cJSON_GetObjectItem(receivedMsg, "port1");
			port2 = cJSON_GetObjectItem(receivedMsg, "port2");
			port3 = cJSON_GetObjectItem(receivedMsg, "port3");
			port4 = cJSON_GetObjectItem(receivedMsg, "port4");


		}
		else if(strstr(call->valuestring, "DISC") != NULL){
			udp_nodes_discover(receivedMsg, socket_fd, sa);
		}
		else if(strstr(call->valuestring, "PAIR") != NULL){
			udp_node_pair(receivedMsg, socket_fd, sa);
		}
		else if(strstr(call->valuestring, "MPACK") != NULL){
			udp_process_mpack(receivedMsg, socket_fd, sa);
		}
		else if (strstr(call->valuestring, "wifi") != NULL) {
			//printf("Wifi data got");
			cJSON * ssid = NULL;
			ssid = cJSON_GetObjectItem(receivedMsg, "ssid");
			cJSON * password = NULL;
			password = cJSON_GetObjectItem(receivedMsg, "password");
			if (ssid != NULL && password != NULL) {
				cJSON* root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "rc", cJSON_CreateString("1"));
				cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
				if(msgid!=NULL)
					cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				char *temp = cJSON_Print(root);
				int leng = strlen(temp);
				vTaskDelay(50);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);
				temp=NULL;
				esp_wifi_disconnect();
				wifi_status_set(STA_DISCONNECT_REASON, WIFI_REASON_DISCONNECTED_MANUALLY);
				if (wifi_sta_configure(ssid->valuestring, password->valuestring, true)!=ESP_OK) {
					esp_wifi_connect();
					cJSON_Delete(receivedMsg);
					return false;
				}
				esp_wifi_connect();
			}
		}
		else if (strstr(call->valuestring, "update") != NULL) {
			cJSON * ip = NULL;
			ip = cJSON_GetObjectItem(receivedMsg, "ip");
			cJSON * port = NULL;
			port = cJSON_GetObjectItem(receivedMsg, "port");
			cJSON * file = NULL;
			file = cJSON_GetObjectItem(receivedMsg, "file");
			if (ip != NULL && port != NULL && file!=NULL) {
				cJSON* root = cJSON_CreateObject();
				cJSON_AddItemToObject(root, "rc", cJSON_CreateString("1"));
				cJSON *msgid = cJSON_GetObjectItem(receivedMsg,"message_id");
				if(msgid!=NULL)
					cJSON_AddItemToObject(root, "message_id", cJSON_CreateNumber(msgid->valueint));
				char *temp = cJSON_Print(root);
				int leng = strlen(temp);
				sendMessageToUdpSocket(socket_fd, temp, leng, sa, NULL);
				cJSON_Delete(root);
				free(temp);
				temp=NULL;
				ota_main(ip->valuestring, port->valueint, file->valuestring);
			}
		}
	}
	else {
		//If Call Is NULL,
		cJSON_Delete(receivedMsg);
		return false;
	}
	cJSON_Delete(receivedMsg);
	return true;
}

void udp_setup(void){
	startGenericUdpTask(2048,9,processReceivedDataUdp);
}
