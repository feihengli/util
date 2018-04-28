#ifdef hi3516av200
#include "sal_standard.h"
#include "sal_av.h"
#include "sal_vo.h"
#include "sal_overlay.h"
#include "sal_util.h"
#include "sal_debug.h"
#include "sal_bitmap.h"
#include "sal_osd.h"
#include "sal_config.h"
#include "sal_statistics.h"
#include "sal_isp.h"
#include "sal_lbr.h"
#include "sal_af.h"
#include "sal_jpeg.h"
#include "sal_audio.h"
#include "sal_draw_rectangle.h"
#include "sal_himm.h"
#include "sal_vi_vpss.h"
#include "sal_t01.h"

/*
* 解决编译问题
* 此函数只是为了生成libipc.so的时候，把函数符号链接进libipc.so。
* 如果不调用，那么生成libipc.so的时候，会自动剔除掉函数符号。暂不明原因
*/

int sal_assist()
{
	sal_jpeg_init(NULL);
    sal_dr_init();
    extern int vo_init();
    vo_init();
    sal_audio_init(NULL);
    extern int sal_pc_init();
    sal_pc_init();
    //extern int sal_sys_init2();
    //sal_sys_init2();
    extern int sal_sys_init1(sal_video_s* video);
    sal_sys_init1(NULL);
    extern int sal_spi_init();
    sal_spi_init();
    
    himm_read(0);
    
    sal_vv_init();
    sal_t01_init();
	
	return 0;
}

#endif

#ifdef hi3518ev200
#include "sal_standard.h"
#include "sal_overlay.h"
#include "sal_util.h"
#include "sal_debug.h"
#include "sal_bitmap.h"
#include "sal_osd.h"
#include "sal_config.h"
#include "sal_statistics.h"
#include "sal_isp.h"
#include "sal_lbr.h"
#include "sal_af.h"
#include "sal_jpeg.h"

/*
* 解决编译问题
* 此函数只是为了生成libipc.so的时候，把函数符号链接进libipc.so。
* 如果不调用，那么生成libipc.so的时候，会自动剔除掉函数符号。暂不明原因
*/

int sal_assist()
{
    sal_jpeg_init(NULL);
    sal_dr_init();

    return 0;
}

#endif
