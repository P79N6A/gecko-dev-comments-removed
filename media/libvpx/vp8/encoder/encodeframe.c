










#include "vpx_ports/config.h"
#include "encodemb.h"
#include "encodemv.h"
#include "vp8/common/common.h"
#include "onyx_int.h"
#include "vp8/common/extend.h"
#include "vp8/common/entropymode.h"
#include "vp8/common/quant_common.h"
#include "segmentation.h"
#include "vp8/common/setupintrarecon.h"
#include "encodeintra.h"
#include "vp8/common/reconinter.h"
#include "rdopt.h"
#include "pickinter.h"
#include "vp8/common/findnearmv.h"
#include "vp8/common/reconintra.h"
#include <stdio.h>
#include <limits.h>
#include "vp8/common/subpixel.h"
#include "vpx_ports/vpx_timer.h"

#if CONFIG_RUNTIME_CPU_DETECT
#define RTCD(x)     &cpi->common.rtcd.x
#define IF_RTCD(x)  (x)
#else
#define RTCD(x)     NULL
#define IF_RTCD(x)  NULL
#endif
extern void vp8_stuff_mb(VP8_COMP *cpi, MACROBLOCKD *x, TOKENEXTRA **t) ;

extern void vp8cx_initialize_me_consts(VP8_COMP *cpi, int QIndex);
extern void vp8_auto_select_speed(VP8_COMP *cpi);
extern void vp8cx_init_mbrthread_data(VP8_COMP *cpi,
                                      MACROBLOCK *x,
                                      MB_ROW_COMP *mbr_ei,
                                      int mb_row,
                                      int count);
void vp8_build_block_offsets(MACROBLOCK *x);
void vp8_setup_block_ptrs(MACROBLOCK *x);
int vp8cx_encode_inter_macroblock(VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t, int recon_yoffset, int recon_uvoffset);
int vp8cx_encode_intra_macro_block(VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t);
static void adjust_act_zbin( VP8_COMP *cpi, MACROBLOCK *x );

#ifdef MODE_STATS
unsigned int inter_y_modes[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int inter_uv_modes[4] = {0, 0, 0, 0};
unsigned int inter_b_modes[15]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int y_modes[5]   = {0, 0, 0, 0, 0};
unsigned int uv_modes[4]  = {0, 0, 0, 0};
unsigned int b_modes[14]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif







#define VP8_ACTIVITY_AVG_MIN (64)






static const unsigned char VP8_VAR_OFFS[16]=
{
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128
};



static unsigned int tt_activity_measure( VP8_COMP *cpi, MACROBLOCK *x )
{
    unsigned int act;
    unsigned int sse;
    






    act =     VARIANCE_INVOKE(&cpi->rtcd.variance, var16x16)(x->src.y_buffer,
                    x->src.y_stride, VP8_VAR_OFFS, 0, &sse);
    act = act<<4;

    
    if (act < 8<<12)
        act = act < 5<<12 ? act : 5<<12;

    return act;
}


static unsigned int alt_activity_measure( VP8_COMP *cpi,
                                          MACROBLOCK *x, int use_dc_pred )
{
    return vp8_encode_intra(cpi,x, use_dc_pred);
}




#define ALT_ACT_MEASURE 1
static unsigned int mb_activity_measure( VP8_COMP *cpi, MACROBLOCK *x,
                                  int mb_row, int mb_col)
{
    unsigned int mb_activity;

    if  ( ALT_ACT_MEASURE )
    {
        int use_dc_pred = (mb_col || mb_row) && (!mb_col || !mb_row);

        
        mb_activity = alt_activity_measure( cpi, x, use_dc_pred );
    }
    else
    {
        
        mb_activity = tt_activity_measure( cpi, x );
    }

    if ( mb_activity < VP8_ACTIVITY_AVG_MIN )
        mb_activity = VP8_ACTIVITY_AVG_MIN;

    return mb_activity;
}


#define ACT_MEDIAN 0
static void calc_av_activity( VP8_COMP *cpi, int64_t activity_sum )
{
#if ACT_MEDIAN
    
    {
        unsigned int median;
        unsigned int i,j;
        unsigned int * sortlist;
        unsigned int tmp;

        
        CHECK_MEM_ERROR(sortlist,
                        vpx_calloc(sizeof(unsigned int),
                        cpi->common.MBs));

        
        vpx_memcpy( sortlist, cpi->mb_activity_map,
                    sizeof(unsigned int) * cpi->common.MBs );


        
        for ( i = 1; i < cpi->common.MBs; i ++ )
        {
            for ( j = i; j > 0; j -- )
            {
                if ( sortlist[j] < sortlist[j-1] )
                {
                    
                    tmp = sortlist[j-1];
                    sortlist[j-1] = sortlist[j];
                    sortlist[j] = tmp;
                }
                else
                    break;
            }
        }

        
        median = ( 1 + sortlist[cpi->common.MBs >> 1] +
                   sortlist[(cpi->common.MBs >> 1) + 1] ) >> 1;

        cpi->activity_avg = median;

        vpx_free(sortlist);
    }
#else
    
    cpi->activity_avg = (unsigned int)(activity_sum/cpi->common.MBs);
#endif

    if (cpi->activity_avg < VP8_ACTIVITY_AVG_MIN)
        cpi->activity_avg = VP8_ACTIVITY_AVG_MIN;

    
    if  ( ALT_ACT_MEASURE )
        cpi->activity_avg = 100000;
}

#define USE_ACT_INDEX   0
#define OUTPUT_NORM_ACT_STATS   0

#if USE_ACT_INDEX

static void calc_activity_index( VP8_COMP *cpi, MACROBLOCK *x )
{
    VP8_COMMON *const cm = & cpi->common;
    int mb_row, mb_col;

    int64_t act;
    int64_t a;
    int64_t b;

#if OUTPUT_NORM_ACT_STATS
    FILE *f = fopen("norm_act.stt", "a");
    fprintf(f, "\n%12d\n", cpi->activity_avg );
#endif

    
    x->mb_activity_ptr = cpi->mb_activity_map;

    
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
        
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
            
            act = *(x->mb_activity_ptr);

            
            a = act + 4*cpi->activity_avg;
            b = 4*act + cpi->activity_avg;

            if ( b >= a )
                *(x->activity_ptr) = (int)((b + (a>>1))/a) - 1;
            else
                *(x->activity_ptr) = 1 - (int)((a + (b>>1))/b);

#if OUTPUT_NORM_ACT_STATS
            fprintf(f, " %6d", *(x->mb_activity_ptr));
#endif
            
            x->mb_activity_ptr++;
        }

#if OUTPUT_NORM_ACT_STATS
        fprintf(f, "\n");
#endif

    }

