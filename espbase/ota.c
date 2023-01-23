#include<stdint.h>

#include "ota.h"
#include "pico_main.h"
#include "wifi.h"

#include "commJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#define ERR_OTA_SOCKET_FAILED					-1
#define ERR_OTA_CONNECT_FAILED					-2
#define ERR_OTA_VERSION_REQUEST_FAILED			-3
#define ERR_OTA_VERSION_REQUEST_TIMEOUT			-4
#define ERR_OTA_VERSION_UNEXPECTED_RESPONSE		-5
#define ERR_OTA_VERSION_INVALID_RESPONSE		-6
#define ERR_OTA_VERSION_ALREADY_LATEST			-7
#define ERR_OTA_FILE_NAME_TOO_LONG				-8
#define ERR_OTA_FILE_REQUEST_FAILED				-9
#define ERR_OTA_FILE_EMPTY						-10
#define ERR_OTA_FIRMWARE_DOWNLOAD_FAILED		-11
#define ERR_OTA_FIRMWARE_DOWNLOAD_TIMEOUT		-12
#define ERR_OTA_FIRMWARE_WRITE_FAILED			-13
#define ERR_OTA_ACK_SEND_FAILED					-14
#define ERR_OTA_CHECKSUM_CHECK_FAILED			-15
#define ERR_OTA_APP_INVALID						-16
#define ERR_OTA_BOOT_SWITCH_FAILED				-17

#define OTA_VERSION_CHECK_SKELETON "{\"action\":\"is_update\",\"CURRENT_VERSION\":%s,\"DATE\":\"%s\",\"Time\":\"%s\",\"version_code\":\"%s\",\"deviceId\":\"%s\",\"file\":\"%s\"}"

#define OTA_SEND_BUF_SIZE						256
#define OTA_RECV_BUF_SIZE						256

#define OTA_WIFI_TIMEOUT					120000000	//120 Seconds Timeout To Wait For WiFi Connection
#define OTA_VERSION_CHECK_TIMEOUT			120000000	//120 Seconds Timeout To Wait For Version Check
#define OTA_DOWNLOAD_TIMEOUT				600000000	//600 Seconds Timeout To Wait For Download

#define JTOKEN ":,}"

static const char *TAG = "ota";

ota_parameters_t *ota_param;

bool ota_running = false;

int8_t ota_connect_to_server(int *sock_id, char *ip, int port) {
	*sock_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock_id == -1) {
		ESP_LOGE(TAG, "Socket Allocation Failed : %d", *sock_id);
		return ERR_OTA_SOCKET_FAILED;
	}

	struct sockaddr_in sock_info;
	memset(&sock_info, 0, sizeof(sock_info));
	sock_info.sin_len = sizeof(sock_info);
	sock_info.sin_family = AF_INET;
	sock_info.sin_addr.s_addr = inet_addr(ip);
	sock_info.sin_port = htons(port);

	if (connect(*sock_id, (struct sockaddr *)&sock_info, sizeof(sock_info))==-1) {
		ESP_LOGE(TAG, "Failed To Connect To Server");
		close(*sock_id);
		return ERR_OTA_CONNECT_FAILED;
	}

	ESP_LOGI(TAG, "Connected To Server");
	return ESP_OK;
}

