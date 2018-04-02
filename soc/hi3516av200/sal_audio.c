#include "hi_comm.h"
#include "sal_audio.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "./driver/board_ctl.h"

#define AW8733A_FILE     "/dev/adc"
#define ACODEC_FILE     "/dev/acodec"
#define HIS_AUDIO_FRAME_HEADER_LEN  4
#define AUDIO_LOCAL_TEST 0

typedef struct vqe_config_s
{
    int vqe_enbale;

    int hpr_enable;
    int hpr_freq;

    int anr_enable;
    int anr_intensity;
    int anr_noise_db_thr;

    int rnr_enable;
    int rnr_nr_mode;
    int rnr_max_nr_level;
    int rnr_noise_thresh;
}
vqe_config_s;

typedef struct sal_audio_args_s
{
    sal_audio_s audio;

    int audio_enc_stop;
    int audio_enc_runing;
    unsigned char *dec_buf;

    vqe_config_s  vqe_cfg;
    int input_vol;
    int mic_vol;
    int output_vol;
    int last_aw8733a_stat;
    struct timeval last_aw8733a_time;

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_audio_args_s;

static sal_audio_args_s* g_audio_args = NULL;

static int ao_bind_adec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn;
    memset(&stSrcChn, 0, sizeof(stSrcChn));
    MPP_CHN_S stDestChn;
    memset(&stDestChn, 0, sizeof(stDestChn));

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = AoDev;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

static int ao_unbind_adec(AUDIO_DEV AoDev, AO_CHN AoChn, ADEC_CHN AdChn)
{
    MPP_CHN_S stSrcChn;
    memset(&stSrcChn, 0, sizeof(stSrcChn));
    MPP_CHN_S stDestChn;
    memset(&stDestChn, 0, sizeof(stDestChn));

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AoDev;
    stSrcChn.s32ChnId = AdChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AoChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

static int aenc_bind_ai(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn;
    memset(&stSrcChn, 0, sizeof(stSrcChn));
    MPP_CHN_S stDestChn;
    memset(&stDestChn, 0, sizeof(stDestChn));

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

static int aenc_unbind_ai(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn)
{
    MPP_CHN_S stSrcChn;
    memset(&stSrcChn, 0, sizeof(stSrcChn));
    MPP_CHN_S stDestChn;
    memset(&stDestChn, 0, sizeof(stDestChn));

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

static int ai_start(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt, AIO_ATTR_S *pstAioAttr)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = HI_MPI_AI_SetPubAttr(AiDevId, pstAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_AI_Enable(AiDevId);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    for (i = 0; i < s32AiChnCnt; i++)
    {
        s32Ret = HI_MPI_AI_EnableChn(AiDevId, i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    return HI_SUCCESS;
}

static int ai_stop(AUDIO_DEV AiDevId, HI_S32 s32AiChnCnt)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_FAILURE;

    for (i = 0; i < s32AiChnCnt; i++)
    {
        s32Ret = HI_MPI_AI_DisableChn(AiDevId, i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("AI DisableChn[%d] done.\n", i);
    }

    s32Ret = HI_MPI_AI_Disable(AiDevId);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("AI DisableDev[%d] done.\n", AiDevId);

    return HI_SUCCESS;
}

static int ao_start(AUDIO_DEV AoDevId, AO_CHN AoChn, AIO_ATTR_S *pstAioAttr)
{
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = HI_MPI_AO_SetPubAttr(AoDevId, pstAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_AO_Enable(AoDevId);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_AO_EnableChn(AoDevId, AoChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int ao_stop(AUDIO_DEV AoDevId, AO_CHN AoChn)
{
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = HI_MPI_AO_DisableChn(AoDevId, AoChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_AO_Disable(AoDevId);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int aenc_start(HI_S32 s32AencChnCnt, PAYLOAD_TYPE_E enType)
{
    AENC_CHN AeChn = 0;
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_FAILURE;
    AENC_CHN_ATTR_S stAencAttr;
    memset(&stAencAttr, 0, sizeof(stAencAttr));
    AENC_ATTR_ADPCM_S stAdpcmAenc;
    memset(&stAdpcmAenc, 0, sizeof(stAdpcmAenc));
    AENC_ATTR_G711_S stAencG711;
    memset(&stAencG711, 0, sizeof(stAencG711));
    AENC_ATTR_G726_S stAencG726;
    memset(&stAencG726, 0, sizeof(stAencG726));
    AENC_ATTR_LPCM_S stAencLpcm;
    memset(&stAencLpcm, 0, sizeof(stAencLpcm));

    /* set AENC chn attr */
    stAencAttr.enType = enType;
    stAencAttr.u32PtNumPerFrm = g_audio_args->audio.ptNumPerFrm;
    stAencAttr.u32BufSize = 30;

    if (PT_ADPCMA == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAdpcmAenc;
        stAdpcmAenc.enADPCMType = ADPCM_TYPE_DVI4;
    }
    else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG711;
    }
    else if (PT_G726 == stAencAttr.enType)
    {
        stAencAttr.pValue       = &stAencG726;
        stAencG726.enG726bps    = G726_16K;
    }
    else if (PT_LPCM == stAencAttr.enType)
    {
        stAencAttr.pValue = &stAencLpcm;
    }
    else
    {
        DBG("invalid aenc payload type:%d\n", stAencAttr.enType);
        return HI_FAILURE;
    }

    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        /* create aenc chn*/
        s32Ret = HI_MPI_AENC_CreateChn(AeChn, &stAencAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    return HI_SUCCESS;
}

static int aenc_stop(HI_S32 s32AencChnCnt)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_FAILURE;

    for (i=0; i<s32AencChnCnt; i++)
    {
        s32Ret = HI_MPI_AENC_DestroyChn(i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("AENC DestroyChn[%d] done.\n", i);
    }

    return HI_SUCCESS;
}

static int adec_start(ADEC_CHN AdChn, PAYLOAD_TYPE_E enType)
{
    HI_S32 s32Ret = HI_FAILURE;
    ADEC_CHN_ATTR_S stAdecAttr;
    memset(&stAdecAttr, 0, sizeof(stAdecAttr));

    ADEC_ATTR_ADPCM_S stAdpcm;
    memset(&stAdpcm, 0, sizeof(stAdpcm));
    ADEC_ATTR_G711_S stAdecG711;
    memset(&stAdecG711, 0, sizeof(stAdecG711));
    ADEC_ATTR_G726_S stAdecG726;
    memset(&stAdecG726, 0, sizeof(stAdecG726));
    ADEC_ATTR_LPCM_S stAdecLpcm;
    memset(&stAdecLpcm, 0, sizeof(stAdecLpcm));

    stAdecAttr.enType = enType;
    stAdecAttr.u32BufSize = 30;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */

    if (PT_ADPCMA == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdpcm;
        stAdpcm.enADPCMType = ADPCM_TYPE_DVI4 ;
    }
    else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG711;
    }
    else if (PT_G726 == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecG726;
        stAdecG726.enG726bps = G726_16K ;
    }
    else if (PT_LPCM == stAdecAttr.enType)
    {
        stAdecAttr.pValue = &stAdecLpcm;
        stAdecAttr.enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
    }
    else
    {
        DBG("invalid aenc payload type:%d\n", stAdecAttr.enType);
        return HI_FAILURE;
    }

    /* create adec chn*/
    s32Ret = HI_MPI_ADEC_CreateChn(AdChn, &stAdecAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int adec_stop(ADEC_CHN AdecChn)
{
    HI_S32 s32Ret = HI_FAILURE;

    s32Ret = HI_MPI_ADEC_DestroyChn(AdecChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int acodec_init(AUDIO_SAMPLE_RATE_E enSample)
{
    HI_S32 fdAcodec = -1;
    HI_S32 ret = HI_SUCCESS;
    unsigned int i2s_fs_sel = 0;;

    ACODEC_MIXER_E input_mode;

    fdAcodec = open(ACODEC_FILE, O_RDWR);
    CHECK(fdAcodec > 0, -1, "can't open file[%s]: %s\n", ACODEC_FILE, strerror(errno));

    ret = ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    if ((AUDIO_SAMPLE_RATE_8000 == enSample)
        || (AUDIO_SAMPLE_RATE_11025 == enSample)
        || (AUDIO_SAMPLE_RATE_12000 == enSample))
    {
        i2s_fs_sel = 0x18;
    }
    else if ((AUDIO_SAMPLE_RATE_16000 == enSample)
        || (AUDIO_SAMPLE_RATE_22050 == enSample)
        || (AUDIO_SAMPLE_RATE_24000 == enSample))
    {
        i2s_fs_sel = 0x19;
    }
    else if ((AUDIO_SAMPLE_RATE_32000 == enSample)
        || (AUDIO_SAMPLE_RATE_44100 == enSample)
        || (AUDIO_SAMPLE_RATE_48000 == enSample))
    {
        i2s_fs_sel = 0x1a;
    }
    else
    {
        DBG("not support enSample:%d\n", enSample);
        ret = HI_FAILURE;
    }

    ret = ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    //select IN or IN_Difference
    input_mode = ACODEC_MIXER_IN1;
    ret = ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &input_mode);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    int val = 0;
    ret = ioctl(fdAcodec, ACODEC_SET_DACL_MUTE, &val);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    ret = ioctl(fdAcodec, ACODEC_SET_DACR_MUTE, &val);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    close(fdAcodec);
    return ret;
}

static int acodec_set_input_vol(int vol)
{
    HI_S32 fdAcodec = -1;
    int ret = 0;

    fdAcodec = open(ACODEC_FILE, O_RDWR);
    CHECK(fdAcodec > 0, -1, "can't open file[%s]: %s\n", ACODEC_FILE, strerror(errno));

    ret = ioctl(fdAcodec, ACODEC_SET_INPUT_VOL, &vol);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    close(fdAcodec);
    return 0;
}

static int acodec_set_output_vol(int vol)
{
    HI_S32 fdAcodec = -1;
    int ret = 0;

    fdAcodec = open(ACODEC_FILE, O_RDWR);
    CHECK(fdAcodec > 0, -1, "can't open file[%s]: %s\n", ACODEC_FILE, strerror(errno));

    ret = ioctl(fdAcodec, ACODEC_SET_OUTPUT_VOL, &vol);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    close(fdAcodec);
    return 0;
}

static int acodec_set_mic_gain(int gain)
{
    HI_S32 fdAcodec = -1;
    int ret = 0;

    fdAcodec = open(ACODEC_FILE, O_RDWR);
    CHECK(fdAcodec > 0, -1, "can't open file[%s]: %s\n", ACODEC_FILE, strerror(errno));

    ret = ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &gain);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    close(fdAcodec);
    return 0;
}

static PAYLOAD_TYPE_E get_payload_type()
{
    PAYLOAD_TYPE_E pType = PT_LPCM;

    if (!strcmp(g_audio_args->audio.encType, "G.711U"))
    {
        pType = PT_G711U;
    }
    else if (!strcmp(g_audio_args->audio.encType, "G.711A"))
    {
        pType = PT_G711A;
    }
    else if (!strcmp(g_audio_args->audio.encType, "LPCM"))
    {
        pType = PT_LPCM;
    }
    else
    {
        pType = PT_LPCM;
    }

    return pType;
}

static AUDIO_SAMPLE_RATE_E get_samplerate()
{
    AUDIO_SAMPLE_RATE_E enSamplerate = AUDIO_SAMPLE_RATE_8000;   /* sample rate*/

    switch(g_audio_args->audio.sampleRate)
    {
        case 8000:
        {
            enSamplerate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
        case 16000:
        {
            enSamplerate = AUDIO_SAMPLE_RATE_16000;
            break;
        }
        default:
        {
            enSamplerate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
    }

    return enSamplerate;
}

static AUDIO_BIT_WIDTH_E get_bitwidth()
{
    AUDIO_BIT_WIDTH_E enBitwidth = AUDIO_BIT_WIDTH_16;     /* bitwidth*/

    switch(g_audio_args->audio.bitWidth)
    {
        case 8:
        {
            enBitwidth = AUDIO_BIT_WIDTH_8;
            break;
        }
        case 16:
        {
            enBitwidth = AUDIO_BIT_WIDTH_16;
        }
        default:
        {
            enBitwidth = AUDIO_BIT_WIDTH_16;
            break;
        }
    }

    return enBitwidth;
}

static AUDIO_SOUND_MODE_E get_sound_mode()
{
    AUDIO_SOUND_MODE_E  enSoundmode = AUDIO_SOUND_MODE_MONO;    /* momo or steror*/

    switch(g_audio_args->audio.channels)
    {
        case 1:
        {
            enSoundmode = AUDIO_SOUND_MODE_MONO;
            break;
        }
        case 2:
        {
            enSoundmode = AUDIO_SOUND_MODE_STEREO;
            break;
        }
        default:
        {
            enSoundmode = AUDIO_SOUND_MODE_MONO;
            break;
        }
    }

    return enSoundmode;
}

static int aenc_write_cb(char *frame, unsigned long len, double pts)
{
    double timestamp = pts/1000;

    if (g_audio_args->audio.cb)
    {
        g_audio_args->audio.cb(frame, len, timestamp);
    }

    if (AUDIO_LOCAL_TEST)
    {
        static FILE* fp = NULL;

        if (fp == NULL)
        {
            char name[32] = "";
            sprintf(name, "audio.%s", g_audio_args->audio.encType);
            if (!access(name, F_OK))
                remove(name);
            fp = fopen(name, "a+");
            if (fp == NULL)
                DBG("failed to open [%s]: %s\n", name, strerror(errno));
        }

        if (fp)
        {
            fwrite(frame, 1, len, fp);
        }
    }

    return 0;
}

/*
arg：对应以下5个状态
状态0：关闭功放
状态1：12dB 没有防破音
状态2：16dB 有防破音
状态3：24dB 没有防破音
状态4：27.5dB 有防破音功能
*/
static int audio_aw8733a(int arg)
{
    HI_S32 fd = -1;
    int ret = 0;

    fd = open(AW8733A_FILE, O_RDWR);
    CHECK(fd > 0, -1, "can't open file[%s]: %s\n", AW8733A_FILE, strerror(errno));

    ret = ioctl(fd, DRV_CMD_AW8733A, arg);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    close(fd);

    DBG("aw8733a status: %d\n", arg);

    return 0;
}

static HI_VOID * audio_thread(void * pParam)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    HI_S32 s32Ret = HI_FAILURE;
    HI_S32 AencFd = -1;
    AUDIO_STREAM_S stStream;
    memset(&stStream, 0, sizeof(stStream));
    fd_set read_fds;
    FD_ZERO(&read_fds);
    struct timeval TimeoutVal = {0, 0};
    AENC_CHN    AeChn = 0;

    AencFd = HI_MPI_AENC_GetFd(AeChn);
    CHECK(AencFd > 0, NULL, "Error with %#x.\n", s32Ret);

    int header_size = HIS_AUDIO_FRAME_HEADER_LEN;
    if (PT_LPCM == get_payload_type())
    {
        header_size = 0;
    }

    while (g_audio_args->running)
    {
        int pass_time = util_time_pass(&g_audio_args->last_aw8733a_time);
        if (pass_time > 30*1000)
        {
            if (g_audio_args->last_aw8733a_stat > 0)
            {
                g_audio_args->last_aw8733a_stat = 0;
                s32Ret = audio_aw8733a(g_audio_args->last_aw8733a_stat);
                CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            }
        }

        TimeoutVal.tv_sec = 0;
        TimeoutVal.tv_usec = 100*1000;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);
        memset(&stStream, 0, sizeof(stStream));

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            ERR("select error: %s\n", strerror(errno));
            break;
        }
        else if (0 == s32Ret)
        {
            WRN("select time out...\n");
            continue;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            int timeout = 0; // 非阻塞
            s32Ret = HI_MPI_AENC_GetStream(AeChn, &stStream, timeout);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

            if (stStream.u32Len > header_size)
            {
                s32Ret = aenc_write_cb((HI_CHAR *)stStream.pStream+header_size, stStream.u32Len-header_size, stStream.u64TimeStamp);
                CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
            }

            s32Ret = HI_MPI_AENC_ReleaseStream(AeChn, &stStream);
            CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
        }
    }

    return NULL;
}

static int audio_vqe_start()
{
    //HI_S32 s32Ret = -1;
    //AI_TALKVQE_CONFIG_S aiVqeConfig;
    //memset(&aiVqeConfig, 0, sizeof(aiVqeConfig));
/*
    AI_VQE_CONFIG_S getAiVqeConfig;
    HI_S32 s32VolumeDb;
*/
    //aiVqeConfig.bHpfOpen = g_audio_args->vqe_cfg.hpr_enable;
    //aiVqeConfig.bAnrOpen = g_audio_args->vqe_cfg.anr_enable;
    //aiVqeConfig.bAgcOpen = 0;
    //aiVqeConfig.bEqOpen = 0;
    //aiVqeConfig.bAecOpen = 0;
    //aiVqeConfig.bRnrOpen = g_audio_args->vqe_cfg.rnr_enable;;
    //aiVqeConfig.bHdrOpen = 0;
    //aiVqeConfig.s32WorkSampleRate = g_audio_args->audio.sampleRate;
    //aiVqeConfig.s32FrameSample = g_audio_args->audio.ptNumPerFrm;
    //aiVqeConfig.enWorkstate = VQE_WORKSTATE_NOISY;

    //aiVqeConfig.stHpfCfg.bUsrMode = HI_TRUE;
    //aiVqeConfig.stHpfCfg.enHpfFreq = (AUDIO_HPF_FREQ_E)g_audio_args->vqe_cfg.hpr_freq;
    //aiVqeConfig.stAnrCfg.bUsrMode = HI_TRUE;
    //aiVqeConfig.stAnrCfg.s16NrIntensity =  g_audio_args->vqe_cfg.anr_intensity;
    //aiVqeConfig.stAnrCfg.s16NoiseDbThr =  g_audio_args->vqe_cfg.anr_noise_db_thr;
    //aiVqeConfig.stAnrCfg.s8SpProSwitch = 0;

    //aiVqeConfig.stRnrCfg.bUsrMode = HI_TRUE;
    //aiVqeConfig.stRnrCfg.s32NrMode = g_audio_args->vqe_cfg.rnr_nr_mode;
    //aiVqeConfig.stRnrCfg.s32MaxNrLevel = g_audio_args->vqe_cfg.rnr_max_nr_level;
    //aiVqeConfig.stRnrCfg.s32NoiseThresh = g_audio_args->vqe_cfg.rnr_noise_thresh;

    //s32Ret = HI_MPI_AI_SetVqeAttr(0, 0, 0, 0, &aiVqeConfig);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    //s32Ret = HI_MPI_AI_EnableVqe(0, 0);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    //AO_VQE_CONFIG_S aoVqeConfig;
    //memset(&aoVqeConfig, 0, sizeof(aoVqeConfig));

    //aoVqeConfig.bHpfOpen = g_audio_args->vqe_cfg.hpr_enable;
    //aoVqeConfig.bAnrOpen = g_audio_args->vqe_cfg.anr_enable;
    //aoVqeConfig.bAgcOpen = 0;
    //aoVqeConfig.bEqOpen = 0;
    //aoVqeConfig.s32WorkSampleRate = g_audio_args->audio.sampleRate;
    //aoVqeConfig.s32FrameSample = g_audio_args->audio.ptNumPerFrm;
    //aoVqeConfig.enWorkstate = VQE_WORKSTATE_NOISY;

    //aoVqeConfig.stHpfCfg.bUsrMode = HI_TRUE;
    //aoVqeConfig.stHpfCfg.enHpfFreq = (AUDIO_HPF_FREQ_E)g_audio_args->vqe_cfg.hpr_freq;
    //aoVqeConfig.stAnrCfg.bUsrMode = HI_TRUE;
    //aoVqeConfig.stAnrCfg.s16NrIntensity =  g_audio_args->vqe_cfg.anr_intensity;
    //aoVqeConfig.stAnrCfg.s16NoiseDbThr =  g_audio_args->vqe_cfg.anr_noise_db_thr;
    //aoVqeConfig.stAnrCfg.s8SpProSwitch = 0;


    //s32Ret = HI_MPI_AO_SetVqeAttr(0, 0, &aoVqeConfig);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    //s32Ret = HI_MPI_AO_EnableVqe(0, 0);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int audio_aenc_start(AIO_ATTR_S * pstAioAttr)
{
    AUDIO_DEV   AiDev = 0;
    AI_CHN      AiChn = 0;
    AENC_CHN    AeChn = 0;
    HI_S32      s32AiChnCnt = pstAioAttr->u32ChnCnt;
    HI_S32      s32AencChnCnt = 1;

    HI_S32 s32Ret = ai_start(AiDev, s32AiChnCnt, pstAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = aenc_start(s32AencChnCnt, get_payload_type());
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = aenc_bind_ai(AiDev, AiChn, AeChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    g_audio_args->running = 1;
    s32Ret = pthread_create(&g_audio_args->pid, NULL, audio_thread, NULL);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int audio_aenc_stop()
{
    HI_S32 s32Ret = HI_FAILURE;
    AUDIO_DEV   AiDev = 0;
    AI_CHN      AiChn = 0;
    AENC_CHN    AeChn = 0;
    HI_S32      s32AencChnCnt = 1;
    HI_S32      s32AiChnCnt = 1;

    if (g_audio_args->running)
    {
        g_audio_args->running = 0;
        pthread_join(g_audio_args->pid, NULL);
    }

    s32Ret = aenc_unbind_ai(AiDev, AiChn, AeChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("AENC[%d] unbind AI[%d] done.\n", AeChn, AiChn);

    s32Ret = aenc_stop(s32AencChnCnt);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = ai_stop(AiDev, s32AiChnCnt);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int audio_adec_start(AIO_ATTR_S * pstAioAttr)
{
    HI_S32 s32Ret= HI_SUCCESS;
    ADEC_CHN    AdChn = 0;
    AUDIO_DEV   AoDev = 0;
    AO_CHN      AoChn = 0;

    s32Ret = adec_start(AdChn, get_payload_type());
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = ao_start(AoDev, AoChn, pstAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = ao_bind_adec(AoDev, AoChn, AdChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int audio_adec_stop()
{
    HI_S32 s32Ret= HI_SUCCESS;
    ADEC_CHN    AdChn = 0;
    AUDIO_DEV   AoDev = 0;
    AO_CHN      AoChn = 0;

    s32Ret = ao_unbind_adec(AoDev, AoChn, AdChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = ao_stop(AoDev, AoChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = adec_stop(AoDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int audio_vqe_config_load()
{
    g_audio_args->vqe_cfg.vqe_enbale = 1;
    g_audio_args->vqe_cfg.hpr_enable = 0;
    g_audio_args->vqe_cfg.hpr_freq = 120;
    g_audio_args->vqe_cfg.anr_enable = 1;
    g_audio_args->vqe_cfg.anr_intensity = 20;
    g_audio_args->vqe_cfg.anr_noise_db_thr = 40;
    g_audio_args->vqe_cfg.rnr_enable = 0;
    g_audio_args->vqe_cfg.rnr_nr_mode = 0;
    g_audio_args->vqe_cfg.rnr_max_nr_level = 15;
    g_audio_args->vqe_cfg.rnr_noise_thresh = -55;

    return 0;
}

int sal_audio_init(sal_audio_s *audio)
{
    CHECK(NULL == g_audio_args, HI_FAILURE, "reinit error, please exit first.\n");
    CHECK(NULL != audio, HI_FAILURE, "null ptr error.\n");
    CHECK(audio->enable, HI_FAILURE, "audio enable %#x.\n", audio->enable);

    HI_S32 s32Ret = -1;
    g_audio_args = (sal_audio_args_s*)malloc(sizeof(sal_audio_args_s));
    CHECK(g_audio_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_audio_args_s));

    memset(g_audio_args, 0, sizeof(sal_audio_args_s));
    pthread_mutex_init(&g_audio_args->mutex, NULL);
    memcpy(&g_audio_args->audio, audio, sizeof(*audio));

    s32Ret = audio_vqe_config_load();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    g_audio_args->input_vol = 26;
    g_audio_args->mic_vol = 4;
    g_audio_args->output_vol = 0; // [-121,6]

    g_audio_args->dec_buf = (unsigned char *)malloc(g_audio_args->audio.ptNumPerFrm*4);
    CHECK(g_audio_args->dec_buf != NULL, -1, "malloc %d bytes failed.\n", sizeof(sal_audio_args_s));

    AIO_ATTR_S stAioAttr;
    memset(&stAioAttr, 0, sizeof(stAioAttr));
    stAioAttr.enSamplerate  = get_samplerate();
    stAioAttr.enBitwidth    = get_bitwidth();
    stAioAttr.enWorkmode    = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode   = get_sound_mode();
    stAioAttr.u32EXFlag     = 0;
    stAioAttr.u32FrmNum     = 30;
    stAioAttr.u32PtNumPerFrm = g_audio_args->audio.ptNumPerFrm;
    stAioAttr.u32ChnCnt     = 1;
    stAioAttr.u32ClkSel     = 0;

    s32Ret = acodec_init(stAioAttr.enSamplerate);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = audio_aenc_start(&stAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = audio_adec_start(&stAioAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (g_audio_args->vqe_cfg.vqe_enbale)
    {
        s32Ret = audio_vqe_start();
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = acodec_set_input_vol(g_audio_args->input_vol);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = acodec_set_mic_gain(g_audio_args->mic_vol);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = acodec_set_output_vol(g_audio_args->output_vol);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    s32Ret = sal_audio_capture_volume_set(g_audio_args->audio.volume);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    //g_audio_args->last_aw8733a_stat = 0;
    //s32Ret = audio_aw8733a(g_audio_args->last_aw8733a_stat);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    //util_time_abs(&g_audio_args->last_aw8733a_time);

    return HI_SUCCESS;
}


int sal_audio_exit(void)
{
    CHECK(NULL != g_audio_args, HI_FAILURE, "module not inited.\n");
    CHECK(g_audio_args->audio.enable, HI_FAILURE, "audio enable %#x.\n", g_audio_args->audio.enable);

    HI_S32 s32Ret = -1;

    s32Ret = audio_adec_stop();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = audio_aenc_stop();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (g_audio_args->dec_buf)
    {
        free(g_audio_args->dec_buf);
        g_audio_args->dec_buf = NULL;
    }

    if (g_audio_args->last_aw8733a_stat > 0)
    {
        g_audio_args->last_aw8733a_stat = 0;
        s32Ret = audio_aw8733a(g_audio_args->last_aw8733a_stat);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_mutex_destroy(&g_audio_args->mutex);
    free(g_audio_args);
    g_audio_args = NULL;

    return HI_SUCCESS;
}

int sal_audio_play(char *frame, int frame_len)
{
    CHECK(NULL != g_audio_args, HI_FAILURE, "module not inited.\n");

    int s32Ret = -1;
    if (g_audio_args->last_aw8733a_stat == 0)
    {
        g_audio_args->last_aw8733a_stat = 4;
        s32Ret = audio_aw8733a(g_audio_args->last_aw8733a_stat);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    util_time_abs(&g_audio_args->last_aw8733a_time);

    ADEC_CHN    AdChn = 0;
    AUDIO_STREAM_S stStream;
    memset(&stStream, 0, sizeof(stStream));
    unsigned char audioHeader[HIS_AUDIO_FRAME_HEADER_LEN];
    memset(audioHeader, 0, sizeof(audioHeader));

    stStream.pStream = g_audio_args->dec_buf;

    audioHeader[0] = 0;
    audioHeader[1] = 1;
    audioHeader[2] = 0xa0;
    audioHeader[3] = 0;

    int header_size = 4;
    if (PT_LPCM == get_payload_type())
    {
        header_size = 0;
    }
    else
    {
        memcpy(stStream.pStream, audioHeader, header_size);
    }

    memcpy(stStream.pStream + header_size, frame, frame_len);
    stStream.u32Len = frame_len + header_size;

    s32Ret = HI_MPI_ADEC_SendStream(AdChn, &stStream, HI_TRUE);
    if(s32Ret != HI_SUCCESS)
    {
        WRN("HI_MPI_ADEC_SendStream failed.%#x\n", s32Ret);
        s32Ret = HI_MPI_ADEC_ClearChnBuf(AdChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    return 0;
}

int sal_audio_capture_volume_set(int vol)
{
    CHECK(NULL != g_audio_args, HI_FAILURE, "module not inited.\n");

    //default vol
    if (vol == 25)
    {
        acodec_set_mic_gain(g_audio_args->mic_vol);
        return 0;
    }

    if(vol < 0 || vol > 100)
    {
        DBG("set input volume failed, volume %d is out of range, should be [0, 100]\n", vol);
        return -1;
    }

/*
Hi3518EV200/Hi3519V100 的模拟增益范围为[0, 16]，其中 0 到 15 按 2db 递增， 0
对应 0db， 15 对应最大增益 30db，而 16 对应的是-1.5db。
*/
    int mic_gain_table[17] = {16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    int tbl_idx = vol*16/100; //index range [0,16]
    acodec_set_mic_gain(mic_gain_table[tbl_idx]);

    return 0;
}

int sal_audio_play_file(char *path)
{
    CHECK(NULL != g_audio_args, HI_FAILURE, "module not inited.\n");

    FILE* fp = fopen(path, "r");
    CHECK(fp, -1, "error with %s\n", strerror(errno));

    int frame_size = g_audio_args->audio.ptNumPerFrm;
    char buffer[1024];
    while (!feof(fp))
    {
        memset(buffer, 0, sizeof(buffer));
        int len = fread(buffer, 1, frame_size, fp);
        if (len == frame_size)
        {
            sal_audio_play(buffer, frame_size);
        }
    }

    fclose(fp);
    DBG("play %s done.\n", path);

    return 0;

}

