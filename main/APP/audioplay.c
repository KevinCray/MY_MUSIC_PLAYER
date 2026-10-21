#include "audioplay.h"

__audiodev g_audiodev;

uint8_t g_audio_output_mode = 0; //默认扬声器模式

/**
 * @brief       判断文件类型是否可以播放
 * @param       type : exfuns_file_type返回值
 */
static uint8_t audio_is_supported(uint8_t type)
{
    return (type == T_WAV) || (type == T_MP3);
}

/**
 * @brief       开始trx
 */
void audio_start(void)
{
    g_audiodev.status = 3 << 0;
    i2s_trx_start();
}

/**
 * @brief       停止trx
 */
void audio_stop(void)
{
    g_audiodev.status = 0;
    i2s_trx_stop();
}

/**
 * @brief       获取path路径下,目标文件的总个数
 * @param       path : 文件路径 
 */
uint16_t audio_get_tnum(uint8_t *path)
{
    uint8_t res;
    uint16_t rval = 0;
    FF_DIR tdir;
    FILINFO *tfileinfo;
    
    tfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    
    res = f_opendir(&tdir, (const TCHAR*)path);
    
    if ((res == FR_OK) && tfileinfo)
    {
        while (1)
        {
            res = f_readdir(&tdir, tfileinfo);
            
            if ((res != FR_OK) || (tfileinfo->fname[0] == 0))
            {
                break;
            }

            res = exfuns_file_type(tfileinfo->fname);
            
            if (audio_is_supported(res))
            {
                rval++; /* 有效文件个数加1 */
            }
        }
    }
    
    free(tfileinfo);
    
    return rval;
}

/**
 * @brief       将簇号转换为扇区号
 * @param       fs:文件系统指针
 * @param       clst:要转换的簇号
 * @retval      =0:转换成功，0:转换失败
 */
static LBA_t atk_clst2sect(FATFS* fs,DWORD clst)
{
    clst -= 2;          /* Cluster number is origin from 2 */

    if (clst >= fs->n_fatent - 2)
    {
        return 0;       /* Is it invalid cluster number? */
    }

    return fs->database + (LBA_t)fs->csize * clst;  /* Start sector number of the cluster */
}

/**
    * @brief       偏移
    * @param       dp:指向目录对象
    * @param       Offset:目录对象偏移量
    * @retval      FR_OK(0):成功；!=0:失败
*/     

FRESULT atk_dir_sdi(FF_DIR* dp,DWORD ofs)
{
    DWORD clst;
    FATFS *fs = dp->obj.fs;

    if (ofs >= (DWORD)((FF_FS_EXFAT && fs->fs_type == FS_EXFAT) ? 0x10000000 : 0x200000) || ofs % 32)
    {
        /* Check range of offset and alignment */
        return FR_INT_ERR;
    }

    dp->dptr = ofs;         /* Set current offset */
    clst = dp->obj.sclust;  /* Table start cluster (0:root) */

    if (clst == 0 && fs->fs_type >= FS_FAT32)
    {	/* Replace cluster# 0 with root cluster# */
        clst = (DWORD)fs->dirbase;

        if (FF_FS_EXFAT)
        {
            dp->obj.stat = 0;
        }
        /* exFAT: Root dir has an FAT chain */
    }

    if (clst == 0)
    {	/* Static table (root-directory on the FAT volume) */
        if (ofs / 32 >= fs->n_rootdir)
        {
            return FR_INT_ERR;  /* Is index out of range? */
        }

        dp->sect = fs->dirbase;

    }
    else
    {   /* Dynamic table (sub-directory or root-directory on the FAT32/exFAT volume) */
        dp->sect = atk_clst2sect(fs, clst);
    }

    dp->clust = clst;   /* Current cluster# */

    if (dp->sect == 0)
    {
        return FR_INT_ERR;
    }

    dp->sect += ofs / fs->ssize;             /* Sector# of the directory entry */
    dp->dir = fs->win + (ofs % fs->ssize);   /* Pointer to the entry in the win[] */

    return FR_OK;
}

