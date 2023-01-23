/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <nvs_header.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "proxy_server.h"
#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "freertos/FreeRTOS.h"
#include "mesh_main.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "board.h"
#include "ble_mesh_example_init.h"
#include "esp_ble_mesh_lighting_model_api.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "local_operation.h"
#include "net.h"
#include "settings.h"
// #include "driver/gpio_ll.h"

#define TAG "EXAMPLE"

#define CID_ESP 0x05f1
#define button_pin GPIO_NUM_5

extern struct _led_state led_state[3];

static uint8_t dev_uuid[16] = {0xdd, 0xdd};

nvs_handle_t store_nvs_handle;

//holds info of your client model(initialized by stack)
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

/* Light CTL state related context */
static esp_ble_mesh_light_ctl_state_t ctl_state;

/* Light CTL Server related context */
ESP_BLE_MESH_MODEL_PUB_DEFINE(ctl_pub, 2 + 9, ROLE_NODE);
static esp_ble_mesh_light_ctl_srv_t ctl_server = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .state = &ctl_state,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(LEVEL_pub_0, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_level_srv_t LEVEL_server_0 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
};
ESP_BLE_MESH_MODEL_PUB_DEFINE(POWER_LEVEL_pub_1, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_power_level_srv_t POWER_LEVEL_server_1 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_1, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_1 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_2, 2 + 3, ROLE_NODE);
static esp_ble_mesh_gen_onoff_srv_t onoff_server_2 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_cli_pub, 2 + 1, ROLE_NODE);

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_LIGHT_CTL_SRV(&ctl_pub, &ctl_server),
     ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoff_cli_pub, &onoff_client)
};

static esp_ble_mesh_model_t extend_model_0[] = {
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_1, &onoff_server_1),
};

static esp_ble_mesh_model_t extend_model_1[] = {
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_2, &onoff_server_2),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
    ESP_BLE_MESH_ELEMENT(0, extend_model_0, ESP_BLE_MESH_MODEL_NONE),
    ESP_BLE_MESH_ELEMENT(0, extend_model_1, ESP_BLE_MESH_MODEL_NONE),
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

// void my_gpio_config()
// {
//     gpio_config_t io_conf;
//     // disable interrupt
//     io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
//     // set as output mode
//     io_conf.mode = GPIO_MODE_INPUT;
//     // bit mask of the pins that you want to set,e.g.GPIO18/19
//     io_conf.pin_bit_mask = GPIO_SEL_5;
//     // disable pull-down mode
//     io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
//     // disable pull-up mode
//     io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
//     // configure GPIO with the given settings
//     gpio_config(&io_conf);
// }
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    ESP_LOGI(TAG, "flags: 0x%02x, iv_index: 0x%08x", flags, iv_index);
    uint8_t* retrn_netkey=bt_mesh_node_get_local_net_key(net_idx);
    ESP_LOGE(TAG, "net key alloc");
    for(int i=0;i<16;i++){
        ESP_LOGE(TAG, "%d",retrn_netkey[i]);
    }
    esp_err_t err=nvs_set_blob(store_nvs_handle,"pstNtkey",retrn_netkey,16);
    if(err==ESP_OK){
         ESP_LOGE(TAG, "success to store netkey");
    }
    nvs_commit(store_nvs_handle);
    board_led_operation(LED_G, LED_OFF);
}

/*
void example_ble_mesh_send_gen_onoff_set(void)
{
    esp_ble_mesh_generic_client_set_state_t set = {0};
    esp_ble_mesh_client_common_param_t common = {0};
    esp_err_t err = ESP_OK;

    common.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
    common.model = onoff_client.model;
    nvs_get_u16(store_nvs_handle, "pstNtid", &common.ctx.net_idx);
    nvs_get_u16(store_nvs_handle, "pstApid", &common.ctx.app_idx);
    // to this unicast addr
    common.ctx.addr = 0x0011;   
     
    common.ctx.send_ttl = 3;
    common.ctx.send_rel = false;
     // 0 indicates that timeout value from menuconfig will be used 
    common.msg_timeout = 0;    
    
    common.msg_role = ROLE_NODE;

    set.onoff_set.op_en = false;
    set.onoff_set.onoff = 1;
    set.onoff_set.tid = 1;

    err = esp_ble_mesh_generic_client_set_state(&common, &set);
    if (err) {
        ESP_LOGE(TAG, "Send Generic OnOff Set Unack failed");
        
    }
    else{
        ESP_LOGE(TAG, "successfully sended Generic OnOff Set Unack ");
    }
return;
}
*/



