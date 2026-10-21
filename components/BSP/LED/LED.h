#ifndef __LED_H_
#define __LED_H_

#include "driver/gpio.h"

#define TRANS_GPIO_PIN    GPIO_NUM_1

enum GPIO_OUTPUT_STATE
{
    PIN_RESET,
    PIN_SET
};
//LED亮灭
#define LED(X) do {X?\
    gpio_set_level(TRANS_GPIO_PIN, PIN_SET):\
    gpio_set_level(TRANS_GPIO_PIN, PIN_RESET);\
} while (0);

//LED反转
#define LED_TOGGLE() do{gpio_set_level(LED_GPIO_PIN, !gpio_get_level(LED_GPIO_PIN));} while (0);

void LED_Init(void);

#endif

