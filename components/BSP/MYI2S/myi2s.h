/**
 ****************************************************************************************************
 * @file        i2s.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       I2S驱动代码
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

#ifndef _MYI2S_H_
#define _MYI2S_H_

#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_std.h"


#define I2S_NUM                 (I2S_NUM_0)                 /* I2S port */
#define I2S_BCK_IO              (GPIO_NUM_46)               /* ES8388_SCLK */
#define I2S_WS_IO               (GPIO_NUM_9)                /* ES8388_LRCK */
#define I2S_DO_IO               (GPIO_NUM_10)               /* ES8388_SDIN */
#define I2S_DI_IO               (GPIO_NUM_14)               /* ES8388_SDOUT */
#define I2S_MCK_IO              (GPIO_NUM_3)                /* ES8388_MCLK */
#define I2S_RECV_BUF_SIZE       (2400)                      /* 接收大小 */
#define I2S_SAMPLE_RATE         (96000)                     /* 采样率 */
#define I2S_MCLK_MULTIPLE       (256)                       /* 如果不使用24位数据宽度，256应该足够了 */

extern i2s_chan_handle_t tx_handle;
extern i2s_chan_handle_t rx_handle;

/* 函数声明 */
esp_err_t myi2s_init(void);                                         /* I2S初始化 */
void i2s_trx_start(void);                                           /* 启动I2S */
void i2s_trx_stop(void);                                            /* 停止I2S */
void i2s_deinit(void);                                              /* 卸载I2S */
size_t i2s_tx_write(uint8_t *buffer, uint32_t frame_size);          /* I2S传输数据 */
size_t i2s_rx_read(uint8_t *buffer, uint32_t frame_size);           /* I2S接收数据 */
void i2s_set_samplerate_bits_sample(int samplerate,int bits_sample);/* 设置采样率和位宽 */

#endif
