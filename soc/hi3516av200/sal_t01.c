#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "sal_vgs.h"
#include "sal_t01.h"
#include "sal_list.h"

typedef struct _sal_t01_args
{
    int md_channel;
    IHwDetAttr ioattr;
    HwDetObj stHwDetObj;
    IngDetObject astIngDetObj[128];//temp
    IngDetObject astIngDetObj1[128];//temp

    int running;
    pthread_t pid;
    pthread_mutex_t mutex;
}sal_t01_args;

static sal_t01_args* g_t01_args = NULL;

static int t01_ioattr_init(IHwDetAttr* pstIoattr, int width, int height, int fps)
{
    /* input */
    pstIoattr->input.interface = INPUT_INTERFACE_BT1120;
    pstIoattr->input.data_type = INPUT_DVP_DATA_TYPE_YUV422_16B;
    pstIoattr->input.image_width = width;
    pstIoattr->input.image_height = height;
    pstIoattr->input.raw_rggb = 0;
    pstIoattr->input.raw_align = 1;
    pstIoattr->input.config.dvp_bus_select = 0;
    pstIoattr->input.fps = fps;
    pstIoattr->input.mclk_enable = 0;

    /* output only for MIPI bypass mode */
    
    return 0;
}

/*
 *t01私有数据存放格式yuyv...yuyv，只提取y，所以要隔开一个字节。
 *每一个y的低4bit，拼成一个unsigned int作为帧序号
 */
static unsigned int t01_transferPrivateCount(unsigned char privateData[16])
{
    unsigned int count = 0;
    unsigned char cc[4] = {0};
    cc[0] |= privateData[0] << 4;
    cc[0] |= privateData[2] & 0x0f;
    cc[1] |= privateData[4] << 4;
    cc[1] |= privateData[6] & 0x0f;
    cc[2] |= privateData[8] << 4;
    cc[2] |= privateData[10] & 0x0f;
    cc[3] |= privateData[12] << 4;
    cc[3] |= privateData[14] & 0x0f;
    memcpy(&count, cc, 4);
    
    //DBG("count: %d\n", count);
    return count;
}

