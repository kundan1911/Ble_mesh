#include "genericUdpTask.h"

#include "mbedtls/aes.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdint.h>

#include "pico_main.h"
#include "nvs_common.h"

#include "esp_log.h"


static const char *TAG = "genericUdpTask";


bool devicePrivate = true;

#define RECEIVER_PORT_NUM 10000
#define RECEIVER_IP_ADDR "0.0.0.0"

int udp_socket_id;

struct DeviceInfoStructure deviceInfo;

int bufferSize= 260;

bool (*udp_process_command)(int,char*,int,struct sockaddr*) = NULL;
int8_t (*udp_process_quark_reply)(char *, struct sockaddr *) = NULL;
int8_t (*udp_process_cmd_reply)(char *, uint16_t, struct in_addr) = NULL;

bool setDeviceInfo(DeviceInfoStructure *newDeviceInfo )
{
	//printf("setDeviceInfo");
	if(newDeviceInfo->deviceType < 1 || newDeviceInfo->deviceType > 50){
		return false;
	}

	memcpy ( deviceInfo.deviceIDUC, newDeviceInfo->deviceIDUC, 16 );
	deviceInfo.deviceType = newDeviceInfo->deviceType;
	deviceInfo.UCFirmwareVersion = newDeviceInfo->UCFirmwareVersion;
	deviceInfo.extraBit = newDeviceInfo->extraBit;

	return true;
}



int encryptionAES(uint8_t *data, int length, uint8_t *output, char *encryption_key)
{
	mbedtls_aes_context context_in;
	unsigned char iv[16];
	memset(iv,0,16);
	mbedtls_aes_setkey_enc( &context_in, (unsigned char *)encryption_key, 256);
	mbedtls_aes_crypt_cbc( &context_in, MBEDTLS_AES_ENCRYPT, length, iv, data, output);
	return 0;
}
int myDecryptAES(uint8_t *data, int length, uint8_t *output, char *decryption_key)
{
	mbedtls_aes_context context_in;
	unsigned char iv[16];
	memset(iv,0,16);
	mbedtls_aes_setkey_dec( &context_in, (unsigned char *)decryption_key, 256 );
	mbedtls_aes_crypt_cbc( &context_in, MBEDTLS_AES_DECRYPT, length, iv, data, output);
	return 0;
}

void sendMessageToUdpSocket(int socket_fd, char *message, short len, struct sockaddr* sa, char *encryption_key) {
	uint8_t bytes[2];

	bytes[1] = len & 0xFF; // heigh byte
	bytes[0] = (len >> 8) & 0xFF;

	int add = (16-(2+len)%16);
	int newlen = 2 + len + add;
	uint8_t* temp = malloc(newlen+33);
	uint8_t* msg = temp + 33;
	memcpy(msg+2,message, len);
	memcpy(msg,bytes, 2);// a+n is destination, b is source and third argument is m
	memset(msg+newlen-add,add,add);
	if (encryption_key == NULL) {
		encryptionAES(msg, newlen, temp, UDP_PASSWORD);
	}
	else {
		encryptionAES(msg, newlen, temp, encryption_key);
	}
	sendto(socket_fd, temp, newlen, 0,sa, sizeof(struct sockaddr));
	//printf("Done sending \n");
	free(temp);
	//printHeap();
}

int recv_data=0;
TaskHandle_t xHandle;
#define INCLUDE_vTaskSuspend	1

#define INCLUDE_vTaskPrioritySet 1