/**
 * @brief 从adc的电压值映射到音量
 */

int map_volume(int raw)
{
    int volume = (raw * VOLUME_MAX)/4095;
    
    if (volume < 0)
    {
        volume = 0;
    }

    if (volume > VOLUME_MAX)
    {
        volume = VOLUME_MAX;
    }
    
    return volume;
}

/**
 * @brief       播放音乐
 * @param       无  
 */
void audio_play(void)
{
    uint8_t res;    //读取成功的确认值
    FF_DIR wavdir;  //定义了一个目录对象
    FILINFO *wavfileinfo;   //定义了一个文件信息结构体指针
    uint8_t *pname; //定义了一个文件名指针
    uint16_t totwavnum; //定义了总的wav文件数量
    uint16_t curindex; //定义了当前文件索引
    uint8_t key; //定义了按键值
    uint32_t temp; //定义了一个临时变量
    uint32_t *wavoffsettbl; //定义了一个wav文件偏移量表指针

    totwavnum = audio_get_tnum((uint8_t *)"0:/MUSIC");          
    
    wavfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    pname = malloc(255 * 2 + 1);
    wavoffsettbl = malloc(4 * totwavnum);

    if ((wavfileinfo == NULL) || (pname == NULL) || (wavoffsettbl == NULL) || (totwavnum == 0))
    {
        printf("audio malloc failed or no music file\r\n");
        free(wavfileinfo);
        free(pname);
        free(wavoffsettbl);
        return;
    }
    
    res = f_opendir(&wavdir, "0:/MUSIC");
    
    if (res == FR_OK)
    {
        curindex = 0;                                          
        while (1)
        {
            
            temp = wavdir.dptr;                                 

            res = f_readdir(&wavdir, wavfileinfo);              
            
            if ((res != FR_OK) || (wavfileinfo->fname[0] == 0))
            {
                break;                                          
            }

            res = exfuns_file_type(wavfileinfo->fname);
            
            if (audio_is_supported(res))
            {
                wavoffsettbl[curindex] = temp;                   
                curindex++;
            }
        }
    }
    
    curindex = 0;                                               
    res = f_opendir(&wavdir, (const TCHAR*)"0:/MUSIC");

    while (res == FR_OK)                                        
    {
        if (atk_dir_sdi(&wavdir, wavoffsettbl[curindex]) != FR_OK)
        {
            break;
        }

        res = f_readdir(&wavdir, wavfileinfo);                  
        if ((res != FR_OK) || (wavfileinfo->fname[0] == 0))
        {
            break;
        }
        
        strcpy((char *)pname, "0:/MUSIC/");
        strcat((char *)pname, (const char *)wavfileinfo->fname);
        key = audio_play_song(pname);
        
        if (key == KEY1_PRES)      
        {
            if (curindex)
            {
                curindex--;
            }
            else
            {
                curindex = totwavnum - 1;
            }
        }
        else if (key == KEY0_PRES) 
        {
            curindex++;

            if (curindex >= totwavnum)
            {
                curindex = 0;
            }
        }
        else
        {
            break;
        }
    }

    free(wavfileinfo);
    free(pname);
    free(wavoffsettbl);
}

/**
 * @brief       播放某首音频文件
 * @param       fname : 文件名
 * @retval      返回值
 *   @arg       KEY0_PRES , 下一首.
 *   @arg       KEY1_PRES , 上一首.
 *   @arg       others , 停止
 */
uint8_t audio_play_song(uint8_t *fname)
{
    uint8_t res;  
    
    res = exfuns_file_type((char *)fname); 

    switch (res)
    {
        case T_WAV:
            res = wav_play_song(fname);
            break;
        case T_MP3:
            res = mp3_play_song(fname);
            break;

        default:            // 非法文件,自动跳转到下一首
            printf("can't play:%s\r\n", fname);
            res = KEY0_PRES;
            break;
    }
    return res;
}