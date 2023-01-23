#include<string.h>

#include "error_manager.h"
#include "cJSON.h"
#include "udp_task.h"
#include "wifi.h"


#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"

int error_code_http = 0;

void creatErrorMessage(int erroeCode,cJSON* root) {
	cJSON_AddItemToObject(root, "ERROR_CODE_NO", cJSON_CreateNumber(erroeCode));
	cJSON *errorarray = cJSON_CreateArray();
	cJSON* error = cJSON_CreateObject();
	int errorType = 0;
	for(int i=0;i<=10;i++)
	{
		if(((erroeCode >> i) & 1) == 1)
		{
			switch(i)
			{
				case 0:
					errorType = wifi_status_get(STA_DISCONNECT_REASON);
					//xQueueSend(error_base_queue,&error,0);
					break;
				case 1:
					errorType = error_code_http;
					//xQueueSend(error_base_queue,&error,0);
					break;
			}
			char snum[5];
			itoa(i, snum, 10);
			cJSON_AddItemToObject(error, snum, cJSON_CreateNumber(errorType));
		}
	}

	cJSON_AddItemToArray(errorarray, error);
	cJSON_AddItemToObject(root, "errorArray", errorarray);
}

void generateErrorHotspot(int error)
{
	if (wifi_sta_get_connected_status()==true && wifi_status_get(AP_DEVICE_COUNT_ABS)==0)
	{
		wifi_config_t wifi_cfg;
		esp_wifi_get_config(ESP_IF_WIFI_AP, &wifi_cfg);

		if(error != 0) {
			char strError[5];
			memset(strError,0,5);
			itoa(error, strError, 10);
			wifi_ap_configure(true, strError);
		}
		else if (wifi_cfg.ap.ssid_hidden==false) {
			wifi_ap_configure(false, NULL);
		}
	}
}

SemaphoreHandle_t errorSemaphore;
int ERROR_CODE_NO = 0;

void setErrorCode(int errorType, bool state)
{
	if (errorSemaphore == NULL) {
		errorSemaphore = xSemaphoreCreateMutex();
	}
	if( xSemaphoreTake( errorSemaphore, 1000 ) != pdTRUE )
	{
		return;
	}

	if(state == true)
		ERROR_CODE_NO |= (1 << errorType);
	else
		ERROR_CODE_NO &= ~(1 << errorType);

	cJSON* root = cJSON_CreateObject();
	creatErrorMessage(ERROR_CODE_NO,root);
	char *temp = cJSON_Print(root);
	int leng = strlen(temp);
	cJSON_Delete(root);
	broadcastUDPMessage(temp, leng);
	free(temp);

	generateErrorHotspot(ERROR_CODE_NO);

	xSemaphoreGive(errorSemaphore);
}