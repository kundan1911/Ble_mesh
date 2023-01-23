#ifndef PICO_NVS_COMMON_H
#define PICO_NVS_COMMON_H

#include<stdint.h>

#define NVS_PART_SETTING "setting"					//Partition To Store A Backup Of Data From ATMega
#define NVS_PART_WIFI "wifi" 						//Partitioin To Store WiFi Credentials
#define NVS_PART_SYSTEM "system" 					//Partition To Store Device ID

#define NVS_KEY_WIFI_SSID "ssid" 					//Key For WiFi STA SSID
#define NVS_KEY_WIFI_PASSWORD "pass" 				//Key For WiFi STA Password

#define NVS_KEY_DEVICE_ID "dev_id"					//Key For Device ID (used in Server Login and AP SSID)
#define NVS_KEY_SERVER_PASSWORD "pass"				//Key For Server Login Password
#define NVS_KEY_UDP_PASSWORD "udp"					//Key For UDP Access Password

#define NVS_NAMESPACE_WIFI_CRED "wifi_cred"			//Namespace For WiFi Credentials in NVS_PART_WIFI Partition
#define NVS_NAMESPACE_DEVICE_CRED "dev_cred"		//Namespace For Device Credentials Parameters (ID, Server Password, UDP Password)

#define NVS_VAL_WIFI_SSID_LEN 32					//Length of WiFi STA SSID Value
#define NVS_VAL_WIFI_PASSWORD_LEN 64				//Length of WiFi STA Password Value

#define NVS_VAL_DEVICE_ID_LEN 17					//Length of Server Login Username length
#define NVS_VAL_SERVER_PASSWORD_LEN 33				//Length of Server Login Password Length
#define NVS_VAL_UDP_PASSWORD_LEN 33					//Length of UDP Password Value

int8_t nvs_get_sta_credentials(char *sta_ssid, char *sta_pass);

int8_t nvs_set_sta_credentials(char *sta_ssid, char *sta_pass);

int8_t nvs_get_dev_id(char *dev_id);

int8_t nvs_set_dev_id(char *dev_id);

int8_t nvs_get_srv_password(char *srv_pass);

int8_t nvs_set_srv_password(char *srv_pass);

int8_t nvs_get_udp_password(char *udp_pass);

int8_t nvs_set_udp_password(char *udp_pass);

void nvs_setup_common(void);

#endif