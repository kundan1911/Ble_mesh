/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "settings.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
// #include "btc_ble_mesh_lighting_model.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "mesh_main.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "board.h"
#include "esp_bt.h"
#include "ble_mesh_example_init.h"
#include "uart_handler.h"
#include "driver/gpio.h"

#define TAG "lightNode"

#define BUTTON_GPIO 4
#define CID_ESP 0x05f1
#define provisioner_addr 0x0001
#define MSG_SEND_TTL        3
#define MSG_SEND_REL        false
#define MSG_TIMEOUT         0
#define MSG_ROLE            ROLE_NODE
#define infoType            1
#define ESP_BLE_MESH_VND_MODEL_ID_CLIENT    0x0000
#define ESP_BLE_MESH_VND_MODEL_ID_SERVER    0x0001

#define ESP_BLE_MESH_VND_MODEL_OP_SEND      ESP_BLE_MESH_MODEL_OP_3(0x00, CID_ESP)
#define ESP_BLE_MESH_VND_MODEL_OP_STATUS    ESP_BLE_MESH_MODEL_OP_3(0x01, CID_ESP)


nvs_handle_t store_nvs_handle;
static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static esp_ble_mesh_client_t onoff_client;
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
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};
static esp_ble_mesh_light_ctl_state_t ctl_state;
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);

/* Light CTL Server related context */
// ESP_BLE_MESH_MODEL_PUB_DEFINE(ctl_pub, 2 + 9, ROLE_NODE);
// ESP_BLE_MESH_MODEL_PUB_DEFINE(light_cli_pub, 9 + 1, ROLE_NODE);
static esp_ble_mesh_light_ctl_srv_t ctl_server = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .state = &ctl_state,
};
/* vendor Server related context */
static esp_ble_mesh_model_op_t vnd_op[] = {
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_SEND, 2),
    ESP_BLE_MESH_MODEL_OP(ESP_BLE_MESH_VND_MODEL_OP_STATUS, 2),
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t vnd_models[] = {
    ESP_BLE_MESH_VENDOR_MODEL(CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,
    vnd_op, NULL, NULL),
};
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client),
//   ESP_BLE_MESH_MODEL_LIGHT_CTL_CLI(&light_cli_pub, &ctl_client),

};
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, vnd_models),
  
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

/* Disable OOB security for SILabs Android app */
static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
#if 0
    .output_size = 4,
    .output_actions = ESP_BLE_MESH_DISPLAY_NUMBER,
    .input_actions = ESP_BLE_MESH_PUSH,
    .input_size = 4,
#else
    .output_size = 0,
    .output_actions = 0,
#endif
};
uint8_t prev_temp=0;
uint8_t prev_bright=0;


static esp_err_t vendor_set_msg_common_tt(esp_ble_mesh_msg_ctx_t *common,
                                                 uint16_t unicast)
{
    if (!common ) {
        return ESP_ERR_INVALID_ARG;
    }
 
    common->net_idx = 0;
    common->app_idx = 0;
    common->addr = unicast;
    common->send_ttl = MSG_SEND_TTL;
    common->send_rel = MSG_SEND_REL;
    return ESP_OK;
}

void button_config(){
gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
}

esp_err_t send_req_to_rx_reset(){
     ESP_LOGI(TAG, "sending dev key request  %d",bt_mesh_is_provisioned());
esp_ble_mesh_msg_ctx_t ctx={0};
vendor_set_msg_common_tt(&ctx,provisioner_addr);
uint8_t mssg[]={0x00,1,2};

   esp_err_t  err = esp_ble_mesh_server_model_send_msg(vnd_models,&ctx, ESP_BLE_MESH_VND_MODEL_OP_STATUS,3, &mssg);
            if (err) {
                ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_VND_MODEL_OP_STATUS);
            }
    else{
          ESP_LOGI(TAG, "mssg send to the server vendor");
    }
    return err;
}
void storeKeysIntoNvs(uint16_t netIdx,uint16_t addr){
        const uint8_t * netkey= esp_ble_mesh_node_get_local_net_key(netIdx);
        
        if(netkey !=NULL){
             ESP_LOGI(TAG,"Netkey-----");
            for(int i=0;i<16;i++){
                ESP_LOGI(TAG,"%d",netkey[i]);
            }
             ESP_LOGI(TAG, "Unicast addr assigned %d",addr);
    nvs_set_blob(store_nvs_handle,"netKey",netkey,16);
    nvs_commit(store_nvs_handle);
    nvs_set_u16(store_nvs_handle, "uniAddr", addr);
    nvs_commit(store_nvs_handle);
    netkey=esp_ble_mesh_node_get_local_app_key(0);
     ESP_LOGI(TAG,"appkey-----");
     if(netkey!=NULL){
            for(int i=0;i<16;i++){
                ESP_LOGI(TAG,"%d",netkey[i]);
            }
    nvs_set_blob(store_nvs_handle,"pvappKey",netkey,16);
    nvs_commit(store_nvs_handle);
     }
     else{
         ESP_LOGI(TAG,"not found appkey-----");
     }
        }
        else{
 ESP_LOGI(TAG,"not found netkey-----");
        }
    send_req_to_rx_reset();
}


