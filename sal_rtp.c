
#include "sal_debug.h"
#include "sal_malloc.h"
#include "sal_rtp.h"


#define ARM_FRAME_SIZE (32) //amr12.2, 1bytes frame header:0x3c, 31bytes data:12200/50=244bits=30.5bytes=31bytes
#define ARM_FRAME_HEADER (0x3C)
//only for 1 frame amr!!!!!!

int rtp_amr_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                      unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                      unsigned long _Ssrc, RTP_SPLIT_S* _pRetRtpSplit)
{
    CHECK(_pu8Frame != NULL, -1, "invalid parameter with: %#x\n", _pu8Frame);
    CHECK(_u32FrameSize > 5, -1, "invalid parameter with: %#x\n", _u32FrameSize);
    CHECK(_pRetRtpSplit != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit);
    CHECK(_pu8Frame[0] == ARM_FRAME_HEADER, -1, "amr frame is invalid, %02x\n", (unsigned int)_pu8Frame[0]);

#define RTP_AMR_HEADER_SIZE (sizeof(RTP_FIXED_HEADER) + 1/*2*/) //1 is for 0xf0(only 1, unknown), other for 0xbc
    CHECK(RTP_AMR_HEADER_SIZE == 13/*14*/, -1, "RTP_AMR_HEADER_SIZE %u\n", RTP_AMR_HEADER_SIZE);

    _pRetRtpSplit->u32SegmentCount = 1;
    _pRetRtpSplit->u32BufSize = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_AMR_HEADER_SIZE + _u32FrameSize;
    _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
    _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
    _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));

    RTSP_INTERLEAVED_FRAME stRtspInterleavedFrame;
    RTP_FIXED_HEADER stRtpFixedHeader;
    unsigned char u8FixHeader = 0xf0;

    memset(&stRtspInterleavedFrame, 0, sizeof(RTSP_INTERLEAVED_FRAME));
    memset(&stRtpFixedHeader, 0, sizeof(RTP_FIXED_HEADER));

    RTSP_INTERLEAVED_FRAME* rtsp_intereaved = &stRtspInterleavedFrame;
    RTP_FIXED_HEADER* rtp_hdr = &stRtpFixedHeader;

    rtsp_intereaved->flag = '$';
    rtsp_intereaved->channel = 2;

    rtp_hdr->version = 2;
    rtp_hdr->marker  = 0;
    rtp_hdr->payload = rtp_payload_get("AMR");
    rtp_hdr->ssrc    = htonl(_Ssrc);

    unsigned char* pu8BufCurr = _pRetRtpSplit->pu8Buf;

    _pRetRtpSplit->ppu8Segment[0] = pu8BufCurr;
    _pRetRtpSplit->pU32SegmentSize[0] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_AMR_HEADER_SIZE + _u32FrameSize;

    rtsp_intereaved->size = htons(RTP_AMR_HEADER_SIZE + _u32FrameSize);

    rtp_hdr->marker=1;
    rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
    rtp_hdr->timestamp = htonl(_u32Timestamp);

    memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
    pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
    memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
    pu8BufCurr += sizeof(RTP_FIXED_HEADER);
    memcpy(pu8BufCurr, &u8FixHeader, sizeof(u8FixHeader));
    pu8BufCurr += sizeof(u8FixHeader);
    memcpy(pu8BufCurr, _pu8Frame, _u32FrameSize);
    pu8BufCurr += _u32FrameSize;

    return 0;
}

