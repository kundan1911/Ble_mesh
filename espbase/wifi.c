#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "wifi.h"
#include "error_manager.h"
#include "nvs_common.h"
#include "pico_main.h"

#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "lwip/ip4_addr.h"
#include "freertos/semphr.h"
#include "tcpip_adapter.h"

#define AP_PASSWORD "picostone123"

#define WIFI_STA_IP_OBTAIN_TIMEOUT 60000000
#define WIFI_AP_VISIBLE_TIMEOUT 15000000

#define AP_DEVICE_COUNT_MAX 4

static const char *TAG = "wifi";

esp_timer_handle_t wifi_sta_noip_timer, wifi_ap_visible_timer;

wifi_status_t wifi_status;
wifi_ap_record_t wifi_connected_ap_info;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *wifi_event)
{
	system_event_info_t *info = &wifi_event->event_info;
	wifi_config_t wifi_cfg;
	switch(wifi_event->event_id)
	{
		case SYSTEM_EVENT_STA_START:
			ESP_LOGI(TAG, "STA Started");
			esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N);
			esp_wifi_connect();
			wifi_status_set(STA_START_FLAG, true);
			break;

		case SYSTEM_EVENT_STA_STOP:
			wifi_status_set(STA_ALL_FLAGS, false);
			break;

		case SYSTEM_EVENT_STA_CONNECTED:
			ESP_LOGI(TAG, "Connected to WiFi");

			ESP_ERROR_CHECK(esp_timer_start_once(wifi_sta_noip_timer, WIFI_STA_IP_OBTAIN_TIMEOUT));
			wifi_status_set(STA_CONNECTED_FLAG, true);
			break;

		case SYSTEM_EVENT_STA_DISCONNECTED:
			ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);

			esp_wifi_disconnect();

			//If Disconnected, Stop The Timer, If It Is Running
			esp_timer_stop(wifi_sta_noip_timer);

			wifi_status_set((STA_CONNECTED_FLAG | STA_GOT_IP_FLAG), false);
			wifi_status_set(STA_DISCONNECT_REASON, info->disconnected.reason);

			//Make AP SSID Visible If Hidden
			esp_wifi_get_config(ESP_IF_WIFI_AP, &wifi_cfg);
			if (wifi_cfg.ap.ssid_hidden==true || wifi_cfg.ap.max_connection==0) {
				wifi_ap_configure(true, NULL);
			}

			esp_wifi_connect();

			break;

		case SYSTEM_EVENT_STA_GOT_IP:
			ESP_LOGI(TAG, "Got IP");
			wifi_status_set(STA_GOT_IP_FLAG, true);

			//Once IP Address Is Obtained, Stop No IP Timer If Running
			esp_timer_stop(wifi_sta_noip_timer);

			//Start Timer To Switch Off Hotspot After Timer Expiry
			ESP_ERROR_CHECK(esp_timer_start_once(wifi_ap_visible_timer, WIFI_AP_VISIBLE_TIMEOUT));

			uint8_t retry;
			for (retry = 0; retry<3; retry++) {
				esp_wifi_sta_get_ap_info(&wifi_connected_ap_info);
				if (wifi_connected_ap_info.rssi > WIFI_STA_RSSI_THRESHOLD) {
					break;
				}
			}

			if (retry == 3) {
				wifi_status_set(STA_DISCONNECT_REASON, WIFI_REASON_LOW_RSSI);
			}
			else {
				wifi_status_set(STA_DISCONNECT_REASON, 0);
			}

			break;

		case SYSTEM_EVENT_STA_LOST_IP:
			ESP_LOGI(TAG, "Lost IP");
			wifi_status_set(STA_GOT_IP_FLAG, false);
			wifi_status_set(STA_DISCONNECT_REASON, WIFI_REASON_CONNECTED_NO_IP);

			if (wifi_status_get(STA_CONNECTED_FLAG) == true) {
				ESP_ERROR_CHECK(esp_timer_start_once(wifi_sta_noip_timer, WIFI_STA_IP_OBTAIN_TIMEOUT));

				//Make AP SSID Visible If Hidden
				esp_wifi_get_config(ESP_IF_WIFI_AP, &wifi_cfg);
				if (wifi_cfg.ap.ssid_hidden==true || wifi_cfg.ap.max_connection==0) {
					wifi_ap_configure(true, NULL);
				}
			}

			break;

		case SYSTEM_EVENT_AP_START:
			ESP_LOGI(TAG, "AP Started");
			esp_wifi_set_protocol(ESP_IF_WIFI_AP, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N);
			wifi_status_set(AP_START_FLAG, true);
			break;

		case SYSTEM_EVENT_AP_STOP:
			wifi_status_set(AP_START_FLAG, false);
			break;

		case SYSTEM_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG, "Station Connected");
			wifi_status_set(AP_DEVICE_COUNT_INC, 1);
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(TAG, "Station Disconnected");
			wifi_status_set(AP_DEVICE_COUNT_DEC, 1);

			if (wifi_sta_get_connected_status()==true && wifi_status_get(AP_DEVICE_COUNT_ABS)==0) {
				wifi_ap_configure(false, NULL);
			}

			break;

		default:
			break;

		}
		return ESP_OK;
}

