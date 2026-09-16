/**
 ****************************************************************************************************
 * @file        adc1.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       ADC驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __ADC_H
#define __ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adc_oneshot.h"
#include "adc_cali.h"
#include "adc_cali_scheme.h"
#include "esp_log.h"


/* 管脚声明 */
#define ADC_CHAN    ADC_CHANNEL_7       /* 对应管脚为GPIO8 */

/* 函数声明 */
void adc_init(void);                                               /* 初始化ADC */
uint32_t adc_get_result_average(adc_channel_t ch, uint32_t times); /* 获取ADC转换且进行均值滤波后的结果 */

#endif