int rtp_g711a_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                      unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                      unsigned long _Ssrc, RTP_SPLIT_S* _pRetRtpSplit)
{
    CHECK(_pu8Frame != NULL, -1, "invalid parameter with: %#x\n", _pu8Frame);
    CHECK(_u32FrameSize > 5, -1, "invalid parameter with: %#x\n", _u32FrameSize);
    CHECK(_pRetRtpSplit != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit);
    //CHECK(_pu8Frame[0] == ARM_FRAME_HEADER, -1, "amr frame is invalid, %02x\n", (unsigned int)_pu8Frame[0]);

#define RTP_G711_HEADER_SIZE (sizeof(RTP_FIXED_HEADER))
    CHECK(RTP_G711_HEADER_SIZE == 12, -1, "RTP_G711_HEADER_SIZE %u\n", RTP_G711_HEADER_SIZE);

    _pRetRtpSplit->u32SegmentCount = 1;
    _pRetRtpSplit->u32BufSize = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_G711_HEADER_SIZE + _u32FrameSize;
    _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
    _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
    _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));

    RTSP_INTERLEAVED_FRAME stRtspInterleavedFrame;
    RTP_FIXED_HEADER stRtpFixedHeader;

    memset(&stRtspInterleavedFrame, 0, sizeof(RTSP_INTERLEAVED_FRAME));
    memset(&stRtpFixedHeader, 0, sizeof(RTP_FIXED_HEADER));

    RTSP_INTERLEAVED_FRAME* rtsp_intereaved = &stRtspInterleavedFrame;
    RTP_FIXED_HEADER* rtp_hdr = &stRtpFixedHeader;

    rtsp_intereaved->flag = '$';
    rtsp_intereaved->channel = 2;

    rtp_hdr->version = 2;
    rtp_hdr->marker  = 0;
    rtp_hdr->payload = rtp_payload_get("G.711A");
    rtp_hdr->ssrc    = htonl(_Ssrc);

    unsigned char* pu8BufCurr = _pRetRtpSplit->pu8Buf;

    _pRetRtpSplit->ppu8Segment[0] = pu8BufCurr;
    _pRetRtpSplit->pU32SegmentSize[0] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_G711_HEADER_SIZE + _u32FrameSize;

    rtsp_intereaved->size = htons(RTP_G711_HEADER_SIZE + _u32FrameSize);

    rtp_hdr->marker=1;
    rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
    rtp_hdr->timestamp = htonl(_u32Timestamp);

    memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
    pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
    memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
    pu8BufCurr += sizeof(RTP_FIXED_HEADER);
    memcpy(pu8BufCurr, _pu8Frame, _u32FrameSize);
    pu8BufCurr += _u32FrameSize;

    return 0;
}

int rtp_vframe_split(unsigned char* _pu8Frame, unsigned int _u32FrameSize)
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