#if OUTPUT_NORM_ACT_STATS
    fclose(f);
#endif

}
#endif



static void build_activity_map( VP8_COMP *cpi )
{
    MACROBLOCK *const x = & cpi->mb;
    MACROBLOCKD *xd = &x->e_mbd;
    VP8_COMMON *const cm = & cpi->common;

#if ALT_ACT_MEASURE
    YV12_BUFFER_CONFIG *new_yv12 = &cm->yv12_fb[cm->new_fb_idx];
    int recon_yoffset;
    int recon_y_stride = new_yv12->y_stride;
#endif

    int mb_row, mb_col;
    unsigned int mb_activity;
    int64_t activity_sum = 0;

    
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
#if ALT_ACT_MEASURE
        
        xd->up_available = (mb_row != 0);
        recon_yoffset = (mb_row * recon_y_stride * 16);
#endif
        
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
#if ALT_ACT_MEASURE
            xd->dst.y_buffer = new_yv12->y_buffer + recon_yoffset;
            xd->left_available = (mb_col != 0);
            recon_yoffset += 16;
#endif
            
            RECON_INVOKE(&xd->rtcd->recon, copy16x16)(x->src.y_buffer, x->src.y_stride, x->thismb, 16);

            
            mb_activity = mb_activity_measure( cpi, x, mb_row, mb_col );

            
            activity_sum += mb_activity;

            
            *x->mb_activity_ptr = mb_activity;

            
            x->mb_activity_ptr++;

            
            x->src.y_buffer += 16;
        }


        
        x->src.y_buffer += 16 * x->src.y_stride - 16 * cm->mb_cols;

#if ALT_ACT_MEASURE
        
        vp8_extend_mb_row(new_yv12, xd->dst.y_buffer + 16,
                          xd->dst.u_buffer + 8, xd->dst.v_buffer + 8);
#endif

    }

    
    calc_av_activity(cpi, activity_sum);

#if USE_ACT_INDEX
    
    calc_activity_index( cpi, x );
#endif

}


void vp8_activity_masking(VP8_COMP *cpi, MACROBLOCK *x)
{
#if USE_ACT_INDEX
    x->rdmult += *(x->mb_activity_ptr) * (x->rdmult >> 2);
    x->errorperbit = x->rdmult * 100 /(110 * x->rddiv);
    x->errorperbit += (x->errorperbit==0);
#else
    int64_t a;
    int64_t b;
    int64_t act = *(x->mb_activity_ptr);

    
    a = act + (2*cpi->activity_avg);
    b = (2*act) + cpi->activity_avg;

    x->rdmult = (unsigned int)(((int64_t)x->rdmult*b + (a>>1))/a);
    x->errorperbit = x->rdmult * 100 /(110 * x->rddiv);
    x->errorperbit += (x->errorperbit==0);
#endif

    
    adjust_act_zbin(cpi, x);
}

