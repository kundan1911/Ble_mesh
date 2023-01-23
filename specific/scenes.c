#include "scenes.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "pico_main.h"
#include "ex_eeprom.h"
#include "rmt_tx.h"

#include "esp_timer.h"

#include "bldc_fan.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



static const char *TAG = "scenes";
QueueHandle_t scene_command_queue;

uint32_t scene_status = 0;



void scene_status_set(int scene_id)
{
    scene_status |= (uint32_t)((uint32_t)1 << (uint32_t)scene_id);
}
void scene_status_check(scene_entry_ir_t *scene_entry_ir)
{
    scene_status = 0;
    for(int scene_id = 0;scene_id < 16;scene_id++)
    {
	    memset(scene_entry_ir, 0, sizeof(scene_entry_ir_t));

        read_scene_command(scene_id,0,scene_entry_ir);

        

        if(scene_entry_ir->scene_id != scene_id)
        {
            continue;
        }
        ESP_LOGI(TAG, "scene id - %d",scene_entry_ir->scene_id);
        // ESP_LOGI(TAG, "command id - %d",scene_entry_ir->command_id);
        ESP_LOGI(TAG, "command idffffffffffffffffffffffffffffffffffffffffffffffff - %d",scene_entry_ir->command_id);
        ESP_LOGI(TAG, "node id - %s",scene_entry_ir->node_id);
        ESP_LOGI(TAG, "before_delay - %d",scene_entry_ir->before_delay);
        ESP_LOGI(TAG, "after_delay - %d",scene_entry_ir->after_delay);
        ESP_LOGI(TAG, "ir_data_len - %d",scene_entry_ir->ir_data_len);
        ESP_LOGI(TAG, "ir_data_multiplier - %d",scene_entry_ir->ir_data_multiplier);
        ESP_LOGI(TAG, "data 0 - %d",scene_entry_ir->ir_data[0]);
        ESP_LOGI(TAG, "data 1 - %d",scene_entry_ir->ir_data[1]);
        ESP_LOGI(TAG, "data 2 - %d",scene_entry_ir->ir_data[2]);
        // scene_status |= (uint32_t)((uint32_t)1 << (uint32_t)scene_id);

        scene_status_set(scene_id);

        
    }
    printf("scene_status = %" PRIu32 " \n",scene_status);
}

void scene_execute()
{
    scene_entry_ir_t scene_entry_ir;
	memset(&scene_entry_ir, 0, sizeof(scene_entry_ir_t));
    scene_status_check(&scene_entry_ir);
    while(1)
    {
        pulse_generator(esp_timer_get_time());
        int received_scene_id;
	    if(xQueuePeek(scene_command_queue, &received_scene_id,10) == false)
        {
            continue;
        }
        xQueueReceive(scene_command_queue, &received_scene_id, 0);
        ESP_LOGI(TAG, "received_scene_id - %d",received_scene_id);
        
        if(received_scene_id > 31)
        {
            continue;
        }
        
	    memset(&scene_entry_ir, 0, sizeof(scene_entry_ir_t));
        

        for(int command_id = 0;command_id < 16;command_id++)
        {
            read_scene_command(received_scene_id,0,&scene_entry_ir);
            
            
            if(scene_entry_ir.scene_id != received_scene_id || scene_entry_ir.command_id != command_id)
            {
                continue;
            }
            ESP_LOGI(TAG, "scene id - %d",scene_entry_ir.scene_id);
            ESP_LOGI(TAG, "command id - %d",scene_entry_ir.command_id);
            ESP_LOGI(TAG, "node id - %s",scene_entry_ir.node_id);
            ESP_LOGI(TAG, "before_delay - %d",scene_entry_ir.before_delay);
            ESP_LOGI(TAG, "after_delay - %d",scene_entry_ir.after_delay);
            ESP_LOGI(TAG, "ir_data_len - %d",scene_entry_ir.ir_data_len);
            ESP_LOGI(TAG, "ir_data_multiplier - %d",scene_entry_ir.ir_data_multiplier);
            ESP_LOGI(TAG, "data 0 - %d",scene_entry_ir.ir_data[0]);
            ESP_LOGI(TAG, "data 1 - %d",scene_entry_ir.ir_data[1]);
            ESP_LOGI(TAG, "data 2 - %d",scene_entry_ir.ir_data[2]);

            if(scene_entry_ir.before_delay > 0)
            {
                vTaskDelay(scene_entry_ir.before_delay);
            }

            int nrf_timer = 0;
            int rc = send_ir_data_new(scene_entry_ir.node_id, scene_entry_ir.ir_data, scene_entry_ir.ir_data_len,scene_entry_ir.ir_data_multiplier);
            
            if(scene_entry_ir.after_delay > 0)
            {
                vTaskDelay(scene_entry_ir.after_delay);
            }
            
        }

        vTaskDelay(10);
    }
}


int add_to_scene_queue(int scene_id) {
    ESP_LOGI(TAG, "aad_to_scene_queue scene id - %d",scene_id);
    ESP_LOGI(TAG, "scene id present - %d",((scene_status >> scene_id) & 1) == 1);
    if(((scene_status >> scene_id) & 1) == 1)
    {
        ESP_LOGI(TAG, "add to q");
        xQueueSend(scene_command_queue,&scene_id,0);
    }
	return 0;
}


void scenes_setup(void) {
    scene_command_queue = xQueueCreate(10 ,sizeof(int));
    
    xTaskCreate(scene_execute,"manager", 8192, NULL, 5, NULL);
}