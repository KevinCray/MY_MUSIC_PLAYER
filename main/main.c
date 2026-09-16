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
#include "adc.h"

#define configSUPPORT_DYNAMIC_ALLOCATION 1

void audio_play_task(void * pvParameters) //注意任务函数签名，必须带有指针
{
    while (1)
    {
        audio_play();
        vTaskDelay(pdMS_TO_TICKS(1000));
    };
    
}



void app_main(void)
{
    esp_err_t ret;

    ret = nvs_flash_init();     /* Initialize NVS */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    };

    LED_Init();                 
    my_spi_init();              
    KEY_Init();                
    myiic_init();               
    xl9555_init();
    es8388_init();
    adc_init();
  
    sd_spi_init();
    ret = exfuns_init();

    BaseType_t taskret = xTaskCreate(
        audio_play_task,    //函数指针
        "audio_play",       //任务名称
        4096,               //栈大小
        NULL,               //指针参数
        5,                  //优先级
        NULL                //任务句柄
    );

    if (taskret != pdPASS)
    {
        ESP_LOGE("MAIN","TASK CREATE FAILED");
    }
    

    vTaskDelete(NULL);  //防止cpu阻塞
}