static
void encode_mb_row(VP8_COMP *cpi,
                   VP8_COMMON *cm,
                   int mb_row,
                   MACROBLOCK  *x,
                   MACROBLOCKD *xd,
                   TOKENEXTRA **tp,
                   int *segment_counts,
                   int *totalrate)
{
    int i;
    int recon_yoffset, recon_uvoffset;
    int mb_col;
    int ref_fb_idx = cm->lst_fb_idx;
    int dst_fb_idx = cm->new_fb_idx;
    int recon_y_stride = cm->yv12_fb[ref_fb_idx].y_stride;
    int recon_uv_stride = cm->yv12_fb[ref_fb_idx].uv_stride;
    int map_index = (mb_row * cpi->common.mb_cols);

#if CONFIG_MULTITHREAD
    const int nsync = cpi->mt_sync_range;
    const int rightmost_col = cm->mb_cols - 1;
    volatile const int *last_row_current_mb_col;

    if ((cpi->b_multi_threaded != 0) && (mb_row != 0))
        last_row_current_mb_col = &cpi->mt_current_mb_col[mb_row - 1];
    else
        last_row_current_mb_col = &rightmost_col;
#endif

    
    xd->above_context = cm->above_context;

    xd->up_available = (mb_row != 0);
    recon_yoffset = (mb_row * recon_y_stride * 16);
    recon_uvoffset = (mb_row * recon_uv_stride * 8);

    cpi->tplist[mb_row].start = *tp;
    

    
    
    xd->mb_to_top_edge = -((mb_row * 16) << 3);
    xd->mb_to_bottom_edge = ((cm->mb_rows - 1 - mb_row) * 16) << 3;

    
    
    x->mv_row_min = -((mb_row * 16) + (VP8BORDERINPIXELS - 16));
    x->mv_row_max = ((cm->mb_rows - 1 - mb_row) * 16)
                        + (VP8BORDERINPIXELS - 16);

    
    x->mb_activity_ptr = &cpi->mb_activity_map[map_index];

    
    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
    {
        
        
        
        xd->mb_to_left_edge = -((mb_col * 16) << 3);
        xd->mb_to_right_edge = ((cm->mb_cols - 1 - mb_col) * 16) << 3;

        
        
        x->mv_col_min = -((mb_col * 16) + (VP8BORDERINPIXELS - 16));
        x->mv_col_max = ((cm->mb_cols - 1 - mb_col) * 16)
                            + (VP8BORDERINPIXELS - 16);

        xd->dst.y_buffer = cm->yv12_fb[dst_fb_idx].y_buffer + recon_yoffset;
        xd->dst.u_buffer = cm->yv12_fb[dst_fb_idx].u_buffer + recon_uvoffset;
        xd->dst.v_buffer = cm->yv12_fb[dst_fb_idx].v_buffer + recon_uvoffset;
        xd->left_available = (mb_col != 0);

        x->rddiv = cpi->RDDIV;
        x->rdmult = cpi->RDMULT;

        
        RECON_INVOKE(&xd->rtcd->recon, copy16x16)(x->src.y_buffer, x->src.y_stride, x->thismb, 16);

#if CONFIG_MULTITHREAD
        if ((cpi->b_multi_threaded != 0) && (mb_row != 0))
        {
            if ((mb_col & (nsync - 1)) == 0)
            {
                while (mb_col > (*last_row_current_mb_col - nsync)
                        && (*last_row_current_mb_col) != (cm->mb_cols - 1))
                {
                    x86_pause_hint();
                    thread_sleep(0);
                }
            }
        }
#endif

        if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
            vp8_activity_masking(cpi, x);

        
        
        if (xd->segmentation_enabled)
        {
            
            if (cpi->segmentation_map[map_index+mb_col] <= 3)
                xd->mode_info_context->mbmi.segment_id = cpi->segmentation_map[map_index+mb_col];
            else
                xd->mode_info_context->mbmi.segment_id = 0;

            vp8cx_mb_init_quantizer(cpi, x);
        }
        else
            xd->mode_info_context->mbmi.segment_id = 0;         

        x->active_ptr = cpi->active_map + map_index + mb_col;

        if (cm->frame_type == KEY_FRAME)
        {
            *totalrate += vp8cx_encode_intra_macro_block(cpi, x, tp);
#ifdef MODE_STATS
            y_modes[xd->mbmi.mode] ++;
#endif
        }
        else
        {
            *totalrate += vp8cx_encode_inter_macroblock(cpi, x, tp, recon_yoffset, recon_uvoffset);

#ifdef MODE_STATS
            inter_y_modes[xd->mbmi.mode] ++;

            if (xd->mbmi.mode == SPLITMV)
            {
                int b;

                for (b = 0; b < xd->mbmi.partition_count; b++)
                {
                    inter_b_modes[x->partition->bmi[b].mode] ++;
                }
            }

#endif

            
            if ((xd->mode_info_context->mbmi.mode == ZEROMV) && (xd->mode_info_context->mbmi.ref_frame == LAST_FRAME))
                cpi->inter_zz_count ++;

            
            
            
            if (cpi->cyclic_refresh_mode_enabled && xd->segmentation_enabled)
            {
                cpi->segmentation_map[map_index+mb_col] = xd->mode_info_context->mbmi.segment_id;

                
                
                
                if (xd->mode_info_context->mbmi.segment_id)
                    cpi->cyclic_refresh_map[map_index+mb_col] = -1;
                else if ((xd->mode_info_context->mbmi.mode == ZEROMV) && (xd->mode_info_context->mbmi.ref_frame == LAST_FRAME))
                {
                    if (cpi->cyclic_refresh_map[map_index+mb_col] == 1)
                        cpi->cyclic_refresh_map[map_index+mb_col] = 0;
                }
                else
                    cpi->cyclic_refresh_map[map_index+mb_col] = 1;

            }
        }

        cpi->tplist[mb_row].stop = *tp;

        
        x->gf_active_ptr++;

        
        x->mb_activity_ptr++;

        
        for (i = 0; i < 16; i++)
            xd->mode_info_context->bmi[i] = xd->block[i].bmi;

        
        x->src.y_buffer += 16;
        x->src.u_buffer += 8;
        x->src.v_buffer += 8;

        recon_yoffset += 16;
        recon_uvoffset += 8;

        
        segment_counts[xd->mode_info_context->mbmi.segment_id] ++;

        
        xd->mode_info_context++;
        x->partition_info++;

        xd->above_context++;
#if CONFIG_MULTITHREAD
        if (cpi->b_multi_threaded != 0)
        {
            cpi->mt_current_mb_col[mb_row] = mb_col;
        }
#endif
    }

    
    vp8_extend_mb_row(
        &cm->yv12_fb[dst_fb_idx],
        xd->dst.y_buffer + 16,
        xd->dst.u_buffer + 8,
        xd->dst.v_buffer + 8);

    
    xd->mode_info_context++;
    x->partition_info++;

#if CONFIG_MULTITHREAD
    if ((cpi->b_multi_threaded != 0) && (mb_row == cm->mb_rows - 1))
    {
        sem_post(&cpi->h_event_end_encoding); 
    }
#endif
}