int rtp_h264_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                       unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                       RTP_SPLIT_S* _pRetRtpSplit)
{
    CHECK(_pu8Frame != NULL, -1, "invalid parameter with: %#x\n", _pu8Frame);
    CHECK(_u32FrameSize > 5, -1, "invalid parameter with: %#x\n", _u32FrameSize);
    CHECK(_pRetRtpSplit != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit);

    CHECK(_pu8Frame[0] == 0x00 && _pu8Frame[1] == 0x00 && _pu8Frame[2] == 0x00 && _pu8Frame[3] == 0x01,
              -1, "h264 frame is invalid, %02x%02x%02x%02x\n",
              (unsigned int)_pu8Frame[0], (unsigned int)_pu8Frame[1], (unsigned int)_pu8Frame[2], (unsigned int)_pu8Frame[3]);
    CHECK(_pu8Frame[4] == 0x67 || _pu8Frame[4] == 0x68 || _pu8Frame[4] == 0x06 || _pu8Frame[4] == 0x65 || _pu8Frame[4] == 0x61,
              -1, "nal %02x is invalid\n", (unsigned int)_pu8Frame[4]);

#define RTP_HEADER_SIZE (sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER))
#define RTP_HEADER_SIZE2 (sizeof(RTP_FIXED_HEADER) + sizeof(FU_INDICATOR) + sizeof(FU_HEADER))
    CHECK(RTP_HEADER_SIZE == 13, -1, "RTP_HEADER_SIZE %u\n", RTP_HEADER_SIZE);
    CHECK(RTP_HEADER_SIZE2 == 14, -1, "RTP_HEADER_SIZE2 %u\n", RTP_HEADER_SIZE2);

    unsigned char u8Nal = _pu8Frame[4];

    _pu8Frame += 5;
    _u32FrameSize -= 5;
    if(_u32FrameSize <= UDP_MAX_SIZE)
    {
        _pRetRtpSplit->u32SegmentCount = 1;
        _pRetRtpSplit->u32BufSize = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE + _u32FrameSize;
        _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
        _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
        _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));
    }
    else
    {
        int packetNum = _u32FrameSize/UDP_MAX_SIZE;
        if ((_u32FrameSize % UDP_MAX_SIZE) != 0)
        {
            packetNum ++;
        }

        int lastPackSize = _u32FrameSize - (packetNum-1) * UDP_MAX_SIZE;

        _pRetRtpSplit->u32SegmentCount = packetNum;
        _pRetRtpSplit->u32BufSize = (sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2 + UDP_MAX_SIZE) * (packetNum - 1)
                                    + (sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2 + lastPackSize);
        _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
        _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
        _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));
    }

    NALU_t stNalut;
    RTSP_INTERLEAVED_FRAME stRtspInterleavedFrame;
    RTP_FIXED_HEADER stRtpFixedHeader;
    NALU_HEADER stNaluHeader;
    FU_INDICATOR stFuIndicator;
    FU_HEADER stFuHeader;
    
    memset(&stNalut, 0, sizeof(stNalut));
    memset(&stRtspInterleavedFrame, 0, sizeof(RTSP_INTERLEAVED_FRAME));
    memset(&stRtpFixedHeader, 0, sizeof(RTP_FIXED_HEADER));
    memset(&stNaluHeader, 0, sizeof(NALU_HEADER));
    memset(&stFuIndicator, 0, sizeof(FU_INDICATOR));
    memset(&stFuHeader, 0, sizeof(FU_HEADER));

    NALU_t* n = &stNalut;
    RTSP_INTERLEAVED_FRAME* rtsp_intereaved = &stRtspInterleavedFrame;
    RTP_FIXED_HEADER* rtp_hdr = &stRtpFixedHeader;
    NALU_HEADER* nalu_hdr = &stNaluHeader;
    FU_INDICATOR* fu_ind = &stFuIndicator;
    FU_HEADER* fu_hdr = &stFuHeader;

    stNalut.buf = (char*)_pu8Frame;
    stNalut.len = _u32FrameSize;
    stNalut.forbidden_bit = u8Nal & 0x80;
    stNalut.nal_reference_idc = u8Nal & 0x60;
    stNalut.nal_unit_type = u8Nal & 0x1f;


    rtsp_intereaved->flag = '$';
    rtsp_intereaved->channel = 0;

    rtp_hdr->version = 2;
    rtp_hdr->marker  = 0;
    rtp_hdr->payload = rtp_payload_get("H.264");
    rtp_hdr->ssrc    = htonl(10);

    unsigned char* pu8BufCurr = _pRetRtpSplit->pu8Buf;
    if(n->len <= UDP_MAX_SIZE)
    {
        _pRetRtpSplit->ppu8Segment[0] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[0] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE + n->len;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE + n->len);

        rtp_hdr->marker=1;
        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->timestamp = htonl(_u32Timestamp);

        nalu_hdr->F = n->forbidden_bit;
        nalu_hdr->NRI = n->nal_reference_idc >> 5;
        nalu_hdr->TYPE = n->nal_unit_type;

        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, nalu_hdr, sizeof(NALU_HEADER));
        pu8BufCurr += sizeof(NALU_HEADER);
        memcpy(pu8BufCurr, n->buf, n->len);
        pu8BufCurr += n->len;
    }
    else
    {
        int packetNum = n->len/UDP_MAX_SIZE;
        if ((n->len % UDP_MAX_SIZE) != 0)
        {
            packetNum ++;
        }

        int lastPackSize = n->len - (packetNum-1) * UDP_MAX_SIZE;
        int packetIndex = 1 ;

        _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2 + UDP_MAX_SIZE;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE2 + UDP_MAX_SIZE);

        rtp_hdr->marker=0;
        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->timestamp = htonl(_u32Timestamp);

        fu_ind->F = n->forbidden_bit;
        fu_ind->NRI = n->nal_reference_idc>>5;
        fu_ind->TYPE = 28;

        fu_hdr->S = 1;
        fu_hdr->E = 0;
        fu_hdr->R = 0;
        fu_hdr->TYPE = n->nal_unit_type;

        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, fu_ind, sizeof(FU_INDICATOR));
        pu8BufCurr += sizeof(FU_INDICATOR);
        memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER));
        pu8BufCurr += sizeof(FU_HEADER);
        memcpy(pu8BufCurr, n->buf, UDP_MAX_SIZE);
        pu8BufCurr += UDP_MAX_SIZE;

        for(packetIndex=2; packetIndex<packetNum; packetIndex++)
        {
            _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
            _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2 + UDP_MAX_SIZE;

            rtsp_intereaved->size = htons(RTP_HEADER_SIZE2 + UDP_MAX_SIZE);

            rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
            rtp_hdr->marker=0;

            fu_ind->F = n->forbidden_bit;
            fu_ind->NRI = n->nal_reference_idc >> 5;
            fu_ind->TYPE = 28;

            fu_hdr->S = 0;
            fu_hdr->E = 0;
            fu_hdr->R = 0;
            fu_hdr->TYPE = n->nal_unit_type;

            memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
            pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
            memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
            pu8BufCurr += sizeof(RTP_FIXED_HEADER);
            memcpy(pu8BufCurr, fu_ind, sizeof(FU_INDICATOR));
            pu8BufCurr += sizeof(FU_INDICATOR);
            memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER));
            pu8BufCurr += sizeof(FU_HEADER);
            memcpy(pu8BufCurr, n->buf + (packetIndex - 1) * UDP_MAX_SIZE, UDP_MAX_SIZE);
            pu8BufCurr += UDP_MAX_SIZE;
        }

        _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2 + lastPackSize;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE2 + lastPackSize);

        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->marker=1;

        fu_ind->F = n->forbidden_bit;
        fu_ind->NRI = n->nal_reference_idc >> 5;
        fu_ind->TYPE = 28;

        fu_hdr->S = 0;
        fu_hdr->E = 1;
        fu_hdr->R = 0;
        fu_hdr->TYPE = n->nal_unit_type;

        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, fu_ind, sizeof(FU_INDICATOR));
        pu8BufCurr += sizeof(FU_INDICATOR);
        memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER));
        pu8BufCurr += sizeof(FU_HEADER);
        memcpy(pu8BufCurr, n->buf + (packetIndex - 1) * UDP_MAX_SIZE, lastPackSize);
        pu8BufCurr += lastPackSize;
    }

    unsigned int u32 = pu8BufCurr - _pRetRtpSplit->pu8Buf;
    CHECK(u32 == _pRetRtpSplit->u32BufSize, -1, "u32=%u, u32BufSize=%u\n", u32, _pRetRtpSplit->u32BufSize);

    return 0;
}