void genericUdpTask(void *p) {
	char dataBuf1[32];

	if (nvs_get_udp_password(UDP_PASSWORD) == ESP_OK) {
		if (UDP_PASSWORD[0]!='\0') {
			devicePrivate = true;
		}
		else {
			devicePrivate = false;
		}
	}
	else {
		devicePrivate = false;
	}

	for (;;) {

		struct sockaddr_in ra;
		struct sockaddr sa;
		if(data_buffer==NULL)
		data_buffer = malloc(bufferSize);
		memset(data_buffer,0,bufferSize);
		if(temp_buffer==NULL)
		temp_buffer=malloc(bufferSize);
		memset(temp_buffer,0,bufferSize);
		/* Creates an UDP socket (SOCK_DGRAM) with Internet Protocol Family (PF_INET).
		 * Protocol family and Address family related. For example PF_INET Protocol Family and AF_INET family are coupled.
		 */

		udp_socket_id = socket(PF_INET, SOCK_DGRAM, 0);

		if (udp_socket_id < 0) {

			printf("socket call failed");

		}
		ra.sin_family = AF_INET;
		ra.sin_addr.s_addr = inet_addr(RECEIVER_IP_ADDR);
		ra.sin_port = htons(10000);

		/* Bind the UDP socket to the port RECEIVER_PORT_NUM and to the current
		 * machines IP address (Its defined by RECEIVER_PORT_NUM).
		 * Once bind is successful for UDP sockets application can operate
		 * on the socket descriptor for sending or receiving data.
		 */
		int optval =1;
		setsockopt(udp_socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

		if (bind(udp_socket_id, (struct sockaddr * )&ra, sizeof(struct sockaddr_in))
				== -1) {

			printf("Bind to Port Number %d ,IP address %s failed\n",
					RECEIVER_PORT_NUM, RECEIVER_IP_ADDR);
			close(udp_socket_id);
		}
		/* RECEIVER_PORT_NUM is port on which Server waits for data to
		 * come in. It copies the received data into receive buffer and
		 * prints the received data as string. If no data is available it
		 * blocks.recv calls typically return any availbale data on the socket instead of waiting for the entire data to come.
		 */
		socklen_t aa;


		while (1) {
			aa = sizeof(struct sockaddr);
			recv_data = recvfrom(udp_socket_id, data_buffer, bufferSize, 0,&sa, &aa);

			if (recv_data > 0) {

				data_buffer[recv_data] = '\0';
				if (strstr(data_buffer, "Quark") != NULL) {
					memset(dataBuf1,0,32);
					ESP_LOGI(TAG, "Data : %s", data_buffer);
					if(devicePrivate==true)
					{
						sprintf(dataBuf1,"alivE-%02d%s",deviceInfo.deviceType-1,DEVICE_ID);
					}
					else{
						sprintf(dataBuf1,"AlivE-%02d%s",deviceInfo.deviceType-1,DEVICE_ID);
					}

					sendto(udp_socket_id, dataBuf1, 24, 0,&sa,sizeof(struct sockaddr));
				}
				else if (strstr(data_buffer, "alivE") != NULL) {
					if (udp_process_quark_reply != NULL) {
						udp_process_quark_reply(data_buffer, &sa);
					}
				}
				else {
					bool udp_cmd_stat = false;
					if (udp_process_command != NULL) {
						if(recv_data%16!=0) {
							continue;
						}
						myDecryptAES((uint8_t *)data_buffer,recv_data,(uint8_t *)temp_buffer, UDP_PASSWORD);
						temp_buffer[recv_data]='\0'; // To avoid cJson to crash
						udp_cmd_stat = udp_process_command(udp_socket_id,temp_buffer+2,recv_data-2,&sa);
					}
					//If Command Wasn't Processed Successfully,
					//Then Pass The Message To The Scenes Reply Parser 
					if (!udp_cmd_stat) {
						if (udp_process_cmd_reply != NULL) {
							udp_process_cmd_reply(data_buffer, recv_data, ((struct sockaddr_in *)(&sa))->sin_addr);
						}
					}
				}
				recv_data=0;
			}


		}
		close(udp_socket_id);
		free(data_buffer);
		free(temp_buffer);
		data_buffer=NULL;
		temp_buffer=NULL;
	}
}

void broadcastQuery()
{
	char dataBuf1[32];
	memset(dataBuf1,0,32);
	if(devicePrivate==true)
	{
		sprintf(dataBuf1,"alivE-%02d%s",deviceInfo.deviceType-1,DEVICE_ID);
	}
	else{
		sprintf(dataBuf1,"AlivE-%02d%s",deviceInfo.deviceType-1,DEVICE_ID);
	}
	struct sockaddr_in sa;
	int udp_socket_id = socket(PF_INET, SOCK_DGRAM, 0);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr("255.255.255.255");
	sa.sin_port = htons(10000);
	sendto(udp_socket_id, dataBuf1, 24, 0,(struct sockaddr*)&sa,sizeof(struct sockaddr));
}

void udp_set_quark_reply_function(int8_t (*quark_reply_function)(char *, struct sockaddr *)) {
	udp_process_quark_reply = quark_reply_function;
}

void udp_set_cmd_reply_function(int8_t (*cmd_reply_function)(char *, uint16_t, struct in_addr)) {
	udp_process_cmd_reply = cmd_reply_function;
}

void startGenericUdpTask(int buffer, int deviceType, \
			bool (*command_function)(int, char *, int, struct sockaddr *)){

 	bufferSize = buffer;
 	//deviceNumber = deviceType;
 	udp_process_command = command_function;
 	memset(&deviceInfo,0,sizeof(struct DeviceInfoStructure));
	deviceInfo.deviceType = deviceType;

	xTaskCreate(genericUdpTask, "udpTask", 8192, NULL, 5, NULL);
}