void init_encode_frame_mb_context(VP8_COMP *cpi)
{
    MACROBLOCK *const x = & cpi->mb;
    VP8_COMMON *const cm = & cpi->common;
    MACROBLOCKD *const xd = & x->e_mbd;

    
    x->gf_active_ptr = (signed char *)cpi->gf_active_flags;

    
    x->mb_activity_ptr = cpi->mb_activity_map;

    x->vector_range = 32;

    x->act_zbin_adj = 0;

    x->partition_info = x->pi;

    xd->mode_info_context = cm->mi;
    xd->mode_info_stride = cm->mode_info_stride;

    xd->frame_type = cm->frame_type;

    xd->frames_since_golden = cm->frames_since_golden;
    xd->frames_till_alt_ref_frame = cm->frames_till_alt_ref_frame;

    
    if (cm->frame_type == KEY_FRAME)
        vp8_init_mbmode_probs(cm);

    
    x->src = * cpi->Source;
    xd->pre = cm->yv12_fb[cm->lst_fb_idx];
    xd->dst = cm->yv12_fb[cm->new_fb_idx];

    
    vp8_setup_intra_recon(&cm->yv12_fb[cm->new_fb_idx]);

    vp8_build_block_offsets(x);

    vp8_setup_block_dptrs(&x->e_mbd);

    vp8_setup_block_ptrs(x);

    xd->mode_info_context->mbmi.mode = DC_PRED;
    xd->mode_info_context->mbmi.uv_mode = DC_PRED;

    xd->left_context = &cm->left_context;

    vp8_zero(cpi->count_mb_ref_frame_usage)
    vp8_zero(cpi->ymode_count)
    vp8_zero(cpi->uv_mode_count)

    x->mvc = cm->fc.mvc;

    vpx_memset(cm->above_context, 0,
               sizeof(ENTROPY_CONTEXT_PLANES) * cm->mb_cols);

    xd->ref_frame_cost[INTRA_FRAME]   = vp8_cost_zero(cpi->prob_intra_coded);

    
    if (cpi->ref_frame_flags == VP8_LAST_FLAG)
    {
        xd->ref_frame_cost[LAST_FRAME]    = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_zero(255);
        xd->ref_frame_cost[GOLDEN_FRAME]  = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_one(255)
                                        + vp8_cost_zero(128);
        xd->ref_frame_cost[ALTREF_FRAME]  = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_one(255)
                                        + vp8_cost_one(128);
    }
    else
    {
        xd->ref_frame_cost[LAST_FRAME]    = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_zero(cpi->prob_last_coded);
        xd->ref_frame_cost[GOLDEN_FRAME]  = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_one(cpi->prob_last_coded)
                                        + vp8_cost_zero(cpi->prob_gf_coded);
        xd->ref_frame_cost[ALTREF_FRAME]  = vp8_cost_one(cpi->prob_intra_coded)
                                        + vp8_cost_one(cpi->prob_last_coded)
                                        + vp8_cost_one(cpi->prob_gf_coded);
    }

}

