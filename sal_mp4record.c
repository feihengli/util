
#include <stdlib.h>
#include "mp4v2/mp4v2.h"
#include "sal_mp4record.h"
#include "sal_debug.h"


typedef struct MP4V2_CONTEXT
{
    int m_vWidth;
    int m_vHeight;
    int m_vFrateR;
    int m_vTimeScale;
    MP4FileHandle m_mp4FHandle;
    MP4TrackId m_vTrackId;
    MP4TrackId m_aTrackId;
    double m_vFrameDur;
    int Flag[2];
    
    unsigned char* buffer;
    int buffer_size;
}MP4V2_CONTEXT;

static int mp4EncoderVframeSplit(unsigned char* _pu8Frame, unsigned int _u32FrameSize)
{
    CHECK(_pu8Frame[0] == 0x00 && _pu8Frame[1] == 0x00 && _pu8Frame[2] == 0x00 && _pu8Frame[3] == 0x01, -1, "invalid parameter\n");

    unsigned int i;
    for(i = 4; i < _u32FrameSize - 4; i++)
    {
        if(_pu8Frame[i+0] == 0x00 && _pu8Frame[i+1] == 0x00 && _pu8Frame[i+2] == 0x00 && _pu8Frame[i+3] == 0x01)
        {
            return i;
        }
    }
    return -1;
}

handle mp4EncoderInit(const char* filename,int width,int height, int fps)
{
    int ret = -1;
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)malloc(sizeof(struct MP4V2_CONTEXT));
    CHECK(recordCtx, NULL, "failed to malloc %d\n", sizeof(struct MP4V2_CONTEXT));
    
    memset(recordCtx, 0, sizeof(*recordCtx));
    recordCtx->m_vWidth = width;
    recordCtx->m_vHeight = height;
    recordCtx->m_vFrateR = fps;
    recordCtx->m_vTimeScale = 90000;
    recordCtx->m_vFrameDur = 300;
    recordCtx->m_vTrackId = 0;
    recordCtx->m_aTrackId = 0;
    recordCtx->Flag[0] = 1;
    recordCtx->Flag[1] = 1;
    
    recordCtx->buffer_size = 1*1024*1024; //width*height*2/5;
    recordCtx->buffer = (unsigned char*)malloc(recordCtx->buffer_size);
    CHECK(recordCtx->buffer, NULL, "failed to malloc %d\n", recordCtx->buffer);
    
    recordCtx->m_mp4FHandle = MP4Create(filename, 0);
    CHECK(recordCtx->m_mp4FHandle, NULL, "error with %#x\n", recordCtx->m_mp4FHandle);

    ret = MP4SetTimeScale(recordCtx->m_mp4FHandle, recordCtx->m_vTimeScale);
    CHECK(ret, NULL, "error with %#x\n", ret);
    
    //------------------------------------------------------------------------------------- audio track
//    recordCtx->m_aTrackId = MP4AddAudioTrack(recordCtx->m_mp4FHandle, 44100, 1024, MP4_MPEG4_AUDIO_TYPE);
//    if (recordCtx->m_aTrackId == MP4_INVALID_TRACK_ID){
//        printf("error : MP4AddAudioTrack  \n");
//        return ret;
//    }
//
//    MP4SetAudioProfileLevel(recordCtx->m_mp4FHandle, 0x2);
//    uint8_t aacConfig[2] = {18,16};
//    MP4SetTrackESConfiguration(recordCtx->m_mp4FHandle,recordCtx->m_aTrackId,aacConfig,2);
//    printf("ok  : initMp4Encoder file=%s  \n",filename);

    return recordCtx;
}

