










#include "vpx_config.h"
#include "vp8_rtcd.h"
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
#include <stdio.h>
#include <limits.h>
#include "vp8/common/invtrans.h"
#include "vpx_ports/vpx_timer.h"
#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
#include "bitstream.h"
#endif
#include "encodeframe.h"

extern void vp8_stuff_mb(VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t) ;
extern void vp8_calc_ref_frame_costs(int *ref_frame_cost,
                                     int prob_intra,
                                     int prob_last,
                                     int prob_garf
                                    );
extern void vp8_convert_rfct_to_prob(VP8_COMP *const cpi);
extern void vp8cx_initialize_me_consts(VP8_COMP *cpi, int QIndex);
extern void vp8_auto_select_speed(VP8_COMP *cpi);
extern void vp8cx_init_mbrthread_data(VP8_COMP *cpi,
                                      MACROBLOCK *x,
                                      MB_ROW_COMP *mbr_ei,
                                      int count);
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
    






    act =  vp8_variance16x16(x->src.y_buffer,
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
            
            vp8_copy_mem16x16(x->src.y_buffer, x->src.y_stride, x->thismb, 16);

            
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
    int recon_yoffset, recon_uvoffset;
    int mb_col;
    int ref_fb_idx = cm->lst_fb_idx;
    int dst_fb_idx = cm->new_fb_idx;
    int recon_y_stride = cm->yv12_fb[ref_fb_idx].y_stride;
    int recon_uv_stride = cm->yv12_fb[ref_fb_idx].uv_stride;
    int map_index = (mb_row * cpi->common.mb_cols);

#if (CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING)
    const int num_part = (1 << cm->multi_token_partition);
    TOKENEXTRA * tp_start = cpi->tok;
    vp8_writer *w;
#endif

#if CONFIG_MULTITHREAD
    const int nsync = cpi->mt_sync_range;
    const int rightmost_col = cm->mb_cols + nsync;
    volatile const int *last_row_current_mb_col;
    volatile int *current_mb_col = &cpi->mt_current_mb_col[mb_row];

    if ((cpi->b_multi_threaded != 0) && (mb_row != 0))
        last_row_current_mb_col = &cpi->mt_current_mb_col[mb_row - 1];
    else
        last_row_current_mb_col = &rightmost_col;
#endif

#if (CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING)
    if(num_part > 1)
        w= &cpi->bc[1 + (mb_row % num_part)];
    else
        w = &cpi->bc[1];
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

#if  (CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING)
        *tp = cpi->tok;
#endif
        



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

        
        vp8_copy_mem16x16(x->src.y_buffer, x->src.y_stride, x->thismb, 16);

#if CONFIG_MULTITHREAD
        if (cpi->b_multi_threaded != 0)
        {
            *current_mb_col = mb_col - 1; 

            if ((mb_col & (nsync - 1)) == 0)
            {
                while (mb_col > (*last_row_current_mb_col - nsync))
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

            vp8cx_mb_init_quantizer(cpi, x, 1);
        }
        else
            
            xd->mode_info_context->mbmi.segment_id = 0;

        x->active_ptr = cpi->active_map + map_index + mb_col;

        if (cm->frame_type == KEY_FRAME)
        {
            *totalrate += vp8cx_encode_intra_macroblock(cpi, x, tp);
#ifdef MODE_STATS
            y_modes[xd->mbmi.mode] ++;
#endif
        }
        else
        {
            *totalrate += vp8cx_encode_inter_macroblock(cpi, x, tp, recon_yoffset, recon_uvoffset, mb_row, mb_col);

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

            





            if ((cpi->current_layer == 0) &&
                (cpi->cyclic_refresh_mode_enabled &&
                 xd->segmentation_enabled))
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

#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
        
        {
            int tok_count = *tp - tp_start;
            pack_tokens(w, tp_start, tok_count);
        }
#endif
        
        x->gf_active_ptr++;

        
        x->mb_activity_ptr++;

        
        x->src.y_buffer += 16;
        x->src.u_buffer += 8;
        x->src.v_buffer += 8;

        recon_yoffset += 16;
        recon_uvoffset += 8;

        
        segment_counts[xd->mode_info_context->mbmi.segment_id] ++;

        
        xd->mode_info_context++;
        x->partition_info++;
        xd->above_context++;
    }

    
    vp8_extend_mb_row( &cm->yv12_fb[dst_fb_idx],
                        xd->dst.y_buffer + 16,
                        xd->dst.u_buffer + 8,
                        xd->dst.v_buffer + 8);

#if CONFIG_MULTITHREAD
    if (cpi->b_multi_threaded != 0)
        *current_mb_col = rightmost_col;
#endif

    
    xd->mode_info_context++;
    x->partition_info++;
}

static void init_encode_frame_mb_context(VP8_COMP *cpi)
{
    MACROBLOCK *const x = & cpi->mb;
    VP8_COMMON *const cm = & cpi->common;
    MACROBLOCKD *const xd = & x->e_mbd;

    
    x->gf_active_ptr = (signed char *)cpi->gf_active_flags;

    
    x->mb_activity_ptr = cpi->mb_activity_map;

    x->act_zbin_adj = 0;

    x->partition_info = x->pi;

    xd->mode_info_context = cm->mi;
    xd->mode_info_stride = cm->mode_info_stride;

    xd->frame_type = cm->frame_type;

    
    if (cm->frame_type == KEY_FRAME)
        vp8_init_mbmode_probs(cm);

    
    x->src = * cpi->Source;
    xd->pre = cm->yv12_fb[cm->lst_fb_idx];
    xd->dst = cm->yv12_fb[cm->new_fb_idx];

    
    vp8_setup_intra_recon(&cm->yv12_fb[cm->new_fb_idx]);

    vp8_build_block_offsets(x);

    xd->mode_info_context->mbmi.mode = DC_PRED;
    xd->mode_info_context->mbmi.uv_mode = DC_PRED;

    xd->left_context = &cm->left_context;

    x->mvc = cm->fc.mvc;

    vpx_memset(cm->above_context, 0,
               sizeof(ENTROPY_CONTEXT_PLANES) * cm->mb_cols);

    


    if (cpi->ref_frame_flags == VP8_LAST_FRAME)
        vp8_calc_ref_frame_costs(x->ref_frame_cost,
                                 cpi->prob_intra_coded,255,128);
    else if ((cpi->oxcf.number_of_layers > 1) &&
               (cpi->ref_frame_flags == VP8_GOLD_FRAME))
        vp8_calc_ref_frame_costs(x->ref_frame_cost,
                                 cpi->prob_intra_coded,1,255);
    else if ((cpi->oxcf.number_of_layers > 1) &&
                (cpi->ref_frame_flags == VP8_ALTR_FRAME))
        vp8_calc_ref_frame_costs(x->ref_frame_cost,
                                 cpi->prob_intra_coded,1,1);
    else
        vp8_calc_ref_frame_costs(x->ref_frame_cost,
                                 cpi->prob_intra_coded,
                                 cpi->prob_last_coded,
                                 cpi->prob_gf_coded);

    xd->fullpixel_mask = 0xffffffff;
    if(cm->full_pixel)
        xd->fullpixel_mask = 0xfffffff8;

    vp8_zero(x->coef_counts);
    vp8_zero(x->ymode_count);
    vp8_zero(x->uv_mode_count)
    x->prediction_error = 0;
    x->intra_error = 0;
    vp8_zero(x->count_mb_ref_frame_usage);
}

static void sum_coef_counts(MACROBLOCK *x, MACROBLOCK *x_thread)
{
    int i = 0;
    do
    {
        int j = 0;
        do
        {
            int k = 0;
            do
            {
                

                
                int t = 0;      

                do
                {
                    x->coef_counts [i][j][k][t] +=
                        x_thread->coef_counts [i][j][k][t];
                }
                while (++t < ENTROPY_NODES);
            }
            while (++k < PREV_COEF_CONTEXTS);
        }
        while (++j < COEF_BANDS);
    }
    while (++i < BLOCK_TYPES);
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
#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
    BOOL_CODER * bc = &cpi->bc[1]; 
    const int num_part = (1 << cm->multi_token_partition);
#endif

    vpx_memset(segment_counts, 0, sizeof(segment_counts));
    totalrate = 0;

    if (cpi->compressor_speed == 2)
    {
        if (cpi->oxcf.cpu_used < 0)
            cpi->Speed = -(cpi->oxcf.cpu_used);
        else
            vp8_auto_select_speed(cpi);
    }

    
    if(!cm->use_bilinear_mc_filter)
    {
        xd->subpixel_predict        = vp8_sixtap_predict4x4;
        xd->subpixel_predict8x4     = vp8_sixtap_predict8x4;
        xd->subpixel_predict8x8     = vp8_sixtap_predict8x8;
        xd->subpixel_predict16x16   = vp8_sixtap_predict16x16;
    }
    else
    {
        xd->subpixel_predict        = vp8_bilinear_predict4x4;
        xd->subpixel_predict8x4     = vp8_bilinear_predict8x4;
        xd->subpixel_predict8x8     = vp8_bilinear_predict8x8;
        xd->subpixel_predict16x16   = vp8_bilinear_predict16x16;
    }

    cpi->mb.skip_true_count = 0;
    cpi->tok_count = 0;

#if 0
    
    cpi->frame_distortion = 0;
    cpi->last_mb_distortion = 0;
#endif

    xd->mode_info_context = cm->mi;

    vp8_zero(cpi->mb.MVcount);

    vp8cx_frame_init_quantizer(cpi);

    vp8_initialize_rd_consts(cpi, x,
                             vp8_dc_quant(cm->base_qindex, cm->y1dc_delta_q));

    vp8cx_initialize_me_consts(cpi, cm->base_qindex);

    if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
    {
        
        init_encode_frame_mb_context(cpi);

        
        build_activity_map(cpi);
    }

    
    init_encode_frame_mb_context(cpi);

#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
    {
        int i;
        for(i = 0; i < num_part; i++)
        {
            vp8_start_encode(&bc[i], cpi->partition_d[i + 1],
                    cpi->partition_d_end[i + 1]);
            bc[i].error = &cm->error;
        }
    }

#endif

    {
        struct vpx_usec_timer  emr_timer;
        vpx_usec_timer_start(&emr_timer);

#if CONFIG_MULTITHREAD
        if (cpi->b_multi_threaded)
        {
            int i;

            vp8cx_init_mbrthread_data(cpi, x, cpi->mb_row_ei,
                                      cpi->encoding_thread_count);

            for (i = 0; i < cm->mb_rows; i++)
                cpi->mt_current_mb_col[i] = -1;

            for (i = 0; i < cpi->encoding_thread_count; i++)
            {
                sem_post(&cpi->h_event_start_encoding[i]);
            }

            for (mb_row = 0; mb_row < cm->mb_rows; mb_row += (cpi->encoding_thread_count + 1))
            {
                vp8_zero(cm->left_context)

#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
                tp = cpi->tok;
#else
                tp = cpi->tok + mb_row * (cm->mb_cols * 16 * 24);
#endif

                encode_mb_row(cpi, cm, mb_row, x, xd, &tp, segment_counts, &totalrate);

                
                x->src.y_buffer += 16 * x->src.y_stride * (cpi->encoding_thread_count + 1) - 16 * cm->mb_cols;
                x->src.u_buffer +=  8 * x->src.uv_stride * (cpi->encoding_thread_count + 1) - 8 * cm->mb_cols;
                x->src.v_buffer +=  8 * x->src.uv_stride * (cpi->encoding_thread_count + 1) - 8 * cm->mb_cols;

                xd->mode_info_context += xd->mode_info_stride * cpi->encoding_thread_count;
                x->partition_info  += xd->mode_info_stride * cpi->encoding_thread_count;
                x->gf_active_ptr   += cm->mb_cols * cpi->encoding_thread_count;

                if(mb_row == cm->mb_rows - 1)
                {
                    sem_post(&cpi->h_event_end_encoding); 
                }
            }

            sem_wait(&cpi->h_event_end_encoding); 

            for (mb_row = 0; mb_row < cm->mb_rows; mb_row ++)
            {
                cpi->tok_count += (unsigned int)
                  (cpi->tplist[mb_row].stop - cpi->tplist[mb_row].start);
            }

            if (xd->segmentation_enabled)
            {
                int j;

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
                int mode_count;
                int c_idx;
                totalrate += cpi->mb_row_ei[i].totalrate;

                cpi->mb.skip_true_count += cpi->mb_row_ei[i].mb.skip_true_count;

                for(mode_count = 0; mode_count < VP8_YMODES; mode_count++)
                    cpi->mb.ymode_count[mode_count] +=
                        cpi->mb_row_ei[i].mb.ymode_count[mode_count];

                for(mode_count = 0; mode_count < VP8_UV_MODES; mode_count++)
                    cpi->mb.uv_mode_count[mode_count] +=
                        cpi->mb_row_ei[i].mb.uv_mode_count[mode_count];

                for(c_idx = 0; c_idx < MVvals; c_idx++)
                {
                    cpi->mb.MVcount[0][c_idx] +=
                        cpi->mb_row_ei[i].mb.MVcount[0][c_idx];
                    cpi->mb.MVcount[1][c_idx] +=
                        cpi->mb_row_ei[i].mb.MVcount[1][c_idx];
                }

                cpi->mb.prediction_error +=
                    cpi->mb_row_ei[i].mb.prediction_error;
                cpi->mb.intra_error += cpi->mb_row_ei[i].mb.intra_error;

                for(c_idx = 0; c_idx < MAX_REF_FRAMES; c_idx++)
                    cpi->mb.count_mb_ref_frame_usage[c_idx] +=
                        cpi->mb_row_ei[i].mb.count_mb_ref_frame_usage[c_idx];

                for(c_idx = 0; c_idx < MAX_ERROR_BINS; c_idx++)
                    cpi->mb.error_bins[c_idx] +=
                        cpi->mb_row_ei[i].mb.error_bins[c_idx];

                
                sum_coef_counts(x, &cpi->mb_row_ei[i].mb);
            }

        }
        else
#endif
        {

            
            for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
            {
                vp8_zero(cm->left_context)

#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
                tp = cpi->tok;
#endif

                encode_mb_row(cpi, cm, mb_row, x, xd, &tp, segment_counts, &totalrate);

                
                x->src.y_buffer += 16 * x->src.y_stride - 16 * cm->mb_cols;
                x->src.u_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
                x->src.v_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
            }

            cpi->tok_count = (unsigned int)(tp - cpi->tok);
        }

#if CONFIG_REALTIME_ONLY & CONFIG_ONTHEFLY_BITPACKING
        {
            int i;
            for(i = 0; i < num_part; i++)
            {
                vp8_stop_encode(&bc[i]);
                cpi->partition_sz[i+1] = bc[i].pos;
            }
        }
#endif

        vpx_usec_timer_mark(&emr_timer);
        cpi->time_encode_mb_row += vpx_usec_timer_elapsed(&emr_timer);
    }


    
    
    if (xd->segmentation_enabled && xd->update_mb_segmentation_map)
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

        tot_modes = cpi->mb.count_mb_ref_frame_usage[INTRA_FRAME]
                    + cpi->mb.count_mb_ref_frame_usage[LAST_FRAME]
                    + cpi->mb.count_mb_ref_frame_usage[GOLDEN_FRAME]
                    + cpi->mb.count_mb_ref_frame_usage[ALTREF_FRAME];

        if (tot_modes)
            cpi->this_frame_percent_intra =
                cpi->mb.count_mb_ref_frame_usage[INTRA_FRAME] * 100 / tot_modes;

    }

#if ! CONFIG_REALTIME_ONLY
    




    if ((cm->frame_type != KEY_FRAME) && ((cpi->oxcf.number_of_layers > 1) ||
        (!cm->refresh_alt_ref_frame && !cm->refresh_golden_frame)))
    {
      vp8_convert_rfct_to_prob(cpi);
    }
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

    ++x->ymode_count[m];
    ++x->uv_mode_count[uvm];

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

int vp8cx_encode_intra_macroblock(VP8_COMP *cpi, MACROBLOCK *x,
                                  TOKENEXTRA **t)
{
    MACROBLOCKD *xd = &x->e_mbd;
    int rate;

    if (cpi->sf.RD && cpi->compressor_speed != 2)
        vp8_rd_pick_intra_mode(x, &rate);
    else
        vp8_pick_intra_mode(x, &rate);

    if(cpi->oxcf.tuning == VP8_TUNE_SSIM)
    {
        adjust_act_zbin( cpi, x );
        vp8_update_zbin_extra(cpi, x);
    }

    if (x->e_mbd.mode_info_context->mbmi.mode == B_PRED)
        vp8_encode_intra4x4mby(x);
    else
        vp8_encode_intra16x16mby(x);

    vp8_encode_intra16x16mbuv(x);

    sum_intra_stats(cpi, x);

    vp8_tokenize_mb(cpi, x, t);

    if (xd->mode_info_context->mbmi.mode != B_PRED)
        vp8_inverse_transform_mby(xd);

    vp8_dequant_idct_add_uv_block
                    (xd->qcoeff+16*16, xd->dequant_uv,
                     xd->dst.u_buffer, xd->dst.v_buffer,
                     xd->dst.uv_stride, xd->eobs+16);
    return rate;
}
#ifdef SPEEDSTATS
extern int cnt_pm;
#endif

extern void vp8_fix_contexts(MACROBLOCKD *x);

int vp8cx_encode_inter_macroblock
(
    VP8_COMP *cpi, MACROBLOCK *x, TOKENEXTRA **t,
    int recon_yoffset, int recon_uvoffset,
    int mb_row, int mb_col
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

#if CONFIG_TEMPORAL_DENOISING
    
    x->best_reference_frame = INTRA_FRAME;
    x->best_zeromv_reference_frame = INTRA_FRAME;
    x->best_sse_inter_mode = 0;
    x->best_sse_mv.as_int = 0;
    x->need_to_clamp_best_mvs = 0;
#endif

    if (cpi->sf.RD)
    {
        int zbin_mode_boost_enabled = x->zbin_mode_boost_enabled;

        
        if(cpi->sf.use_fastquant_for_pick)
        {
            x->quantize_b      = vp8_fast_quantize_b;
            x->quantize_b_pair = vp8_fast_quantize_b_pair;

            

            x->zbin_mode_boost_enabled = 0;
        }
        vp8_rd_pick_inter_mode(cpi, x, recon_yoffset, recon_uvoffset, &rate,
                               &distortion, &intra_error);

        
        if (cpi->sf.improved_quant)
        {
            x->quantize_b      = vp8_regular_quantize_b;
            x->quantize_b_pair = vp8_regular_quantize_b_pair;
        }

        
        x->zbin_mode_boost_enabled = zbin_mode_boost_enabled;

    }
    else
    {
        vp8_pick_inter_mode(cpi, x, recon_yoffset, recon_uvoffset, &rate,
                            &distortion, &intra_error, mb_row, mb_col);
    }

    x->prediction_error += distortion;
    x->intra_error += intra_error;

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
        
        if (cpi->current_layer == 0 && cpi->cyclic_refresh_mode_enabled)
        {
            
            if ((xd->mode_info_context->mbmi.segment_id == 1) &&
                ((xd->mode_info_context->mbmi.ref_frame != LAST_FRAME) || (xd->mode_info_context->mbmi.mode != ZEROMV)))
            {
                xd->mode_info_context->mbmi.segment_id = 0;

                
                vp8cx_mb_init_quantizer(cpi, x, 1);
            }
        }
    }

    {
        



        x->zbin_mode_boost = 0;
        if (x->zbin_mode_boost_enabled)
        {
            if ( xd->mode_info_context->mbmi.ref_frame != INTRA_FRAME )
            {
                if (xd->mode_info_context->mbmi.mode == ZEROMV)
                {
                    if (xd->mode_info_context->mbmi.ref_frame != LAST_FRAME &&
                        cpi->oxcf.number_of_layers == 1)
                        x->zbin_mode_boost = GF_ZEROMV_ZBIN_BOOST;
                    else
                        x->zbin_mode_boost = LF_ZEROMV_ZBIN_BOOST;
                }
                else if (xd->mode_info_context->mbmi.mode == SPLITMV)
                    x->zbin_mode_boost = 0;
                else
                    x->zbin_mode_boost = MV_ZBIN_BOOST;
            }
        }

        

        if (cpi->sf.improved_quant)
            vp8_update_zbin_extra(cpi, x);
    }

    x->count_mb_ref_frame_usage[xd->mode_info_context->mbmi.ref_frame] ++;

    if (xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {
        vp8_encode_intra16x16mbuv(x);

        if (xd->mode_info_context->mbmi.mode == B_PRED)
        {
            vp8_encode_intra4x4mby(x);
        }
        else
        {
            vp8_encode_intra16x16mby(x);
        }

        sum_intra_stats(cpi, x);
    }
    else
    {
        int ref_fb_idx;

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
            vp8_encode_inter16x16(x);
        }
        else
            vp8_build_inter16x16_predictors_mb(xd, xd->dst.y_buffer,
                                           xd->dst.u_buffer, xd->dst.v_buffer,
                                           xd->dst.y_stride, xd->dst.uv_stride);

    }

    if (!x->skip)
    {
        vp8_tokenize_mb(cpi, x, t);

        if (xd->mode_info_context->mbmi.mode != B_PRED)
            vp8_inverse_transform_mby(xd);

        vp8_dequant_idct_add_uv_block
                        (xd->qcoeff+16*16, xd->dequant_uv,
                         xd->dst.u_buffer, xd->dst.v_buffer,
                         xd->dst.uv_stride, xd->eobs+16);
    }
    else
    {
        
        xd->mode_info_context->mbmi.mb_skip_coeff = 1;

        if (cpi->common.mb_no_coeff_skip)
        {
            x->skip_true_count ++;
            vp8_fix_contexts(xd);
        }
        else
        {
            vp8_stuff_mb(cpi, x, t);
        }
    }

    return rate;
}
