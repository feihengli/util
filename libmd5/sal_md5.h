#ifndef __SAL_MD5_H__
#define __SAL_MD5_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

/*
 函 数 名: md5_filesum
 功能描述: 计算文件的md5值
 输入参数: _szFilePath 文件路径
 输出参数: sum 文件的md5值
 返 回 值: 成功返回0,失败返回小于0
*/
int md5_filesum(char* _szFilePath, unsigned char sum[64]);

/*
 函 数 名: md5_memsum
 功能描述: 计算buf的md5值
 输入参数: input 输入buf
            len buf的长度
 输出参数: sum 文件的md5值
 返 回 值: 成功返回0,失败返回小于0
*/
int md5_memsum(char* input, int len, unsigned char sum[64]);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
