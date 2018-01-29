
#ifndef sal_jpeg1_h__
#define sal_jpeg1_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif


int jpeg1_create_chn(int channel, int widht, int height);
int jpeg1_destroy_chn(int channel);
int jpeg1_set_Qfactor(int channel, int Qfactor);
int jpeg1_get_picture(int channel, unsigned char** out, int* len);
int jpeg1_release_picture(int channel, unsigned char** out, int* len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

