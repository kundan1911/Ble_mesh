#ifndef BLE_MESH_H_
#define BLE_MESH_H_
#include<stdint.h>
#include<string.h>




void ble_mesh_init();
int ble_send_command(int device_id,int element,int data);
int ble_send_command_color(int device_id,int element,int data);

void start_provisioner_provisioning();
void clear_all_recv_pkg_arr();
int snd_mssg_to_clt(uint16_t ,uint16_t );

#endif
