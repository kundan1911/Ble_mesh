#include "ex_eeprom.h"
#include "esp_flash.h"
#include "esp_spi_flash.h"
#include "esp_flash_spi_init.h"
#include "driver/spi_common.h"
#include "esp_partition.h"

#include "scenes.h"

#include "esp_log.h"

#define MAX_EEPROM_BLOCK_SIZE 65536
#define MAX_EEPROM_SECTOR_SIZE 4096

#define SCENE_ID_TO_BLOCK_ID(x) (uint32_t)((uint32_t)x * (uint32_t)MAX_EEPROM_BLOCK_SIZE) 
#define COMMAND_ID_TO_SECTOR_ADDR(s_id,sec_id) (uint32_t)((uint32_t)SCENE_ID_TO_BLOCK_ID(s_id) + ((uint32_t)sec_id * (uint32_t)MAX_EEPROM_SECTOR_SIZE))

static const char *TAG = "ex_eeprom";
esp_flash_t* ext_flash;
uint32_t block_adr;

// #define SCENE_ID_TO_BLOCK_ADDR(x) (uint32_t)((uint32_t)MAX_EEPROM_BLOCK_SIZE * ((uint32_t)x +(uint32_t)1)

void ex_eeprom_int()
{
  ESP_LOGI(TAG, " #### ex_eeprom_int\n");
  const spi_bus_config_t bus_config = {
        .mosi_io_num = VSPI_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = VSPI_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = VSPI_IOMUX_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    const esp_flash_spi_device_config_t device_config = {
        .host_id = VSPI_HOST,
        .cs_id = 0,
        .cs_io_num = VSPI_IOMUX_PIN_NUM_CS,
        .io_mode = SPI_FLASH_FASTRD,
        .speed = ESP_FLASH_40MHZ
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &bus_config, 1));

    // Add device to the SPI bus
    

    // This fails if `SPIRAM_USE_MALLOC` is set.
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    // Probe the Flash chip and initialize it
    int8_t ret = esp_flash_init(ext_flash);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }

    // Print out the ID and size
    uint32_t id;
    ret = esp_flash_read_id(ext_flash, &id);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read id: %s (0x%x)", esp_err_to_name(ret), ret);
        return ret;
    }
    ESP_LOGI(TAG, "Initialized external Flash, size=%d KB, ID=0x%x", ext_flash->size / 1024, id);

}



void ex_eeprom_store_scene_ir(scene_entry_ir_t scene_entry)
{
    // ESP_LOGI(TAG, "ex_eeprom_store_scene_ir");
    // ESP_LOGI(TAG, "scene_entry.ir_data_len - %d",scene_entry.ir_data_len);
    // ESP_LOGI(TAG, "scene_entry.node_id - %s",scene_entry.node_id);
    esp_err_t ret = esp_flash_erase_region(ext_flash,0,4096);
    esp_flash_write(ext_flash,&scene_entry,0,sizeof(scene_entry_ir_t));

    scene_entry_ir_t scene_entry_read;
    esp_flash_read(ext_flash,&scene_entry_read,0,sizeof(scene_entry_ir_t));
    // ESP_LOGI(TAG, "scene_entry_read.ir_data_len - %d",scene_entry_read.ir_data_len);
    // ESP_LOGI(TAG, "scene_entry_read.node_id - %s",scene_entry_read.node_id);
    
}
int aa = 0;
void eeprom_test(uint16_t a)
{
    esp_err_t res = esp_flash_erase_region(ext_flash,0,4096);
    ESP_LOGI(TAG, "data to be written:%d", aa);
    aa++;
    esp_flash_write(ext_flash , &aa, 0, sizeof(uint16_t));
    ESP_LOGI(TAG, "done");

    uint16_t b;
    esp_flash_read(ext_flash,&b,0,sizeof(uint16_t));
    ESP_LOGI(TAG,"data is %d", b);
}

void read_scene_command(int scene_id,int command_id, scene_entry_ir_t *scene_data)
{
    // scene_entry_ir_t a;
    ESP_LOGI(TAG, "reading command");
    ESP_LOGI(TAG, "sector addr - %d",COMMAND_ID_TO_SECTOR_ADDR(scene_id, command_id));
    esp_flash_read(ext_flash, scene_data, COMMAND_ID_TO_SECTOR_ADDR(scene_id, command_id), sizeof(scene_entry_ir_t));
    ESP_LOGI(TAG, "scene_id: %d",scene_data->scene_id);
    ESP_LOGI(TAG, "command_id: %d",scene_data->command_id);
}

int write_command_to_eeprom( scene_entry_ir_t *scene_data)
{
    int scene_id = scene_data->scene_id;
    if(scene_id > 31 || scene_data->command_id > 15)
    {
        return -2;
    }
    ESP_LOGI(TAG, "scene_id: %d",scene_data->scene_id);
    ESP_LOGI(TAG, "command_id: %d",scene_data->command_id);
    ESP_LOGI(TAG, "sector addr: %d",COMMAND_ID_TO_SECTOR_ADDR(scene_data->scene_id, scene_data->command_id));
    esp_err_t res = esp_flash_erase_region(ext_flash , COMMAND_ID_TO_SECTOR_ADDR(scene_data->scene_id, scene_data->command_id), MAX_EEPROM_SECTOR_SIZE);
    esp_flash_write(ext_flash ,scene_data, COMMAND_ID_TO_SECTOR_ADDR(scene_data->scene_id, scene_data->command_id), sizeof(scene_entry_ir_t));
    ESP_LOGI(TAG, "command stored");

    esp_flash_read(ext_flash, scene_data,COMMAND_ID_TO_SECTOR_ADDR(scene_data->scene_id, scene_data->command_id), sizeof(scene_entry_ir_t));
    if (scene_id == scene_data->scene_id && scene_id < 31)
    {
        scene_status_set(scene_id);
    }
    return 1;
}

void erase_scene(uint8_t scene_id)
{
    esp_flash_erase_region(ext_flash , SCENE_ID_TO_BLOCK_ID(scene_id), MAX_EEPROM_BLOCK_SIZE);
    ESP_LOGI(TAG, "scene erased");
}

