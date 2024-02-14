#ifndef BLE_MESH_H_
#define BLE_MESH_H_
#include<stdint.h>
#include<string.h>


void ble_mesh_init();
int ble_send_command(uint16_t device_id,void*);
int ble_send_command_color(int device_id,int data);
int ble_send_onoff_command(int device_id,int data);
void start_provisioner_provisioning();
void clear_all_recv_pkg_arr();
// int snd_mssg_to_clt(uint16_t ,uint16_t );
int snd_mssg_to_vnd_srv(uint16_t);
void get_node_on_bus_enable_call(uint8_t ,uint8_t);
void control_gear_search_ble_enable(uint8_t );
_Bool get_all_node_on_bus();
// void assignModelNdKey(uint8_t modelType);
void pushProvNodeToMqtt(uint16_t);
uint8_t mqtt_snd_mssg_to_vnd_srv(uint8_t,uint16_t);
uint8_t sendMessageToNode(uint16_t ,uint16_t,void* );
uint8_t deleteBleDevice(uint16_t);
uint8_t addProvisionedNode(const uint8_t *addr,const uint8_t* uuid,uint16_t unicast_addr,uint16_t net_idx,const uint8_t* dev_key ,uint8_t addrType);
#endif
