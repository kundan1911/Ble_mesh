#ifndef BLDC_FAN_H_
#define BLDC_FAN_H_
#include<stdint.h>
#include<string.h>

void pulse_generator(int64_t current_time);
int set_bldc_fan_speed(uint8_t sub_device_id ,uint8_t port,uint8_t pulse_count,uint16_t pulse_time);
#endif