static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08x", flags, iv_index);
    board_led_operation(LED_G, LED_OFF);
    // provision_blink();
    
    // storeKeysIntoNvs(net_idx,addr);
}
esp_err_t prov_to_prev_master(){
            uint16_t uniAddr;
            uint8_t prev_net_key[16];
            uint8_t len=16;
             ESP_LOGI(TAG, "provisioning to previous master");
             
            nvs_get_u16(store_nvs_handle,"uniAddr",&uniAddr);
            ESP_LOGI(TAG, "prev unicast %d",uniAddr);
            nvs_get_blob(store_nvs_handle,"netKey",prev_net_key,&len);
            ESP_LOGI(TAG,"netkey-----");
            for(int i=0;i<16;i++){
                ESP_LOGI(TAG,"%d",prev_net_key[i]);

            }
       esp_err_t err= bt_mesh_provision(prev_net_key,0,
                      1,  0, uniAddr,
                     prev_net_key);
              nvs_get_blob(store_nvs_handle,"pvappKey",prev_net_key,&len);   
                 ESP_LOGI(TAG,"pvappkey-----");
            for(int i=0;i<16;i++){
                ESP_LOGI(TAG,"%d",prev_net_key[i]);
            }  
                  err= esp_ble_mesh_node_add_local_app_key(prev_net_key,0,0);
    // err=esp_ble_mesh_node_bind_app_key_to_local_model( esp_ble_mesh_get_primary_element_address(),ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV,0);
    //         if(!err){
    //             ESP_LOGI(TAG, "light binding success");
    //         }
    uint16_t curr_addr=esp_ble_mesh_get_primary_element_address();
    err=esp_ble_mesh_node_bind_app_key_to_local_model(curr_addr,CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,0);
            if(!err){
                ESP_LOGI(TAG, "vendor server binding success");
            }
    
         err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI,0);
            if(!err){
                ESP_LOGI(TAG, "generic client binding success");
            }
        ESP_LOGI(TAG,"previous master provision %d",err);
        return err;
}

int onoffstate=0;
int tid=0;
void example_ble_mesh_send_gen_onoff_set(uint16_t addr)
{
    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
    common.model = onoff_client.model;
    common.ctx.net_idx = 0;
    common.ctx.app_idx = 0;
    common.ctx.addr = 22;   /* to all nodes */
    common.ctx.send_ttl = 3;
    common.ctx.send_rel = false;
    common.msg_timeout = 0;     /* 0 indicates that timeout value from menuconfig will be used */
    common.msg_role = ROLE_NODE;

    set.onoff_set.op_en = false;
    set.onoff_set.onoff = onoffstate;
    set.onoff_set.tid = tid++;

    err = esp_ble_mesh_generic_client_set_state(&common, &set);
    if (err) {
        ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
        return;
    }

    onoffstate = !onoffstate;
    // mesh_example_info_store(); /* Store proper mesh example info */
}

esp_err_t snd_light_status_to_clt(esp_ble_mesh_model_t *model,esp_ble_mesh_msg_ctx_t *ctx){
    uint32_t opcode;
    esp_err_t err;
    opcode = ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_STATUS;
   uint8_t mssg_data[5];
   //as per requirement the mssg_data must be filled
   mssg_data[0]=1;
   mssg_data[1]=2;
   mssg_data[2]=2;
  mssg_data[3]=2;


     err = esp_ble_mesh_server_model_send_msg(model, ctx, opcode,4, mssg_data);
            if (err) {
                ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS);
            }
    else{
          ESP_LOGI(TAG, "ack send to the client");
    }
    return err;

}