int8_t ota_check_for_updates(int *sock_id, char *filename) {
	char ota_version_check_message[strlen(OTA_VERSION_CHECK_SKELETON) + strlen(ESP_FIRMWARE_VERSION) +
		strlen(ESP_FIRMWARE_BUILD_DATE) + strlen(ESP_FIRMWARE_BUILD_TIME) + strlen(ESP_FIRMWARE_CODENAME) +
		strlen(DEVICE_ID) + strlen(filename)];

	sprintf(ota_version_check_message, OTA_VERSION_CHECK_SKELETON, ESP_FIRMWARE_VERSION,
		ESP_FIRMWARE_BUILD_DATE, ESP_FIRMWARE_BUILD_TIME, ESP_FIRMWARE_CODENAME, DEVICE_ID, filename);

	if (send(*sock_id, ota_version_check_message, strlen(ota_version_check_message), 0)<0) {
		ESP_LOGE(TAG, "Request For Version Check Failed");
		close(*sock_id);
		return ERR_OTA_VERSION_REQUEST_FAILED;
	}

	uint16_t ota_recv_bytes = 0;
	uint8_t ota_recv_data[OTA_RECV_BUF_SIZE];
	int64_t update_check_start_time = esp_timer_get_time();
	while (esp_timer_get_time() - update_check_start_time < OTA_VERSION_CHECK_TIMEOUT) {
		ota_recv_bytes = recv(*sock_id, (void *)ota_recv_data, OTA_RECV_BUF_SIZE, 0);

		//ota_recv_data[0] & ota_recv_bytes Checks Are Important In Case
		//Some Other Server Is Running on The Specified Address and Port
		if (ota_recv_bytes > 0 && ota_recv_data[0]!=0 && ota_recv_bytes<=OTA_RECV_BUF_SIZE) {
			char *ota_version_check = getJSONvalue((char *)ota_recv_data,"isupdate",JTOKEN);

			if (strcmp(ota_version_check, "true") == 0) {
				ESP_LOGI(TAG, "Update Available");
				close(*sock_id);
				return ESP_OK;
			}
			else if(strcmp(ota_version_check,"false")==0) {
				ESP_LOGE(TAG, "Firmware Already In Latest Version");
				close(*sock_id);
				return ERR_OTA_VERSION_ALREADY_LATEST;
			}
			else {
				ESP_LOGE(TAG, "Unexpected Response");
				close(*sock_id);
				return ERR_OTA_VERSION_UNEXPECTED_RESPONSE;
			}
		}

		else {
			ESP_LOGE(TAG, "Invalid Response");
			close(*sock_id);
			return ERR_OTA_VERSION_INVALID_RESPONSE;
		}
	}
	ESP_LOGI(TAG, "Update Check Timed Out");
	close(*sock_id);
	return ERR_OTA_VERSION_REQUEST_TIMEOUT;
}

int8_t ota_download_firmware(int* sock_id, char *filename, esp_ota_handle_t handle) {
	if (strlen(filename) > OTA_SEND_BUF_SIZE) {
		ESP_LOGE(TAG, "Filename Too Long");
		return ERR_OTA_FILE_NAME_TOO_LONG;
	}

	if (send(*sock_id, filename, strlen(filename)+1, 0)<0) {
		ESP_LOGE(TAG, "Request For Update File Failed");
		return ERR_OTA_FILE_REQUEST_FAILED;
	}
	size_t ota_firmware_size=0;
	uint16_t ota_recv_bytes;
	uint8_t ota_recv_data[OTA_RECV_BUF_SIZE];
	int64_t update_download_start_time = esp_timer_get_time();
	while (esp_timer_get_time() - update_download_start_time < OTA_DOWNLOAD_TIMEOUT) {
		ota_recv_bytes = recv(*sock_id, (void *)ota_recv_data, OTA_RECV_BUF_SIZE, 0);

		//ota_recv_bytes Checks Are Important In Case Some Other
		//Server Is Running on The Specified Address and Port
		//DO NOT Include ota_recv_data[0] Check Over Here Since
		//Data Is Not Received In The Form Of Strings
		if (ota_recv_bytes > 0 && ota_recv_bytes<=OTA_RECV_BUF_SIZE) {
			ota_firmware_size += ota_recv_bytes;

			//Better To Keep This Commented, Since This Blocks UART
			//And Ends Up Rebooting the ESP
			//ESP_LOGI(TAG, "%d Bytes Downloaded", ota_firmware_size);

			if (esp_ota_write(handle, ota_recv_data, ota_recv_bytes)!=ESP_OK) {
				ESP_LOGE(TAG, "Couldn't Write Downloaded Data To Memory");
				close(*sock_id);
				return ERR_OTA_FIRMWARE_WRITE_FAILED;
			}
			if (send(*sock_id, "N", 1, 0)<0) {
				ESP_LOGE(TAG, "Failed To Send ACK To Server");
				close(*sock_id);
				return ERR_OTA_ACK_SEND_FAILED;
			}
		}
		else if (ota_recv_bytes == 0) {
			if (ota_firmware_size == 0) {
				ESP_LOGE(TAG, "Empty Update File");
				close(*sock_id);
				return ERR_OTA_FILE_EMPTY;
			}
			else {
				ESP_LOGI(TAG, "Downloaded Update");
				close(*sock_id);
				return ESP_OK;
			}
		}
		else {
			ESP_LOGE(TAG, "Download Failed");
			close(*sock_id);
			return ERR_OTA_FIRMWARE_DOWNLOAD_FAILED;
		}
		vTaskDelay(1);
	}
	ESP_LOGE(TAG, "Download Timed Out");
	close(*sock_id);
	return ERR_OTA_FIRMWARE_DOWNLOAD_TIMEOUT;
}

