#ifndef PICO_NVS_CORE_H
#define PICO_NVS_CORE_H

#include "nvs.h"

#include <stdint.h>
#include <stddef.h>

#define ERR_NVS_LOCK_FAILED				-1
#define ERR_NVS_PAIR_COUNT_ZERO			-2
#define ERR_NVS_PART_OPEN_FAILED		-3
#define ERR_NVS_KEY_ERASE_FAILED		-4
#define ERR_NVS_KEY_GET_FAILED			-5
#define ERR_NVS_KEY_SET_FAILED			-6
#define ERR_NVS_VALUE_TYPE_INVALID		-7
#define ERR_NVS_COMMIT_FAILED			-8
#define ERR_NVS_KEYS_RETRIEVE_FAILED	-9
#define ERR_NVS_ERASE_SPACE_FAILED		-10

#define NVS_KEY_LENGTH 16

typedef struct {
	char *key;
	void* value;
	size_t length;
}nvs_key_value_pair_t;

typedef char (*nvs_space_keys_t)[NVS_KEY_LENGTH];

void nvs_init_part(char *nvs_part_name);

int8_t nvs_get_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, \
						uint8_t pair_count, nvs_key_value_pair_t* pairs);

int8_t nvs_set_pairs(char *nvs_part_name, char *nvs_namespace, nvs_type_t nvs_value_type, \
						uint8_t pair_count, nvs_key_value_pair_t* pairs);

int8_t nvs_get_namespace_keys(char *nvs_part_name, char *nvs_space_name, \
						nvs_space_keys_t *nvs_keys, size_t *nvs_entries_count);

int8_t nvs_erase_namespace(char *nvs_part_name, char *nvs_space_name);

#endif