#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "hi_comm.h"
#include "libixml/ixml.h"
#include "sal_isp.h"
#include "sal_av.h"
#include "sal_debug.h"

//#define ISP_AUTO_STENGTH_NUM 16

// if exist, then use here xml
#define REAL_ISP_CONFIG_PATH "/etc/ov2710.xml"

typedef enum GAMMA_LUMA_STATE
{
    GAMMA_LUMA_STATE_LOW = 0,
    GAMMA_LUMA_STATE_MIDDLE,
    GAMMA_LUMA_STATE_HIGH,
    GAMMA_LUMA_STATE_BUTT
}GAMMA_LUMA_STATE;

typedef enum ISO_LEVEL
{
    ISO_LEVEL_0 = 0,
    ISO_LEVEL_1,
    ISO_LEVEL_2,
    ISO_LEVEL_3,
    ISO_LEVEL_4,
    ISO_LEVEL_5,
    ISO_LEVEL_6,
    ISO_LEVEL_7,
    ISO_LEVEL_8,
    ISO_LEVEL_9,
    ISO_LEVEL_10,
    ISO_LEVEL_11,
    ISO_LEVEL_12,
    ISO_LEVEL_13,
    ISO_LEVEL_14,
    ISO_LEVEL_15,
    ISO_LEVEL_BUTT
}ISO_LEVEL;

typedef struct _ISP_CONFIG
{
    SAMPLE_VI_MODE_E sensor_type;
    ISP_EXPOSURE_ATTR_S stExpAttr;
    ISP_AE_ROUTE_S stAERoute;
    ISP_BLACK_LEVEL_S stBlackLevel[ISP_AUTO_STENGTH_NUM];
    ISP_WB_ATTR_S stWBAttr;
    ISP_AWB_ATTR_EX_S stAWBAttrEx;
    ISP_COLORMATRIX_ATTR_S stCCMAttr;
    ISP_SATURATION_ATTR_S stSaturation;
    //ISP_SHARPEN_ATTR_S stSharpenAttr;
    //VPSS_NR_PARAM_U astNR[ISP_AUTO_STENGTH_NUM];
    ISP_NR_ATTR_S stNRAttr;
    ISP_DEMOSAIC_ATTR_S stDemosaicAttr;
    ISP_ANTI_FALSECOLOR_S stAntiFC;
    ISP_GAMMA_ATTR_S stGammaAttr[GAMMA_LUMA_STATE_BUTT];
    HI_U16 GammaSysgainThreshold[GAMMA_LUMA_STATE_BUTT];
    ISP_DEFOG_ATTR_S stDefogAttr[ISP_AUTO_STENGTH_NUM];
    ISP_DRC_ATTR_S astDrcAttr[ISP_AUTO_STENGTH_NUM];
    VI_DCI_PARAM_S stDci;
    ISP_DP_DYNAMIC_ATTR_S stDPDynamic;
    //ISP_UVNR_ATTR_S stUvnrAttr;
    ISP_CR_ATTR_S stCRAttr;

    ISO_LEVEL last_iso_level;
    GAMMA_LUMA_STATE last_gamma_state;
    int last_iso;

    //real time info
    ISP_EXP_INFO_S stExpInfo;

    //last isp day night mode
    SAL_ISP_MODE_E last_mode;
    sal_image_attr_s image_attr;

    ISP_EXPOSURE_ATTR_S stSLExpAttr; //starlight config

    //drop frame cfg
    int bSlowShutter;
    int frame_level[5];
    int last_fps;
    int max_fps[2]; // current max fps
    int max_fps_cfg[2];  // config max fps
    struct timeval last_level_time;
    int delay_time; // 稳定时间(毫秒)

/*
    //降到6帧的时候需要降低码率
    int vbr[2];
    int bitrate[2];
*/

/*
    //advance_param
    int bUpdate;
    sal_isp_advance_s advance_default_param;
    sal_isp_advance_s advance_param;
    int last_drc_time_mode;// 1: 在设置的时间段中 0: 不在设置的时间段中
    struct timeval last_drc_time;
    int ldc_support;
*/



/*
    //SC2135热燥逻辑
    int isp_gain_max;
    struct timeval last_hot_noise_time;
*/

    //run args
    pthread_mutex_t mutex;
    pthread_t pid;
    int running;
}
ISP_CONFIG;

static ISP_CONFIG* gp_isp_config = NULL;

static int array_num(char* pStr, int* pArray,int n)
{
    char *pToken;
    int i = 0;

    if(pStr == NULL || pArray == NULL)
    {
        DBG("pStr or pArray is NULL\n");
        return HI_FAILURE;
    }
    pToken = strtok(pStr, ",");
    while(pToken != NULL && i < n)
    {
        pArray[i] = strtod(pToken, NULL);
        pToken = strtok(NULL, ",");
        i++;
    }

    return i;
}

static int str2array(char* pStr, int* pArray,int n)
{
    char *pToken;
    int i = 0;

    if(pStr == NULL || pArray == NULL)
    {
        printf("pStr or pArray is NULL\n");
        return HI_FAILURE;
    }
    if(n <= 0)
    {
        printf("invalid n:%d\n", n);
        return HI_FAILURE;
    }
    pToken = strtok(pStr, ",");
    while(pToken != NULL && i < n)
    {
        pArray[i] = strtod(pToken, NULL);
        pToken = strtok(NULL, ",");
        i++;
    }
    //兼容老版本图像参数会被赋不合法值0的情况
    if ((n == ISP_AUTO_STENGTH_NUM) && pArray[0] && (pArray[1] == 0))
    {
        for(i = 1; i < n; i++)
        {
            pArray[i] = pArray[0];
        }
    }

    return HI_SUCCESS;
}

