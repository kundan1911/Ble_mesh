/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "ble_mesh.h"
#include "esp_log.h"
// #include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "transport.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "ble_mesh_example_init.h"
#include "ble_mesh_protocol.h"
#include "cJSON.h"
#include "mqtt_publish.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "provisioner_main.h"


#define TAG "Provisioner"

#define LED_OFF             0x0
#define LED_ON              0x1

#define CID_ESP             0x05f1

#define PROV_OWN_ADDR       0x0001

#define MSG_SEND_TTL        3
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         250
#define MSG_ROLE            ROLE_PROVISIONER

#define COMP_DATA_PAGE_0    0x00

#define APP_KEY_IDX         0x0000
#define APP_KEY_OCTET       0x12
#define NET_KEY_Octet       0x13
#define acceptPkgCunt       0x0f
// for generic onoff and vendor 
#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)

esp_timer_handle_t pkgTimer;

uint8_t get_node_on_bus_enable = 0;
uint8_t get_node_on_bus_current_count = 0;
uint8_t prov_node_total_count =0;
uint8_t unprov_pkg_to_mqtt = 0;
uint8_t retransTime;
static uint8_t dev_uuid[16];
uint8_t* already_push_to_mqtt[acceptPkgCunt];
int count_of_present_dev_id=0;
typedef struct {
    uint8_t  uuid[16];
    uint16_t unicast;
    uint8_t  elem_num;
    uint8_t  onoff;
    uint8_t  level;
} esp_ble_mesh_node_info_t;

static esp_ble_mesh_node_info_t nodes[CONFIG_BLE_MESH_MAX_PROV_NODES] = {
    [0 ... (CONFIG_BLE_MESH_MAX_PROV_NODES - 1)] = {
        .unicast = ESP_BLE_MESH_ADDR_UNASSIGNED,
        .elem_num = 0,
        .onoff = LED_OFF,
    }
};

static struct esp_ble_mesh_key {
    uint16_t net_idx;
    uint16_t app_idx;
    uint8_t  app_key[16];
} prov_key;

static const esp_ble_mesh_client_op_pair_t vnd_op_pair[] = {
    { ESP_BLE_MESH_VND_MODEL_OP_SEND, ESP_BLE_MESH_VND_MODEL_OP_STATUS },
};

static esp_ble_mesh_client_t vendor_client = {
    .op_pair_size = ARRAY_SIZE(vnd_op_pair),
    .op_pair = vnd_op_pair,
};


static esp_ble_mesh_model_op_t vnd_op[] = {
    // ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 0),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};



static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_CLIENT,
    vnd_op, NULL, &vendor_client),
};

static esp_ble_mesh_client_t config_client;
static esp_ble_mesh_client_t level_client;
static esp_ble_mesh_client_t ctl_client;
static esp_ble_mesh_client_t onoff_client;

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 9 + 1, ROLE_PROVISIONER);
ESP_BLE_MESH_MODEL_PUB_DEFINE(light_cli_pub, 9 + 1, ROLE_PROVISIONER);
static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 1,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(0, 10),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(0,10),
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client),
      //  ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
     ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
    ESP_BLE_MESH_MODEL_LIGHT_CTL_CLI(&light_cli_pub, &ctl_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0,root_models,vnd_models),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid           = dev_uuid,
    .prov_unicast_addr   = PROV_OWN_ADDR,
    .prov_start_address  = 0x0009,
    .prov_attention      = 0x00,
    .prov_algorithm      = 0x00,
    .prov_pub_key_oob    = 0x00,
    .prov_static_oob_val = NULL,
    .prov_static_oob_len = 0x00,
    .flags               = 0x00,
    .iv_index            = 0x00,
};


void default_push_to_mqtt(){
   cJSON * node_info=cJSON_CreateObject();
   ESP_LOGI(TAG,"default not mqtt");
    cJSON_AddItemToObject(node_info,"devKey" , cJSON_CreateString(""));
    cJSON_AddItemToObject(node_info,"deviceUuid" , cJSON_CreateString(""));
    cJSON_AddItemToObject(node_info, "address", cJSON_CreateString(""));
    cJSON_AddItemToObject(node_info,"addrType" , cJSON_CreateNumber(0));
    cJSON_AddItemToObject(node_info,"bearer" , cJSON_CreateNumber(0));
     cJSON_AddItemToObject(node_info,"unicastAddr" , cJSON_CreateNumber(0));
    cJSON_AddItemToObject(node_info,"netIdx" , cJSON_CreateNumber(0));
    cJSON_AddItemToObject(node_info,"InfoType" , cJSON_CreateNumber(0));

    char* cjson_node_info=cJSON_Print(node_info);
     postBleDeviceInfo(cjson_node_info);
    cJSON_Delete(node_info);  
}
static esp_err_t example_ble_mesh_set_msg_common_tt(esp_ble_mesh_client_common_param_t *common,
                                                 uint16_t unicast,
                                                 esp_ble_mesh_model_t *model, uint32_t opcode)
{
    if (!common || !model) {
        return ESP_ERR_INVALID_ARG;
    }
 

    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = unicast;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;

    return ESP_OK;
}
static esp_err_t vendor_set_msg_common_tt(esp_ble_mesh_msg_ctx_t *common,
                                                 uint16_t unicast)
{
    if (!common ) {
        return ESP_ERR_INVALID_ARG;
    }
 
    common->net_idx = prov_key.net_idx;
    common->app_idx = prov_key.app_idx;
    common->addr = unicast;
    common->send_ttl = MSG_SEND_TTL;
    common->send_rel = MSG_SEND_REL;
    return ESP_OK;
}

