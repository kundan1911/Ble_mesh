/*
 * upd_task.h
 */

#ifndef INCLUDE_Generic_UPD_TASK_H_
#define INCLUDE_Generic_UPD_TASK_H_
#include<stdint.h>
#include<stdbool.h>

#include "lwip/sockets.h"

char UDP_PASSWORD[33];

//void sendMessageToUdpSocket(int udp_socket_id, char *message, short len , struct sockaddr* sa );
//void startGenericUdpTask(int buffer,int deviceType,bool (*f)(int,char*,int,struct sockaddr*));
//void broadcastQuery();
char *data_buffer;
char *temp_buffer;

extern int udp_socket_id;

extern bool devicePrivate;

extern struct DeviceInfoStructure deviceInfo;

typedef struct DeviceInfoStructure
{
	char deviceIDUC[16];
	uint16_t deviceType; // This will be big-endian. hence LSb will sent first then MSB
	uint16_t UCFirmwareVersion; // This will be big-endian. hence LSb will sent first then MSB
	uint16_t extraBit; // This will be big-endian. hence LSb will sent first then MSB
}DeviceInfoStructure;

bool setDeviceInfo(DeviceInfoStructure *newDeviceInfo );

void sendMessageToUdpSocket(int socket_fd, char *message, short len , struct sockaddr* sa, char *encryption_key);

void startGenericUdpTask(int buffer,int deviceType,bool (*f)(int,char*,int,struct sockaddr*));

int myDecryptAES(uint8_t *data, int length, uint8_t *output, char *decryption_key);

void udp_set_quark_reply_function(int8_t (*quark_reply_function)(char *, struct sockaddr *));

void udp_set_cmd_reply_function(int8_t (*cmd_reply_function)(char *, uint16_t, struct in_addr));

#endif /* INCLUDE_UPD_TASK_H_ */

