#ifndef __SAL_BITMAP_H__
#define __SAL_BITMAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define FORE_GROUND 'X'
#define BACK_GROUND '-'
#define FONT_EDGE 'o'

#define MAX_LINE_NUM 20
#define MAX_LINE_PLL 100

typedef struct
{
    int row;
    int col;
    char* bit_map;
}result_s;

int bitmap_create(const char* hz_path, int is_gbk, const char* asc_path);
int bitmap_destroy();
int bitmap_alloc(const unsigned char* chs, int font_size, int edge_enable, int gap, result_s* result);
int bitmap_free(result_s* result);
int bitmap_demo();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
