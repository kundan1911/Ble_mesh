#include "nvs.h"
#include<stdint.h>

// #define NVS_KEY_LENGTH 16
#define NVS_PART_SYSTEM "store"
#define NVS_KEY_DEVICE_ID "dev_id"					//Key For Device ID (used in Server to represent it's index)
#define NVS_NAMESPACE_DEVICE_CRED "dev_id_ns"		//Namespace For Device ID
#define NVS_TAG "nvs_comment"


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

typedef struct {
	char *key;
	void* value;
}nvs_key_value_pair_t;


int8_t nvs_set_dev_id(int8_t* dev_id);

int8_t nvs_get_dev_id(int8_t* dev_id);

void nvs_setup(void);