int rtp_h265_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                   unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                   RTP_SPLIT_S* _pRetRtpSplit)
{
    CHECK(_pu8Frame != NULL, -1, "invalid parameter with: %#x\n", _pu8Frame);
    CHECK(_u32FrameSize > 6, -1, "invalid parameter with: %#x\n", _u32FrameSize);
    CHECK(_pRetRtpSplit != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit);

    CHECK(_pu8Frame[0] == 0x00 
            && _pu8Frame[1] == 0x00 
            && _pu8Frame[2] == 0x00 
            && _pu8Frame[3] == 0x01,
        -1, "h265 frame is invalid, %02x%02x%02x%02x\n",_pu8Frame[0], _pu8Frame[1], _pu8Frame[2], _pu8Frame[3]);
    CHECK((_pu8Frame[4] == 0x40 
            || _pu8Frame[4] == 0x42 
            || _pu8Frame[4] == 0x44 
            || _pu8Frame[4] == 0x4e 
            || _pu8Frame[4] == 0x26
            || _pu8Frame[4] == 0x02)
            && _pu8Frame[5] == 0x01,
        -1, "nal %02x %02x is invalid\n", _pu8Frame[4], _pu8Frame[5]);

#define RTP_HEADER_SIZE_H265 (sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER_H265) /*+ sizeof(unsigned short)*/)
#define RTP_HEADER_SIZE2_H265 (sizeof(RTP_FIXED_HEADER) + sizeof(NALU_HEADER_H265) + sizeof(FU_HEADER)/* + sizeof(unsigned short)*/)
    CHECK(RTP_HEADER_SIZE_H265 == 16-2, -1, "RTP_HEADER_SIZE_H265 %u\n", RTP_HEADER_SIZE_H265);
    CHECK(RTP_HEADER_SIZE2_H265 == 17-2, -1, "RTP_HEADER_SIZE2_H265 %u\n", RTP_HEADER_SIZE2_H265);

    unsigned short u16NalHeader = 0;
    memcpy(&u16NalHeader, _pu8Frame+4, sizeof(u16NalHeader));
    
    _pu8Frame += 6;
    _u32FrameSize -= 6;
    if(_u32FrameSize <= UDP_MAX_SIZE)
    {
        _pRetRtpSplit->u32SegmentCount = 1;
        _pRetRtpSplit->u32BufSize = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE_H265 + _u32FrameSize;
        _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
        _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
        _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));
    }
    else
    {
        int packetNum = _u32FrameSize/UDP_MAX_SIZE;
        if ((_u32FrameSize % UDP_MAX_SIZE) != 0)
        {
            packetNum ++;
        }

        int lastPackSize = _u32FrameSize - (packetNum-1) * UDP_MAX_SIZE;

        _pRetRtpSplit->u32SegmentCount = packetNum;
        _pRetRtpSplit->u32BufSize = (sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2_H265 + UDP_MAX_SIZE) * (packetNum - 1)
            + (sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2_H265 + lastPackSize);
        _pRetRtpSplit->pu8Buf = (unsigned char*)mem_malloc(_pRetRtpSplit->u32BufSize);
        _pRetRtpSplit->ppu8Segment = (unsigned char**)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned char**));
        _pRetRtpSplit->pU32SegmentSize = (unsigned int*)mem_malloc(_pRetRtpSplit->u32SegmentCount * sizeof(unsigned int*));
    }

    NALU_t stNalut;
    RTSP_INTERLEAVED_FRAME stRtspInterleavedFrame;
    RTP_FIXED_HEADER stRtpFixedHeader;
    NALU_HEADER_H265 stNaluHeader;
    FU_HEADER_H265 stFuHeader;
    
    memset(&stNalut, 0, sizeof(stNalut));
    memset(&stRtspInterleavedFrame, 0, sizeof(RTSP_INTERLEAVED_FRAME));
    memset(&stRtpFixedHeader, 0, sizeof(RTP_FIXED_HEADER));
    memset(&stNaluHeader, 0, sizeof(NALU_HEADER));
    memset(&stFuHeader, 0, sizeof(FU_HEADER_H265));

    NALU_t* n = &stNalut;
    RTSP_INTERLEAVED_FRAME* rtsp_intereaved = &stRtspInterleavedFrame;
    RTP_FIXED_HEADER* rtp_hdr = &stRtpFixedHeader;
    NALU_HEADER_H265* nalu_hdr = &stNaluHeader;
    FU_HEADER_H265* fu_hdr = &stFuHeader;
    
    unsigned short u16Nal = htons(u16NalHeader);
    stNalut.buf = (char*)_pu8Frame;
    stNalut.len = _u32FrameSize;
    stNalut.forbidden_bit = (u16Nal & 0x8000) >> 15;
    stNalut.nal_layer_id = (u16Nal & 0x1f80) >> 3;
    stNalut.nal_tid = (u16Nal & 0x0007);
    stNalut.nal_unit_type = (u16Nal & 0x7e00) >> 9;
    //DBG("u16Nal: %#x, forbidden_bit: %d, nal_layer_id: %d, nal_tid: %d, nal_unit_type: %d\n", u16Nal, stNalut.forbidden_bit, stNalut.nal_layer_id, stNalut.nal_tid, stNalut.nal_unit_type);

    rtsp_intereaved->flag = '$';
    rtsp_intereaved->channel = 0;

    rtp_hdr->version = 2;
    rtp_hdr->marker  = 0;
    rtp_hdr->payload = rtp_payload_get("H.265");
    rtp_hdr->ssrc    = htonl(10);
    
    unsigned short DONL = 0;
    
    unsigned char* pu8BufCurr = _pRetRtpSplit->pu8Buf;
    if(n->len <= UDP_MAX_SIZE)
    {
        _pRetRtpSplit->ppu8Segment[0] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[0] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE_H265 + n->len;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE_H265 + n->len);

        rtp_hdr->marker=1;
        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->timestamp = htonl(_u32Timestamp);
        DONL = htons(DONL);
        
        memcpy(nalu_hdr, &u16NalHeader, sizeof(u16NalHeader));

        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, nalu_hdr, sizeof(NALU_HEADER_H265));
        pu8BufCurr += sizeof(NALU_HEADER_H265);
        //memcpy(pu8BufCurr, &DONL, sizeof(DONL));
        //pu8BufCurr += sizeof(DONL);
        memcpy(pu8BufCurr, n->buf, n->len);
        pu8BufCurr += n->len;
    }
    else
    {
        int packetNum = n->len/UDP_MAX_SIZE;
        if ((n->len % UDP_MAX_SIZE) != 0)
        {
            packetNum ++;
        }
        int lastPackSize = n->len - (packetNum-1) * UDP_MAX_SIZE;
        int packetIndex = 1 ;
        //DBG("packetNum: %d, lastPackSize: %d\n", packetNum, lastPackSize);

        _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2_H265 + UDP_MAX_SIZE;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE2_H265 + UDP_MAX_SIZE);

        rtp_hdr->marker=0;
        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->timestamp = htonl(_u32Timestamp);

        fu_hdr->S = 1;
        fu_hdr->E = 0;
        fu_hdr->TYPE = n->nal_unit_type;
        
        unsigned short u16NalFu = htons(u16NalHeader);
        u16NalFu &= ~0x7e00;
        u16NalFu |= (49 << 9);//fu type
        u16NalFu = htons(u16NalFu);
        memcpy(nalu_hdr, &u16NalFu, sizeof(u16NalFu));
        //DBG("fu nalu_hdr: %#x\n", u16NalFu);
        
        DONL = htons(DONL);
        
        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, nalu_hdr, sizeof(NALU_HEADER_H265));
        pu8BufCurr += sizeof(NALU_HEADER_H265);
        memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER_H265));
        pu8BufCurr += sizeof(FU_HEADER_H265);
        //memcpy(pu8BufCurr, &DONL, sizeof(DONL));
        //pu8BufCurr += sizeof(DONL);
        memcpy(pu8BufCurr, n->buf, UDP_MAX_SIZE);
        pu8BufCurr += UDP_MAX_SIZE;

        for(packetIndex=2; packetIndex<packetNum; packetIndex++)
        {
            _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
            _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2_H265 + UDP_MAX_SIZE;

            rtsp_intereaved->size = htons(RTP_HEADER_SIZE2_H265 + UDP_MAX_SIZE);

            rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
            rtp_hdr->marker=0;

            fu_hdr->S = 0;
            fu_hdr->E = 0;
            fu_hdr->TYPE = n->nal_unit_type;

            memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
            pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
            memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
            pu8BufCurr += sizeof(RTP_FIXED_HEADER);
            memcpy(pu8BufCurr, nalu_hdr, sizeof(NALU_HEADER_H265));
            pu8BufCurr += sizeof(NALU_HEADER_H265);
            memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER_H265));
            pu8BufCurr += sizeof(FU_HEADER_H265);
            //memcpy(pu8BufCurr, &DONL, sizeof(DONL));
            //pu8BufCurr += sizeof(DONL);
            memcpy(pu8BufCurr, n->buf + (packetIndex - 1) * UDP_MAX_SIZE, UDP_MAX_SIZE);
            pu8BufCurr += UDP_MAX_SIZE;
        }

        _pRetRtpSplit->ppu8Segment[packetIndex - 1] = pu8BufCurr;
        _pRetRtpSplit->pU32SegmentSize[packetIndex - 1] = sizeof(RTSP_INTERLEAVED_FRAME) + RTP_HEADER_SIZE2_H265 + lastPackSize;

        rtsp_intereaved->size = htons(RTP_HEADER_SIZE2_H265 + lastPackSize);

        rtp_hdr->seq_no = htons((*_pu16SeqNumber)++);
        rtp_hdr->marker=1;

        fu_hdr->S = 0;
        fu_hdr->E = 1;
        fu_hdr->TYPE = n->nal_unit_type;

        memcpy(pu8BufCurr, rtsp_intereaved, sizeof(RTSP_INTERLEAVED_FRAME));
        pu8BufCurr += sizeof(RTSP_INTERLEAVED_FRAME);
        memcpy(pu8BufCurr, rtp_hdr, sizeof(RTP_FIXED_HEADER));
        pu8BufCurr += sizeof(RTP_FIXED_HEADER);
        memcpy(pu8BufCurr, nalu_hdr, sizeof(NALU_HEADER_H265));
        pu8BufCurr += sizeof(NALU_HEADER_H265);
        memcpy(pu8BufCurr, fu_hdr, sizeof(FU_HEADER_H265));
        pu8BufCurr += sizeof(FU_HEADER_H265);
        //memcpy(pu8BufCurr, &DONL, sizeof(DONL));
        //pu8BufCurr += sizeof(DONL);
        memcpy(pu8BufCurr, n->buf + (packetIndex - 1) * UDP_MAX_SIZE, lastPackSize);
        pu8BufCurr += lastPackSize;
    }

    unsigned int u32 = pu8BufCurr - _pRetRtpSplit->pu8Buf;
    CHECK(u32 == _pRetRtpSplit->u32BufSize, -1, "u32=%u, u32BufSize=%u\n", u32, _pRetRtpSplit->u32BufSize);

    return 0;
}