int mp4VEncoderWrite(handle hndMp4, unsigned char* frame ,int size)
{
    CHECK(frame && size > 5 && 
            frame[0]==0 && frame[1]==0 && frame[2]==0 && frame[3]==1, -1, "invalid nalu\n");
    CHECK(hndMp4, -1, "invalid parameter with %#x\n", hndMp4);
    
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)hndMp4;
    int ret = -1;
    
    CHECK(size <= recordCtx->buffer_size, -1, "buffer is too small with %d\n", recordCtx->buffer_size);
    memcpy(recordCtx->buffer, frame, size);
    
    unsigned char* pu8Frame = NULL;
    unsigned int u32FrameSize = 0;
    unsigned char* pu8Tmp = (unsigned char*)recordCtx->buffer;
    unsigned int u32TmpSize = size;
    int offset;
    do
    {
        offset = mp4EncoderVframeSplit(pu8Tmp, u32TmpSize);
        if (offset != -1)
        {
            //DBG("offset=%d", offset);
            pu8Frame = pu8Tmp;
            u32FrameSize = offset;

            pu8Tmp += offset;
            u32TmpSize -= offset;
        }
        else
        {
            //DBG("u32TmpSize=%u", u32TmpSize);
            pu8Frame = pu8Tmp;
            u32FrameSize = u32TmpSize;
        }
    
        //第一次进来必须是sps，其次是pps，idr p ...p...p
        unsigned char type = pu8Frame[4];

        if (type == 0x67)
        {
            if (recordCtx->Flag[0])
            {
                //DBG("sps\n");
                if(recordCtx->m_vTrackId == MP4_INVALID_TRACK_ID)
                {
                    recordCtx->m_vTrackId = MP4AddH264VideoTrack(recordCtx->m_mp4FHandle,
                                                                recordCtx->m_vTimeScale,
                                                                recordCtx->m_vTimeScale / recordCtx->m_vFrateR,
                                                                recordCtx->m_vWidth,     // width
                                                                recordCtx->m_vHeight,    // height
                                                                pu8Frame[5], // sps[1] AVCProfileIndication
                                                                pu8Frame[6], // sps[2] profile_compat
                                                                pu8Frame[7], // sps[3] AVCLevelIndication
                                                                3);           // 4 bytes length before each NAL unit
                    CHECK(recordCtx->m_vTrackId != MP4_INVALID_TRACK_ID, -1, "error with %#x\n", recordCtx->m_vTrackId);

                    MP4SetVideoProfileLevel(recordCtx->m_mp4FHandle, 0x7F); //  Simple Profile @ Level 3
                }
                MP4AddH264SequenceParameterSet(recordCtx->m_mp4FHandle,recordCtx->m_vTrackId,pu8Frame+4,u32FrameSize-4);
                recordCtx->Flag[0] = 0;
            }
        }
        else if (type == 0x68)
        {
            if (recordCtx->Flag[1])
            {
                //DBG("pps\n");
                MP4AddH264PictureParameterSet(recordCtx->m_mp4FHandle,recordCtx->m_vTrackId,pu8Frame+4,u32FrameSize-4);
                recordCtx->Flag[1] = 0;
            }
        }
        else if (type == 0x06)
        {
            //SEI do nothing
        }
        else if (type == 0x65 || type == 0x61)
        {
            //DBG("idr/p\n");
            //开始码去掉 换成_naluSize且为大端顺序
            pu8Frame[0] = (u32FrameSize-4) >>24;  
            pu8Frame[1] = (u32FrameSize-4) >>16;  
            pu8Frame[2] = (u32FrameSize-4) >>8;  
            pu8Frame[3] = (u32FrameSize-4) &0xff;

            ret = MP4WriteSample(recordCtx->m_mp4FHandle, recordCtx->m_vTrackId, pu8Frame, u32FrameSize, MP4_INVALID_DURATION, 0, 1);
            CHECK(ret, -1, "error with #%x\n", ret);
        }
        else
        {
            WRN("Unknown nalu type with %02x\n", type);
        }
    }
    while (offset != -1);
        
    return 0;
}

int mp4AEncoderWrite(handle hndMp4, unsigned char* frame ,int size)
{
    CHECK(hndMp4, -1, "invalid parameter with %#x\n", hndMp4);
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)hndMp4;
    int ret = -1;

    ret = MP4WriteSample(recordCtx->m_mp4FHandle, recordCtx->m_aTrackId, frame, size , MP4_INVALID_DURATION, 0, 1);
    CHECK(ret, -1, "error with #%x\n", ret);
    
    recordCtx->m_vFrameDur += 1024;
    return 0;
}

int mp4Encoderclose(handle hndMp4)
{
    CHECK(hndMp4, -1, "invalid parameter with %#x\n", hndMp4);
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)hndMp4;

    if (recordCtx->m_mp4FHandle != MP4_INVALID_FILE_HANDLE) 
    {
        MP4Close(recordCtx->m_mp4FHandle,0);
        recordCtx->m_mp4FHandle = NULL;
    }
    
    if (recordCtx->buffer != NULL)
    {
        free(recordCtx->buffer);
        recordCtx->buffer = NULL;
        recordCtx->buffer_size = 0;
    }
    
    free(recordCtx);
    recordCtx = NULL;
    return 0;
}
