










#include "./vpx_scale_rtcd.h"
#include "vp8/common/onyxc_int.h"
#include "onyx_int.h"
#include "quantize.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/vpx_scale.h"
#include "vp8/common/alloccommon.h"
#include "vp8/common/loopfilter.h"
#if ARCH_ARM
#include "vpx_ports/arm.h"
#endif

extern int vp8_calc_ss_err(YV12_BUFFER_CONFIG *source, YV12_BUFFER_CONFIG *dest);

void vp8_yv12_copy_partial_frame_c(YV12_BUFFER_CONFIG *src_ybc,
                                   YV12_BUFFER_CONFIG *dst_ybc)
{
    unsigned char *src_y, *dst_y;
    int yheight;
    int ystride;
    int yoffset;
    int linestocopy;

    yheight  = src_ybc->y_height;
    ystride  = src_ybc->y_stride;

    
    linestocopy = (yheight >> 4) / PARTIAL_FRAME_FRACTION;
    linestocopy = linestocopy ? linestocopy << 4 : 16;     

    



    linestocopy += 4;
    
    yoffset  = ystride * (((yheight >> 5) * 16) - 4);
    src_y = src_ybc->y_buffer + yoffset;
    dst_y = dst_ybc->y_buffer + yoffset;

    vpx_memcpy(dst_y, src_y, ystride * linestocopy);
}

static int calc_partial_ssl_err(YV12_BUFFER_CONFIG *source,
                                YV12_BUFFER_CONFIG *dest)
{
    int i, j;
    int Total = 0;
    int srcoffset, dstoffset;
    unsigned char *src = source->y_buffer;
    unsigned char *dst = dest->y_buffer;

    int linestocopy;

    
    linestocopy = (source->y_height >> 4) / PARTIAL_FRAME_FRACTION;
    linestocopy = linestocopy ? linestocopy << 4 : 16;     


    
    srcoffset = source->y_stride * ((dest->y_height >> 5) * 16);
    dstoffset = dest->y_stride   * ((dest->y_height >> 5) * 16);

    src += srcoffset;
    dst += dstoffset;

    


    for (i = 0; i < linestocopy; i += 16)
    {
        for (j = 0; j < source->y_width; j += 16)
        {
            unsigned int sse;
            Total += vp8_mse16x16(src + j, source->y_stride,
                                                     dst + j, dest->y_stride,
                                                     &sse);
        }

        src += 16 * source->y_stride;
        dst += 16 * dest->y_stride;
    }

    return Total;
}


static int get_min_filter_level(VP8_COMP *cpi, int base_qindex)
{
    int min_filter_level;

    if (cpi->source_alt_ref_active && cpi->common.refresh_golden_frame &&
        !cpi->common.refresh_alt_ref_frame)
        min_filter_level = 0;
    else
    {
        if (base_qindex <= 6)
            min_filter_level = 0;
        else if (base_qindex <= 16)
            min_filter_level = 1;
        else
            min_filter_level = (base_qindex / 8);
    }

    return min_filter_level;
}


static int get_max_filter_level(VP8_COMP *cpi, int base_qindex)
{
    

    


    int max_filter_level = MAX_LOOP_FILTER;
    (void)base_qindex;

    if (cpi->twopass.section_intra_rating > 8)
        max_filter_level = MAX_LOOP_FILTER * 3 / 4;

    return max_filter_level;
}

void vp8cx_pick_filter_level_fast(YV12_BUFFER_CONFIG *sd, VP8_COMP *cpi)
{
    VP8_COMMON *cm = &cpi->common;

    int best_err = 0;
    int filt_err = 0;
    int min_filter_level = get_min_filter_level(cpi, cm->base_qindex);
    int max_filter_level = get_max_filter_level(cpi, cm->base_qindex);
    int filt_val;
    int best_filt_val = cm->filter_level;
    YV12_BUFFER_CONFIG * saved_frame = cm->frame_to_show;

    
    cm->frame_to_show = &cpi->pick_lf_lvl_frame;

    if (cm->frame_type == KEY_FRAME)
        cm->sharpness_level = 0;
    else
        cm->sharpness_level = cpi->oxcf.Sharpness;

    if (cm->sharpness_level != cm->last_sharpness_level)
    {
        vp8_loop_filter_update_sharpness(&cm->lf_info, cm->sharpness_level);
        cm->last_sharpness_level = cm->sharpness_level;
    }

    


    if (cm->filter_level < min_filter_level)
        cm->filter_level = min_filter_level;
    else if (cm->filter_level > max_filter_level)
        cm->filter_level = max_filter_level;

    filt_val = cm->filter_level;
    best_filt_val = filt_val;

    

    
    vp8_yv12_copy_partial_frame(saved_frame, cm->frame_to_show);
    vp8_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

    best_err = calc_partial_ssl_err(sd, cm->frame_to_show);

    filt_val -= 1 + (filt_val > 10);

    
    while (filt_val >= min_filter_level)
    {
        
        vp8_yv12_copy_partial_frame(saved_frame, cm->frame_to_show);
        vp8_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

        
        filt_err = calc_partial_ssl_err(sd, cm->frame_to_show);

        
        if (filt_err < best_err)
        {
            best_err = filt_err;
            best_filt_val = filt_val;
        }
        else
            break;

        
        filt_val -= 1 + (filt_val > 10);
    }

    
    filt_val = cm->filter_level + 1 + (filt_val > 10);

    if (best_filt_val == cm->filter_level)
    {
        
        best_err -= (best_err >> 10);

        while (filt_val < max_filter_level)
        {
            
            vp8_yv12_copy_partial_frame(saved_frame, cm->frame_to_show);

            vp8_loop_filter_partial_frame(cm, &cpi->mb.e_mbd, filt_val);

            
            filt_err = calc_partial_ssl_err(sd, cm->frame_to_show);

            
            if (filt_err < best_err)
            {
                


                best_err = filt_err - (filt_err >> 10);

                best_filt_val = filt_val;
            }
            else
                break;

            
            filt_val += 1 + (filt_val > 10);
        }
    }

    cm->filter_level = best_filt_val;

    if (cm->filter_level < min_filter_level)
        cm->filter_level = min_filter_level;

    if (cm->filter_level > max_filter_level)
        cm->filter_level = max_filter_level;

    
    cm->frame_to_show = saved_frame;
}


