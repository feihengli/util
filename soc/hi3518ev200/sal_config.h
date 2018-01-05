#ifndef __SAL_CONFIG_H__
#define __SAL_CONFIG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef struct sal_config_video_s
{
    int enable;
    char type[32];
    char resolution[32];
    int width;
    int height;
    char profile[32];
    int fps;
    int gop;
    int bitrate;
    char bitrate_ctrl[32];
}sal_config_video_s;

typedef struct sal_config_jpeg_s
{
    int enable;
    int channel; //对应vpss channel id
    int widht;
    int height;
}sal_config_jpeg_s;

typedef struct sal_config_md_s
{
    int enable;
    int channel; //对应vpss channel id
    int widht;
    int height;
}sal_config_md_s;

typedef struct sal_config_lbr_s
{
    int enable;
    int bitrate;
}sal_config_lbr_s;

typedef struct sal_config_s
{
    char version[32];
    sal_config_video_s video[2];
    sal_config_md_s md;
    sal_config_md_s jpeg;
    sal_config_lbr_s lbr[2];
}sal_config_s;

extern sal_config_s g_config;


int sal_config_default(sal_config_s* config);
int sal_config_save(char* path, sal_config_s* config);
int sal_config_parse(char* path, sal_config_s* config);
int sal_config_parse_data();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