esp_err_t snd_vendor_status_to_clt(esp_ble_mesh_model_t *model,esp_ble_mesh_msg_ctx_t *ctx){
    uint32_t opcode;
    opcode = ESP_BLE_MESH_VND_MODEL_OP_STATUS;
   uint8_t mssg_data[3];
   ctx->addr=22;
   ctx->net_idx=0;
   mssg_data[0]=composition.pid;
   mssg_data[1]=composition.vid;
    mssg_data[2]=infoType;
   esp_err_t  err = esp_ble_mesh_server_model_send_msg(model,ctx, opcode,3, mssg_data);
            if (err) {
                ESP_LOGE(TAG, "Failed to send message 0x%06x", ESP_BLE_MESH_VND_MODEL_OP_STATUS);
            }
    else{
          ESP_LOGI(TAG, "mssg send to the server vendor");
    }
    return err;

}
static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
            param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
            param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT");
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr,
            param->node_prov_complete.flags, param->node_prov_complete.iv_index);
            // send_req_for_dev_key();

        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_RESET_EVT");
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }
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
    common->ctx.net_idx = 0;
    common->ctx.app_idx = 0;
    common->ctx.addr = unicast;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;

    return ESP_OK;
}
static void example_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Generic client, event %u, error code %d, opcode is 0x%04x",
        event, param->error_code, param->params->opcode);

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET) {
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET, onoff %d", param->status_cb.onoff_status.present_onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET, onoff %d", param->status_cb.onoff_status.present_onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT");
        if (param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET) {
            /* If failed to get the response of Generic OnOff Set, resend Generic OnOff Set  */
            example_ble_mesh_send_gen_onoff_set(22);
        }
        break;
    default:
        break;
    }
}


static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
        switch (param->ctx.recv_op) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                param->value.state_change.appkey_add.net_idx,
                param->value.state_change.appkey_add.app_idx);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            esp_err_t err;
            // esp_ble_mesh_node_bind_app_key_to_local_model( esp_ble_mesh_get_primary_element_address(),ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV,0);
            // if(!err){
            //     ESP_LOGI(TAG, "light binding success");
            // }
    uint16_t curr_addr=esp_ble_mesh_get_primary_element_address();
             err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,0);
            if(!err){
                ESP_LOGI(TAG, "vendor server binding success");
            }

            err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI,0);
            if(!err){
                ESP_LOGI(TAG, "generic client binding success");
            }
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_app_bind.element_addr,
                param->value.state_change.mod_app_bind.app_idx,
                param->value.state_change.mod_app_bind.company_id,
                param->value.state_change.mod_app_bind.model_id);
                storeKeysIntoNvs(0,param->value.state_change.mod_app_bind.element_addr);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD");
            ESP_LOGI(TAG, "elem_addr 0x%04x, sub_addr 0x%04x, cid 0x%04x, mod_id 0x%04x",
                param->value.state_change.mod_sub_add.element_addr,
                param->value.state_change.mod_sub_add.sub_addr,
                param->value.state_change.mod_sub_add.company_id,
                param->value.state_change.mod_sub_add.model_id);
            break;
        default:
            break;
        }
    }
}

uint16_t prev_id;

static void example_ble_mesh_custom_model_cb(esp_ble_mesh_model_cb_event_t event,
                                             esp_ble_mesh_model_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        ESP_LOGI(TAG, "received vendor pkg");
        uint8_t* messg=param->model_operation.msg;
        uint8_t lw=messg[0];
        uint8_t up=messg[1];
        uint16_t grpAddr=((uint16_t)up << 8) | lw;
        uint16_t curr_addr=esp_ble_mesh_get_primary_element_address();
        esp_err_t err= esp_ble_mesh_model_subscribe_group_addr(curr_addr,ESP_BLE_MESH_CID_NVAL,ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV,grpAddr);
        ESP_LOGE(TAG, "group addr subscribe: %d and curr addr %d",err,grpAddr);
        if (param->model_operation.opcode == ESP_BLE_MESH_VND_MODEL_OP_SEND) {
            //  protocolUARTSend3(100,100);
                snd_vendor_status_to_clt(param->model_operation.model, param->model_operation.ctx);
 
        }
        
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        if (param->model_send_comp.err_code) {
            ESP_LOGE(TAG, "Failed to send message 0x%06" PRIx32, param->model_send_comp.opcode);
            break;
        }
        ESP_LOGI(TAG, "Send 0x%06" PRIx32, param->model_send_comp.opcode);
        break;
    default:
        break;
}
    }
