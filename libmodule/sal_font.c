#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <errno.h>

#include "sal_font.h"
#include "sal_debug.h"

typedef struct
{
    int valid_num;
    font_s font[MAX_FONT_NUM];
}font_mgr_s;

static font_mgr_s font_mgr;

static int font_get_idle()
{
    int index = -1;
    unsigned int i = 0;
    for (i = 0; i < MAX_FONT_NUM; i++)
    {
        if (font_mgr.font[i].enc_len == 0) // not used
        {
            index = i;
            break;
        }
    }
    CHECK(i < MAX_FONT_NUM, -1, "failed to found idle node.max: %d\n", MAX_FONT_NUM);

    return index;
}

static int font_clear()
{
    unsigned int i = 0;
    for (i = 0; i < MAX_FONT_NUM; i++)
    {
        if (font_mgr.font[i].reference == 0)
        {
            if (font_mgr.font[i].array)
            {
                free(font_mgr.font[i].array);
            }
            memset(&font_mgr.font[i], 0, sizeof(font_s));
            font_mgr.valid_num--;
        }
    }
    DBG("font lib clear ok\n");

    return 0;
}

int font_add(const unsigned char* enc, int len, char* src, int row, int col, int font_size)
{
    int ret = -1;
    int index = font_get_idle();
    if (-1 == index)
    {
        ret = font_clear();
        CHECK(ret == 0, -1, "error with %#x\n", ret);
        index = font_get_idle();
        CHECK(index >= 0, -1, "error with %#x\n", ret);
    }

    font_mgr.font[index].enc_len = len;
    memcpy(font_mgr.font[index].enc, enc, len);
    font_mgr.font[index].row = row;
    font_mgr.font[index].col = col;
    font_mgr.font[index].font_size = font_size;

    font_mgr.font[index].array = (unsigned char*)malloc(row*col);
    CHECK(font_mgr.font[index].array, -1, "failed to malloc %d bytes\n", row*col);

    memcpy(font_mgr.font[index].array, src, row*col);
    font_mgr.valid_num++;

    return 0;
}

int font_init()
{
    memset(&font_mgr, 0, sizeof(font_mgr));

    return 0;
}

int font_exit()
{
    unsigned int i = 0;
    for (i = 0; i < MAX_FONT_NUM; i++)
    {
        if (font_mgr.font[i].array)
        {
            free(font_mgr.font[i].array);
            memset(&font_mgr.font[i], 0, sizeof(font_s));
        }
    }

    return 0;
}

font_s* font_get(const unsigned char* enc, int len, int font_size)
{
    unsigned int i = 0;
    for (i = 0; i < MAX_FONT_NUM; i++)
    {
        if (font_mgr.font[i].enc_len == len && font_mgr.font[i].font_size == font_size)
        {
            if (!memcmp(font_mgr.font[i].enc, enc, len))
            {
                font_mgr.font[i].reference++;
                //DBG("reference: %d, chs: %s ok\n", font_mgr.font[i].reference, font_mgr.font[i].enc);
                return &font_mgr.font[i];
            }
        }
    }

    return NULL;
}

int font_release(font_s* font)
{
    unsigned int i = 0;
    for (i = 0; i < MAX_FONT_NUM; i++)
    {
        if (font_mgr.font[i].enc_len == font->enc_len && font_mgr.font[i].font_size == font->font_size)
        {
            if (!memcmp(font_mgr.font[i].enc, font->enc, font->enc_len))
            {
                font_mgr.font[i].reference--;
                //DBG("reference: %d, chs: %s ok\n", font_mgr.font[i].reference, font_mgr.font[i].enc);
                if (font_mgr.font[i].reference < 0)
                {
                    font_mgr.font[i].reference = 0;
                }

                return 0;
            }
        }
    }

    return -1;
}

int font_get_num()
{
    return font_mgr.valid_num;
}