void vp8_encode_frame(VP8_COMP *cpi)
{
    int mb_row;
    MACROBLOCK *const x = & cpi->mb;
    VP8_COMMON *const cm = & cpi->common;
    MACROBLOCKD *const xd = & x->e_mbd;

    TOKENEXTRA *tp = cpi->tok;
    int segment_counts[MAX_MB_SEGMENTS];
    int totalrate;

    vpx_memset(segment_counts, 0, sizeof(segment_counts));
    totalrate = 0;

    if (cpi->compressor_speed == 2)
    {
        if (cpi->oxcf.cpu_used < 0)
            cpi->Speed = -(cpi->oxcf.cpu_used);
        else
            vp8_auto_select_speed(cpi);
    }

    
    if (cm->mcomp_filter_type == SIXTAP)
    {
        xd->subpixel_predict        = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, sixtap4x4);
        xd->subpixel_predict8x4     = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, sixtap8x4);
        xd->subpixel_predict8x8     = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, sixtap8x8);
        xd->subpixel_predict16x16   = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, sixtap16x16);
    }
    else
    {
        xd->subpixel_predict        = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, bilinear4x4);
        xd->subpixel_predict8x4     = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, bilinear8x4);
        xd->subpixel_predict8x8     = SUBPIX_INVOKE(
                                        &cpi->common.rtcd.subpix, bilinear8x8);
        xd->subpixel_predict16x16   = SUBPIX_INVOKE(
                                      &cpi->common.rtcd.subpix, bilinear16x16);
    }

    
    cpi->inter_zz_count = 0;

    vpx_memset(segment_counts, 0, sizeof(segment_counts));

    cpi->prediction_error = 0;
    cpi->intra_error = 0;
    cpi->skip_true_count = 0;
    cpi->skip_false_count = 0;

#if 0
    
    cpi->frame_distortion = 0;
    cpi->last_mb_distortion = 0;
#endif

    xd->mode_info_context = cm->mi;

    vp8_zero(cpi->MVcount);
    vp8_zero(cpi->coef_counts);

    vp8cx_frame_init_quantizer(cpi);

    vp8_initialize_rd_consts(cpi,
                             vp8_dc_quant(cm->base_qindex, cm->y1dc_delta_q));

    vp8cx_initialize_me_consts(cpi, cm->base_qindex);

    if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
    {
        
        init_encode_frame_mb_context(cpi);

        
        build_activity_map(cpi);
    }

    
    init_encode_frame_mb_context(cpi);

    {
        struct vpx_usec_timer  emr_timer;
        vpx_usec_timer_start(&emr_timer);

#if CONFIG_MULTITHREAD
        if (cpi->b_multi_threaded)
        {
            int i;

            vp8cx_init_mbrthread_data(cpi, x, cpi->mb_row_ei, 1,  cpi->encoding_thread_count);

            for (i = 0; i < cm->mb_rows; i++)
                cpi->mt_current_mb_col[i] = -1;

            for (i = 0; i < cpi->encoding_thread_count; i++)
            {
                sem_post(&cpi->h_event_start_encoding[i]);
            }

            for (mb_row = 0; mb_row < cm->mb_rows; mb_row += (cpi->encoding_thread_count + 1))
            {
                vp8_zero(cm->left_context)

                tp = cpi->tok + mb_row * (cm->mb_cols * 16 * 24);

                encode_mb_row(cpi, cm, mb_row, x, xd, &tp, segment_counts, &totalrate);

                
                x->src.y_buffer += 16 * x->src.y_stride * (cpi->encoding_thread_count + 1) - 16 * cm->mb_cols;
                x->src.u_buffer +=  8 * x->src.uv_stride * (cpi->encoding_thread_count + 1) - 8 * cm->mb_cols;
                x->src.v_buffer +=  8 * x->src.uv_stride * (cpi->encoding_thread_count + 1) - 8 * cm->mb_cols;

                xd->mode_info_context += xd->mode_info_stride * cpi->encoding_thread_count;
                x->partition_info  += xd->mode_info_stride * cpi->encoding_thread_count;
                x->gf_active_ptr   += cm->mb_cols * cpi->encoding_thread_count;

            }

            sem_wait(&cpi->h_event_end_encoding); 

            cpi->tok_count = 0;

            for (mb_row = 0; mb_row < cm->mb_rows; mb_row ++)
            {
                cpi->tok_count += cpi->tplist[mb_row].stop - cpi->tplist[mb_row].start;
            }

            if (xd->segmentation_enabled)
            {
                int i, j;

                if (xd->segmentation_enabled)
                {

                    for (i = 0; i < cpi->encoding_thread_count; i++)
                    {
                        for (j = 0; j < 4; j++)
                            segment_counts[j] += cpi->mb_row_ei[i].segment_counts[j];
                    }
                }
            }

            for (i = 0; i < cpi->encoding_thread_count; i++)
            {
                totalrate += cpi->mb_row_ei[i].totalrate;
            }

        }
        else
#endif
        {
            
            for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
            {

                vp8_zero(cm->left_context)

                encode_mb_row(cpi, cm, mb_row, x, xd, &tp, segment_counts, &totalrate);

                
                x->src.y_buffer += 16 * x->src.y_stride - 16 * cm->mb_cols;
                x->src.u_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
                x->src.v_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
            }

            cpi->tok_count = tp - cpi->tok;

        }

        vpx_usec_timer_mark(&emr_timer);
        cpi->time_encode_mb_row += vpx_usec_timer_elapsed(&emr_timer);

    }


    
    if (xd->segmentation_enabled)
    {
        int tot_count;
        int i;

        
        vpx_memset(xd->mb_segment_tree_probs, 255 , sizeof(xd->mb_segment_tree_probs));

        tot_count = segment_counts[0] + segment_counts[1] + segment_counts[2] + segment_counts[3];

        if (tot_count)
        {
            xd->mb_segment_tree_probs[0] = ((segment_counts[0] + segment_counts[1]) * 255) / tot_count;

            tot_count = segment_counts[0] + segment_counts[1];

            if (tot_count > 0)
            {
                xd->mb_segment_tree_probs[1] = (segment_counts[0] * 255) / tot_count;
            }

            tot_count = segment_counts[2] + segment_counts[3];

            if (tot_count > 0)
                xd->mb_segment_tree_probs[2] = (segment_counts[2] * 255) / tot_count;

            
            for (i = 0; i < MB_FEATURE_TREE_PROBS; i ++)
            {
                if (xd->mb_segment_tree_probs[i] == 0)
                    xd->mb_segment_tree_probs[i] = 1;
            }
        }
    }

    
    cpi->projected_frame_size = totalrate >> 8;   

    
    if (cm->frame_type == KEY_FRAME)
    {
        cpi->this_frame_percent_intra = 100;
    }
    else
    {
        int tot_modes;

        tot_modes = cpi->count_mb_ref_frame_usage[INTRA_FRAME]
                    + cpi->count_mb_ref_frame_usage[LAST_FRAME]
                    + cpi->count_mb_ref_frame_usage[GOLDEN_FRAME]
                    + cpi->count_mb_ref_frame_usage[ALTREF_FRAME];

        if (tot_modes)
            cpi->this_frame_percent_intra = cpi->count_mb_ref_frame_usage[INTRA_FRAME] * 100 / tot_modes;

    }

