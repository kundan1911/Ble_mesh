#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"

#include <string.h> /* memset */
#include <unistd.h> /* close */

#include "rmt_tx.h"

static const char *TAG = "RMT Tx";

#define RMT_TX_0_CHANNEL RMT_CHANNEL_0
#define RMT_TX_1_CHANNEL RMT_CHANNEL_1
#define RMT_TX_2_CHANNEL RMT_CHANNEL_2
#define RMT_TX_3_CHANNEL RMT_CHANNEL_3

#define RMT_TX_0_GPIO 27
#define RMT_TX_1_GPIO 26
#define RMT_TX_2_GPIO 32
#define RMT_TX_3_GPIO 21
// #define SAMPLE_CNT  (439)

#define CLK_DIV 80

// Number of clock ticks that represent 10us.  10 us = 1/100th msec.
#define TICK_10_US (80000000 / CLK_DIV / 100000)




void buildItem(rmt_item32_t *item,int high_us,int low_us,int multiplier){
  item->level0 = 1;
  item->duration0 = ((high_us*multiplier) / 10 * TICK_10_US);
  item->level1 = 0;
  item->duration1 = ((low_us*multiplier) / 10 * TICK_10_US);
}


/*
 * RMT Tx channel config
 */
void pico_rmt_config(rmt_channel_t channel, int frequency)
{
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = channel;
    if (channel == RMT_TX_0_CHANNEL)
    {
      config.gpio_num = RMT_TX_0_GPIO;
      config.mem_block_num = 1;
    }

    else if (channel == RMT_TX_1_CHANNEL)
    {
      config.gpio_num = RMT_TX_1_GPIO;
      config.mem_block_num = 2;
    }

    else if (channel == RMT_TX_2_CHANNEL)
    {
      config.gpio_num = RMT_TX_2_GPIO;
      config.mem_block_num = 3;
    }

    else if (channel == RMT_TX_3_CHANNEL)
    {
      config.gpio_num = RMT_TX_3_GPIO;
      config.mem_block_num = 4;
    }

    
    config.tx_config.loop_en = 0;
    // enable the carrier to be able to hear the Morse sound
    // if the RMT_TX_GPIO is connected to a speaker
    config.tx_config.carrier_en = 1;
    config.tx_config.idle_output_en = 1;
    config.tx_config.idle_level = 0;
    config.tx_config.carrier_duty_percent = 50;
    // set audible career frequency of 611 Hz
    // actually 611 Hz is the minimum, that can be set
    // with current implementation of the RMT API
    config.tx_config.carrier_freq_hz = frequency*1000;
    config.tx_config.carrier_level = 1;
    // set the maximum clock divider to be able to output
    // RMT pulses in range of about one hundred milliseconds
    config.clk_div = 80;

    ESP_ERROR_CHECK(rmt_config(&config));
    // ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    // ESP_ERROR_CHECK(rmt_translator_init(config.channel, u8_to_rmt));
}
/*
 * Initialize the RMT Tx channel
 */
void rmt_tx_int()
{

    pico_rmt_config(RMT_TX_0_CHANNEL,38);
    ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_0_CHANNEL, 0, 0));
    pico_rmt_config(RMT_TX_1_CHANNEL,38);
    ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_1_CHANNEL, 0, 0));
    pico_rmt_config(RMT_TX_2_CHANNEL,38);
    ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_2_CHANNEL, 0, 0));
    pico_rmt_config(RMT_TX_3_CHANNEL,38);
    ESP_ERROR_CHECK(rmt_driver_install(RMT_TX_3_CHANNEL, 0, 0));

    // ESP_ERROR_CHECK(rmt_translator_init(config.channel, u8_to_rmt));
}



