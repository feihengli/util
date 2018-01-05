#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>

#include "sal_font.h"
#include "sal_bitmap.h"
#include "sal_debug.h"

typedef struct tag_bitmap_s
{
    int inited;
    int gbk; // 1: gbk 0: gb2312

    char hz16x16[32];
    FILE* hz_fp;
    char asc16x8[32];
    FILE* asc_fp;
}bitmap_s;

static bitmap_s bitmap;

// gb2312 简体 16x16
static int bitmap_read_hzk16(const unsigned char *s, char* chs)
{
    int ret = -1;

    int size = 32;
    unsigned long offset = ((s[0]-0xa1)*94+(s[1]-0xa1))*size;

    ret = fseek(bitmap.hz_fp, offset, SEEK_SET);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    ret = fread(chs, 1, size, bitmap.hz_fp);
    CHECK(ret == size, -1, "error: read %d bytes, expect %d bytes\n", ret, size);

    return 0;
}

// gbk 字库 16x16 兼容gb2312简体 扩展了繁体
static int bitmap_read_gbk16(const unsigned char *s, char* chs)
{
    int ret = -1;

    int size = 32;
    int qh = s[0] - 0x81;
    int wh = (s[1] < 0x7f) ? (s[1] - 0x40) : (s[1] - 0x41);
    unsigned long offset=(qh*190+wh)*size;

    ret = fseek(bitmap.hz_fp, offset, SEEK_SET);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    ret = fread(chs, 1, size, bitmap.hz_fp);
    CHECK(ret == size, -1, "error: read %d bytes, expect %d bytes\n", ret, size);

    return 0;
}

// ascii 16x8
static int bitmap_read_asc16(const unsigned char *s, char* asc)
{
    int ret = -1;

    int size = 16;
    unsigned long offset=s[0]*size;

    ret = fseek(bitmap.asc_fp, offset,SEEK_SET);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    ret = fread(asc,1,size, bitmap.asc_fp);
    CHECK(ret == size, -1, "error: read %d bytes, expect %d bytes\n", ret, size);

    return 0;
}

static int bitmap_show_str(const char* p, int row, int col)
{
    int i = 0;
    for (i = 0; i < row*col; i++)
    {
        if (i%col == 0)
        {
            printf("\n");
        }
        printf("%c", p[i]);
    }
    printf("\n\n");
    return 0;
}

static int bitmap_show_hex(const unsigned char* begin)
{
    int len = strlen((const char*)begin);
    int i = 0;

    for (i = 0; i < len; i++)
    {
        printf("%02x ", begin[i]);
    }

    printf("\n");

    return 0;
}

static int bitmap_bit2char(const char* chs, int size, char* font)
{
    int i = 0;
    int j = 0;
    int idx = 0;

    for (i = 0; i < size; i++)
    {
        for (j = 7; j >= 0; j--)
        {
            if (chs[i]&(0x1<<j))
            {
                font[idx++] = FORE_GROUND;
            }
            else
            {
                font[idx++] = BACK_GROUND;
            }
        }
    }
    return 0;
}

