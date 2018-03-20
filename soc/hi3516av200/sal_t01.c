#include "hi_comm.h"
#include "sal_debug.h"
#include "sal_util.h"
#include "sal_vgs.h"
#include "sal_t01.h"

#include "t01/object.h"
#include "t01/ioattr.h"
#include "t01/ihwdet.h"
#include "t01/ext.h"
#include "t01/event.h"
#include "t01/debug.h"


#define SENSOR_FPS	25

int stop = 0;
pthread_t tid_user_0;
pthread_t tid_user_1;
pthread_t tid_user_2;
int user_0_md_channel;
int user_1_md_channel;
int user_2_md_channel;

IHwDetAttr g_ioattr;

extern void osd_render(int type, int x, int y, int w, int h, int tilt, int yaw, unsigned long long ts);
extern void osd_render_cls(void);

void osd_render(int type, int x, int y, int w, int h, int tilt, int yaw, unsigned long long ts)
{
    
}

void osd_render_cls(void)
{

}

static void ioattr_init(void)
{
	/* input */
	g_ioattr.input.interface = INPUT_INTERFACE_DVP_SONY;
	g_ioattr.input.data_type = INPUT_DVP_DATA_TYPE_RAW12;
	g_ioattr.input.image_width = 1920;
	g_ioattr.input.image_height = 1080;
	g_ioattr.input.raw_rggb = 0;
	g_ioattr.input.raw_align = 1;
	g_ioattr.input.blanking.hfb_num = 11;
	g_ioattr.input.blanking.vic_input_vpara3 = 17;
	g_ioattr.input.config.dvp_bus_select = 0;
	g_ioattr.input.fps = SENSOR_FPS;
	g_ioattr.input.mclk_enable = 0;

	/* output only for MIPI bypass mode */
}

static void *user_thread_0(void *data)
{
	int i;
	int frame_cnt = 0;
	char *user_0_log_file = (char *)"user_0.log";
	char log_buf[1024];

	prctl(PR_SET_NAME, "user_thread_0");

	FILE *fp = fopen(user_0_log_file, "a");
	if (!fp) {
		DBG("Can't create or open file [%s]\n", user_0_log_file);
		return NULL;
	}
	DBG("USER 0 PROCESS: store result to [%s]\n", user_0_log_file);

	while (1) {
		int md_context = 0;
		IngDetDesc desc;

		if (stop == 1)
			break;

		md_context = IHwDet_Request_MD(user_0_md_channel, &desc);

		if (md_context) {
			sprintf(log_buf, "\n Frame %d, objects %d\n", desc.frame, desc.objnum);
			fwrite(log_buf, 1, strlen(log_buf), fp);

			for (i = 0; i < desc.objnum; i++) {
				IngDetObject *obj;

				obj = IHwDet_Get_Object(md_context, i);
				if(!obj){
					DBG("ERROR: failed to parse Object of user 0!\n");
					break;
				}

				 /*osd_render(obj->type, obj->x, obj->y, */
						 /*obj->width, obj->height, obj->tilt, */
						 /*obj->yaw, desc.ts); */

				sprintf(log_buf, "\t OBJECT:\n\t\tid:%d\n\t\t type:%d\n\t\t x:%d\n"
					"\t\t y:%d\n\t\t width:%d\n\t\t height:%d\n"
					"\t\t yaw:%d\n\t\t tilt:%d\n\t\t strength:%f\n"
					"\t\t parents:[(%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d)]\n",
					obj->id, obj->type, obj->x, obj->y, obj->width,
					obj->height, obj->yaw, obj->tilt, obj->strength,
					obj->parents[0].type, obj->parents[0].offset,
					obj->parents[1].type, obj->parents[1].offset,
					obj->parents[2].type, obj->parents[2].offset,
					obj->parents[3].type, obj->parents[3].offset,
					obj->parents[4].type, obj->parents[4].offset);
				fwrite(log_buf, 1, strlen(log_buf), fp);
			}

			frame_cnt++;

			if (!(frame_cnt % (SENSOR_FPS * 5))) {
				DBG("USER 0: detection frame count = [%d]\n", frame_cnt);
			}

			 /*osd_render_cls(); */
			IHwDet_Release_MD(user_0_md_channel, md_context);
		}
	}

	fclose(fp);

	return NULL;
}

