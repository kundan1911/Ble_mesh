
/*
 * aeProtocol.c
 *
 *  Created on: Apr 19, 2015
 *  Author: Virang
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "uart_protocol.h"
#include "genericUdpTask.h"
#include "parsemanager.h"
#include "scenes.h"
#include "uart_handler.h"
#include "udp_task.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_publish.h"
#include "cJSON.h"
#include "pico_main.h"

#include "esp_log.h"

static const char *TAG = "uart_protocol";

esp_timer_handle_t dali_long_command_timer;



bool set_arc_level(uint8_t device_id,uint8_t port_number, uint8_t arc_level)
{
	// printf("set_arc_level device_id - %d, port_number - %d, arc_level -%d",device_id,port_number,arc_level);
	if(device_id < 33)
	{
		if(device_status.basic[device_id].switch_config[port_number].arc_level != arc_level)
			device_status.basic[device_id].update = true;
		device_status.basic[device_id].switch_config[port_number].arc_level = arc_level;
	}
	else if(device_id == 33)
	{
		if (port_number < device_status.dali[0].no_of_control_gear)
		{
			if(device_status.dali[0].control_gear[port_number].arc_level != arc_level)
				device_status.dali[0].update = true;
			device_status.dali[0].control_gear[port_number].arc_level = arc_level;
		}
	}
	
	return false;
}

bool set_color_temp(uint8_t device_id,uint8_t port_number,uint16_t color_temp)
{
	if(device_id == 33)
	{
		if (port_number < device_status.dali[0].no_of_control_gear)
		{
			if(device_status.dali[0].control_gear[port_number].color_temp != color_temp)
				device_status.dali[0].update = true;
			device_status.dali[0].control_gear[port_number].color_temp = color_temp;
		}
	}
	
	return false;
}


bool set_device_info(uint8_t device_id,uint8_t type,uint8_t version)
{
	printf("set_device_info\n");
	printf("device_id - %d , type - %d\n",device_id,type);

	if(type == 8)
	{
		device_id = device_id - 33;
		if(device_id > 2)
		{
			return false;
		}
		printf("device_id - %d , type - %d\n",device_id,type);
		device_status.dali[device_id].active = true;
		device_status.dali[device_id].device_type = type;
		device_status.dali[device_id].uc_firmware_version = version;
	}
	else
	{
		if(device_id > 32)
		{
			return false;
		}
		device_status.basic[device_id].active = true;
		device_status.basic[device_id].device_type = type;
		device_status.basic[device_id].uc_firmware_version = version;
	}
	
	return true;
}



//UC to ESP Serial Protocol
//////////////////////////////////////////////////////Command type 1
										//output reply with all port and their types
bool aeQuery(uint8_t device_id) {
	// printf("Query\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.device_id = device_id;
	pd.length = 1;
	pd.id = 0;
	pd.data[0] = 1;
	struct protocolData rcvd_pd;
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0)
	{
		// printf("Query - Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);

		if(rcvd_pd.id == 0 && rcvd_pd.length > 0 && rcvd_pd.length < 20 && rcvd_pd.rc_code == 1)
		{
			printf("data = ");
			for(int i =0;i<rcvd_pd.length;i++)
			{
				printf("%d ",rcvd_pd.data[i]);
			}
			printf("\n");

			device_status.basic[device_id].no_of_switch = rcvd_pd.length/2;

			// device_status.no_of_switch = rcvd_pd.length/2;
			// for(int i=0;i<rcvd_pd.length;i=i+2)
			// {
			// 	device_status.switch_config[rcvd_pd.data[i]-1].type = rcvd_pd.data[i+1];
			// }
			
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////Command type 2
													//get arc_level of single port
int ae_get_arc_level(uint8_t device_id,uint8_t addr) {
	// printf("ae_get_arc_level\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	// ////pd.data = malloc(pd.length);
	pd.data[0] = 2;
	pd.data[1] = addr;
	struct protocolData rcvd_pd;
	////rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0) {
		// printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0  && rcvd_pd.rc_code == 1)
		{
			printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			// printf("data1 = %d \n",rcvd_pd.data[1]);
			// printf("addr = %d \n",addr);

			if(device_id < 33 && (device_status.basic[device_id].device_type == 1 || \
									device_status.basic[device_id].device_type == 5))
			{
				for(int i=0;i<rcvd_pd.length;i=i+2)
				{
					if(device_status.basic[device_id].switch_config[rcvd_pd.data[i]-1].pulse_count > 0 )
						continue;
					// t_control_gear[rcvd_pd.data[i]-1].arc_level = rcvd_pd.data[i+1];
					set_arc_level(device_id,rcvd_pd.data[i]-1,rcvd_pd.data[i+1]);
				}
			}
			else if(device_id == 33)
			{
				for(int i=addr;i<rcvd_pd.length;i++)
				{
					// t_control_gear[i].arc_level = rcvd_pd.data[i];
					set_arc_level(device_id,i,rcvd_pd.data[i]);
				}
			}

			return rcvd_pd.length;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;

}

//////////////////////////////////////////////////////Command type 3
													//change the arc level of port
bool ae_change_states(uint8_t device_id,uint8_t port, uint8_t new_state) {
	// printf("aeChangeStates\n");
	if (device_status.basic[device_id].switch_config[port-1].pulse_count > 0 && new_state==0){
		device_status.basic[device_id].switch_config[port-1].pulse_count=0;
		}
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 3;
	pd.device_id = device_id;
	pd.id = 0;
	// //pd.data = malloc(pd.length);
	pd.data[0] = 3;
	pd.data[1] = port;
	pd.data[2] = new_state;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// t_control_gear[port-1].arc_level = new_state;
			if(device_id < 33 && (device_status.basic[device_id].device_type == 1 ||\
									device_status.basic[device_id].device_type == 5))
			{
				set_arc_level(device_id,port-1,new_state);
			}
			else if(device_id == 33)
			{
				if(port > 63)
				{
					get_all_arc_level(device_status.dali[0].no_of_control_gear);
					get_all_color_temp(device_status.dali[0].no_of_control_gear);
				}else
				{
					set_arc_level(device_id,port,new_state);
				}
			}
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

//////////////////////////////////////////////////////Command type 3
													//change the arc level only for bldc pulse
bool ae_change_states_pulse(uint8_t device_id,uint8_t port, uint8_t new_state) {
	// printf("aeChangeStates\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 3;
	pd.device_id = device_id;
	pd.id = 0;
	// //pd.data = malloc(pd.length);
	pd.data[0] = 3;
	pd.data[1] = port;
	pd.data[2] = new_state;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// t_control_gear[port-1].arc_level = new_state;
			
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

//////////////////////////////////////////////////////Command type 4
//////////////////////////////////////////////////////Command type 5
													//to get State Recovery Type
bool ae_get_state_recovery_type(uint8_t device_id) {
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 1;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 5;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0)
	{
		// printf("get_state_recovery_typee - Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			//printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// device_status.state_recovery = rcvd_pd.data[0];
			//free(pd.data);
			//free(rcvd_pd.data);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

//////////////////////////////////////////////////////Command type 6
													//to Set State Recovery Type
bool ae_set_state_recovery_type(uint8_t device_id,uint8_t Rcovery)
{
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 6;
	pd.data[1] = Rcovery;
	struct protocolData rcvd_pd;

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0) {
		// printf("11 Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			//printf("22 Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);

			//This Delay Is Necessary to Match 400ms delay on AtMega
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}


////////////////////////////////////////////////////////Command type 7   Not in New Basic


////////////////////////////////////////////////////////Command type 8
													//to Set Switch Type 
bool ae_set_switch_type(int device_id,int portNumber,int switch_type,int tap_no,int scene_id,int scene_address)
{
	printf("ae_set_switch_type\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 6;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 8;
	pd.data[1] = portNumber;
	pd.data[2] = switch_type;
	pd.data[3] = tap_no;
	pd.data[4] = scene_address;
	pd.data[5] = scene_id;
	struct protocolData rcvd_pd;

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			//printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			return true;

		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}


////////////////////////////////////////////////////////Command type 10
													//to get the information of device
bool uc_get_device_info(int device_id) {
	printf("uc_get_device_info\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 1;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 10;
	struct protocolData rcvd_pd;

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 2) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 && rcvd_pd.length == 22 && rcvd_pd.rc_code == 1)
		{
			//printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			printf("device_type = %d \n",rcvd_pd.data[16]);
			printf("uc_firmware_version = %d \n",rcvd_pd.data[18]);
			int ret = set_device_info(device_id,rcvd_pd.data[16],rcvd_pd.data[18]);

			// device_status.device_type = rcvd_pd.data[16];
			// device_status.uc_firmware_version = rcvd_pd.data[18];
			return(ret);
		}
	}
	return(false);

}



////////////////////////////////////////////////////////Command type 18
													//to store the scene
bool ae_scene_store(int device_id,uint8_t scene_number, uint8_t scene_type,uint8_t *load_state) {
	printf("ae_scene_store\n");
	int switchCount = 4;//device_status.basic[device_id].no_of_switch;
	if(device_id != 255){
		switchCount = device_status.basic[device_id].no_of_switch;
	}
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = switchCount+3;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 18;
	pd.data[1] = scene_number;
	pd.data[2] = scene_type;
	for(int i=0;i<switchCount;i++)
	{
		pd.data[i+3] = load_state[i];
	}
	struct protocolData rcvd_pd;
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			return true;
		}
	}
	return false;

}
////////////////////////////////////////////////////////Command type 18
													//to store the scene dali
bool ae_scene_store_dali(int device_id,uint8_t addr,uint8_t grpup_addr, uint8_t gear_type, uint8_t color_type,uint8_t scene_id, uint8_t scene_type,uint8_t dali_scene_id, uint8_t arc_level,uint16_t color_temp) {
	printf("ae_scene_store\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 14;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 18;
	pd.data[1] = addr;
	pd.data[2] = grpup_addr;
	pd.data[3] = scene_id;
	pd.data[4] = scene_type;
	pd.data[5] = dali_scene_id;
	pd.data[6] = gear_type;
	pd.data[7] = color_type;
	pd.data[8] = arc_level;

	pd.data[9] = (color_temp >> 0) & 0x00FF; // heigh byte
	pd.data[10] = (color_temp >> 8) & 0x00FF;

	pd.data[11] = 0;
	pd.data[12] = 0;
	pd.data[13] = 0;

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 100) != 0) {
		//printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}
//////////////////////////////////////////////////////////Command type 19
													//to get the scene 
bool ae_scene_get(int device_id,uint8_t addr,uint8_t scene_id) {
	printf("ae_scene_get\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 3;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 19;
	pd.data[1] = addr;
	pd.data[2] = scene_id;


	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

//////////////////////////////////////////////////////Command type 21
													//change the arc level of port
bool ae_change_states_timer(uint8_t device_id,uint8_t port, uint8_t new_state,int timer) {
	// printf("aeChangeStates\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 5;
	pd.device_id = device_id;
	pd.id = 0;
	// //pd.data = malloc(pd.length);
	pd.data[0] = 3;
	pd.data[1] = port;
	pd.data[2] = new_state;

	pd.data[3] = (timer >> 8) & 0xFF;
	pd.data[4] = (timer >> 0) & 0xFF;

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// t_control_gear[port-1].arc_level = new_state;
			if(device_id < 33 && device_status.basic[device_id].device_type == 5)
			{
				set_arc_level(device_id,port-1,new_state);
			}
			else if(device_id == 33)
			{
				set_arc_level(device_id,port,new_state);
			}
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

////////////////////////////////////////////////////////Command type 23
													//to set dimming Strategy
bool ae_set_dimming_strategy(int device_id,int port, int strategy)
{
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 3;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 23;
	pd.data[1] = port;
	pd.data[2] = strategy;
	struct protocolData rcvd_pd;

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0) {
		//printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			//printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			return true;
		}
	}
	return false;
}

////////////////////////////////////////////////////////Command type 24
													//to set fade time
bool ae_fade_time_set(int device_id,uint8_t port,uint8_t port_type, uint8_t fade_time) {
	printf("ae_fade_time_set\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 24;
	pd.data[1] = port;
	pd.data[2] = port_type;
	pd.data[3] = fade_time;

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		//printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

////////////////////////////////////////////////////////Command type 24
													//to set fade time for dali
bool ae_fade_time_set_dali(int device_id,uint8_t addr,uint8_t fade_time, uint8_t fade_rate) {
	printf("ae_fade_time_set\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 24;
	pd.data[1] = addr;
	pd.data[2] = fade_time;
	pd.data[3] = fade_rate;

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		//printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}



////////////////////////////////////////////////////////Command type 31
													//scene_execuate
bool 	ae_scene_execuate(int device_id,uint8_t addr, uint8_t scene_id) {
	printf("ae_scene_execuate\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 3;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 31;
	pd.data[1] = addr;
	pd.data[2] = scene_id;

	struct protocolData rcvd_pd;
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 0) != 0) {
		printf("ae_scene_execuate - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ae_scene_execuate - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			sync_state_enable_call(1);
			return true;
		}
	}
	else if(device_id == 255)
	{
		return true;
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}


////////////////////////////////////////////////////////Command type 32
													//get color_temp of single port
int ae_get_color_temp(int device_id,uint8_t addr) {
	// printf("ae_get_color_temp\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	// //pd.data = malloc(pd.length);
	pd.data[0] = 32;
	pd.data[1] = addr;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0) {
		// printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 && rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[1]);
			// printf("data1 = %d \n",rcvd_pd.data[2]);
			int addr_count = addr;
			if(device_id == 33)
			{
				
				uint16_t color_temp = 0;
				for(int i=0;i<rcvd_pd.length;i=i+2)
				{
					color_temp  = ((uint8_t)rcvd_pd.data[i]) << 0;
					color_temp += ((uint8_t)rcvd_pd.data[i+1]) << 8;

					set_color_temp(device_id,addr_count,color_temp);

					addr_count++;

				}
			}
			
			
			// color_temp   = ((uint8_t)rcvd_pd.data[1]) << 8;
			// color_temp  += ((uint8_t)rcvd_pd.data[2]) << 0;


			return addr_count;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;

}


////////////////////////////////////////////////////////Command type 33
													//to changr the color temp
bool ae_set_color_temp(int device_id,uint8_t port, uint16_t color_temp) {
	printf("ae_set_color_temp\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 33;

	pd.data[1] = port;

	
	pd.data[2] = ( color_temp >> 0) & 0x00FF;
	pd.data[3] = (color_temp >> 8) & 0x00FF; // heigh byte

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("ae_set_color_temp - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ae_set_color_temp - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			if(port > 63)
			{
				get_all_arc_level(device_status.dali[0].no_of_control_gear);
				get_all_color_temp(device_status.dali[0].no_of_control_gear);
			}else
			{
				set_color_temp(device_id,port,color_temp);
			}
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}


////////////////////////////////////////////////////////Command type 34
													//to remove scene 
bool ae_scene_remove(uint8_t device_id,uint8_t addr,uint8_t scene_id) {
	printf("ae_scene_remove\n");
	struct protocolData pd;
	// if(device_id == 33)
	// {
		pd.rc_code = 0;
		pd.length = 3;
		pd.device_id = device_id;
		pd.id = 0;
		//pd.data = malloc(pd.length);
		pd.data[0] = 34;
		pd.data[1] = addr;
		pd.data[2] = scene_id;
	// }
	// else if(device_id < 33)
	// {
	// 	pd.rc_code = 0;
	// 	pd.length = device_status.basic[device_id].no_of_switch+3;
	// 	pd.device_id = device_id;
	// 	pd.id = 0;
	// 	pd.data[0] = 18;
	// 	pd.data[1] = scene_id;
	// 	pd.data[2] = 0;
	// 	for(int i=0;i<device_status.basic[device_id].no_of_switch;i++)
	// 	{
	// 		pd.data[i+3] = 255;
	// 	}
	// }
	

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		//printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}


////////////////////////////////////////////////////////Command type 35
													//to set group address
bool ae_group_addr_set(int device_id,uint8_t addr,uint8_t group, uint8_t active) {
	printf("ae_group_addr_set\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 35;
	pd.data[1] = addr;
	pd.data[2] = group;
	pd.data[3] = active;

	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);
	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		//printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("ChangeState - ID : %d, Len : %d, RC : %d\n", rcvd_pd.id, rcvd_pd.length, rcvd_pd.rc_code);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;

}

////////////////////////////////////////////////////////Command type 37
													//to add new control_gear in dali bus
bool ae_control_gear_add(int device_id,uint8_t present_control_gear_number)
{
	// printf("ae_control_gear_add\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 37;
	pd.data[1] = present_control_gear_number;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			//free(pd.data);
			//free(rcvd_pd.data);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}

////////////////////////////////////////////////////////Command type 38
													//to add new control_gear in dali bus
int ae_get_control_gear_status(int device_id,uint64_t *control_gear_status)
{
	// printf("ae_get_control_gear_status\n");
	int8_t total_device = 0;
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 38;
	pd.data[1] = 1;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 10) != 0) {
		// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 && rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			// printf("data1 = %d \n",rcvd_pd.data[1]);
			// printf("data2 = %d \n",rcvd_pd.data[2]);
			// printf("data3 = %d \n",rcvd_pd.data[3]);
			// printf("data4 = %d \n",rcvd_pd.data[4]);
			// printf("data5 = %d \n",rcvd_pd.data[5]);
			// printf("data6 = %d \n",rcvd_pd.data[6]);
			// printf("data7 = %d \n",rcvd_pd.data[7]);
			// printf("data8 = %d \n",rcvd_pd.data[8]);

			total_device  =  rcvd_pd.data[0];

			// uint64_t data  = ((uint8_t)rcvd_pd.data[2]) << 56;
			// 		data  += ((uint8_t)rcvd_pd.data[3]) << 48;
			// 		data  += ((uint8_t)rcvd_pd.data[4]) << 40;
			// 		data  += ((uint8_t)rcvd_pd.data[5]) << 32;
			// 		data  += ((uint8_t)rcvd_pd.data[6]) << 24;
			// 		data  += ((uint8_t)rcvd_pd.data[7]) << 16;
			// 		data  += ((uint8_t)rcvd_pd.data[8]) << 8;
			// 		data  += ((uint8_t)rcvd_pd.data[9]) << 0;

			uint64_t data  = ((uint8_t)rcvd_pd.data[1]) << 0;
					data  += ((uint8_t)rcvd_pd.data[2]) << 8;
					data  += ((uint8_t)rcvd_pd.data[3]) << 16;
					data  += ((uint8_t)rcvd_pd.data[4]) << 24;
					data  += ((uint8_t)rcvd_pd.data[5]) << 32;
					data  += ((uint8_t)rcvd_pd.data[6]) << 40;
					data  += ((uint8_t)rcvd_pd.data[7]) << 48;
					data  += ((uint8_t)rcvd_pd.data[8]) << 56;
			

			*control_gear_status = data;

			// printf("total_device = %d device_status = %" PRIu64 " \n",total_device,data);


			//free(pd.data);
			//free(rcvd_pd.data);
			return total_device;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;
}


////////////////////////////////////////////////////////Command type 39
													//to new control_gear scerch in dali bus
bool ae_new_control_gear_scerch(int device_id)
{
	printf("ae_new_control_gear_scerch\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 39;
	pd.data[1] = 1;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 200) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			printf("data0 = %d \n",rcvd_pd.data[0]);
			//free(pd.data);
			//free(rcvd_pd.data);
			ESP_ERROR_CHECK(esp_timer_start_once(dali_long_command_timer, 10000000));
			dali_long_command_receiving = true;
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}

////////////////////////////////////////////////////////Command type 40
													//to reset control_gear in dali bus
bool ae_reset_control_gear(int device_id,uint8_t addr,uint8_t rst_type)
{
	printf("ae_reset_control_gear\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 40;
	pd.data[1] = addr;
	pd.data[2] = rst_type;
	pd.data[3] = 1;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			printf("data0 = %d \n",rcvd_pd.data[0]);
			//free(pd.data);
			//free(rcvd_pd.data);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}

// //////////////////////////////////////////////////////Command type 41
// 													to reset control_gear in dali bus
bool ae_update_control_gear_info_dali(int device_id)
{
	printf("ae_update_control_gear_info\n");
	
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 1;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 41;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			printf("data0 = %d \n",rcvd_pd.data[0]);
			ESP_ERROR_CHECK(esp_timer_start_once(dali_long_command_timer, 10000000));
			dali_long_command_receiving = true;
			//free(pd.data);
			//free(rcvd_pd.data);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}

////////////////////////////////////////////////////////Command type 42
													//to reset control_gear in dali bus
bool ae_add_control_gear_using_long_addr(int device_id,uint8_t addr,uint32_t longaddr)
{
	printf("ae_blink_control_gear_using_long_addr\n");
	printf("long addr - %"PRIu32"\n",longaddr);
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 5;
	pd.device_id = device_id;
	pd.id = 0;
	pd.data[0] = 42;
	pd.data[1] = addr;
	pd.data[2] = ( longaddr >> 0) & 0x000000FF;
	pd.data[3] = (longaddr >> 8) & 0x000000FF; // heigh byte
	pd.data[4] = (longaddr >> 16) & 0x000000FF; // heigh byte

	printf("data0 = %d \n",pd.data[2]);
	printf("data1 = %d \n",pd.data[3]);
	printf("data2 = %d \n",pd.data[4]);


	struct protocolData rcvd_pd;

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			printf("data0 = %d \n",rcvd_pd.data[0]);
			//free(pd.data);
			//free(rcvd_pd.data);
			return true;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return false;
}


////////////////////////////////////////////////////////Command type 43
													//to reset control_gear in dali bus
int ae_dali_raw_command(int device_id,uint8_t type,uint8_t command,uint8_t data)
{
	// printf("ae_reset_control_gear\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 4;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 43;
	pd.data[1] = type;
	pd.data[2] = command;
	pd.data[3] = data;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			uint8_t ret = rcvd_pd.data[0];
			//free(pd.data);
			//free(rcvd_pd.data);
			return ret;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;
}

////////////////////////////////////////////////////////Command type 44
													//to dali step up down
int ae_dali_step_up_down(int device_id,uint8_t addr,uint8_t up_down,uint8_t color_type,uint8_t step_count)
{
	// printf("ae_reset_control_gear\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 5;
	pd.device_id = device_id;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 44;
	pd.data[1] = addr;
	pd.data[2] = up_down;
	pd.data[3] = color_type;
	pd.data[4] = step_count;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			uint8_t ret = rcvd_pd.data[0];
			//free(pd.data);
			//free(rcvd_pd.data);
			return ret;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;
}
////////////////////////////////////////////////////////Command type 45
													//to start dali sync state
int ae_dali_start_sync_state(int device_id)
{
	// printf("ae_reset_control_gear\n");
	struct protocolData pd;
	pd.rc_code = 0;
	pd.length = 2;
	pd.id = 0;
	//pd.data = malloc(pd.length);
	pd.data[0] = 45;
	pd.device_id = device_id;
	pd.data[1] = 1;
	struct protocolData rcvd_pd;
	//rcvd_pd.data = (char *)calloc(1,UARTProDataLength+1);

	if(SendAndReceiveUartData(&pd, &rcvd_pd, 50) != 0) {
		// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
		if(rcvd_pd.id == 0 &&  rcvd_pd.rc_code == 1)
		{
			// printf("Length = %d, ID = %d, RC Code = %d\n",rcvd_pd.length,rcvd_pd.id,rcvd_pd.rc_code);
			// printf("data0 = %d \n",rcvd_pd.data[0]);
			uint8_t ret = rcvd_pd.data[0];
			//free(pd.data);
			//free(rcvd_pd.data);
			return ret;
		}
	}
	//free(rcvd_pd.data);
	//free(pd.data);
	return -1;
}







void INTupdateState(struct protocolData *rcvdUratData)
{
	if(rcvdUratData->id == 1)
	{
		uint8_t subDevice = rcvdUratData->data[1];
		uint8_t port = rcvdUratData->data[2];
		uint8_t state = rcvdUratData->data[3];
		printf("INTupdateState deviceId - %d port - %d state - %d",subDevice,port,state);
		if (subDevice > device_status.no_of_rs485_device) return;
		if (port > device_status.basic[subDevice].no_of_switch) return;

		if(device_status.basic[subDevice].switch_config[port-1].pulse_count > 0 && state == 0)
		{
			device_status.basic[subDevice].switch_config[port-1].pulse_count = 0;
		}


			
		// 	setState(port, state,false);
		// }
		// else {
		// 	vTaskDelay(30);
		// 	// aeAllStates_temp();
		// 	state  = 0;
		// }

		// CreateMessageAndBroadcast(port,state);
		addToChangeLog(subDevice, port,state,LOCAL_SOURCE_SWITCHBOARD,0);
	}
}

void INT_update_dali_power(struct protocolData *rcvdUratData)
{
	if(rcvdUratData->id == 1 && rcvdUratData->length == 3)
	{
		printf("INT_update_dali_power");
		uint8_t DevicePower = rcvdUratData->data[1];
		if (DevicePower == 0 || DevicePower == 1) return;
		device_status.dali[0].dali_power = DevicePower;
	}
}
void INTupdateError(struct protocolData *rcvdUratData)
{
	if(rcvdUratData->id == 1 && rcvdUratData->length == 2)
	{
		printf("INTupdateError");
		// uint8_t Error = rcvdUratData->data[1];
		// if(Error == 53)
		// 	deviceSetting.power = 0;
		// else if(Error == 51)
		// 	deviceSetting.error = 3;
	}
}

void INT_update_scene(struct protocolData *rcvdUratData)
{
	printf("INT_update_scene");
	if(rcvdUratData->id == 1)
	{
		uint8_t trigger_data[4];

		trigger_data[0] = rcvdUratData->data[1];
		trigger_data[1] = rcvdUratData->data[2];
		trigger_data[2] = rcvdUratData->data[3];
		if(rcvdUratData->length > 4)
		{
			trigger_data[3] = rcvdUratData->data[4]; 
		}
		else
		{
			trigger_data[3] = 1;
		}
		ESP_LOGE(TAG,"scene_id - %d addr - %d sw - %d",trigger_data[1],trigger_data[2],trigger_data[0]);
		// printf("scene_id - %d addr - %d sw - %d",trigger_data[1],trigger_data[2],trigger_data[0]);
		publish_switch_scene((uint8_t)trigger_data[2],(uint8_t)trigger_data[1],(uint8_t)trigger_data[0],(uint8_t)trigger_data[3]);
		add_to_scene_queue((uint8_t)trigger_data[1]);
		sync_state_enable_call(1);
		// scene_trigger_action(SCENE_SOURCE_SWITCH, (void *)trigger_data);
		

		//CreateMessageAndBroadcast(port, state);
		//addToChangeLog(port, state, LOCAL_SOURCE_SWITCHBOARD);
	}
}

void INT_update_new_long_addr(struct protocolData *rcvdUratData)
{
	printf("INT_update_new_long_addr");
	if(rcvdUratData->id == 1)
	{
		uint32_t data = 0;
		data   = ((uint8_t)rcvdUratData->data[2]) << 0;
		data  += ((uint8_t)rcvdUratData->data[3]) << 8;
		data  += ((uint8_t)rcvdUratData->data[4]) << 16;

		printf("device long addr = %" PRIu32 " \n",data);
		publish_control_gear_long_addr(rcvdUratData->data[1],data);
	}
}

void INT_update_new_dali_addr(struct protocolData *rcvdUratData)
{
	// printf("INT_update_new_dali_addr");
	// printf("info_type = %d \n",rcvdUratData->data[1]);
	// printf("addr = %d \n",rcvdUratData->data[2]);
	// printf("device_type = %d \n",rcvdUratData->data[3]);
	// printf("color_type = %d \n",rcvdUratData->data[4]);
	// printf("status = %d \n",rcvdUratData->data[5]);
	// printf("group addr = %d \n",rcvdUratData->data[6]);
	// printf("group addr = %d \n",rcvdUratData->data[7]);
	// printf("fade time = %d \n",rcvdUratData->data[8]);
	// printf("fade rate = %d \n",rcvdUratData->data[9]);
	// printf("phy min = %d \n",rcvdUratData->data[10]);
	// printf("max level = %d \n",rcvdUratData->data[11]);
	// printf("min level = %d \n",rcvdUratData->data[12]);
	if(rcvdUratData->id == 1)
	{
		control_gear_details_t control_gear_details;
		control_gear_details.info_type = rcvdUratData->data[1];
		control_gear_details.addr = rcvdUratData->data[2];
		control_gear_details.device_type = rcvdUratData->data[3];
		control_gear_details.color_type = rcvdUratData->data[4];
		if(control_gear_details.info_type == 1)
		{
			dali_long_command_receiving = false;
		}

		
		publish_control_gear_detail(control_gear_details);

	}
}

void INT_update_intrupt(struct protocolData *rcvdUratData)
{
	printf("INT_update_intrupt");
	if(rcvdUratData->id == 1)
	{
		if(rcvdUratData->data[1] ==1)
		{
			// update_all_control_gear_state();
			// reportCommand();
		}
	}
}


void processUartInterrupt(struct protocolData *rcvdUratData) {
	switch(rcvdUratData->data[0])
	{
		case 4:
			INTupdateState(rcvdUratData);
			break;
		case 5:
			INT_update_dali_power(rcvdUratData);
			break;
		case 6:
			INTupdateError(rcvdUratData);
			break;
		case 31:
			INT_update_scene(rcvdUratData);
			break;
		case 9:
			INT_update_new_long_addr(rcvdUratData);
			break;
		case 10:
			INT_update_new_dali_addr(rcvdUratData);
			break;
		case 11:
			INT_update_intrupt(rcvdUratData);
			break;
		default:
			break;
	}
	return;
}


bool get_all_arc_level(uint8_t no_of_control_gear)
{
	if(no_of_control_gear == 0)
	{
		return false;
	}
	int temp_count = 0;
	do
	{
		int ret = ae_get_arc_level(33,temp_count);
		if (ret < 0)
		{
			return false;
		}
		temp_count = temp_count + ret;
	} while (temp_count < no_of_control_gear);
	return true;
}


bool get_all_color_temp(uint8_t no_of_control_gear)
{
	if(no_of_control_gear == 0)
	{
		return false;
	}
	int temp_count = 0;
	do
	{
		int ret = ae_get_color_temp(33,temp_count);
		if (ret < 0)
		{
			return false;
		}
		temp_count = temp_count + ret;
	} while (temp_count < no_of_control_gear);
	return true;
}


uint8_t sync_state_enable = 0;
uint8_t sync_state_current_count = 0;

bool sync_state_all_device()
{
	if(sync_state_enable == 0)
	{
		return false;
	}
	// ESP_LOGI(TAG, "sync_state_all_device current_count - %d , %d",sync_state_current_count,device_status.no_of_rs485_device);
	if(sync_state_current_count < device_status.no_of_rs485_device)
	{
		if(device_status.basic[sync_state_current_count].active == true)
		{
			// ESP_LOGI(TAG, "device_id for sunc - %d",sync_state_current_count);
			int64_t start_time = esp_timer_get_time();
			ae_get_arc_level(sync_state_current_count,0);
			int64_t req_time = esp_timer_get_time() - start_time;
			device_status.basic[sync_state_current_count].serial_update_time = req_time;
			printf("req_time - %" PRIu64 "\n",device_status.basic[sync_state_current_count].serial_update_time);
		}
	}
	else if (device_status.dali[0].active == true && sync_state_current_count == device_status.no_of_rs485_device)
	{
		int64_t start_time = esp_timer_get_time();
		get_all_arc_level(device_status.dali[0].no_of_control_gear);
		get_all_color_temp(device_status.dali[0].no_of_control_gear);
		device_status.update_dali = true;
		device_status.update = true;
		sync_state_enable = false;
		sync_state_current_count = 0;
		int64_t req_time = esp_timer_get_time() - start_time;
		device_status.dali[0].serial_update_time = req_time;

	}
	else
	{
		device_status.update = true;
		sync_state_enable = false;
		sync_state_current_count = 0;
	}
	
	sync_state_current_count++;
	return true;
}

void sync_state_enable_call(uint8_t state)
{
	printf("sync_state_enable_call - %d\n",state);
	sync_state_enable = state;
	sync_state_current_count = 0;
}




uint8_t get_device_on_bus_enable = 0;
uint8_t get_device_on_bus_current_count = 0;

bool update_device_detal(uint8_t infoType,uint8_t divice_id)
{
	if(get_device_on_bus_enable != 2)
	{
		return 0;
	}
	if(divice_id < 33)
	{
		cJSON *devices = cJSON_CreateObject();
		cJSON_AddItemToObject(devices, "infoType",cJSON_CreateNumber(infoType));
		cJSON_AddItemToObject(devices, "deviceType",cJSON_CreateNumber(device_status.basic[divice_id].device_type));
		cJSON_AddItemToObject(devices, "subDeviceId",cJSON_CreateNumber(divice_id));
		cJSON_AddItemToObject(devices, "noOfPort",cJSON_CreateNumber(device_status.basic[divice_id].no_of_switch));
		cJSON_AddItemToObject(devices, "ucFV",cJSON_CreateNumber(device_status.basic[divice_id].uc_firmware_version));

		char *temp = cJSON_Print(devices);
		cJSON_Delete(devices);
		int leng = strlen(temp);
		printf("length - %d\n",leng);
		int ret = 0;
		if(leng > 2)
			ret = http_post_device_status(temp);
		free(temp);
	}
	else if(divice_id == 33)
	{
		cJSON *devices = cJSON_CreateObject();
		cJSON_AddItemToObject(devices, "infoType",cJSON_CreateNumber(infoType));
		cJSON_AddItemToObject(devices, "deviceType",cJSON_CreateNumber(device_status.dali[0].device_type));
		cJSON_AddItemToObject(devices, "subDeviceId",cJSON_CreateNumber(divice_id));
		cJSON_AddItemToObject(devices, "ucFV",cJSON_CreateNumber(device_status.dali[0].uc_firmware_version));

		char *temp = cJSON_Print(devices);
		cJSON_Delete(devices);
		int leng = strlen(temp);
		printf("length - %d\n",leng);
		int ret = 0;
		if(leng > 2)
			ret = http_post_device_status(temp);
		free(temp);
	}
	return true;
	
}
bool get_device_on_bus(uint8_t rs485_device_id)
{
	printf("device id - %d\n",get_device_on_bus_current_count);
	if(get_device_on_bus_current_count < 33)
	{
		bool ret = uc_get_device_info(get_device_on_bus_current_count);
		if (ret == false)
		{
			device_status.basic[get_device_on_bus_current_count].active = false;
			return 0;
		}
		printf("uc_get_device_info ret - %d\n",ret);
		device_status.no_of_rs485_device = get_device_on_bus_current_count + 1;
		aeQuery(get_device_on_bus_current_count);
		update_device_detal(1,rs485_device_id);
		ae_get_arc_level(rs485_device_id,0);
		device_status.basic[rs485_device_id].update = true;
	}
	return true;
}
bool get_dali_on_bus(uint8_t rs485_dali_id)
{
	bool ret = uc_get_device_info(rs485_dali_id);
	if (ret == false)
	{
		device_status.dali[0].active = false;
		return 0;
	}
	aeQuery(rs485_dali_id);
	update_device_detal(1,rs485_dali_id);
	int no_of_control_gear = ae_get_control_gear_status(rs485_dali_id,&device_status.dali[0].control_gear_status);
	printf("ret from ae_get_control_gear_status - %d\n",no_of_control_gear);
	if (no_of_control_gear < 0 || no_of_control_gear > DALI_NUMBER_OF_MAX_CONTROL_GEAR)
	{
		return false;
	}
	device_status.dali[0].no_of_control_gear = no_of_control_gear;
	printf("active status- %" PRIu64 "\n",device_status.dali[0].control_gear_status);

	for(int i=0;i<device_status.dali[0].no_of_control_gear;i++)
	{
		// printf("loop  %d , status - %d\n",i,(uint8_t)((device_status.control_gear_status >> i) & 1));
		if(((device_status.dali[0].control_gear_status >> i) & 1) == 1)
		{
			// printf("control gear active %d\n",i);
			device_status.dali[0].control_gear[i].active = true;
		}
	}
	get_all_arc_level(device_status.dali[0].no_of_control_gear);
	get_all_color_temp(device_status.dali[0].no_of_control_gear);
	device_status.dali[0].update = true;
	
	return true;
}




bool get_all_device_on_bus()
{
	if(get_device_on_bus_enable <= 0)
	{
		return false;
	}
	printf("get_all_device_on_bus device id - %d\n",get_device_on_bus_current_count);
	if(get_device_on_bus_current_count < 33)
	{
		get_device_on_bus(get_device_on_bus_current_count);
	}
	else if(get_device_on_bus_current_count == 33)
	{
		get_dali_on_bus(33);
	}
	else
	{
		update_device_detal(0,0); // end of sync device on rs485 bus
		get_device_on_bus_enable = 0;
		get_device_on_bus_current_count = 0;
		sync_state_enable_call(1);
		return true;

	}
	
	get_device_on_bus_current_count++;
	return true;
	
}

void get_device_on_bus_enable_call(uint8_t state)
{
	printf("get_device_on_bus_enable_call - %d\n",state);
	get_device_on_bus_enable = state;
	get_device_on_bus_current_count = 0;
}




void dali_long_command_timer_cb(void *args) {
	//If IP Is Not Obtained Till Timeout, Restart the Connection Processs
	dali_long_command_receiving = false;
	printf("dali_long_command_timer_cb dali_long_command_receivingaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");

	return;
}



bool initaeProtocol()
{
	esp_timer_create_args_t dali_long_command_timer_args = {
		.callback = dali_long_command_timer_cb,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "WiFi STA No IP"
	};
	esp_timer_create(&dali_long_command_timer_args, &dali_long_command_timer);


	memset(&device_status, 0, sizeof(device_status_t));
	get_all_device_on_bus();
// 	struct protocolData ret_data;
// 	uint8_t ret = aeQuery(&ret_data);



// 	uint64_t temp_control_gear_status = 0;

// 	int64_t start_time = esp_timer_get_time();

// 	int no_of_control_gear = ae_get_control_gear_status(&device_status.control_gear_status);
// 	// printf("ret from ae_get_control_gear_status - %d\n",no_of_control_gear);
// 	if (no_of_control_gear < 0 || no_of_control_gear > NUMBER_OF_MAX_CONTROL_GEAR)
// 	{
// 		return false;
// 	}
// 	device_status.no_of_control_gear = no_of_control_gear;
// 	// printf("active status- %" PRIu64 "\n",device_status.control_gear_status);

// 	for(int i=0;i<device_status.no_of_control_gear;i++)
// 	{
// 		// printf("loop  %d , status - %d\n",i,(uint8_t)((device_status.control_gear_status >> i) & 1));
// 		if(((device_status.control_gear_status >> i) & 1) == 1)
// 		{
// 			// printf("control gear active %d\n",i);
// 			control_gear[i].active = true;
// 		}
// 	}
// 	update_all_control_gear_state();

// 	// printf("time required  -  %" PRIu64 "\n",(esp_timer_get_time() - start_time)/(int64_t)1000000);


return true;
}