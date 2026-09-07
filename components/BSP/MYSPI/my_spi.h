/**
 ****************************************************************************************************
 * @file        my_spi.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-01-01
 * @brief       MYSPI驱动代码
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

#ifndef __MY_SPI_H
#define __MY_SPI_H

#include <unistd.h>
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"

/* SPI驱动管脚 */
#define SPI_SCLK_PIN        GPIO_NUM_12
#define SPI_MOSI_PIN        GPIO_NUM_11
#define SPI_MISO_PIN        GPIO_NUM_13
/* 总线设备引脚定义 */
#define SD_CS_PIN           GPIO_NUM_2
/* SPI端口 */
#define MY_SPI_HOST         SPI2_HOST
/* 设备句柄 */
extern spi_device_handle_t MY_SD_Handle;   /* SD卡句柄 */

/* 函数声明 */
esp_err_t my_spi_init(void);    /* SPI初始化 */

#endif