void wifi_sta_noip_timer_cb(void *args) {
	//If IP Is Not Obtained Till Timeout, Restart the Connection Processs
	esp_wifi_disconnect();
	wifi_status_set((STA_CONNECTED_FLAG | STA_GOT_IP_FLAG), false);
	wifi_status_set(STA_DISCONNECT_REASON, WIFI_REASON_CONNECTED_NO_IP);
	vTaskDelay(50);

	esp_wifi_connect();
	return;
}

void wifi_ap_visible_timer_cb(void *args) {
	//On Timer, Make SoftAP SSID Hidden
	wifi_ap_configure(false, NULL);
	return;
}

void wifi_status_set(uint8_t wifi_status_type, uint8_t value) {
	if (wifi_status_get(wifi_status_type) == value) {
		return;
	}
	if (wifi_status_type == AP_DEVICE_COUNT_ABS) {
		wifi_status.ap_device_count = value;
	}
	else if (wifi_status_type == AP_DEVICE_COUNT_INC) {
		wifi_status.ap_device_count += value;
	}
	else if (wifi_status_type == AP_DEVICE_COUNT_DEC) {
		if (wifi_status.ap_device_count > value) {
			wifi_status.ap_device_count -= value;
		}
		else {
			wifi_status.ap_device_count = 0;
		}
	}
	else if (wifi_status_type == STA_DISCONNECT_REASON) {
		wifi_status.sta_disconnect_reason = value;
		if (value == 0) {
			setErrorCode(ERROR_WIFI_CONNECTION, false);
		}
		else if (value != WIFI_REASON_DISCONNECTED_MANUALLY) {
			setErrorCode(ERROR_WIFI_CONNECTION, true);
		}
	}
	else if (wifi_status_type < AP_DEVICE_COUNT_ABS) {
		if (value == true) {
			wifi_status.flags |= wifi_status_type;
		}
		else {
			wifi_status.flags &= (~wifi_status_type);
		}
	}
	return;
}

uint8_t wifi_status_get(uint8_t wifi_status_type) {
	//If Number of Stations Connected To SoftAP is Requested
	if (wifi_status_type == AP_DEVICE_COUNT_ABS) {
		return wifi_status.ap_device_count;
	}

	//Else if STA Disconnected Reason Is Requested
	else if (wifi_status_type == STA_DISCONNECT_REASON) {
		return wifi_status.sta_disconnect_reason;
	}

	//Else if Status of One or Multiple Flag Bits is Requested
	//Note : Multiple Bits can be Requested By Using Bitwise OR while Calling This Function
	else if (wifi_status_type < AP_DEVICE_COUNT_ABS) {
		return ((wifi_status.flags & wifi_status_type) == wifi_status_type);
	}

	return 0;
}

