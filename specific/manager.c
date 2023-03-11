#include "manager.h"
#include "parsemanager.h"
#include "uart_protocol.h"
#include "udp_task.h"
#include "uart_handler.h"
#include "httpProtocol.h"
#include "genericUdpTask.h"
#include "pico_main.h"
#include "wifi.h"

#include "genericUdpTask.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "mqtt_protocol.h"

#include "driver/gpio.h"
#include "ex_eeprom.h"
#include "bldc_fan.h"

#include "ble_mesh.h"

static const char *TAG = "manager";
int WD_HttpProtocol_ex = 0;

#define GPIO_OUTPUT_IO_WD    4
#define GPIO_OUTPUT_IO_LED    22
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_WD) | (1ULL<<GPIO_OUTPUT_IO_LED))


void gpio_init()
{
	gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 1;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
	gpio_set_level(GPIO_OUTPUT_IO_LED, 0);
}

void taskWatchDog()
{
	// if(WD_HttpProtocol++>300)
	// {
	// 	ESP_LOGE(TAG, "Restarting since no No HTTP WatchDog");
	// 	rst_execute(SW_RST_NO_HTTP_WATCHDOG);
	// }
	if (mqtt_get_connected_status() == false)
		WD_HttpProtocol_ex++;
	else
		WD_HttpProtocol_ex = 0;

	// if (WD_HttpProtocol_ex > 10000) {
	// 	ESP_LOGE(TAG, "Restarting since no No HeartBeat WatchDog");
	// 	rst_execute(SW_RST_NO_HEARTBEAT_WATCHDOG);
	// }
}

void firstTimeGetInfo(){
	ESP_LOGI(TAG, "?HardwareIdentification DeviceId %s-",DEVICE_ID);
	vTaskDelay(100);
	ESP_LOGI(TAG, "\nEncryption key is %s\n", UDP_PASSWORD);
	initaeProtocol();

	
}
void firstTimeConnectDisconnectToWiFi()
{

	// if (wifi_sta_get_connected_status())
	// 	LEDChangeStates(1);
	// else
	// 	LEDChangeStates(0);
	
	// printf("\nEncryption key is %s\n", encryptionKey);
}
int uc_wd_count = 0;
void runEvery5Seconds(){
	// if(wifi_sta_get_connected_status() || uc_wd_count++ < 30)
	// {
	// 	aeQuery();
	// }
	// aeAllStates();
	// udp_broadcast_quark();
	ESP_LOGI(TAG, "Heap is (%d)\n", xPortGetFreeHeapSize()/sizeof(char));

	sync_state_enable_call(1);
	// reportCommand();
	
}

int cnt = 0;
void runEvery2Seconds(){
	printf("cnt: %d\n", cnt);
	//reportCommand();
	if(wifi_sta_get_connected_status() || uc_wd_count++ < 100)
	{
		gpio_set_level(GPIO_OUTPUT_IO_WD, cnt++ % 2);
	}
}

void stm_interrupt(){
	gpio_init();
	ex_eeprom_int();
	gpio_set_level(GPIO_OUTPUT_IO_WD, cnt++ % 2);
	for(int c = 0;c < 4; c++)
	{
		vTaskDelay(200);
		gpio_set_level(GPIO_OUTPUT_IO_WD, cnt++ % 2);
	}
	//get_device_on_bus_enable_call(1);

	firstTimeGetInfo();
	int64_t Every5Seconds = esp_timer_get_time() + 15000000;
	int64_t Every2Seconds = esp_timer_get_time() + 2000000;
	bool ConnectToWiFi = 0;
	for (;;)
	{
		// ble_send_command(1,1);
		vTaskDelay(10);
	
		//taskWatchDog();
		ReceiveInterruptUartData();
		//get_all_device_on_bus();
		get_all_node_on_bus();
		//sync_state_all_device();
		// pulse_generator(esp_timer_get_time());
		if(esp_timer_get_time() > Every5Seconds)
		{
			Every5Seconds = esp_timer_get_time() + 15000000;
			//runEvery5Seconds();
		}
		if(esp_timer_get_time() > Every2Seconds)
		{
			Every2Seconds = esp_timer_get_time() + 2000000;
			runEvery2Seconds();
			// ir_send();
		}
		if(ConnectToWiFi != wifi_sta_get_connected_status())
		{
			ConnectToWiFi = wifi_sta_get_connected_status();
			firstTimeConnectDisconnectToWiFi();
		}
	}

 }