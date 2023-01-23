#ifndef PICO_SCENES_H
#define PICO_SCENES_H

#include<stdint.h>

#include "lwip/sockets.h"

#define SCENE_CMD_TYPE_PORT_CONTROL		0
#define SCENE_CMD_TYPE_IR_DATA			1

#define SCENE_NODE_TYPE_NRF24			0
#define SCENE_NODE_TYPE_NRF52			1

#define SCENE_IR_DATA_TYPE_HUFFMAN		0
#define SCENE_IR_DATA_TYPE_SINGLE		1
#define SCENE_IR_DATA_TYPE_DOUBLE		2

#define MAX_PORT_COUNT					10

// typedef struct {
// 	uint8_t scene_id;
// 	uint8_t scene_type;
// 	char device_id[17];
// 	char udp_key[33];
// 	uint8_t command_type;
// 	uint8_t command_byte_len;
// 	uint8_t ip_addr[4];
// 	uint16_t ip_port;

// 	uint8_t node_type;
// 	char node_id[6];
// 	uint8_t ir_frequency;
// 	uint8_t ir_data_type;
// 	uint16_t ir_data_len;
// 	uint16_t ir_data[1400];
	
// }	scene_entry_ir_t;

typedef struct {
	char device_id[17];
	char udp_key[33];
	uint8_t device_type;
	int64_t device_last_resp_time;
	struct in_addr device_addr;
}scene_device_info_t;

typedef enum {
	SCENE_SOURCE_SWITCH = 0,
	SCENE_SOURCE_MOTION,
	SCENE_SOURCE_DOOR,
	SCENE_SOURCE_TMPR,
}scene_source_t;




// typedef union {
// 	struct {
// 		uint8_t scene_id;
// 		uint8_t command_id;
// 		uint8_t scene_type;
// 		char device_id[17];
// 		char udp_key[33];
// 		uint8_t command_type;
// 		uint8_t command_byte_len;
// 		uint8_t ip_addr[4];
// 		uint16_t ip_port;


// 		uint8_t node_type;
// 		char node_id[6];
// 		uint8_t ir_frequency;
// 		uint8_t ir_data_type;
// 		uint16_t ir_data_len;
// 		uint16_t ir_data[100];

// 	};


// 	struct{
// 		uint8_t data[1000];
// 	};
// }scene_entry_ir_t;


typedef struct {
	 
	uint8_t scene_id;
	uint8_t command_id;
	uint8_t scene_type;
	char device_id[17];
	char udp_key[33];
	uint8_t command_type;
	uint8_t command_byte_len;
	uint8_t ip_addr[4];
	uint16_t ip_port;
	char temp[100];


	uint8_t node_type;
	char node_id[6];
	uint8_t ir_frequency;
	uint8_t ir_data_type;
	uint8_t ir_data_multiplier;
	uint16_t ir_data_len;
	uint16_t after_delay;
	uint16_t before_delay;
	char temp2[100];
	uint16_t ir_data[1400];

}scene_entry_ir_t;



void scene_execute();
void scenes_setup(void);
int add_to_scene_queue(int scene_id);
void scene_status_set(int scene_id);


#endif