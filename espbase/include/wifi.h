#ifndef PICO_WIFI_H
#define PICO_WIFI_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_wifi_types.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_STA_RSSI_THRESHOLD -75

#define STA_START_FLAG 		0b00000001
#define STA_CONNECTED_FLAG	0b00000010
#define STA_GOT_IP_FLAG		0b00000100
#define AP_START_FLAG		0b00001000

#define AP_DEVICE_COUNT_ABS 0b00100000
#define AP_DEVICE_COUNT_INC 0b01000000
#define AP_DEVICE_COUNT_DEC 0b01100000

#define STA_DISCONNECT_REASON 0b10000000

#define STA_ALL_FLAGS (STA_START_FLAG | STA_CONNECTED_FLAG | STA_GOT_IP_FLAG)

#define WIFI_REASON_LOW_RSSI 253
#define WIFI_REASON_DISCONNECTED_MANUALLY 254
#define WIFI_REASON_CONNECTED_NO_IP 255

typedef struct {
	uint8_t flags:5;
	uint8_t ap_device_count:3;
	uint8_t sta_disconnect_reason;
}wifi_status_t;

extern wifi_ap_record_t wifi_connected_ap_info;

void wifi_sta_noip_timer_cb(void *args);

uint8_t wifi_ap_configure(bool show_ssid, char *info);

uint8_t wifi_sta_configure(char *wifi_sta_ssid, char *wifi_sta_password, bool wifi_sta_cfg_set_to_nvs);

void wifi_status_set(uint8_t wifi_status_type, uint8_t value);

uint8_t wifi_status_get(uint8_t wifi_status_type);

bool wifi_sta_get_connected_status(void);

void wifi_sta_get_ip(char *wifi_sta_ip_str);

void wifi_sta_get_mac(char *wifi_sta_mac_str);

void wifi_ap_ssid_append(uint8_t* ap_ssid_current, char *ap_ssid_to_add);

void wifi_setup(void);

#endif