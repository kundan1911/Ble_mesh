#include "nvs_common.h"
#include "nvs_core.h"

#include "nvs.h"

//Get WiFi STA Credentials From Flash
int8_t nvs_get_sta_credentials(char *sta_ssid, char *sta_pass) {
	nvs_key_value_pair_t pairs[2];
	uint8_t pair_count = 0;

	if (sta_ssid != NULL) {
		pairs[pair_count].key = NVS_KEY_WIFI_SSID;
		pairs[pair_count].value = (void *)sta_ssid;
		pairs[pair_count].length = NVS_VAL_WIFI_SSID_LEN;
		pair_count++;
	}

	if (sta_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_WIFI_PASSWORD;
		pairs[pair_count].value = (void *)sta_pass;
		pairs[pair_count].length = NVS_VAL_WIFI_PASSWORD_LEN;
		pair_count++;
	}

	return (nvs_get_pairs(NVS_PART_WIFI, NVS_NAMESPACE_WIFI_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Store WiFi STA Credentials In Flash
int8_t nvs_set_sta_credentials(char *sta_ssid, char *sta_pass) {
	nvs_key_value_pair_t pairs[2];
	uint8_t pair_count = 0;

	if (sta_ssid != NULL) {
		pairs[pair_count].key = NVS_KEY_WIFI_SSID;
		pairs[pair_count].value = (void *)sta_ssid;
		pair_count++;
	}

	if (sta_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_WIFI_PASSWORD;
		pairs[pair_count].value = (void *)sta_pass;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_WIFI, NVS_NAMESPACE_WIFI_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Get Device ID From Flash
int8_t nvs_get_dev_id(char *dev_id) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (dev_id != NULL) {
		pairs[pair_count].key = NVS_KEY_DEVICE_ID;
		pairs[pair_count].value = (void *)dev_id;
		pairs[pair_count].length = NVS_VAL_DEVICE_ID_LEN;
		pair_count++;
	}
printf("before get dev id");
	return (nvs_get_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Store Device ID In Flash
int8_t nvs_set_dev_id(char *dev_id) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (dev_id != NULL) {
		pairs[pair_count].key = NVS_KEY_DEVICE_ID;
		pairs[pair_count].value = (void *)dev_id;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Get Server Password From Flash
int8_t nvs_get_srv_password(char *srv_pass) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (srv_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_SERVER_PASSWORD;
		pairs[pair_count].value = (void *)srv_pass;
		pairs[pair_count].length = NVS_VAL_SERVER_PASSWORD_LEN;
		pair_count++;
	}

	return (nvs_get_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Store Server Password In Flash
int8_t nvs_set_srv_password(char *srv_pass) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (srv_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_SERVER_PASSWORD;
		pairs[pair_count].value = (void *)srv_pass;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Get UDP Password From Flash
int8_t nvs_get_udp_password(char *udp_pass) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (udp_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_UDP_PASSWORD;
		pairs[pair_count].value = (void *)udp_pass;
		pairs[pair_count].length = NVS_VAL_UDP_PASSWORD_LEN;
		pair_count++;
	}

	return (nvs_get_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

//Store UDP Password In Flash
int8_t nvs_set_udp_password(char *udp_pass) {
	nvs_key_value_pair_t pairs[1];
	uint8_t pair_count = 0;

	if (udp_pass != NULL) {
		pairs[pair_count].key = NVS_KEY_UDP_PASSWORD;
		pairs[pair_count].value = (void *)udp_pass;
		pair_count++;
	}

	return (nvs_set_pairs(NVS_PART_SYSTEM, NVS_NAMESPACE_DEVICE_CRED, NVS_TYPE_STR, pair_count, pairs));
}

void nvs_setup_common(void) {
	//Initialize The Default NVS Partition
	nvs_init_part(NVS_DEFAULT_PART_NAME);

	//Initialize The Settings NVS Partition
	nvs_init_part(NVS_PART_SETTING);

	//Initialize The WiFi Credentials NVS Partition
	nvs_init_part(NVS_PART_WIFI);

	//Initialize The System Info NVS Partition
	nvs_init_part(NVS_PART_SYSTEM);
}