#include "mp3play.h"
#include "mp3dec.h"               // Helix 解码库对外头文件
#include "ff.h"
#include "myi2s.h"
#include "es8388.h"
#include "key.h"
#include "xl9555.h"
#include "audioplay.h"            // 用到 g_audiodev、audio_start 等
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "exfuns.h"
#include <string.h>
#include <stdio.h>

/*MP3 buffer*/
#define MP3_INPUT_BUF_SIZE 4096
#define MP3_MAX_SAMPLES 1152
#define MP3_PCM_BUF_SIZE ( MP3_MAX_SAMPLES * 2)

/*分别为输入和输出pcm分配内存*/
static  HMP3Decoder s_mp3_decoder = NULL;
static  uint8_t s_input_buf[MP3_INPUT_BUF_SIZE];
static  int16_t s_pcm_buf[MP3_PCM_BUF_SIZE];

/*播放控制标志（和wav里面的一致）*/
extern esp_err_t i2s_play_end;
extern esp_err_t i2s_play_next_prev;    /* 下一首或者上一首标志位 */
extern volatile long long int i2s_table_size;  /* 积累每次发送音频数据总大小 */

uint8_t mp3_play_song(uint8_t *fname)
{
    FIL file;   //文件句柄
    FRESULT res;    //状态码
    UINT br = 0;    //实际读写字符数量
    uint8_t *read_ptr;  //指向下一个解码字节
    int byte_left;  //缓冲区中当前有效数据的总字节数（还没被解码的）数据总量
    int offset;
    int err;
    MP3FrameInfo frame_info;
    static uint32_t last_sample_rate = 0;//记录上次采样率
    uint8_t key;

    //-------复位播放标识
    i2s_play_end = ESP_FAIL;
    i2s_play_next_prev = ESP_FAIL;
    i2s_table_size = 0; 

    //-------打开mp3文件
    res = f_open(&file, (TCHAR*)fname, FA_READ);
    if (res != FR_OK || br == 0)
    {
        f_close(&file); //释放资源
        return KEY0_PRES;
    }

    //-------初始化i2s
    myi2s_init();
    if (g_audio_output_mode == 0)
    {
        xl9555_pin_write(SPK_EN_IO,0);     /* 打开喇叭  0开启，1关闭*/
        es8388_output_cfg(0,1);            /* 喇叭通道和耳机通道打开，第一个耳机，1开启 */
        es8388_spkvol_set(10);   
    }
    else
    {
        xl9555_pin_write(SPK_EN_IO, 1);   // 关喇叭功放
        es8388_output_cfg(1, 0);         // 关喇叭，开耳机
        es8388_hpvol_set(10);
    }
    memset(s_pcm_buf, 0, sizeof(s_pcm_buf));
    i2s_tx_write(s_pcm_buf, MP3_INPUT_BUF_SIZE);  /* 先发送一段无声音的数据 */
    vTaskDelay(pdMS_TO_TICKS(20));
    
    //-------初始化解码器
    s_mp3_decoder = MP3InitDecoder();
    if (s_mp3_decoder == NULL)
    {
        f_close(&file);
        return KEY0_PRES;
    }

    //-------预读取数据
    res = f_read(&file, s_input_buf, MP3_INPUT_BUF_SIZE, &br);
    if (res != FR_OK || br == 0)
    {
        MP3FreeDecoder(s_mp3_decoder);
        return KEY0_PRES;
    }
    read_ptr = s_input_buf;
    byte_left = br;

    //-------解码主循环
    while (1)
    {
        //跳过id3标签
        offset = MP3FindSyncWord(read_ptr, byte_left);
        if (offset < 0)
        {
            if (byte_left > 0)
            {
                memmove(s_input_buf, read_ptr, byte_left);
            };
            
            res = f_read(
                &file,
                s_input_buf + byte_left,    //写入位置
                MP3_INPUT_BUF_SIZE - byte_left, //待读数量
                &br //已读数量
            ); 
            
            if (res != FR_OK || br == 0)
            {
                break;
            }
            //文件结束
            read_ptr = s_input_buf;
            byte_left += br;
            continue;
        }
        read_ptr += offset;
        byte_left -= offset;

        //-------解码一个数据帧
        err = MP3Decode(
            s_mp3_decoder,
            &read_ptr,      //必须使用双重指针
            &byte_left,
            s_pcm_buf,
            0               //是否启用读取限制
        );

        if (err != ERR_MP3_NONE)    //报错么跳克下一个字节
        {
            read_ptr ++;
            byte_left --;
            continue;    
        }
        
        //-------获取帧信息
        MP3GetLastFrameInfo(s_mp3_decoder, &frame_info);
        
        //-------动态设置采样率
        if (frame_info.samprate != last_sample_rate) 
        {
            last_sample_rate = frame_info.samprate;
            i2s_set_samplerate_bits_sample
            (
                frame_info.samprate,
                I2S_BITS_PER_SAMPLE_16BIT
            );
        }

        //-------将单声道的音频转换为双声道
        int out_samples = frame_info.outputSamps;     
        if (frame_info.nChans == 1)
        {
            for (int i = out_samples - 1; i >= 0; i--)  //只能从后开始，不然会被覆盖
            {
                s_pcm_buf[i * 2]     = s_pcm_buf[i];   // 左声道
                s_pcm_buf[i * 2 + 1] = s_pcm_buf[i];   // 右声道
            }
            
        }
        
        //-------发送处理好的PCM
        int pcm_bytes = out_samples * sizeof(int16_t);
        if (frame_info.nChans == 1)
        {
            pcm_bytes *= 2;
        }
        i2s_table_size += i2s_tx_write((uint8_t *)s_pcm_buf, pcm_bytes); //返回了已发送数据长度

        //-------adc音量检测
        static int last_volume = -1;    //音量初始化，保证每一次开机都更新
        uint16_t adc_data = adc_get_result_average(ADC_CHAN ,10);   //获取adc数值
        uint8_t volume = map_volume(adc_data);
        if (volume != last_volume)
        {
            es8388_hpvol_set(volume);
            es8388_spkvol_set(volume);
        }

        //-------boot按键切换输出模式
        static int last_boot_state = 1;
        int boot_state = gpio_get_level(BOOT_GPIO_PIN);

        if (last_boot_state == 1 && boot_state == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (gpio_get_level(BOOT_GPIO_PIN) == 0)
            {
                g_audio_output_mode = !g_audio_output_mode;
                if (g_audio_output_mode == 0)
                {
                    xl9555_pin_write(SPK_EN_IO,0);
                    es8388_output_cfg(0,1);
                    es8388_spkvol_set(10);
                }
                else
                {
                    xl9555_pin_write(SPK_EN_IO,1);
                    es8388_output_cfg(1,0);
                    es8388_hpvol_set(10);
                }
            }
        }
        last_boot_state = boot_state;
        
        //-------按键扫描
        key = xl9555_key_scan(0);
        
        switch (key)
        {
        case KEY0_PRES:
        case KEY1_PRES:
            i2s_play_next_prev = ESP_OK;
            break;
        
        case KEY2_PRES:
            if ((g_audiodev.status & 0x0F) == 0x03) //用掩码取低四位，以判断播放状态
            {
                audio_stop();
            }
            else if ((g_audiodev.status & 0x0F) == 0x00)
            {
                audio_start();
            }
            break;
        }

        //-------补充缓冲区的数据
        if (byte_left < MP3_INPUT_BUF_SIZE / 2)
        {
            if (byte_left > 0)
            {
                memmove(s_input_buf, read_ptr, byte_left);
            }
            
            res = f_read(
                &file,
                s_input_buf + byte_left,
                MP3_INPUT_BUF_SIZE - byte_left,
                &br
            );

            if (res == FR_OK && br > 0)
            {
                read_ptr = s_input_buf;     //数据读取完毕，需要将指针重置到缓冲区起点
                byte_left += br;
            }
            else
            {
                if (br <= 0)
                {
                    break;
                }
                
            }    
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    //-------清理
    MP3FreeDecoder(s_mp3_decoder);
    s_mp3_decoder = NULL;
    f_close(&file);
    last_sample_rate = 0;

    return KEY0_PRES; //结束标志
}