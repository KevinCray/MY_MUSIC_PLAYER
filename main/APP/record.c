#include "record.h"

//-------录音缓冲区
static uint16_t s_record_buf[RECORD_BUF_SIZE / 2];

//-------录音状态标记,1:正在录音
static volatile  uint8_t s_recording = 0;

//-------wave文件头
typedef struct 
{
    uint32_t ChunkID;       //riff
    uint32_t Chunksize;     //文件总大小减8
    uint32_t Format;        //wav格式
    uint32_t Subchunk1ID;   //fmt格式块
    uint32_t Subchunk1Size; //16
    uint16_t AudioFormat;   //PCM格式
    uint16_t NumChannels;   //声道数量
    uint32_t SampleRate;        /* 采样率 */
    uint32_t ByteRate;          /* 字节率 */
    uint16_t BlockAlign;        /* 块对齐 */
    uint16_t BitsPerSample;     /* 位深 */
    uint32_t SubChunk2ID;       /* "data" */
    uint32_t SubChunk2Size;     /* 数据大小 */
}__attribute__((packed)) wave_header_t;

/**
 * @brief 写入wav文件头
 * @param file 文件对象
 * @param datasize pcm文件总字节数
 */
static void wave_write_header(FIL *file, uint32_t data_size)
{
    wave_header_t h;
    UINT bw;    //接收f-write实际写入字节数量

    h.ChunkID       = 0x46464952;   /* "RIFF" */
    h.Chunksize     = data_size + 36;
    h.Format        = 0x45564157;   /* "WAVE" */
    h.Subchunk1ID   = 0x20746D66;   /* "fmt " */
    h.Subchunk1Size = 16;
    h.AudioFormat   = 1;            /* PCM */
    h.NumChannels   = RECORD_CHANNEL;
    h.SampleRate    = RECORD_SAMPLE_RATE;
    h.ByteRate      = RECORD_SAMPLE_RATE * RECORD_CHANNEL * RECORD_BITS / 8;
    h.BlockAlign    = RECORD_CHANNEL * RECORD_BITS / 8;
    h.BitsPerSample = RECORD_BITS;
    h.SubChunk2ID   = 0x61746164;   /* "data" */
    h.SubChunk2Size = data_size;

    f_write(file, &h, sizeof(h), &bw);
}

/**
 * @brief 驱动es8388进入录音模式
 * @note  麦克风接在IN1上，已经有偏置电压
 */
void es8388_record_init(void)
{
    uint8_t reg_val;    //寄存器储存值

    /*打开ADC*/
    es8388_adda_cfg(0,1);
    vTaskDelay(pdMS_TO_TICKS(10));

    /*选择input1*/
    es8388_input_cfg(0);

    /*设置麦克风增益*/
    es8388_mic_gain(5);

    /*单声道混音*/
    es8388_read_reg(0x0B, &reg_val);
    
}
