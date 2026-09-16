/**
 ****************************************************************************************************
 * @file        adc1.c
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

#include "adc.h"


adc_oneshot_unit_handle_t adc_handle = NULL;    /* ADC句柄 */

/**
 * @brief       初始化ADC
 * @param       无
 * @retval      无
 */
void adc_init(void)
{
    adc_oneshot_unit_init_cfg_t adc_config = {  /* 初始化配置结构体 */
        .unit_id  = ADC_UNIT_1,                 /* ADC单元:ADC1/ADC2 */
        .ulp_mode = ADC_ULP_MODE_DISABLE,       /* 不支持ADC在ULP模式下工作 */
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc_handle));    /* ADC初始化(单次转换模式) */

    /* 配置 ADC */
    adc_oneshot_chan_cfg_t config = {
        .atten    = ADC_ATTEN_DB_12,            /* ADC衰减 */
        .bitwidth = ADC_BITWIDTH_12,            /* ADC分辨率 */
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHAN, &config));     /* 配置ADC通道 */
}

#define LOST_VAL    1

/**
 * @brief       获取ADC转换且进行多次采样后排序去除最高和最低值再做均值滤波后的结果
 * @note        ESP32P4 ADC对噪声敏感,可能导致ADC读数出现较大偏差
 * @note        软件上:可通过多次采样进一步降低噪声影响;硬件上:可加旁路电容连在在ADC使用引脚上
 * @param       ch      : 通道号, 0~9
 * @param       times   : 获取次数
 * @retval      通道ch的times次转换结果平均值
 */
uint32_t adc_get_result_average(adc_channel_t ch, uint32_t times)
{
    uint32_t sum = 0;
    uint16_t temp_val = 0;

    /* 申请存放ADC原始数据buffer */
    int *rawdata = heap_caps_malloc(times * sizeof(int), MALLOC_CAP_INTERNAL);     
    if (NULL == rawdata)
    {
        ESP_LOGE("adc", "Memory for adc is not enough");
    }

    for (uint32_t t = 0; t < times; t++)                /* 多次ADC采样 */
    {
        adc_oneshot_read(adc_handle, ch, &rawdata[t]);  /* 读取原始数据 */
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    for (uint16_t i = 0; i < times - 1; i++)            /* 对数据进行排序 */
    {
        for (uint16_t j = i + 1; j < times; j++)
        {
            if (rawdata[i] > rawdata[j])                /* 升序排列 */
            {
                temp_val   = rawdata[i];
                rawdata[i] = rawdata[j];
                rawdata[j] = temp_val;
            }
        }
    }

    for (uint32_t i = LOST_VAL; i < times - LOST_VAL; i++)      /* 去掉两端的丢弃值 */
    {
        sum += rawdata[i];                                      /* 累加去掉丢弃值以后的数据 */
    }

    return sum / (times - 2 * LOST_VAL);                        /* 返回平均值 */
}