uint32_t prev_vnd_id;
//  esp_err_t snd_mssg_to_vnd_srv(uint16_t ndAddr){
//     esp_ble_mesh_msg_ctx_t commonContext = {0};
//     esp_err_t err;
//     uint32_t opcode=ESP_BLE_MESH_VND_MODEL_OP_SEND;
//     // err=vendor_set_msg_common_tt(&commonContext,ndAddr);
//     commonContext.net_idx = prov_key.net_idx;
//     commonContext.app_idx = prov_key.app_idx;
//     commonContext.addr = ndAddr;
//     commonContext.send_ttl = MSG_SEND_TTL;
//     commonContext.send_rel = MSG_SEND_REL;
//     uint8_t mssg[]={0xff,0xff};
//     retransTime++;
//     err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &commonContext, opcode,
//             2, mssg, MSG_TIMEOUT, true, MSG_ROLE);
//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
//         return err;
//     }
//     else{
//           ESP_LOGI(TAG, "mssg send to the server vendor");
//     }
//     return err;
// }
uint8_t mqtt_snd_mssg_to_vnd_srv(uint8_t ndAddr,uint16_t grpAddr){
esp_ble_mesh_msg_ctx_t ctx = {0};
esp_err_t err;
err= esp_ble_mesh_model_subscribe_group_addr(PROV_OWN_ADDR,ESP_BLE_MESH_CID_NVAL,ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI,grpAddr);
        ESP_LOGE(TAG, "group addr subscribe: %d and curr addr %d",err,grpAddr);
    uint32_t opcode;
    
    uint8_t lw=grpAddr & 0x00FF;
     uint8_t up=grpAddr >> 8;   
    uint8_t mss[]={lw,up,3,4,5};
    // ESP_LOGI(TAG, "node addr ");
    ctx.net_idx = prov_key.net_idx;
    ctx.app_idx = prov_key.app_idx;
    ctx.addr = (uint16_t) ndAddr;
    ctx.send_ttl = MSG_SEND_TTL;
    ctx.send_rel = MSG_SEND_REL;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;
    err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
            5, mss, MSG_TIMEOUT, true, MSG_ROLE);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
        return err;
    }
    else{
          ESP_LOGI(TAG, "mssg send to the server vendor");
    }
    return err;

}
esp_err_t snd_mssg_to_vnd_srv(uint16_t ndAddr){
    // esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
    //                 ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP);
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t err; 
    uint8_t mss[]={1,2,3,4,5};
    // ESP_LOGI(TAG, "node addr ");
    ctx.net_idx = prov_key.net_idx;
    ctx.app_idx = prov_key.app_idx;
    ctx.addr = ndAddr;
    ctx.send_ttl = MSG_SEND_TTL;
    ctx.send_rel = MSG_SEND_REL;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;
    err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
            5, mss, MSG_TIMEOUT, true, MSG_ROLE);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
        return err;
    }
    else{
          ESP_LOGI(TAG, "mssg send to the server vendor");
    }
    return err;

}
//  esp_err_t snd_dev_to_prov_node(uint16_t ndAddr){
//    esp_ble_mesh_msg_ctx_t ctx = {0};
//     esp_err_t err;
//     uint32_t opcode=ESP_BLE_MESH_VND_MODEL_OP_SEND;
//     // err=vendor_set_msg_common_tt(&commonContext,ndAddr);
//        ctx.net_idx = prov_key.net_idx;
//     ctx.app_idx = prov_key.app_idx;
//     ctx.addr = ndAddr;
//     ctx.send_ttl = MSG_SEND_TTL;
//     ctx.send_rel = MSG_SEND_REL;
//     opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;
//     uint8_t messg_len=16;
//     // uint8_t messg[messg_len];
    // struct bt_mesh_node* node=bt_mesh_provisioner_get_node_with_addr(ndAddr);