static void *user_thread_1(void *data)
{
	int i;
	int frame_cnt = 0;

	prctl(PR_SET_NAME, "user_thread_1");

	while (1) {
		int md_context = 0;
		IngDetDesc desc;

		if (stop == 1)
			break;

		md_context = IHwDet_Request_MD(user_1_md_channel, &desc);

		if (md_context) {
			for (i = 0; i < desc.objnum; i++) {
				IngDetObject *obj;

				obj = IHwDet_Get_Object(md_context, i);
				if(!obj){
					DBG("ERROR: failed to parse Object of user 1!\n");
					break;
				}
			}

			frame_cnt++;

			if (!(frame_cnt % (SENSOR_FPS * 5))) {
				DBG("USER 1: detection frame count = [%d]\n", frame_cnt);
			}

			IHwDet_Release_MD(user_1_md_channel, md_context);
		} else {
			usleep(1000000 / SENSOR_FPS);
		}
	}

	return NULL;
}

static void *user_thread_2(void *data)
{
	int i;
	char *user_2_log_file = (char *)"user_2.log";
	char log_buf[1024];

	prctl(PR_SET_NAME, "user_thread_2");

	FILE *fp = fopen(user_2_log_file, "a");
	if (!fp) {
		DBG("Can't create or open file [%s]\n", user_2_log_file);
		return NULL;
	}
	DBG("USER 2 PROCESS: store result to [%s]\n", user_2_log_file);

	while (1) {
		int md_context = 0;
		IngDetDesc_T desc_T;

		if (stop == 1)
			break;

		md_context = IHwDet_Request_MD_T(user_2_md_channel, &desc_T);

		if (md_context) {
			sprintf(log_buf, "\n Frame %d, targets %d\n", desc_T.frame, desc_T.tgtnum);
			fwrite(log_buf, 1, strlen(log_buf), fp);

			for (i = 0; i < desc_T.tgtnum; i++) {
				IngTarget_T *target = IHwDet_Get_Target(md_context, i);
				if (target) {
					osd_render(OBJ_TYPE_PERSON, target->x, target->y,
					           target->width, target->height, target->tilt,
					           target->yaw, desc_T.ts);

					sprintf(log_buf, "\t Target %d\n\t id:%d\n\t type:%d\n"
						"\t x:%d\n\t y:%d\n\t width:%d\n\t height:%d\n"
						"\t yaw:%d\n\t tilt:%d\n\t strength:%f\n", i, target->id,
						target->type, target->x, target->y, target->width,
						target->height, target->yaw, target->tilt, target->strength);
					fwrite(log_buf, 1, strlen(log_buf), fp);

					if (IHwDet_Target_Have_Face(target)) {
						IngDetObject *obj_face = IHwDet_Target_Get_Object(target, OBJ_TYPE_FACE);
						if (!obj_face) {
							DBG("ERROR: failed to get face of target!\n");
							continue;
						}
						osd_render(OBJ_TYPE_FACE, obj_face->x, obj_face->y,
						          obj_face->width, obj_face->height, obj_face->tilt,
						          obj_face->yaw, desc_T.ts);

						sprintf(log_buf, "\t\t FACE:\n\t\t\tid:%d\n\t\t\t type:%d\n\t\t\t x:%d\n"
							"\t\t\t y:%d\n\t\t\t width:%d\n\t\t\t height:%d\n"
							"\t\t\t yaw:%d\n\t\t\t tilt:%d\n\t\t\t strength:%f\n", obj_face->id,
							obj_face->type, obj_face->x, obj_face->y, obj_face->width,
							obj_face->height, obj_face->yaw, obj_face->tilt, obj_face->strength);
						fwrite(log_buf, 1, strlen(log_buf), fp);
					}

					if (IHwDet_Target_Have_HeadAndShoulder(target)) {
						IngDetObject *obj_hs = IHwDet_Target_Get_Object(target, OBJ_TYPE_HEAD_AND_SHOULDER);
						if (!obj_hs) {
							DBG("ERROR: failed to get head and shoulder of target!\n");
							continue;
						}
						sprintf(log_buf, "\t\t HS:\n\t\t\tid:%d\n\t\t\t type:%d\n\t\t\t x:%d\n"
							"\t\t\t y:%d\n\t\t\t width:%d\n\t\t\t height:%d\n"
							"\t\t\t yaw:%d\n\t\t\t tilt:%d\n\t\t\t strength:%f\n", obj_hs->id,
							obj_hs->type, obj_hs->x, obj_hs->y, obj_hs->width,
							obj_hs->height, obj_hs->yaw, obj_hs->tilt, obj_hs->strength);
						fwrite(log_buf, 1, strlen(log_buf), fp);
					}

					if (IHwDet_Target_Have_Figure(target)) {
						IngDetObject *obj_figure = IHwDet_Target_Get_Object(target, OBJ_TYPE_FIGURE);
						if (!obj_figure) {
							DBG("ERROR: failed to get figure of target!\n");
							continue;
						}
						sprintf(log_buf, "\t\t HS:\n\t\t\tid:%d\n\t\t\t type:%d\n\t\t\t x:%d\n"
							"\t\t\t y:%d\n\t\t\t width:%d\n\t\t\t height:%d\n"
							"\t\t\t yaw:%d\n\t\t\t tilt:%d\n\t\t\t strength:%f\n", obj_figure->id,
							obj_figure->type, obj_figure->x, obj_figure->y, obj_figure->width,
							obj_figure->height, obj_figure->yaw, obj_figure->tilt, obj_figure->strength);
						fwrite(log_buf, 1, strlen(log_buf), fp);
					}
				} else {
					DBG("ERROR: failed to get target!\n");
				}
			}

			osd_render_cls();

			IHwDet_Release_MD_T(user_2_md_channel, md_context);
		}
	}

	fclose(fp);

	return NULL;
}

