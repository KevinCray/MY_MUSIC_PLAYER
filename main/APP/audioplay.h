#ifndef _AUDIOPLAY_H_
#define _AUDIOPLAY_H_

#include "ff.h"
#include "wavplay.h"
#include "exfuns.h"
#include "myi2s.h"
#include "adc.h"
#include "driver/gpio.h"
#include "mp3play.h"

#define VOLUME_MAX 33

void audio_start(void);
void audio_stop(void);
uint16_t audio_get_tnum(uint8_t *path);
static LBA_t atk_clst2sect(FATFS* fs,DWORD clst);
FRESULT atk_dir_sdi(FF_DIR* dp,DWORD ofs);
void audio_play(void);
uint8_t audio_play_song(uint8_t *fname);
int map_volume(int raw);
extern uint8_t g_audio_output_mode;

typedef struct
{
    uint8_t *tbuf;      /* 零时数组,仅在24bit解码的时候需要用到 */
    FIL *file;          /* 音频文件指针 */

    uint8_t status;     /* bit0:0,暂停播放;1,继续播放 */
                        /* bit1:0,结束播放;1,开启播放 */
} __audiodev;



extern __audiodev g_audiodev;



#endif