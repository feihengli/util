#ifndef __MA_RTP_H__
#define __MA_RTP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "sal_standard.h"

typedef struct RTSP_INTERLEAVED_FRAME
{
    unsigned char flag;
    unsigned char channel;
    unsigned short size;
} RTSP_INTERLEAVED_FRAME;

/******************************************************************
RTP_FIXED_HEADER
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

******************************************************************/

typedef struct RTP_FIXED_HEADER
{
    /* byte 0 */
    unsigned char csrc_len:4; /* CC expect 0 */
    unsigned char extension:1;/* X  expect 1, see RTP_OP below */
    unsigned char padding:1;  /* P  expect 0 */
    unsigned char version:2;  /* V  expect 2 */
    /* byte 1 */
    unsigned char payload:7; /* PT  RTP_PAYLOAD_RTSP */
    unsigned char marker:1;  /* M   expect 1 */
    /* byte 2,3 */
    unsigned short seq_no;   /*sequence number*/
    /* byte 4-7 */
    unsigned  long timestamp;
    /* byte 8-11 */
    unsigned long ssrc; /* stream number is used here. */
} RTP_FIXED_HEADER;/*12 bytes*/

//only for h265
enum NalUnitType  
{  
    NAL_UNIT_CODED_SLICE_TRAIL_N = 0,   // 0  
    NAL_UNIT_CODED_SLICE_TRAIL_R,   // 1  

    NAL_UNIT_CODED_SLICE_TSA_N,     // 2  
    NAL_UNIT_CODED_SLICE_TLA,       // 3   // Current name in the spec: TSA_R  

    NAL_UNIT_CODED_SLICE_STSA_N,    // 4  
    NAL_UNIT_CODED_SLICE_STSA_R,    // 5  

    NAL_UNIT_CODED_SLICE_RADL_N,    // 6  
    NAL_UNIT_CODED_SLICE_DLP,       // 7 // Current name in the spec: RADL_R  

    NAL_UNIT_CODED_SLICE_RASL_N,    // 8  
    NAL_UNIT_CODED_SLICE_TFD,       // 9 // Current name in the spec: RASL_R  

    NAL_UNIT_RESERVED_10,  
    NAL_UNIT_RESERVED_11,  
    NAL_UNIT_RESERVED_12,  
    NAL_UNIT_RESERVED_13,  
    NAL_UNIT_RESERVED_14,  
    NAL_UNIT_RESERVED_15,
    NAL_UNIT_CODED_SLICE_BLA,       // 16   // Current name in the spec: BLA_W_LP  
    NAL_UNIT_CODED_SLICE_BLANT,     // 17   // Current name in the spec: BLA_W_DLP  
    NAL_UNIT_CODED_SLICE_BLA_N_LP,  // 18  
    NAL_UNIT_CODED_SLICE_IDR,       // 19  // Current name in the spec: IDR_W_DLP  
    NAL_UNIT_CODED_SLICE_IDR_N_LP,  // 20  
    NAL_UNIT_CODED_SLICE_CRA,       // 21  
    NAL_UNIT_RESERVED_22,  
    NAL_UNIT_RESERVED_23,  

    NAL_UNIT_RESERVED_24,  
    NAL_UNIT_RESERVED_25,  
    NAL_UNIT_RESERVED_26,  
    NAL_UNIT_RESERVED_27,  
    NAL_UNIT_RESERVED_28,  
    NAL_UNIT_RESERVED_29,  
    NAL_UNIT_RESERVED_30,  
    NAL_UNIT_RESERVED_31,  

