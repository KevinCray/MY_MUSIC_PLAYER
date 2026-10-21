#ifndef _RECORD_H_
#define _RECORD_H_

#include "stdint.h"
#include "ff.h"
#include "myi2s.h"
#include "es8388.h"
#include "xl9555.h"
#include "key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*-------录音参数*/
#define RECORD_SAMPLE_RATE 16000        
#define RECORD_CHANNEL     1    //录音声道数量
#define RECORD_BITS        16      //16位
#define RECORD_DIR         "0:/RECORDER"
#define RECORD_BUF_SIZE    4096

static void wave_write_header(FIL *file, uint32_t data_size);

#endif