#if 0
    {
        int cnt = 0;
        int flag[2] = {0, 0};

        for (cnt = 0; cnt < MVPcount; cnt++)
        {
            if (cm->fc.pre_mvc[0][cnt] != cm->fc.mvc[0][cnt])
            {
                flag[0] = 1;
                vpx_memcpy(cm->fc.pre_mvc[0], cm->fc.mvc[0], MVPcount);
                break;
            }
        }

        for (cnt = 0; cnt < MVPcount; cnt++)
        {
            if (cm->fc.pre_mvc[1][cnt] != cm->fc.mvc[1][cnt])
            {
                flag[1] = 1;
                vpx_memcpy(cm->fc.pre_mvc[1], cm->fc.mvc[1], MVPcount);
                break;
            }
        }

        if (flag[0] || flag[1])
            vp8_build_component_cost_table(cpi->mb.mvcost, (const MV_CONTEXT *) cm->fc.mvc, flag);
    }
#endif

    
    
    
    if ((cm->frame_type != KEY_FRAME) && !cm->refresh_alt_ref_frame && !cm->refresh_golden_frame)
    {
        const int *const rfct = cpi->count_mb_ref_frame_usage;
        const int rf_intra = rfct[INTRA_FRAME];
        const int rf_inter = rfct[LAST_FRAME] + rfct[GOLDEN_FRAME] + rfct[ALTREF_FRAME];

        if ((rf_intra + rf_inter) > 0)
        {
            cpi->prob_intra_coded = (rf_intra * 255) / (rf_intra + rf_inter);

            if (cpi->prob_intra_coded < 1)
                cpi->prob_intra_coded = 1;

            if ((cm->frames_since_golden > 0) || cpi->source_alt_ref_active)
            {
                cpi->prob_last_coded = rf_inter ? (rfct[LAST_FRAME] * 255) / rf_inter : 128;

                if (cpi->prob_last_coded < 1)
                    cpi->prob_last_coded = 1;

                cpi->prob_gf_coded = (rfct[GOLDEN_FRAME] + rfct[ALTREF_FRAME])
                                     ? (rfct[GOLDEN_FRAME] * 255) / (rfct[GOLDEN_FRAME] + rfct[ALTREF_FRAME]) : 128;

                if (cpi->prob_gf_coded < 1)
                    cpi->prob_gf_coded = 1;
            }
        }
    }

#if 0
    
    cpi->last_frame_distortion = cpi->frame_distortion;
#endif

}
void vp8_setup_block_ptrs(MACROBLOCK *x)
{
    int r, c;
    int i;

    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            x->block[r*4+c].src_diff = x->src_diff + r * 4 * 16 + c * 4;
        }
    }

    for (r = 0; r < 2; r++)
    {
        for (c = 0; c < 2; c++)
        {
            x->block[16 + r*2+c].src_diff = x->src_diff + 256 + r * 4 * 8 + c * 4;
        }
    }


    for (r = 0; r < 2; r++)
    {
        for (c = 0; c < 2; c++)
        {
            x->block[20 + r*2+c].src_diff = x->src_diff + 320 + r * 4 * 8 + c * 4;
        }
    }

    x->block[24].src_diff = x->src_diff + 384;


    for (i = 0; i < 25; i++)
    {
        x->block[i].coeff = x->coeff + i * 16;
    }
}

