#include "audioplay.h"

__audiodev g_audiodev;

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
            
            if ((res & 0xF0) == 0x40)
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
 * @brief       播放音乐
 * @param       无  
 */
void audio_play(void)
{
    uint8_t res;
    FF_DIR wavdir;
    FILINFO *wavfileinfo;
    uint8_t *pname;
    uint16_t totwavnum;
    uint16_t curindex;
    uint8_t key;
    uint32_t temp;
    uint32_t *wavoffsettbl;

    totwavnum = audio_get_tnum((uint8_t *)"0:/MUSIC");          
    
    wavfileinfo = (FILINFO*)malloc(sizeof(FILINFO));
    pname = malloc(255 * 2 + 1);
    wavoffsettbl = malloc(4 * totwavnum);
    
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
            
            if ((res & 0xF0) == 0x40)
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
        atk_dir_sdi(&wavdir, wavoffsettbl[curindex]);           

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
        
            break;

        default:            // 非法文件,自动跳转到下一首
            printf("can't play:%s\r\n", fname);
            res = KEY0_PRES;
            break;
    }
    return res;
}