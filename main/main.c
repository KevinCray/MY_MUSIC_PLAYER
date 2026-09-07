#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "myiic.h"
#include "my_spi.h"
#include "xl9555.h"
#include "spi_sd.h"
#include "sdmmc_cmd.h"
#include "key.h"
#include "es8388.h"
#include "myi2s.h"
#include "exfuns.h"
#include "audioplay.h"
#include <stdio.h>


void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();     /* Initialize NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    LED_Init();                 
    my_spi_init();              
    KEY_Init();                
    myiic_init();               
    xl9555_init();
    es8388_init();
  
    sd_spi_init();
    ret = exfuns_init();

    while (1)
    {
        audio_play(); 
    }
}
