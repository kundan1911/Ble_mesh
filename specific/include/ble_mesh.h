#ifndef BLE_MESH_H_
#define BLE_MESH_H_
#include<stdint.h>
#include<string.h>


void ble_mesh_init();
int ble_send_command(int device_id,void*);
int ble_send_command_color(int device_id,int data);
int ble_send_onoff_command(int device_id,int data);
void start_provisioner_provisioning();
void clear_all_recv_pkg_arr();
// int snd_mssg_to_clt(uint16_t ,uint16_t );
int snd_mssg_to_vnd_srv(uint16_t);
void get_node_on_bus_enable_call(uint8_t );
_Bool get_all_node_on_bus();
// void assignModelNdKey(uint8_t modelType);
uint8_t sendMessageToNode(uint16_t ,uint16_t,void* );
#endif
