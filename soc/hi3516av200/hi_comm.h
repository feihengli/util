#ifndef __SAMPLE_COMM_H__
#define __SAMPLE_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
//#include "hi_comm_vdec.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_defines.h"
#include "hi_mipi.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
//#include "mpi_vdec.h"
#include "mpi_region.h"
#include "mpi_adec.h"
#include "mpi_aenc.h"
#include "mpi_ai.h"
#include "mpi_ao.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"
#include "hi_vreg.h"
#include "hi_sns_ctrl.h"

#include "hi_comm_video.h"
#include "hi_comm_ive.h"
#include "hi_comm_vgs.h"
#include "mpi_ive.h"
#include "mpi_vgs.h"
#include "ivs_md.h"
#include "mpi_ive.h"
#include "acodec.h"


typedef enum sample_vi_mode_e
{
    APTINA_AR0130_DC_720P_30FPS = 0,
    APTINA_9M034_DC_720P_30FPS,
    APTINA_AR0230_HISPI_1080P_30FPS,
    SONY_IMX122_DC_1080P_30FPS,
    SONY_IMX122_DC_720P_30FPS,
    SONY_IMX291_LVDS_1080P_30FPS,
    SONY_IMX323_DC_1080P_30FPS,
    SONY_IMX290_LVDS_1080P_30FPS,
    SONY_IMX226_LVDS_12M_30FPS,
    PANASONIC_MN34222_MIPI_1080P_30FPS,
    OMNIVISION_OV9712_DC_720P_30FPS,
    OMNIVISION_OV9732_DC_720P_30FPS,
    OMNIVISION_OV9750_MIPI_720P_30FPS,
    OMNIVISION_OV9752_MIPI_720P_30FPS,
    OMNIVISION_OV9732_MIPI_720P_30FPS,
    OMNIVISION_OV2710_MIPI_1080P_30FPS,
    SMARTSENS_SC1135_DC_960P_30FPS,
    SMARTSENS_SC2135_DC_1080P_30FPS,
    APTINA_AR0237_DC_1080P_30FPS,
    SAMPLE_VI_MODE_1_D1,
    SAMPLE_VI_MODE_BT1120_720P,
    SAMPLE_VI_MODE_BT1120_1080P,
}SAMPLE_VI_MODE_E;


/*******************************************************
    structure define
*******************************************************/
typedef struct sample_vi_param_s
{
    HI_S32 s32ViDevCnt;         // VI Dev Total Count
    HI_S32 s32ViDevInterval;    // Vi Dev Interval
    HI_S32 s32ViChnCnt;         // Vi Chn Total Count
    HI_S32 s32ViChnInterval;    // VI Chn Interval
}SAMPLE_VI_PARAM_S;


typedef struct sample_vi_config_s
{
    SAMPLE_VI_MODE_E enViMode;
    VIDEO_NORM_E enNorm;           /*DC: VIDEO_ENCODING_MODE_AUTO */
    ROTATE_E enRotate;
    WDR_MODE_E  enWDRMode;
}SAMPLE_VI_CONFIG_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif