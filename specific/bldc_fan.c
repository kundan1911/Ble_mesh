
#include "bldc_fan.h"

#include<stdint.h>
#include<string.h>

#include "uart_protocol.h"
#include "esp_log.h"
#include "esp_timer.h"

uint8_t pulse_count = 0;

static const char *TAG = "bldc_fan";




void send_pulse_to_port(basic_port_status_t* port_status,uint8_t sub_device_id ,uint8_t port,int64_t current_time)
{

  if(port_status->change_pulse_time < current_time && port_status->pulse_start_time < current_time)
  {
    ESP_LOGI(TAG, "port_number id - %d pulse_count - %d, aa - %d, bb- %d", port,port_status->pulse_count, port_status->pulse_count % 2,port_status->arc_level == 60 ? 0:60);
    if(port_status->pulse_count % 2 == 1)
    {
      ae_change_states_pulse(sub_device_id,port+1,60);
    }
    else
    {
      ae_change_states_pulse(sub_device_id,port+1,0);
    }


    port_status->pulse_count--;
    port_status->change_pulse_time = current_time+port_status->pulse_time_us;
  }
}

void pulse_generator(int64_t current_time){

for (int j=0; j<NUMBER_OF_MAX_RS485_DEVICE; j++)
{ 
  if(device_status.basic[j].active != true)
  {
    continue;
  }
  for(int i=0;i<NUMBER_OF_MAX_SWITCH_INPUT;i++)
  {
    if(device_status.basic[j].switch_config[i].pulse_count > 0)
    {
      send_pulse_to_port(&device_status.basic[j].switch_config[i],j,i,current_time);
    }
    
  }
 }
}


int set_bldc_fan_speed(uint8_t sub_device_id,uint8_t port,uint8_t pulse_count,uint16_t pulse_time)
{
  ESP_LOGI(TAG, "port_number id - %d pulse_count - %d, current state - %d, current pulse- %d", port,pulse_count,device_status.basic[sub_device_id].switch_config[port].arc_level,device_status.basic[sub_device_id].switch_config[port].pulse_count);
  if(sub_device_id > NUMBER_OF_MAX_RS485_DEVICE || pulse_count > 50 || pulse_time > 10000)
  {
    return -2;
  }
  port = port -1;
  device_status.basic[sub_device_id].switch_config[port].pulse_start_time = esp_timer_get_time();
  if(device_status.basic[sub_device_id].switch_config[port].arc_level != 60 ||  device_status.basic[sub_device_id].switch_config[port].pulse_count > 0)
  {
    ESP_LOGI(TAG, "afffffffffffffaa");
    ae_change_states(sub_device_id, port+1,60);
     device_status.basic[sub_device_id].switch_config[port].pulse_start_time += 5000000;
  }

   device_status.basic[sub_device_id].switch_config[port].pulse_count = pulse_count*2;

   device_status.basic[sub_device_id].switch_config[port].pulse_time_us = pulse_time*1000;

   device_status.basic[sub_device_id].switch_config[port].change_pulse_time =  device_status.basic[sub_device_id].switch_config[port].pulse_start_time +  device_status.basic[sub_device_id].switch_config[port].pulse_time_us;

  return 1;

}