int rtp_free(RTP_SPLIT_S* _pRetRtpSplit)
{
    CHECK(_pRetRtpSplit != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit);
    CHECK(_pRetRtpSplit->pu8Buf != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit->pu8Buf);
    CHECK(_pRetRtpSplit->ppu8Segment != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit->ppu8Segment);
    CHECK(_pRetRtpSplit->pU32SegmentSize != NULL, -1, "invalid parameter with: %#x\n", _pRetRtpSplit->pU32SegmentSize);

    mem_free(_pRetRtpSplit->pu8Buf);
    mem_free(_pRetRtpSplit->ppu8Segment);
    mem_free(_pRetRtpSplit->pU32SegmentSize);

    return 0;
}

int rtp_payload_get(const char* encode_type)
{
    CHECK(encode_type, -1, "invalid parameter with: %#x\n", encode_type);

    int payload = 96;
    if (strstr(encode_type, "H.264")||strstr(encode_type, "H.265"))
    {
        payload = 96;
    }
    else if (strstr(encode_type, "AMR"))
    {
        payload = 97;
    }
    else if (strstr(encode_type, "G.711A"))
    {
        payload = 8;
    }
    else if (strstr(encode_type, "G.711U"))
    {
        payload = 0;
    }
    else
    {
        ERR("Unknown encode_type: %s\n", encode_type);
    }

    return payload;
}