bool wifi_sta_get_connected_status(void) {
	return wifi_status_get(STA_CONNECTED_FLAG | STA_GOT_IP_FLAG);
}

//Configure Soft AP Interface
uint8_t wifi_ap_configure(bool show_ssid, char *info) {
	wifi_ap_config_t ap_cfg = {
		.password = AP_PASSWORD,
		.ssid_hidden = !show_ssid,
		.authmode = WIFI_AUTH_WPA_WPA2_PSK
	};

	memset(ap_cfg.ssid, 0, 32);
	wifi_ap_ssid_append(ap_cfg.ssid, "PicoStone ");

	if (!show_ssid) {
		wifi_ap_ssid_append(ap_cfg.ssid, "w");
		tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
		ap_cfg.max_connection = 0;
	}
	else {
		if (info == NULL) {
			ap_cfg.max_connection = AP_DEVICE_COUNT_MAX;
			tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
		}
		else {
			wifi_ap_ssid_append(ap_cfg.ssid, info);
			wifi_ap_ssid_append(ap_cfg.ssid, " ");
			tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
			ap_cfg.max_connection = 0;
		}
	}
	wifi_ap_ssid_append(ap_cfg.ssid, DEVICE_ID);

	ap_cfg.ssid_len = strlen((char*)ap_cfg.ssid);
	wifi_config_t wifi_cfg = {.ap = ap_cfg};
	return(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_cfg));
}

//Configure Station Interface
uint8_t wifi_sta_configure(char *wifi_sta_ssid, char *wifi_sta_password, bool wifi_sta_cfg_set_to_nvs) {
	uint8_t len;
	wifi_sta_config_t sta_cfg = {
		.scan_method = WIFI_ALL_CHANNEL_SCAN,
		.bssid_set = 0,
		.channel = 0,
		.listen_interval = 0,
		.sort_method = WIFI_CONNECT_AP_BY_SIGNAL
	};
	len = strlen(wifi_sta_ssid);
	memcpy(sta_cfg.ssid, wifi_sta_ssid, len);
	sta_cfg.ssid[len] = '\0';
	len = strlen(wifi_sta_password);
	memcpy(sta_cfg.password, wifi_sta_password, len);
	sta_cfg.password[len] = '\0';

	wifi_config_t wifi_cfg = {.sta = sta_cfg};
	esp_err_t wifi_sta_cfg_set_err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg);
	if (wifi_sta_cfg_set_err != ESP_OK) {
		return wifi_sta_cfg_set_err;
	}

	//If Set To NVS Parameter Is True, Store The Credentials In NVs As Well
	if (wifi_sta_cfg_set_to_nvs == true) {
		return(nvs_set_sta_credentials(wifi_sta_ssid, wifi_sta_password));
	}

	return(ESP_OK);
}

//Retrieve IP Address Assigned to the STA Interface
void wifi_sta_get_ip(char *wifi_sta_ip_str) {
	tcpip_adapter_ip_info_t wifi_sta_ip_info;
	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &wifi_sta_ip_info);
	sprintf(wifi_sta_ip_str, IPSTR, IP2STR(&wifi_sta_ip_info.ip));
	return;
}

//Get MAC Address of STA Interface
void wifi_sta_get_mac(char *wifi_sta_mac_str) {
	uint8_t wifi_sta_mac_addr[6];
	esp_efuse_mac_get_default(wifi_sta_mac_addr);
	sprintf(wifi_sta_mac_str, MACSTR, MAC2STR(wifi_sta_mac_addr));
	return;
}