//     uint8_t* devKey=node->dev_key;
//     // for(uint8_t i=0;i<16;i++){
//     //     messg[i]=devKey[i];
//     // }
//     ESP_LOGI(TAG,"sended Dev key-----");
//                   for(int i=0;i<16;i++){
//                 ESP_LOGI(TAG,"%d",devKey[i]);
//             }
//     retransTime++;
//     err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, ESP_BLE_MESH_VND_MODEL_OP_SEND,
//             messg_len, devKey, MSG_TIMEOUT, false, MSG_ROLE);
//             // err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, opcode,
//             // 21, mssg_data, MSG_TIMEOUT, true, MSG_ROLE);
//     if (err != ESP_OK) {
//         ESP_LOGI(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
//         return err;
//     }
//     else{
//           ESP_LOGI(TAG, "mssg send to the server vendor");
//     }
//     return err;
// }
 esp_err_t snd_mssg_to_clt(uint16_t clt_addr){
    esp_ble_mesh_msg_ctx_t ctx = {0};
    uint32_t opcode;
    esp_err_t err;
    uint8_t mssg_data[]={0x00};
    // ESP_LOGE(TAG, "clt addr 0x%04x, srv addr 0x%04x" ,clt_addr, srv_addr);
    ctx.net_idx = 0;
    ctx.app_idx = 0;
    ctx.addr = 12;
    ctx.send_ttl = MSG_SEND_TTL;
    ctx.send_rel = 1;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_SEND;

    // mssg_data[0]=0;
    // for(uint8_t i=1;i<6;i++){
    //     mssg_data[i]=i+10;
    // }
    err = esp_ble_mesh_client_model_send_msg(vendor_client.model, &ctx, ESP_BLE_MESH_VND_MODEL_OP_SEND,
            1, mssg_data, MSG_TIMEOUT, true, MSG_ROLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send vendor message 0x%06" PRIx32, opcode);
        return err;
    }
    else{
          ESP_LOGE(TAG, "mssg send to the server vendor");
    }
    return err;

}

void control_gear_search_ble_enable(uint8_t state){
	unprov_pkg_to_mqtt = state;
     esp_timer_start_once(pkgTimer,10000000);
return ;
}

void get_node_on_bus_enable_call(uint8_t state,uint8_t sizeProvNode){
	printf("get_node_comp_data_enable - %d\n",state);
	get_node_on_bus_enable = state;
    prov_node_total_count=11+sizeProvNode;
    get_node_on_bus_current_count=11;
    return;
}

_Bool get_node_on_bus(uint8_t device_id)
{
	printf("device id - %d\n",get_node_on_bus_current_count);
	snd_mssg_to_vnd_srv(device_id);	
	return true;
}
_Bool get_all_node_on_bus()
{
	if(get_node_on_bus_enable <= 0)
	{
		return false;
	}
	printf("get_all_device_on_bus device id - %d\n",get_node_on_bus_current_count);
		if(get_node_on_bus_current_count > prov_node_total_count){
		// update_device_detal(0,0); // end of sync device on rs485 bus
         default_push_to_mqtt();
         ESP_LOGI(TAG,"no deault push");
		get_node_on_bus_enable = 0;
		get_node_on_bus_current_count = 10;
		return true;
	}
	get_node_on_bus(get_node_on_bus_current_count);
	get_node_on_bus_current_count++;
    // get_node_on_bus_enable = 0;
	// 	get_node_on_bus_current_count = 10;
	return true;	
}  
uint8_t already_pushed_pkg_to_mqtt(uint8_t* n_dev_id){
    if(!count_of_present_dev_id){
        already_push_to_mqtt[0]=(uint8_t*)malloc(16 * sizeof(uint8_t));
        for(uint8_t i=0;i<16;i++){
            already_push_to_mqtt[0][i]=n_dev_id[i];
        }
        count_of_present_dev_id=count_of_present_dev_id+1;
        return 0;
    }
    ESP_LOGI(TAG,"value of present_dev_id = %d",count_of_present_dev_id);
    uint8_t i,j;
    for( i=0;i<count_of_present_dev_id;i++){
            for( j=0;j<16;j++){
                if(n_dev_id[j]!=already_push_to_mqtt[i][j]){
                    break;
                }
            }
            if(j==16){
        return 1; 
            }
            }
            if(count_of_present_dev_id <= acceptPkgCunt){
already_push_to_mqtt[count_of_present_dev_id]=(uint8_t*)malloc(16 * sizeof(uint8_t));
        for(uint8_t k=0;k<16;k++){
            already_push_to_mqtt[count_of_present_dev_id][k]=n_dev_id[k];
        }
        count_of_present_dev_id=count_of_present_dev_id+1;
        return 0;
            }
    return 1;
}
void clear_all_recv_pkg_arr(){
    for(int i=0;i<count_of_present_dev_id;i++){
        free(already_push_to_mqtt[i]);
    }
    count_of_present_dev_id=0;
    return;
}
void endPkgRec(){
    esp_timer_stop(pkgTimer);
    clear_all_recv_pkg_arr();
unprov_pkg_to_mqtt=0;
}

