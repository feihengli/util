#ifndef __SAL_FONT_H__
#define __SAL_FONT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define MAX_FONT_NUM 128

typedef struct
{
    int reference;
    int enc_len;
    unsigned char enc[2];
    int font_size; // default font size 1
    int row;
    int col;
    unsigned char* array; // row*col
}font_s;

int font_init();
int font_exit();
int font_add(const unsigned char* enc, int len, char* src, int row, int col, int font_size);
font_s* font_get(const unsigned char* enc, int len, int font_size);
int font_release(font_s* font);
int font_get_num();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