static int t01_process_result(md_context_t md_context, HwDetObj* pstObj)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int idx = 0;
    memset(g_t01_args->astIngDetObj, 0, sizeof(g_t01_args->astIngDetObj));
    memset(g_t01_args->astIngDetObj1, 0, sizeof(g_t01_args->astIngDetObj1));
    
    //DBG("objnum: %d\n", pstObj->desc.objnum);
    ASSERT(pstObj->desc.objnum < sizeof(pstObj->obj)/sizeof(pstObj->obj[0]), "error with %d\n", pstObj->desc.objnum);
    
    unsigned int sum = 0;
    unsigned int sum1 = 0;
    for (i = 0; i < pstObj->desc.objnum; i++) 
    {
        IngDetObject* obj = IHwDet_Get_Object(md_context, i);
        CHECK(obj, -1, "Error with: %#x\n", obj);
        
        //DBG("i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
        if (obj->strength >= 0.3 && obj->type <= 2 /*obj->type == 2*/) //[0,6]
        {
            //memcpy(&g_t01_args->astIngDetObj[sum++], obj, sizeof(IngDetObject));
            memcpy(&pstObj->obj[sum++], obj, sizeof(IngDetObject));
            //DBG("valid i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
        }
    }
    pstObj->valid_obj_num = sum;
    
    ////同一对象，type最重要的保留
    ////DBG("sum: %d\n", sum);
    //for (i = 0; i < sum; i++) 
    //{
    //    //DBG("i: %d, id: %d, type: %d, strength: %f, life: %02x\n", i, g_t01_args->astIngDetObj[i].id, g_t01_args->astIngDetObj[i].type, g_t01_args->astIngDetObj[i].strength, g_t01_args->astIngDetObj[i].life);
    //    if (g_t01_args->astIngDetObj[i].life != 0xff)
    //    {
    //        memcpy(&g_t01_args->astIngDetObj1[sum1], &g_t01_args->astIngDetObj[i], sizeof(IngDetObject));
    //        for (j = i+1; j < sum; j++)
    //        {
    //            if (g_t01_args->astIngDetObj[i].id == g_t01_args->astIngDetObj[j].id)
    //            {
    //                if (g_t01_args->astIngDetObj[i].type > g_t01_args->astIngDetObj[j].type)
    //                {
    //                    memcpy(&g_t01_args->astIngDetObj1[sum1], &g_t01_args->astIngDetObj[j], sizeof(IngDetObject));
    //                }
    //                g_t01_args->astIngDetObj[j].life = 0xff;
    //            }
    //        }
    //        sum1++;
    //    }
    //}
    //
    ////矩形重叠的，保留type重要的一项
    ////DBG("sum1: %d\n", sum1);
    //unsigned int k = 0;
    //for (i = 0; i < sum1; i++) 
    //{
    //    //DBG("i: %d, id: %d, type: %d, strength: %f, life: %02x\n", i, g_t01_args->astIngDetObj1[i].id, g_t01_args->astIngDetObj1[i].type, g_t01_args->astIngDetObj1[i].strength, g_t01_args->astIngDetObj1[i].life);
    //    if (g_t01_args->astIngDetObj1[i].life != 0xff) //有效的object
    //    {
    //        memcpy(&pstObj->obj[idx], &g_t01_args->astIngDetObj1[i], sizeof(IngDetObject));
    //        //左上角坐标
    //        unsigned int x1 = g_t01_args->astIngDetObj1[i].x;
    //        unsigned int y1 = g_t01_args->astIngDetObj1[i].y;
    //        //右上角坐标
    //        unsigned int x2 = g_t01_args->astIngDetObj1[i].x+g_t01_args->astIngDetObj1[i].width;
    //        unsigned int y2 = g_t01_args->astIngDetObj1[i].y;
    //        //左下角坐标
    //        unsigned int x3 = g_t01_args->astIngDetObj1[i].x;
    //        unsigned int y3 = g_t01_args->astIngDetObj1[i].y+g_t01_args->astIngDetObj1[i].height;
    //        //右下角坐标
    //        unsigned int x4 = g_t01_args->astIngDetObj1[i].x+g_t01_args->astIngDetObj1[i].width;
    //        unsigned int y4 = g_t01_args->astIngDetObj1[i].y+g_t01_args->astIngDetObj1[i].height;
    //        unsigned int original_area1 = g_t01_args->astIngDetObj1[i].width*g_t01_args->astIngDetObj1[i].height;
    //        for (j = i+1; j < sum1; j++)
    //        {
    //            unsigned int x5 = g_t01_args->astIngDetObj1[j].x;
    //            unsigned int y5 = g_t01_args->astIngDetObj1[j].y;
    //            unsigned int x6 = g_t01_args->astIngDetObj1[j].x+g_t01_args->astIngDetObj1[j].width;
    //            unsigned int y6 = g_t01_args->astIngDetObj1[j].y;
    //            unsigned int x7 = g_t01_args->astIngDetObj1[j].x;
    //            unsigned int y7 = g_t01_args->astIngDetObj1[j].y+g_t01_args->astIngDetObj1[j].height;
    //            unsigned int x8 = g_t01_args->astIngDetObj1[j].x+g_t01_args->astIngDetObj1[j].width;
    //            unsigned int y8 = g_t01_args->astIngDetObj1[j].y+g_t01_args->astIngDetObj1[j].height;
    //            unsigned int original_area2 = g_t01_args->astIngDetObj1[j].width*g_t01_args->astIngDetObj1[j].height;

    //            if ((x1 >= x5 && x1 <= x8 && y1 >= y5 && y1 <= y8) 
    //                || (x2 >= x5 && x2 <= x8 && y2 >= y5 && y2 <= y8)
    //                || (x3 >= x5 && x3 <= x8 && y3 >= y5 && y3 <= y8)
    //                || (x4 >= x5 && x4 <= x8 && y4 >= y5 && y4 <= y8)) /*两个矩形重叠*/
    //            {
    //                //WRN("overlap\n");
    //                int overlap_width = x1-g_t01_args->astIngDetObj1[j].x;
    //                int overlap_height = y1-g_t01_args->astIngDetObj1[j].y;
    //                int overlap_area = overlap_width*overlap_height;
    //                
    //                if (g_t01_args->astIngDetObj1[i].type > g_t01_args->astIngDetObj1[j].type)
    //                {
    //                    memcpy(&pstObj->obj[idx], &g_t01_args->astIngDetObj1[j], sizeof(IngDetObject));
    //                }
    //                g_t01_args->astIngDetObj1[j].life = 0xff;
    //            }
    //        }
    //        idx++;
    //    }
    //}

    //pstObj->valid_obj_num = idx;
    //
    //DBG("valid_obj_num: %d\n", pstObj->valid_obj_num);
    //for (i = 0; i < pstObj->valid_obj_num; i++)
    //{
    //    DBG("i: %d, id: %d, type: %d, xywh[%d %d %d %d]strength: %f, life: %02x\n", i, pstObj->obj[i].id, pstObj->obj[i].type, 
    //            pstObj->obj[i].x, pstObj->obj[i].y, pstObj->obj[i].width, pstObj->obj[i].height, pstObj->obj[i].strength, pstObj->obj[i].life);
    //}
     
    return 0;
}