void vp8_build_block_offsets(MACROBLOCK *x)
{
    int block = 0;
    int br, bc;

    vp8_build_block_doffsets(&x->e_mbd);

    
    x->thismb_ptr = &x->thismb[0];
    for (br = 0; br < 4; br++)
    {
        for (bc = 0; bc < 4; bc++)
        {
            BLOCK *this_block = &x->block[block];
            
            
            
            this_block->base_src = &x->thismb_ptr;
            this_block->src_stride = 16;
            this_block->src = 4 * br * 16 + 4 * bc;
            ++block;
        }
    }

    
    for (br = 0; br < 2; br++)
    {
        for (bc = 0; bc < 2; bc++)
        {
            BLOCK *this_block = &x->block[block];
            this_block->base_src = &x->src.u_buffer;
            this_block->src_stride = x->src.uv_stride;
            this_block->src = 4 * br * this_block->src_stride + 4 * bc;
            ++block;
        }
    }

    
    for (br = 0; br < 2; br++)
    {
        for (bc = 0; bc < 2; bc++)
        {
            BLOCK *this_block = &x->block[block];
            this_block->base_src = &x->src.v_buffer;
            this_block->src_stride = x->src.uv_stride;
            this_block->src = 4 * br * this_block->src_stride + 4 * bc;
            ++block;
        }
    }
}

static void sum_intra_stats(VP8_COMP *cpi, MACROBLOCK *x)
{
    const MACROBLOCKD *xd = & x->e_mbd;
    const MB_PREDICTION_MODE m = xd->mode_info_context->mbmi.mode;
    const MB_PREDICTION_MODE uvm = xd->mode_info_context->mbmi.uv_mode;

#ifdef MODE_STATS
    const int is_key = cpi->common.frame_type == KEY_FRAME;

    ++ (is_key ? uv_modes : inter_uv_modes)[uvm];

    if (m == B_PRED)
    {
        unsigned int *const bct = is_key ? b_modes : inter_b_modes;

        int b = 0;

        do
        {
            ++ bct[xd->block[b].bmi.mode];
        }
        while (++b < 16);
    }

#endif

    ++cpi->ymode_count[m];
    ++cpi->uv_mode_count[uvm];

}



static void adjust_act_zbin( VP8_COMP *cpi, MACROBLOCK *x )
{
#if USE_ACT_INDEX
    x->act_zbin_adj = *(x->mb_activity_ptr);
#else
    int64_t a;
    int64_t b;
    int64_t act = *(x->mb_activity_ptr);

    
    a = act + 4*cpi->activity_avg;
    b = 4*act + cpi->activity_avg;

    if ( act > cpi->activity_avg )
        x->act_zbin_adj = (int)(((int64_t)b + (a>>1))/a) - 1;
    else
        x->act_zbin_adj = 1 - (int)(((int64_t)a + (b>>1))/b);
#endif
}

int vp8cx_encode_intra_macro_block(VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t)
{
    int rate;

    if (cpi->sf.RD && cpi->compressor_speed != 2)
        vp8_rd_pick_intra_mode(cpi, x, &rate);
    else
        vp8_pick_intra_mode(cpi, x, &rate);

    if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
    {
        adjust_act_zbin( cpi, x );
        vp8_update_zbin_extra(cpi, x);
    }

    if (x->e_mbd.mode_info_context->mbmi.mode == B_PRED)
        vp8_encode_intra4x4mby(IF_RTCD(&cpi->rtcd), x);
    else
        vp8_encode_intra16x16mby(IF_RTCD(&cpi->rtcd), x);

    vp8_encode_intra16x16mbuv(IF_RTCD(&cpi->rtcd), x);
    sum_intra_stats(cpi, x);
    vp8_tokenize_mb(cpi, &x->e_mbd, t);

    return rate;
}
#ifdef SPEEDSTATS
extern int cnt_pm;
#endif

extern void vp8_fix_contexts(MACROBLOCKD *x);

