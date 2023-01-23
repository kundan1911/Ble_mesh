
#include "mqtt_protocol.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "esp_log.h"

#include "nvs_common.h"

#include "mqtt_client.h"

#include "httpUtils.h"
#include "pico_main.h"
#include "wifi.h"


#include "parsemanager.h"

// #include "httpClient.h"

int (*mqtt_connected_call)() = NULL;
int (*mqtt_process_commands)() = NULL;

char SERVER_PASSWORD[33];

bool rst_reason_reported = false;

bool mqtt_connected = false;


static const char *TAG = "MQTT_PROTOCOL";
char * command_data;
int ret_command = 0;


bool mqtt_get_connected_status() {
    
	// return client -> state;
	return mqtt_connected;
}


//POST /commanddone
//command objectid and token
int deleteCommand_new(char* objId,int res) {
	char publish_data[100];
	memset(publish_data,0,100);

    char mqtt_topic[60];
	memset(mqtt_topic,0,60);

	// sprintf(stats,"objectid=%s&token=%s&res=%d",objId,sessionToken,res);
	// sprintf(header,"Content-Length: %d\r\n",(int)strlen(stats));

    sprintf(publish_data,"{\"objectid\":\"%s\",\"res\":%d}",objId,res);

    sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"commanddone");

	ESP_LOGI(TAG, "sent publish data - %s", publish_data);
	
	int publish_data_len = strlen(publish_data);

	int msg_id;

	msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 1, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);


	return 0;
}

int mqtt_command_call(char *command_data)
{
    ESP_LOGI(TAG, "mqtt_command_call");
    ret_command = mqtt_process_commands(command_data);
    char objid[60];
    memset(objid,0,60);
    int length;
    extractData(command_data,"_id",objid,&length);
    if(objid[0]!='\0'){
        int ret  = -1;
        ret = deleteCommand_new(objid,ret_command);
        if(ret<0) {
            return -1;
        }
    }
    return 1;

}

void publish_device_detail(){

    char publish_data[250];
	memset(publish_data,0,250);
	char mqtt_topic[60];
	memset(mqtt_topic,0,60);
    int msg_id;

    char ipAddress[16];
	wifi_sta_get_ip(ipAddress);

	uint8_t rst_reason =0;
	// if (rst_reason_reported == false) {
	// 	rst_reason = rst_get_reason();
	// }
	// else {
	// 	rst_reason = 0xFF;
	// }

	uint64_t wifi_reason = 0;//wifi_ap_discon_code_get();
	// int wifi_time =  get_wifi_conn_time();
	int wifi_time =  0;

    memset(mqtt_topic,0,60);
    sprintf(publish_data,"{\"version\":\"%s\",\"ipAddress\":\"%s\",\"wifi_ssid\":\"%s\",\"wifi_bssid\":\""MACSTR"\",\"RSSI\":%2d,\"channel\":%d,\"partition\":%d,\"rst_reason\":%u,\"wifi_reason\":%X,\"wifi_time\":%6d}",\
    ESP_FIRMWARE_VERSION,ipAddress,wifi_connected_ap_info.ssid,MAC2STR(wifi_connected_ap_info.bssid),wifi_connected_ap_info.rssi, \
    wifi_connected_ap_info.primary,(RUNNING_PARTITION[4]-'0'),rst_reason,(uint32_t)(wifi_reason & UINT32_MAX),wifi_time/1000);

    sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"device_detail");

    int publish_data_len = strlen(publish_data);

    msg_id = esp_mqtt_client_publish(client, mqtt_topic, publish_data, publish_data_len, 1, 0);
    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    // esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char mqtt_topic[60];
	memset(mqtt_topic,0,60);

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"command");
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);


            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"ping");
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"sync");
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic, 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"status");

            msg_id = esp_mqtt_client_publish(client, mqtt_topic, "online", 6, 1, 1);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            publish_device_detail();

            // reportCurrentState();
            mqtt_connected_call();



            

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            mqtt_connected = true;
            // msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            mqtt_connected = true;
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            mqtt_connected = true;
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            // printf("DATA=%.*s\r\n", event->data_len, event->data);
            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"ping");
            if (strncmp(event->topic,mqtt_topic, event->topic_len) == 0)
            {
                // msg_id = esp_mqtt_client_publish(client, mqtt_topic, "online", 6, 1, 1);
                // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            }
            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"sync");
            if (strncmp(event->topic,mqtt_topic, event->topic_len) == 0)
            {
                publish_device_detail();
                mqtt_connected_call();
            }
            memset(mqtt_topic,0,60);
            sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"command");
            if(strncmp(event->topic,mqtt_topic, event->topic_len) == 0)
            {
                command_data = malloc(event->data_len);
                memcpy(command_data, event->data, event->data_len);
                mqtt_command_call(command_data);
                free(command_data);
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            mqtt_connected = false;
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            mqtt_connected = false;
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    memset(SERVER_PASSWORD,0,sizeof(SERVER_PASSWORD));
	if (nvs_get_srv_password(SERVER_PASSWORD)) {
		ESP_LOGE(TAG, "Server Password Retrieve Failed");
		esp_restart();
	}
    char mqtt_topic[60];
	memset(mqtt_topic,0,60);
    sprintf(mqtt_topic,"/device/%s/%s",DEVICE_ID,"status");

    char client_id_[20];
	memset(client_id_,0,20);
    sprintf(client_id_,"%s",DEVICE_ID);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = WEB_SERVER,
		// .reconnect_timeout_ms = 5000,
		.keepalive = KEEPALIVE_TIMEOUT,
        .lwt_retain = 1,
        .lwt_topic = mqtt_topic,
        .lwt_msg = "offline",
        .skip_cert_common_name_check = true,
        .client_id = client_id_,
        .username = DEVICE_ID,
        .password = SERVER_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


bool changedNetifDefault = false;



void mqtt_init(int(*mqtt_connected_)(), int(*mqtt_process_commands_)(char *data)) {

    mqtt_connected_call = mqtt_connected_;
    mqtt_process_commands = mqtt_process_commands_;
	
	LOCK_TCPIP_CORE();
	netif_set_default(netif_list->next);
	UNLOCK_TCPIP_CORE();
	mqtt_app_start();
}