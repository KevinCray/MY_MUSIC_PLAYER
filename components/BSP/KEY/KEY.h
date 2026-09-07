#ifndef __KEY_H__
#define __KEY_H__

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"  

#define BOOT_GPIO_PIN GPIO_NUM_0

#define BOOT    gpio_get_level(BOOT_GPIO_PIN)
#define BOOT_PRESSED  1

void KEY_Init(void);
uint8_t KEY_SCAN(uint8_t mode);

#endif