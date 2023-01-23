#include<string.h>

#include "nvs_core.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs_flash.h"

#define nvs_exit(exit_code) \
	do { \
		nvs_core_release();	\
		return exit_code; \
	} while(0)

static const char *TAG = "nvs_core";

xSemaphoreHandle nvs_mutex = NULL;

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

//Release The Lock On NVS Core API
void nvs_core_release(void) {
	xSemaphoreGive(nvs_mutex);
}

//Initialize The Specified NVS Partition
void nvs_init_part(char *nvs_part_name) {
	esp_err_t ret = nvs_flash_init_partition(nvs_part_name);
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase_partition(nvs_part_name));
		ret = nvs_flash_init_partition(nvs_part_name);
	}
	ESP_ERROR_CHECK(ret);
}

//Get NVS Key Value Pairs From The Specified Partition
int8_t nvs_get_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, uint8_t pair_count, nvs_key_value_pair_t* pairs) {
	if (nvs_core_lock() == false) {
		ESP_LOGE(TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	if (pair_count == 0) {
		ESP_LOGE(TAG, "Pair Count For %s is Zero", nvs_part_name);
		nvs_exit(ERR_NVS_PAIR_COUNT_ZERO);
	}
	nvs_handle nvs_get_pair_handle;
	if (nvs_open_from_partition(nvs_part_name, nvs_namespace, NVS_READONLY, &nvs_get_pair_handle)) {
		ESP_LOGE(TAG, "Failed To Open Namespace %s From Partition %s", nvs_namespace, nvs_part_name);
		nvs_close(nvs_get_pair_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}
	for (int i = 0; i<pair_count; i++) {
		esp_err_t ret;
		switch (nvs_value_type) {
			case NVS_TYPE_STR :
				//If Length Is Set To Zero, Then Calculate Automatically
				if (pairs[i].length == 0) {
					nvs_get_str(nvs_get_pair_handle, pairs[i].key, NULL, &(pairs[i].length));
				}
				ret = nvs_get_str(nvs_get_pair_handle, pairs[i].key, (char *)(pairs[i].value), &(pairs[i].length));
				break;
			case NVS_TYPE_BLOB :
				//If Length Is Set To Zero, Then Calculate Automatically And Allocate Memory
				if (pairs[i].length == 0) {
					nvs_get_blob(nvs_get_pair_handle, pairs[i].key, NULL, &(pairs[i].length));
				}
				ret = nvs_get_blob(nvs_get_pair_handle, pairs[i].key, (void *)pairs[i].value, &(pairs[i].length));
				break;
			case NVS_TYPE_U8:
				ret = nvs_get_u8(nvs_get_pair_handle, pairs[i].key, (uint8_t *)(pairs[i].value));
				break;
			default :
				ESP_LOGE(TAG, "Value Type For %s Is Invalid : 0x%02X", pairs[i].key, nvs_value_type);
				nvs_exit(ERR_NVS_VALUE_TYPE_INVALID);
		}
		if (ret) {
			ESP_LOGE(TAG, "Get %s Failed", pairs[i].key);
			nvs_close(nvs_get_pair_handle);
			nvs_exit(ERR_NVS_KEY_GET_FAILED);
		}
	}
	nvs_close(nvs_get_pair_handle);
	ESP_LOGD(TAG, "Get From %s Successful", nvs_part_name);
	nvs_exit(ESP_OK);
}

//Set NVS Key Value Pairs In The Specified Partition
int8_t nvs_set_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, uint8_t pair_count, nvs_key_value_pair_t* pairs) {
	if (nvs_core_lock() == false) {
		ESP_LOGE(TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	if (pair_count == 0) {
		ESP_LOGE(TAG, "Pair Count For %s is Zero", nvs_part_name);
		nvs_exit(ERR_NVS_PAIR_COUNT_ZERO);
	}
	nvs_handle nvs_set_pair_handle;
	if (nvs_open_from_partition(nvs_part_name, nvs_namespace, NVS_READWRITE, &nvs_set_pair_handle)) {
		ESP_LOGE(TAG, "Failed To Open Namespace %s From Partition %s", nvs_namespace, nvs_part_name);
		nvs_close(nvs_set_pair_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}
	for (int i = 0; i<pair_count; i++) {
		esp_err_t ret = nvs_erase_key(nvs_set_pair_handle, pairs[i].key);
		if (ret!=ESP_OK && ret!=ESP_ERR_NVS_NOT_FOUND) {
			ESP_LOGE(TAG, "Erase %s Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_KEY_ERASE_FAILED);
		}
		if (ret == ESP_ERR_NVS_NOT_FOUND) {
			ESP_LOGI(TAG, "Erase Failed. Key Not Found");
		}
		switch (nvs_value_type) {
			case NVS_TYPE_STR :
				ret = nvs_set_str(nvs_set_pair_handle, pairs[i].key, (char *)(pairs[i].value));
				break;
			case NVS_TYPE_BLOB :
				ret = nvs_set_blob(nvs_set_pair_handle, pairs[i].key, (void *)(pairs[i].value), pairs[i].length);
				break;
			case NVS_TYPE_U8:
				ret = nvs_set_u8(nvs_set_pair_handle, pairs[i].key, *((uint8_t *)(pairs[i].value)));
				break;
			default :
				ESP_LOGE(TAG, "Value Type For %s Is Invalid : 0x%02X", pairs[i].key, nvs_value_type);
				nvs_exit(ERR_NVS_VALUE_TYPE_INVALID);
		}
		if (ret) {
			ESP_LOGE(TAG, "Set %s Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_KEY_SET_FAILED);
		}
		if (nvs_commit(nvs_set_pair_handle)) {
			ESP_LOGE(TAG, "Commit After %s Set Failed", pairs[i].key);
			nvs_close(nvs_set_pair_handle);
			nvs_exit(ERR_NVS_COMMIT_FAILED);
		}
	}
	nvs_close(nvs_set_pair_handle);
	ESP_LOGD(TAG, "Set To %s Successful", nvs_part_name);
	nvs_exit(ESP_OK);
}

//Function To Get List Of All Keys Stored In A Namespace.
//Pass NULL Pointer To Only Get Total Number Of Keys.
int8_t nvs_get_namespace_keys(char *nvs_part_name, char *nvs_space_name, \
				nvs_space_keys_t *nvs_keys, size_t *nvs_entries_count) {

	if (nvs_core_lock() == false) {
		ESP_LOGE(TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	nvs_handle nvs_get_keys_handle;

	esp_err_t ret = nvs_open_from_partition(nvs_part_name, nvs_space_name, NVS_READONLY, &nvs_get_keys_handle);

	*nvs_entries_count = 0;
	if (ret == ESP_OK) {
		//First Only Count Number Of Key Value Pairs
		//Do Not Use The Built-In Get Used Entries Function
		//Since One Pair May Span Over Multiple Entries Also
		nvs_iterator_t nvs_entries = nvs_entry_find(nvs_part_name, nvs_space_name, NVS_TYPE_ANY);
		while (nvs_entries != NULL) {
			nvs_entry_info_t nvs_entry;
			nvs_entry_info(nvs_entries, &nvs_entry);

			nvs_entries = nvs_entry_next(nvs_entries);
			*nvs_entries_count += 1;
		}

	}
	else if (ret == ESP_ERR_NVS_NOT_FOUND) {
		ESP_LOGW(TAG, "Namespace %s In Partition %s Not Found", nvs_space_name, nvs_part_name);
	}
	else {
		ESP_LOGE(TAG, "%s Open Failed", nvs_part_name);
		nvs_close(nvs_get_keys_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}

	if (nvs_keys != NULL) {
		*nvs_keys = (char (*)[NVS_KEY_LENGTH])malloc(sizeof(**nvs_keys) * (*nvs_entries_count));
		nvs_iterator_t nvs_entries = nvs_entry_find(nvs_part_name, nvs_space_name, NVS_TYPE_ANY);

		for (size_t i=0; i<(*nvs_entries_count); i++) {
			nvs_entry_info_t nvs_entry;
			nvs_entry_info(nvs_entries, &nvs_entry);
			strcpy((*nvs_keys)[i], nvs_entry.key);

			nvs_entries = nvs_entry_next(nvs_entries);
		}
	}

	nvs_close(nvs_get_keys_handle);
	ESP_LOGD(TAG, "Keys Retrieved From %s Namespace In %s Partition", nvs_space_name, nvs_part_name);
	nvs_exit(ESP_OK);
}

int8_t nvs_erase_namespace(char *nvs_part_name, char *nvs_space_name) {
	if (nvs_core_lock() == false) {
		ESP_LOGE(TAG, "Unable To Get Lock");
		nvs_exit(ERR_NVS_LOCK_FAILED);
	}
	
	nvs_handle nvs_space_erase_handle;
	if (nvs_open_from_partition(nvs_part_name, nvs_space_name, NVS_READWRITE, &nvs_space_erase_handle)) {
		ESP_LOGE(TAG, "%s Open Failed", nvs_part_name);
		nvs_close(nvs_space_erase_handle);
		nvs_exit(ERR_NVS_PART_OPEN_FAILED);
	}

	if (nvs_erase_all(nvs_space_erase_handle)) {
		ESP_LOGE(TAG, "Erasing Failed For Namespace %s In Partition %s", nvs_space_name, nvs_part_name);
		nvs_close(nvs_space_erase_handle);
		nvs_exit(ERR_NVS_ERASE_SPACE_FAILED);
	}

	if (nvs_commit(nvs_space_erase_handle)) {
		ESP_LOGE(TAG, "Commit After Namespace %s Erase Failed", nvs_space_name);
		nvs_close(nvs_space_erase_handle);
		nvs_exit(ERR_NVS_COMMIT_FAILED);
	}

	nvs_close(nvs_space_erase_handle);
	ESP_LOGD(TAG, "Namespace %s In Partition %s Erased Successfully", nvs_space_name, nvs_part_name);
	nvs_exit(ESP_OK);
}