static void example_change_led_state(esp_ble_mesh_model_t *model,
                                     esp_ble_mesh_msg_ctx_t *ctx, uint8_t onoff)
{
    uint16_t primary_addr = esp_ble_mesh_get_primary_element_address();
    uint8_t elem_count = esp_ble_mesh_get_element_count();
    struct _led_state *led = NULL;
    uint8_t i;
    ESP_LOGI(TAG, "change_led_state ");

    if (ESP_BLE_MESH_ADDR_IS_UNICAST(ctx->recv_dst))
    {
        for (i = 0; i < elem_count; i++)
        {
            if (ctx->recv_dst == (primary_addr + i))
            {
                led = &led_state[i];
                board_led_operation(led->pin, onoff);
            }
        }
    }
    else if (ESP_BLE_MESH_ADDR_IS_GROUP(ctx->recv_dst))
    {
        if (esp_ble_mesh_is_model_subscribed_to_group(model, ctx->recv_dst))
        {
            led = &led_state[model->element->element_addr - primary_addr];
            board_led_operation(led->pin, onoff);
        }
    }
    else if (ctx->recv_dst == 0xFFFF)
    {
        led = &led_state[model->element->element_addr - primary_addr];
        board_led_operation(led->pin, onoff);
    }
}

static void example_handle_gen_onoff_msg(esp_ble_mesh_model_t *model,
                                         esp_ble_mesh_msg_ctx_t *ctx,
                                         esp_ble_mesh_server_recv_gen_onoff_set_t *set)
{
    esp_ble_mesh_gen_onoff_srv_t *srv = model->user_data;

    switch (ctx->recv_op)
    {
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
        esp_ble_mesh_server_model_send_msg(model, ctx,
                                           ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        break;
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
    case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK:
        if (set->op_en == false)
        {
            srv->state.onoff = set->onoff;
        }
        else
        {
            /* TODO: Delay and state transition */
            srv->state.onoff = set->onoff;
        }
        if (ctx->recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET)
        {
            esp_ble_mesh_server_model_send_msg(model, ctx,
                                               ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
        }
        esp_ble_mesh_model_publish(model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
                                   sizeof(srv->state.onoff), &srv->state.onoff, ROLE_NODE);
        example_change_led_state(model, ctx, srv->state.onoff);
        break;
    default:
        break;
    }
}

/*
static void example_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                               esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Generic client, event %u, error code %d, opcode is 0x%04" PRIx32,
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
           // If failed to get the response of Generic OnOff Set, resend Generic OnOff Set  
            example_ble_mesh_send_gen_onoff_set();
        }
        break;
    default:
        break;
    }
}
*/
static void example_ble_mesh_provisioning_cb(esp_ble_mesh_prov_cb_event_t event,
                                             esp_ble_mesh_prov_cb_param_t *param)
{
    ESP_LOGI(TAG, "mesh_prov_evt 0x%02x", event);
    switch(event){
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

static void example_ble_mesh_lighting_server_cb(esp_ble_mesh_lighting_server_cb_event_t event,
                                                esp_ble_mesh_lighting_server_cb_param_t *param)
{
    ESP_LOGI(TAG, "event 0x%02x, opcode 0x%04x, src 0x%04x, dst 0x%04x",
             event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

    switch (event)
    {
    case ESP_BLE_MESH_LIGHTING_SERVER_STATE_CHANGE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHTING_SERVER_STATE_CHANGE_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_LIGHT_CTL_SET_UNACK)
        {
            ESP_LOGI(TAG, "lightness 0x%02x , temperature - %d.", param->value.state_change.ctl_set.lightness, param->value.state_change.ctl_set.temperature);
            light_driver_set(param->value.state_change.ctl_set.lightness, param->value.state_change.ctl_set.temperature);
            // example_change_led_state(param->model, &param->ctx, param->value.state_change.onoff_set.onoff);
        }
        break;
    case ESP_BLE_MESH_LIGHTING_SERVER_RECV_GET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHTING_SERVER_RECV_GET_MSG_EVT");

        break;
    case ESP_BLE_MESH_LIGHTING_SERVER_RECV_SET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_LIGHTING_SERVER_RECV_SET_MSG_EVT");

        break;
    default:
        ESP_LOGE(TAG, "Unknown Generic Server event 0x%02x", event);
        break;
    }
}

static void example_ble_mesh_generic_server_cb(esp_ble_mesh_generic_server_cb_event_t event,
                                               esp_ble_mesh_generic_server_cb_param_t *param)
{
    esp_ble_mesh_gen_onoff_srv_t *srv;
    ESP_LOGI(TAG, "event 0x%02x, opcode 0x%04x, src 0x%04x, dst 0x%04x",
             event, param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

    switch (event)
    {
    case ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK)
        {
            ESP_LOGI(TAG, "onoff 0x%02x , element addr - %d.", param->value.state_change.onoff_set.onoff, param->model->element->element_addr);
            example_change_led_state(param->model, &param->ctx, param->value.state_change.onoff_set.onoff);
        }
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK)
        {
            ESP_LOGI(TAG, "level 0x%d , element addr - %d.", param->value.state_change.level_set.level, param->model->element->element_addr);
            // board_led_operation_level(param->value.state_change.level_set.level);
            // example_change_led_state(param->model, &param->ctx, param->value.state_change.onoff_set.onoff);
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET)
        {
            srv = param->model->user_data;
            ESP_LOGI(TAG, "onoff 0x%02x", srv->state.onoff);
            example_handle_gen_onoff_msg(param->model, &param->ctx, NULL);
        }
        break;
    case ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT");
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK)
        {
            ESP_LOGI(TAG, "onoff 0x%02x, tid 0x%02x, element addr - %d.", param->value.set.onoff.onoff, param->value.set.onoff.tid, param->model->element->element_addr);
            if (param->value.set.onoff.op_en)
            {
                ESP_LOGI(TAG, "trans_time 0x%02x, delay 0x%02x",
                         param->value.set.onoff.trans_time, param->value.set.onoff.delay);
            }
            example_handle_gen_onoff_msg(param->model, &param->ctx, &param->value.set.onoff);
        }
        break;
    default:
        ESP_LOGE(TAG, "Unknown Generic Server event 0x%02x", event);
        break;
    }
}

static void example_ble_mesh_config_server_cb(esp_ble_mesh_cfg_server_cb_event_t event,
                                              esp_ble_mesh_cfg_server_cb_param_t *param)
{
    ESP_LOGI(TAG, "mesh_prov_evt %d", event);
    if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT)
    {
        switch (param->ctx.recv_op)
        {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD");
            ESP_LOGI(TAG, "net_idx 0x%04x, app_idx 0x%04x",
                     param->value.state_change.appkey_add.net_idx,
                     param->value.state_change.appkey_add.app_idx);
                     uint8_t* ext_pst_appkey=param->value.state_change.appkey_add.app_key;
                     ESP_LOGI(TAG, "after appkey");
            ESP_LOGI(TAG, "after for loop");
            nvs_set_u16(store_nvs_handle, "pstApid", param->value.state_change.appkey_add.app_idx);
            nvs_set_u16(store_nvs_handle, "pstNtid", param->value.state_change.appkey_add.net_idx);
            nvs_set_blob(store_nvs_handle,"pstApkey",ext_pst_appkey,16);
            nvs_commit(store_nvs_handle);
            ESP_LOG_BUFFER_HEX("AppKey", param->value.state_change.appkey_add.app_key, 16);
            break;
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND");
            ESP_LOGI(TAG, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
                     param->value.state_change.mod_app_bind.element_addr,
                     param->value.state_change.mod_app_bind.app_idx,
                     param->value.state_change.mod_app_bind.company_id,
                     param->value.state_change.mod_app_bind.model_id);
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

// void node_provison_enble(){

// }

static esp_err_t ble_mesh_init(void)
{
    esp_err_t err = ESP_OK;

    esp_ble_mesh_register_prov_callback(example_ble_mesh_provisioning_cb);
    esp_ble_mesh_register_config_server_callback(example_ble_mesh_config_server_cb);
    // esp_ble_mesh_register_generic_client_callback(example_ble_mesh_generic_client_cb);
    esp_ble_mesh_register_generic_server_callback(example_ble_mesh_generic_server_cb);
    esp_ble_mesh_register_lighting_server_callback(example_ble_mesh_lighting_server_cb);

     err = esp_ble_mesh_init(&provision, &composition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize mesh stack (err %d)", err);
        return err;
    }
   
    // ESP_LOGI(TAG, "BLE Mesh Node initialized");
    board_led_operation(LED_G, LED_ON);
    return err;
}

// void increment_counter(){
//     int32_t read_data;
//     int8_t part_success;
//     part_success=nvs_get_i32(int_nvs_handle, "data", &read_data);
//     if(read_data>3){
//         // provisioning begin
//         node_provison_enble();
//         ESP_LOGI(TAG,"provisioning mode activated");
//         //reset counter
//         nvs_set_i32(int_nvs_handle,"data",0);
//     }
// //    if(part_success==ESP_OK){
// //         ESP_LOGI(TAG,"Successfully read the value= %d stored at data key location",read_data);
// //     }
//     read_data++;
//     part_success=nvs_set_i32(int_nvs_handle,"data",read_data);
//     // if(part_success==ESP_OK){
//     //     ESP_LOGI(TAG,"Successfully written the value  at data key location %d",read_data);
//     // }
// }
esp_err_t node_provison_enble()
{
    //  int64_t Every3mins = esp_timer_get_time() + 3*(60000000);
   
     esp_err_t err = esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to enable mesh node (err %d)", err);
        return err;
    }
    ESP_LOGI(TAG, "BLE Mesh Node initialized");

    return ESP_OK;
}
// void *pvParameter
void node_task()
{
    // static bt_mesh_conn node_conn_dt={.handle=Node_unicast,.ref=1};
    // ESP_LOGI(TAG,"Executed1");
    
    uint32_t read_data;
    // my_gpio_config();
    // nvs_flash_init();
    nvs_get_u32(store_nvs_handle, "data", &read_data);
    ESP_LOGI(TAG, "Successfully read the value= %d stored at data key location", read_data);
    if(read_data > 100)
    {
        nvs_set_u32(store_nvs_handle, "data", 0);
        nvs_commit(store_nvs_handle);
    }

    read_data++;
    nvs_set_u32(store_nvs_handle, "data", read_data);
    nvs_commit(store_nvs_handle);
    vTaskDelay(400);
    nvs_set_u32(store_nvs_handle, "data", 0);
    nvs_commit(store_nvs_handle);

    ESP_LOGI(TAG, "Successfully read the value = %d", read_data);
    
    // if (read_data !=0)
    // {
    //     nvs_set_i32(int_nvs_handle, "data", 0);
    //     nvs_commit(int_nvs_handle);
    //     // ESP_LOGI(TAG,"Executed");
    // }
    // if (read_data > 3)
    // {
    //     ESP_LOGI(TAG, "provisioning mode activated");
    //     node_provison_enble();
    //     //  nvs_set_i32(int_nvs_handle,"data",0);
    // }
    // int64_t Every2Seconds = esp_timer_get_time() + 1000000;
    // esp_light_sleep_start();

    
        //  trigger_pulse=gpio_get_level(POWER_INPUT);
        //  ESP_LOGI(TAG,"GPI value %d",trigger_pulse);
        //    if(esp_timer_get_time()<Every2Seconds && (!trigger_pulse) ){
        //  if(read_data>3){
        //     nvs_set_i32(int_nvs_handle,"data",0);
        //      break;
        //  }
    if (read_data > 3)
    {
        ESP_LOGI(TAG, "provisioning mode activated");

        esp_ble_mesh_node_local_reset();
        node_provison_enble();

        vTaskDelay(1*6000);
        //*(bt_mesh.flags)->5(if provisioned),  1(not provisioned)
	// ESP_LOGI(TAG,"bt_mesh.flags%d ", *(bt_mesh.flags));
        if (!bt_mesh_is_provisioned())
        {
            *(bt_mesh.flags)=5;
            ESP_LOGE(TAG, "not provisioned");
            uint16_t nvs_netidx;
            uint16_t nvs_appidx;
            uint8_t nvs_appkey[16];
            uint8_t nvs_Netkey[16];
            size_t required_size = sizeof(nvs_appkey) / sizeof(nvs_appkey[0]);
            nvs_get_u16(store_nvs_handle, "pstApid", &nvs_appidx);
            ESP_LOGE(TAG, "after appid");
            nvs_get_u16(store_nvs_handle, "pstNtid", &nvs_netidx);
            ESP_LOGE(TAG, "after netid");
            nvs_get_blob(store_nvs_handle,"pstApkey",nvs_appkey,&required_size);
            ESP_LOGE(TAG, "after appkey");
            nvs_get_blob(store_nvs_handle,"pstNtkey",nvs_Netkey,&required_size);
            ESP_LOGE(TAG, "after netid");
            esp_err_t err=bt_mesh_node_local_net_key_add(nvs_netidx,nvs_Netkey);
            if (err == ESP_OK)
            {
                ESP_LOGE(TAG, "successfully added net key (err %d)", err);

            }
            err=bt_mesh_node_local_app_key_add(nvs_netidx,nvs_appidx,nvs_appkey);
            if (err == ESP_OK)
            {
                ESP_LOGE(TAG, "successfully added app key (err %d)", err);
            }
             err = esp_ble_mesh_node_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
            if (err == ESP_OK)
            {
                ESP_LOGE(TAG, "successfully disabled (err %d)", err);

            }
            else{
            ESP_LOGE(TAG, "failed to disable mesh node (err %d)", err);
            }
            
        }
        ESP_LOGE(TAG, "return task  ");
    }
    while(1)
    {
        // uint8_t trigger_pulse=gpio_get_level(button_pin);
        //  ESP_LOGI(TAG,"button value %d",trigger_pulse);
        //  if(!trigger_pulse){
        //     example_ble_mesh_send_gen_onoff_set();
        //  }
        vTaskDelay(50);
    }
}

void app_main(void)
{


esp_err_t err;

ESP_LOGI(TAG, "Initializing...");

board_init();

err = nvs_flash_init();
if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
}
ESP_ERROR_CHECK(err);
 nvs_open("storage", NVS_READWRITE, &store_nvs_handle);


err = bluetooth_init();
if (err) {
    ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
    return;
}

ble_mesh_get_dev_uuid(dev_uuid);

/* Initialize the Bluetooth Mesh Subsystem */
err = ble_mesh_init();
if (err) {
    ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
}
bt_mesh_set_device_name("PicoStone_2");
// creating an task that execute the function whose address is provided , name of task that
// will be utilized while debugginh , storage for the task
// a task is nothing but a infinite loop ,which executes the
    xTaskCreate(&node_task, "node_task", 4096, NULL, 5, NULL);
    // node_task();
}