static int parse_exposure_attr_config(IXML_Node* pNode, ISP_EXPOSURE_ATTR_S* pExpAttr)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    ISP_AE_ATTR_S* pAe = &pExpAttr->stAuto;
    int tmp[2] = {0, 0};
    int ret = -1;
    ISP_EXPOSURE_ATTR_S stSLExpAttrTmp;

    ret = HI_MPI_ISP_GetExposureAttr(0, pExpAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    memcpy(&stSLExpAttrTmp, pExpAttr, sizeof(stSLExpAttrTmp));

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "HistStatAdjust"))
        {
            tmpAttr = tmpChild->firstAttr;
            pExpAttr->bHistStatAdjust = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "AERouteExValid"))
        {
            tmpAttr = tmpChild->firstAttr;
            pExpAttr->bAERouteExValid = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "ExpTimeRange"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stExpTimeRange.u32Max = tmp[0];
            pAe->stExpTimeRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "AGainRange"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stAGainRange.u32Max = tmp[0];
            pAe->stAGainRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "DGainRange"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stDGainRange.u32Max = tmp[0];
            pAe->stDGainRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "ISPDGainRange"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stISPDGainRange.u32Max = tmp[0];
            pAe->stISPDGainRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "SysGainRange"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stSysGainRange.u32Max = tmp[0];
            pAe->stSysGainRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "GainThreshold"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u32GainThreshold = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Speed"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u8Speed = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Tolerance"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u8Tolerance = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Compensation"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u8Compensation = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "EVBias"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u16EVBias = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "AEStrategyMode"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->enAEStrategyMode = (ISP_AE_STRATEGY_E)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "HistRatioSlope"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u16HistRatioSlope = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "MaxHistOffset"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->u8MaxHistOffset = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "AEMode"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAe->enAEMode = (ISP_AE_MODE_E)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Antiflicker"))
        {
            tmpAttr = tmpChild->firstAttr;
            int Antiflicker[3] = {0, 0, 0};
            ret = str2array(tmpAttr->nodeValue, Antiflicker, sizeof(Antiflicker)/sizeof(*Antiflicker));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stAntiflicker.bEnable = (HI_BOOL)Antiflicker[0];
            pAe->stAntiflicker.u8Frequency = Antiflicker[1];
            pAe->stAntiflicker.enMode = (ISP_ANTIFLICKER_MODE_E)Antiflicker[2];
        }
        else if(!strcmp(tmpChild->nodeName, "Subflicker"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stSubflicker.bEnable = (HI_BOOL)tmp[0];
            pAe->stSubflicker.u8LumaDiff = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "AEDelayAttr"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            pAe->stAEDelayAttr.u16BlackDelayFrame = tmp[0];
            pAe->stAEDelayAttr.u16WhiteDelayFrame = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "Weight"))
        {
            tmpAttr = tmpChild->firstAttr;
            int weight[AE_ZONE_ROW*AE_ZONE_COLUMN];
            memset(weight, 0, sizeof(weight));

            ret = str2array(tmpAttr->nodeValue, weight, sizeof(weight)/sizeof(*weight));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            int i = 0;
            int j = 0;
            for (i = 0; i < AE_ZONE_ROW; i++)
            {
                for (j = 0; j < AE_ZONE_COLUMN; j++)
                {
                    pAe->au8Weight[i][j] = weight[i*AE_ZONE_COLUMN+j];
                    //printf("%d,", pAe->au8Weight[i][j]);
                }
                //printf("\n");
            }
        }
        else if(!strcmp(tmpChild->nodeName, "ExpTimeRangeStarLight"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            stSLExpAttrTmp.stAuto.stExpTimeRange.u32Max = tmp[0];
            stSLExpAttrTmp.stAuto.stExpTimeRange.u32Min = tmp[1];
        }
        else if(!strcmp(tmpChild->nodeName, "GainThresholdStarLight"))
        {
            tmpAttr = tmpChild->firstAttr;
            stSLExpAttrTmp.stAuto.u32GainThreshold = strtold(tmpAttr->nodeValue, NULL);
        }
        else
        {
            WRN("exposure_attr_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    memcpy(&gp_isp_config->stSLExpAttr, pExpAttr, sizeof(gp_isp_config->stSLExpAttr));
    memcpy(&gp_isp_config->stSLExpAttr.stAuto.stExpTimeRange, &stSLExpAttrTmp.stAuto.stExpTimeRange, sizeof(stSLExpAttrTmp.stAuto.stExpTimeRange));
    gp_isp_config->stSLExpAttr.stAuto.u32GainThreshold = stSLExpAttrTmp.stAuto.u32GainThreshold;

    return HI_SUCCESS;
}

static int parse_ae_route_config(IXML_Node* pNode, ISP_AE_ROUTE_S* pAERoute)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[3*ISP_AE_ROUTE_MAX_NODES];
    memset(temp, 0, sizeof(temp));
    HI_S32 i = 0;

    s32Ret = HI_MPI_ISP_GetAERouteAttr(IspDev, pAERoute);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "TotalNum"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAERoute->u32TotalNum = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "RouteNode"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AE_ROUTE_MAX_NODES; i++)
            {
                pAERoute->astRouteNode[i].u32IntTime = temp[i*3];
                pAERoute->astRouteNode[i].u32SysGain = temp[i*3 + 1];
                pAERoute->astRouteNode[i].enIrisFNO = (ISP_IRIS_F_NO_E)temp[i*3 + 2];
            }
        }
        else
        {
            WRN("ae_route_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int parse_wb_attr_config(IXML_Node* pNode, ISP_BLACK_LEVEL_S* pBlackLevel, ISP_WB_ATTR_S* pWBAttr, ISP_AWB_ATTR_EX_S* pAWBAttrEx)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
    memset(temp, 0, sizeof(temp));
    HI_U32 i = 0, j = 0;

    for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
    {
        s32Ret = HI_MPI_ISP_GetBlackLevelAttr(IspDev, &pBlackLevel[i]);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    }
    s32Ret = HI_MPI_ISP_GetWBAttr(IspDev, pWBAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    s32Ret = HI_MPI_ISP_GetAWBAttrEx(IspDev, pAWBAttrEx);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        memset(temp, 0, sizeof(temp));
        if(!strcmp(tmpChild->nodeName, "BlackLevel"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                        pBlackLevel[i].au16BlackLevel[j] = temp[j];
                }
            }
        }
        else if(!strcmp(tmpChild->nodeName, "BlackLevel_0"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pBlackLevel[i].au16BlackLevel[0] = temp[i];
            }
        }
        else if (!strcmp(tmpChild->nodeName, "BlackLevel_1"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pBlackLevel[i].au16BlackLevel[1] = temp[i];
            }
        }
        else if (!strcmp(tmpChild->nodeName, "BlackLevel_2"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pBlackLevel[i].au16BlackLevel[2] = temp[i];
            }
        }
        else if (!strcmp(tmpChild->nodeName, "BlackLevel_3"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pBlackLevel[i].au16BlackLevel[3] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "StaticWB"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < sizeof(pWBAttr->stAuto.au16StaticWB)/sizeof(HI_U16); i++)
            {
                pWBAttr->stAuto.au16StaticWB[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "CurvePara"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < sizeof(pWBAttr->stAuto.as32CurvePara)/sizeof(HI_S32); i++)
            {
                pWBAttr->stAuto.as32CurvePara[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "HighColorTemp"))
        {
            tmpAttr = tmpChild->firstAttr;
            pWBAttr->stAuto.u16HighColorTemp = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "LowColorTemp"))
        {
            tmpAttr = tmpChild->firstAttr;
            pWBAttr->stAuto.u16LowColorTemp = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "ShiftLimit"))
        {
            tmpAttr = tmpChild->firstAttr;
            pWBAttr->stAuto.u8ShiftLimit = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "InOrOutOutThresh"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAWBAttrEx->stInOrOut.u32OutThresh = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "MultiLightSourceEn"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAWBAttrEx->bMultiLightSourceEn = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
/*
        else if(!strcmp(tmpChild->nodeName, "MultiLSType"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAWBAttrEx->enMultiLSType = (ISP_AWB_MULTI_LS_TYPE_E)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "MultiLSScaler"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAWBAttrEx->u16MultiLSScaler = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "MultiCTBin"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("str2array failed\n");
                break;
            }
            for(i = 0; i < sizeof(pAWBAttrEx->au16MultiCTBin)/sizeof(HI_U16); i++)
            {
                pAWBAttrEx->au16MultiCTBin[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "MultiCTWt"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("str2array failed\n");
                break;
            }
            for(i = 0; i < sizeof(pAWBAttrEx->au16MultiCTWt)/sizeof(HI_U16); i++)
            {
                pAWBAttrEx->au16MultiCTWt[i] = temp[i];
            }
        }
*/
        else if(!strcmp(tmpChild->nodeName, "AWBAlgType"))
        {
            tmpAttr = tmpChild->firstAttr;
            pWBAttr->stAuto.enAlgType = (ISP_AWB_ALG_TYPE_E)strtold(tmpAttr->nodeValue, NULL);
        }
        else
        {
            WRN("wb_attr_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int parse_ccm_config(IXML_Node* pNode, ISP_COLORMATRIX_ATTR_S* pCCMAttr)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[9];
    memset(temp, 0, sizeof(temp));
    HI_S32 i = 0;

    s32Ret = HI_MPI_ISP_GetCCMAttr(IspDev, pCCMAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "HighColorTemp"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCCMAttr->stAuto.u16HighColorTemp = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "HighCCM"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < 9; i++)
            {
                pCCMAttr->stAuto.au16HighCCM[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "MidColorTemp"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCCMAttr->stAuto.u16MidColorTemp = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "MidCCM"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < 9; i++)
            {
                pCCMAttr->stAuto.au16MidCCM[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "LowColorTemp"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCCMAttr->stAuto.u16LowColorTemp = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "LowCCM"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < 9; i++)
            {
                pCCMAttr->stAuto.au16LowCCM[i] = temp[i];
            }
        }
        else if (!strcmp(tmpChild->nodeName, "AutoISOActEn"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCCMAttr->stAuto.bISOActEn = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
        else if (!strcmp(tmpChild->nodeName, "AutoTempActEn"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCCMAttr->stAuto.bTempActEn = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
        else
        {
            WRN("ccm_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int parse_saturation_config(IXML_Node* pNode, ISP_SATURATION_ATTR_S* pSaturation)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
    ISP_DEV IspDev = 0;
    HI_S32 i = 0;

    s32Ret = HI_MPI_ISP_GetSaturationAttr(IspDev, pSaturation);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "Saturation"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pSaturation->stAuto.au8Sat[i] = temp[i];
            }
        }
        else
        {
            WRN("saturation_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

//static int parse_sharpen_config(IXML_Node* pNode, ISP_SHARPEN_ATTR_S* pSharpenAttr)
//{
//    IXML_Node *tmpChild = NULL;
//    IXML_Node *tmpAttr = NULL;
//    HI_S32 s32Ret = HI_SUCCESS;
//    ISP_DEV IspDev = 0;
//    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
//    memset(temp, 0, sizeof(temp));
//    HI_S32 i = 0;
//
//    s32Ret = HI_MPI_ISP_GetSharpenAttr(IspDev, pSharpenAttr);
//    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//    tmpChild = pNode->firstChild;
//    while(tmpChild)
//    {
//        if(!strcmp(tmpChild->nodeName, "SharpenD"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8SharpenD[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "SharpenUD"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8SharpenUd[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "Overshoot"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8OverShoot[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "Undershoot"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8UnderShoot[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "TextureNoiseThd"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8TextureNoiseThd[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "EdgeNoiseThd"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.au8EdgeNoiseThd[i] = temp[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "EnLowLumaShoot"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                pSharpenAttr->stAuto.abEnLowLumaShoot[i] = (HI_BOOL)temp[i];
//            }
//        }
//        else
//        {
//            WRN("sharpen_config not support %s\n", tmpChild->nodeName);
//        }
//        tmpChild =  tmpChild->nextSibling;
//    }
//
//    return HI_SUCCESS;
//}

//static int parse_nr_config(IXML_Node* pNode, VPSS_NR_PARAM_U* p3DNR, ISP_NR_ATTR_S* p2DNR)
//{
//    IXML_Node *tmpChild = NULL;
//    IXML_Node *tmpAttr = NULL;
//    HI_S32 s32Ret = HI_SUCCESS;
//    HI_S32 str[ISP_AUTO_STENGTH_NUM];
//    HI_S32 i;
//
//    VPSS_GRP VpssGrp = 0;
//    ISP_DEV IspDev = 0;
//    s32Ret = HI_MPI_ISP_GetNRAttr(IspDev, p2DNR);
//    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//    s32Ret = HI_MPI_VPSS_GetNRParam(VpssGrp, p3DNR);
//    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//    tmpChild = pNode->firstChild;
//    while(tmpChild)
//    {
//        if (!strcmp(tmpChild->nodeName, "Denoise"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            p2DNR->bEnable = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
//        }
//        else if(!strcmp(tmpChild->nodeName, "VarStrength"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p2DNR->stAuto.au8VarStrength[i] = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "FixStrength"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p2DNR->stAuto.au8FixStrength[i] = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "LowFreqSlope"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p2DNR->stAuto.au8LowFreqSlope[i] = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "Threshold"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p2DNR->stAuto.au16Threshold[i] = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YPKStr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YPKStr = str[i];
//            }
//        }
//        else if (!strcmp(tmpChild->nodeName, "YSFStr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSFStr = str[i];
//            }
//        }
//         else if(!strcmp(tmpChild->nodeName, "YTFStr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YTFStr = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "TFStrMax"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32TFStrMax = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "TFStrMov"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32TFStrMov = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YSmthStr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSmthStr = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YSmthRat"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSmthRat = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YSFStrDlt"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSFStrDlt = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YSFStrDl"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSFStrDl = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YTFStrDlt"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YTFStrDlt = str[i];
//            }
//        }
//         else if(!strcmp(tmpChild->nodeName, "YTFStrDl"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YTFStrDl = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YSFBriRat"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YSFBriRat = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "CSFStr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32CSFStr = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "CTFstr"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32CTFstr = str[i];
//            }
//        }
//        else if(!strcmp(tmpChild->nodeName, "YTFMdWin"))
//        {
//            tmpAttr = tmpChild->firstAttr;
//            s32Ret = str2array(tmpAttr->nodeValue, str, ISP_AUTO_STENGTH_NUM);
//            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
//
//            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
//            {
//                p3DNR[i].stNRParam_V1.s32YTFMdWin = str[i];
//            }
//        }
//        else
//        {
//            WRN("nr_config not support %s\n", tmpChild->nodeName);
//        }
//        tmpChild =  tmpChild->nextSibling;
//    }
//
//    return HI_SUCCESS;
//}

static int parse_demosaic_config(IXML_Node* pNode, ISP_ANTI_FALSECOLOR_S* pAntiFC, ISP_DEMOSAIC_ATTR_S* pDemosaicAttr)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
    memset(temp, 0, sizeof(temp));
    HI_U32 i = 0;

    s32Ret = HI_MPI_ISP_GetDemosaicAttr(IspDev, pDemosaicAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    s32Ret = HI_MPI_ISP_GetAntiFalseColorAttr(IspDev, pAntiFC);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "vhSlope"))
        {
            //tmpAttr = tmpChild->firstAttr;
            //pDemosaicAttr->u16VhSlope = strtold(tmpAttr->nodeValue, NULL);
        }
        /*else if(!strcmp(tmpChild->nodeName, "UuSlope"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDemosaicAttr->u16UuSlope = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "VhLimit"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDemosaicAttr->u8VhLimit = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "VhOffset"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDemosaicAttr->u8VhOffset = strtold(tmpAttr->nodeValue, NULL);
        }*/
        /*else if(!strcmp(tmpChild->nodeName, "NpOffset"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDemosaicAttr->au16NpOffset[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "AntiFalseColor"))
        {
            tmpAttr = tmpChild->firstAttr;
            pAntiFC->bEnable = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }*/
        else if(!strcmp(tmpChild->nodeName, "AntiFalseColorStrength"))
        {
            /*tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pAntiFC->stAuto.au8Strength[i] = temp[i];
            }*/
        }
        else if (!strcmp(tmpChild->nodeName, "AntiFalseColorThreshold"))
        {
            /*tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pAntiFC->stAuto.au8Threshold[i] = temp[i];
            }*/
        }
        else
        {
            WRN("demosaic_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int parse_gamma_config(IXML_Node* pNode, ISP_GAMMA_ATTR_S* pGmmmaAttr, HI_U16* pSysgainThreshold)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[GAMMA_NODE_NUM];
    HI_S32 i = 0;

    for (i = 0; i < GAMMA_LUMA_STATE_BUTT; i++)
    {
        s32Ret = HI_MPI_ISP_GetGammaAttr(IspDev, &pGmmmaAttr[i]);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    }

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "LowLuma"))
        {
            tmpAttr = tmpChild->firstAttr;
            pSysgainThreshold[GAMMA_LUMA_STATE_LOW] = strtold(tmpAttr->nodeValue, NULL);
            tmpAttr = tmpAttr->nextSibling;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < GAMMA_NODE_NUM; i++)
            {
                pGmmmaAttr[GAMMA_LUMA_STATE_LOW].u16Table[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "MidLuma"))
        {
            tmpAttr = tmpChild->firstAttr;
            pSysgainThreshold[GAMMA_LUMA_STATE_MIDDLE] = strtold(tmpAttr->nodeValue, NULL);
            tmpAttr = tmpAttr->nextSibling;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < GAMMA_NODE_NUM; i++)
            {
                pGmmmaAttr[GAMMA_LUMA_STATE_MIDDLE].u16Table[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "HighLuma"))
        {
            tmpAttr = tmpChild->firstAttr;
            pSysgainThreshold[GAMMA_LUMA_STATE_HIGH] = strtold(tmpAttr->nodeValue, NULL);
            tmpAttr = tmpAttr->nextSibling;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < GAMMA_NODE_NUM; i++)
            {
                pGmmmaAttr[GAMMA_LUMA_STATE_HIGH].u16Table[i] = temp[i];
            }
        }
        else
        {
            WRN("gamma_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }
    return HI_SUCCESS;
}


static int parse_defog_config(IXML_Node* pNode, ISP_DEFOG_ATTR_S* pDefog)
{
    /*IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 ret = 0 ;
    int i = 0;

    for (i = 0; i < ISP_AUTO_ISO_STENGTH_NUM; i++)
    {
        ret = HI_MPI_ISP_GetDeFogAttr(0, &pDefog[i]);
        CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

        pDefog[i].bEnable = (HI_BOOL)strtold(pNode->firstAttr->nodeValue, NULL);
        pDefog[i].enOpType = OP_TYPE_AUTO;
    }

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "HorizontalBlock"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_ISO_STENGTH_NUM; i++)
            {
                pDefog[i].u8HorizontalBlock = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "VerticalBlock"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_ISO_STENGTH_NUM; i++)
            {
                pDefog[i].u8VerticalBlock = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "Strength"))
        {
            tmpAttr = tmpChild->firstAttr;
            int Strength[ISP_AUTO_STENGTH_NUM];
            int argNum = 0;
            memset(Strength, 0, sizeof(Strength));

            argNum = array_num(tmpAttr->nodeValue, Strength, sizeof(Strength)/sizeof(*Strength));
            if(argNum == 1)
            {
                for (i = 0;i < ISP_AUTO_STENGTH_NUM; i++)
                {
                    pDefog[i].stAuto.u8strength = Strength[0];
                }
            }
            else if(argNum == 16)
            {
                for (i = 0;i < ISP_AUTO_STENGTH_NUM; i++)
                {
                    pDefog[i].stAuto.u8strength = Strength[i];
                }
            }
            else
            {
                WRN("defog_config args not reasonable,arg num is %d\n",argNum);
            }
        }
        else
        {
            WRN("defog_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }*/
    return HI_SUCCESS;
}

static int parse_drc_config(IXML_Node* pNode, ISP_DRC_ATTR_S* pDrc)
{
   /* IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;

    ISP_DRC_ATTR_S tmp;
    int ret = HI_MPI_ISP_GetDRCAttr(0, &tmp);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    int i = 0;
    for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
    {
        memcpy(&pDrc[i], &tmp, sizeof(tmp));
        pDrc[i].bEnable = (HI_BOOL)strtold(pNode->firstAttr->nodeValue, NULL);
    }

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "SpatialVar"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8SpatialVar = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "RangeVar"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8RangeVar = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "Asymmetry"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8Asymmetry = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "SecondPole"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8SecondPole = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "Stretch"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8Stretch = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "LocalMixingThres"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u8LocalMixingThres = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "DarkGainLmtY"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u16DarkGainLmtY = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "DarkGainLmtC"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u16DarkGainLmtC = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "BrightGainLmt"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].u16BrightGainLmt = strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "OpType"))
        {
            tmpAttr = tmpChild->firstAttr;
            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].enOpType = (ISP_OP_TYPE_E)strtold(tmpAttr->nodeValue, NULL);
            }
        }
        else if(!strcmp(tmpChild->nodeName, "ManualStrength"))
        {
            tmpAttr = tmpChild->firstAttr;
            int ManualStrength[ISP_AUTO_STENGTH_NUM];
            memset(ManualStrength, 0, sizeof(ManualStrength));

            ret = str2array(tmpAttr->nodeValue, ManualStrength, sizeof(ManualStrength)/sizeof(*ManualStrength));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].stManual.u8Strength = ManualStrength[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "ManualLocalMixingBrigtht"))
        {
            tmpAttr = tmpChild->firstAttr;
            int ManualLocalMixingBrigtht[ISP_AUTO_STENGTH_NUM];
            memset(ManualLocalMixingBrigtht, 0, sizeof(ManualLocalMixingBrigtht));

            ret = str2array(tmpAttr->nodeValue, ManualLocalMixingBrigtht, sizeof(ManualLocalMixingBrigtht)/sizeof(*ManualLocalMixingBrigtht));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].stManual.u8LocalMixingBrigtht = ManualLocalMixingBrigtht[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "ManualLocalMixingDark"))
        {
            tmpAttr = tmpChild->firstAttr;
            int ManualLocalMixingDark[ISP_AUTO_STENGTH_NUM];
            memset(ManualLocalMixingDark, 0, sizeof(ManualLocalMixingDark));

            ret = str2array(tmpAttr->nodeValue, ManualLocalMixingDark, sizeof(ManualLocalMixingDark)/sizeof(*ManualLocalMixingDark));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDrc[i].stManual.u8LocalMixingDark = ManualLocalMixingDark[i];
            }
        }
        else
        {
            WRN("drc_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }*/

    return HI_SUCCESS;
}

static int parse_dci_config(IXML_Node* pNode, VI_DCI_PARAM_S* pDci)
{
    IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;

    int ret = HI_MPI_VI_GetDCIParam(0, pDci);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    pDci->bEnable = (HI_BOOL)strtold(pNode->firstAttr->nodeValue, NULL);
    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "BlackGain"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDci->u32BlackGain = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "ContrastGain"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDci->u32ContrastGain = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "LightGain"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDci->u32LightGain = strtold(tmpAttr->nodeValue, NULL);
        }
        else
        {
            WRN("dci_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int parse_defect_config(IXML_Node* pNode, ISP_DP_DYNAMIC_ATTR_S* pDPDynamicAttr)
{
    /*IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 tmp[ISP_AUTO_STENGTH_NUM] = {0};
    HI_S32 i = 0;

    int ret = HI_MPI_ISP_GetDPDynamicAttr(0, pDPDynamicAttr);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "DPDynamic"))
        {
            tmpAttr = tmpChild->firstAttr;
            pDPDynamicAttr->bEnable = (HI_BOOL)strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Slope"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDPDynamicAttr->stAuto.au16Slope[i] = tmp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "BlendRatio"))
        {
            tmpAttr = tmpChild->firstAttr;
            ret = str2array(tmpAttr->nodeValue, tmp, sizeof(tmp)/sizeof(*tmp));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            for (i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pDPDynamicAttr->stAuto.au16BlendRatio[i] = tmp[i];
            }
        }
        else
        {
            WRN("defect_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }*/

    return HI_SUCCESS;
}

//static int parse_uvnr_config(IXML_Node* pNode, ISP_UVNR_ATTR_S* pUvnrAttr)
//{
    /*IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
    memset(temp, 0, sizeof(temp));
    HI_S32 i = 0;

    s32Ret = HI_MPI_ISP_GetUVNRAttr(IspDev, pUvnrAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    pUvnrAttr->bEnable = (HI_BOOL)strtold(pNode->firstAttr->nodeValue, NULL);
    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "Colorcast"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pUvnrAttr->stAuto.au8ColorCast[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "Uvnrthreshold"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pUvnrAttr->stAuto.au8UvnrThreshold[i] = temp[i];
            }
        }
        else if(!strcmp(tmpChild->nodeName, "Uvnrstrength"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pUvnrAttr->stAuto.au8UvnrStrength[i] = temp[i];
            }
        }
        else
        {
            WRN("uvnr_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }*/

    //return HI_SUCCESS;
//}

static int parse_cross_talk_removal_config(IXML_Node* pNode, ISP_CR_ATTR_S* pCRAttr)
{
    /*IXML_Node *tmpChild = NULL;
    IXML_Node *tmpAttr = NULL;
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 temp[ISP_AUTO_STENGTH_NUM];
    memset(temp, 0, sizeof(temp));
    HI_S32 i = 0;

    s32Ret = HI_MPI_ISP_GetCrosstalkAttr(IspDev, pCRAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    pCRAttr->bEnable = (HI_BOOL)strtold(pNode->firstAttr->nodeValue, NULL);
    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "Threshold"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCRAttr->u16Threshold = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Slope"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCRAttr->u8Slope = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Sensitivity"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCRAttr->u8Sensitivity = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Sensithreshold"))
        {
            tmpAttr = tmpChild->firstAttr;
            pCRAttr->u16SensiThreshold = strtold(tmpAttr->nodeValue, NULL);
        }
        else if(!strcmp(tmpChild->nodeName, "Strength"))
        {
            tmpAttr = tmpChild->firstAttr;
            s32Ret = str2array(tmpAttr->nodeValue, temp, sizeof(temp)/sizeof(*temp));
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

            for(i = 0; i < ISP_AUTO_STENGTH_NUM; i++)
            {
                pCRAttr->au16Strength[i] = temp[i];
            }
        }
        else
        {
            WRN("cross_talk_removal_config not support %s\n", tmpChild->nodeName);
        }
        tmpChild =  tmpChild->nextSibling;
    }*/

    return HI_SUCCESS;
}

/*线性插值*/
static HI_S32 _linear(int level, int iso, int pre_val, int next_val)
{
    HI_S32 min = 0;
    HI_S32 max = 0;
    //线性插值后设置到相关模块的参数
    HI_S32 effective = 0;
    HI_S32 iso_interval = (1 << level) * 100;
    HI_S32 iso_level_min = (1 << level) * 100;

    //线性递增
    if(pre_val < next_val)
    {
        min = pre_val;
        max = next_val;
        //这个公式保证*param_effective 在min到max之间
        effective = min + (iso - iso_level_min)* (max - min) / iso_interval;
        //DBG("param:0x%02x, add 0x%02x, min:0x%02x, max:0x%02x\n", effective, (iso - iso_level_min)* (max - min) / iso_interval, min, max);
    }
    //线性递减
    else
    {
        min = next_val;
        max = pre_val;
        //这个公式保证*param_effective 在min到max之间
        effective = max - (iso - iso_level_min) * (max - min) / iso_interval;
        //DBG("param:0x%02x, sub 0x%02x, min:0x%02x, max:0x%02x\n", effective, (iso - iso_level_min)* (max - min) / iso_interval, min, max);
    }

    return effective;
}


static HI_S32 set_defect_config()
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;

    //s32Ret = HI_MPI_ISP_SetDPDynamicAttr(IspDev, &gp_isp_config->stDPDynamic);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_dci_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    s32Ret = HI_MPI_VI_SetDCIParam(IspDev, &gp_isp_config->stDci);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_cross_talk_removal_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    s32Ret = HI_MPI_ISP_SetCrosstalkAttr(IspDev, &gp_isp_config->stCRAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_exposure_attr_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    gp_isp_config->stExpAttr.enOpType = OP_TYPE_AUTO;

    //show_exposure_attr(&g_isp_config.stExpAttr);
    s32Ret = HI_MPI_ISP_SetExposureAttr(IspDev, &gp_isp_config->stExpAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_drc_config(ISO_LEVEL level, int iso)
{
    //HI_S32 ret = HI_SUCCESS;
    //ISP_DRC_ATTR_S stDrcAttr;

    //int iso_level = (int)level;

    //if(!(iso_level >= 0 && iso_level < ISP_AUTO_STENGTH_NUM))
    //{
    //    ERR("iso_level is invalid\n");
    //    return HI_FAILURE;
    //}

    // //消除drc 档位中的线性变化
    //if(access("/mnt/nand/remove_drc_linear.flag", F_OK) == 0)
    //{
    //    ret = HI_MPI_ISP_SetDRCAttr(0, &gp_isp_config->astDrcAttr[iso_level]);
    //    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    //    return HI_SUCCESS;
    //}

    //if(iso_level + 1 < 16)
    //{
    //    ISP_DRC_ATTR_S *pstDrcAttr = gp_isp_config->astDrcAttr;

    //    memset(&stDrcAttr, 0 , sizeof(stDrcAttr));
    //    memcpy(&stDrcAttr, &gp_isp_config->astDrcAttr[iso_level], sizeof(stDrcAttr));
    //    stDrcAttr.stManual.u8Strength = _linear(iso_level, iso, pstDrcAttr[iso_level].stManual.u8Strength, pstDrcAttr[iso_level+1].stManual.u8Strength);
    //    stDrcAttr.stManual.u8LocalMixingBrigtht = _linear(iso_level, iso, pstDrcAttr[iso_level].stManual.u8LocalMixingBrigtht, pstDrcAttr[iso_level+1].stManual.u8LocalMixingBrigtht);
    //    stDrcAttr.stManual.u8LocalMixingDark = _linear(iso_level, iso, pstDrcAttr[iso_level].stManual.u8LocalMixingDark, pstDrcAttr[iso_level+1].stManual.u8LocalMixingDark);

    //    ret = HI_MPI_ISP_SetDRCAttr(0, &stDrcAttr);
    //    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
    //}
    ////最后一档
    //else
    //{
    //    ret = HI_MPI_ISP_SetDRCAttr(0, &gp_isp_config->astDrcAttr[iso_level]);
    //    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
    //}

    return HI_SUCCESS;
}

static HI_S32 set_ae_route_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    s32Ret = HI_MPI_ISP_SetAERouteAttr(IspDev, &gp_isp_config->stAERoute);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_wb_attr_config(int level)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    s32Ret = HI_MPI_ISP_SetBlackLevelAttr(IspDev, &gp_isp_config->stBlackLevel[level]);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    gp_isp_config->stWBAttr.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetWBAttr(IspDev, &gp_isp_config->stWBAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    gp_isp_config->stAWBAttrEx.stInOrOut.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetAWBAttrEx(IspDev, &gp_isp_config->stAWBAttrEx);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_ccm_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    gp_isp_config->stCCMAttr.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetCCMAttr(IspDev, &gp_isp_config->stCCMAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_saturation_config()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    gp_isp_config->stSaturation.enOpType = OP_TYPE_AUTO;
    s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &gp_isp_config->stSaturation);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_sharpen_config()
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;

    //gp_isp_config->stSharpenAttr.enOpType = OP_TYPE_AUTO;
    //s32Ret = HI_MPI_ISP_SetSharpenAttr(IspDev, &gp_isp_config->stSharpenAttr);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_3dnr_config(ISO_LEVEL level, int iso)
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //VPSS_GRP VpssGrp = 0;
    //int iso_level = (int)level;
    //VPSS_NR_PARAM_U unNrParam;
    ////iso档位的跨度
    //HI_S32 iso_interval;
    ////iso档位的最小值
    //HI_S32 iso_level_min;
    ////iq xml参数中两个相邻iso档位的值
    //HI_S32 *p3dnr_param;
    //HI_S32 *p3dnr_param_next_level;
    ////iq xml参数中两个相邻iso档位的最大最小值
    //HI_S32 min;
    //HI_S32 max;
    ////线性插值后设置到3d降噪模块的参数
    //HI_S32 *p3dnr_param_effective;
    //HI_U32 i;

    //if(!(iso_level >= 0 && iso_level < 16))
    //{
    //    ERR("iso_level is invalid\n");
    //    return HI_FAILURE;
    //}

    ////remove_3dnr_linear.flag,消除3d降噪档位中的线性变化
    //if(access("/mnt/nand/remove_3dnr_linear.flag", F_OK) == 0)
    //{
    //    s32Ret = HI_MPI_VPSS_SetNRParam(VpssGrp, &gp_isp_config->astNR[iso_level]);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //    return HI_SUCCESS;
    //}

    ////DBG("iso level:%d\n", iso_level);
    ////iso值在各种档位中要有线性关系
    //if(iso_level + 1 < 16)
    //{
    //    iso_interval = (1 << iso_level) * 100;
    //    iso_level_min = (1 << iso_level) * 100;
    //    p3dnr_param = &(gp_isp_config->astNR[iso_level].stNRParam_V1.s32YPKStr);
    //    p3dnr_param_next_level = &(gp_isp_config->astNR[iso_level + 1].stNRParam_V1.s32YPKStr);
    //    p3dnr_param_effective = &(unNrParam.stNRParam_V1.s32YPKStr);

    //    //遍历3d降噪的各个参数，根据当前iso修改成线性的值
    //    for(i = 0; i < sizeof(VPSS_NR_PARAM_U)/sizeof(HI_S32); i++)
    //    {
    //        //线性递增
    //        if(*p3dnr_param < *p3dnr_param_next_level)
    //        {
    //            min = *p3dnr_param;
    //            max = *p3dnr_param_next_level;
    //            //这个公式保证*p3dnr_param_effective 在min到max之间
    //            *p3dnr_param_effective = min + (iso - iso_level_min)* (max - min) / iso_interval;
    //            //DBG("3dnr param:0x%02x, add 0x%02x, min:0x%02x, max:0x%02x\n", *p3dnr_param_effective, (iso - iso_level_min)* (max - min) / iso_interval, min, max);
    //        }
    //        //线性递减
    //        else
    //        {
    //            min = *p3dnr_param_next_level;
    //            max = *p3dnr_param;
    //            //这个公式保证*p3dnr_param_effective 在min到max之间
    //            *p3dnr_param_effective = max - (iso - iso_level_min) * (max - min) / iso_interval;
    //            //DBG("3dnr param:0x%02x, sub 0x%02x, min:0x%02x, max:0x%02x\n", *p3dnr_param_effective, (iso - iso_level_min)* (max - min) / iso_interval, min, max);
    //        }

    //        p3dnr_param++;
    //        p3dnr_param_next_level++;
    //        p3dnr_param_effective++;
    //    }

    //    s32Ret = HI_MPI_VPSS_SetNRParam(VpssGrp, &unNrParam);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //}
    ////最后一档
    //else
    //{
    //    s32Ret = HI_MPI_VPSS_SetNRParam(VpssGrp, &gp_isp_config->astNR[iso_level]);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //}

    return HI_SUCCESS;
}

static HI_S32 set_blc_config(ISO_LEVEL level, int iso)
{
    HI_S32 ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    int i = 0;
    ISP_BLACK_LEVEL_S stBlackLevel;

    if(access("/mnt/nand/remove_blc_linear.flag", F_OK) == 0)
    {
        ret = HI_MPI_ISP_SetBlackLevelAttr(IspDev, &gp_isp_config->stBlackLevel[level]);
        CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        return HI_SUCCESS;
    }

    //iso值在各种档位中要有线性关系
    ret = HI_MPI_ISP_GetBlackLevelAttr(IspDev, &stBlackLevel);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    if(level + 1 < 16)
    {
        for (i = 0; i < 4; i++)
        {
            ISP_BLACK_LEVEL_S* pstBlackLevel = gp_isp_config->stBlackLevel;
            stBlackLevel.au16BlackLevel[i] = _linear(level, iso, pstBlackLevel[level].au16BlackLevel[i], pstBlackLevel[level+1].au16BlackLevel[i]);
        }
        ret = HI_MPI_ISP_SetBlackLevelAttr(IspDev, &stBlackLevel);
        CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
    }
    //最后一档
    else
    {
        ret = HI_MPI_ISP_SetBlackLevelAttr(IspDev, &gp_isp_config->stBlackLevel[level]);
        CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
    }

    return HI_SUCCESS;
}


static HI_S32 set_nr_config(ISO_LEVEL level)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VPSS_GRP VpssGrp = 0;
    ISP_DEV IspDev= 0;
    int iso_level = (int)level;

    if(!(iso_level >= 0 && iso_level < 16))
    {
        DBG("iso_level is invalid\n");
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_ISP_SetNRAttr(IspDev, &gp_isp_config->stNRAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    //s32Ret = HI_MPI_VPSS_SetNRParam(VpssGrp, &gp_isp_config->astNR[iso_level]);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_demosaic_config()
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;

    //s32Ret = HI_MPI_ISP_SetDemosaicAttr(IspDev, &gp_isp_config->stDemosaicAttr);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    //gp_isp_config->stAntiFC.enOpType = OP_TYPE_AUTO;
    //s32Ret = HI_MPI_ISP_SetAntiFalseColorAttr(IspDev, &gp_isp_config->stAntiFC);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static HI_S32 set_uvnr_config()
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;

    //gp_isp_config->stUvnrAttr.enOpType = OP_TYPE_AUTO;
    //s32Ret = HI_MPI_ISP_SetUVNRAttr(IspDev, &gp_isp_config->stUvnrAttr);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

/*
static int show_gamma(ISP_GAMMA_ATTR_S *pstGammaAttr)
{
    DBG("bEnable: %d\n", pstGammaAttr->bEnable);
    DBG("enCurveType: %d\n", pstGammaAttr->enCurveType);
    unsigned int i = 0;
    for (i = 0; i < sizeof(pstGammaAttr->u16Table)/sizeof(*pstGammaAttr->u16Table); i++)
    {
        if (i % 16 == 0)
        {
            printf("\n");
        }
        printf("%#x,", pstGammaAttr->u16Table[i]);

    }
    printf("\n");

    return 0;
}
*/

static HI_S32 set_gamma_config(GAMMA_LUMA_STATE gamma_state)
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;

    gp_isp_config->stGammaAttr[gamma_state].enCurveType = ISP_GAMMA_CURVE_USER_DEFINE;
    s32Ret = HI_MPI_ISP_SetGammaAttr(IspDev, &gp_isp_config->stGammaAttr[gamma_state]);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    //DBG("new gamma: \n");
    //show_gamma(&gp_isp_config->stGammaAttr[gamma_state]);

    return HI_SUCCESS;
}

static HI_S32 set_defog_config(ISO_LEVEL level,int iso)
{
    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;
    //int iso_level = (int)level;

    //s32Ret = HI_MPI_ISP_SetDeFogAttr(IspDev, &gp_isp_config->stDefogAttr[iso_level]);

    //ISP_DEFOG_ATTR_S stDefogParam;

    //s32Ret = HI_MPI_ISP_GetDeFogAttr(IspDev, &stDefogParam);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);


    ////remove_defog_linear.flag,消除3d降噪档位中的线性变化
    //if(access("/mnt/nand/remove_defog_linear.flag", F_OK) == 0)
    //{
    //    s32Ret = HI_MPI_ISP_SetDeFogAttr(IspDev, &gp_isp_config->stDefogAttr[iso_level]);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //    return HI_SUCCESS;
    //}

    //if(level + 1 < 16)
    //{
    //    ISP_DEFOG_ATTR_S *pstDefogParam = gp_isp_config->stDefogAttr;

    //    stDefogParam.enOpType = pstDefogParam[level].enOpType;
    //    stDefogParam.bEnable = pstDefogParam[level].bEnable;
    //    stDefogParam.u8HorizontalBlock = pstDefogParam[level].u8HorizontalBlock;
    //    stDefogParam.u8VerticalBlock = pstDefogParam[level].u8VerticalBlock;
    //    stDefogParam.stAuto.u8strength = _linear(level, iso, pstDefogParam[level].stAuto.u8strength, pstDefogParam[level+1].stAuto.u8strength);
    //    s32Ret = HI_MPI_ISP_SetDeFogAttr(IspDev, &stDefogParam);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //}
    //else
    //{
    //    s32Ret = HI_MPI_ISP_SetDeFogAttr(IspDev, &gp_isp_config->stDefogAttr[iso_level]);
    //    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);
    //}


    return HI_SUCCESS;
}


static int process_all(IXML_Node* pNode)
{
    IXML_Node *tmpChild = NULL;
    int ret = -1;

    tmpChild = pNode->firstChild;
    while(tmpChild)
    {
        if(!strcmp(tmpChild->nodeName, "ExposureAttr"))
        {
            ret = parse_exposure_attr_config(tmpChild, &(gp_isp_config->stExpAttr));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_exposure_attr_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "AERoute"))
        {
            ret = parse_ae_route_config(tmpChild, &(gp_isp_config->stAERoute));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_ae_route_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "WBAttr"))
        {
            ret = parse_wb_attr_config(tmpChild, gp_isp_config->stBlackLevel, &gp_isp_config->stWBAttr, &gp_isp_config->stAWBAttrEx);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_wb_attr_config(gp_isp_config->last_iso_level);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "CCM"))
        {
            ret = parse_ccm_config(tmpChild, &(gp_isp_config->stCCMAttr));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_ccm_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Saturation"))
        {
            ret = parse_saturation_config(tmpChild, &(gp_isp_config->stSaturation));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_saturation_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Sharpen"))
        {
            /*ret = parse_sharpen_config(tmpChild, &gp_isp_config->stSharpenAttr);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_sharpen_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);*/
        }
        else if(!strcmp(tmpChild->nodeName, "NR"))
        {
            //ret = parse_nr_config(tmpChild, gp_isp_config->astNR, &gp_isp_config->stNRAttr);
            //CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            //ret = set_nr_config(gp_isp_config->last_iso_level);
            //CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "DRC"))
        {
            ret = parse_drc_config(tmpChild, (gp_isp_config->astDrcAttr));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_drc_config(gp_isp_config->last_iso_level, gp_isp_config->last_iso);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Uvnr"))
        {
            //ret = parse_uvnr_config(tmpChild, (&gp_isp_config->stUvnrAttr));
            //CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            //ret = set_uvnr_config();
            //CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Demosaic"))
        {
            ret = parse_demosaic_config(tmpChild, &gp_isp_config->stAntiFC, &gp_isp_config->stDemosaicAttr);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_demosaic_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "DCI"))
        {
            ret = parse_dci_config(tmpChild, &(gp_isp_config->stDci));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_dci_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Gamma"))
        {
            ret = parse_gamma_config(tmpChild, gp_isp_config->stGammaAttr, gp_isp_config->GammaSysgainThreshold);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_gamma_config(gp_isp_config->last_gamma_state);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "DeFog"))
        {
            ret = parse_defog_config(tmpChild, gp_isp_config->stDefogAttr);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_defog_config(gp_isp_config->last_iso_level,gp_isp_config->last_iso);
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "Defect"))
        {
            ret = parse_defect_config(tmpChild, &(gp_isp_config->stDPDynamic));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_defect_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else if(!strcmp(tmpChild->nodeName, "CrosstalkRemoval"))
        {
            ret = parse_cross_talk_removal_config(tmpChild, &(gp_isp_config->stCRAttr));
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

            ret = set_cross_talk_removal_config();
            CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
        }
        else
        {
            DBG("isp config not support %s\n", tmpChild->nodeName);
        }
        tmpChild = tmpChild->nextSibling;
    }

    return HI_SUCCESS;
}

static int isp_saturation_set(HI_U8 value)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");

    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    ISP_SATURATION_ATTR_S stSatAttr;
    memset(&stSatAttr, 0, sizeof(stSatAttr));

    if (value == 128)
    {
        // 自动模式
        set_saturation_config();
        return HI_SUCCESS;
    }

    stSatAttr.enOpType = OP_TYPE_MANUAL;
    stSatAttr.stManual.u8Saturation = value;
    s32Ret = HI_MPI_ISP_SetSaturationAttr(IspDev, &stSatAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

// 用DCI ContrastGain 来做对比度设置
static int isp_contrast_set(HI_U8 value)
{
    CHECK(NULL != gp_isp_config, -1,"isp is not inited.\n");

    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    VI_DCI_PARAM_S stDciAttr;
    memset(&stDciAttr, 0, sizeof(stDciAttr));

    if (value == 128)
    {
        // 自动模式
        set_dci_config();
        return HI_SUCCESS;
    }

    HI_U8 Contrast = value / 4;
    if(Contrast >= 63)
        Contrast = 63;

    memcpy(&stDciAttr, &gp_isp_config->stDci, sizeof(stDciAttr));
    stDciAttr.u32ContrastGain = Contrast;
    s32Ret = HI_MPI_VI_SetDCIParam(IspDev, &stDciAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

// Defog 在高级参数提供独立配置功能 AUTO MANU DISABLE
/*
static int isp_defog_set(HI_U8 DefogValue)
{
    CHECK(NULL != gp_isp_config, -1,"isp is not inited.\n");

    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEFOG_ATTR_S stDefogAttr;
    memset(&stDefogAttr, 0, sizeof(stDefogAttr));

    s32Ret = set_defog_config(gp_isp_config->last_iso_level,gp_isp_config->last_iso);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}
*/

static int isp_brightness_set(HI_U8 value)
{
    CHECK(NULL != gp_isp_config, -1,"isp is not inited.\n");

    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    ISP_EXPOSURE_ATTR_S stExpAttr;
    memcpy(&stExpAttr, &gp_isp_config->stExpAttr, sizeof(stExpAttr));

    int target = stExpAttr.stAuto.u8Compensation * value / 128;
    if (target > 0xff)
    {
        target = 0xff;
    }
    stExpAttr.stAuto.u8Compensation = target;

    DBG("value: %d, u8Compensation: %d, target: %d.\n", value, stExpAttr.stAuto.u8Compensation, target);
    s32Ret = HI_MPI_ISP_SetExposureAttr(IspDev, &stExpAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static int isp_sharpness_set(HI_U8 value)
{
    //CHECK(NULL != gp_isp_config, -1,"isp is not inited.\n");

    //HI_S32 s32Ret = HI_SUCCESS;
    //ISP_DEV IspDev = 0;
    //ISP_SHARPEN_ATTR_S stSharpenAttr;
    //memset(&stSharpenAttr, 0, sizeof(stSharpenAttr));

    //if (value == 128)
    //{
    //    // 自动模式
    //    set_sharpen_config();
    //    return HI_SUCCESS;
    //}

    //s32Ret = HI_MPI_ISP_GetSharpenAttr(IspDev, &stSharpenAttr);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    //stSharpenAttr.enOpType = OP_TYPE_MANUAL;
    //stSharpenAttr.stManual.u8SharpenD = value;
    //stSharpenAttr.stManual.u8SharpenUd = value;

    //s32Ret = HI_MPI_ISP_SetSharpenAttr(IspDev, &stSharpenAttr);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    return HI_SUCCESS;
}

static const char* isp_get_xml_path()
{
    SAMPLE_VI_MODE_E sensor_type = SENSOR_TYPE;
    const char* path = NULL;
    //default xml
    switch(sensor_type)
    {
        case OMNIVISION_OV9732_MIPI_720P_30FPS:
            path = "/opt/topsee/th32e6.xml";
            break;
        case OMNIVISION_OV2710_MIPI_1080P_30FPS:
            path = "/usr/bin/ov2710.xml";
            break;
        case SONY_IMX291_LVDS_1080P_30FPS:
            path = "/opt/topsee/th38q.xml";
            break;
        case SONY_IMX323_DC_1080P_30FPS:
            path = "/opt/topsee/th38j.xml";
            break;
        case SMARTSENS_SC1135_DC_960P_30FPS:
            path = "/opt/topsee/th38c20.xml";
            break;
        case APTINA_AR0237_DC_1080P_30FPS:
            path = "/opt/topsee/th38j10.xml";
            break;
        case SMARTSENS_SC2135_DC_1080P_30FPS:
            path = "/opt/topsee/th38a10.xml";
            break;
        case APTINA_AR0130_DC_720P_30FPS:
            path = "/opt/topsee/th38c33.xml";
            break;
        default :
            DBG("unknown sensor type %d\n", sensor_type);
            break;
    }

    if (!access(REAL_ISP_CONFIG_PATH, F_OK))
    {
        path = REAL_ISP_CONFIG_PATH;
    }

    return path;
}

static int isp_day_night_switch()
{
    int ret = -1;

    sal_image_attr_s* image_attr = &gp_isp_config->image_attr;
    isp_saturation_set(image_attr->saturation);

    const char* path = isp_get_xml_path();
    CHECK(path != NULL, -1, "xml path is null.\n");

    int fd = open(path, O_RDONLY);
    CHECK(fd >= 0, -1, "open %s failed by %s\n", path, strerror(errno));

    int file_len = lseek(fd, 0, SEEK_END);
    char* pCfgXml = (char*)malloc(file_len + 1);
    if (pCfgXml == NULL)
    {
        close(fd);
        DBG("failed to malloc pCfgXml %d bytes.\n", file_len+1);
        return -1;
    }

    pCfgXml[file_len] = '\0';
    lseek(fd, 0, SEEK_SET);

    ret = read(fd, pCfgXml, file_len);
    if (ret != file_len)
    {
        free(pCfgXml);
        close(fd);
        DBG("read error %d, expect %d\n", ret, file_len);
        return -1;
    }

    IXML_Document *pDocNode = NULL;
    if (strstr(pCfgXml, "USER_DEFINE_ENCRYPT_ISP"))
    {
        DBG("get USER_DEFINE_ENCRYPT_ISP flag, is an encrypted isp cfg file\n");
/*
        int decodeBufLen = file_len * 2;
        char *pDecodeBuf = (char *)malloc(decodeBufLen);
        if (pDecodeBuf == NULL)
        {
            free(pCfgXml);
            close(fd);
            DBG("failed to malloc pDecodeBuf %d bytes.\n", decodeBufLen);
            return -1;
        }

        ret = IspCfgDecrypt(pCfgXml, pDecodeBuf, decodeBufLen);
        if (ret != 0)
        {
            free(pDecodeBuf);
            free(pCfgXml);
            close(fd);
            DBG("IspCfgDecrypt error %d\n", ret);
            return -1;
        }

        pDocNode = ixmlParseBuffer(pDecodeBuf);
        free(pDecodeBuf);
*/
    }
    else
    {
        pDocNode = ixmlParseBuffer(pCfgXml);
    }

    free(pCfgXml);
    close(fd);

    CHECK(pDocNode != NULL, -1, "ixmlParseBuffer error\r\n");


    IXML_NodeList* pNodelist = ixmlDocument_getElementsByTagName(pDocNode, (char*)"ISPCONFIG");
    if(pNodelist != NULL)
    {
        IXML_Node* pNode = pNodelist->nodeItem->firstChild;
        while(pNode != NULL)
        {
            if(gp_isp_config->last_mode == SAL_ISP_MODE_DAY && !strcmp(pNode->nodeName, "ColorConfig"))
            {
                ret = process_all(pNode);
                CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
            }
            else if(gp_isp_config->last_mode == SAL_ISP_MODE_NIGHT && !strcmp(pNode->nodeName, "BWConfig"))
            {
                ret = process_all(pNode);
                CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);
            }
            else
            {
                WRN("isp config not support %s\n", pNode->nodeName);
            }
            pNode = pNode->nextSibling;
        }

        ixmlNodeList_free(pNodelist);
        ixmlDocument_free(pDocNode);
    }
    else
    {
        ERR("get isp config failed!\n");
        ixmlDocument_free(pDocNode);
        return HI_FAILURE;
    }

    DBG("get isp config success, file: %s, mode: %s\n", path, (gp_isp_config->last_mode == SAL_ISP_MODE_DAY) ? "Color" : "BlackWhite");
    isp_saturation_set(image_attr->saturation);
    isp_contrast_set(image_attr->contrast);
    isp_brightness_set(image_attr->brightness);
    isp_sharpness_set(image_attr->sharpness);

    DBG("set isp config success!\n");

    return ret;
}

static ISO_LEVEL iso_level_get(HI_S32 iso)
{
    ISO_LEVEL level;

    //iso最低100，所以最低档为100 ~200,不包含200
    if(iso < 200)
    {
        level = ISO_LEVEL_0;
    }
    else if(iso < 400)
    {
        level = ISO_LEVEL_1;
    }
    else if(iso < 800)
    {
        level = ISO_LEVEL_2;
    }
    else if(iso < 1600)
    {
        level = ISO_LEVEL_3;
    }
    else if(iso < 3200)
    {
        level = ISO_LEVEL_4;
    }
    else if(iso < 6400)
    {
        level = ISO_LEVEL_5;
    }
    else if(iso < 12800)
    {
        level = ISO_LEVEL_6;
    }
    else if(iso < 25600)
    {
        level = ISO_LEVEL_7;
    }
    else if(iso < 51200)
    {
        level = ISO_LEVEL_8;
    }
    else if(iso < 102400)
    {
        level = ISO_LEVEL_9;
    }
    else if(iso < 204800)
    {
        level = ISO_LEVEL_10;
    }
    else if(iso < 409600)
    {
        level = ISO_LEVEL_11;
    }
    else if(iso < 819200)
    {
        level = ISO_LEVEL_12;
    }
    else if(iso < 1638400)
    {
        level = ISO_LEVEL_13;
    }
    else if(iso < 3276800)
    {
        level = ISO_LEVEL_14;
    }
    else
    {
        level = ISO_LEVEL_15;
    }

    return level;
}

static int isp_skin_reg_set()
{
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    ISP_REG_ATTR_S stRegAttr;
    memset(&stRegAttr, 0, sizeof(stRegAttr));
    s32Ret = HI_MPI_ISP_GetISPRegAttr(IspDev, &stRegAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", s32Ret);

    char* addr = (char*)(stRegAttr.u32AwbExtRegAddr + 0xf8);
    //DBG("old: %#x.\n", *addr);

    if (!access("/mnt/nand/close_skin.flag", F_OK))
    {
        (*addr) &= ~(1 << 0);
    }

    //DBG("new: %#x.\n", *addr);

    return HI_SUCCESS;
}

static int isp_abstime_get(struct timeval* timeval)
{
    int ret = -1;
    struct timespec tp;
    ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    CHECK(ret == 0, -1, "Error with %s\n", strerror(errno));

    timeval->tv_sec = tp.tv_sec;
    timeval->tv_usec = tp.tv_nsec / 1000;

    return 0;
}

static int isp_get_fps()
{
    int i = 0;
    int level_cnt = sizeof(gp_isp_config->frame_level)/sizeof(*gp_isp_config->frame_level);
    int max_fps = gp_isp_config->max_fps[0];
    int min_fps = 1000*1000/gp_isp_config->stExpAttr.stAuto.stExpTimeRange.u32Max;
    double threshold = gp_isp_config->stExpAttr.stAuto.u32GainThreshold/1024.0;

/*
    if (gp_isp_config->advance_param.sl_bEnable)
    {
        min_fps = 1000*1000/gp_isp_config->stSLExpAttr.stAuto.stExpTimeRange.u32Max;
        threshold = gp_isp_config->stSLExpAttr.stAuto.u32GainThreshold/1024.0;
    }
*/
    if (min_fps <= 0)
    {
        min_fps = 5;
    }

    int frame_level = max_fps;
    for (i = 0; i < level_cnt; i++)
    {
        gp_isp_config->frame_level[i] = frame_level;
        frame_level -= 30;
        if (gp_isp_config->frame_level[i] < min_fps)
        {
            gp_isp_config->frame_level[i] = min_fps;
        }
        //DBG("min_fps: %d, frame_level: %d, frame_level[%d]: %d\n", min_fps, frame_level, i, gp_isp_config->frame_level[i]);
    }

    double sys_gain = (gp_isp_config->stExpInfo.u32AGain/1024.0) * (gp_isp_config->stExpInfo.u32DGain/1024.0) * (gp_isp_config->stExpInfo.u32ISPDGain/1024.0);
    int fps = gp_isp_config->last_fps;
    double up_threshold = 0.0;
    if (sys_gain >= threshold)
    {
        for (i = 0; i < level_cnt; i++)
        {
            if (fps > gp_isp_config->frame_level[i])
            {
                fps = gp_isp_config->frame_level[i];
                break;
            }
        }
    }
    else
    {
        for (i = level_cnt-1; i >= 0; i--)
        {
            if (i - 1 >= 0)
            {
                //通过帧率和降帧阈值，计算等价曝光量的升帧gain，以防震荡再减去1得到升帧阈值
                up_threshold = ((1000.0/gp_isp_config->frame_level[i-1])*threshold) / (1000.0/gp_isp_config->frame_level[i]) - 1.0;
            }
            if (sys_gain <= up_threshold && fps < gp_isp_config->frame_level[i])
            {
                fps = gp_isp_config->frame_level[i];
                break;
            }
        }
    }
    if (fps < min_fps)
    {
        fps = min_fps;
    }
    if (fps > max_fps)
    {
        fps = max_fps;
    }

    //DBG("sys_gain: %f, threshold: %f, up_threshold: %f, fps: %d\n", sys_gain, threshold, up_threshold, fps);

    return fps;
}

static int isp_slow_shutter()
{
    int ret = -1;
    int channel = 0;

    int fps = isp_get_fps();
    if (fps == gp_isp_config->last_fps)
    {
        ret = isp_abstime_get(&gp_isp_config->last_level_time);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);
        return 0;
    }

    struct timeval cur = {0, 0};
    ret = isp_abstime_get(&cur);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    int pass_time = (cur.tv_sec - gp_isp_config->last_level_time.tv_sec) * 1000 + (cur.tv_usec - gp_isp_config->last_level_time.tv_usec) / 1000;
    DBG("last_fps: %d, fps: %d, pass time: %d ms\n", gp_isp_config->last_fps, fps, pass_time);
    if (pass_time >= gp_isp_config->delay_time)
    {
        gp_isp_config->last_fps = fps;
        for (channel = 0; channel < 2; channel++)
        {
            int dst_fps = (channel == 0) ? fps : gp_isp_config->max_fps[channel];

            if (channel == 0)
            {
                ret = sal_video_vi_framerate_set(dst_fps);
                CHECK(ret == 0, -1, "Error with %#x.\n", ret);
            }

            if (dst_fps > gp_isp_config->max_fps_cfg[channel])
            {
                dst_fps = gp_isp_config->max_fps_cfg[channel];
            }

            ret = sal_video_framerate_set(channel, dst_fps);
            CHECK(ret == 0, -1, "Error with %#x.\n", ret);

/*
            int min_fps = 1000*1000/gp_isp_config->stSLExpAttr.stAuto.stExpTimeRange.u32Max;
            if (!gp_isp_config->vbr[channel] && dst_fps == min_fps)
            {
                ret = sal_video_bitrate_set(channel, gp_isp_config->vbr[channel], gp_isp_config->bitrate[channel]*0.6);
                CHECK(ret == 0, -1, "Error with %#x.\n", ret);
            }
            else if (!gp_isp_config->vbr[channel])
            {
                ret = sal_video_bitrate_set(channel, gp_isp_config->vbr[channel], gp_isp_config->bitrate[channel]);
                CHECK(ret == 0, -1, "Error with %#x.\n", ret);
            }
*/
        }

        ret = isp_abstime_get(&gp_isp_config->last_level_time);
        CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    }

    return ret;
}

/*
static int isp_switch_framerate(int framerate)
{
    int ret = 0;
    int channel = 0;

    ret = sal_video_vi_framerate_set(framerate);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    for (channel = 0; channel < 2; channel++)
    {
       int dst_fps = (channel == 0) ? framerate : gp_isp_config->max_fps[channel];

       if (dst_fps > gp_isp_config->max_fps_cfg[channel])
       {
          dst_fps = gp_isp_config->max_fps_cfg[channel];
       }

       ret = sal_video_framerate_set(channel, dst_fps);
       CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    }

    return HI_SUCCESS;
}
*/

static void *isp_thread(void *arg)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    HI_S32 s32Ret = HI_SUCCESS;
    ISP_DEV IspDev = 0;
    HI_S32 iso = 0;
    ISO_LEVEL iso_level = ISO_LEVEL_BUTT;
    GAMMA_LUMA_STATE gamma_luma_state = GAMMA_LUMA_STATE_BUTT;
    int mode = -1;
    float tmp;
    sleep(1); // 需等ISP模块稳定后在设置IQ参数

    while (gp_isp_config->running)
    {
        pthread_mutex_lock(&gp_isp_config->mutex);
        s32Ret = HI_MPI_ISP_QueryExposureInfo(IspDev, &gp_isp_config->stExpInfo);
        CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

        tmp = (gp_isp_config->stExpInfo.u32AGain / 1024.0) * (gp_isp_config->stExpInfo.u32DGain / 1024.0) * (gp_isp_config->stExpInfo.u32ISPDGain / 1024.0);
        iso = tmp * 100;
        iso_level = iso_level_get(iso);

        if (mode != gp_isp_config->last_mode)
        {
            mode = gp_isp_config->last_mode;

            s32Ret = isp_day_night_switch();
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            //白天和晚上切换时需要重新更新自己设计的联动参数
            gp_isp_config->last_iso_level = iso_level;
            gp_isp_config->last_iso = iso;
            s32Ret = set_drc_config(iso_level,  iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

/*
            if (ENC_SAL_DEFOG_MODE_AUTO == gp_isp_config->advance_param.DefogMode)
            {
                s32Ret = set_defog_config(iso_level,iso);
                CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
            }
*/

            s32Ret = set_3dnr_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            s32Ret = set_blc_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            if(tmp <= (float)gp_isp_config->GammaSysgainThreshold[GAMMA_LUMA_STATE_HIGH])
            {
                gamma_luma_state = GAMMA_LUMA_STATE_HIGH;
            }
            else if(tmp <= (float)gp_isp_config->GammaSysgainThreshold[GAMMA_LUMA_STATE_MIDDLE])
            {
                gamma_luma_state = GAMMA_LUMA_STATE_MIDDLE;
            }
            else
            {
                gamma_luma_state = GAMMA_LUMA_STATE_LOW;
            }
            gp_isp_config->last_gamma_state = gamma_luma_state;
            s32Ret = set_gamma_config(gamma_luma_state);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            //日夜切换需更新高级参数
            //gp_isp_config->bUpdate = 1;
        }

        if (iso_level != gp_isp_config->last_iso_level)
        {
            gp_isp_config->last_iso_level = iso_level;
            gp_isp_config->last_iso       = iso;
            s32Ret = set_drc_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

/*
            if (ENC_SAL_DEFOG_MODE_AUTO == gp_isp_config->advance_param.DefogMode)
            {
                s32Ret = set_defog_config(iso_level,iso);
                CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
            }
*/

            s32Ret = set_3dnr_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            s32Ret = set_blc_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
        }
        //iso 变化50以上就重新更新需要线性变化的参数
        else if (abs(iso - gp_isp_config->last_iso) >= 50)
        {
            //DBG("3dnr iso change new:%d, old:%d\n", iso, gp_isp_config->last_iso);
            gp_isp_config->last_iso = iso;

            s32Ret = set_drc_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

            s32Ret = set_3dnr_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);

/*
            if (ENC_SAL_DEFOG_MODE_AUTO == gp_isp_config->advance_param.DefogMode)
            {
                s32Ret = set_defog_config(iso_level,iso);
                CHECK_UNLOCK(s32Ret == HI_SUCCESS, NULL, &gp_isp_config->mutex, "Error with %#x.\n", s32Ret);
            }
*/

            s32Ret = set_blc_config(iso_level, iso);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
        }

        if (tmp <= (float)gp_isp_config->GammaSysgainThreshold[GAMMA_LUMA_STATE_HIGH])
        {
            gamma_luma_state = GAMMA_LUMA_STATE_HIGH;
        }
        else if (tmp <= (float)gp_isp_config->GammaSysgainThreshold[GAMMA_LUMA_STATE_MIDDLE])
        {
            gamma_luma_state = GAMMA_LUMA_STATE_MIDDLE;
        }
        else
        {
            gamma_luma_state = GAMMA_LUMA_STATE_LOW;
        }

        if (gamma_luma_state != gp_isp_config->last_gamma_state)
        {
            gp_isp_config->last_gamma_state = gamma_luma_state;
            DBG("switch gamma config level: %d\n", gamma_luma_state);
            s32Ret = set_gamma_config(gamma_luma_state);
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
        }

        if (gp_isp_config->bSlowShutter)
        {
            s32Ret = isp_slow_shutter();
            CHECK(s32Ret == HI_SUCCESS || pthread_mutex_unlock(&gp_isp_config->mutex), NULL, "Error with %#x.\n", s32Ret);
        }

        pthread_mutex_unlock(&gp_isp_config->mutex);
        usleep(50*1000);
    }

    return NULL;
}

int sal_isp_day_night_switch(SAL_ISP_MODE_E day_night_mode, sal_image_attr_s* image_attr)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");
    CHECK(NULL != image_attr, HI_FAILURE, "null ptr error.\n");

    pthread_mutex_lock(&gp_isp_config->mutex);

    gp_isp_config->last_mode = day_night_mode;
    memcpy(&gp_isp_config->image_attr, image_attr, sizeof(sal_image_attr_s));
    pthread_mutex_unlock(&gp_isp_config->mutex);

    return HI_SUCCESS;
}

int sal_isp_init(sal_isp_s* isp_param)
{
    CHECK(NULL == gp_isp_config, HI_FAILURE, "reinit error, please uninit first.\n");
    int ret = -1;

    gp_isp_config = (ISP_CONFIG*)malloc(sizeof(ISP_CONFIG));
    if (!gp_isp_config)
    {
        DBG("malloc %d bytes failed.\n", sizeof(ISP_CONFIG));
        return HI_FAILURE;
    }
    memset(gp_isp_config, 0, sizeof(ISP_CONFIG));

    gp_isp_config->last_mode = SAL_ISP_MODE_DAY;
    memcpy(&gp_isp_config->image_attr, &isp_param->image_attr, sizeof(sal_image_attr_s));
    pthread_mutex_init(&gp_isp_config->mutex, NULL);

    gp_isp_config->bSlowShutter = isp_param->slow_shutter_enable;
    sal_stream_s video;
    memset(&video, 0, sizeof(video));

    //gp_isp_config->ldc_support = isp_param->ldc_support;

    ret = sal_video_args_get(0, &video);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);

    gp_isp_config->delay_time = 2000;
    gp_isp_config->last_fps = video.framerate;
    gp_isp_config->max_fps[0] = video.framerate;

/*
    memcpy(gp_isp_config->vbr, isp_param->vbr, sizeof(gp_isp_config->vbr));
    memcpy(gp_isp_config->bitrate, isp_param->bitrate, sizeof(gp_isp_config->bitrate));
*/

/*
    ret = sal_video_args_get(1, &video);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    gp_isp_config->max_fps[1] = video.framerate;
*/

    gp_isp_config->max_fps_cfg[0] = gp_isp_config->max_fps[0];
    gp_isp_config->max_fps_cfg[1] = gp_isp_config->max_fps[1];

    gp_isp_config->bSlowShutter = 0;
    // for test
    if (!access("/mnt/nand/slow_shutter.flag", F_OK)
        || !access("/opt/topsee/slow_shutter.flag", F_OK))
    {
        gp_isp_config->bSlowShutter = 1;
    }

/*
    ret = isp_set_default_advance_param(&gp_isp_config->advance_default_param);
    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
    memcpy(&gp_isp_config->advance_param, &isp_param->advance_param, sizeof(gp_isp_config->advance_param));
    gp_isp_config->bUpdate = 1;
*/

    gp_isp_config->sensor_type = SENSOR_TYPE;
    gp_isp_config->running = 1;
    ret = pthread_create(&gp_isp_config->pid, NULL, isp_thread, NULL);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x\n", ret);

    isp_skin_reg_set();

    return HI_SUCCESS;
}

int sal_isp_exit(void)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");

    if (gp_isp_config->running)
    {
        gp_isp_config->running = 0;
        pthread_join(gp_isp_config->pid, NULL);
    }

    pthread_mutex_destroy(&gp_isp_config->mutex);

    free(gp_isp_config);
    gp_isp_config = NULL;
    DBG("done.\n");

    return HI_SUCCESS;
}

int sal_isp_image_attr_set(SAL_IMAGE_E type, unsigned char value)
{
    pthread_mutex_lock(&gp_isp_config->mutex);

    int ret = HI_FAILURE;
    switch(type)
    {
        case SAL_IMAGE_SATURATION:
            //夜晚模式设置饱和度不生效
            if(gp_isp_config->last_mode != SAL_ISP_MODE_NIGHT)
            {
                ret = isp_saturation_set(value);
            }
            break;
        case SAL_IMAGE_CONTRAST:
            ret = isp_contrast_set(value);
            break;
        case SAL_IMAGE_BRIGHTNESS:
            ret = isp_brightness_set(value);
            break;
        case SAL_IMAGE_SHARPNESS:
            ret = isp_sharpness_set(value);
            break;
        default:
            WRN("unknown type %d\n", type);
            break;
    }

    pthread_mutex_unlock(&gp_isp_config->mutex);

    return ret;
}

int sal_isp_slow_shutter_set(int enable)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");

    //暂时不支持上层设置降帧
    //gp_isp_config->bSlowShutter = enable;

    return HI_SUCCESS;
}

int sal_isp_exposure_info_get(sal_exposure_info_s* info)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");
    CHECK(NULL != info, HI_FAILURE, "info is NULL.\n");
    ISP_DEV IspDev = 0;
    ISP_WB_INFO_S stWBInfo;
    int ret;

    ret = HI_MPI_ISP_QueryWBInfo(IspDev, &stWBInfo);
    CHECK(ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", ret);

    info->AGain = gp_isp_config->stExpInfo.u32AGain;
    info->DGain = gp_isp_config->stExpInfo.u32DGain;
    info->ISPGain = gp_isp_config->stExpInfo.u32ISPDGain;
    info->ExpTime = gp_isp_config->stExpInfo.u32ExpTime;
    info->AveLuma = gp_isp_config->stExpInfo.u8AveLum;
    info->ColorTemp = stWBInfo.u16ColorTemp;

    return HI_SUCCESS;
}

/*
int sal_isp_advanced_attr_set(sal_isp_advance_s* advance_param)
{
    CHECK(NULL != gp_isp_config, HI_FAILURE, "isp is not inited.\n");
    CHECK(NULL != advance_param, HI_FAILURE, "advance_param is NULL.\n");

    pthread_mutex_lock(&gp_isp_config->mutex);

    memcpy(&gp_isp_config->advance_param, advance_param, sizeof(*advance_param));
    gp_isp_config->bUpdate = 1;

    pthread_mutex_unlock(&gp_isp_config->mutex);

    return 0;
}
*/



