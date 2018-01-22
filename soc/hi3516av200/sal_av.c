#include "sal_av.h"
#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_af.h"
#include "sal_config.h"
#include "sal_statistics.h"
//#include "sal_special.h"

#define FILE_WRITE_TEST 0
#define PIPE_WRITE_TEST 0

#define SMART_P_MAX_IDRGOP_SECONDS  (40)


///////////////////////////////
typedef struct sal_av_args
{
    SAMPLE_VI_MODE_E sensor_type;
    PIXEL_FORMAT_E pixel_fmt;
    sal_video_s video;
    int video_chn_num;
    int vi_width;
    int vi_height;
    int vi_fps;

    //ext chn attr. only use for scale
    int scale_enable; //different main resolution crop or scale enable
    int ext_chn_enable;
    int ext_chn_number;
    int ext_chn_width;
    int ext_chn_height;

    int smartP_enable;
    int gop_times; // smartp gop放大倍数

    // 根据相应驱动ko插入的参数指定
    int lowdelay_enable;  // 低延迟模式
    int onebuffer_enable; // 单vb模式，需结合低延迟模式才有效
    int onestream_enable;//单包模式/多包模式
    char* multiple_buffer; //多包模式下需要使用buffer来拼合一个帧

    int running;
    pthread_t tid;
    pthread_t isp_tid;
    pthread_mutex_t mutex;
}
sal_av_args;

static sal_av_args* g_av_args = NULL;

static VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE =
{
    /* interface mode */
    VI_MODE_MIPI,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            720,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {0, 0, 1920, 1080}
};

static VI_DEV_ATTR_EX_S DEV_ATTR_LVDS_BASE_EX =
{
    /* interface mode */
    VI_INPUT_MODE_LVDS,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    VI_COMBINE_COMPOSITE,
    VI_COMP_MODE_SINGLE,
    VI_CLK_EDGE_SINGLE_DOWN,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },

    {BT656_FIXCODE_1, BT656_FIELD_POLAR_STD},
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {0, 30, 1920, 1080}
};

static VI_DEV_ATTR_S DEV_ATTR_LVDS_BASE =
{
    /* interface mode */
    VI_MODE_LVDS,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
        {0xFFF00000,    0x0},
        /* progessive or interleaving */
        VI_SCAN_PROGRESSIVE,
        /*AdChnId*/
        { -1, -1, -1, -1},
        /*enDataSeq, only support yuv*/
        VI_INPUT_DATA_YUYV,

        /* synchronization information */
        {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

            /*hsync_hfb    hsync_act    hsync_hhb*/
            {
            0,            1280,        0,
                /*vsync0_vhb vsync0_act vsync0_hhb*/
                0,            720,        0,
                /*vsync1_vhb vsync1_act vsync1_hhb*/
                0,            0,            0
            }
        },
            /* use interior ISP */
            VI_PATH_ISP,
            /* input data type */
            VI_DATA_TYPE_RGB,
            /* bRever */
            HI_FALSE,
            /* DEV CROP */
            {0, 0, 1920, 1080},
            {
                {
                    {1920, 1080},
                        HI_FALSE,

                },
                {
                VI_REPHASE_MODE_NONE,
                    VI_REPHASE_MODE_NONE
                    }
            }
};

/*9M034 DC 12bit输入720P@30fps*/
static VI_DEV_ATTR_S DEV_ATTR_AR0130_DC_720P_BASE =
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF0000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {370,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     6,            720,        6,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB,
    /* Data Reverse */
    HI_FALSE,
    {0, 0, 1280, 720}
};

/* DC 12bit输入*/
VI_DEV_ATTR_S DEV_ATTR_DC_BASE =
{
    /* interface mode */
    VI_MODE_DIGITAL_CAMERA,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF0000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

     /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,
    /* bRevert */
    HI_FALSE,
    /* stDevRect */
    {200, 20, 1920, 1080}
};

static VI_DEV_ATTR_EX_S DEV_ATTR_DC_BASE_EX =
{
    /* interface mode */
    VI_INPUT_MODE_DIGITAL_CAMERA,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    VI_COMBINE_COMPOSITE,
    VI_COMP_MODE_SINGLE,
    VI_CLK_EDGE_SINGLE_DOWN,
    /* r_mask    g_mask    b_mask*/
    {0xFFF0000,    0x0},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, only support yuv*/
    VI_INPUT_DATA_YUYV,

    /* synchronization information */
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,

    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1920,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            1080,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/
     0,            0,            0}
    },

    {BT656_FIXCODE_1, BT656_FIELD_POLAR_STD},
    /* use interior ISP */
    VI_PATH_ISP,
    /* input data type */
    VI_DATA_TYPE_RGB,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {0, 0, 1920, 1080}
};

combo_dev_attr_t LVDS_10lane_SENSOR_IMX226_12BIT_12M_NOWDR_ATTR = 
    {
    .devno = 0,
    .input_mode = INPUT_MODE_LVDS,
    .phy_clk_share = PHY_CLK_SHARE_PHY0,
    .img_rect = {132, 40, 4000, 3000},
    .lvds_attr = 
        {
        .raw_data_type    = RAW_DATA_12BIT,
        .wdr_mode         = HI_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SAV,
        .vsync_type       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_type         = {LVDS_FID_NONE, HI_TRUE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
        //.lane_id = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1},
        .lane_id = {0, 1, 2, 4, 5, 7, 8, 9, 3, 6, -1, -1},
        .sync_code = 
            {
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane 0
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane 1
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane2
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane3
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane4
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane5
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane6
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane7
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane8
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
                {{0xab0, 0xb60, 0x800, 0x9d0},      // lane9
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}, 
                {0xab0, 0xb60, 0x800, 0x9d0}},
            }
        }
    };

static int sys_vb_size(int width, int height, int align_width)
{
    int size = CEILING_2_POWER(width, align_width) * CEILING_2_POWER(height, align_width) * 1.5;
    int head_size = 0;
    VB_PIC_HEADER_SIZE(width, height, g_av_args->pixel_fmt, head_size);
    size += head_size;

    return size;
}

