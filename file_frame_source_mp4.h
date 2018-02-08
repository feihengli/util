#ifndef file_frame_source_mp4_h__
#define file_frame_source_mp4_h__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

typedef int (*video_frame_cb)(int stream, unsigned char *frame, unsigned long len, int key, double pts);

handle frame_source_mp4_init(const char* path, video_frame_cb frame_cb);
int frame_source_mp4_exit(handle hndMp4);




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
