#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <mp4v2/mp4v2.h>

#include "sal_debug.h"
#include "file_frame_source_mp4.h"

#define SAVE_2_FILE_ENABLE 1

typedef struct frame_source_mp4_s
{
    unsigned char buffer[1*1024*1024];
    unsigned char start_code[4];
    unsigned char vps[128]; //only h265
    int vps_len;
    unsigned char sps[128];
    int sps_len;
    unsigned char pps[128];
    int pps_len;
    unsigned char sei[128];
    int sei_len;

    video_frame_cb frame_cb;
    double pts;
    
    MP4FileHandle oMp4File;
    char path[64];
    
    MP4TrackId VtrackId;
    MP4TrackId AtrackId;
    
    unsigned int VFrameCount;
    unsigned int AFrameCount;
    
    int Vfps;
    int Afps;
    
    int running;
    pthread_t pid;
}frame_source_mp4_s;

static int frame_source_mp4_save2file(int stream_id, const char* file, const void* addr, int size)
{
    static FILE* fp[2] = {NULL, NULL};

    if (fp[stream_id] == NULL)
    {
        char name[32] = "";
        sprintf(name, "%s_%d", file, stream_id);
        if (!access(name, F_OK))
            remove(name);
        fp[stream_id] = fopen(name, "a+");
        if (fp[stream_id] == NULL)
            DBG("failed to open [%s]: %s\n", name, strerror(errno));
    }

    if (fp[stream_id])
    {
        fwrite(addr, 1, size, fp[stream_id]);
    }

    return 0;
}

static int frame_source_mp4_open(frame_source_mp4_s* pstFrameSource)
{
    int i = 0;
    bool bRet = 0;
    
    pstFrameSource->oMp4File = MP4Read(pstFrameSource->path);
    CHECK(pstFrameSource->oMp4File, -1, "error with %#x\n", pstFrameSource->oMp4File);
    
    uint32_t numSamples = 0;
    uint32_t timescale = 0;
    uint64_t duration = 0;
    unsigned int oStreamDuration = 0;
    double fps = 0.0;
    MP4TrackId trackId = MP4_INVALID_TRACK_ID;
    
    uint32_t numTracks = MP4GetNumberOfTracks(pstFrameSource->oMp4File,NULL,0);
    CHECK(numTracks > 0, -1, "error with %d\n", numTracks);
    
    DBG("numTracks:%d\n",numTracks);

    for (i = 0; i < numTracks; i++)
    {
        trackId = MP4FindTrackId(pstFrameSource->oMp4File, i,NULL,0);
        CHECK(trackId != MP4_INVALID_TRACK_ID, -1, "error with %d\n", trackId);
        
        const char* trackType = MP4GetTrackType(pstFrameSource->oMp4File, trackId);
        if (MP4_IS_VIDEO_TRACK_TYPE(trackType))
        {
            pstFrameSource->VtrackId = trackId;
            DBG("VtrackId: %d\n", pstFrameSource->VtrackId);

            duration = MP4GetTrackDuration(pstFrameSource->oMp4File, trackId);
            numSamples = MP4GetTrackNumberOfSamples(pstFrameSource->oMp4File, trackId);
            timescale = MP4GetTrackTimeScale(pstFrameSource->oMp4File, trackId);
            oStreamDuration = duration/(timescale/1000);//ms
            pstFrameSource->VFrameCount = numSamples;
            fps = pstFrameSource->VFrameCount*1000.0/oStreamDuration;
            pstFrameSource->Vfps = fps;
            DBG("VFrameCount: %u, duration: %llu, timescale: %u,\n oStreamDuration: %ums, fps: %d\n", pstFrameSource->VFrameCount, duration, timescale, oStreamDuration, pstFrameSource->Vfps);

            // read sps/pps 
            uint8_t **seqheader;
            uint8_t **pictheader;
            uint32_t *pictheadersize;
            uint32_t *seqheadersize;
            uint32_t ix;
            bRet = MP4GetTrackH264SeqPictHeaders(pstFrameSource->oMp4File, trackId, &seqheader, &seqheadersize, &pictheader, &pictheadersize);
            CHECK(bRet == 1, -1, "error with %#x\n", bRet);
            
            for (ix = 0; seqheadersize[ix] != 0; ix++)
            {
                memcpy(pstFrameSource->sps, seqheader[ix], seqheadersize[ix]);
                pstFrameSource->sps_len = seqheadersize[ix];
                free(seqheader[ix]);
            }
            free(seqheader);
            free(seqheadersize);

            for (ix = 0; pictheadersize[ix] != 0; ix++)
            {
                memcpy(pstFrameSource->pps, pictheader[ix], pictheadersize[ix]);
                pstFrameSource->pps_len = pictheadersize[ix];
                free(pictheader[ix]);
            }
           free(pictheader);
           free(pictheadersize);
        }
        else if (MP4_IS_AUDIO_TRACK_TYPE(trackType))
        {
            pstFrameSource->AtrackId = trackId;
            DBG("AtrackId:%d\n",pstFrameSource->AtrackId);
        }
        else
        {
            ERR("invalid trackType: %s\n", trackType);
            return -1;
        }
    }

    return 0;
}