    NAL_UNIT_VPS,                   // 32  
    NAL_UNIT_SPS,                   // 33  
    NAL_UNIT_PPS,                   // 34  
    NAL_UNIT_ACCESS_UNIT_DELIMITER, // 35  
    NAL_UNIT_EOS,                   // 36  
    NAL_UNIT_EOB,                   // 37  
    NAL_UNIT_FILLER_DATA,           // 38  
    NAL_UNIT_SEI,                   // 39 Prefix SEI  
    NAL_UNIT_SEI_SUFFIX,            // 40 Suffix SEI  
    NAL_UNIT_RESERVED_41,  
    NAL_UNIT_RESERVED_42,  
    NAL_UNIT_RESERVED_43,  
    NAL_UNIT_RESERVED_44,  
    NAL_UNIT_RESERVED_45,  
    NAL_UNIT_RESERVED_46,  
    NAL_UNIT_RESERVED_47,  
    NAL_UNIT_UNSPECIFIED_48,  
    NAL_UNIT_UNSPECIFIED_49,  
    NAL_UNIT_UNSPECIFIED_50,  
    NAL_UNIT_UNSPECIFIED_51,  
    NAL_UNIT_UNSPECIFIED_52,  
    NAL_UNIT_UNSPECIFIED_53,  
    NAL_UNIT_UNSPECIFIED_54,  
    NAL_UNIT_UNSPECIFIED_55,  
    NAL_UNIT_UNSPECIFIED_56,  
    NAL_UNIT_UNSPECIFIED_57,  
    NAL_UNIT_UNSPECIFIED_58,  
    NAL_UNIT_UNSPECIFIED_59,  
    NAL_UNIT_UNSPECIFIED_60,  
    NAL_UNIT_UNSPECIFIED_61,  
    NAL_UNIT_UNSPECIFIED_62,  
    NAL_UNIT_UNSPECIFIED_63,  
    NAL_UNIT_INVALID,  
}; 

/******************************************************************
NALU_HEADER
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
******************************************************************/
typedef struct NALU_HEADER
{
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2;
    unsigned char F:1;
} NALU_HEADER; /* 1 byte */

/******************************************************************
NALU_HEADER_H265
+---------------+---------------+
|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|F|    Type   |  LayerID  | Tid |
+---------------+---------------+
******************************************************************/
typedef struct NALU_HEADER_H265
{
    unsigned char buffer[2];
} NALU_HEADER_H265; /* 2 byte */

/******************************************************************
FU_INDICATOR
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
******************************************************************/
typedef struct FU_INDICATOR
{
    //byte 0
    unsigned char TYPE:5;
    unsigned char NRI:2;
    unsigned char F:1;
} FU_INDICATOR; /*1 byte */


/******************************************************************
FU_HEADER
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|R|  Type   |
+---------------+
******************************************************************/
typedef struct FU_HEADER
{
    //byte 0
    unsigned char TYPE:5;
    unsigned char R:1;
    unsigned char E:1;
    unsigned char S:1;
} FU_HEADER; /* 1 byte */

/******************************************************************
FU_HEADER_H265
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|    Type   |
+---------------+
******************************************************************/
typedef struct FU_HEADER_H265
{
    //byte 0
    unsigned char TYPE:6;
    unsigned char E:1;
    unsigned char S:1;
} FU_HEADER_H265; /* 1 byte */

typedef struct NALU_t
{
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;            //! Nal Unit Buffer size
    int forbidden_bit;            //! should be always FALSE
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx  only for h264
    int nal_layer_id;             //! NALU_PRIORITY_xxxx  only for h265
    int nal_tid;                   //! NALU_PRIORITY_xxxx only for h265
    int nal_unit_type;            //! NALU_TYPE_xxxx
    char *buf;                    //! contains the first byte followed by the EBSP
    unsigned short lost_packets;  //! true, if packet loss is detected
} NALU_t;

//在udp传输模式、大分辨下I帧较大，RTP封包需较大vlc才能接收出图
#define UDP_MAX_SIZE (4*1024) //(1399) //(8*1024)