esp_err_t node_provison_enble()
{
     esp_err_t err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
        return err;
    }
    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    return ESP_OK;
}
void node_task()
{
  
    uint32_t read_data;
  
    nvs_get_u32(store_nvs_handle, "data", &read_data);
    ESP_LOGI(TAG, "Successfully read the value= %d stored at data key location", read_data);
    if(read_data >= 10)
    {
        nvs_set_u32(store_nvs_handle, "data", 0);
        nvs_commit(store_nvs_handle);
    }
    vTaskDelay(300);
    nvs_set_u32(store_nvs_handle, "data", 0);
    nvs_commit(store_nvs_handle);

    ESP_LOGI(TAG, "Successfully read the value = %d", read_data);
    //  protocolUARTSend();
    // vTaskDelay(250);
    //  protocolUARTSend2(0x28);
    // vTaskDelay(100);
    if (read_data > 3)
    {
        
        ESP_LOGI(TAG, "provisioning mode activated");

        esp_ble_mesh_node_local_reset();
        node_provison_enble();
        // provision_blink();
        vTaskDelay(1*6000);
       
        if (!bt_mesh_is_provisioned())
        {
           prov_to_prev_master();
             uint8_t err = esp_ble_mesh_node_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
            if (err == ESP_OK)
            {
                ESP_LOGE(TAG, "successfully disabled (err %d)", err);

            }
            else{
            ESP_LOGE(TAG, "failed to disable mesh node (err %d)", err);
            }
            bt_mesh_clear_seq();
        }
       
        ESP_LOGE(TAG, "return task  ");
    }
    while(1)
    {
         int button_state = gpio_get_level(BUTTON_GPIO);

        if (button_state == 0) {
            printf("Button is pressed\n");
            // ble_send_command(22);
            example_ble_mesh_send_gen_onoff_set(22);
        } else {
            printf("Button is not pressed\n");
        }

         vTaskDelay(500);
    }
}
static esp_err_t ble_mesh_init(uint8_t ifbind)
{
   
    esp_err_t err = ESP_OK;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
        esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    esp_ble_mesh_register_custom_model_callback(example_ble_mesh_custom_model_cb);
    err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }
    // ESP_LOGI(TAG,"provsiioned %d",ifbind);
    if(ifbind){
        uint16_t curr_addr=esp_ble_mesh_get_primary_element_address();
//  err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV,0);
//             if(!err){
//                 ESP_LOGI(TAG, "light binding success");
//             }
    
    err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,CID_ESP, ESP_BLE_MESH_VND_MODEL_ID_SERVER,0);
            if(!err){
                ESP_LOGI(TAG, "vendor server binding success");
            }
             err=esp_ble_mesh_node_bind_app_key_to_local_model( curr_addr,ESP_BLE_MESH_CID_NVAL, ESP_BLE_MESH_MODEL_ID_GEN_ONOFF_CLI,0);
            if(!err){
                ESP_LOGI(TAG, "generic client binding success");
            }
        //      err= esp_ble_mesh_model_subscribe_group_addr(curr_addr,ESP_BLE_MESH_CID_NVAL,ESP_BLE_MESH_MODEL_ID_LIGHT_CTL_SRV,0xC000);
        // ESP_LOGE(TAG, "group addr subscribe: %d and curr addr %d",err,curr_addr);
    }
     ESP_LOGI(TAG,"provsiioned %d",ifbind);
    ESP_LOGI(TAG, "BLE Mesh Node initialized ");
    // send_req_for_dev_key(5);
    bt_mesh_clear_rpl();
    
//     ESP_LOGI(TAG,"transmit pwr %d",esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
//  err=esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, 7);
// ESP_LOGI(TAG,"transmit pwr %d and set err ",esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV));
    return err;
}

void app_main(void)
{
    esp_err_t err;
    ESP_LOGI(TAG, "Initializing...");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
 nvs_open("storage", NVS_READWRITE, &store_nvs_handle);
 uint32_t read_data;
  
    nvs_get_u32(store_nvs_handle, "data", &read_data);
    ESP_LOGI(TAG, "Successfully read the value= %d stored at data key location", read_data);
    read_data++;
    nvs_set_u32(store_nvs_handle, "data", read_data);
    nvs_commit(store_nvs_handle);


// board_init();
uart_setup();
// ledc_init();
    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    ble_mesh_get_dev_uuid(dev_uuid);

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init(bt_mesh_is_provisioned());
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }
    button_config();
    xTaskCreate(&node_task, "node_task", 2048, NULL, 5, NULL);
}
