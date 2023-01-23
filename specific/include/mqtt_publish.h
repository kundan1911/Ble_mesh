#ifndef INCLUDE_HTTP_CLIENT_H_
#define INCLUDE_HTTP_CLIENT_H_

#include<stdint.h>
#include "uart_protocol.h"

int publish_control_gear_long_addr(uint8_t info_type,uint32_t long_addr);
int publish_control_gear_detail(control_gear_details_t details);
int postStatistic(char * publish_data);
int publish_switch_scene(uint8_t sub_device_id,uint8_t scene_id, uint8_t switch_no,uint8_t tap);
int8_t http_post_device_status(char * publish_data);
int postDiagnostic(char * publish_data);
int postBleDeviceInfo(char * publish_data);

#endif
