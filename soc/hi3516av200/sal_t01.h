
#ifndef sal_t01_h__
#define sal_t01_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#include "sal_standard.h"
#include "t01/object.h"
#include "t01/ioattr.h"
#include "t01/ihwdet.h"
#include "t01/ext.h"
#include "t01/event.h"
#include "t01/debug.h"

typedef struct _HwDetObj
{
    unsigned int usrPrivateCount;
    IngDetDesc desc;
    IngDetDesc_T desc_T;
    IngDetObject obj[128];
    int valid_obj_num;
}HwDetObj;


int sal_t01_init();
int sal_t01_exit();
int sal_t01_HwDetObjGet(HwDetObj* pstHwDetObj);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