static int t01_process_result_T(md_context_t md_context, HwDetObj* pstObj)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int idx = 0;
    memset(g_t01_args->astIngDetObj, 0, sizeof(g_t01_args->astIngDetObj));
    memset(g_t01_args->astIngDetObj1, 0, sizeof(g_t01_args->astIngDetObj1));

    //DBG("tgtnum: %d\n", pstObj->desc_T.tgtnum);
    //ASSERT(pstObj->desc_T.objnum < sizeof(pstObj->obj)/sizeof(pstObj->obj[0]), "error with %d\n", pstObj->desc.objnum);

    unsigned int sum = 0;
    unsigned int sum1 = 0;
    for (i = 0; i < pstObj->desc_T.tgtnum; i++) 
    {
        IngTarget_T *target = IHwDet_Get_Target(md_context, i);
        CHECK(target, -1, "Error with: %#x\n", target);
        
        if (IHwDet_Target_Have_Face(target))
        {
            IngDetObject *obj = IHwDet_Target_Get_Object(target, OBJ_TYPE_FACE);
            CHECK(obj, -1, "Error with: %#x\n", obj);
            DBG("i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            if (obj->strength >= 0.3) //[0,6]
            {
                //memcpy(&g_t01_args->astIngDetObj[sum++], obj, sizeof(IngDetObject));
                memcpy(&pstObj->obj[sum++], obj, sizeof(IngDetObject));
                //DBG("valid i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            }
        }
        if (IHwDet_Target_Have_HeadAndShoulder(target))
        {
            IngDetObject *obj = IHwDet_Target_Get_Object(target, OBJ_TYPE_HEAD_AND_SHOULDER);
            CHECK(obj, -1, "Error with: %#x\n", obj);
            DBG("i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            if (obj->strength >= 0.3) //[0,6]
            {
                //memcpy(&g_t01_args->astIngDetObj[sum++], obj, sizeof(IngDetObject));
                memcpy(&pstObj->obj[sum++], obj, sizeof(IngDetObject));
                //DBG("valid i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            }
        }

        if (IHwDet_Target_Have_Figure(target))
        {
            IngDetObject *obj = IHwDet_Target_Get_Object(target, OBJ_TYPE_FIGURE);
            CHECK(obj, -1, "Error with: %#x\n", obj);
            DBG("i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            if (obj->strength >= 0.3) //[0,6]
            {
                //memcpy(&g_t01_args->astIngDetObj[sum++], obj, sizeof(IngDetObject));
                memcpy(&pstObj->obj[sum++], obj, sizeof(IngDetObject));
                //DBG("valid i: %d, id: %d, type: %d, strength: %f, life: %d\n", i, obj->id, obj->type, obj->strength, obj->life);
            }
        }
    }
    pstObj->valid_obj_num = sum;

    ////同一对象，type最重要的保留
    ////DBG("sum: %d\n", sum);
    //for (i = 0; i < sum; i++) 
    //{
    //    //DBG("i: %d, id: %d, type: %d, strength: %f, life: %02x\n", i, g_t01_args->astIngDetObj[i].id, g_t01_args->astIngDetObj[i].type, g_t01_args->astIngDetObj[i].strength, g_t01_args->astIngDetObj[i].life);
    //    if (g_t01_args->astIngDetObj[i].life != 0xff)
    //    {
    //        memcpy(&g_t01_args->astIngDetObj1[sum1], &g_t01_args->astIngDetObj[i], sizeof(IngDetObject));
    //        for (j = i+1; j < sum; j++)
    //        {
    //            if (g_t01_args->astIngDetObj[i].id == g_t01_args->astIngDetObj[j].id)
    //            {
    //                if (g_t01_args->astIngDetObj[i].type > g_t01_args->astIngDetObj[j].type)
    //                {
    //                    memcpy(&g_t01_args->astIngDetObj1[sum1], &g_t01_args->astIngDetObj[j], sizeof(IngDetObject));
    //                }
    //                g_t01_args->astIngDetObj[j].life = 0xff;
    //            }
    //        }
    //        sum1++;
    //    }
    //}
    //
    ////矩形重叠的，保留type重要的一项
    ////DBG("sum1: %d\n", sum1);
    //unsigned int k = 0;
    //for (i = 0; i < sum1; i++) 
    //{
    //    //DBG("i: %d, id: %d, type: %d, strength: %f, life: %02x\n", i, g_t01_args->astIngDetObj1[i].id, g_t01_args->astIngDetObj1[i].type, g_t01_args->astIngDetObj1[i].strength, g_t01_args->astIngDetObj1[i].life);
    //    if (g_t01_args->astIngDetObj1[i].life != 0xff) //有效的object
    //    {
    //        memcpy(&pstObj->obj[idx], &g_t01_args->astIngDetObj1[i], sizeof(IngDetObject));
    //        //左上角坐标
    //        unsigned int x1 = g_t01_args->astIngDetObj1[i].x;
    //        unsigned int y1 = g_t01_args->astIngDetObj1[i].y;
    //        //右上角坐标
    //        unsigned int x2 = g_t01_args->astIngDetObj1[i].x+g_t01_args->astIngDetObj1[i].width;
    //        unsigned int y2 = g_t01_args->astIngDetObj1[i].y;
    //        //左下角坐标
    //        unsigned int x3 = g_t01_args->astIngDetObj1[i].x;
    //        unsigned int y3 = g_t01_args->astIngDetObj1[i].y+g_t01_args->astIngDetObj1[i].height;
    //        //右下角坐标
    //        unsigned int x4 = g_t01_args->astIngDetObj1[i].x+g_t01_args->astIngDetObj1[i].width;
    //        unsigned int y4 = g_t01_args->astIngDetObj1[i].y+g_t01_args->astIngDetObj1[i].height;
    //        unsigned int original_area1 = g_t01_args->astIngDetObj1[i].width*g_t01_args->astIngDetObj1[i].height;
    //        for (j = i+1; j < sum1; j++)
    //        {
    //            unsigned int x5 = g_t01_args->astIngDetObj1[j].x;
    //            unsigned int y5 = g_t01_args->astIngDetObj1[j].y;
    //            unsigned int x6 = g_t01_args->astIngDetObj1[j].x+g_t01_args->astIngDetObj1[j].width;
    //            unsigned int y6 = g_t01_args->astIngDetObj1[j].y;
    //            unsigned int x7 = g_t01_args->astIngDetObj1[j].x;
    //            unsigned int y7 = g_t01_args->astIngDetObj1[j].y+g_t01_args->astIngDetObj1[j].height;
    //            unsigned int x8 = g_t01_args->astIngDetObj1[j].x+g_t01_args->astIngDetObj1[j].width;
    //            unsigned int y8 = g_t01_args->astIngDetObj1[j].y+g_t01_args->astIngDetObj1[j].height;
    //            unsigned int original_area2 = g_t01_args->astIngDetObj1[j].width*g_t01_args->astIngDetObj1[j].height;

    //            if ((x1 >= x5 && x1 <= x8 && y1 >= y5 && y1 <= y8) 
    //                || (x2 >= x5 && x2 <= x8 && y2 >= y5 && y2 <= y8)
    //                || (x3 >= x5 && x3 <= x8 && y3 >= y5 && y3 <= y8)
    //                || (x4 >= x5 && x4 <= x8 && y4 >= y5 && y4 <= y8)) /*两个矩形重叠*/
    //            {
    //                //WRN("overlap\n");
    //                int overlap_width = x1-g_t01_args->astIngDetObj1[j].x;
    //                int overlap_height = y1-g_t01_args->astIngDetObj1[j].y;
    //                int overlap_area = overlap_width*overlap_height;
    //                
    //                if (g_t01_args->astIngDetObj1[i].type > g_t01_args->astIngDetObj1[j].type)
    //                {
    //                    memcpy(&pstObj->obj[idx], &g_t01_args->astIngDetObj1[j], sizeof(IngDetObject));
    //                }
    //                g_t01_args->astIngDetObj1[j].life = 0xff;
    //            }
    //        }
    //        idx++;
    //    }
    //}

    //pstObj->valid_obj_num = idx;

    DBG("valid_obj_num: %d\n", pstObj->valid_obj_num);
    for (i = 0; i < pstObj->valid_obj_num; i++)
    {
        DBG("i: %d, id: %d, type: %d, xywh[%d %d %d %d]strength: %f, life: %02x\n", i, pstObj->obj[i].id, pstObj->obj[i].type, 
            pstObj->obj[i].x, pstObj->obj[i].y, pstObj->obj[i].width, pstObj->obj[i].height, pstObj->obj[i].strength, pstObj->obj[i].life);
    }

    return 0;
}

static void* t01_user_thread_0(void *data)
{
    prctl(PR_SET_NAME, __FUNCTION__);
    
    int ret = -1;
    //int i = 0;
    md_context_t md_context = 0;
    IngDetDesc desc;
    memset(&desc, 0, sizeof(desc));
    
    g_t01_args->md_channel = IHwDet_Create_Channel(MD_NOBLOCK);
    CHECK(g_t01_args->md_channel != 0, NULL, "Error with: %#x\n", g_t01_args->md_channel);

    ret = IHwDet_Enable_Tracking(g_t01_args->md_channel, TRACKING_TYPE_FACE, DEFAULT_STRENGTH_FILTER);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    while (g_t01_args->running) 
    {
        memset(&desc, 0, sizeof(desc));
        memset(&g_t01_args->stHwDetObj, 0, sizeof(g_t01_args->stHwDetObj));
        md_context = IHwDet_Request_MD(g_t01_args->md_channel, &desc);
        if (md_context) 
        {
            pthread_mutex_lock(&g_t01_args->mutex);
            
            memcpy(&g_t01_args->stHwDetObj.desc, &desc, sizeof(desc));
            g_t01_args->stHwDetObj.usrPrivateCount = t01_transferPrivateCount(desc._private);
            
            ret = t01_process_result(md_context, &g_t01_args->stHwDetObj);
            CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
            
            ret = IHwDet_Release_MD(g_t01_args->md_channel, md_context);
            CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
            
            pthread_mutex_unlock(&g_t01_args->mutex);
        }
        
        usleep(10*1000);
    }
    
    ret = IHwDet_Disable_Tracking(g_t01_args->md_channel);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    ret = IHwDet_Destroy_Channel(g_t01_args->md_channel);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    return NULL;
}

static void* t01_user_thread_1(void *data)
{
    prctl(PR_SET_NAME, __FUNCTION__);

    int ret = -1;
    //int i = 0;
    md_context_t md_context = 0;
    IngDetDesc_T desc;
    memset(&desc, 0, sizeof(desc));

    g_t01_args->md_channel = IHwDet_Create_Channel_T(MD_NOBLOCK);
    CHECK(g_t01_args->md_channel != 0, NULL, "Error with: %#x\n", g_t01_args->md_channel);

    //ret = IHwDet_Enable_Tracking(g_t01_args->md_channel, TRACKING_TYPE_FACE, DEFAULT_STRENGTH_FILTER);
    //CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    while (g_t01_args->running) 
    {
        memset(&desc, 0, sizeof(desc));
        memset(&g_t01_args->stHwDetObj, 0, sizeof(g_t01_args->stHwDetObj));
        md_context = IHwDet_Request_MD_T(g_t01_args->md_channel, &desc);
        if (md_context) 
        {
            pthread_mutex_lock(&g_t01_args->mutex);
            
            memcpy(&g_t01_args->stHwDetObj.desc_T, &desc, sizeof(desc));
            g_t01_args->stHwDetObj.usrPrivateCount = t01_transferPrivateCount(desc._private);

            ret = t01_process_result_T(md_context, &g_t01_args->stHwDetObj);
            CHECK(ret == 0, NULL, "Error with %#x.\n", ret);

            ret = IHwDet_Release_MD_T(g_t01_args->md_channel, md_context);
            CHECK(ret == 0, NULL, "Error with %#x.\n", ret);
            
            pthread_mutex_unlock(&g_t01_args->mutex);
        }

        usleep(10*1000);
    }

    ret = IHwDet_Disable_Tracking(g_t01_args->md_channel);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    ret = IHwDet_Destroy_Channel_T(g_t01_args->md_channel);
    CHECK(ret == 0, NULL, "Error with: %#x\n", ret);

    return NULL;
}

int sal_t01_init()
{
    CHECK(NULL == g_t01_args, -1, "reinit error, please uninit first.\n");

    int ret = -1;
    g_t01_args = (sal_t01_args*)malloc(sizeof(sal_t01_args));
    CHECK(NULL != g_t01_args, -1, "malloc %d bytes failed.\n", sizeof(sal_t01_args));

    memset(g_t01_args, 0, sizeof(sal_t01_args));
    pthread_mutex_init(&g_t01_args->mutex, NULL);

    ret = t01_ioattr_init(&g_t01_args->ioattr, 1920, 1080, 30);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = IHwDet_Init(&g_t01_args->ioattr);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);

    ret = IHwDet_Start(META_DATA_OBJECT, THUMBNAILS_ENABLE);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = IHwDet_Skip_Frame_Count(0);
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    //ret = IHwDet_Time_Sync(1, 1);
    //CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    g_t01_args->running = 1;
    ret = pthread_create(&g_t01_args->pid, NULL, t01_user_thread_0, NULL);
    //ret = pthread_create(&g_t01_args->pid, NULL, t01_user_thread_1, NULL);
    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));

    return 0;
}

int sal_t01_exit()
{
    CHECK(NULL != g_t01_args, HI_FAILURE, "module not inited.\n");
    
    int ret = -1;
    
    //IHwDet_Set_Channel_Block(g_t01_args->md_channel, MD_NOBLOCK);
    
    if (g_t01_args->running)
    {
        g_t01_args->running = 0;
        pthread_join(g_t01_args->pid, NULL);
    }

    ret = IHwDet_Stop();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    ret = IHwDet_DeInit();
    CHECK(ret == 0, -1, "Error with: %#x\n", ret);
    
    free(g_t01_args);
    g_t01_args = NULL;
    
    DBG("done.\n");
    return 0;
}

int sal_t01_HwDetObjGet(HwDetObj* pstHwDetObj)
{
    CHECK(NULL != g_t01_args, HI_FAILURE, "module not inited.\n");
    CHECK(NULL != pstHwDetObj, HI_FAILURE, "invalid parameter with: %#x\n", pstHwDetObj);
    
    pthread_mutex_lock(&g_t01_args->mutex);
    
    memcpy(pstHwDetObj, &g_t01_args->stHwDetObj, sizeof(g_t01_args->stHwDetObj));
    
    pthread_mutex_unlock(&g_t01_args->mutex);
    return 0;
}