int vp8cx_encode_inter_macroblock
(
    VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t,
    int recon_yoffset, int recon_uvoffset
)
{
    MACROBLOCKD *const xd = &x->e_mbd;
    int intra_error = 0;
    int rate;
    int distortion;

    x->skip = 0;

    if (xd->segmentation_enabled)
        x->encode_breakout = cpi->segment_encode_breakout[xd->mode_info_context->mbmi.segment_id];
    else
        x->encode_breakout = cpi->oxcf.encode_breakout;

    if (cpi->sf.RD)
    {
        int zbin_mode_boost_enabled = cpi->zbin_mode_boost_enabled;

        
        if(cpi->sf.use_fastquant_for_pick)
        {
            cpi->mb.quantize_b      = QUANTIZE_INVOKE(&cpi->rtcd.quantize,
                                                      fastquantb);
            cpi->mb.quantize_b_pair = QUANTIZE_INVOKE(&cpi->rtcd.quantize,
                                                      fastquantb_pair);

            

            cpi->zbin_mode_boost_enabled = 0;
        }
        vp8_rd_pick_inter_mode(cpi, x, recon_yoffset, recon_uvoffset, &rate,
                               &distortion, &intra_error);

        
        if (cpi->sf.improved_quant)
        {
            cpi->mb.quantize_b      = QUANTIZE_INVOKE(&cpi->rtcd.quantize,
                                                      quantb);
            cpi->mb.quantize_b_pair = QUANTIZE_INVOKE(&cpi->rtcd.quantize,
                                                      quantb_pair);
        }

        
        cpi->zbin_mode_boost_enabled = zbin_mode_boost_enabled;

    }
    else
        vp8_pick_inter_mode(cpi, x, recon_yoffset, recon_uvoffset, &rate,
                            &distortion, &intra_error);

    cpi->prediction_error += distortion;
    cpi->intra_error += intra_error;

    if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
    {
        
        adjust_act_zbin( cpi, x );
    }

#if 0
    
    cpi->frame_distortion += distortion;
    cpi->last_mb_distortion = distortion;
#endif

    
    if (xd->segmentation_enabled)
    {
        
        if (cpi->cyclic_refresh_mode_enabled)
        {
            
            if ((xd->mode_info_context->mbmi.segment_id == 1) &&
                ((xd->mode_info_context->mbmi.ref_frame != LAST_FRAME) || (xd->mode_info_context->mbmi.mode != ZEROMV)))
            {
                xd->mode_info_context->mbmi.segment_id = 0;

                
                vp8cx_mb_init_quantizer(cpi, x);
            }
        }
    }

    {
        
        
        cpi->zbin_mode_boost = 0;
        if (cpi->zbin_mode_boost_enabled)
        {
            if ( xd->mode_info_context->mbmi.ref_frame != INTRA_FRAME )
            {
                if (xd->mode_info_context->mbmi.mode == ZEROMV)
                {
                    if (xd->mode_info_context->mbmi.ref_frame != LAST_FRAME)
                        cpi->zbin_mode_boost = GF_ZEROMV_ZBIN_BOOST;
                    else
                        cpi->zbin_mode_boost = LF_ZEROMV_ZBIN_BOOST;
                }
                else if (xd->mode_info_context->mbmi.mode == SPLITMV)
                    cpi->zbin_mode_boost = 0;
                else
                    cpi->zbin_mode_boost = MV_ZBIN_BOOST;
            }
        }
        vp8_update_zbin_extra(cpi, x);
    }

    cpi->count_mb_ref_frame_usage[xd->mode_info_context->mbmi.ref_frame] ++;

    if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {
        vp8_encode_intra16x16mbuv(IF_RTCD(&cpi->rtcd), x);

        if (xd->mode_info_context->mbmi.mode == B_PRED)
        {
            vp8_encode_intra4x4mby(IF_RTCD(&cpi->rtcd), x);
        }
        else
        {
            vp8_encode_intra16x16mby(IF_RTCD(&cpi->rtcd), x);
        }

        sum_intra_stats(cpi, x);
    }
    else
    {
        int ref_fb_idx;

        vp8_build_uvmvs(xd, cpi->common.full_pixel);

        if (xd->mode_info_context->mbmi.ref_frame == LAST_FRAME)
            ref_fb_idx = cpi->common.lst_fb_idx;
        else if (xd->mode_info_context->mbmi.ref_frame == GOLDEN_FRAME)
            ref_fb_idx = cpi->common.gld_fb_idx;
        else
            ref_fb_idx = cpi->common.alt_fb_idx;

        xd->pre.y_buffer = cpi->common.yv12_fb[ref_fb_idx].y_buffer + recon_yoffset;
        xd->pre.u_buffer = cpi->common.yv12_fb[ref_fb_idx].u_buffer + recon_uvoffset;
        xd->pre.v_buffer = cpi->common.yv12_fb[ref_fb_idx].v_buffer + recon_uvoffset;

        if (!x->skip)
        {
            vp8_encode_inter16x16(IF_RTCD(&cpi->rtcd), x);

            
            if (!cpi->common.mb_no_coeff_skip)
                xd->mode_info_context->mbmi.mb_skip_coeff = 0;

        }
        else
            vp8_build_inter16x16_predictors_mb(xd, xd->dst.y_buffer,
                                           xd->dst.u_buffer, xd->dst.v_buffer,
                                           xd->dst.y_stride, xd->dst.uv_stride);

    }

    if (!x->skip)
        vp8_tokenize_mb(cpi, xd, t);
    else
    {
        if (cpi->common.mb_no_coeff_skip)
        {
            xd->mode_info_context->mbmi.mb_skip_coeff = 1;
            cpi->skip_true_count ++;
            vp8_fix_contexts(xd);
        }
        else
        {
            vp8_stuff_mb(cpi, xd, t);
            xd->mode_info_context->mbmi.mb_skip_coeff = 0;
            cpi->skip_false_count ++;
        }
    }

    return rate;
}
