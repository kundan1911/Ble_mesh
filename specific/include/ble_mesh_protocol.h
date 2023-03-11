
#include<stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
// #include "esp_ble_mesh_defs.h"

extern xSemaphoreHandle temp;

uint16_t tid;
typedef struct {
    uint8_t tid;
    int16_t  desAddr;     
    uint8_t  err;             
}nodeAck;

typedef struct {
    bool     op_en;            /*!< Indicate if optional parameters are included */
    uint16_t ctl_lightness;    /*!< Target value of light ctl lightness state */
    uint16_t ctl_temperatrue;  /*!< Target value of light ctl temperature state */
    int16_t  ctl_delta_uv;     /*!< Target value of light ctl delta UV state */
    uint8_t  tid;              /*!< Transaction ID */
    uint8_t  trans_time;       /*!< Time to complete state transition (optional) */
    uint8_t  delay;            /*!< Indicate message execution delay (C.1) */
} set_t;

void pushAckToQueue(nodeAck ackw);
uint8_t sendAndReceiveBleData(uint8_t ,uint16_t ,uint8_t );
void ble_setup();