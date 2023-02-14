#include "nvs_header.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"


xSemaphoreHandle nvs_mutex = NULL;

//Release The Lock On NVS Core API
void nvs_core_release(void) {
	xSemaphoreGive(nvs_mutex);
}


#define nvs_exit(exit_code) \
	do { \
		nvs_core_release();	\
		return exit_code; \
	} while(0)


//Init The Lock On NVS Core API 
bool nvs_core_lock(void) {
	if (nvs_mutex == NULL) {
		nvs_mutex = xSemaphoreCreateMutex();
	}
	if (xSemaphoreTake(nvs_mutex, 100) != pdTRUE) {
		return false;
	}
	return true;
}

//Initialize The Specified NVS Partition
void nvs_init_part(char *nvs_part_name){
	esp_err_t ret = nvs_flash_init_partition(nvs_part_name);
if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase_partition(nvs_part_name));
		ret = nvs_flash_init_partition(nvs_part_name);
	}

	ESP_ERROR_CHECK(ret);
    return ;
}


int8_t nvs_get_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, \
uint8_t pair_count, nvs_key_value_pair_t* pairs)
{	
    if (nvs_core_lock() == false) {
		ESP_LOGI(NVS_TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	if (pair_count == 0) {
		ESP_LOGI(NVS_TAG, "Pair Count For %s is Zero", nvs_part_name);
		nvs_exit(ERR_NVS_PAIR_COUNT_ZERO);
	}
    nvs_handle nvs_get_pair_handle;
if (nvs_open_from_partition(nvs_part_name, nvs_namespace, NVS_READONLY, &nvs_get_pair_handle)) {
		ESP_LOGI(NVS_TAG, "Failed To Open Namespace %s From Partition %s", nvs_namespace, nvs_part_name);
		nvs_close(nvs_get_pair_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}
    for (int i = 0; i<pair_count; i++) {
		esp_err_t ret;
		switch (nvs_value_type) {
			case NVS_TYPE_I8 :
				// //If Length Is Set To Zero, Then Calculate Automatically
				// if (pairs[i].length == 0) {
				// 	nvs_get_i8(nvs_get_pair_handle, pairs[i].key, NULL);
				// }
				ret = nvs_get_i8(nvs_get_pair_handle, pairs[i].key, (int8_t *)(pairs[i].value));
				break;
			default :
				ESP_LOGI(NVS_TAG, "Value Type For %s Is Invalid : 0x%02X", pairs[i].key, nvs_value_type);
				nvs_exit(ERR_NVS_VALUE_TYPE_INVALID);
		}
		if (ret) {
			ESP_LOGI(NVS_TAG, "Get %s Failed", pairs[i].key);
			nvs_close(nvs_get_pair_handle);
			nvs_exit(ERR_NVS_KEY_GET_FAILED);
		}
	}
    nvs_close(nvs_get_pair_handle);
	ESP_LOGI(NVS_TAG, "Get From %s Successful", nvs_part_name);
	nvs_exit(ESP_OK);
}

int8_t nvs_set_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, \
						uint8_t pair_count, nvs_key_value_pair_t* pairs)
{
if (nvs_core_lock() == false) {
		ESP_LOGI(NVS_TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	if (pair_count == 0) {
		ESP_LOGI(NVS_TAG, "Pair Count For %s is Zero", nvs_part_name);
		nvs_exit(ERR_NVS_PAIR_COUNT_ZERO);
	}
	nvs_handle nvs_set_pair_handle;
	if (nvs_open_from_partition(nvs_part_name, nvs_namespace, NVS_READWRITE, &nvs_set_pair_handle)) {
		ESP_LOGI(NVS_TAG, "Failed To Open Namespace %s From Partition %s", nvs_namespace, nvs_part_name);
		nvs_close(nvs_set_pair_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}
	for (int i = 0; i<pair_count; i++) {
		esp_err_t ret = nvs_erase_key(nvs_set_pair_handle, pairs[i].key);
		if (ret!=ESP_OK && ret!=ESP_ERR_NVS_NOT_FOUND) {
			ESP_LOGI(NVS_TAG, "Erase %s Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_KEY_ERASE_FAILED);
		}
		if (ret == ESP_ERR_NVS_NOT_FOUND) {
			ESP_LOGI(NVS_TAG, "Erase Failed. Key Not Found");
		}
		switch (nvs_value_type) {
			case NVS_TYPE_I8 :
				ret = nvs_set_i8(nvs_set_pair_handle, pairs[i].key,  *(int *)pairs[i].value);
				break;
			default :
				ESP_LOGI(NVS_TAG, "Value Type For %s Is Invalid : 0x%02X", pairs[i].key, nvs_value_type);
				nvs_exit(ERR_NVS_VALUE_TYPE_INVALID);
		}
		if (ret) {
			ESP_LOGI(NVS_TAG, "Set %s Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_KEY_SET_FAILED);
		}
		if (nvs_commit(nvs_set_pair_handle)) {
			ESP_LOGI(NVS_TAG, "Commit After %s Set Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_COMMIT_FAILED);
		}
	}
	nvs_close(nvs_set_pair_handle);
	ESP_LOGI(NVS_TAG, "Set To %s Successful", nvs_part_name);
	nvs_exit(ESP_OK);
}


int8_t nvs_set_dev_id(int8_t* dev_id) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	// if (dev_id ) {
		pairs[pair_count].key = NVS_KEY_DEVICE_ID;
		pairs[pair_count].value =(int8_t *)dev_id;
		pair_count++;
	// }
	return nvs_set_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_I8, pair_count, pairs);
}

int8_t nvs_get_dev_id(int8_t* dev_id){
    // nvs_key_value_pair_t pairs[1];
	// uint8_t pair_count = 0;
	// if (dev_id != NULL) {
	// 	pairs[pair_count].key = NVS_KEY_DEVICE_ID;
	// 	pairs[pair_count].value = (void *)dev_id;
	// 	pair_count++;
	// }
	nvs_handle_t nvs_get_pair_handle;
    nvs_open_from_partition(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_READONLY, &nvs_get_pair_handle);
    return nvs_get_i8(nvs_get_pair_handle, NVS_KEY_DEVICE_ID, dev_id);
	// return nvs_get_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_I8, pair_count, pairs);
}






void nvs_setup(){
	ESP_LOGI(NVS_TAG,"nvs setup started");
    nvs_init_part(NVS_PART_SYSTEM);
    return ;
}