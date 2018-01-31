
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
}MP4V2_CONTEXT;

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

int mp4VEncoderWrite(handle hndMp4, unsigned char* _naluData ,int _naluSize)
{
    CHECK(_naluData && _naluSize > 5 && 
            _naluData[0]==0 && _naluData[1]==0 && _naluData[2]==0 && _naluData[3]==1, -1, "invalid nalu\n");
    CHECK(hndMp4, -1, "invalid parameter with %#x\n", hndMp4);
    
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)hndMp4;
    int ret = -1;
    
    //第一次进来必须是sps，其次是pps，idr p ...p...p
    unsigned char type = _naluData[4];

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
                                                            _naluData[5], // sps[1] AVCProfileIndication
                                                            _naluData[6], // sps[2] profile_compat
                                                            _naluData[7], // sps[3] AVCLevelIndication
                                                            3);           // 4 bytes length before each NAL unit
                CHECK(recordCtx->m_vTrackId != MP4_INVALID_TRACK_ID, -1, "error with %#x\n", recordCtx->m_vTrackId);

                MP4SetVideoProfileLevel(recordCtx->m_mp4FHandle, 0x7F); //  Simple Profile @ Level 3
            }
            MP4AddH264SequenceParameterSet(recordCtx->m_mp4FHandle,recordCtx->m_vTrackId,_naluData+4,_naluSize-4);
            recordCtx->Flag[0] = 0;
        }
    }
    else if (type == 0x68)
    {
        if (recordCtx->Flag[1])
        {
            //DBG("pps\n");
            MP4AddH264PictureParameterSet(recordCtx->m_mp4FHandle,recordCtx->m_vTrackId,_naluData+4,_naluSize-4);
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
        _naluData[0] = (_naluSize-4) >>24;  
        _naluData[1] = (_naluSize-4) >>16;  
        _naluData[2] = (_naluSize-4) >>8;  
        _naluData[3] = (_naluSize-4) &0xff;

        ret = MP4WriteSample(recordCtx->m_mp4FHandle, recordCtx->m_vTrackId, _naluData, _naluSize, MP4_INVALID_DURATION, 0, 1);
        CHECK(ret, -1, "error with #%x\n", ret);
    }
    else
    {
        WRN("Unknown nalu type with %02x\n", type);
    }
        
    return 0;
}

int mp4AEncoderWrite(handle hndMp4, unsigned char* data ,int len)
{
    CHECK(hndMp4, -1, "invalid parameter with %#x\n", hndMp4);
    MP4V2_CONTEXT* recordCtx = (MP4V2_CONTEXT*)hndMp4;
    int ret = -1;

    ret = MP4WriteSample(recordCtx->m_mp4FHandle, recordCtx->m_aTrackId, data, len , MP4_INVALID_DURATION, 0, 1);
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
    
    free(recordCtx);
    recordCtx = NULL;
    return 0;
}