int8_t ota_finish(esp_ota_handle_t handle, const esp_partition_t *part) {
	if (esp_ota_end(handle)!=ESP_OK) {
		ESP_LOGE(TAG, "App Validation Failed");
		return ERR_OTA_APP_INVALID;
	}
	if (esp_ota_set_boot_partition(part)!=ESP_OK) {
		ESP_LOGE(TAG, "Boot Partition Switch Failed");
		return ERR_OTA_BOOT_SWITCH_FAILED;
	}
	ESP_LOGI(TAG, "OTA Update Successful. Restarting Device");
	ota_running = false;
	esp_restart();
	return ESP_OK;
}

void ota_exit() {
	free(ota_param->ip);
	free(ota_param->filename);
	free(ota_param);
	ota_running = false;
	ESP_LOGI(TAG, "Exiting OTA Process");
	vTaskDelete(NULL);
}


void ota_task(void* pvParameters) {

	if (wifi_sta_get_connected_status()==false) {
		ESP_LOGE(TAG, "Not Connected to WiFi. OTA Failed.");
		ota_exit();
	}

	const esp_partition_t *ota_target_partition = NULL;
	ota_target_partition = esp_ota_get_next_update_partition(NULL);
	esp_ota_handle_t ota_handle=0;
	if (esp_ota_begin(ota_target_partition, OTA_SIZE_UNKNOWN, &ota_handle)!=ESP_OK) {
		ESP_LOGE(TAG, "Failed To Begin OTA Update");
		ota_exit();
	}

	int ota_sock = -1;
	ESP_LOGI(TAG, "Starting OTA. IP : %s, Port : %d, File : %s", ota_param->ip, ota_param->port, ota_param->filename);

	if (ota_connect_to_server(&ota_sock, ota_param->ip, ota_param->port)!=ESP_OK) {
		ota_exit();
	}

	if (ota_check_for_updates(&ota_sock, ota_param->filename)!=ESP_OK) {
		ota_exit();
	}

	if (ota_connect_to_server(&ota_sock, ota_param->ip, ota_param->port)!=ESP_OK) {
		ota_exit();
	}

	if (ota_download_firmware(&ota_sock, ota_param->filename, ota_handle)!=ESP_OK) {
		ota_exit();
	}

	if (ota_finish(ota_handle, ota_target_partition)!=ESP_OK) {
		ota_exit();
	}

	ota_exit();
}

void ota_main(char *ip, int port, char *filename) {
	if (ota_running == true) {
		ESP_LOGE(TAG, "OTA Task Already Running");
		return;
	}

	ota_running = true;

	ota_param = (ota_parameters_t *)malloc(sizeof(ota_parameters_t));

	ota_param->ip = (char *)malloc((strlen(ip) + 1)*sizeof(char));
	strcpy(ota_param->ip, ip);

	ota_param->port = port;

	ota_param->filename = (char *)malloc((strlen(filename) + 1)*sizeof(char));
	strcpy(ota_param->filename, filename);

	if (xTaskCreate(&ota_task, "ota_task", 8192, NULL, 5, NULL)!=pdPASS) {
		ota_running = false;
	}
	return;
}