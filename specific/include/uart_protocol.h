#ifndef PICO_UART_PROTOCOL_H
#define PICO_UART_PROTOCOL_H

#include<stdint.h>
#include<stdbool.h>



#define NUMBER_OF_MAX_RS485_DEVICE 32
#define NUMBER_OF_MAX_RS485_DEVICE_DALI 1


#define DALI_NUMBER_OF_MAX_CONTROL_GEAR 64
#define NUMBER_OF_MAX_SWITCH_INPUT 6





typedef enum  {
	UNICOLOR = 0,
	TEMPERATURE,
	RGB,
	UNSUPPORTED
}color_type_t;

typedef struct
{
	color_type_t color_type;
	bool active;
	uint8_t arc_level;
	uint16_t color_temp;
	uint8_t R_level;
	uint8_t G_level;
	uint8_t B_level;
}dali_control_gear_t;

typedef struct
{
	
	uint8_t type;
	uint8_t update;
	uint8_t tap_count;
}dali_switch_status_t;



typedef struct
{
	bool active;
	uint8_t device_type;
	uint8_t error;
	uint8_t dali_power;
	uint8_t no_of_control_gear;
	uint8_t no_of_switch;
	uint64_t control_gear_status;
	uint8_t update;
	uint8_t uc_firmware_version;
	int64_t serial_update_time;
	dali_switch_status_t switch_config[NUMBER_OF_MAX_SWITCH_INPUT];
	dali_control_gear_t control_gear[DALI_NUMBER_OF_MAX_CONTROL_GEAR];
}dali_t;


typedef struct
{
	uint8_t arc_level;
	uint16_t color_temp;

	uint8_t pulse_count;
	int64_t pulse_time_us;
	int64_t pulse_start_time;
	int64_t change_pulse_time;
}basic_port_status_t;

typedef struct
{
	uint8_t device_type;
	bool active;
	int64_t serial_update_time;
	uint8_t no_of_switch;
	uint8_t update;
	uint8_t uc_firmware_version;
	basic_port_status_t switch_config[NUMBER_OF_MAX_SWITCH_INPUT];
}basic_t;

typedef struct
{
	uint8_t device_type;
	uint8_t uc_firmware_version;
	uint8_t error;
	uint8_t update;
	uint8_t update_dali;

	uint8_t no_of_rs485_device;
	dali_t dali[2];
	basic_t basic[33];

}device_status_t;


device_status_t device_status;



typedef struct
{
	uint8_t info_type;
	uint8_t addr;
	uint8_t device_type;
	color_type_t color_type;

}control_gear_details_t;



bool aeQuery(uint8_t device_id);         //Command type 1
int ae_get_arc_level(uint8_t device_id,uint8_t addr);  //Command type 2
bool ae_change_states(uint8_t device_id,uint8_t port, uint8_t new_state);   //Command type 3
//Command type 4
bool ae_get_state_recovery_type(uint8_t device_id);  //Command type 5
bool ae_set_state_recovery_type(uint8_t device_id,uint8_t Rcovery);  //Command type 6
//Command type 7
bool ae_set_switch_type(int device_id,int portNumber,int switch_type,int scene_type,int scene_id,int scene_address);  //Command type 8
bool uc_get_device_info(int device_id);  //Command type 10
bool ae_scene_store(int device_id,uint8_t scene_number, uint8_t scene_type,uint8_t *load_state);   //Command type 18
bool ae_scene_store_dali(int device_id,uint8_t addr,uint8_t grpup_addr, uint8_t gear_type, uint8_t color_type,uint8_t scene_id, uint8_t scene_type,uint8_t dali_scene_id, uint8_t arc_level,uint16_t color_temp);  //Command type 18
bool ae_scene_get(int device_id,uint8_t addr,uint8_t scene_id);  //Command type 19
bool ae_change_states_timer(uint8_t device_id,uint8_t port, uint8_t new_state,int timer);  //Command type 21
bool ae_set_dimming_strategy(int device_id,int port, int strategy);  //Command type 23
bool ae_fade_time_set_dali(int device_id,uint8_t addr,uint8_t fade_time, uint8_t fade_rate);  //Command type 24
bool ae_fade_time_set(int device_id,uint8_t port,uint8_t port_type, uint8_t fade_time);   //Command type 24
bool ae_scene_execuate(int device_id,uint8_t addr, uint8_t scene_id);  //Command type 31
int ae_get_color_temp(int device_id,uint8_t addr);  //Command type 32
bool ae_set_color_temp(int device_id,uint8_t port, uint16_t color_temp);  //Command type 33
bool ae_scene_remove(uint8_t device_id,uint8_t addr,uint8_t scene_id);  //Command type 34
bool ae_group_addr_set(int device_id,uint8_t addr,uint8_t group, uint8_t active);  //Command type 35
bool ae_control_gear_add(int device_id,uint8_t present_control_gear_number);  //Command type 37
int ae_get_control_gear_status(int device_id,uint64_t *control_gear_status);  //Command type 38
bool ae_new_control_gear_scerch(int device_id);  //Command type 39
bool ae_reset_control_gear(int device_id,uint8_t addr,uint8_t rst_type);  //Command type 40
bool ae_update_control_gear_info_dali(int device_id);  //Command type 41
bool ae_add_control_gear_using_long_addr(int device_id,uint8_t addr,uint32_t longaddr);  //Command type 42
int ae_dali_raw_command(int device_id,uint8_t type,uint8_t command,uint8_t data);  //Command type 43
int ae_dali_step_up_down(int device_id,uint8_t addr,uint8_t up_down,uint8_t color_type,uint8_t step_count);  //Command type 44
int ae_dali_start_sync_state(int device_id);  //Command type 45


bool initaeProtocol();
bool sync_state_all_device();
bool get_all_device_on_bus();

void get_device_on_bus_enable_call(uint8_t state);
void sync_state_enable_call(uint8_t state);
bool ae_change_states_pulse(uint8_t device_id,uint8_t port, uint8_t new_state);

bool get_all_color_temp(uint8_t no_of_control_gear);
bool get_all_arc_level(uint8_t no_of_control_gear);




#endif /* INCLUDE_AEPROTOCOL_H_ */