typedef struct RTP_SPLIT_S
{
    unsigned char* pu8Buf;        //存放所有RTP包的buffer
    unsigned int u32BufSize;      //buffer的大小
    unsigned int u32SegmentCount; //RTP包的个数
    unsigned char** ppu8Segment;  //每一个RTP包的地址offset，来源是pu8Buf
    unsigned int* pU32SegmentSize;//每一个RTP包的大小
} RTP_SPLIT_S;

typedef struct RTP_KEY_S
{
    int sps_offset;
    int sps_len;
    int pps_offset;
    int pps_len;
    int sei_offset;
    int sei_len;
    int idr_offset;
    int idr_len;
} RTP_KEY_S;

/*
 函 数 名: rtp_amr_alloc
 功能描述: RTP封装AMR的音频帧 每个RTP包都包含RTP OVER TCP的交错帧头
 输入参数: _pu8Frame 音频帧
            _u32FrameSize 帧大小
            _pu16SeqNumber RTP包的序号
            _u32Timestamp RTP包的时间戳
            _Ssrc RTP包的ssrc
 输出参数: _pRetRtpSplit 保存RTP封包的结果，需调用rtp_free来释放
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_amr_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                      unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                      unsigned long _Ssrc, RTP_SPLIT_S* _pRetRtpSplit);

/*
 函 数 名: rtp_g711_alloc
 功能描述: RTP封装G711的音频帧 每个RTP包都包含RTP OVER TCP的交错帧头
 输入参数: _pu8Frame 音频帧
            _u32FrameSize 帧大小
            _pu16SeqNumber RTP包的序号
            _u32Timestamp RTP包的时间戳
            _Ssrc RTP包的ssrc
 输出参数: _pRetRtpSplit 保存RTP封包的结果，需调用rtp_free来释放
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_g711a_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                      unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                      unsigned long _Ssrc, RTP_SPLIT_S* _pRetRtpSplit);

/*
 函 数 名: rtp_vframe_split
 功能描述: h264/h265裸流分割nalu，获取每个nalu的分界点
 输入参数: _pu8Frame 视频帧
            _u32FrameSize 帧大小
 输出参数: 无
 返 回 值: 成功返回第二个nalu的分界点,失败返回小于0
*/
int rtp_vframe_split(unsigned char* _pu8Frame, unsigned int _u32FrameSize);

/*
 函 数 名: rtp_h264_alloc
 功能描述: RTP封装h264的视频帧，每个RTP包都包含RTP OVER TCP的交错帧头
 输入参数: _pu8Frame 视频帧
            _u32FrameSize 帧大小
            _pu16SeqNumber RTP包的序号
            _u32Timestamp RTP包的时间戳
 输出参数: _pRetRtpSplit 保存RTP封包的结果
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_h264_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                       unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                       RTP_SPLIT_S* _pRetRtpSplit);

/*
 函 数 名: rtp_h265_alloc
 功能描述: RTP封装h265的视频帧，每个RTP包都包含RTP OVER TCP的交错帧头
 输入参数: _pu8Frame 视频帧
            _u32FrameSize 帧大小
            _pu16SeqNumber RTP包的序号
            _u32Timestamp RTP包的时间戳
 输出参数: _pRetRtpSplit 保存RTP封包的结果
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_h265_alloc(unsigned char* _pu8Frame, unsigned int _u32FrameSize,
                   unsigned short* _pu16SeqNumber, unsigned int _u32Timestamp,
                   RTP_SPLIT_S* _pRetRtpSplit);

/*
 函 数 名: rtp_free
 功能描述: 释放RTP包封装的资源
 输入参数: _pRetRtpSplit 保存RTP封包的结果
 输出参数:
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_free(RTP_SPLIT_S* _pRetRtpSplit);

/*
 函 数 名: rtp_payload_get
 功能描述: 获取RTP包负荷类型
 输入参数: encode_type 编码格式
 输出参数:
 返 回 值: 成功返回0,失败返回小于0
*/
int rtp_payload_get(const char* encode_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif


