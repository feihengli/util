
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <stdint.h> /* for uint32_t, etc */  
#include "sps_pps_parser.h"
#include "sal_debug.h"

#define SPS_PPS_DEBUG 1
  
#define MAX_LEN 32  
  
/** 
*  @brief Function get_1bit()   读1个bit 
*  @param[in]     h     get_bit_context structrue 
*  @retval        0: success, -1 : failure 
*  @pre 
*  @post 
*/  
static int get_1bit(void *h)  
{  
    get_bit_context *ptr = (get_bit_context *)h;  
    int ret = 0;  
    uint8_t *cur_char = NULL;  
    uint8_t shift;  
  
    if (NULL == ptr)  
    {  
        ERR("NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    cur_char = ptr->buf + (ptr->bit_pos >> 3);  
    shift = 7 - (ptr->cur_bit_pos);  
    ptr->bit_pos++;  
    ptr->cur_bit_pos = ptr->bit_pos & 0x7;  
    ret = ((*cur_char) >> shift) & 0x01;  
  
exit:  
    return ret;  
}  
  
/** 
*  @brief Function get_bits()  读n个bits，n不能超过32 
*  @param[in]     h     get_bit_context structrue 
*  @param[in]     n     how many bits you want? 
*  @retval        0: success, -1 : failure 
*  @pre 
*  @post 
*/  
static int get_bits(void *h, int n)  
{  
    get_bit_context *ptr = (get_bit_context *)h;  
    uint8_t temp[5] = { 0 };  
    uint8_t *cur_char = NULL;  
    uint8_t nbyte;  
    uint8_t shift;  
    uint32_t result;  
    uint64_t ret = 0;  
  
    if (NULL == ptr)  
    {  
        ERR("NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    if (n > MAX_LEN)  
    {  
        n = MAX_LEN;  
    }  
    if ((ptr->bit_pos + n) > ptr->total_bit)  
    {  
        n = ptr->total_bit - ptr->bit_pos;  
    }  
  
    cur_char = ptr->buf + (ptr->bit_pos >> 3);  
    nbyte = (ptr->cur_bit_pos + n + 7) >> 3;  
    shift = (8 - (ptr->cur_bit_pos + n)) & 0x07;  
  
    if (n == MAX_LEN)  
    {  
        DBG("12(ptr->bit_pos(:%d) + n(:%d)) > ptr->total_bit(:%d)!!! \n", ptr->bit_pos, n, ptr->total_bit);  
        DBG("0x%x 0x%x 0x%x 0x%x\n", (*cur_char), *(cur_char + 1), *(cur_char + 2), *(cur_char + 3));  
    }  
  
    memcpy(&temp[5 - nbyte], cur_char, nbyte);  
    ret = (uint32_t)temp[0] << 24;  
    ret = ret << 8;  
    ret = ((uint32_t)temp[1] << 24) | ((uint32_t)temp[2] << 16) | ((uint32_t)temp[3] << 8) | temp[4];  
  
    ret = (ret >> shift) & (((uint64_t)1 << n) - 1);  
  
    result = ret;  
    ptr->bit_pos += n;  
    ptr->cur_bit_pos = ptr->bit_pos & 0x7;  
  
exit:  
    return result;  
}  
  
  
/** 
*  @brief Function parse_codenum() 指数哥伦布编码解析，参考h264标准第9节 
*  @param[in]     buf 
*  @retval        code_num 
*  @pre 
*  @post 
*/  
static int parse_codenum(void *buf)  
{  
    uint8_t leading_zero_bits = -1;  
    uint8_t b;  
    uint32_t code_num = 0;  
  
    for (b = 0; !b; leading_zero_bits++)  
    {  
        b = get_1bit(buf);  
    }  
  
    code_num = ((uint32_t)1 << leading_zero_bits) - 1 + get_bits(buf, leading_zero_bits);  
  
    return code_num;  
}  
  
/** 
*  @brief Function parse_ue() 指数哥伦布编码解析 ue(),参考h264标准第9节 
*  @param[in]     buf       sps_pps parse buf 
*  @retval        code_num 
*  @pre 
*  @post 
*/  
static int parse_ue(void *buf)  
{  
    return parse_codenum(buf);  
}  
  
/** 
*  @brief Function parse_se() 指数哥伦布编码解析 se(), 参考h264标准第9节 
*  @param[in]     buf       sps_pps parse buf 
*  @retval        code_num 
*  @pre 
*  @post 
*/  
static int parse_se(void *buf)  
{  
    int ret = 0;  
    int code_num;  
  
    code_num = parse_codenum(buf);  
    ret = (code_num + 1) >> 1;  
    ret = (code_num & 0x01) ? ret : -ret;  
  
    return ret;  
}  
  
/** 
*  @brief Function get_bit_context_free()  申请的get_bit_context结构内存释放 
*  @param[in]     buf       get_bit_context buf 
*  @retval        none 
*  @pre 
*  @post 
*/  
static void get_bit_context_free(void *buf)  
{  
    get_bit_context *ptr = (get_bit_context *)buf;  
  
    if (ptr)  
    {  
        if (ptr->buf)  
        {  
            free(ptr->buf);  
        }  
  
        free(ptr);  
    }  
}  
  
static void *de_emulation_prevention(void *buf)  
{  
    get_bit_context *ptr = NULL;  
    get_bit_context *buf_ptr = (get_bit_context *)buf;  
    int i = 0, j = 0;  
    uint8_t *tmp_ptr = NULL;  
    int tmp_buf_size = 0;  
    int val = 0;  
    if (NULL == buf_ptr)  
    {  
        ERR("NULL ptr\n");  
        goto exit;  
    }  
    ptr = (get_bit_context *)malloc(sizeof(get_bit_context));  
    if (NULL == ptr)  
    {  
        ERR("NULL ptr\n");  
        goto exit;  
    }  
    memcpy(ptr, buf_ptr, sizeof(get_bit_context));  
    DBG("ptr->buf_size=%d\n", ptr->buf_size);  
    ptr->buf = (uint8_t *)malloc(ptr->buf_size);  
    if (NULL == ptr->buf)  
    {  
        ERR("NULL ptr ");  
        goto exit;  
    }  
    memcpy(ptr->buf, buf_ptr->buf, buf_ptr->buf_size);  
    tmp_ptr = ptr->buf;  
    tmp_buf_size = ptr->buf_size;  
    for (i = 0; i<(tmp_buf_size - 2); i++)  
    {  
        /*检测0x000003*/  
        val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);  
        if (val == 0)  
        {  
            /*剔除0x03*/  
            for (j = i + 2; j<tmp_buf_size - 1; j++)  
            {  
                tmp_ptr[j] = tmp_ptr[j + 1];  
            }  
  
            /*相应的bufsize要减小*/  
            ptr->buf_size--;  
        }  
    }  
  
    /*重新计算total_bit*/  
    ptr->total_bit = ptr->buf_size << 3;  
    return (void *)ptr;  
  
exit:  
    get_bit_context_free(ptr);  
    return NULL;  
}  
  
/** 
*  @brief Function get_bit_context_free()  VUI_parameters 解析，原理参考h264标准 
*  @param[in]     buf       get_bit_context buf 
*  @param[in]     vui_ptr   vui解析结果 
*  @retval        0: success, -1 : failure 
*  @pre 
*  @post 
*/  
static int vui_parameters_set(void *buf, vui_parameters_t *vui_ptr)  
{  
    int ret = 0;  
    int SchedSelIdx = 0;  
  
    if (NULL == vui_ptr || NULL == buf)  
    {  
        ERR("ERR NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    vui_ptr->aspect_ratio_info_present_flag = get_1bit(buf);  
    if (vui_ptr->aspect_ratio_info_present_flag)  
    {  
        vui_ptr->aspect_ratio_idc = get_bits(buf, 8);  
        if (vui_ptr->aspect_ratio_idc == Extended_SAR)  
        {  
            vui_ptr->sar_width = get_bits(buf, 16);  
            vui_ptr->sar_height = get_bits(buf, 16);  
        }  
    }  
  
    vui_ptr->overscan_info_present_flag = get_1bit(buf);  
    if (vui_ptr->overscan_info_present_flag)  
    {  
        vui_ptr->overscan_appropriate_flag = get_1bit(buf);  
    }  
  
    vui_ptr->video_signal_type_present_flag = get_1bit(buf);  
    if (vui_ptr->video_signal_type_present_flag)  
    {  
        vui_ptr->video_format = get_bits(buf, 3);  
        vui_ptr->video_full_range_flag = get_1bit(buf);  
  
        vui_ptr->colour_description_present_flag = get_1bit(buf);  
        if (vui_ptr->colour_description_present_flag)  
        {  
            vui_ptr->colour_primaries = get_bits(buf, 8);  
            vui_ptr->transfer_characteristics = get_bits(buf, 8);  
            vui_ptr->matrix_coefficients = get_bits(buf, 8);  
        }  
    }  
  
    vui_ptr->chroma_loc_info_present_flag = get_1bit(buf);  
    if (vui_ptr->chroma_loc_info_present_flag)  
    {  
        vui_ptr->chroma_sample_loc_type_top_field = parse_ue(buf);  
        vui_ptr->chroma_sample_loc_type_bottom_field = parse_ue(buf);  
    }  
  
    vui_ptr->timing_info_present_flag = get_1bit(buf);  
    if (vui_ptr->timing_info_present_flag)  
    {  
        vui_ptr->num_units_in_tick = get_bits(buf, 32);  
        vui_ptr->time_scale = get_bits(buf, 32);  
        vui_ptr->fixed_frame_rate_flag = get_1bit(buf);  
    }  
  
    vui_ptr->nal_hrd_parameters_present_flag = get_1bit(buf);  
    if (vui_ptr->nal_hrd_parameters_present_flag)  
    {  
        vui_ptr->cpb_cnt_minus1 = parse_ue(buf);  
        vui_ptr->bit_rate_scale = get_bits(buf, 4);  
        vui_ptr->cpb_size_scale = get_bits(buf, 4);  
  
        for (SchedSelIdx = 0; SchedSelIdx <= vui_ptr->cpb_cnt_minus1; SchedSelIdx++)  
        {  
            vui_ptr->bit_rate_value_minus1[SchedSelIdx] = parse_ue(buf);  
            vui_ptr->cpb_size_value_minus1[SchedSelIdx] = parse_ue(buf);  
            vui_ptr->cbr_flag[SchedSelIdx] = get_1bit(buf);  
        }  
  
        vui_ptr->initial_cpb_removal_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->cpb_removal_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->dpb_output_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->time_offset_length = get_bits(buf, 5);  
    }  
  
  
    vui_ptr->vcl_hrd_parameters_present_flag = get_1bit(buf);  
    if (vui_ptr->vcl_hrd_parameters_present_flag)  
    {  
        vui_ptr->cpb_cnt_minus1 = parse_ue(buf);  
        vui_ptr->bit_rate_scale = get_bits(buf, 4);  
        vui_ptr->cpb_size_scale = get_bits(buf, 4);  
  
        for (SchedSelIdx = 0; SchedSelIdx <= vui_ptr->cpb_cnt_minus1; SchedSelIdx++)  
        {  
            vui_ptr->bit_rate_value_minus1[SchedSelIdx] = parse_ue(buf);  
            vui_ptr->cpb_size_value_minus1[SchedSelIdx] = parse_ue(buf);  
            vui_ptr->cbr_flag[SchedSelIdx] = get_1bit(buf);  
        }  
        vui_ptr->initial_cpb_removal_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->cpb_removal_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->dpb_output_delay_length_minus1 = get_bits(buf, 5);  
        vui_ptr->time_offset_length = get_bits(buf, 5);  
    }  
  
    if (vui_ptr->nal_hrd_parameters_present_flag || vui_ptr->vcl_hrd_parameters_present_flag)  
    {  
        vui_ptr->low_delay_hrd_flag = get_1bit(buf);  
    }  
  
    vui_ptr->pic_struct_present_flag = get_1bit(buf);  
  
    vui_ptr->bitstream_restriction_flag = get_1bit(buf);  
    if (vui_ptr->bitstream_restriction_flag)  
    {  
        vui_ptr->motion_vectors_over_pic_boundaries_flag = get_1bit(buf);  
        vui_ptr->max_bytes_per_pic_denom = parse_ue(buf);  
        vui_ptr->max_bits_per_mb_denom = parse_ue(buf);  
        vui_ptr->log2_max_mv_length_horizontal = parse_ue(buf);  
        vui_ptr->log2_max_mv_length_vertical = parse_ue(buf);  
        vui_ptr->num_reorder_frames = parse_ue(buf);  
        vui_ptr->max_dec_frame_buffering = parse_ue(buf);  
    }  
  
exit:  
    return ret;  
}  
  
/*SPS 信息打印，调试使用*/  
static void sps_info_print(SPS* sps_ptr)  
{  
    if (NULL != sps_ptr)  
    {  
        DBG("profile_idc: %d\n", sps_ptr->profile_idc);  
        DBG("constraint_set0_flag: %d\n", sps_ptr->constraint_set0_flag);  
        DBG("constraint_set1_flag: %d\n", sps_ptr->constraint_set1_flag);  
        DBG("constraint_set2_flag: %d\n", sps_ptr->constraint_set2_flag);  
        DBG("constraint_set3_flag: %d\n", sps_ptr->constraint_set3_flag);  
        DBG("reserved_zero_4bits: %d\n", sps_ptr->reserved_zero_4bits);  
        DBG("level_idc: %d\n", sps_ptr->level_idc);  
        DBG("seq_parameter_set_id: %d\n", sps_ptr->seq_parameter_set_id);  
        DBG("chroma_format_idc: %d\n", sps_ptr->chroma_format_idc);  
        DBG("separate_colour_plane_flag: %d\n", sps_ptr->separate_colour_plane_flag);  
        DBG("bit_depth_luma_minus8: %d\n", sps_ptr->bit_depth_luma_minus8);  
        DBG("bit_depth_chroma_minus8: %d\n", sps_ptr->bit_depth_chroma_minus8);  
        DBG("qpprime_y_zero_transform_bypass_flag: %d\n", sps_ptr->qpprime_y_zero_transform_bypass_flag);  
        DBG("seq_scaling_matrix_present_flag: %d\n", sps_ptr->seq_scaling_matrix_present_flag);  
        //DBG("seq_scaling_list_present_flag:%d\n", sps_ptr->seq_scaling_list_present_flag);   
        DBG("log2_max_frame_num_minus4: %d\n", sps_ptr->log2_max_frame_num_minus4);  
        DBG("pic_order_cnt_type: %d\n", sps_ptr->pic_order_cnt_type);  
        DBG("num_ref_frames: %d\n", sps_ptr->num_ref_frames);  
        DBG("gaps_in_frame_num_value_allowed_flag: %d\n", sps_ptr->gaps_in_frame_num_value_allowed_flag);  
        DBG("pic_width_in_mbs_minus1: %d\n", sps_ptr->pic_width_in_mbs_minus1);  
        DBG("pic_height_in_map_units_minus1: %d\n", sps_ptr->pic_height_in_map_units_minus1);  
        DBG("frame_mbs_only_flag: %d\n", sps_ptr->frame_mbs_only_flag);  
        DBG("mb_adaptive_frame_field_flag: %d\n", sps_ptr->mb_adaptive_frame_field_flag);  
        DBG("direct_8x8_inference_flag: %d\n", sps_ptr->direct_8x8_inference_flag);  
        DBG("frame_cropping_flag: %d\n", sps_ptr->frame_cropping_flag);  
        DBG("frame_crop_left_offset: %d\n", sps_ptr->frame_crop_left_offset);  
        DBG("frame_crop_right_offset: %d\n", sps_ptr->frame_crop_right_offset);  
        DBG("frame_crop_top_offset: %d\n", sps_ptr->frame_crop_top_offset);  
        DBG("frame_crop_bottom_offset: %d\n", sps_ptr->frame_crop_bottom_offset);  
        DBG("vui_parameters_present_flag: %d\n", sps_ptr->vui_parameters_present_flag);  
  
        if (sps_ptr->vui_parameters_present_flag)  
        {  
            DBG("aspect_ratio_info_present_flag: %d\n", sps_ptr->vui_parameters.aspect_ratio_info_present_flag);  
            DBG("aspect_ratio_idc: %d\n", sps_ptr->vui_parameters.aspect_ratio_idc);  
            DBG("sar_width: %d\n", sps_ptr->vui_parameters.sar_width);  
            DBG("sar_height: %d\n", sps_ptr->vui_parameters.sar_height);  
            DBG("overscan_info_present_flag: %d\n", sps_ptr->vui_parameters.overscan_info_present_flag);  
            DBG("overscan_info_appropriate_flag: %d\n", sps_ptr->vui_parameters.overscan_appropriate_flag);  
            DBG("video_signal_type_present_flag: %d\n", sps_ptr->vui_parameters.video_signal_type_present_flag);  
            DBG("video_format: %d\n", sps_ptr->vui_parameters.video_format);  
            DBG("video_full_range_flag: %d\n", sps_ptr->vui_parameters.video_full_range_flag);  
            DBG("colour_description_present_flag: %d\n", sps_ptr->vui_parameters.colour_description_present_flag);  
            DBG("colour_primaries: %d\n", sps_ptr->vui_parameters.colour_primaries);  
            DBG("transfer_characteristics: %d\n", sps_ptr->vui_parameters.transfer_characteristics);  
            DBG("matrix_coefficients: %d\n", sps_ptr->vui_parameters.matrix_coefficients);  
            DBG("chroma_loc_info_present_flag: %d\n", sps_ptr->vui_parameters.chroma_loc_info_present_flag);  
            DBG("chroma_sample_loc_type_top_field: %d\n", sps_ptr->vui_parameters.chroma_sample_loc_type_top_field);  
            DBG("chroma_sample_loc_type_bottom_field: %d\n", sps_ptr->vui_parameters.chroma_sample_loc_type_bottom_field);  
            DBG("timing_info_present_flag: %d\n", sps_ptr->vui_parameters.timing_info_present_flag);  
            DBG("num_units_in_tick: %d\n", sps_ptr->vui_parameters.num_units_in_tick);  
            DBG("time_scale: %d\n", sps_ptr->vui_parameters.time_scale);  
            DBG("fixed_frame_rate_flag: %d\n", sps_ptr->vui_parameters.fixed_frame_rate_flag);  
            DBG("nal_hrd_parameters_present_flag: %d\n", sps_ptr->vui_parameters.nal_hrd_parameters_present_flag);  
            DBG("cpb_cnt_minus1: %d\n", sps_ptr->vui_parameters.cpb_cnt_minus1);  
            DBG("bit_rate_scale: %d\n", sps_ptr->vui_parameters.bit_rate_scale);  
            DBG("cpb_size_scale: %d\n", sps_ptr->vui_parameters.cpb_size_scale);  
            DBG("initial_cpb_removal_delay_length_minus1: %d\n", sps_ptr->vui_parameters.initial_cpb_removal_delay_length_minus1);  
            DBG("cpb_removal_delay_length_minus1: %d\n", sps_ptr->vui_parameters.cpb_removal_delay_length_minus1);  
            DBG("dpb_output_delay_length_minus1: %d\n", sps_ptr->vui_parameters.dpb_output_delay_length_minus1);  
            DBG("time_offset_length: %d\n", sps_ptr->vui_parameters.time_offset_length);  
            DBG("vcl_hrd_parameters_present_flag: %d\n", sps_ptr->vui_parameters.vcl_hrd_parameters_present_flag);  
            DBG("low_delay_hrd_flag: %d\n", sps_ptr->vui_parameters.low_delay_hrd_flag);  
            DBG("pic_struct_present_flag: %d\n", sps_ptr->vui_parameters.pic_struct_present_flag);  
            DBG("bitstream_restriction_flag: %d\n", sps_ptr->vui_parameters.bitstream_restriction_flag);  
            DBG("motion_vectors_over_pic_boundaries_flag: %d\n", sps_ptr->vui_parameters.motion_vectors_over_pic_boundaries_flag);  
            DBG("max_bytes_per_pic_denom: %d\n", sps_ptr->vui_parameters.max_bytes_per_pic_denom);  
            DBG("max_bits_per_mb_denom: %d\n", sps_ptr->vui_parameters.max_bits_per_mb_denom);  
            DBG("log2_max_mv_length_horizontal: %d\n", sps_ptr->vui_parameters.log2_max_mv_length_horizontal);  
            DBG("log2_max_mv_length_vertical: %d\n", sps_ptr->vui_parameters.log2_max_mv_length_vertical);  
            DBG("num_reorder_frames: %d\n", sps_ptr->vui_parameters.num_reorder_frames);  
            DBG("max_dec_frame_buffering: %d\n", sps_ptr->vui_parameters.max_dec_frame_buffering);  
        }  
  
    }  
}  
  
/** 
*  @brief Function h264dec_seq_parameter_set()  h264 SPS infomation 解析 
*  @param[in]     buf       buf ptr, 需同步00 00 00 01 X7后传入 
*  @param[in]     sps_ptr   sps指针，保存SPS信息 
*  @retval        0: success, -1 : failure 
*  @pre 
*  @post 
*/  
int h264dec_seq_parameter_set(get_bit_context *buf_ptr, SPS *sps_ptr)  
{  
    SPS *sps = sps_ptr;  
    int ret = 0;  
    int profile_idc = 0;  
    int i, j, last_scale, next_scale, delta_scale;  
    void *buf = NULL;  
  
    if (NULL == buf_ptr || NULL == sps)  
    {  
        ERR("ERR NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    memset((void *)sps, 0, sizeof(SPS));  
    buf = de_emulation_prevention(buf_ptr);  
    if (NULL == buf)  
    {  
        ERR("ERR NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
    sps->profile_idc = get_bits(buf, 8);  
    sps->constraint_set0_flag = get_1bit(buf);  
    sps->constraint_set1_flag = get_1bit(buf);  
    sps->constraint_set2_flag = get_1bit(buf);  
    sps->constraint_set3_flag = get_1bit(buf);  
    sps->reserved_zero_4bits = get_bits(buf, 4);  
    sps->level_idc = get_bits(buf, 8);  
    sps->seq_parameter_set_id = parse_ue(buf);  
    profile_idc = sps->profile_idc;  
    if ((profile_idc == 100) || (profile_idc == 110) || (profile_idc == 122) || (profile_idc == 244)  
        || (profile_idc == 44) || (profile_idc == 83) || (profile_idc == 86) || (profile_idc == 118) || (profile_idc == 128))  
    {  
        sps->chroma_format_idc = parse_ue(buf);  
        if (sps->chroma_format_idc == 3)  
        {  
            sps->separate_colour_plane_flag = get_1bit(buf);  
        }  
  
        sps->bit_depth_luma_minus8 = parse_ue(buf);  
        sps->bit_depth_chroma_minus8 = parse_ue(buf);  
        sps->qpprime_y_zero_transform_bypass_flag = get_1bit(buf);  
        sps->seq_scaling_matrix_present_flag = get_1bit(buf);  
        if (sps->seq_scaling_matrix_present_flag)  
        {  
            for (i = 0; i < ((sps->chroma_format_idc != 3) ? 8 : 12); i++)  
            {  
                sps->seq_scaling_list_present_flag[i] = get_1bit(buf);  
                if (sps->seq_scaling_list_present_flag[i])  
                {  
                    if (i < 6)  
                    {  
                        for (j = 0; j < 16; j++)  
                        {  
                            last_scale = 8;  
                            next_scale = 8;  
                            if (next_scale != 0)  
                            {  
                                delta_scale = parse_se(buf);  
                                next_scale = (last_scale + delta_scale + 256) % 256;  
                                sps->UseDefaultScalingMatrix4x4Flag[i] = ((j == 0) && (next_scale == 0));  
                            }  
                            sps->ScalingList4x4[i][j] = (next_scale == 0) ? last_scale : next_scale;  
                            last_scale = sps->ScalingList4x4[i][j];  
                        }  
                    }  
                    else  
                    {  
                        int ii = i - 6;  
                        next_scale = 8;  
                        last_scale = 8;  
                        for (j = 0; j < 64; j++)  
                        {  
                            if (next_scale != 0)  
                            {  
                                delta_scale = parse_se(buf);  
                                next_scale = (last_scale + delta_scale + 256) % 256;  
                                sps->UseDefaultScalingMatrix8x8Flag[ii] = ((j == 0) && (next_scale == 0));  
                            }  
                            sps->ScalingList8x8[ii][j] = (next_scale == 0) ? last_scale : next_scale;  
                            last_scale = sps->ScalingList8x8[ii][j];  
                        }  
                    }  
                }  
            }  
        }  
    }  
    sps->log2_max_frame_num_minus4 = parse_ue(buf);  
    sps->pic_order_cnt_type = parse_ue(buf);  
    if (sps->pic_order_cnt_type == 0)  
    {  
        sps->log2_max_pic_order_cnt_lsb_minus4 = parse_ue(buf);  
    }  
    else if (sps->pic_order_cnt_type == 1)  
    {  
        sps->delta_pic_order_always_zero_flag = get_1bit(buf);  
        sps->offset_for_non_ref_pic = parse_se(buf);  
        sps->offset_for_top_to_bottom_field = parse_se(buf);  
  
        sps->num_ref_frames_in_pic_order_cnt_cycle = parse_ue(buf);  
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)  
        {  
            sps->offset_for_ref_frame_array[i] = parse_se(buf);  
        }  
    }  
    sps->num_ref_frames = parse_ue(buf);  
    sps->gaps_in_frame_num_value_allowed_flag = get_1bit(buf);  
    sps->pic_width_in_mbs_minus1 = parse_ue(buf);  
    sps->pic_height_in_map_units_minus1 = parse_ue(buf);  
    sps->frame_mbs_only_flag = get_1bit(buf);  
    if (!sps->frame_mbs_only_flag)  
    {  
        sps->mb_adaptive_frame_field_flag = get_1bit(buf);  
    }  
    sps->direct_8x8_inference_flag = get_1bit(buf);  
  
    sps->frame_cropping_flag = get_1bit(buf);  
    if (sps->frame_cropping_flag)  
    {  
        sps->frame_crop_left_offset = parse_ue(buf);  
        sps->frame_crop_right_offset = parse_ue(buf);  
        sps->frame_crop_top_offset = parse_ue(buf);  
        sps->frame_crop_bottom_offset = parse_ue(buf);  
    }  
    sps->vui_parameters_present_flag = get_1bit(buf);  
    if (sps->vui_parameters_present_flag)  
    {  
        vui_parameters_set(buf, &sps->vui_parameters);  
    }  
  
#if SPS_PPS_DEBUG  
    sps_info_print(sps);  
#endif  
exit:  
    get_bit_context_free(buf);  
    return ret;  
}  
  
/** 
*  @brief Function more_rbsp_data()  计算pps串最后一个为1的比特位及其后都是比特0的个数 
*  @param[in]     buf       get_bit_context structure 
*  @retval 
*  @pre 
*  @post 
*  @note  这段代码来自网友的帮助，并没有验证，使用时需注意 
*/  
static int more_rbsp_data(void *buf)  
{  
    get_bit_context *ptr = (get_bit_context *)buf;  
    get_bit_context tmp;  
  
    if (NULL == buf)  
    {  
        ERR("NULL pointer, err\n");  
        return -1;  
    }  
  
    memset(&tmp, 0, sizeof(get_bit_context));  
    memcpy(&tmp, ptr, sizeof(get_bit_context));  
  
    for (tmp.bit_pos = ptr->total_bit - 1; tmp.bit_pos > ptr->bit_pos; tmp.bit_pos -= 2)  
    {  
        if (get_1bit(&tmp))  
        {  
            break;  
        }  
    }  
    return tmp.bit_pos == ptr->bit_pos ? 0 : 1;  
}  
  
/** 
*  @brief Function h264dec_picture_parameter_set()  h264 PPS infomation 解析 
*  @param[in]     buf       buf ptr, 需同步00 00 00 01 X8后传入 
*  @param[in]     pps_ptr   pps指针，保存pps信息 
*  @retval        0: success, -1 : failure 
*  @pre 
*  @post 
*  @note: 用法参考sps解析 
*/  
int h264dec_picture_parameter_set(void *buf_ptr, PPS *pps_ptr)  
{  
    PPS *pps = pps_ptr;  
    int ret = 0;  
    void *buf = NULL;  
    int iGroup = 0;  
    int i, j, last_scale, next_scale, delta_scale;  
  
    if (NULL == buf_ptr || NULL == pps_ptr)  
    {  
        ERR("NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    memset((void *)pps, 0, sizeof(PPS));  
  
    buf = de_emulation_prevention(buf_ptr);  
    if (NULL == buf)  
    {  
        ERR("ERR NULL pointer\n");  
        ret = -1;  
        goto exit;  
    }  
  
    pps->pic_parameter_set_id = parse_ue(buf);  
    pps->seq_parameter_set_id = parse_ue(buf);  
    pps->entropy_coding_mode_flag = get_1bit(buf);  
    pps->pic_order_present_flag = get_1bit(buf);  
  
    pps->num_slice_groups_minus1 = parse_ue(buf);  
    if (pps->num_slice_groups_minus1 > 0)  
    {  
        pps->slice_group_map_type = parse_ue(buf);  
        if (pps->slice_group_map_type == 0)  
        {  
            for (iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++)  
            {  
                pps->run_length_minus1[iGroup] = parse_ue(buf);  
            }  
        }  
        else if (pps->slice_group_map_type == 2)  
        {  
            for (iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++)  
            {  
                pps->top_left[iGroup] = parse_ue(buf);  
                pps->bottom_right[iGroup] = parse_ue(buf);  
            }  
        }  
        else if (pps->slice_group_map_type == 3 
            || pps->slice_group_map_type == 4
            || pps->slice_group_map_type == 5)  
        {  
            pps->slice_group_change_direction_flag = get_1bit(buf);  
            pps->slice_group_change_rate_minus1 = parse_ue(buf);  
        }  
        else if (pps->slice_group_map_type == 6)  
        {  
            pps->pic_size_in_map_units_minus1 = parse_ue(buf);  
            for (i = 0; i<pps->pic_size_in_map_units_minus1; i++)  
            {  
                /*这地方可能有问题，对u(v)理解偏差*/  
                pps->slice_group_id[i] = get_bits(buf, pps->pic_size_in_map_units_minus1);  
            }  
        }  
    }  
  
    pps->num_ref_idx_10_active_minus1 = parse_ue(buf);  
    pps->num_ref_idx_11_active_minus1 = parse_ue(buf);  
    pps->weighted_pred_flag = get_1bit(buf);  
    pps->weighted_bipred_idc = get_bits(buf, 2);  
    pps->pic_init_qp_minus26 = parse_se(buf); /*relative26*/  
    pps->pic_init_qs_minus26 = parse_se(buf); /*relative26*/  
    pps->chroma_qp_index_offset = parse_se(buf);  
    pps->deblocking_filter_control_present_flag = get_1bit(buf);  
    pps->constrained_intra_pred_flag = get_1bit(buf);  
    pps->redundant_pic_cnt_present_flag = get_1bit(buf);  
  
    if (more_rbsp_data(buf))  
    {  
        pps->transform_8x8_mode_flag = get_1bit(buf);  
        pps->pic_scaling_matrix_present_flag = get_1bit(buf);  
        if (pps->pic_scaling_matrix_present_flag)  
        {  
            for (i = 0; i<6 + 2 * pps->transform_8x8_mode_flag; i++)  
            {  
                pps->pic_scaling_list_present_flag[i] = get_1bit(buf);  
                if (pps->pic_scaling_list_present_flag[i])  
                {  
                    if (i<6)  
                    {  
                        for (j = 0; j<16; j++)  
                        {  
                            next_scale = 8;  
                            last_scale = 8;  
                            if (next_scale != 0)  
                            {  
                                delta_scale = parse_se(buf);  
                                next_scale = (last_scale + delta_scale + 256) % 256;  
                                pps->UseDefaultScalingMatrix4x4Flag[i] = ((j == 0) && (next_scale == 0));  
                            }  
                            pps->ScalingList4x4[i][j] = (next_scale == 0) ? last_scale : next_scale;  
                            last_scale = pps->ScalingList4x4[i][j];  
                        }  
                    }  
                    else  
                    {  
                        int ii = i - 6;  
                        next_scale = 8;  
                        last_scale = 8;  
                        for (j = 0; j<64; j++)  
                        {  
                            if (next_scale != 0)  
                            {  
                                delta_scale = parse_se(buf);  
                                next_scale = (last_scale + delta_scale + 256) % 256;  
                                pps->UseDefaultScalingMatrix8x8Flag[ii] = ((j == 0) && (next_scale == 0));  
                            }  
                            pps->ScalingList8x8[ii][j] = (next_scale == 0) ? last_scale : next_scale;  
                            last_scale = pps->ScalingList8x8[ii][j];  
                        }  
                    }  
                }  
            }  
  
            pps->second_chroma_qp_index_offset = parse_se(buf);  
        }  
    }  
  
exit:  
    get_bit_context_free(buf);  
    return ret;  
}  
  
// calculation width height and framerate  
int h264_get_width(SPS *sps_ptr)  
{  
    return (sps_ptr->pic_width_in_mbs_minus1 + 1) * 16;  
}  
  
int h264_get_height(SPS *sps_ptr)  
{  
    DBG("sps_ptr->frame_mbs_only_flag=%d\n", sps_ptr->frame_mbs_only_flag);  
    return (sps_ptr->pic_height_in_map_units_minus1 + 1) * 16 * (2 - sps_ptr->frame_mbs_only_flag);  
}  
  
int h264_get_format(SPS *sps_ptr)  
{  
    return sps_ptr->frame_mbs_only_flag;  
}  
  
  
int h264_get_framerate(float *framerate, SPS *sps_ptr)  
{  
    if (sps_ptr->vui_parameters.timing_info_present_flag)  
    {  
        if (sps_ptr->frame_mbs_only_flag)  
        {  
            //*framerate = (float)sps_ptr->vui_parameters.time_scale / (float)sps_ptr->vui_parameters.num_units_in_tick;  
            *framerate = (float)sps_ptr->vui_parameters.time_scale / (float)sps_ptr->vui_parameters.num_units_in_tick / 2.0;  
            //fr_int = sps_ptr->vui_parameters.time_scale / sps_ptr->vui_parameters.num_units_in_tick;  
        }  
        else  
        {  
            *framerate = (float)sps_ptr->vui_parameters.time_scale / (float)sps_ptr->vui_parameters.num_units_in_tick / 2.0;  
            //fr_int = sps_ptr->vui_parameters.time_scale / sps_ptr->vui_parameters.num_units_in_tick / 2;  
        }  
        return 0;  
    }  
    else  
    {  
        return 1;  
    }  
}  
  
void memcpy_sps_data(uint8_t *dst, uint8_t *src, int len)  
{  
    int tmp;  
    for (tmp = 0; tmp < len; tmp++)  
    {  
        //printf("0x%02x ", src[tmp]);  
        dst[(tmp / 4) * 4 + (3 - (tmp % 4))] = src[tmp];  
    }  
}  
