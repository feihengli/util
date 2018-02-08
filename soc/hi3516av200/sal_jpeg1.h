
#ifndef sal_jpeg1_h__
#define sal_jpeg1_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/*
 函 数 名: jpeg1_create_chn
 功能描述: 创建JPEG编码通道，调HI_MPI_VENC_SendFrame手动送帧
 输入参数: channel VENC 编码通道，不能与其它的编码通道冲突
            widht 宽
            height 高
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int jpeg1_create_chn(int channel, int widht, int height);

/*
 函 数 名: jpeg1_destroy_chn
 功能描述: 销毁JPEG编码通道
 输入参数: channel VENC 编码通道，不能与其它的编码通道冲突
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int jpeg1_destroy_chn(int channel);

/*
 函 数 名: jpeg1_set_Qfactor
 功能描述: 设置JPEG质量 1-100
 输入参数: channel VENC 编码通道，不能与其它的编码通道冲突
            Qfactor 质量 1-100
 输出参数:
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int jpeg1_set_Qfactor(int channel, int Qfactor);

/*
 函 数 名: jpeg1_get_picture
 功能描述: 取jpeg编码后的图像
 输入参数: channel VENC 编码通道，不能与其它的编码通道冲突
 输出参数: out 输出图像
            len 输出图像长度
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int jpeg1_get_picture(int channel, unsigned char** out, int* len);

/*
 函 数 名: jpeg1_release_picture
 功能描述: 释放图像资源
 输入参数: channel VENC 编码通道，不能与其它的编码通道冲突
            in 图像
            len 图像长度
 输出参数: 无
 返 回 值: 设置成功返回0, 失败返回小于0
*/
int jpeg1_release_picture(int channel, unsigned char** in, int* len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

