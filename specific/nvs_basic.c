#include "nvs_basic.h"
#include "nvs_common.h"
#include "nvs_core.h"

//static const char *TAG = "nvs_basic";

//Get Scene Entry From Flash
int8_t nvs_get_scene_entry(char *scene_name, char *entry_key, void *entry_value, uint16_t *entry_size) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (entry_value != NULL) {
		pairs[pair_count].key = entry_key;
		pairs[pair_count].value = entry_value;
		pairs[pair_count].length = 0;
		pair_count++;
	}

	int8_t ret = nvs_get_pairs(NVS_PART_SCENE, scene_name, NVS_TYPE_BLOB, pair_count, pairs);
	if (ret != ESP_OK) {
		return ret;
	}

	*entry_size = pairs[0].length;
	return ESP_OK;
}

//Store Scene Entry In Flash
int8_t nvs_set_scene_entry(char *scene_name, char *entry_key, void *entry_value, uint16_t *entry_size) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (entry_value != NULL) {
		pairs[pair_count].key = entry_key;
		pairs[pair_count].value = entry_value;
		pairs[pair_count].length = *entry_size;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SCENE, scene_name, NVS_TYPE_BLOB, pair_count, pairs));
}

//Get Scene Status From Flash
int8_t nvs_get_scene_status(char *scene_name, uint8_t *scene_status) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (scene_status != NULL) {
		pairs[pair_count].key = scene_name;
		pairs[pair_count].value = (void *)scene_status;
		pair_count++;
	}

	return (nvs_get_pairs(NVS_PART_SCENE, NVS_NAMESPACE_SCENE_STAT, NVS_TYPE_U8, pair_count, pairs));
}

//Set Scene Status From Flash
int8_t nvs_set_scene_status(char *scene_name, uint8_t *scene_status) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (scene_status != NULL) {
		pairs[pair_count].key = scene_name;
		pairs[pair_count].value = (void *)scene_status;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SCENE, NVS_NAMESPACE_SCENE_STAT, NVS_TYPE_U8, pair_count, pairs));
}

//Get Trigger Action From Flash
int8_t nvs_get_trigger_action(char *trigger_type, char *trigger_id, uint8_t *action_scene_index) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (action_scene_index != NULL) {
		pairs[pair_count].key = trigger_id;
		pairs[pair_count].value = (void *)action_scene_index;
		pair_count++;
	}

	return (nvs_get_pairs(NVS_PART_SCENE, trigger_type, NVS_TYPE_U8, pair_count, pairs));
}

//Set Trigger Action From Flash
int8_t nvs_set_trigger_action(char *trigger_type, char *trigger_id, uint8_t *action_scene_index) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (action_scene_index != NULL) {
		pairs[pair_count].key = trigger_id;
		pairs[pair_count].value = (void *)action_scene_index;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SCENE, trigger_type, NVS_TYPE_U8, pair_count, pairs));
}

void nvs_setup_basic(void) {
	//Initialize The Scenes NVS Partition
	nvs_init_part(NVS_PART_SCENE);
}