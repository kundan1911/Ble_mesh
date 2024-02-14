#include "pico_main.h"

#include "nvs_basic.h"
#include "nvs_common.h"
#include "ota.h"
#include "manager.h"
#include "parsemanager.h"
#include "scenes.h"
#include "uart_handler.h"
#include "udp_task.h"
#include "wifi.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_spi_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "rmt_tx.h"

#include "ble_mesh.h"


//#define TRANSITION_SDK_OLD_TO_NEW

//Uncomment The Next Line to Set SSID & Password
#define SET_NEW_WIFI_CRED
//Comment & Upload Again After Setting

//Uncomment The Next Line to Perform OTA on Boot
//#define PERFORM_OTA_ON_BOOT

#ifdef SET_NEW_WIFI_CRED
#define WIFI_STA_SSID "realme X2"
#define WIFI_STA_PASSWORD "deepak100"
//#define WIFI_STA_SSID "Sharma_Home"
//#define WIFI_STA_PASSWORD "1234A701#"
// #define WIFI_STA_SSID "Error505"
// #define WIFI_STA_PASSWORD "itspersonal"
#endif

#ifdef TRANSITION_SDK_OLD_TO_NEW
#include "flash_cfg_old_to_new.h"
#endif

char* getName(char*  name){
	return "PicoBasic";
}

void app_main() {
	nvs_setup_common();
	nvs_setup_basic();

	// const esp_partition_t *running_partition = esp_ota_get_running_partition();
	// strcpy(RUNNING_PARTITION, running_partition->label);

	#ifdef TRANSITION_SDK_OLD_TO_NEW
	flash_config_convert_old_to_new();
	#endif

	//Retrieve Device ID From NVS
	
	// memset(DEVICE_ID,0,sizeof(DEVICE_ID));
	// if (nvs_get_dev_id(DEVICE_ID)!=ESP_OK) {
	//  	esp_restart();
	// }
	char* str="MASW0100001AA121";
	for(int i=0;i<17;i++){
		DEVICE_ID[i]=str[i];
	}
	wifi_sta_get_mac(DEVICE_MAC_STR);

	
	uart_setup();

	// rmt_tx_int();

	#ifdef SET_NEW_WIFI_CRED
	if (nvs_set_sta_credentials(WIFI_STA_SSID, WIFI_STA_PASSWORD)!=ESP_OK) {
		esp_restart();
	}
	#endif

	wifi_setup();

	#ifdef PERFORM_OTA_ON_BOOT
	ota_main("13.127.6.227", 20000, "firmware_add_basic4port20190718113705.bin");
	#endif

	initParsemanager();

	// udp_setup();

	ble_mesh_init();

	// scenes_setup();

	

	xTaskCreate(stm_interrupt,"manager", 4096, NULL, 5, NULL);
	// scenes_setup();
}