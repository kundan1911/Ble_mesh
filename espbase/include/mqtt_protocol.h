#ifndef INCLUDE_MQTT_PROTOCOL_H_
#define INCLUDE_MQTT_PROTOCOL_H_


#include "mqtt_client.h"
#include "esp_log.h"

// #define WEB_SERVER "mqtt://192.168.43.33:1883"  /*esp32node*/ 
 #define WEB_SERVER "mqtt://192.168.216.191:1883"
// #define WEB_SERVER "mqtts://mqtt.picostone.com" /*PicoStone Main Server */ 
#define KEEPALIVE_TIMEOUT 10

//****** MQTT Error code*********************
#define INTERNET_CONNECT_FAILED 2
#define MQTT_SERVER_LOGIN_FAILED 4
#define INTERNET_CONNECTED 5
#define MQTT_SERVER_CONNECTED 6

//******************************************



esp_mqtt_client_handle_t client;



int adi;

void mqtt_init(int(*mqtt_connected_)(), int(*mqtt_process_commands_)(char *data));
bool mqtt_get_connected_status();
// void initParseRun(void);





//void Define_parseRun(int(*first_time1)());
// void Define_parseRun(int(*first_time1)(), int(*processHTTPCommands1)(), int(*needToReport1)(), int(*reportCommand1)());


#endif


