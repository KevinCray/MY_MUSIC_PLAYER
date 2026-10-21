#include "KEY.h"

void KEY_Init(void)
{
    gpio_config_t boot_io_conf = {
        .pin_bit_mask = (1ULL << BOOT_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&boot_io_conf);
    
}

uint8_t KEY_SCAN(uint8_t mode)
{
    uint8_t keyvalue = 0;
    static uint8_t key_boot = 1;

    if (mode)
    {
        key_boot = 1;
    }

    if (key_boot && (BOOT==0))
    {
        vTaskDelay(10);
        key_boot = 0;

        if (BOOT==0)
        {
            keyvalue = BOOT_PRESSED;
        }
    }
    else if (BOOT==1)
    {
        key_boot = 1;
    }

    return keyvalue;
    
    
}