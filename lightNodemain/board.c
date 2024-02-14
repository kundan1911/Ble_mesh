/* board.c - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "board.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define TAG "BOARD"



struct _led_state led_state[3] = {
    { LED_OFF, LED_OFF, LED_R, "red"   },
    { LED_OFF, LED_OFF, LED_G, "green" },
    { LED_OFF, LED_OFF, LED_B, "blue"  },
};



#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO_1          (2) // Define the output GPIO
#define LEDC_OUTPUT_IO_2          (4) // Define the output GPIO
#define CHANNEL_ID_WARM            LEDC_CHANNEL_0
#define CHANNEL_ID_COLD            LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz



/**
 * @brief The state of the five-color light
 */
typedef struct {
    uint8_t mode;
    uint8_t on;
    uint8_t color_temperature;
    uint8_t brightness;
    uint32_t fade_period_ms;
    uint32_t blink_period_ms;
} light_status_t;

static light_status_t cct_light_status = {0};

static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel_1 = {
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_ID_WARM,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO_1,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_1));

    ledc_channel_config_t ledc_channel_2 = {
        .speed_mode     = LEDC_MODE,
        .channel        = CHANNEL_ID_COLD,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO_2,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
        
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_2));
}




int light_driver_set(uint16_t brightness,uint16_t color_temperature)
{
    static uint16_t last_temperature = 0xFFFF;

    ESP_LOGI(TAG, "light_driver_set brightness - %d, color_temperature %d", brightness, color_temperature);

    if(color_temperature >= 800 && color_temperature <= 900)
    {
        color_temperature = color_temperature - 800;
        cct_light_status.color_temperature = color_temperature;
    }
    if(brightness <=100)
    {
        cct_light_status.brightness = brightness;
    }

    uint8_t warm_tmp = cct_light_status.color_temperature * cct_light_status.brightness / 100;
    uint8_t cold_tmp = (100 - cct_light_status.color_temperature) * cct_light_status.brightness / 100;

    ESP_LOGI(TAG, " warm_tmp - %d, cold_tmp %d", warm_tmp, cold_tmp);


    uint32_t warm_tmp_level = ((uint32_t)8191*warm_tmp)/(uint32_t)100;
    uint32_t cold_tmp_level = ((uint32_t)8191*cold_tmp)/(uint32_t)100;

    ESP_LOGI(TAG, " warm_tmp_level - %d, cold_tmp_level %d", warm_tmp_level, cold_tmp_level);

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, CHANNEL_ID_COLD, cold_tmp_level));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, CHANNEL_ID_COLD));

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, CHANNEL_ID_WARM, warm_tmp_level));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, CHANNEL_ID_WARM));
    
    return 1;

}

// void board_led_operation_level(uint32_t level)
// {
//     ESP_LOGI(TAG, "level_t 0x%d", level);
//     uint32_t level_t = ((uint32_t)8191*level)/(uint32_t)100;
//     ESP_LOGI(TAG, "level_t 0x%d\n", ((uint32_t)8191*level));
//     ESP_LOGI(TAG, "level_t 0x%d\n", level_t);
//     ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, level_t));
//     // Update duty to apply the new value
//     ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0));
// }
void board_led_operation(uint8_t pin, uint8_t onoff)
{
    for (int i = 0; i < 3; i++) {
        if (led_state[i].pin != pin) {
            continue;
        }
        if (onoff == led_state[i].previous) {
            ESP_LOGW(TAG, "led %s is already %s",
                     led_state[i].name, (onoff ? "on" : "off"));
            return;
        }
        gpio_set_level(pin, onoff);
        led_state[i].previous = onoff;
        return;
    }

    ESP_LOGE(TAG, "LED is not found!");
}

static void board_led_init(void)
{
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(led_state[i].pin);
        gpio_set_direction(led_state[i].pin, GPIO_MODE_OUTPUT);
        gpio_set_level(led_state[i].pin, LED_OFF);
        led_state[i].previous = LED_OFF;
    }
}

void board_init(void)
{
    board_led_init();
    ledc_init();
}