static void auto_deinit(void)
{
	int ret = 0;

	ret = IHwDet_Stop();
	if(ret != 0) {
		DBG("IHwDet_Stop failed\n");
		return;
	}

	ret = IHwDet_DeInit();
	if(ret != 0) {
		DBG("IHwDet_DeInit failed\n");
		return;
	}
}

static void signal_handler(int signal)
{
	switch (signal) {
	case SIGINT:
	case SIGTSTP:
	case SIGKILL:
	case SIGTERM:
	case SIGSEGV:
		auto_deinit();
		exit(0);
		break;
	}
}

static void signal_init(void)
{
	signal(SIGINT, signal_handler);
	signal(SIGTSTP, signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
}

int __main(void)
{
	int ret = 0;

	//ret = rtsp_server_start();
	//if (ret) {
	//	DBG("rtsp_server_start failed\n");
	//	return -1;
	//}

	DBG("IHwDet_Init ...\n");
	ioattr_init();
	ret = IHwDet_Init(&g_ioattr);
	if(ret != 0) {
		DBG("IHwDet_Init failed\n");
		return -1;
	}
	DBG("IHwDet_Init ... OK!\n");

	DBG("IHwDet_Start ...\n");
	ret = IHwDet_Start(META_DATA_OBJECT, THUMBNAILS_ENABLE);
	if(ret != 0) {
		DBG("IHwDet_Start failed\n");
		return -1;
	}
	DBG("IHwDet_Start ... OK!\n");

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*\
	 + create test for user 0 in block mode (NORMAL Channel)
	\*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	{
		DBG("IHwDet_Create_Channel for user 0 ...\n");
		user_0_md_channel = IHwDet_Create_Channel(MD_BLOCK);
		if (!user_0_md_channel) {
			DBG("IHwDet_Create_Channel for user 0 failed\n");
			return -1;
		}
		DBG("IHwDet_Create_Channel for user 0 ... OK!\n");

		DBG("IHwDet_Enable_Tracking for user 0 ...\n");
		IHwDet_Enable_Tracking(user_0_md_channel, TRACKING_TYPE_UPPER_BODY, DEFAULT_STRENGTH_FILTER);
		DBG("IHwDet_Enable_Tracking for user 0 ... OK!\n");

		DBG("Create process thread for user 0 ...\n");
		ret = pthread_create(&tid_user_0, NULL, user_thread_0, NULL);
		if (ret < 0) {
			DBG("create thread for user 0 error!\n");
			return -1;
		}
		DBG("Create process thread for user 0 ... OK\n");
	}

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*\
	 + create test for user 1 in noblock mode (NORMAL Channel)
	\*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	{
		DBG("IHwDet_Create_Channel for user 1 ...\n");
		user_1_md_channel = IHwDet_Create_Channel(MD_NOBLOCK);
		if (!user_1_md_channel) {
			DBG("IHwDet_Create_Channel for user 1 failed\n");
			return -1;
		}
		DBG("IHwDet_Create_Channel for user 1 ... OK!\n");

		DBG("Create process thread for user 1 ...\n");
		ret = pthread_create(&tid_user_1, NULL, user_thread_1, NULL);
		if (ret < 0) {
			DBG("create thread for user 1 error!\n");
			return -1;
		}
		DBG("Create process thread for user 1 ... OK\n");
	}

	/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*\
	 + create test for user 2 in block mode (T Channel)
	\*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
	{
		DBG("IHwDet_Create_Channel for user 2 ...\n");
		user_2_md_channel = IHwDet_Create_Channel_T(MD_BLOCK);
		if (!user_2_md_channel) {
			DBG("IHwDet_Create_Channel for user 2 failed\n");
			return -1;
		}
		DBG("IHwDet_Create_Channel for user 2 ... OK!\n");

		DBG("Create process thread for user 2 ...\n");
		ret = pthread_create(&tid_user_2, NULL, user_thread_2, NULL);
		if (ret < 0) {
			DBG("create thread for user 2 error!\n");
			return -1;
		}
		DBG("Create process thread for user 2 ... OK\n");
	}

	/* set signal handler */
	signal_init();

	while (stop == 0)
		sleep(1);

	/*-----------------------------------------------------------*\
	 - finish test for user 0 in block mode (NORMAL Channel)
	\*-----------------------------------------------------------*/
	{
		DBG("IHwDet_Set_Channel_Block as MD_NOBLOCK for user 0 ...\n");
		IHwDet_Set_Channel_Block(user_0_md_channel, MD_NOBLOCK);
		DBG("IHwDet_Set_Channel_Block as MD_NOBLOCK for user 0 ... OK\n");

		DBG("Wait user 0 thread finished ...\n");
		pthread_join(tid_user_0, NULL);
		DBG("Wait user 0 thread finished ... OK\n");

		DBG("IHwDet_Destroy_Channel for user 0 ...\n");
		ret = IHwDet_Destroy_Channel(user_0_md_channel);
		if (ret) {
			DBG("IHwDet_Destory_Channel for user 0 failed\n");
			return -1;
		}
		DBG("IHwDet_Destroy_Channel for user 0 ... OK!\n");
	}

	/*-----------------------------------------------------------*\
	 - finish test for user 1 in noblock mode (NORMAL Channel)
	\*-----------------------------------------------------------*/
	{
		DBG("Wait user 1 thread finished ...\n");
		pthread_join(tid_user_1, NULL);
		DBG("Wait user 1 thread finished ... OK\n");

		DBG("IHwDet_Destroy_Channel for user 1 ...\n");
		ret = IHwDet_Destroy_Channel(user_1_md_channel);
		if (ret) {
			DBG("IHwDet_Destory_Channel for user 1 failed\n");
			return -1;
		}
		DBG("IHwDet_Destroy_Channel for user 1 ... OK!\n");
	}

	/*-----------------------------------------------------------*\
	 - finish test for user 2 in block mode (T Channel)
	\*-----------------------------------------------------------*/
	{
		DBG("IHwDet_Set_Channel_Block as MD_NOBLOCK for user 2 ...\n");
		IHwDet_Set_Channel_Block_T(user_2_md_channel, MD_NOBLOCK);
		DBG("IHwDet_Set_Channel_Block as MD_NOBLOCK for user 2 ... OK\n");

		DBG("Wait user 2 thread finished ...\n");
		pthread_join(tid_user_2, NULL);
		DBG("Wait user 2 thread finished ... OK\n");

		DBG("IHwDet_Destroy_Channel for user 2 ...\n");
		ret = IHwDet_Destroy_Channel_T(user_2_md_channel);
		if (ret) {
			DBG("IHwDet_Destory_Channel for user 2 failed\n");
			return -1;
		}
		DBG("IHwDet_Destroy_Channel for user 2 ... OK!\n");
	}

	DBG("IHwDet_Stop ...\n");
	ret = IHwDet_Stop();
	if(ret != 0) {
		DBG("IHwDet_Stop failed\n");
		return -1;
	}
	DBG("IHwDet_Stop ... OK!\n");

	DBG("IHwDet_DeInit ...\n");
	ret = IHwDet_DeInit();
	if(ret != 0) {
		DBG("IHwDet_DeInit failed\n");
		return -1;
	}
	DBG("IHwDet_DeInit ... OK!\n");

	return 0;
}


//typedef struct sal_t01_args
//{
//    struct timeval current;
//    int vpss_chn;
//
//    int running;
//    pthread_t pid;
//    pthread_mutex_t mutex;
//}sal_t01_args;
//
//static sal_t01_args* g_t01_args = NULL;
//
//
//
//static void* t01_proc(void* args)
//{
//    prctl(PR_SET_NAME, __FUNCTION__);
//
//    int s32Ret = -1;
//    VIDEO_FRAME_INFO_S stFrame;
//    memset(&stFrame, 0, sizeof(stFrame));
//
//    VI_CHN ViChn = 0;
//    HI_U32 u32Depth = 0;
//
//    s32Ret = HI_MPI_VI_GetFrameDepth(ViChn, &u32Depth);
//    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
//
//    if (u32Depth < 1)
//    {
//        u32Depth = 1;
//        DBG("vi_chn: %d, u32Depth: %u\n", ViChn, u32Depth);
//    }
//
//    s32Ret = HI_MPI_VI_SetFrameDepth(ViChn, u32Depth);
//    CHECK(s32Ret == HI_SUCCESS, NULL, "Error with %#x.\n", s32Ret);
//    
//    while (g_t01_args->running)
//    {
//        util_time_abs(&g_t01_args->current);
//        
//        
//        
//        break;
//    }
//
//    return NULL;
//}
//
//static int t01_init_dev()
//{
//    int ret = -1;
//    IHwDetAttr ioattr;
//    memset(&ioattr, 0, sizeof(ioattr));
//    
//    /* input */
//    ioattr.input.interface = INPUT_INTERFACE_DVP_SONY;
//    ioattr.input.data_type = INPUT_DVP_DATA_TYPE_RAW12;
//    ioattr.input.image_width = 1920;
//    ioattr.input.image_height = 1080;
//    ioattr.input.raw_rggb = 0;
//    ioattr.input.raw_align = 1;
//    ioattr.input.blanking.hfb_num = 11;
//    ioattr.input.blanking.vic_input_vpara3 = 17;
//    ioattr.input.config.dvp_bus_select = 0;
//    ioattr.input.fps = 25; //SENSOR_FPS;
//    ioattr.input.mclk_enable = 0;
//
//    /* output only for MIPI bypass mode */
//    ret = IHwDet_Init(&ioattr);
//    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
//    
//    DBG("IHwDet_Init ... OK!\n");
//
//    DBG("IHwDet_Start ...\n");
//    
//    ret = IHwDet_Start(META_DATA_OBJECT, THUMBNAILS_ENABLE);
//    CHECK(ret == 0, -1, "Error with %#x.\n", ret);
//    
//    DBG("IHwDet_Start ... OK!\n");
//    
//    return 0;
//}
//
//int sal_t01_init()
//{
//    CHECK(NULL == g_t01_args, -1, "reinit error, please uninit first.\n");
//
//    int ret = -1;
//
//    g_t01_args = (sal_t01_args*)malloc(sizeof(sal_t01_args));
//    CHECK(NULL != g_t01_args, -1, "malloc %d bytes failed.\n", sizeof(sal_t01_args));
//
//    memset(g_t01_args, 0, sizeof(sal_t01_args));
//    pthread_mutex_init(&g_t01_args->mutex, NULL);
//    
//    g_t01_args->vpss_chn = 1;
//
//    g_t01_args->running = 1;
//    ret = pthread_create(&g_t01_args->pid, NULL, dr_proc1, NULL);
//    CHECK(ret == 0, -1, "Error with %s.\n", strerror(errno));
//
//    return 0;
//}
//
//int sal_t01_exit()
//{
//
//
//    return 0;
//}


