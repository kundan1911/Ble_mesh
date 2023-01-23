/*
 * upd_task.h
 *
 *  Created on: Apr 5, 2015
 *	  Author: Virang
 */

#ifndef INCLUDE_UPD_TASK_H_
#define INCLUDE_UPD_TASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include<stdint.h>

#include "lwip/inet.h"
#include "lwip/sockets.h"

struct process_udp_data {
	int socket_fd;
	char *data;
	int length;
	struct sockaddr* sa;
};

typedef struct {
	int64_t udp_start_time;
	int64_t nrf_start_time;
	char* data;
	in_addr_t source_ip;
	uint16_t total_size;
	uint16_t packet_size;
	uint16_t msg_id;
	uint8_t total_packets_count;
	uint8_t recd_packets_count;
	char node_id[6];
	char node_addr[6];
	uint8_t multiplier;
}mpack_rx_t;

typedef struct {
	struct sockaddr sa;
	char* data;
	int socket_fd;
	uint16_t total_size;
	uint16_t msg_id;
}mpack_tx_t;

QueueHandle_t process_udp_data_queue;
void udp_setup(void);
void udp_broadcast_quark(void);
void broadcastUDPMessage(char *message, int len);
void CreateMessageAndBroadcast(uint8_t port, uint8_t state);
void broadcastQuery();

#endif