int send_ir_data(char *node_id, char *pipe_addr, char *buf, uint16_t SAMPLE_CNT, int *timerNow, char *res){

  ESP_LOGI(TAG, "send_ir_data :");

    int *data = malloc(sizeof(int)*SAMPLE_CNT);
    memset((void*) data, 0, sizeof(int)*SAMPLE_CNT);
    //build item
    ESP_LOGI(TAG, "send_ir_data : %d",SAMPLE_CNT);
    size_t size = (sizeof(rmt_item32_t) * SAMPLE_CNT);
    rmt_item32_t* item = (rmt_item32_t*) malloc(size); //allocate memory
    memset((void*) item, 0, size); //wipe current data in memory
    
    int x = 0;

    for (int i = 0; i < SAMPLE_CNT; i++) {
      data[x] = buf[i];
      if((i+2) <SAMPLE_CNT && buf[i+1]==0 ){
        if(buf[i+2] >0){
          data[x] = (data[x]*256)+buf[i+2];
          i +=2;
        }
        else if(data[x]==0){
          break;
        }
      }
      x++;
    }
    SAMPLE_CNT = x;
    ESP_LOGI(TAG, "SAMPLE_CNT : %d",SAMPLE_CNT);
    ESP_LOGI(TAG, "freq : %d",data[0]);
    ESP_LOGI(TAG, "send_ir_data : %d",data[1]);
    ESP_LOGI(TAG, "send_ir_data : %d",data[2]);

    rmt_channel_t chennel = RMT_TX_0_CHANNEL;

    ESP_LOGI(TAG, "send_ir_data node : %d",strlen(node_id));
    ESP_LOGI(TAG, "send_ir_data node : %c",node_id[4]);

    if ((node_id[4] == '1' || node_id[4] == '5'))
      chennel = RMT_TX_0_CHANNEL;
    else if ((node_id[4] == '2' || node_id[4] == '6'))
      chennel = RMT_TX_1_CHANNEL;
    else if ((node_id[4] == '3' || node_id[4] == '7'))
      chennel = RMT_TX_2_CHANNEL;
    else if ((node_id[4] == '4' || node_id[4] == '8'))
      chennel = RMT_TX_3_CHANNEL;

    

    ESP_LOGI(TAG, "chennel node : %d",chennel);





    // pico_rmt_config(chennel,data[0]);
    uint32_t rmt_source_clk_hz = 80*1000000;
    uint16_t carrier_duty_percent = 50;
    bool carrier_en = 1;
    rmt_carrier_level_t carrier_level = 1;
    // rmt_get_source_clk(chennel,&rmt_source_clk_hz);
    uint32_t duty_div, duty_h, duty_l;
    duty_div = rmt_source_clk_hz / (data[0]*1000);
    duty_h = duty_div * carrier_duty_percent / 100;
    duty_l = duty_div - duty_h;

    rmt_set_tx_carrier(chennel,carrier_en,duty_h, duty_l,carrier_level);

    x=1;
    int multiplier  = 0;
    if (data[x] == 999)
    {
      multiplier = 10;
      x++;
    }
    else
    {
      multiplier = 50;
    }
    



    int i = 0;
     while(x < SAMPLE_CNT) {

            //    To build a series of waveforms.

            buildItem(&item[i],(int)data[x],(int)data[x+1],multiplier);
            x=x+2;
            i++;
     }

     //To send data according to the waveform items.
     rmt_write_items(chennel, item, i, true);
     //Wait until sending is done.
     //before we free the data, make sure sending is already done.
     free(item);
     free(data);
     return 1;
}



int send_ir_data_new(char *node_id, uint16_t *data, uint16_t SAMPLE_CNT,int multiplier){

  ESP_LOGI(TAG, "send_ir_data :");

    // int *data = malloc(sizeof(int)*SAMPLE_CNT);
    // memset((void*) data, 0, sizeof(int)*SAMPLE_CNT);
    //build item
    ESP_LOGI(TAG, "send_ir_data : %d",SAMPLE_CNT);
    size_t size = (sizeof(rmt_item32_t) * SAMPLE_CNT);
    rmt_item32_t* item = (rmt_item32_t*) malloc(size); //allocate memory
    memset((void*) item, 0, size); //wipe current data in memory
    

    ESP_LOGI(TAG, "SAMPLE_CNT : %d",SAMPLE_CNT);
    ESP_LOGI(TAG, "freq : %d",data[0]);
    ESP_LOGI(TAG, "send_ir_data : %d",data[1]);
    ESP_LOGI(TAG, "send_ir_data : %d",data[2]);

    rmt_channel_t chennel = RMT_TX_0_CHANNEL;

    ESP_LOGI(TAG, "send_ir_data node : %d",strlen(node_id));
    ESP_LOGI(TAG, "send_ir_data node : %c",node_id[4]);

    if ((node_id[4] == '1' || node_id[4] == '5'))
      chennel = RMT_TX_0_CHANNEL;
    else if ((node_id[4] == '2' || node_id[4] == '6'))
      chennel = RMT_TX_1_CHANNEL;
    else if ((node_id[4] == '3' || node_id[4] == '7'))
      chennel = RMT_TX_2_CHANNEL;
    else if ((node_id[4] == '4' || node_id[4] == '8'))
      chennel = RMT_TX_3_CHANNEL;

    

    ESP_LOGI(TAG, "chennel node : %d",chennel);





    // pico_rmt_config(chennel,data[0]);
    uint32_t rmt_source_clk_hz = 80*1000000;
    uint16_t carrier_duty_percent = 50;
    bool carrier_en = 1;
    rmt_carrier_level_t carrier_level = 1;
    // rmt_get_source_clk(chennel,&rmt_source_clk_hz);
    uint32_t duty_div, duty_h, duty_l;
    duty_div = rmt_source_clk_hz / (data[0]*1000);
    duty_h = duty_div * carrier_duty_percent / 100;
    duty_l = duty_div - duty_h;

    rmt_set_tx_carrier(chennel,carrier_en,duty_h, duty_l,carrier_level);

    int x=1;
    



    int i = 0;
     while(x < SAMPLE_CNT) {

            //    To build a series of waveforms.

            buildItem(&item[i],(int)data[x],(int)data[x+1],multiplier);
            x=x+2;
            i++;
     }

     //To send data according to the waveform items.
     rmt_write_items(chennel, item, i, true);
     //Wait until sending is done.
     //before we free the data, make sure sending is already done.
     free(item);
     return 1;
}