void vp8cx_set_alt_lf_level(VP8_COMP *cpi, int filt_val)
{
    MACROBLOCKD *mbd = &cpi->mb.e_mbd;
    (void) filt_val;

    mbd->segment_feature_data[MB_LVL_ALT_LF][0] = cpi->segment_feature_data[MB_LVL_ALT_LF][0];
    mbd->segment_feature_data[MB_LVL_ALT_LF][1] = cpi->segment_feature_data[MB_LVL_ALT_LF][1];
    mbd->segment_feature_data[MB_LVL_ALT_LF][2] = cpi->segment_feature_data[MB_LVL_ALT_LF][2];
    mbd->segment_feature_data[MB_LVL_ALT_LF][3] = cpi->segment_feature_data[MB_LVL_ALT_LF][3];
}

void vp8cx_pick_filter_level(YV12_BUFFER_CONFIG *sd, VP8_COMP *cpi)
{
    VP8_COMMON *cm = &cpi->common;

    int best_err = 0;
    int filt_err = 0;
    int min_filter_level = get_min_filter_level(cpi, cm->base_qindex);
    int max_filter_level = get_max_filter_level(cpi, cm->base_qindex);

    int filter_step;
    int filt_high = 0;
    
    int filt_mid = cm->filter_level;
    int filt_low = 0;
    int filt_best;
    int filt_direction = 0;

    
    int Bias = 0;

    int ss_err[MAX_LOOP_FILTER + 1];

    YV12_BUFFER_CONFIG * saved_frame = cm->frame_to_show;

    vpx_memset(ss_err, 0, sizeof(ss_err));

    
    cm->frame_to_show = &cpi->pick_lf_lvl_frame;

    if (cm->frame_type == KEY_FRAME)
        cm->sharpness_level = 0;
    else
        cm->sharpness_level = cpi->oxcf.Sharpness;

    


    filt_mid = cm->filter_level;

    if (filt_mid < min_filter_level)
        filt_mid = min_filter_level;
    else if (filt_mid > max_filter_level)
        filt_mid = max_filter_level;

    
    filter_step = (filt_mid < 16) ? 4 : filt_mid / 4;

    

    
    vpx_yv12_copy_y(saved_frame, cm->frame_to_show);

    vp8cx_set_alt_lf_level(cpi, filt_mid);
    vp8_loop_filter_frame_yonly(cm, &cpi->mb.e_mbd, filt_mid);

    best_err = vp8_calc_ss_err(sd, cm->frame_to_show);

    ss_err[filt_mid] = best_err;

    filt_best = filt_mid;

    while (filter_step > 0)
    {
        Bias = (best_err >> (15 - (filt_mid / 8))) * filter_step;

        if (cpi->twopass.section_intra_rating < 20)
            Bias = Bias * cpi->twopass.section_intra_rating / 20;

        filt_high = ((filt_mid + filter_step) > max_filter_level) ? max_filter_level : (filt_mid + filter_step);
        filt_low = ((filt_mid - filter_step) < min_filter_level) ? min_filter_level : (filt_mid - filter_step);

        if ((filt_direction <= 0) && (filt_low != filt_mid))
        {
            if(ss_err[filt_low] == 0)
            {
                
                vpx_yv12_copy_y(saved_frame, cm->frame_to_show);
                vp8cx_set_alt_lf_level(cpi, filt_low);
                vp8_loop_filter_frame_yonly(cm, &cpi->mb.e_mbd, filt_low);

                filt_err = vp8_calc_ss_err(sd, cm->frame_to_show);
                ss_err[filt_low] = filt_err;
            }
            else
                filt_err = ss_err[filt_low];

            


            if ((filt_err - Bias) < best_err)
            {
                
                if (filt_err < best_err)
                    best_err = filt_err;

                filt_best = filt_low;
            }
        }

        
        if ((filt_direction >= 0) && (filt_high != filt_mid))
        {
            if(ss_err[filt_high] == 0)
            {
                vpx_yv12_copy_y(saved_frame, cm->frame_to_show);
                vp8cx_set_alt_lf_level(cpi, filt_high);
                vp8_loop_filter_frame_yonly(cm, &cpi->mb.e_mbd, filt_high);

                filt_err = vp8_calc_ss_err(sd, cm->frame_to_show);
                ss_err[filt_high] = filt_err;
            }
            else
                filt_err = ss_err[filt_high];

            
            if (filt_err < (best_err - Bias))
            {
                best_err = filt_err;
                filt_best = filt_high;
            }
        }

        


        if (filt_best == filt_mid)
        {
            filter_step = filter_step / 2;
            filt_direction = 0;
        }
        else
        {
            filt_direction = (filt_best < filt_mid) ? -1 : 1;
            filt_mid = filt_best;
        }
    }

    cm->filter_level = filt_best;

    
    cm->frame_to_show = saved_frame;
}