static void* frame_source_mp4_proc(void* arg)
{
    int ret = -1;
    frame_source_mp4_s* pstFrameSource = (frame_source_mp4_s*)arg;
    
    ret = frame_source_mp4_open(pstFrameSource);
    CHECK(ret == 0, NULL, "error with %#x\n", ret);
    
    double timestamp_rise = 1*1000*1000.0/pstFrameSource->Vfps;
    DBG("timestamp_rise: %f\n", timestamp_rise);
    unsigned char *pData = NULL;
    unsigned int nSize = 0;
    MP4Timestamp pStartTime;
    MP4Duration pDuration;
    MP4Duration pRenderingOffset;
    bool pIsSyncSample = 0;

    int nReadIndex = 0;
    int offset = 0;
    int isKey = 0;
    while (pstFrameSource->running && nReadIndex < pstFrameSource->VFrameCount)
    {   
        offset = 0;
        nReadIndex++;
        isKey = 0;
        ret = MP4ReadSample(pstFrameSource->oMp4File,pstFrameSource->VtrackId,nReadIndex,&pData,&nSize,&pStartTime,&pDuration,&pRenderingOffset,&pIsSyncSample);
        CHECK(ret == 1, NULL, "error with %#x\n", ret);
        
        //IDR帧，写入sps pps先
        if(pIsSyncSample)
        {
            memcpy(pstFrameSource->buffer+offset, pstFrameSource->start_code, sizeof(pstFrameSource->start_code));
            offset += sizeof(pstFrameSource->start_code);
            memcpy(pstFrameSource->buffer+offset, pstFrameSource->sps, pstFrameSource->sps_len);
            offset += pstFrameSource->sps_len;
            memcpy(pstFrameSource->buffer+offset, pstFrameSource->start_code, sizeof(pstFrameSource->start_code));
            offset += sizeof(pstFrameSource->start_code);
            memcpy(pstFrameSource->buffer+offset, pstFrameSource->pps, pstFrameSource->pps_len);
            offset += pstFrameSource->pps_len;
            isKey = 1;
        }

        //264frame
        if(pData && nSize > 4)
        {
            //标准的264帧，前面几个字节就是frame的长度.
            //需要替换为标准的264 nal 头.
            pData[0] = 0x00;
            pData[1] = 0x00;
            pData[2] = 0x00;
            pData[3] = 0x01;
            CHECK(nSize <= (sizeof(pstFrameSource->buffer)-offset), NULL, "buffer is too small %d\n", (sizeof(pstFrameSource->buffer)-offset));
            memcpy(pstFrameSource->buffer+offset, pData, nSize);
            offset += nSize;
            
            if (SAVE_2_FILE_ENABLE)
            {
                frame_source_mp4_save2file(0, "out_h264", pstFrameSource->buffer, offset);
            }
            if (pstFrameSource->frame_cb)
            {
                pstFrameSource->frame_cb(0, pstFrameSource->buffer, offset, isKey, pstFrameSource->pts);
                pstFrameSource->pts += timestamp_rise;
            }
        }

        //如果传入MP4ReadSample的视频pData是null
        // 它内部就会new 一个内存
        //如果传入的是已知的内存区域，
        //则需要保证空间bigger then max frames size.
        free(pData);
        pData = NULL;
        usleep(timestamp_rise);
    }

    MP4Close(pstFrameSource->oMp4File, 0);
    return 0;
}

handle frame_source_mp4_init(const char* path, video_frame_cb frame_cb)
{
    frame_source_mp4_s* pstFrameSourceMp4 = (frame_source_mp4_s*)malloc(sizeof(frame_source_mp4_s));
    CHECK(pstFrameSourceMp4, NULL, "failed to malloc %d\n", sizeof(frame_source_mp4_s));
    
    memset(pstFrameSourceMp4, 0, sizeof(*pstFrameSourceMp4));
    pstFrameSourceMp4->frame_cb = frame_cb;
    char NAL[4] = {0x00,0x00,0x00,0x01};
    memcpy(pstFrameSourceMp4->start_code, NAL, sizeof(NAL));
    strcpy(pstFrameSourceMp4->path, path);

    pstFrameSourceMp4->running = 1;
    pthread_create(&pstFrameSourceMp4->pid, NULL, frame_source_mp4_proc, pstFrameSourceMp4);

    return pstFrameSourceMp4;
}

int frame_source_mp4_exit(handle hndMp4)
{
    frame_source_mp4_s* pstFrameSourceMp4 = (frame_source_mp4_s*)hndMp4;
    if (pstFrameSourceMp4->running)
    {
        pstFrameSourceMp4->running = 0;
        pthread_join(pstFrameSourceMp4->pid, NULL);
    }
    
    free(pstFrameSourceMp4);
    pstFrameSourceMp4 = NULL;

    return 0;
}



