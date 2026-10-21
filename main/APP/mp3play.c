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

/*跳过文件开头的ID3v2标签，返回音频数据起始偏移*/
static FSIZE_t mp3_id3_offset(FIL *fp)
{
    uint8_t hdr[10];
    UINT br = 0;
    FSIZE_t pos = 0;
    FSIZE_t fsz = f_size(fp);
    FSIZE_t size;

    while (pos + 10 <= fsz)
    {
        if (f_lseek(fp, pos) != FR_OK)
        {
            break;
        }

        if (f_read(fp, hdr, sizeof(hdr), &br) != FR_OK || br != sizeof(hdr))
        {
            break;
        }

        if (hdr[0] != 'I' || hdr[1] != 'D' || hdr[2] != '3')
        {
            break;
        }

        if (hdr[3] < 2 || hdr[3] > 4)
        {
            break;
        }

        if ((hdr[6] | hdr[7] | hdr[8] | hdr[9]) & 0x80)
        {
            break;
        }

        size = ((FSIZE_t)(hdr[6] & 0x7F) << 21)
             | ((FSIZE_t)(hdr[7] & 0x7F) << 14)
             | ((FSIZE_t)(hdr[8] & 0x7F) << 7)
             |  (FSIZE_t)(hdr[9] & 0x7F);

        pos += 10 + size;

        if (hdr[5] & 0x10)
        {
            pos += 10;
        }
    }

    return (pos < fsz) ? pos : 0;
}

uint8_t mp3_play_song(uint8_t *fname)
{
    FIL file;   //文件句柄
    FRESULT res;    //状态码
    UINT br = 0;    //实际读写字符数量
    uint8_t *read_ptr;  //指向下一个解码字节
    int byte_left;  //缓冲区中当前有效数据的总字节数（还没被解码的）数据总量
    int offset;
    int err;
    int eof = 0;    //文件已读完标志
    int keep;
    int out_samples;
    int pcm_bytes;
    MP3FrameInfo frame_info;
    uint32_t last_sample_rate = 0;  //记录上次采样率
    uint8_t key;

    //-------复位播放标识
    i2s_play_end = ESP_FAIL;
    i2s_play_next_prev = ESP_FAIL;
    i2s_table_size = 0; 

    //-------打开mp3文件
    res = f_open(&file, (TCHAR*)fname, FA_READ);
    if (res != FR_OK )
    {
        printf("mp3 open failed:%s\r\n", (char *)fname);
        return KEY0_PRES;
    }

    //-------跳过ID3v2标签
    if (f_lseek(&file, mp3_id3_offset(&file)) != FR_OK)
    {
        printf("mp3 seek failed:%s\r\n", (char *)fname);
        f_close(&file);
        return KEY0_PRES;
    }

    //-------初始化i2s
    myi2s_init();
    g_audiodev.status = 3 << 0;     /* myi2s_init已经使能了收发通道，状态要同步，不能再调audio_start */
    es8388_adda_cfg(1,0);   /* 打开DAC，关闭ADC */
    es8388_input_cfg(0);    /* 录音关闭 */

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
    i2s_tx_write((uint8_t *)s_pcm_buf, sizeof(s_pcm_buf));  /* 先发送一段无声音的数据 */
    vTaskDelay(pdMS_TO_TICKS(20));
    
    //-------初始化解码器
    s_mp3_decoder = MP3InitDecoder();
    if (s_mp3_decoder == NULL)
    {
        printf("mp3 decoder init failed\r\n");
        f_close(&file);
        audio_stop();
        i2s_deinit();
        return KEY0_PRES;
    }

    //-------预读取数据
    res = f_read(&file, s_input_buf, MP3_INPUT_BUF_SIZE, &br);
    if (res != FR_OK || br == 0)
    {
        printf("mp3 read failed:%s\r\n", (char *)fname);
        MP3FreeDecoder(s_mp3_decoder);
        s_mp3_decoder = NULL;
        f_close(&file);
        audio_stop();
        i2s_deinit();
        return KEY0_PRES;
    }
    read_ptr = s_input_buf;
    byte_left = br;

    //-------解码主循环
    while (1)
    {
        if (i2s_play_next_prev == ESP_OK)
        {
            break;
        }

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

        //-------暂停时只做按键扫描，不往i2s里写数据
        if ((g_audiodev.status & 0x0F) != 0x03)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        //-------补充缓冲区的数据
        if (eof == 0 && byte_left < MP3_INPUT_BUF_SIZE / 2)
        {
            if (byte_left > 0)
            {
                memmove(s_input_buf, read_ptr, byte_left);
            }

            read_ptr = s_input_buf;     //数据读取完毕，需要将指针重置到缓冲区起点

            res = f_read(
                &file,
                s_input_buf + byte_left,    //写入位置
                MP3_INPUT_BUF_SIZE - byte_left, //待读数量
                &br //已读数量
            ); 

            if (res != FR_OK || br == 0)
            {
                eof = 1;
            }
            else
            {
                byte_left += br;
            }
        }

        if (byte_left <= 0)
        {
            break;
        }

        //跳过id3标签
        offset = MP3FindSyncWord(read_ptr, byte_left);
        if (offset < 0)
        {
            if (eof)
            {
                break;
            }

            keep = (byte_left > 2) ? 2 : byte_left;     //保留末尾字节，防止同步字被截断
            read_ptr += byte_left - keep;
            byte_left = keep;
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

        if (err == ERR_MP3_INDATA_UNDERFLOW)
        {
            if (eof)
            {
                break;
            }
            continue;       //数据不够一帧，指针不前进，下一轮补数据
        }

        if (err != ERR_MP3_NONE)    //其他错误才跳过这个字节
        {
            if (byte_left <= 0)
            {
                break;
            }
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
            i2s_trx_start();    //该接口内部会先关闭通道，重配后必须重新使能
        }

        //-------将单声道的音频转换为双声道
        out_samples = frame_info.outputSamps;     
        if (out_samples > MP3_PCM_BUF_SIZE)
        {
            out_samples = MP3_PCM_BUF_SIZE;
        }

        if (frame_info.nChans == 1)
        {
            if (out_samples > MP3_PCM_BUF_SIZE / 2)
            {
                out_samples = MP3_PCM_BUF_SIZE / 2;
            }

            for (int i = out_samples - 1; i >= 0; i--)  //只能从后开始，不然会被覆盖
            {
                s_pcm_buf[i * 2]     = s_pcm_buf[i];   // 左声道
                s_pcm_buf[i * 2 + 1] = s_pcm_buf[i];   // 右声道
            }
            out_samples *= 2;
        }
        
        //-------发送处理好的PCM
        pcm_bytes = out_samples * sizeof(int16_t);
        i2s_table_size += i2s_tx_write((uint8_t *)s_pcm_buf, pcm_bytes); //返回了已发送数据长度

        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    //-------清理
    MP3FreeDecoder(s_mp3_decoder);
    s_mp3_decoder = NULL;
    f_close(&file);

    if ((g_audiodev.status & 0x0F) == 0x03)
    {
        audio_stop();       //先关通道，i2s_deinit要求通道处于关闭状态
    }
    i2s_deinit(); 

    return KEY0_PRES; //结束标志
}