static esp_err_t example_ble_mesh_store_node_info(const uint8_t uuid[16], uint16_t unicast,
                                                  uint8_t elem_num, uint8_t onoff_state)
{
    int i;
    ESP_LOGW(TAG, "%s: uuuuuuuuuuunicast device - %d", __func__, unicast);

    if (!uuid || !ESP_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Judge if the device has been provisioned before */
    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        if (!memcmp(nodes[i].uuid, uuid, 16)) {
            ESP_LOGW(TAG, "%s: reprovisioned device 0x%04x", __func__, unicast);
            nodes[i].unicast = unicast;
            nodes[i].elem_num = elem_num;
            nodes[i].onoff = onoff_state;
            return ESP_OK;
        }
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        if (nodes[i].unicast == ESP_BLE_MESH_ADDR_UNASSIGNED) {
            memcpy(nodes[i].uuid, uuid, 16);
            nodes[i].unicast = unicast;
            nodes[i].elem_num = elem_num;
            nodes[i].onoff = onoff_state;
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

static esp_ble_mesh_node_info_t *example_ble_mesh_get_node_info(uint16_t unicast)
{
    int i;

    if (!ESP_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        if (nodes[i].unicast <= unicast &&
                nodes[i].unicast + nodes[i].elem_num > unicast) {
            return &nodes[i];
        }
    }

    return NULL;
}

static esp_err_t example_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
                                                 esp_ble_mesh_node_info_t *node,
                                                 esp_ble_mesh_model_t *model, uint32_t opcode)
{
    if (!common || !node || !model) {
        return ESP_ERR_INVALID_ARG;
    }

    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = node->unicast;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;

    return ESP_OK;
}


uint8_t local_tid=0;

void example_ble_mesh_send_gen_onoff_set(uint16_t device_id,uint8_t onoff)
{
    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;
    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET;
    common.model = onoff_client.model;
    common.ctx.net_idx = 0;
    common.ctx.app_idx = 0;
    common.ctx.addr = device_id;   /* to all nodes */
    common.ctx.send_ttl = 3;
    common.ctx.recv_rssi = 0;
    common.ctx.recv_ttl = 5;
    common.ctx.send_rel = false;
    common.msg_timeout = 300;     /* 0 indicates that timeout value from menuconfig will be used */
    common.msg_role = ROLE_PROVISIONER;

    // set.onoff_set.op_en = false;
    // set.onoff_set.onoff = onoff;
    // set.onoff_set.tid = 0;

//   example_ble_mesh_set_msg_common_tt(&common, addr, onoff_client.model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK);
    set.onoff_set.op_en = true;
    set.onoff_set.onoff = onoff;
    set.onoff_set.tid = local_tid++;
    set.onoff_set.trans_time=0;
    set.onoff_set.delay=0;
    err = esp_ble_mesh_generic_client_set_state(&common, &set);
    ESP_LOGI(TAG,"send messge success %d",err);
    if (err) {
        ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
        return;
    }

}

void gen_onoff_push_mqtt(uint16_t src,uint16_t dest,uint8_t onoff,uint8_t evnt){
    cJSON * gen_info=cJSON_CreateObject();
    cJSON_AddItemToObject(gen_info,"src_addr" , cJSON_CreateNumber(src));
    cJSON_AddItemToObject(gen_info, "dest_addr", cJSON_CreateNumber(dest));
    cJSON_AddItemToObject(gen_info,"on_off" , cJSON_CreateNumber(onoff));
    cJSON_AddItemToObject(gen_info,"event" , cJSON_CreateNumber(evnt));

    char* cjson_gen_info=cJSON_Print(gen_info);
    postBleDeviceInfo(cjson_gen_info);
    cJSON_Delete(gen_info);
    
}
void pushProvNodeToMqtt(uint16_t unicast){
   struct bt_mesh_node* nodeInfo= bt_mesh_provisioner_get_node_with_addr(unicast);

    cJSON * node_info=cJSON_CreateObject();
     cJSON_AddItemToObject(node_info,"devKey" , cJSON_CreateString(bt_hex(nodeInfo->dev_key, 16)));
    cJSON_AddItemToObject(node_info,"deviceUuid" , cJSON_CreateString(bt_hex(nodeInfo->dev_uuid, 16)));
    cJSON_AddItemToObject(node_info, "address", cJSON_CreateString(bt_hex(nodeInfo->addr, BD_ADDR_LEN)));
    cJSON_AddItemToObject(node_info,"addrType" , cJSON_CreateNumber(nodeInfo->addr_type));
     cJSON_AddItemToObject(node_info,"unicastAddr" , cJSON_CreateNumber(unicast));
    cJSON_AddItemToObject(node_info,"netIdx" , cJSON_CreateNumber(nodeInfo->net_idx));
     cJSON_AddItemToObject(node_info,"infoType" , cJSON_CreateNumber(2));
    char* cjson_node_info=cJSON_Print(node_info);
    ESP_LOGI(TAG,"node Info :- %s",cjson_node_info);
    postBleDeviceInfo(cjson_node_info);
    cJSON_Delete(node_info);
    
}


static esp_err_t prov_complete(int node_idx, const esp_ble_mesh_octet16_t uuid,
                               uint16_t unicast, uint8_t elem_num, uint16_t net_idx)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_get_state_t get_state = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    char name[11] = {0};
    int err;

    ESP_LOGI(TAG, "node index: 0x%x, unicast address: 0x%02x, element num: %d, netkey index: 0x%02x",
             node_idx, unicast, elem_num, net_idx);
    ESP_LOGI(TAG, "device uuid: %s", bt_hex(uuid, 16));

    sprintf(name, "%s%d", "NODE-", node_idx);
    err = esp_ble_mesh_provisioner_set_node_name(node_idx, name);
    if (err) {
        ESP_LOGE(TAG, "%s: Set node name failed", __func__);
        return ESP_FAIL;
    }

    err = example_ble_mesh_store_node_info(uuid, unicast, elem_num, LED_OFF);
    if (err) {
        ESP_LOGE(TAG, "%s: Store node info failed", __func__);
        return ESP_FAIL;
    }

    node = example_ble_mesh_get_node_info(unicast);
    if (!node) {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return ESP_FAIL;
    }

    example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get_state.comp_data_get.page = COMP_DATA_PAGE_0;
    err = esp_ble_mesh_config_client_get_state(&common, &get_state);
    if (err) {
        ESP_LOGE(TAG, "%s: Send config comp data get failed", __func__);
        return ESP_FAIL;
    }
    
    pushProvNodeToMqtt(unicast);
    return ESP_OK;
}

static void prov_link_open(esp_ble_mesh_prov_bearer_t bearer)
{
    ESP_LOGI(TAG, "%s link open", bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
}

static void prov_link_close(esp_ble_mesh_prov_bearer_t bearer, uint8_t reason)
{
    ESP_LOGI(TAG, "%s link close, reason 0x%02x",
             bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", reason);
}

static void recv_unprov_adv_pkt(uint8_t dev_uuid[16], uint8_t addr[BD_ADDR_LEN],
                                esp_ble_mesh_addr_type_t addr_type, uint16_t oob_info,
                                uint8_t adv_type, esp_ble_mesh_prov_bearer_t bearer)
{

    /* Due to the API esp_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
     * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
     * to the application layer.
     */
    
    ESP_LOGI(TAG, "address: %s, address type: %d, adv type: %d", bt_hex(addr, BD_ADDR_LEN), addr_type, adv_type);
    ESP_LOGI(TAG, "device uuid: %s", bt_hex(dev_uuid, 16));
    ESP_LOGI(TAG, "oob info: %d, bearer: %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");
    // add unprov_pkg_to_mqtt in below to set control to the user
    if( !already_pushed_pkg_to_mqtt(dev_uuid)){
    cJSON * node_info=cJSON_CreateObject();
    cJSON_AddItemToObject(node_info,"deviceUuid" , cJSON_CreateString(bt_hex(dev_uuid, 16)));
    cJSON_AddItemToObject(node_info, "address", cJSON_CreateString(bt_hex(addr, BD_ADDR_LEN)));
    cJSON_AddItemToObject(node_info,"addrType" , cJSON_CreateNumber(addr_type));
    cJSON_AddItemToObject(node_info,"bearer" , cJSON_CreateNumber(bearer));
     cJSON_AddItemToObject(node_info,"infoType" , cJSON_CreateNumber(2));
    char* cjson_node_info=cJSON_Print(node_info);
    postBleDeviceInfo(cjson_node_info);
    }
    return;
}

static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_RECV_UNPROV_ADV_PKT_EVT");
        recv_unprov_adv_pkt(param->provisioner_recv_unprov_adv_pkt.dev_uuid, param->provisioner_recv_unprov_adv_pkt.addr,
                            param->provisioner_recv_unprov_adv_pkt.addr_type, param->provisioner_recv_unprov_adv_pkt.oob_info,
                            param->provisioner_recv_unprov_adv_pkt.adv_type, param->provisioner_recv_unprov_adv_pkt.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        prov_link_open(param->provisioner_prov_link_open.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        prov_link_close(param->provisioner_prov_link_close.bearer, param->provisioner_prov_link_close.reason);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
        prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
                      param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
                      param->provisioner_prov_complete.netkey_idx);
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT: {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
        if (param->provisioner_set_node_name_comp.err_code == ESP_OK) {
            const char *name = NULL;
            name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
            if (!name) {
                ESP_LOGE(TAG, "Get node name failed");
                return;
            }
            ESP_LOGI(TAG, "Node %d name is: %s", param->provisioner_set_node_name_comp.node_index, name);
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT: {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
        if (param->provisioner_add_app_key_comp.err_code == ESP_OK) {
            prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
        break;
    default:
        break;
    }

    return;
}

static void example_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
                                              esp_ble_mesh_cfg_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    uint32_t opcode;
    uint16_t addr;
    int err;

    opcode = param->params->opcode;
    addr = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
             __func__, param->error_code, event, param->params->ctx.addr, opcode);

    if (param->error_code) {
        ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04x", opcode);
        return;
    }

    node = example_ble_mesh_get_node_info(addr);
    if (!node) {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
            ESP_LOGI(TAG, "composition data %s", bt_hex(param->status_cb.comp_data_status.composition_data->data,
                     param->status_cb.comp_data_status.composition_data->len));
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set_state.app_key_add.net_idx = prov_key.net_idx;
            set_state.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
                return;
            }
        //     cJSON * gen_info=cJSON_CreateObject();
        // cJSON_AddItemToObject(gen_info,"success" , cJSON_CreateNumber(0));
        // cJSON_AddItemToObject(gen_info, "deviceType", cJSON_CreateNumber(1));
        // char* cjson_node_info=cJSON_Print(gen_info);
        // postBleDeviceInfo(cjson_node_info);
        // cJSON_Delete(gen_info);
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV;
            set_state.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
            /*
            set_state.model_app_bind.model_id = ESP_BLE_MESH_VND_MODEL_ID_SERVER;
            set_state.model_app_bind.company_id = CID_ESP;
             set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI;
            set_state.model_app_bind.company_id = 0x05f1;
            
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send Config Model App Bind");
            }
            else{
                  ESP_LOGE(TAG, "Success to app bind generic server ");
            }
*/
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
            esp_ble_mesh_generic_client_get_state_t get_state = {0};
            example_ble_mesh_set_msg_common(&common, node, ctl_client.model, ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_GET);
            err = esp_ble_mesh_light_client_set_state(&common, &get_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Generic OnOff Get failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS:
            ESP_LOG_BUFFER_HEX("composition data %s", param->status_cb.comp_data_status.composition_data->data,
                               param->status_cb.comp_data_status.composition_data->len);
             
            break;
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_STATUS:
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
            esp_ble_mesh_cfg_client_get_state_t get_state = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
            get_state.comp_data_get.page = COMP_DATA_PAGE_0;
            err = esp_ble_mesh_config_client_get_state(&common, &get_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Composition Data Get failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set_state.app_key_add.net_idx = prov_key.net_idx;
            set_state.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            example_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV;
            set_state.model_app_bind.company_id = ESP_BLE_MESH_CID_NVAL;
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
         
            break;
        }
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Not a config client status message event");
        break;
    }
}
uint16_t prev_id;
void* prevSetState;

int ble_send_command(uint16_t device_id,void * set_state1)
{
  int err;
  ESP_LOGI(TAG, "start addr %d",device_id);
  uint16_t addr = device_id;
prev_id=device_id;
prevSetState=set_state1;
  esp_ble_mesh_client_common_param_t common = {0};
  esp_ble_mesh_light_client_set_state_t* set_state = (struct esp_ble_mesh_light_client_set_state_t* ) set_state1;

  ESP_LOGI(TAG, "ble_send_command onoff:");
    ESP_LOGI(TAG, "temp %d light %d:",set_state->ctl_set.ctl_temperatrue,set_state->ctl_set.ctl_lightness);
  /* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
  example_ble_mesh_set_msg_common_tt(&common, addr, ctl_client.model, ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_SET);
retransTime++;
ESP_LOGI(TAG, "send addr %d",common.ctx.addr);
  err = esp_ble_mesh_light_client_set_state(&common,set_state);
  if (err) {
    ESP_LOGE(TAG, "%s: Generic OnOff Set failed (err %d)", __func__,err);
    // return err;
  }
  return err;
}
// int ble_send_command_color(int device_id,int data)
// {
//   int err;
//   esp_ble_mesh_client_common_param_t common = {0};
//   esp_ble_mesh_light_client_set_state_t set_state = {0};
//   //node->onoff = param->status_cb.onoff_status.present_onoff;
//   ESP_LOGI(TAG, "ble_send_command_color onoff: 0x%02x", data);
//   /* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
//   example_ble_mesh_set_msg_common_tt(&common, device_id, ctl_client.model, ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_SET);
//   //as per nrf mesh mssg
//   set_state.ctl_set.op_en = true;
// //   set_state.ctl_set.ctl_lightness= 255;
// //   set_state.ctl_set.ctl_temperatrue= 0x0320 + data;
// // //   set_state.ctl_set.tid = 0;
// //   set_state.ctl_set.tid=tid++;
// // set_state.ctl_set.trans_time=0;
// //   set_state.ctl_set.delay=0;

//   err = esp_ble_mesh_light_client_set_state(&common, &set_state);
//   if (err) {
//     ESP_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
//     // return err;
//   }
//   return err;
// }

uint8_t deleteBleDevice(uint16_t unicast){
 esp_err_t err=esp_ble_mesh_provisioner_delete_node_with_addr(unicast);
              if (err == ESP_OK) {
                ESP_LOGI(TAG, "success deleted");
                
            }
            else{
                 ESP_LOGE(TAG, "not deleted");
            }
            return err;
}
uint8_t addProvisionedNode(const uint8_t *addr,const uint8_t* uuid,uint16_t unicast_addr,uint16_t net_idx,const uint8_t* dev_key ,uint8_t addrType){
     bt_mesh_addr_t ProvAddrVal;
     uint16_t ind=0;
            memcpy(ProvAddrVal.val, addr, 6);
            ProvAddrVal.type=addrType;
 esp_err_t err= bt_mesh_provisioner_provision(&ProvAddrVal,uuid,0,unicast_addr,1,net_idx,0,0,dev_key,&ind);
 return err;
}


static void example_ble_mesh_lightness_client_model_cb(esp_ble_mesh_light_client_cb_event_t event,
                                             esp_ble_mesh_light_client_cb_param_t *param)
{
ESP_LOGI(TAG, "event %d opcode %d",event, param->params->opcode);
 nodeAck ack;
    switch (event) {
    case ESP_BLE_MESH_LIGHT_CLIENT_GET_STATE_EVT:
         ESP_LOGI(TAG, "Get server state event 0x%06" PRIx32, param->params->opcode);
        break;
    case ESP_BLE_MESH_LIGHT_CLIENT_SET_STATE_EVT:
    // mytimeXy=esp_timer_get_time()-mytimeXy;
    // ESP_LOGI(TAG,"tm %lld",(mytimeXy/1000));
     ack.desAddr=param->params->ctx.addr;
            // ack.tid=2;
            ack.err=param->error_code;
        if (param->error_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->params->opcode);
           
            break;
        }
            pushAckToQueue(ack);

        ESP_LOGI(TAG, "receive acknowlegement addr 0x%04x",param->params->ctx.addr);
        retransTime=0;
        break;
    case ESP_BLE_MESH_LIGHT_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "Receive after timeout publish message ");
         ESP_LOGI(TAG, "src 0x%04x",param->params->ctx.addr);
        
    case ESP_BLE_MESH_LIGHT_CLIENT_TIMEOUT_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHT_CLIENT_TIMEOUT_EVT");
       
        ESP_LOGI(TAG, "error_code   %d",param->error_code);
        
        if(retransTime==1)
        ble_send_command(prev_id,prevSetState);
        else{
            retransTime=0;
        }
    default:
        break;
    }
    
}
static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    // static int64_t start_time;
 nodeAck ack;
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
      ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OPERATION_EVT 0x%06" PRIx32, param->model_send_comp.opcode);
        ESP_LOGI(TAG, " opcode 0x%04x, src 0x%04x, dst 0x%04x",
             param->model_operation.ctx->recv_op, param->model_operation.ctx->addr, param->model_operation.ctx->recv_dst);
            
             ack.desAddr= param->model_operation.ctx->addr;
            // ack.tid=2;
            ack.err=0;
            pushAckToQueue(ack);
             retransTime=0;
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
        }
        // start_time = esp_timer_get_time();
        
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT:
        ESP_LOGI(TAG, "Receive publish message 0x%06" PRIx32, param->client_recv_publish_msg.opcode);
        ESP_LOGI(TAG, " opcode 0x%04x, src 0x%04x, dst 0x%04x",
             param->model_operation.ctx->recv_op, param->model_operation.ctx->addr, param->model_operation.ctx->recv_dst);
             uint8_t* recv_mssg=param->model_operation.msg;
             if(recv_mssg[0]==0 && recv_mssg[1]==1 && recv_mssg[2]==2){
                      ESP_LOGI(TAG, "bt_mesh_clear_rpl");
           bt_mesh_rx_reset_single(param->model_operation.ctx->addr);
             }
            cJSON * status_info=cJSON_CreateObject();
    cJSON_AddItemToObject(status_info,"vId" , cJSON_CreateNumber(param->model_operation.msg[0]));
    cJSON_AddItemToObject(status_info,"pId" , cJSON_CreateNumber(param->model_operation.msg[1]));
    cJSON_AddItemToObject(status_info,"unicastAddr" , cJSON_CreateNumber(param->model_operation.ctx->addr));
    cJSON_AddItemToObject(status_info,"infoType" , cJSON_CreateNumber(param->model_operation.msg[2]));
    char* cjson_status_info=cJSON_Print(status_info);
    postBleDeviceInfo(cjson_status_info);
    cJSON_Delete(status_info);
        break;
    case ESP_BLE_MESH_CLIENT_MODEL_SEND_TIMEOUT_EVT:
        ESP_LOGW(TAG, "Client message 0x%06" PRIx32 " timeout", param->client_send_timeout.opcode);
       
        if(retransTime==1)
         snd_mssg_to_vnd_srv(prev_vnd_id);
        else{
            retransTime=0;
        }
        break;
    default:
        break;
    }
}
uint8_t sendMessageToNode(uint16_t modelId,uint16_t deviceId,void* set_state){
    uint8_t err=0;
switch (modelId)
{
case ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI:
    
    break;
case ESP_BLE_MESH_MODEL_ID_SENSOR_CLI:
    break;
case ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI:
    ESP_LOGI(TAG, "light message send device Id %d",deviceId);
    err=ble_send_command(deviceId,set_state);
    break;
default:
    break;
}
return err;
}
static esp_err_t ble_mesh_init_t(void)
{
    // uint8_t match[2] = {0xdd, 0xdd};
    esp_err_t err = ESP_OK;
    // uint8_t net_key_arr[16];
    prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
    prov_key.app_idx = APP_KEY_IDX;
    memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));



//// custom model only to be used in future
    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_client_callback(example_ble_mesh_config_client_cb);
    esp_ble_mesh_register_light_client_callback(example_ble_mesh_lightness_client_model_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }

    // err = esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to set matching device uuid (err %d)", err);
    //     return err;
    // }
    err = esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add local AppKey (err %d)", err);
        return err;
    }
    // memset(net_key_arr, NET_KEY_Octet, sizeof(net_key_arr));
    // uint8_t* ntky=esp_ble_mesh_provisioner_get_local_net_key(prov_key.net_idx);
    //   ESP_LOGE(TAG, "net key alloc");
    // for(int i=0;i<16;i++){
    //     ESP_LOGE(TAG, "%d",ntky[i]);
    // }
    err = esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable mesh provisioner (err %d)", err);
        return err;
    }
 err =esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
                    ESP_BLE_MESH_VND_MODEL_ID_CLIENT, CID_ESP);;
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to bind AppKey to vendor client ");
            }
            else{
                ESP_LOGE(TAG, "success to bind AppKey to vendor client addr: 0x%04x",prov_key.app_idx);

            }
     err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROV_OWN_ADDR, prov_key.app_idx,
                    ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI, ESP_BLE_MESH_CID_NVAL);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Provisioner bind local model appkey failed");
                
            }
    else{
                ESP_LOGE(TAG, "success to bind AppKey to light client addr: 0x%04x",prov_key.app_idx);

            }
deleteBleDevice(10);
deleteBleDevice(14);
    //    err= esp_ble_mesh_model_subscribe_group_addr(PROV_OWN_ADDR,ESP_BLE_MESH_CID_NVAL,ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_CLI,0xC000);
        ESP_LOGE(TAG, "group addr subscribe: %d",err);
   esp_timer_create_args_t unprov_pkg_args = {
		.callback = endPkgRec,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "RecTimer"
	};
	esp_timer_create(&unprov_pkg_args, &pkgTimer);

    //   ESP_LOGI(TAG, "bt_mesh_clear_rpl");
    //         bt_mesh_clear_rpl();
    //         bt_mesh_clear_seq();
    return err;
}

void ble_mesh_init(void)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing...");
ESP_LOGI(TAG,"transmit pwr %d",esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
 err=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, 7);
ESP_LOGI(TAG,"transmit pwr %d and set err ",esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
//    err = nvs_flash_init();
//    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
//        ESP_ERROR_CHECK(nvs_flash_erase());
//        err = nvs_flash_init();
//    }
//    ESP_ERROR_CHECK(err);
    ble_mssg_queue_setup();
    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init_t();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    esp_ble_mesh_client_model_init(&vnd_models[0]);

}