//Append parameter 2 to parameter 1 for SoftAP SSID
void wifi_ap_ssid_append(uint8_t* ap_ssid_current, char *ap_ssid_to_add) {
	memcpy(ap_ssid_current + strlen((char*)ap_ssid_current), ap_ssid_to_add, strlen(ap_ssid_to_add));
	return;
}

//Main WiFi Function
void wifi_setup(void) {

	esp_timer_create_args_t wifi_sta_noip_timer_args = {
		.callback = wifi_sta_noip_timer_cb,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "WiFi STA No IP"
	};
	esp_timer_create(&wifi_sta_noip_timer_args, &wifi_sta_noip_timer);

	esp_timer_create_args_t wifi_ap_visible_timer_args = {
		.callback = wifi_ap_visible_timer_cb,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "WiFi SoftAP ON"
	};
	esp_timer_create(&wifi_ap_visible_timer_args, &wifi_ap_visible_timer);

	tcpip_adapter_init();
	//Initialize WiFi Event Handler
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));

	//Initialize WiFi Init Config with Default Values
	wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));

	//Store WiFi Parameters In Flash.
	//Sself Stored NVS Values Will Be Used As Backup
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

	//Set WiFi Mode to STA+AP
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

	wifi_config_t wifi_cfg;
	char wifi_sta_ssid[32], wifi_sta_password[64];
	memset(wifi_sta_ssid,0,32);
	memset(wifi_sta_password,0,64);
	memset(wifi_cfg.sta.ssid,0,32);
	memset(wifi_cfg.sta.password,0,64);

	//Get WiFi Config Stored in NVS by API
	esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg);

	//If Unable to Retrieve BackUp, Store Existing Details in BackUp
	if (nvs_get_sta_credentials(wifi_sta_ssid, wifi_sta_password) != ESP_OK) {
		ESP_ERROR_CHECK(wifi_sta_configure((char *)wifi_cfg.sta.ssid, (char *)wifi_cfg.sta.password, true));
	}
	//If BackUp Retrieved Successfully
	else {
		//If BackUp Matches with Config, Set Current Config
		if (strcmp((char *)wifi_cfg.sta.ssid, wifi_sta_ssid)==0 && strcmp((char *)wifi_cfg.sta.password, wifi_sta_password)==0) {
			ESP_LOGI(TAG, "Config Matches With BackUp");
			ESP_ERROR_CHECK(wifi_sta_configure((char *)wifi_cfg.sta.ssid, (char *)wifi_cfg.sta.password, false));
		}
		//If BackUp Is Different From Current Config
		else {
			//If BackUp Config Is Empty, Save Current Config To BackUp
			if (wifi_sta_ssid[0]=='\0' || wifi_sta_password[0]=='\0') {
				ESP_LOGE(TAG, "BackUp Invalid");
				ESP_LOGE(TAG, "Resetting BackUp to Current Config");
				ESP_ERROR_CHECK(wifi_sta_configure((char *)wifi_cfg.sta.ssid, (char *)wifi_cfg.sta.password, true));
			}
			//If BackUp Config Is Not Empty, Copy BackUp To Current Config
			else {
				ESP_LOGE(TAG, "Possible Data Corruption");
				ESP_LOGI(TAG, "Setting WiFi Credentials From Backup");
				ESP_ERROR_CHECK(wifi_sta_configure(wifi_sta_ssid, wifi_sta_password, false));
			}
		}
	}

	/*
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	tcpip_adapter_ip_info_t wifi_ap_ip_config;
	IP4_ADDR(&(wifi_ap_ip_config.ip), 172, 168, 4, 1);
	IP4_ADDR(&(wifi_ap_ip_config.netmask), 255, 255, 255, 0);
	IP4_ADDR(&(wifi_ap_ip_config.gw), 172, 168, 4, 1);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &wifi_ap_ip_config));
	tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP);
	*/

	ESP_ERROR_CHECK(wifi_ap_configure(true, NULL));
	ESP_ERROR_CHECK(esp_wifi_start());
	return;
}