static int sys_init(VB_CONF_S *pstVbConf, int align_width)
{
    MPP_SYS_CONF_S stSysConf = {0};
    HI_S32 s32Ret = HI_FAILURE;

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    s32Ret = HI_MPI_VB_SetConf(pstVbConf);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VB_Init();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stSysConf.u32AlignWidth = align_width;
    s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_SYS_Init();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int sys_vb_init()
{
    int s32Ret = 0;

    VB_CONF_S stVbConf;
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    stVbConf.u32MaxPoolCnt = 128;
    int align_width = 64;

    /*video buffer*/
    int i = 0;
    for (i = 0; i < g_av_args->video_chn_num; i++)
    {
        if (g_av_args->video.stream[i].enable)
        {
            sal_stream_s* stream = &g_av_args->video.stream[i];
            stVbConf.astCommPool[i].u32BlkSize = sys_vb_size(stream->width, stream->height, align_width);
            stVbConf.astCommPool[i].u32BlkCnt = 10;

            if (!g_av_args->ext_chn_enable && g_av_args->video.rotate)
                stVbConf.astCommPool[i].u32BlkCnt += 2;
        }
        // 单buffer模式
        if (0 == i && g_av_args->onebuffer_enable)
        {
            stVbConf.astCommPool[i].u32BlkCnt = 1;
        }
    }
    if (g_av_args->ext_chn_enable)
    {
        stVbConf.astCommPool[i].u32BlkSize = sys_vb_size(g_av_args->vi_width, g_av_args->vi_height, align_width);
        stVbConf.astCommPool[i].u32BlkCnt = 5;
        i++;
    }

    if (g_config.md.enable)
    {
        stVbConf.astCommPool[i].u32BlkSize = sys_vb_size(g_config.md.widht, g_config.md.height, align_width);
        stVbConf.astCommPool[i].u32BlkCnt = (g_av_args->video.rotate) ? 2 : 1;
        i++;
    }
    if (g_config.jpeg.enable)
    {
        stVbConf.astCommPool[i].u32BlkSize = sys_vb_size(g_config.jpeg.widht, g_config.jpeg.height, align_width);
        stVbConf.astCommPool[i].u32BlkCnt = (g_av_args->video.rotate) ? 2 : 1;
        i++;
    }

    s32Ret = sys_init(&stVbConf, align_width);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int vi_set_mipi(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_S32 fd;
    combo_dev_attr_t stcomboDevAttr;
    memset(&stcomboDevAttr, 0, sizeof(stcomboDevAttr));

    /* mipi reset unrest */
    fd = open("/dev/hi_mipi", O_RDWR);
    CHECK(fd >= 0, HI_FAILURE, "failed to open hi_mipi dev[/dev/hi_mipi]: %s.\n", strerror(errno));

    if (pstViConfig->enViMode == OMNIVISION_OV9732_MIPI_720P_30FPS
        || pstViConfig->enViMode == OMNIVISION_OV2710_MIPI_1080P_30FPS)
    {
        stcomboDevAttr.input_mode = INPUT_MODE_MIPI;
        stcomboDevAttr.mipi_attr.raw_data_type = RAW_DATA_12BIT;
        short lane_id[MIPI_LANE_NUM] = {0, -1, -1, -1, -1, -1, -1, -1};
        memcpy(stcomboDevAttr.mipi_attr.lane_id, lane_id, sizeof(lane_id));
    }
    //else if (pstViConfig->enViMode == APTINA_AR0130_DC_720P_30FPS
    //    || pstViConfig->enViMode == SONY_IMX323_DC_1080P_30FPS
    //    || pstViConfig->enViMode == SMARTSENS_SC1135_DC_960P_30FPS
    //    || pstViConfig->enViMode == APTINA_AR0237_DC_1080P_30FPS
    //    || pstViConfig->enViMode == SMARTSENS_SC2135_DC_1080P_30FPS)
    //{
    //    memset(&stcomboDevAttr, 0, sizeof(stcomboDevAttr));
    //    stcomboDevAttr.input_mode = INPUT_MODE_CMOS_33V;
    //}
    else if (pstViConfig->enViMode == SONY_IMX291_LVDS_1080P_30FPS)
    {
        stcomboDevAttr.input_mode = INPUT_MODE_SUBLVDS;
        stcomboDevAttr.lvds_attr.raw_data_type = RAW_DATA_12BIT;
        stcomboDevAttr.lvds_attr.sync_mode = LVDS_SYNC_MODE_SAV;
        stcomboDevAttr.lvds_attr.sync_code_endian = LVDS_ENDIAN_BIG;
        stcomboDevAttr.lvds_attr.data_endian = LVDS_ENDIAN_BIG;
        //stcomboDevAttr.lvds_attr.img_size.width = g_av_args->vi_width;
        //stcomboDevAttr.lvds_attr.img_size.height = g_av_args->vi_height;

        short lane_id[LVDS_LANE_NUM]= {0, 1, 2, 3, -1, -1, -1, -1};
        memcpy(stcomboDevAttr.lvds_attr.lane_id, lane_id, sizeof(lane_id));

        unsigned short sync_code[LVDS_LANE_NUM][WDR_VC_NUM][SYNC_CODE_NUM] =
        {
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
            {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        };
        memcpy(stcomboDevAttr.lvds_attr.sync_code, sync_code, sizeof(sync_code));
    }
    else if (pstViConfig->enViMode == SONY_IMX226_LVDS_12M_30FPS)
    {
        //stcomboDevAttr.input_mode = INPUT_MODE_LVDS;
        //stcomboDevAttr.lvds_attr.raw_data_type = RAW_DATA_12BIT;
        //stcomboDevAttr.lvds_attr.sync_mode = LVDS_SYNC_MODE_SAV;
        //stcomboDevAttr.lvds_attr.sync_code_endian = LVDS_ENDIAN_BIG;
        //stcomboDevAttr.lvds_attr.data_endian = LVDS_ENDIAN_BIG;
        ////stcomboDevAttr.lvds_attr.img_size.width = g_av_args->vi_width;
        ////stcomboDevAttr.lvds_attr.img_size.height = g_av_args->vi_height;

        //short lane_id[LVDS_LANE_NUM]= {0, 1, 2, 3, -1, -1, -1, -1};
        //memcpy(stcomboDevAttr.lvds_attr.lane_id, lane_id, sizeof(lane_id));

        //unsigned short sync_code[LVDS_LANE_NUM][WDR_VC_NUM][SYNC_CODE_NUM] =
        //    {
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //        {{0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}, {0xab0, 0xb60, 0x800, 0x9d0}},
        //    };
        //memcpy(stcomboDevAttr.lvds_attr.sync_code, sync_code, sizeof(sync_code));
        memcpy(&stcomboDevAttr, &LVDS_10lane_SENSOR_IMX226_12BIT_12M_NOWDR_ATTR, sizeof(stcomboDevAttr));
    }
    else
    {
        DBG("Unknown sensor type: %d.\n", pstViConfig->enViMode);
        return HI_FAILURE;
    }
    
    /* 1.reset mipi */
    ioctl(fd, HI_MIPI_RESET_MIPI, &stcomboDevAttr.devno);

    /* 2.reset sensor */
    ioctl(fd, HI_MIPI_RESET_SENSOR, &stcomboDevAttr.devno);
    
    /* 3.set mipi attr */
    if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, &stcomboDevAttr) < 0)
    {
        DBG("set mipi attr failed: %s\n", strerror(errno));
        close(fd);
        return HI_FAILURE;
    }
    
    usleep(10000);
    
    /* 4.unreset mipi */
    ioctl(fd, HI_MIPI_UNRESET_MIPI, &stcomboDevAttr.devno);

    /* 5.unreset sensor */
    ioctl(fd, HI_MIPI_UNRESET_SENSOR, &stcomboDevAttr.devno);

    //if ((pstViConfig->enViMode == SONY_IMX117_LVDS_4K_30FPS) || (pstViConfig->enViMode == SONY_IMX117_LVDS_12M_28FPS))
    //{
    //    phy_cmv_t cmv;
    //    cmv.devno = stcomboDevAttr.devno;
    //    cmv.cmv_mode = PHY_CMV_LT900MV;
    //    if (ioctl(fd, HI_MIPI_SET_PHY_CMVMODE, &cmv))
    //        {
    //        printf("set mipi phy common voltage mode failed\n");
    //        close(fd);
    //        return HI_FAILURE;
    //        }    
    //}
    close(fd);

    return HI_SUCCESS;
}

static int vi_set_dev(VI_DEV ViDev, SAMPLE_VI_MODE_E enViMode)
{
    HI_S32 s32Ret;
    HI_S32 s32IspDev = 0;
    ISP_WDR_MODE_S stWdrMode;

    VI_DEV_ATTR_S  stViDevAttr;

    HI_BOOL bATTR_EX = HI_FALSE;
    VI_DEV_ATTR_EX_S  stViDevAttrEx;

    memset(&stViDevAttr,0,sizeof(stViDevAttr));
    memset(&stViDevAttrEx,0,sizeof(stViDevAttrEx));

    if (g_av_args->sensor_type == APTINA_AR0130_DC_720P_30FPS)
    {
        memcpy(&stViDevAttr,&DEV_ATTR_AR0130_DC_720P_BASE,sizeof(stViDevAttr));
    }
    else if (g_av_args->sensor_type == OMNIVISION_OV9732_MIPI_720P_30FPS
            || g_av_args->sensor_type == OMNIVISION_OV2710_MIPI_1080P_30FPS)
    {
        memcpy(&stViDevAttr,&DEV_ATTR_MIPI_BASE,sizeof(stViDevAttr));
    }
    else if (g_av_args->sensor_type == SONY_IMX291_LVDS_1080P_30FPS)
    {
        bATTR_EX = HI_TRUE;
        memcpy(&stViDevAttrEx, &DEV_ATTR_LVDS_BASE_EX, sizeof(stViDevAttrEx));
    }
    else if (g_av_args->sensor_type == SONY_IMX323_DC_1080P_30FPS)
    {
        memcpy(&stViDevAttr,&DEV_ATTR_DC_BASE, sizeof(stViDevAttr));
    }
    else if ((g_av_args->sensor_type == SMARTSENS_SC1135_DC_960P_30FPS)
            || (g_av_args->sensor_type == SMARTSENS_SC2135_DC_1080P_30FPS))
    {
        bATTR_EX = HI_TRUE;
        memcpy(&stViDevAttrEx,&DEV_ATTR_DC_BASE_EX, sizeof(stViDevAttrEx));
        stViDevAttrEx.stSynCfg.enVsyncValid = VI_VSYNC_NORM_PULSE;
    }
    else if (g_av_args->sensor_type == APTINA_AR0237_DC_1080P_30FPS)
    {
        bATTR_EX = HI_TRUE;
        memcpy(&stViDevAttrEx,&DEV_ATTR_DC_BASE_EX, sizeof(stViDevAttrEx));
    }
    else if (g_av_args->sensor_type == SONY_IMX226_LVDS_12M_30FPS)
    {
        memcpy(&stViDevAttr, &DEV_ATTR_LVDS_BASE, sizeof(stViDevAttr));
    }
    else
    {
        DBG("Unknown sensor %d.\n", g_av_args->sensor_type);
        return HI_FAILURE;
    }

    if (bATTR_EX)
    {
        stViDevAttrEx.stSynCfg.stTimingBlank.u32HsyncAct = g_av_args->vi_width;
        stViDevAttrEx.stSynCfg.stTimingBlank.u32VsyncVbact = g_av_args->vi_height;
        stViDevAttrEx.stDevRect.u32Width  = g_av_args->vi_width;
        stViDevAttrEx.stDevRect.u32Height = g_av_args->vi_height;

        s32Ret = HI_MPI_VI_SetDevAttrEx(ViDev, &stViDevAttrEx);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else
    {
        /*stViDevAttr.stSynCfg.stTimingBlank.u32HsyncAct = g_av_args->vi_width;
        stViDevAttr.stSynCfg.stTimingBlank.u32VsyncVbact = g_av_args->vi_height;*/
        
        stViDevAttr.stDevRect.s32X = 0;
        stViDevAttr.stDevRect.s32Y = 0;
        stViDevAttr.stDevRect.u32Width  = g_av_args->vi_width;
        stViDevAttr.stDevRect.u32Height = g_av_args->vi_height;
        stViDevAttr.stBasAttr.stSacleAttr.stBasSize.u32Width  = g_av_args->vi_width;
        stViDevAttr.stBasAttr.stSacleAttr.stBasSize.u32Height = g_av_args->vi_height;
        stViDevAttr.stBasAttr.stSacleAttr.bCompress = HI_FALSE;

        s32Ret = HI_MPI_VI_SetDevAttr(ViDev, &stViDevAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if ((SAMPLE_VI_MODE_BT1120_1080P != enViMode)
        &&(SAMPLE_VI_MODE_BT1120_720P != enViMode))
    {
        s32Ret = HI_MPI_ISP_GetWDRMode(s32IspDev, &stWdrMode);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        if (stWdrMode.enWDRMode)  //wdr mode
        {
            VI_WDR_ATTR_S stWdrAttr;

            stWdrAttr.enWDRMode = stWdrMode.enWDRMode;
            stWdrAttr.bCompress = HI_FALSE;

            s32Ret = HI_MPI_VI_SetWDRAttr(ViDev, &stWdrAttr);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int vi_set_chn(VI_CHN ViChn, RECT_S *pstCapRect, SIZE_S *pstTarSize)
{
    HI_S32 s32Ret;
    VI_CHN_ATTR_S stChnAttr;
    ROTATE_E enRotate = ROTATE_NONE;

    /* step  5: config & start vicap dev */
    memcpy(&stChnAttr.stCapRect, pstCapRect, sizeof(RECT_S));
    stChnAttr.enCapSel = VI_CAPSEL_BOTH;
    /* to show scale. this is a sample only, we want to show dist_size = D1 only */
    stChnAttr.stDestSize.u32Width = pstTarSize->u32Width;
    stChnAttr.stDestSize.u32Height = pstTarSize->u32Height;
    stChnAttr.enPixFormat = g_av_args->pixel_fmt;   /* sp420 or sp422 */

    stChnAttr.bMirror = HI_FALSE;
    stChnAttr.bFlip = HI_FALSE;

    stChnAttr.s32SrcFrameRate = -1;
    stChnAttr.s32DstFrameRate = -1;
    stChnAttr.enCompressMode = COMPRESS_MODE_NONE;

    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, &stChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if(ROTATE_NONE != enRotate)
    {
        s32Ret = HI_MPI_VI_SetRotate(ViChn, enRotate);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int vi_mode2param(SAMPLE_VI_MODE_E enViMode, SAMPLE_VI_PARAM_S *pstViParam)
{
    switch (enViMode)
    {
        default:
            pstViParam->s32ViDevCnt      = 1;
            pstViParam->s32ViDevInterval = 1;
            pstViParam->s32ViChnCnt      = 1;
            pstViParam->s32ViChnInterval = 1;
            break;
    }
    return HI_SUCCESS;
}

static int vi_unbind_vpss(SAMPLE_VI_MODE_E enViMode)
{
    HI_S32 i, j, s32Ret;
    VPSS_GRP VpssGrp;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    SAMPLE_VI_PARAM_S stViParam;
    VI_DEV ViDev;
    VI_CHN ViChn;

    s32Ret = vi_mode2param(enViMode, &stViParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    VpssGrp = 0;
    for (i=0; i<stViParam.s32ViDevCnt; i++)
    {
        ViDev = i * stViParam.s32ViDevInterval;

        for (j=0; j<stViParam.s32ViChnCnt; j++)
        {
            ViChn = j * stViParam.s32ViChnInterval;

            stSrcChn.enModId = HI_ID_VIU;
            stSrcChn.s32DevId = ViDev;
            stSrcChn.s32ChnId = ViChn;

            stDestChn.enModId = HI_ID_VPSS;
            stDestChn.s32DevId = VpssGrp;
            stDestChn.s32ChnId = 0;

            s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

            VpssGrp ++;
        }
    }
    return HI_SUCCESS;
}

static int isp_init(WDR_MODE_E  enWDRMode)
{
    ISP_DEV IspDev = 0;
    HI_S32 s32Ret;
    ISP_PUB_ATTR_S stPubAttr;
    ALG_LIB_S stAeLib;
    memset(&stAeLib, 0, sizeof(stAeLib));
    ALG_LIB_S stAwbLib;
    memset(&stAwbLib, 0, sizeof(stAwbLib));
    ALG_LIB_S stLib;
    memset(&stLib, 0, sizeof(stAwbLib));
    
    stAeLib.s32Id = 0;
    strcpy(stAeLib.acLibName, HI_AE_LIB_NAME);

    stAwbLib.s32Id = 0;
    strcpy(stAwbLib.acLibName, HI_AWB_LIB_NAME);
    
    if (g_av_args->sensor_type == SONY_IMX226_LVDS_12M_30FPS)
    {
        extern ISP_SNS_OBJ_S stSnsImx226Obj;
        s32Ret = stSnsImx226Obj.pfnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    
    /* 2. register hisi ae lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    s32Ret = HI_MPI_AE_Register(IspDev, &stLib);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    /* 3. register hisi awb lib */
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    s32Ret = HI_MPI_AWB_Register(IspDev, &stLib);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    /* 4. register hisi af lib */
    s32Ret = sal_af_register_callback();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);


    //stLib.s32Id = 0;
    //strcpy(stLib.acLibName, HI_AF_LIB_NAME);
    //s32Ret = HI_MPI_AF_Register(IspDev, &stLib);
    //CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);


    /* 5. isp mem init */
    s32Ret = HI_MPI_ISP_MemInit(IspDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    /* 6. isp set WDR mode */
    ISP_WDR_MODE_S stWdrMode;
    stWdrMode.enWDRMode  = enWDRMode;
    s32Ret = HI_MPI_ISP_SetWDRMode(0, &stWdrMode);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (g_av_args->sensor_type == APTINA_AR0130_DC_720P_30FPS
        || g_av_args->sensor_type == APTINA_AR0237_DC_1080P_30FPS)
    {
        stPubAttr.enBayer = BAYER_GRBG;
    }
    else if (g_av_args->sensor_type == OMNIVISION_OV9732_MIPI_720P_30FPS
            || g_av_args->sensor_type == OMNIVISION_OV2710_MIPI_1080P_30FPS
            || g_av_args->sensor_type == SMARTSENS_SC1135_DC_960P_30FPS
            || g_av_args->sensor_type == SMARTSENS_SC2135_DC_1080P_30FPS)
    {
        stPubAttr.enBayer = BAYER_BGGR;
    }
    else if (g_av_args->sensor_type == SONY_IMX291_LVDS_1080P_30FPS)
    {
        stPubAttr.enBayer = BAYER_GBRG;
    }
    else if (g_av_args->sensor_type == SONY_IMX323_DC_1080P_30FPS)
    {
        stPubAttr.enBayer = BAYER_RGGB;
    }
    else if (g_av_args->sensor_type == SONY_IMX226_LVDS_12M_30FPS)
    {
        stPubAttr.enBayer = BAYER_RGGB;
    }
    else
    {
        DBG("Unknown sensor %d.\n", g_av_args->sensor_type);
        return HI_FAILURE;
    }

    stPubAttr.f32FrameRate          = g_av_args->vi_fps;//main stream frame rate
    stPubAttr.stWndRect.s32X        = 0;
    stPubAttr.stWndRect.s32Y        = 0;
    stPubAttr.stWndRect.u32Width    = g_av_args->vi_width;
    stPubAttr.stWndRect.u32Height   = g_av_args->vi_height;
    stPubAttr.stSnsSize.u32Width    = g_av_args->vi_width;;
    stPubAttr.stSnsSize.u32Height   = g_av_args->vi_height;

    s32Ret = HI_MPI_ISP_SetPubAttr(IspDev, &stPubAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    /* 8. isp init */
    s32Ret = HI_MPI_ISP_Init(IspDev);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static HI_VOID* isp_run(HI_VOID *param)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    ISP_DEV IspDev = 0;
    HI_MPI_ISP_Run(IspDev);

    return HI_NULL;
}

static int isp_start()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 4096 * 1024);
    if (0 != pthread_create(&g_av_args->isp_tid, &attr, isp_run, NULL))
    {
        DBG("%s: create isp running thread failed!\n", __FUNCTION__);
        pthread_attr_destroy(&attr);
        return HI_FAILURE;
    }
    usleep(1000);
    pthread_attr_destroy(&attr);

    return HI_SUCCESS;
}

static void isp_stop()
{
    ISP_DEV IspDev = 0;

    HI_MPI_ISP_Exit(IspDev);
    if (g_av_args->isp_tid != 0)
    {
        pthread_join(g_av_args->isp_tid, NULL);
    }

    return;
}

static int vi_stop_isp(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_U32 i;
    HI_S32 s32Ret;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;

    if(!pstViConfig)
    {
        DBG("%s: null ptr\n", __FUNCTION__);
        return HI_FAILURE;
    }

    /*** Stop VI Chn ***/
    for(i=0;i < u32ChnNum; i++)
    {
        /* Stop vi phy-chn */
        ViChn = i;
        s32Ret = HI_MPI_VI_DisableChn(ViChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    /*** Stop VI Dev ***/
    for(i=0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = HI_MPI_VI_DisableDev(ViDev);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    isp_stop();
    return HI_SUCCESS;
}

static int vi_start_all()
{
    SAMPLE_VI_CONFIG_S stViConfig;
    stViConfig.enViMode   = g_av_args->sensor_type;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enWDRMode  = WDR_MODE_NONE;

    HI_U32 i, s32Ret = HI_SUCCESS;
    VI_DEV ViDev;
    VI_CHN ViChn;
    HI_U32 u32DevNum = 1;
    HI_U32 u32ChnNum = 1;
    SIZE_S stTargetSize;
    RECT_S stCapRect;

    s32Ret = vi_set_mipi(&stViConfig);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = isp_init(stViConfig.enWDRMode);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = isp_start();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    for (i = 0; i < u32DevNum; i++)
    {
        ViDev = i;
        s32Ret = vi_set_dev(ViDev, stViConfig.enViMode);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    for (i = 0; i < u32ChnNum; i++)
    {
        ViChn = i;

        stCapRect.s32X = 0;
        stCapRect.s32Y = 0;
        stCapRect.u32Width = g_av_args->vi_width;
        stCapRect.u32Height = g_av_args->vi_height;

        stTargetSize.u32Width = stCapRect.u32Width;
        stTargetSize.u32Height = stCapRect.u32Height;
        s32Ret = vi_set_chn(ViChn, &stCapRect, &stTargetSize);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    return s32Ret;
}

static int vpss_set_group(VPSS_GRP VpssGrp, VPSS_GRP_ATTR_S *pstVpssGrpAttr)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int vpss_stop_group(VPSS_GRP VpssGrp)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int vpss_bind_vi()
{
    HI_S32 j, s32Ret;
    VPSS_GRP VpssGrp;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    SAMPLE_VI_PARAM_S stViParam;
    VI_CHN ViChn;

    s32Ret = vi_mode2param(g_av_args->sensor_type, &stViParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    VpssGrp = 0;
    for (j=0; j<stViParam.s32ViChnCnt; j++)
    {
        ViChn = j * stViParam.s32ViChnInterval;

        stSrcChn.enModId  = HI_ID_VIU;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = ViChn;

        stDestChn.enModId  = HI_ID_VPSS;
        stDestChn.s32DevId = VpssGrp;
        stDestChn.s32ChnId = 0;

        s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        VpssGrp ++;
    }
    return HI_SUCCESS;
}

static int vpss_start()
{
    int s32Ret = 0;

    VPSS_GRP VpssGrp = 0;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    memset(&stVpssGrpAttr, 0, sizeof(stVpssGrpAttr));
    stVpssGrpAttr.u32MaxW = g_av_args->vi_width;
    stVpssGrpAttr.u32MaxH = g_av_args->vi_height;
    stVpssGrpAttr.bIeEn = HI_FALSE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enNrType = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.u32RefFrameNum = 2;
    stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrRefSource = VPSS_NR_REF_FROM_RFR;
    stVpssGrpAttr.stNrAttr.stNrVideoAttr.enNrOutputMode = VPSS_NR_OUTPUT_NORMAL;
    stVpssGrpAttr.bHistEn = HI_FALSE;
    stVpssGrpAttr.bDciEn = HI_FALSE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
    stVpssGrpAttr.enPixFmt = g_av_args->pixel_fmt;
    
    stVpssGrpAttr.bStitchBlendEn = HI_FALSE;
    DBG("\n");
    s32Ret = vpss_set_group(VpssGrp, &stVpssGrpAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = vpss_bind_vi();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int vpss_set_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn,VPSS_CHN_ATTR_S *pstVpssChnAttr,VPSS_CHN_MODE_S *pstVpssChnMode,VPSS_EXT_CHN_ATTR_S *pstVpssExtChnAttr)
{
    HI_S32 s32Ret;

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, pstVpssChnAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else
    {
        s32Ret = HI_MPI_VPSS_SetExtChnAttr(VpssGrp, VpssChn, pstVpssExtChnAttr);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (VpssChn < VPSS_MAX_PHY_CHN_NUM && HI_NULL != pstVpssChnMode)
    {
        if (VpssChn != 0 && g_av_args->video.rotate)
        {
            HI_U32 temp = pstVpssChnMode->u32Width;
            pstVpssChnMode->u32Width = pstVpssChnMode->u32Height;
            pstVpssChnMode->u32Height = temp;
        }
        s32Ret = HI_MPI_VPSS_SetChnMode(VpssGrp, VpssChn, pstVpssChnMode);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (g_av_args->video.rotate && VpssChn < VPSS_MAX_PHY_CHN_NUM)
    {
        s32Ret = HI_MPI_VPSS_SetRotate(VpssGrp, VpssChn, ROTATE_90);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int vpss_disable_chn(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int venc_destroy_chn(VENC_CHN VencChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VENC_StopRecvPic(VencChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VENC_DestroyChn(VencChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int venc_enable_vui(VENC_CHN VencChn)
{
    int s32Ret = 0;
    VENC_PARAM_H264_VUI_S vui;
    memset(&vui, 0, sizeof(vui));

    s32Ret = HI_MPI_VENC_GetH264Vui(VencChn, &vui);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    sal_stream_s* stream = &g_av_args->video.stream[VencChn];
    vui.stVuiTimeInfo.timing_info_present_flag = 1;
    vui.stVuiTimeInfo.fixed_frame_rate_flag = 1;
    vui.stVuiTimeInfo.num_units_in_tick = 1;
    vui.stVuiTimeInfo.time_scale = 2 * stream->framerate;

    s32Ret = HI_MPI_VENC_SetH264Vui(VencChn, &vui);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_set_chn(VPSS_CHN VpssChn)
{
    VPSS_GRP VpssGrp = 0;
    int s32Ret = 0;

    sal_stream_s* stream = &g_av_args->video.stream[VpssChn];

    VPSS_CHN_MODE_S stVpssChnMode;
    memset(&stVpssChnMode, 0, sizeof(stVpssChnMode));

    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = g_av_args->pixel_fmt;
    stVpssChnMode.u32Width       = (0 == VpssChn) ? g_av_args->vi_width : stream->width;
    stVpssChnMode.u32Height      = (0 == VpssChn) ? g_av_args->vi_height : stream->height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    VPSS_CHN_ATTR_S stVpssChnAttr;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));

    //vpss frame rate control
    stVpssChnAttr.s32SrcFrameRate = -1;
    stVpssChnAttr.s32DstFrameRate = -1;
    s32Ret = vpss_set_chn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_set_chn_ext(VPSS_CHN VpssPhyChn, VPSS_CHN VpssExtChn)
{
    VPSS_GRP VpssGrp = 0;
    int s32Ret = 0;

    sal_stream_s* stream = &g_av_args->video.stream[VpssPhyChn];
    VPSS_CHN_MODE_S stVpssChnMode;
    memset(&stVpssChnMode, 0, sizeof(stVpssChnMode));

    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble        = HI_FALSE;
    stVpssChnMode.enPixelFormat  = g_av_args->pixel_fmt;
    stVpssChnMode.u32Width       = (0 == VpssPhyChn) ? g_av_args->vi_width : stream->width;
    stVpssChnMode.u32Height      = (0 == VpssPhyChn) ? g_av_args->vi_height : stream->height;
    stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;

    VPSS_EXT_CHN_ATTR_S stExtChnAttr;
    memset(&stExtChnAttr, 0, sizeof(stExtChnAttr));
    stExtChnAttr.s32BindChn = VpssPhyChn;
    stExtChnAttr.u32Width = stream->width;
    stExtChnAttr.u32Height = stream->height;
    stExtChnAttr.s32SrcFrameRate = -1;
    stExtChnAttr.s32DstFrameRate = -1;
    stExtChnAttr.enPixelFormat = g_av_args->pixel_fmt;;
    stExtChnAttr.enCompressMode = COMPRESS_MODE_NONE;

    s32Ret = vpss_set_chn(VpssGrp, VpssExtChn, HI_NULL, &stVpssChnMode, &stExtChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_set_qp(VENC_CHN VencChn)
{
    int s32Ret = HI_FAILURE;
    VENC_RC_PARAM_S stRcParam;
    s32Ret = HI_MPI_VENC_GetRcParam(VencChn, &stRcParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    VENC_CHN_ATTR_S stVencChnAttr;
    s32Ret = HI_MPI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    if (stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR)
    {
        stRcParam.stParamH264Cbr.u32MinIQp = 20;
        stRcParam.stParamH264Cbr.u32MinQp = 10;
        stRcParam.stParamH264Cbr.u32MaxQp = 51;
    }
    else if (stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H265CBR)
    {
        stRcParam.stParamH265Cbr.u32MinIQp = 20;
        stRcParam.stParamH265Cbr.u32MinQp = 10;
        stRcParam.stParamH265Cbr.u32MaxQp = 51;
    }
    else
    {
        //DBG("Unknown bitrate_ctl: %d\n", stVencChnAttr.stRcAttr.enRcMode);
        //return -1;
    }

    s32Ret = HI_MPI_VENC_SetRcParam(VencChn, &stRcParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int venc_set_h264(VENC_CHN VencChn, HI_U32  u32Profile)
{
    HI_S32 s32Ret = -1;
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
    VENC_ATTR_H264_S stH264Attr;
    memset(&stH264Attr, 0, sizeof(stH264Attr));
    VENC_ATTR_H264_CBR_S    stH264Cbr;
    memset(&stH264Cbr, 0, sizeof(stH264Cbr));
    VENC_ATTR_H264_VBR_S    stH264Vbr;
    memset(&stH264Vbr, 0, sizeof(stH264Vbr));

    sal_stream_s* stream = &g_av_args->video.stream[VencChn];
    stVencChnAttr.stVeAttr.enType = PT_H264;

    stH264Attr.u32MaxPicWidth = stream->width;
    stH264Attr.u32MaxPicHeight = stream->height;
    stH264Attr.u32PicWidth = stream->width;
    stH264Attr.u32PicHeight = stream->height;
    stH264Attr.u32BufSize  = stream->width * stream->height*2;
    stH264Attr.u32Profile  = u32Profile;/*0: baseline; 1:MP; 2:HP;  3:svc_t */
    stH264Attr.bByFrame = HI_TRUE;
    DBG("res[%dx%d], u32Profile: %d\n", stream->width, stream->height, stH264Attr.u32Profile);
    //stH264Attr.u32BFrameNum = 0;
    //stH264Attr.u32RefNum = 1;
    memcpy(&stVencChnAttr.stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));
    
    stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 4;//IP帧QP差值，用于调节呼吸效应。建议值：[2, 6]
    
    if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_CBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
        stH264Cbr.u32Gop            = stream->gop;
        stH264Cbr.u32StatTime       = stream->gop / stream->framerate;
        stH264Cbr.u32SrcFrmRate  = g_av_args->vi_fps;
        stH264Cbr.fr32DstFrmRate = stream->framerate;
        stH264Cbr.u32BitRate = stream->bitrate;
        stH264Cbr.u32FluctuateLevel = 1;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
        DBG("u32SrcFrmRate: %u\n", stH264Cbr.u32SrcFrmRate);
        
    }
    else if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_VBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
        stH264Vbr.u32Gop = stream->gop;
        stH264Vbr.u32StatTime = stream->gop / stream->framerate;
        stH264Vbr.u32SrcFrmRate = g_av_args->vi_fps;
        stH264Vbr.fr32DstFrmRate = stream->framerate;
        stH264Vbr.u32MinQp = 25;
        stH264Vbr.u32MinIQp = 25;
        stH264Vbr.u32MaxQp = 51;
        stH264Vbr.u32MaxBitRate = stream->bitrate;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
    }
    
    DBG("VencChn: %d\n", VencChn);
    s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int venc_set_h265(VENC_CHN VencChn, HI_U32  u32Profile)
{
    HI_S32 s32Ret = -1;
    VENC_CHN_ATTR_S stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(stVencChnAttr));
    VENC_ATTR_H265_S stH265Attr;
    memset(&stH265Attr, 0, sizeof(stH265Attr));
    VENC_ATTR_H265_CBR_S    stH265Cbr;
    memset(&stH265Cbr, 0, sizeof(stH265Cbr));
    VENC_ATTR_H265_VBR_S    stH265Vbr;
    memset(&stH265Vbr, 0, sizeof(stH265Vbr));

    sal_stream_s* stream = &g_av_args->video.stream[VencChn];
    stVencChnAttr.stVeAttr.enType = PT_H265;

    stH265Attr.u32MaxPicWidth = stream->width;
    stH265Attr.u32MaxPicHeight = stream->height;
    stH265Attr.u32PicWidth = stream->width;
    stH265Attr.u32PicHeight = stream->height;
    stH265Attr.u32BufSize  = HIALIGN(stream->width * stream->height*3/4, 64);
    /*Hi3516A/D H265只支持MP*/
    stH265Attr.u32Profile  = 0;
    stH265Attr.bByFrame = HI_TRUE;
    memcpy(&stVencChnAttr.stVeAttr.stAttrH265e, &stH265Attr, sizeof(VENC_ATTR_H265_S));

    if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_CBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
        stH265Cbr.u32Gop            = stream->gop;
        stH265Cbr.u32StatTime       = stream->gop / stream->framerate;
        stH265Cbr.u32SrcFrmRate  = g_av_args->vi_fps;
        stH265Cbr.fr32DstFrmRate = stream->framerate;
        stH265Cbr.u32BitRate = stream->bitrate;
        stH265Cbr.u32FluctuateLevel = 1;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH265Cbr, &stH265Cbr, sizeof(VENC_ATTR_H265_CBR_S));
    }
    else if (stream->bitrate_ctl == SAL_BITRATE_CONTROL_VBR)
    {
        stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
        stH265Vbr.u32Gop = stream->gop;
        stH265Vbr.u32StatTime = stream->gop / stream->framerate;
        stH265Vbr.u32SrcFrmRate = g_av_args->vi_fps;
        stH265Vbr.fr32DstFrmRate = stream->framerate;
        stH265Vbr.u32MinIQp = 25;
        stH265Vbr.u32MinQp = 25;
        stH265Vbr.u32MaxQp = 51;
        stH265Vbr.u32MaxBitRate = stream->bitrate;
        memcpy(&stVencChnAttr.stRcAttr.stAttrH265Vbr, &stH265Vbr, sizeof(VENC_ATTR_H265_VBR_S));
    }

    if (0 == VencChn && g_av_args->smartP_enable)
    {
        int real_idr_interval = stream->gop*g_av_args->gop_times;
        stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
        stVencChnAttr.stGopAttr.stSmartP.u32BgInterval = real_idr_interval;
        stVencChnAttr.stGopAttr.stSmartP.s32BgQpDelta = 7;
        stVencChnAttr.stGopAttr.stSmartP.s32ViQpDelta = 2;
        // vbr/cbr reset stattime
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = real_idr_interval/stream->framerate;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = real_idr_interval/stream->framerate;
    }

    s32Ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VENC_StartRecvPic(VencChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return HI_SUCCESS;
}

static int venc_bind_vpss(VENC_CHN VencChn, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VencChn;

    s32Ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_unbind_vpss(VENC_CHN VeChn,VPSS_GRP VpssGrp,VPSS_CHN VpssChn)
{
    HI_S32 s32Ret = HI_SUCCESS;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VeChn;

    s32Ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_set_cfg(VENC_CHN VencChn)
{
    unsigned int u32Profile = 2; // 2 h264 high profile 3 svc-t
    int s32Ret = 0;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = VencChn;

    sal_stream_s* stream = &g_av_args->video.stream[VpssChn];
    if (SAL_ENCODE_TYPE_H264 == stream->encode_type)
    {
        s32Ret = venc_set_h264(VencChn, u32Profile);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        s32Ret = venc_enable_vui(VencChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else
    {
        s32Ret = venc_set_h265(VencChn, u32Profile);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    if (0 == VencChn && g_av_args->lowdelay_enable)
    {
        VPSS_LOW_DELAY_INFO_S stLowDelayInfo;
        sal_stream_s* stream = &g_av_args->video.stream[VpssChn];
        memset(&stLowDelayInfo, 0, sizeof(VPSS_LOW_DELAY_INFO_S));
        s32Ret = HI_MPI_VPSS_GetLowDelayAttr(VpssGrp,VpssChn,&stLowDelayInfo);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        stLowDelayInfo.bEnable = HI_TRUE;
        stLowDelayInfo.u32LineCnt = stream->height/2;
        s32Ret = HI_MPI_VPSS_SetLowDelayAttr(VpssGrp,VpssChn,&stLowDelayInfo);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    return s32Ret;
}

static int venc_file_write(int stream_id, const char* file, const void* addr, int size)
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

static int venc_pipe_write(int stream_id, const void* frame, int len)
{

#ifndef F_LINUX_SPECIFIC_BASE
#define F_LINUX_SPECIFIC_BASE       1024
#endif
#ifndef F_SETPIPE_SZ
#define F_SETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 7)
#endif
#ifndef F_GETPIPE_SZ
#define F_GETPIPE_SZ	(F_LINUX_SPECIFIC_BASE + 8)
#endif
#define RTSP_PIPE_PATH          "/tmp/rtspfifo.264"


    int ret = -1;
    if (access(RTSP_PIPE_PATH, F_OK))
    {
        //unlink(PIPE_PATH);
        ret = mkfifo(RTSP_PIPE_PATH, 0666);
        CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));
    }

    static int pipe_fd = -1;
    if (pipe_fd < 0)
    {
        pipe_fd = open(RTSP_PIPE_PATH, O_RDWR);
        CHECK(pipe_fd > 0, -1, "Error with %s.\n", strerror(errno));

        int flag = fcntl(pipe_fd, F_GETFL);
        CHECK(flag != -1, -1, "failed to get fcntl flag: %s\n", strerror(errno));
        CHECK(fcntl(pipe_fd, F_SETFL, flag | O_NONBLOCK) != -1, -1, "failed to set nonblock: %s\n", strerror(errno));
        DBG("setnoblock success.fd: %d\n", pipe_fd);

        flag = fcntl(pipe_fd, F_SETPIPE_SZ, 1*1024*1024);
        CHECK(flag != -1, -1, "Error with: %s\n", strerror(errno));
    }

    if (stream_id == 0 && pipe_fd)
    {
        ret = write(pipe_fd, frame, len);
        CHECK(ret == len, -1, "Error with: ret: %d. %s.\n", ret, strerror(errno));
    }

    return 0;
}

static int venc_write_cb(int stream_id, unsigned long long pts, char *data, int len, int keyFrame, SAL_ENCODE_TYPE_E encode_type)
{
    double timestamp = pts/1000;

    sal_statistics_proc(stream_id, timestamp, len, keyFrame,
                        g_av_args->video.stream[stream_id].gop,
                        g_av_args->video.stream[stream_id].framerate);

    if (g_av_args->video.cb != NULL)
    {
        g_av_args->video.cb(stream_id, data, len, keyFrame, timestamp, encode_type);
    }

    if (FILE_WRITE_TEST)
    {
        const char* path = (SAL_ENCODE_TYPE_H264 == encode_type) ? "stream_264" : "stream_265";
        venc_file_write(stream_id, path, data, len);
    }

    if (PIPE_WRITE_TEST)
    {
        venc_pipe_write(stream_id, data, len);
    }

    return 0;
}

static int venc_one_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
{
    HI_S32 s32Ret = HI_FAILURE;
    VENC_PACK_S pack;
    memset(&pack, 0, sizeof(pack));

    CHECK(1 == pstStat->u32CurPacks, HI_FAILURE, "Error with %#x.\n", s32Ret);
    pstStream->pstPack = &pack;
    pstStream->u32PackCount = pstStat->u32CurPacks;

    int timeout = 0; // 非阻塞
    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, timeout);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    HI_U8* frame_addr = pstStream->pstPack->pu8Addr + pstStream->pstPack->u32Offset;
    HI_U32 frame_len = pstStream->pstPack->u32Len - pstStream->pstPack->u32Offset;
    int isKey = 0;
    sal_stream_s* stream = &g_av_args->video.stream[i];
    if (SAL_ENCODE_TYPE_H264 == stream->encode_type)
    {
        isKey = (pstStream->pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ? 1 : 0;
    }
    else if (SAL_ENCODE_TYPE_H265 == stream->encode_type)
    {
        isKey = (pstStream->pstPack->DataType.enH265EType == H265E_NALU_IDRSLICE) ? 1 : 0;
    }

    s32Ret = venc_write_cb(i, pstStream->pstPack->u64PTS, (char*)frame_addr, frame_len, isKey, stream->encode_type);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return 0;
}

static int venc_multiple_pack(VENC_CHN i, VENC_STREAM_S* pstStream, VENC_CHN_STAT_S* pstStat)
{
    HI_S32 s32Ret = HI_FAILURE;
    char* buffer = g_av_args->multiple_buffer;

    //DBG("u32CurPacks: %u\n", pstStat->u32CurPacks);
    pstStream->pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * pstStat->u32CurPacks);
    CHECK(pstStream->pstPack, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(VENC_PACK_S) * pstStat->u32CurPacks);

    pstStream->u32PackCount = pstStat->u32CurPacks;

    s32Ret = HI_MPI_VENC_GetStream(i, pstStream, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    int isKey = 0;
    int buffer_offset = 0;
    int j = 0;
    sal_stream_s* stream = &g_av_args->video.stream[i];
    for (j = 0; j < pstStream->u32PackCount; j++)
    {
        HI_U8* frame_addr = pstStream->pstPack[j].pu8Addr + pstStream->pstPack[j].u32Offset;
        HI_U32 frame_len = pstStream->pstPack[j].u32Len - pstStream->pstPack[j].u32Offset;

        if (SAL_ENCODE_TYPE_H264 == stream->encode_type)
        {
            isKey |= (pstStream->pstPack[j].DataType.enH264EType == H264E_NALU_IDRSLICE) ? 1 : 0;
            //DBG("stream: %d type: %d isKey: %d\n", i, pstStream->pstPack[j].DataType.enH264EType, isKey);
        }
        else if (SAL_ENCODE_TYPE_H265 == stream->encode_type)
        {
            isKey |= (pstStream->pstPack[j].DataType.enH265EType == H265E_NALU_IDRSLICE) ? 1 : 0;
        }
        
        memcpy(buffer+buffer_offset, frame_addr, frame_len);
        buffer_offset += frame_len;
    }

    //DBG("stream: %d, len: %d isKey: %d, u64PTS: %llu\n", i, buffer_offset, isKey, pstStream->pstPack[j-1].u64PTS);
    s32Ret = venc_write_cb(i, pstStream->pstPack[j-1].u64PTS, buffer, buffer_offset, isKey, stream->encode_type);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = HI_MPI_VENC_ReleaseStream(i, pstStream);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    free(pstStream->pstPack);
    pstStream->pstPack = NULL;

    return 0;
}

static void* venc_thread(void *p)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    HI_S32 maxfd = 0;
    fd_set read_fds;
    HI_S32 VencFd[VENC_MAX_CHN_NUM];
    VENC_CHN_STAT_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret = HI_FAILURE;

    int i = 0;
    for (i = 0; i < g_av_args->video_chn_num; i++)
    {
        if (g_av_args->video.stream[i].enable)
        {
            VencFd[i] = HI_MPI_VENC_GetFd(i);
            CHECK(VencFd[i] >= 0, NULL, "Error fd %d.\n", VencFd[i]);

            if (maxfd <= VencFd[i])
            {
                maxfd = VencFd[i];
            }
        }
    }

    while (g_av_args->running)
    {
        FD_ZERO(&read_fds);
        for (i = 0; i < g_av_args->video_chn_num; i++)
        {
            FD_SET(VencFd[i], &read_fds);
        }

        struct timeval TimeoutVal = {0, 100*1000};
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            DBG("select failed!\n");
            break;
        }
        else if (s32Ret == 0)
        {
            //DBG("get venc stream time out.\n");
            continue;
        }

        for (i = 0; i < g_av_args->video_chn_num; i++)
        {
            if (FD_ISSET(VencFd[i], &read_fds))
            {
                memset(&stStat, 0, sizeof(stStat));
                memset(&stStream, 0, sizeof(stStream));

                s32Ret = HI_MPI_VENC_Query(i, &stStat);
                CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);

                if(0 == stStat.u32CurPacks)
                {
                      DBG("NOTE: Current  frame is NULL!\n");
                      continue;
                }

                if (g_av_args->onestream_enable)
                {
                    s32Ret = venc_one_pack(i, &stStream, &stStat);
                    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
                }
                else
                {
                    s32Ret = venc_multiple_pack(i, &stStream, &stStat);
                    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
                }
            }
        }
    }


    return NULL;
}

static int venc_set_h264base_args(int channel)
{
    CHECK(channel < g_av_args->video_chn_num, HI_FAILURE, "channel [%d] illegal\n", channel);

    int s32Ret = 0;
    VENC_CHN_ATTR_S stVencChnAttr;

    int bitrate = g_av_args->video.stream[channel].bitrate;
    int bitrate_ctl = g_av_args->video.stream[channel].bitrate_ctl;
    int gop = g_av_args->video.stream[channel].gop;
    int src_fps = g_av_args->vi_fps;
    int dst_fps = g_av_args->video.stream[channel].framerate;

    if (dst_fps > src_fps)
    {
        DBG("channel: %d, dst_fps[%d] is more than src_fps[%d].reset it\n", channel, dst_fps, src_fps);
        dst_fps = src_fps;
        g_av_args->video.stream[channel].framerate = g_av_args->vi_fps;
    }

    s32Ret = HI_MPI_VENC_GetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    VENC_RC_MODE_E enNewRcMode = (bitrate_ctl == SAL_BITRATE_CONTROL_VBR) ? VENC_RC_MODE_H264VBR : VENC_RC_MODE_H264CBR;
    stVencChnAttr.stRcAttr.enRcMode = enNewRcMode;
    if (enNewRcMode == VENC_RC_MODE_H264CBR)
    {
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32Gop = gop;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32StatTime = gop / dst_fps;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = src_fps;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.fr32DstFrmRate = dst_fps;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 1;
        stVencChnAttr.stRcAttr.stAttrH264Cbr.u32BitRate = bitrate;
    }
    else
    {
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32Gop = gop;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32StatTime = gop / dst_fps;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = src_fps;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.fr32DstFrmRate = dst_fps;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MinQp = 25;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = 51;
        stVencChnAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = bitrate;
    }

    s32Ret = HI_MPI_VENC_SetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

static int venc_set_h265base_args(int channel)
{
    CHECK(channel < g_av_args->video_chn_num, HI_FAILURE, "channel [%d] illegal\n", channel);

    int s32Ret = 0;
    VENC_CHN_ATTR_S stVencChnAttr;

    int bitrate = g_av_args->video.stream[channel].bitrate;
    int bitrate_ctl = g_av_args->video.stream[channel].bitrate_ctl;
    int gop = g_av_args->video.stream[channel].gop;
    int src_fps = g_av_args->vi_fps;
    int dst_fps = g_av_args->video.stream[channel].framerate;

    if (dst_fps > src_fps)
    {
        DBG("channel: %d, dst_fps[%d] is more than src_fps[%d].reset it\n", channel, dst_fps, src_fps);
        dst_fps = src_fps;
        g_av_args->video.stream[channel].framerate = g_av_args->vi_fps;
    }

    s32Ret = HI_MPI_VENC_GetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    VENC_RC_MODE_E enNewRcMode = (bitrate_ctl == SAL_BITRATE_CONTROL_VBR) ? VENC_RC_MODE_H265VBR : VENC_RC_MODE_H265CBR;
    stVencChnAttr.stRcAttr.enRcMode = enNewRcMode;
    if (enNewRcMode == VENC_RC_MODE_H265CBR)
    {
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32Gop = gop;
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = gop / dst_fps;
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRate = src_fps;
        stVencChnAttr.stRcAttr.stAttrH265Cbr.fr32DstFrmRate = dst_fps;
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 1;
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32BitRate = bitrate;
    }
    else
    {
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32Gop = gop;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = gop / dst_fps;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate = src_fps;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.fr32DstFrmRate = dst_fps;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MinQp = 25;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = 51;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = bitrate;
    }

    if (0 == channel && g_av_args->smartP_enable)
    {
        if (gop*g_av_args->gop_times != SMART_P_MAX_IDRGOP_SECONDS*dst_fps)
        {
            g_av_args->gop_times = SMART_P_MAX_IDRGOP_SECONDS*dst_fps/gop;
            DBG("gop: %d, dst_fps: %d, gop_times: %d\n", gop, dst_fps, g_av_args->gop_times);
        }

        int real_idr_interval = gop*g_av_args->gop_times;
        stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
        stVencChnAttr.stGopAttr.stSmartP.u32BgInterval = real_idr_interval;
        stVencChnAttr.stGopAttr.stSmartP.s32BgQpDelta = 7;
        stVencChnAttr.stGopAttr.stSmartP.s32ViQpDelta = 2;
        // vbr/cbr reset stattime
        int stattime = (real_idr_interval/dst_fps > 60) ? 60 : real_idr_interval/dst_fps; // max 60s
        stVencChnAttr.stRcAttr.stAttrH265Cbr.u32StatTime = stattime;
        stVencChnAttr.stRcAttr.stAttrH265Vbr.u32StatTime = stattime;
        //DBG("gop: %d, dst_fps: %d, u32StatTime: %d\n", gop, dst_fps, stattime);
    }

    s32Ret = HI_MPI_VENC_SetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

int sal_sys_init(sal_video_s* video)
{
    CHECK(NULL == g_av_args, HI_FAILURE, "reinit error, please exit first.\n");
    CHECK(NULL != video, HI_FAILURE, "null ptr error.\n");

    g_av_args = (sal_av_args*)malloc(sizeof(sal_av_args));
    CHECK(g_av_args != NULL, HI_FAILURE, "malloc %d bytes failed.\n", sizeof(sal_av_args));

    memset(g_av_args, 0, sizeof(sal_av_args));
    pthread_mutex_init(&g_av_args->mutex, NULL);
    memcpy(&g_av_args->video, video, sizeof(*video));
    g_av_args->pixel_fmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    g_av_args->sensor_type = SENSOR_TYPE;
    g_av_args->vi_width = g_av_args->video.stream[0].width;
    g_av_args->vi_height = g_av_args->video.stream[0].height;
    g_av_args->vi_fps = g_av_args->video.stream[0].framerate;
    g_av_args->ext_chn_width = g_av_args->video.stream[0].width;
    g_av_args->ext_chn_height = g_av_args->video.stream[0].height;

    g_av_args->scale_enable = (g_av_args->sensor_type == SONY_IMX291_LVDS_1080P_30FPS) ? 1 : 0;

    if (g_av_args->scale_enable && g_av_args->sensor_type == SONY_IMX291_LVDS_1080P_30FPS)
    {
        g_av_args->vi_width = 1920;
        g_av_args->vi_height = 1080;
        g_av_args->ext_chn_enable = 1;
        g_av_args->ext_chn_number = 4;
    }

    if (g_av_args->video.rotate)
    {
        if (!g_av_args->scale_enable)
        {
            int swap = g_av_args->vi_width;
            g_av_args->vi_width = g_av_args->vi_height;
            g_av_args->vi_height = swap;
        }
    }

    unsigned int i = 0;
    for (i = 0; i < sizeof(g_av_args->video.stream)/sizeof(*g_av_args->video.stream); i++)
    {
        if (g_av_args->video.stream[i].enable)
        {
            g_av_args->video_chn_num++;
        }
    }
    
    g_av_args->smartP_enable = video->smartP_enable;
    g_av_args->gop_times = 20;
    int gop = g_av_args->video.stream[0].gop;
    int dst_fps = g_av_args->video.stream[0].framerate;
    if (gop*g_av_args->gop_times != SMART_P_MAX_IDRGOP_SECONDS*dst_fps)
    {
        g_av_args->gop_times = SMART_P_MAX_IDRGOP_SECONDS*dst_fps/gop;
    }

    CHECK(g_av_args->video_chn_num > 0, HI_FAILURE, "error: video chn num is %d\n", g_av_args->video_chn_num);


    {
        // LBR功能
        for (i = 0; i < g_av_args->video_chn_num; i++)
        {
            g_config.lbr[i].enable = (g_av_args->video.stream[i].bitrate_ctl == SAL_BITRATE_CONTROL_VBR) ? 1 : 0;
            g_config.lbr[i].bitrate = g_av_args->video.stream[i].bitrate;
        }
        // STREAM info
        for (i = 0; i < g_av_args->video_chn_num; i++)
        {
            g_config.video[i].enable = 1;
            sprintf(g_config.video[i].type, "H264");
            sprintf(g_config.video[i].resolution, "%dx%d", g_av_args->video.stream[i].width, g_av_args->video.stream[i].height);
            g_config.video[i].width = g_av_args->video.stream[i].width;
            g_config.video[i].height = g_av_args->video.stream[i].height;
            sprintf(g_config.video[i].profile, "high");
            g_config.video[i].fps = g_av_args->video.stream[i].framerate;
            g_config.video[i].gop = g_av_args->video.stream[i].gop;
            g_config.video[i].bitrate = g_av_args->video.stream[i].bitrate;
            sprintf(g_config.video[i].bitrate_ctrl, "%s", (g_av_args->video.stream[i].bitrate_ctl == SAL_BITRATE_CONTROL_VBR) ? "VBR" : "CBR");
        }
    }

    g_av_args->lowdelay_enable = 0;
    g_av_args->onebuffer_enable = 0;
    g_av_args->onestream_enable = 0;
    if (!g_av_args->onestream_enable)
    {
        int size = g_config.video[0].width*g_config.video[0].height/3;
        g_av_args->multiple_buffer = malloc(size);
        CHECK(g_av_args->multiple_buffer, HI_FAILURE, "malloc %d bytes failed.\n", size);
    }

    int s32Ret = 0;

    s32Ret = sys_vb_init();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = vi_start_all();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    s32Ret = vpss_start();
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    int chn_tmp = 0;
    for (i = 0; i < (unsigned int)g_av_args->video_chn_num; i++)
    {
        chn_tmp = i;
        s32Ret = venc_set_chn(i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        if (i ==0 && g_av_args->ext_chn_enable)
        {
            chn_tmp = g_av_args->ext_chn_number;
            s32Ret = venc_set_chn_ext(i, chn_tmp);
            CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        }

        s32Ret = venc_set_cfg(i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        s32Ret = venc_bind_vpss(i, chn_tmp);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

        s32Ret = venc_set_qp(i);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024 * 1024);
    g_av_args->running = 1;
    s32Ret = pthread_create(&g_av_args->tid, &attr, venc_thread, NULL);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %s.\n", strerror(errno));
    usleep(1000);
    pthread_attr_destroy(&attr);

    return 0;
}

int sal_sys_exit(void)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");

    int s32Ret = 0;

    if (g_av_args->running)
    {
        g_av_args->running = 0;
        pthread_join(g_av_args->tid, NULL);
    }

    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = g_av_args->video_chn_num - 1;
    VENC_CHN VencChn = g_av_args->video_chn_num - 1;
    for (VpssChn = g_av_args->video_chn_num - 1; VpssChn >= 0; VpssChn--)
    {
        VPSS_CHN VpssChnTmp = VpssChn;
        if (VencChn == 0 && g_av_args->ext_chn_enable)
        {
            VpssChnTmp = g_av_args->ext_chn_number;
        }

        s32Ret = venc_unbind_vpss(VencChn, VpssGrp, VpssChnTmp);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("venc[%d] unbind vpss[%d] done.\n", VencChn, VpssChnTmp);

        s32Ret = venc_destroy_chn(VencChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("VENC_Stop[%d] done.\n", VencChn);
        VencChn--;
    }

    if (g_av_args->ext_chn_enable)
    {
        s32Ret = vpss_disable_chn(VpssGrp, g_av_args->ext_chn_number);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("vpss_disable_chn[%d] done.\n", g_av_args->ext_chn_number);
    }

    VpssChn = g_av_args->video_chn_num - 1;
    for (VpssChn = g_av_args->video_chn_num - 1; VpssChn >= 0; VpssChn--)
    {
        s32Ret = vpss_disable_chn(VpssGrp, VpssChn);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
        DBG("DisableChn[%d] done.\n", VpssChn);
    }

    SAMPLE_VI_CONFIG_S stViConfig;
    memset(&stViConfig, 0, sizeof(stViConfig));
    stViConfig.enViMode   = g_av_args->sensor_type;

    //vpss unbind
    s32Ret = vi_unbind_vpss(stViConfig.enViMode);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("UnBindVpss done.\n");

    //vpss stop
    s32Ret = vpss_stop_group(VpssGrp);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("StopGroup done.\n");

    //vi stop
    s32Ret = vi_stop_isp(&stViConfig);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    DBG("StopVi done.\n");

    //system exit
    HI_MPI_SYS_Exit();
    DBG("SYS_Exit done.\n");
    HI_MPI_VB_Exit();
    DBG("VB_Exit done.\n");

    if (!g_av_args->onestream_enable && g_av_args->multiple_buffer)
    {
        free(g_av_args->multiple_buffer);
        g_av_args->multiple_buffer = NULL;
    }

    pthread_mutex_destroy(&g_av_args->mutex);
    free(g_av_args);
    g_av_args = NULL;

    return 0;
}

int sal_sys_version(char* buffer, int len)
{
    int s32Ret = -1;
    MPP_VERSION_S stVersion;
    memset(&stVersion, 0, sizeof(stVersion));

    s32Ret = HI_MPI_SYS_GetVersion(&stVersion);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    char tmp[512];
    memset(tmp, 0, sizeof(tmp));
    snprintf(tmp, sizeof(tmp), "[%s] libipc.so has built at %s %s",
                                stVersion.aVersion, __DATE__, __TIME__);
    CHECK(len > strlen(tmp), HI_FAILURE, "Error with %d.\n", len);
    memset(buffer, 0, len);
    strcpy(buffer, tmp);

    return 0;
}

int sal_video_vi_framerate_set(int framerate)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");

    if (g_av_args->vi_fps == framerate)
    {
        return 0;
    }

    DBG("old vi framerate: %d, new vi framerate: %d\n", g_av_args->vi_fps, framerate);
    g_av_args->vi_fps = framerate;

    pthread_mutex_lock(&g_av_args->mutex);
    int s32Ret = -1;
    ISP_DEV IspDev = 0;
    ISP_PUB_ATTR_S stPubAttr;
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    s32Ret = HI_MPI_ISP_GetPubAttr(IspDev, &stPubAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    stPubAttr.f32FrameRate = framerate;

    s32Ret = HI_MPI_ISP_SetPubAttr(IspDev, &stPubAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    pthread_mutex_unlock(&g_av_args->mutex);

    return 0;
}

int sal_video_framerate_set(int stream, int framerate)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(stream < g_av_args->video_chn_num, HI_FAILURE, "stream [%d] illegal\n", stream);

/*
    int src_fps = g_av_args->video.stream[0].framerate;
    int last_fps = g_av_args->video.stream[stream].framerate;
    if (last_fps == framerate && src_fps >= framerate)
    {
        return 0;
    }
*/

    int s32Ret = -1;


    pthread_mutex_lock(&g_av_args->mutex);
    
    DBG("stream: %d, old framerate: %d, new framerate: %d\n", stream, g_av_args->video.stream[stream].framerate, framerate);
    g_av_args->video.stream[stream].framerate = framerate;

    if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H264)
    {
        s32Ret = venc_set_h264base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H265)
    {
        s32Ret = venc_set_h265base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_mutex_unlock(&g_av_args->mutex);

    return 0;
}

int sal_video_args_get(int stream, sal_stream_s* video)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(stream < g_av_args->video_chn_num, HI_FAILURE, "stream [%d] illegal\n", stream);
    CHECK(NULL != video, HI_FAILURE, "null ptr error.\n");

    pthread_mutex_lock(&g_av_args->mutex);

    memcpy(video, &g_av_args->video.stream[stream], sizeof(*video));

    pthread_mutex_unlock(&g_av_args->mutex);
    return 0;
}

int sal_video_bitrate_set(int stream, SAL_BITRATE_CONTROL_E bitrate_ctl, int bitrate)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(stream < g_av_args->video_chn_num, HI_FAILURE, "stream [%d] illegal\n", stream);
    pthread_mutex_lock(&g_av_args->mutex);

    int s32Ret = 0;

    // LBR功能启用
    {
        g_config.lbr[stream].enable = (bitrate_ctl == SAL_BITRATE_CONTROL_VBR) ? 1 : 0;
        g_config.lbr[stream].bitrate = bitrate;
    }

    g_av_args->video.stream[stream].bitrate_ctl = bitrate_ctl;
    g_av_args->video.stream[stream].bitrate = bitrate;
    
    if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H264)
    {
        s32Ret = venc_set_h264base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H265)
    {
        s32Ret = venc_set_h265base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_mutex_unlock(&g_av_args->mutex);
    return 0;
}

int sal_video_lbr_set(int stream, SAL_BITRATE_CONTROL_E bitrate_ctl, int bitrate)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(stream < g_av_args->video_chn_num, HI_FAILURE, "stream [%d] illegal\n", stream);
    pthread_mutex_lock(&g_av_args->mutex);

    int s32Ret = 0;

    g_av_args->video.stream[stream].bitrate_ctl = bitrate_ctl;
    g_av_args->video.stream[stream].bitrate = bitrate;
    
    if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H264)
    {
        s32Ret = venc_set_h264base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H265)
    {
        s32Ret = venc_set_h265base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_mutex_unlock(&g_av_args->mutex);
    return 0;
}

int sal_video_gop_set(int stream, int gop)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(stream < g_av_args->video_chn_num, HI_FAILURE, "stream [%d] illegal\n", stream);
    pthread_mutex_lock(&g_av_args->mutex);

    int s32Ret = 0;

    g_av_args->video.stream[stream].gop = gop;
    if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H264)
    {
        s32Ret = venc_set_h264base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }
    else if (g_av_args->video.stream[stream].encode_type == SAL_ENCODE_TYPE_H265)
    {
        s32Ret = venc_set_h265base_args(stream);
        CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    }

    pthread_mutex_unlock(&g_av_args->mutex);
    return 0;
}

int sal_video_mirror_set(int enable)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");

    /*int ret = -1;

    switch(g_av_args->sensor_type)
    {
        case OMNIVISION_OV9732_MIPI_720P_30FPS:
        {
            ret = ov9732_mirror_set(enable);
            break;
        }
        case OMNIVISION_OV2710_MIPI_1080P_30FPS:
        {
            ret = ov2710_mirror_set(enable);
            break;
        }
        case SONY_IMX291_LVDS_1080P_30FPS:
        {
            ret = imx291_mirror_set(enable);
            break;
        }
        case SONY_IMX323_DC_1080P_30FPS:
        {
            ret = imx323_mirror_set(enable);
            break;
        }
        case SMARTSENS_SC1135_DC_960P_30FPS:
        {
            ret = sc1135_mirror_set(enable);
            break;
        }
        case APTINA_AR0237_DC_1080P_30FPS:
        {
            ret = ar0237_mirror_set(enable);
            break;
        }
        case SMARTSENS_SC2135_DC_1080P_30FPS:
        {
            ret = sc2135_mirror_set(enable);
            break;
        }
        case APTINA_AR0130_DC_720P_30FPS:
        {
            ret = ar0130_mirror_set(enable);
            break;
        }
        default:
            DBG("unknown sensor type %d.\n", g_av_args->sensor_type);
            break;
    }*/

    return 0;
}

/*flip需要设置sensor停流和开流，否则会有短暂偏红*/
int sal_video_flip_set(int enable)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");

    /*int ret = -1;

    switch(g_av_args->sensor_type)
    {
        case OMNIVISION_OV9732_MIPI_720P_30FPS:
        {
            ret = ov9732_flip_set(enable);
            break;
        }
        case OMNIVISION_OV2710_MIPI_1080P_30FPS:
        {
            ret = ov2710_flip_set(enable);
            break;
        }
        case SONY_IMX291_LVDS_1080P_30FPS:
        {
            ret = imx291_flip_set(enable);
            break;
        }
        case SONY_IMX323_DC_1080P_30FPS:
        {
            ret = imx323_flip_set(enable);
            break;
        }
        case SMARTSENS_SC1135_DC_960P_30FPS:
        {
            ret = sc1135_flip_set(enable);
            break;
        }
        case APTINA_AR0237_DC_1080P_30FPS:
        {
            ret = ar0237_flip_set(enable);
            break;
        }
        case SMARTSENS_SC2135_DC_1080P_30FPS:
        {
            ret = sc2135_flip_set(enable);
            break;
        }
        case APTINA_AR0130_DC_720P_30FPS:
        {
            ret = ar0130_flip_set(enable);
            break;
        }
        default:
            DBG("unknown sensor type %d.\n", g_av_args->sensor_type);
            break;
    }*/

    return 0;
}


int sal_video_force_idr(int channel)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(channel < g_av_args->video_chn_num, HI_FAILURE, "channel [%d] illegal\n", channel);

    int s32Ret = 0;

    s32Ret = HI_MPI_VENC_RequestIDR(channel, HI_TRUE);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

int sal_video_cbr_qp_set(int channel, sal_video_qp_s* qp_args)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(NULL != qp_args, HI_FAILURE, "NULL ptr error.\n");
    CHECK(channel < g_av_args->video_chn_num, HI_FAILURE, "channel [%d] illegal\n", channel);

    int s32Ret = HI_FAILURE;
    VENC_CHN_ATTR_S stVencChnAttr;
    s32Ret = HI_MPI_VENC_GetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    CHECK(stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR, HI_FAILURE, "bitrate mode is not cbr\n");

    VENC_RC_PARAM_S stRcParam;
    s32Ret = HI_MPI_VENC_GetRcParam(channel, &stRcParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    if (g_av_args->video.stream[channel].encode_type == SAL_ENCODE_TYPE_H264)
    {
        stRcParam.stParamH264Cbr.u32MinIQp = qp_args->min_i_qp;
        stRcParam.stParamH264Cbr.u32MaxIQp = qp_args->max_i_qp;
        stRcParam.stParamH264Cbr.u32MinQp = qp_args->min_qp;
        stRcParam.stParamH264Cbr.u32MaxQp = qp_args->max_qp;
    }
    else if (g_av_args->video.stream[channel].encode_type == SAL_ENCODE_TYPE_H265)
    {
        stRcParam.stParamH265Cbr.u32MinIQp = qp_args->min_i_qp;
        stRcParam.stParamH265Cbr.u32MaxIQp = qp_args->max_i_qp;
        stRcParam.stParamH265Cbr.u32MinQp = qp_args->min_qp;
        stRcParam.stParamH265Cbr.u32MaxQp = qp_args->max_qp;
    }

    s32Ret = HI_MPI_VENC_SetRcParam(channel, &stRcParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);

    return s32Ret;
}

int sal_video_cbr_qp_get(int channel, sal_video_qp_s* qp_args)
{
    CHECK(NULL != g_av_args, HI_FAILURE, "module not inited.\n");
    CHECK(NULL != qp_args, HI_FAILURE, "NULL ptr error.\n");
    CHECK(channel < g_av_args->video_chn_num, HI_FAILURE, "channel [%d] illegal\n", channel);

    int s32Ret = HI_FAILURE;
    memset(qp_args, 0, sizeof(*qp_args));
    VENC_CHN_ATTR_S stVencChnAttr;
    s32Ret = HI_MPI_VENC_GetChnAttr(channel, &stVencChnAttr);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    CHECK(stVencChnAttr.stRcAttr.enRcMode == VENC_RC_MODE_H264CBR, HI_FAILURE, "bitrate mode is not cbr\n");

    VENC_RC_PARAM_S stRcParam;
    s32Ret = HI_MPI_VENC_GetRcParam(channel, &stRcParam);
    CHECK(s32Ret == HI_SUCCESS, HI_FAILURE, "Error with %#x.\n", s32Ret);
    
    if (g_av_args->video.stream[channel].encode_type == SAL_ENCODE_TYPE_H264)
    {
        qp_args->min_i_qp = stRcParam.stParamH264Cbr.u32MinIQp;
        qp_args->max_i_qp = stRcParam.stParamH264Cbr.u32MaxIQp;
        qp_args->min_qp = stRcParam.stParamH264Cbr.u32MinQp;
        qp_args->max_qp = stRcParam.stParamH264Cbr.u32MaxQp;
    }
    else if (g_av_args->video.stream[channel].encode_type == SAL_ENCODE_TYPE_H265)
    {
        qp_args->min_i_qp = stRcParam.stParamH265Cbr.u32MinIQp;
        qp_args->max_i_qp = stRcParam.stParamH265Cbr.u32MaxIQp;
        qp_args->min_qp = stRcParam.stParamH265Cbr.u32MinQp;
        qp_args->max_qp = stRcParam.stParamH265Cbr.u32MaxQp;
    }

    return s32Ret;
}


