/**
 ****************************************************************************************************
 * @file        es8388.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       ES8388驱动代码
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

#ifndef __ES8388_H_
#define __ES8388_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "myiic.h"
#include "math.h"
#include "string.h"

#define ES8388_ADDR             0x10                                    /* ES8388的器件地址,固定为0x10 */

/* 声明函数 */
uint8_t es8388_init(void);                                              /* ES8388初始化 */
esp_err_t es8388_deinit(void);                                          /* 复位或者暂停ES8388初始化 */
esp_err_t es8388_write_reg(uint8_t reg, uint8_t val);                   /* ES8388写寄存器 */
esp_err_t es8388_read_reg(uint8_t reg_add, uint8_t *p_data);            /* ES8388读寄存器 */
void es8388_i2s_cfg(uint8_t fmt, uint8_t len);                          /* 设置ES8388工作模式 */
void es8388_hpvol_set(uint8_t volume);                                  /* 设置耳机音量 */
void es8388_spkvol_set(uint8_t volume);                                 /* 设置喇叭音量 */
void es8388_3d_set(uint8_t depth);                                      /* 设置3D环绕声 */
void es8388_adda_cfg(uint8_t dacen, uint8_t adcen);                     /* ES8388 DAC/ADC配置 */
void es8388_output_cfg(uint8_t o1en, uint8_t o2en);                     /* ES8388 DAC输出通道配置 */
void es8388_mic_gain(uint8_t gain);                                     /* ES8388 MIC增益设置(MIC PGA增益) */
void es8388_alc_ctrl(uint8_t sel, uint8_t maxgain, uint8_t mingain);    /* ES8388 ALC设置 */
void es8388_input_cfg(uint8_t in);                                      /* ES8388 ADC输出通道配置 */

#endif