// 四周再包一个像素
static int bitmap_add_edge(const char* src, int row, int col, char* dst)
{
    row += 2;
    col += 2;
    memset(dst, BACK_GROUND, row*col);

    int i = 0;
    int j = 0;
    int k = 0;

    for (i = 1; i < row-1; i++)
    {
        for (j = 1; j < col-1; j++)
        {
            int idx = i*col+j;
            dst[idx] = src[k++];
        }
    }

    for (i = 1; i < row-1; i++)
    {
        for (j = 1; j < col-1; j++)
        {
            int idx = i*col+j;
            if (dst[idx] == FORE_GROUND)
            {
                int top_idx = idx - col;
                int bottom_idx = idx + col;
                int left_idx = idx - 1;
                int right_idx = idx + 1;
                //printf("ij[%d %d] idx: %d, top_idx: %d, bottom_idx: %d, left_idx: %d, right_idx: %d\n", i ,j, idx, top_idx, bottom_idx, left_idx, right_idx);

                dst[top_idx] = (dst[top_idx] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[top_idx-1] = (dst[top_idx-1] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[top_idx+1] = (dst[top_idx+1] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[bottom_idx] = (dst[bottom_idx] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[bottom_idx-1] = (dst[bottom_idx-1] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[bottom_idx+1] = (dst[bottom_idx+1] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[left_idx] = (dst[left_idx] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                dst[right_idx] = (dst[right_idx] == FORE_GROUND) ? FORE_GROUND : FONT_EDGE;
                //printf("dst[top_idx] %c, dst[bottom_idx] %c, dst[left_idx] %c, dst[right_idx] %c\n", dst[top_idx], dst[bottom_idx], dst[left_idx], dst[right_idx]);
            }
        }
    }

    return 0;
}

// 字体等比放大font_size倍,row和col都放大了font_size倍
static int bitmap_font_scale(char* src, int row, int col, char* dst, int font_size)
{
    int i = 0;
    int j = 0;
    int m = 0;
    int n = 0;

    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            char c = src[i*col+j];
            int x = i*font_size;
            int y = j*font_size;
            int dst_col = col*font_size;
            int idx = x*dst_col+y;

            for (m = idx; m < idx+font_size*dst_col; m += dst_col)
            {
                for (n = 0; n < font_size; n++)
                {
                    dst[m+n] = c;
                }
            }
        }
    }

    return 0;
}

static int bitmap_draw(font_s** sentence, int num, char* dst, int row, int col)
{
    int i = 0;
    int j = 0;
    char* begin = dst;
    int offset[MAX_LINE_PLL];
    memset(offset, 0, sizeof(offset));
    int total_col = 0;

    for (i = 0; i < row; i++)
    {
        total_col = 0;
        for (j = 0; j < num; j++)
        {
            if (!sentence[j])
            {
                if (total_col < col)
                {
                    //printf("total_col: %d, col: %d\n", total_col, col);
                    memset(begin, BACK_GROUND, col-total_col);
                    begin += col-total_col;
                }
                break;
            }
            memcpy(begin, sentence[j]->array+offset[j], sentence[j]->col);
            offset[j] += sentence[j]->col;
            begin += sentence[j]->col;
            total_col += sentence[j]->col;
        }
    }

    return 0;
}

static int bitmap_get_info(font_s** sentence, int total, int* size)
{
    int i = 0;
    int num = 0;
    int sum = 0;

    for (i = 0; i < total; i++)
    {
        if (!sentence[i])
        {
            break;
        }

        sum += sentence[i]->row * sentence[i]->col;
        num++;
    }

    *size = sum;

    return 0;
}

static int bitmap_font_add(const unsigned char* chs, int len, int font_size, int edge_enable)
{
    int ret = -1;
    char tmp[32];
    memset(tmp, 0, sizeof(tmp));
    char* out = NULL;

    char font16x16[16*16];
    char font16x8[16*8];
    char* base_font = NULL;
    int base_row = 16;
    int base_col = 16;

    if (len == 2)
    {
        base_font = font16x16;
        ret = (bitmap.gbk) ? bitmap_read_gbk16(chs, tmp) : bitmap_read_hzk16(chs, tmp);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        ret = bitmap_bit2char(tmp, base_row*len, base_font);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
    }
    else if (len == 1)
    {
        base_col = 8;
        base_font = font16x8;
        ret = bitmap_read_asc16(chs, tmp);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
        ret = bitmap_bit2char(tmp, base_row*len, base_font);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
    }
    else
    {
        printf("Unknow len: %d\n", len);
        return -1;
    }

    int row = base_row*font_size;
    int col = base_col*font_size;
    char* fontWxH = (char*)malloc(row*col);
    CHECK(fontWxH, -1, "failed to malloc %d bytes\n", row*col);

    ret = bitmap_font_scale(base_font, base_row, base_col, fontWxH, font_size);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);
    out = fontWxH;

    char* fontWxH_edge = NULL;
    if (edge_enable)
    {
        row += 2;
        col += 2;
        fontWxH_edge = (char*)malloc(row*col);
        CHECK(fontWxH_edge, -1, "failed to malloc %d bytes\n", row*col);

        ret = bitmap_add_edge(fontWxH, row-2, col-2, fontWxH_edge);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
        out = fontWxH_edge;
    }

    ret = font_add(chs, len, out, row, col, font_size);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    free(fontWxH_edge); // safe to free NULL
    free(fontWxH);

    return ret;
}

static int bitmap_font_get(const unsigned char* chs, int font_size, int edge_enable, font_s** sentence)
{
    int ret = -1;
    const unsigned char* begin = chs;
    int len = strlen((const char*)chs);
    int idx = 0;

    while (len > 0)
    {
        int enc_len = 1;
        if (*begin > 0x80) // 区号大于0x80的为汉字
        {
            enc_len = 2;
        }

        font_s* _font = font_get(begin, enc_len, font_size);
        if (!_font)
        {
            ret = bitmap_font_add(begin, enc_len, font_size, edge_enable);
            CHECK(ret == 0, -1, "Error with %#x\n", ret);

            _font = font_get(begin, enc_len, font_size);
            CHECK(_font, -1, "Error: null pointer\n");
        }
        sentence[idx++] = _font;

        begin += enc_len;
        len -= enc_len;
    }

    return 0;
}

static int bitmap_font_release(font_s** sentence, int total)
{
    int ret = -1;
    int i = 0;

    for (i = 0; i < total; i++)
    {
        if (!sentence[i])
        {
            break;
        }

        ret = font_release(sentence[i]);
        CHECK(ret == 0, -1, "error with %#x\n", ret);
    }

    return 0;
}

static int bitmap_parse_line(unsigned char* src, int* maxlen, int* linecnt)
{
    unsigned char* begin = (unsigned char*)src;
    int linelen[MAX_LINE_NUM];
    int max_line_len = 0;
    int linecount = 0;
    unsigned char *line_begin = (unsigned char*)src;

    while (*begin)
    {
        if (*begin == '^')
        {
            linelen[linecount] = begin - line_begin;
            if (max_line_len < linelen[linecount])
            {
                max_line_len = linelen[linecount];
            }
            linecount++;
            line_begin = begin + 1;
        }
        begin++;
    }

    linelen[linecount] = strlen((const char*)line_begin);
    if(max_line_len < linelen[linecount])
        max_line_len = linelen[linecount];

    linecount++;

    *maxlen = max_line_len;
    *linecnt = linecount;

    return 0;
}

static int bitmap_get_matrix(unsigned char* src, unsigned char* dst, int* maxlen, int* linecnt)
{
    int ret = -1;

    ret= bitmap_parse_line(src, maxlen, linecnt);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    int size = (*maxlen)*(*linecnt);
    memset(dst, ' ', size); // 用空格清空
    //printf("linecount: %d, max_line_len: %d\n", (*linecnt), (*maxlen));

    char* s_begin = (char*)src;
    char* d_begin = (char*)dst;
    char* find = NULL;
    do
    {
        find = strstr((char*)s_begin, "^");
        if (find)
        {
            memcpy(d_begin, s_begin, find-s_begin);
            find++;
            s_begin = find;
            d_begin += (*maxlen);
        }
        else // the last line
        {
            memcpy(d_begin, s_begin, strlen(s_begin));
        }
    }while(find);

    //printf("src[%d]: %s, dst[%d]: %s\n", strlen(src), src, strlen(dst), dst);

    return 0;
}

int bitmap_create(const char* hz_path, int is_gbk, const char* asc_path)
{
    CHECK(hz_path, -1, "Error with %p\n", hz_path);
    CHECK(asc_path, -1, "Error with %p\n", asc_path);
    CHECK(!bitmap.inited, -1, "Error with %#x\n", bitmap.inited);

    int ret = -1;
    memset(&bitmap, 0, sizeof(bitmap));

    bitmap.gbk = (is_gbk) ? 1 : 0;
    strcpy(bitmap.hz16x16, hz_path);
    bitmap.hz_fp = fopen(hz_path, "r");
    CHECK(bitmap.hz_fp, -1, "failed to open %s: %s\n", hz_path, strerror(errno));

    strcpy(bitmap.asc16x8, asc_path);
    bitmap.asc_fp = fopen(asc_path, "r");
    CHECK(bitmap.asc_fp, -1, "failed to open %s: %s\n", asc_path, strerror(errno));

    ret = font_init();
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    bitmap.inited = 1;

    return 0;
}

int bitmap_destroy()
{
    CHECK(bitmap.inited, -1, "Error with %#x\n", bitmap.inited);
    int ret = -1;

    if (bitmap.hz_fp)
    {
        fclose(bitmap.hz_fp);
        bitmap.hz_fp = NULL;
    }
    if (bitmap.asc_fp)
    {
        fclose(bitmap.asc_fp);
        bitmap.asc_fp = NULL;
    }

    ret = font_exit();
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    return 0;
}

int bitmap_alloc(const unsigned char* chs, int font_size, int edge_enable, int gap, result_s* result)
{
    CHECK(bitmap.inited, -1, "Error with %#x\n", bitmap.inited);
    int ret = -1;
    memset(result, 0, sizeof(*result));
    int linecnt = 0;
    int maxlen = 0;
    unsigned char dst[MAX_LINE_NUM*MAX_LINE_PLL+1];
    memset(dst, 0, sizeof(dst));

    ret = bitmap_get_matrix((unsigned char*)chs, dst, &maxlen, &linecnt);
    CHECK(ret == 0, -1, "Error with %#x\n", ret);

    font_s* sentence[MAX_LINE_NUM][MAX_LINE_PLL];
    memset(sentence, 0, sizeof(sentence));

    int i = 0;
    unsigned char tmp[MAX_LINE_PLL+1];
    unsigned char* d_begin = dst;
    int max_col = 0;

    for (i = 0; i < linecnt; i++)
    {
        memset(tmp, 0, sizeof(tmp));
        memcpy(tmp, d_begin, maxlen);
        d_begin += maxlen;
        ret= bitmap_font_get(tmp, font_size, edge_enable, sentence[i]);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        int size = 0;
        ret = bitmap_get_info(sentence[i], MAX_LINE_PLL, &size);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);

        int col = size/sentence[i][0]->row;
        if (max_col < col)
        {
            max_col = col;
        }
    }

    result->row = sentence[0][0]->row*linecnt + (linecnt-1)*gap;
    result->col = max_col;
    result->bit_map =  (char*)malloc(result->row*result->col);
    CHECK(result->bit_map, -1, "failed to malloc %d bytes\n", result->row*result->col);

    char* begin = result->bit_map;
    for (i = 0; i < linecnt; i++)
    {
        ret = bitmap_draw(sentence[i], MAX_LINE_PLL, begin, sentence[i][0]->row, result->col);
        CHECK(ret == 0, -1, "Error with %#x\n", ret);
        //show_str(begin, sentence[i][0]->row, result->col);
        begin += sentence[i][0]->row*result->col;

        if (i < linecnt-1)
        {
            memset(begin, BACK_GROUND, result->col*gap);
            begin += result->col*gap;
        }

        ret = bitmap_font_release(sentence[i], MAX_LINE_PLL);
        CHECK(ret == 0, -1, "error with %#x\n", ret);
    }

    return 0;
}

int bitmap_free(result_s* result)
{
    CHECK(bitmap.inited, -1, "Error with %#x\n", bitmap.inited);
    if (result->bit_map)
    {
        free(result->bit_map);
    }

    return 0;
}

int bitmap_demo()
{
    bitmap_create("gbk.dzk", 1, "asc16.dzk");

    const unsigned char chs[] = "我们";
    bitmap_show_hex(chs);

    result_s result;
    memset(&result, 0, sizeof(result));

    bitmap_alloc(chs, 1, 0, 8, &result);

    bitmap_show_str(result.bit_map, result.row, result.col);;

    bitmap_free(&result);

    bitmap_destroy();

    return 0;
}


