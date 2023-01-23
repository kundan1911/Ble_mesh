#ifndef PICO_NVS_BASIC_H
#define PICO_NVS_BASIC_H

#include<stdint.h>

#define NVS_PART_SCENE				"scene"				//Partition To Store Scenes

#define NVS_NAMESPACE_SCENE_STAT	"scene_stat"		//Namespace To Store Active/Inactive Status For Scenes
#define NVS_NAMESPACE_TRIG_SWITCH	"trig_switch"		//Namespace To Store Trigger Map For Switches
#define NVS_NAMESPACE_TRIG_MOTION	"trig_motion"		//Namespace To Store Trigger Map For Motion Sensors
#define NVS_NAMESPACE_TRIG_DOOR		"trig_door"			//Namespace To Store Trigger Map For Door Sensors
#define NVS_NAMESPACE_TRIG_TMPR		"trig_tmpr"			//Namespace To Store Trigger Map For Temperature Sensors
#define NVS_NAMESPACE_NODE_INFO     "node_info"

int8_t nvs_get_scene_entry(char *space_name, char *entry_key, \
						void *entry_value, uint16_t *entry_size);

int8_t nvs_set_scene_entry(char *scene_name, char *entry_key, \
						void *entry_value, uint16_t *entry_size);

int8_t nvs_get_scene_status(char *scene_name, uint8_t *scene_status);

int8_t nvs_set_scene_status(char *scene_name, uint8_t *scene_status);

int8_t nvs_get_trigger_action(char *trigger_type, char *trigger_id, uint8_t *action_scene_index);

int8_t nvs_set_trigger_action(char *trigger_type, char *trigger_id, uint8_t *action_scene_index);

void nvs_setup_basic(void);

#endif