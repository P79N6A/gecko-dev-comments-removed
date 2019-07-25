









#include "math.h"
#include "limits.h"
#include "block.h"
#include "onyx_int.h"
#include "variance.h"
#include "encodeintra.h"
#include "vp8/common/setupintrarecon.h"
#include "mcomp.h"
#include "firstpass.h"
#include "vpx_scale/vpxscale.h"
#include "encodemb.h"
#include "vp8/common/extend.h"
#include "vp8/common/systemdependent.h"
#include "vpx_scale/yv12extend.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8/common/swapyv12buffer.h"
#include <stdio.h>
#include "rdopt.h"
#include "vp8/common/quant_common.h"
#include "encodemv.h"



#if CONFIG_RUNTIME_CPU_DETECT
#define IF_RTCD(x) (x)
#else
#define IF_RTCD(x) NULL
#endif

extern void vp8_build_block_offsets(MACROBLOCK *x);
extern void vp8_setup_block_ptrs(MACROBLOCK *x);
extern void vp8cx_frame_init_quantizer(VP8_COMP *cpi);
extern void vp8_set_mbmode_and_mvs(MACROBLOCK *x, MB_PREDICTION_MODE mb, int_mv *mv);
extern void vp8_alloc_compressor_data(VP8_COMP *cpi);



#define GFQ_ADJUSTMENT vp8_gf_boost_qadjustment[Q]
extern int vp8_kf_boost_qadjustment[QINDEX_RANGE];

extern const int vp8_gf_boost_qadjustment[QINDEX_RANGE];

#define IIFACTOR   1.5
#define IIKFACTOR1 1.40
#define IIKFACTOR2 1.5
#define RMAX       14.0
#define GF_RMAX    48.0

#define KF_MB_INTRA_MIN 300
#define GF_MB_INTRA_MIN 200

#define DOUBLE_DIVIDE_CHECK(X) ((X)<0?(X)-.000001:(X)+.000001)

#define POW1 (double)cpi->oxcf.two_pass_vbrbias/100.0
#define POW2 (double)cpi->oxcf.two_pass_vbrbias/100.0

#define NEW_BOOST 1

static int vscale_lookup[7] = {0, 1, 1, 2, 2, 3, 3};
static int hscale_lookup[7] = {0, 0, 1, 1, 2, 2, 3};


static const int cq_level[QINDEX_RANGE] =
{
    0,0,1,1,2,3,3,4,4,5,6,6,7,8,8,9,
    9,10,11,11,12,13,13,14,15,15,16,17,17,18,19,20,
    20,21,22,22,23,24,24,25,26,27,27,28,29,30,30,31,
    32,33,33,34,35,36,36,37,38,39,39,40,41,42,42,43,
    44,45,46,46,47,48,49,50,50,51,52,53,54,55,55,56,
    57,58,59,60,60,61,62,63,64,65,66,67,67,68,69,70,
    71,72,73,74,75,75,76,77,78,79,80,81,82,83,84,85,
    86,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100
};

static void find_next_key_frame(VP8_COMP *cpi, FIRSTPASS_STATS *this_frame);


static void reset_fpf_position(VP8_COMP *cpi, FIRSTPASS_STATS *Position)
{
    cpi->twopass.stats_in = Position;
}

static int lookup_next_frame_stats(VP8_COMP *cpi, FIRSTPASS_STATS *next_frame)
{
    if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
        return EOF;

    *next_frame = *cpi->twopass.stats_in;
    return 1;
}


static int read_frame_stats( VP8_COMP *cpi,
                             FIRSTPASS_STATS *frame_stats,
                             int offset )
{
    FIRSTPASS_STATS * fps_ptr = cpi->twopass.stats_in;

    
    if ( offset >= 0 )
    {
        if ( &fps_ptr[offset] >= cpi->twopass.stats_in_end )
             return EOF;
    }
    else if ( offset < 0 )
    {
        if ( &fps_ptr[offset] < cpi->twopass.stats_in_start )
             return EOF;
    }

    *frame_stats = fps_ptr[offset];
    return 1;
}

static int input_stats(VP8_COMP *cpi, FIRSTPASS_STATS *fps)
{
    if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
        return EOF;

    *fps = *cpi->twopass.stats_in;
    cpi->twopass.stats_in =
         (void*)((char *)cpi->twopass.stats_in + sizeof(FIRSTPASS_STATS));
    return 1;
}

static void output_stats(const VP8_COMP            *cpi,
                         struct vpx_codec_pkt_list *pktlist,
                         FIRSTPASS_STATS            *stats)
{
    struct vpx_codec_cx_pkt pkt;
    pkt.kind = VPX_CODEC_STATS_PKT;
    pkt.data.twopass_stats.buf = stats;
    pkt.data.twopass_stats.sz = sizeof(FIRSTPASS_STATS);
    vpx_codec_pkt_list_add(pktlist, &pkt);


#if OUTPUT_FPF

    {
        FILE *fpfile;
        fpfile = fopen("firstpass.stt", "a");

        fprintf(fpfile, "%12.0f %12.0f %12.0f %12.4f %12.4f %12.4f %12.4f"
                " %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f"
                " %12.0f %12.4f\n",
                stats->frame,
                stats->intra_error,
                stats->coded_error,
                stats->ssim_weighted_pred_err,
                stats->pcnt_inter,
                stats->pcnt_motion,
                stats->pcnt_second_ref,
                stats->pcnt_neutral,
                stats->MVr,
                stats->mvr_abs,
                stats->MVc,
                stats->mvc_abs,
                stats->MVrv,
                stats->MVcv,
                stats->mv_in_out_count,
                stats->count,
                stats->duration);
        fclose(fpfile);
    }
#endif
}

static void zero_stats(FIRSTPASS_STATS *section)
{
    section->frame      = 0.0;
    section->intra_error = 0.0;
    section->coded_error = 0.0;
    section->ssim_weighted_pred_err = 0.0;
    section->pcnt_inter  = 0.0;
    section->pcnt_motion  = 0.0;
    section->pcnt_second_ref = 0.0;
    section->pcnt_neutral = 0.0;
    section->MVr        = 0.0;
    section->mvr_abs     = 0.0;
    section->MVc        = 0.0;
    section->mvc_abs     = 0.0;
    section->MVrv       = 0.0;
    section->MVcv       = 0.0;
    section->mv_in_out_count  = 0.0;
    section->count      = 0.0;
    section->duration   = 1.0;
}

static void accumulate_stats(FIRSTPASS_STATS *section, FIRSTPASS_STATS *frame)
{
    section->frame += frame->frame;
    section->intra_error += frame->intra_error;
    section->coded_error += frame->coded_error;
    section->ssim_weighted_pred_err += frame->ssim_weighted_pred_err;
    section->pcnt_inter  += frame->pcnt_inter;
    section->pcnt_motion += frame->pcnt_motion;
    section->pcnt_second_ref += frame->pcnt_second_ref;
    section->pcnt_neutral += frame->pcnt_neutral;
    section->MVr        += frame->MVr;
    section->mvr_abs     += frame->mvr_abs;
    section->MVc        += frame->MVc;
    section->mvc_abs     += frame->mvc_abs;
    section->MVrv       += frame->MVrv;
    section->MVcv       += frame->MVcv;
    section->mv_in_out_count  += frame->mv_in_out_count;
    section->count      += frame->count;
    section->duration   += frame->duration;
}

static void avg_stats(FIRSTPASS_STATS *section)
{
    if (section->count < 1.0)
        return;

    section->intra_error /= section->count;
    section->coded_error /= section->count;
    section->ssim_weighted_pred_err /= section->count;
    section->pcnt_inter  /= section->count;
    section->pcnt_second_ref /= section->count;
    section->pcnt_neutral /= section->count;
    section->pcnt_motion /= section->count;
    section->MVr        /= section->count;
    section->mvr_abs     /= section->count;
    section->MVc        /= section->count;
    section->mvc_abs     /= section->count;
    section->MVrv       /= section->count;
    section->MVcv       /= section->count;
    section->mv_in_out_count   /= section->count;
    section->duration   /= section->count;
}


static double calculate_modified_err(VP8_COMP *cpi, FIRSTPASS_STATS *this_frame)
{
    double av_err = cpi->twopass.total_stats->ssim_weighted_pred_err;
    double this_err = this_frame->ssim_weighted_pred_err;
    double modified_err;

    
    
    
    

    
    

    




















    if (this_err > av_err)
        modified_err = av_err * pow((this_err / DOUBLE_DIVIDE_CHECK(av_err)), POW1);
    else
        modified_err = av_err * pow((this_err / DOUBLE_DIVIDE_CHECK(av_err)), POW2);

    




    return modified_err;
}

static const double weight_table[256] = {
0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
0.020000, 0.031250, 0.062500, 0.093750, 0.125000, 0.156250, 0.187500, 0.218750,
0.250000, 0.281250, 0.312500, 0.343750, 0.375000, 0.406250, 0.437500, 0.468750,
0.500000, 0.531250, 0.562500, 0.593750, 0.625000, 0.656250, 0.687500, 0.718750,
0.750000, 0.781250, 0.812500, 0.843750, 0.875000, 0.906250, 0.937500, 0.968750,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000
};

static double simple_weight(YV12_BUFFER_CONFIG *source)
{
    int i, j;

    unsigned char *src = source->y_buffer;
    double sum_weights = 0.0;

    
    i = source->y_height;
    do
    {
        j = source->y_width;
        do
        {
            sum_weights += weight_table[ *src];
            src++;
        }while(--j);
        src -= source->y_width;
        src += source->y_stride;
    }while(--i);

    sum_weights /= (source->y_height * source->y_width);

    return sum_weights;
}



static int frame_max_bits(VP8_COMP *cpi)
{
    
    int max_bits;

    
    
    if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
    {
        double buffer_fullness_ratio = (double)cpi->buffer_level / DOUBLE_DIVIDE_CHECK((double)cpi->oxcf.optimal_buffer_level);

        
        max_bits = (int)(cpi->av_per_frame_bandwidth * ((double)cpi->oxcf.two_pass_vbrmax_section / 100.0));

        
        if (buffer_fullness_ratio < 1.0)
        {
            
            int min_max_bits = ((cpi->av_per_frame_bandwidth >> 2) < (max_bits >> 2)) ? cpi->av_per_frame_bandwidth >> 2 : max_bits >> 2;

            max_bits = (int)(max_bits * buffer_fullness_ratio);

            if (max_bits < min_max_bits)
                max_bits = min_max_bits;       
        }
    }
    
    else
    {
        
        max_bits = (int)(((double)cpi->twopass.bits_left / (cpi->twopass.total_stats->count - (double)cpi->common.current_video_frame)) * ((double)cpi->oxcf.two_pass_vbrmax_section / 100.0));
    }

    
    if (max_bits < 0)
        max_bits = 0;

    return max_bits;
}

void vp8_init_first_pass(VP8_COMP *cpi)
{
    zero_stats(cpi->twopass.total_stats);
}

void vp8_end_first_pass(VP8_COMP *cpi)
{
    output_stats(cpi, cpi->output_pkt_list, cpi->twopass.total_stats);
}

static void zz_motion_search( VP8_COMP *cpi, MACROBLOCK * x, YV12_BUFFER_CONFIG * recon_buffer, int * best_motion_err, int recon_yoffset )
{
    MACROBLOCKD * const xd = & x->e_mbd;
    BLOCK *b = &x->block[0];
    BLOCKD *d = &x->e_mbd.block[0];

    unsigned char *src_ptr = (*(b->base_src) + b->src);
    int src_stride = b->src_stride;
    unsigned char *ref_ptr;
    int ref_stride=d->pre_stride;

    
    xd->pre.y_buffer = recon_buffer->y_buffer + recon_yoffset;

    ref_ptr = (unsigned char *)(*(d->base_pre) + d->pre );

    VARIANCE_INVOKE(IF_RTCD(&cpi->rtcd.variance), mse16x16) ( src_ptr, src_stride, ref_ptr, ref_stride, (unsigned int *)(best_motion_err));
}

static void first_pass_motion_search(VP8_COMP *cpi, MACROBLOCK *x,
                                     int_mv *ref_mv, MV *best_mv,
                                     YV12_BUFFER_CONFIG *recon_buffer,
                                     int *best_motion_err, int recon_yoffset )
{
    MACROBLOCKD *const xd = & x->e_mbd;
    BLOCK *b = &x->block[0];
    BLOCKD *d = &x->e_mbd.block[0];
    int num00;

    int_mv tmp_mv;
    int_mv ref_mv_full;

    int tmp_err;
    int step_param = 3;                                       
    int further_steps = (MAX_MVSEARCH_STEPS - 1) - step_param; 
    int n;
    vp8_variance_fn_ptr_t v_fn_ptr = cpi->fn_ptr[BLOCK_16X16];
    int new_mv_mode_penalty = 256;

    
    v_fn_ptr.vf    = VARIANCE_INVOKE(IF_RTCD(&cpi->rtcd.variance), mse16x16);

    
    xd->pre.y_buffer = recon_buffer->y_buffer + recon_yoffset;

    
    tmp_mv.as_int = 0;
    ref_mv_full.as_mv.col = ref_mv->as_mv.col>>3;
    ref_mv_full.as_mv.row = ref_mv->as_mv.row>>3;
    tmp_err = cpi->diamond_search_sad(x, b, d, &ref_mv_full, &tmp_mv, step_param,
                                      x->sadperbit16, &num00, &v_fn_ptr,
                                      x->mvcost, ref_mv);
    if ( tmp_err < INT_MAX-new_mv_mode_penalty )
        tmp_err += new_mv_mode_penalty;

    if (tmp_err < *best_motion_err)
    {
        *best_motion_err = tmp_err;
        best_mv->row = tmp_mv.as_mv.row;
        best_mv->col = tmp_mv.as_mv.col;
    }

    
    n = num00;
    num00 = 0;

    while (n < further_steps)
    {
        n++;

        if (num00)
            num00--;
        else
        {
            tmp_err = cpi->diamond_search_sad(x, b, d, &ref_mv_full, &tmp_mv,
                                              step_param + n, x->sadperbit16,
                                              &num00, &v_fn_ptr, x->mvcost,
                                              ref_mv);
            if ( tmp_err < INT_MAX-new_mv_mode_penalty )
                tmp_err += new_mv_mode_penalty;

            if (tmp_err < *best_motion_err)
            {
                *best_motion_err = tmp_err;
                best_mv->row = tmp_mv.as_mv.row;
                best_mv->col = tmp_mv.as_mv.col;
            }
        }
    }
}

void vp8_first_pass(VP8_COMP *cpi)
{
    int mb_row, mb_col;
    MACROBLOCK *const x = & cpi->mb;
    VP8_COMMON *const cm = & cpi->common;
    MACROBLOCKD *const xd = & x->e_mbd;

    int recon_yoffset, recon_uvoffset;
    YV12_BUFFER_CONFIG *lst_yv12 = &cm->yv12_fb[cm->lst_fb_idx];
    YV12_BUFFER_CONFIG *new_yv12 = &cm->yv12_fb[cm->new_fb_idx];
    YV12_BUFFER_CONFIG *gld_yv12 = &cm->yv12_fb[cm->gld_fb_idx];
    int recon_y_stride = lst_yv12->y_stride;
    int recon_uv_stride = lst_yv12->uv_stride;
    int64_t intra_error = 0;
    int64_t coded_error = 0;

    int sum_mvr = 0, sum_mvc = 0;
    int sum_mvr_abs = 0, sum_mvc_abs = 0;
    int sum_mvrs = 0, sum_mvcs = 0;
    int mvcount = 0;
    int intercount = 0;
    int second_ref_count = 0;
    int intrapenalty = 256;
    int neutral_count = 0;

    int sum_in_vectors = 0;

    int_mv zero_ref_mv;

    zero_ref_mv.as_int = 0;

    vp8_clear_system_state();  

    x->src = * cpi->Source;
    xd->pre = *lst_yv12;
    xd->dst = *new_yv12;

    x->partition_info = x->pi;

    xd->mode_info_context = cm->mi;

    vp8_build_block_offsets(x);

    vp8_setup_block_dptrs(&x->e_mbd);

    vp8_setup_block_ptrs(x);

    
    vp8_setup_intra_recon(new_yv12);
    vp8cx_frame_init_quantizer(cpi);

    
    
    
    {
        int flag[2] = {1, 1};
        vp8_initialize_rd_consts(cpi, vp8_dc_quant(cm->base_qindex, cm->y1dc_delta_q));
        vpx_memcpy(cm->fc.mvc, vp8_default_mv_context, sizeof(vp8_default_mv_context));
        vp8_build_component_cost_table(cpi->mb.mvcost, (const MV_CONTEXT *) cm->fc.mvc, flag);
    }

    
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
        int_mv best_ref_mv;

        best_ref_mv.as_int = 0;

        
        xd->up_available = (mb_row != 0);
        recon_yoffset = (mb_row * recon_y_stride * 16);
        recon_uvoffset = (mb_row * recon_uv_stride * 8);

        
        x->mv_row_min = -((mb_row * 16) + (VP8BORDERINPIXELS - 16));
        x->mv_row_max = ((cm->mb_rows - 1 - mb_row) * 16) + (VP8BORDERINPIXELS - 16);


        
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
            int this_error;
            int gf_motion_error = INT_MAX;
            int use_dc_pred = (mb_col || mb_row) && (!mb_col || !mb_row);

            xd->dst.y_buffer = new_yv12->y_buffer + recon_yoffset;
            xd->dst.u_buffer = new_yv12->u_buffer + recon_uvoffset;
            xd->dst.v_buffer = new_yv12->v_buffer + recon_uvoffset;
            xd->left_available = (mb_col != 0);

            
            RECON_INVOKE(&xd->rtcd->recon, copy16x16)(x->src.y_buffer, x->src.y_stride, x->thismb, 16);

            
            this_error = vp8_encode_intra(cpi, x, use_dc_pred);

            
            
            
            
            this_error += intrapenalty;

            
            intra_error += (int64_t)this_error;

            
            x->mv_col_min = -((mb_col * 16) + (VP8BORDERINPIXELS - 16));
            x->mv_col_max = ((cm->mb_cols - 1 - mb_col) * 16) + (VP8BORDERINPIXELS - 16);

            
            if (cm->current_video_frame > 0)
            {
                BLOCKD *d = &x->e_mbd.block[0];
                MV tmp_mv = {0, 0};
                int tmp_err;
                int motion_error = INT_MAX;

                
                zz_motion_search( cpi, x, lst_yv12, &motion_error, recon_yoffset );
                d->bmi.mv.as_mv.row = 0;
                d->bmi.mv.as_mv.col = 0;

                
                
                first_pass_motion_search(cpi, x, &best_ref_mv,
                                        &d->bmi.mv.as_mv, lst_yv12,
                                        &motion_error, recon_yoffset);

                
                if (best_ref_mv.as_int)
                {
                   tmp_err = INT_MAX;
                   first_pass_motion_search(cpi, x, &zero_ref_mv, &tmp_mv,
                                     lst_yv12, &tmp_err, recon_yoffset);

                   if ( tmp_err < motion_error )
                   {
                        motion_error = tmp_err;
                        d->bmi.mv.as_mv.row = tmp_mv.row;
                        d->bmi.mv.as_mv.col = tmp_mv.col;
                   }
                }

                
                if (cm->current_video_frame > 1)
                {
                    first_pass_motion_search(cpi, x, &zero_ref_mv, &tmp_mv, gld_yv12, &gf_motion_error, recon_yoffset);

                    if ((gf_motion_error < motion_error) && (gf_motion_error < this_error))
                    {
                        second_ref_count++;
                        
                        
                        
                    }
                    







                    
                    xd->pre.y_buffer = lst_yv12->y_buffer + recon_yoffset;
                    xd->pre.u_buffer = lst_yv12->u_buffer + recon_uvoffset;
                    xd->pre.v_buffer = lst_yv12->v_buffer + recon_uvoffset;
                }

                
                best_ref_mv.as_int = 0;

                if (motion_error <= this_error)
                {
                    
                    
                    
                    
                    if( (((this_error-intrapenalty) * 9) <=
                         (motion_error*10)) &&
                        (this_error < (2*intrapenalty)) )
                    {
                        neutral_count++;
                    }

                    d->bmi.mv.as_mv.row <<= 3;
                    d->bmi.mv.as_mv.col <<= 3;
                    this_error = motion_error;
                    vp8_set_mbmode_and_mvs(x, NEWMV, &d->bmi.mv);
                    vp8_encode_inter16x16y(IF_RTCD(&cpi->rtcd), x);
                    sum_mvr += d->bmi.mv.as_mv.row;
                    sum_mvr_abs += abs(d->bmi.mv.as_mv.row);
                    sum_mvc += d->bmi.mv.as_mv.col;
                    sum_mvc_abs += abs(d->bmi.mv.as_mv.col);
                    sum_mvrs += d->bmi.mv.as_mv.row * d->bmi.mv.as_mv.row;
                    sum_mvcs += d->bmi.mv.as_mv.col * d->bmi.mv.as_mv.col;
                    intercount++;

                    best_ref_mv.as_int = d->bmi.mv.as_int;

                    
                    if (d->bmi.mv.as_int)
                    {
                        mvcount++;

                        
                        if (mb_row < cm->mb_rows / 2)
                        {
                            if (d->bmi.mv.as_mv.row > 0)
                                sum_in_vectors--;
                            else if (d->bmi.mv.as_mv.row < 0)
                                sum_in_vectors++;
                        }
                        else if (mb_row > cm->mb_rows / 2)
                        {
                            if (d->bmi.mv.as_mv.row > 0)
                                sum_in_vectors++;
                            else if (d->bmi.mv.as_mv.row < 0)
                                sum_in_vectors--;
                        }

                        
                        if (mb_col < cm->mb_cols / 2)
                        {
                            if (d->bmi.mv.as_mv.col > 0)
                                sum_in_vectors--;
                            else if (d->bmi.mv.as_mv.col < 0)
                                sum_in_vectors++;
                        }
                        else if (mb_col > cm->mb_cols / 2)
                        {
                            if (d->bmi.mv.as_mv.col > 0)
                                sum_in_vectors++;
                            else if (d->bmi.mv.as_mv.col < 0)
                                sum_in_vectors--;
                        }
                    }
                }
            }

            coded_error += (int64_t)this_error;

            
            x->src.y_buffer += 16;
            x->src.u_buffer += 8;
            x->src.v_buffer += 8;

            recon_yoffset += 16;
            recon_uvoffset += 8;
        }

        
        x->src.y_buffer += 16 * x->src.y_stride - 16 * cm->mb_cols;
        x->src.u_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;
        x->src.v_buffer += 8 * x->src.uv_stride - 8 * cm->mb_cols;

        
        vp8_extend_mb_row(new_yv12, xd->dst.y_buffer + 16, xd->dst.u_buffer + 8, xd->dst.v_buffer + 8);
        vp8_clear_system_state();  
    }

    vp8_clear_system_state();  
    {
        double weight = 0.0;

        FIRSTPASS_STATS fps;

        fps.frame      = cm->current_video_frame ;
        fps.intra_error = intra_error >> 8;
        fps.coded_error = coded_error >> 8;
        weight = simple_weight(cpi->Source);


        if (weight < 0.1)
            weight = 0.1;

        fps.ssim_weighted_pred_err = fps.coded_error * weight;

        fps.pcnt_inter  = 0.0;
        fps.pcnt_motion = 0.0;
        fps.MVr        = 0.0;
        fps.mvr_abs     = 0.0;
        fps.MVc        = 0.0;
        fps.mvc_abs     = 0.0;
        fps.MVrv       = 0.0;
        fps.MVcv       = 0.0;
        fps.mv_in_out_count  = 0.0;
        fps.count      = 1.0;

        fps.pcnt_inter   = 1.0 * (double)intercount / cm->MBs;
        fps.pcnt_second_ref = 1.0 * (double)second_ref_count / cm->MBs;
        fps.pcnt_neutral = 1.0 * (double)neutral_count / cm->MBs;

        if (mvcount > 0)
        {
            fps.MVr = (double)sum_mvr / (double)mvcount;
            fps.mvr_abs = (double)sum_mvr_abs / (double)mvcount;
            fps.MVc = (double)sum_mvc / (double)mvcount;
            fps.mvc_abs = (double)sum_mvc_abs / (double)mvcount;
            fps.MVrv = ((double)sum_mvrs - (fps.MVr * fps.MVr / (double)mvcount)) / (double)mvcount;
            fps.MVcv = ((double)sum_mvcs - (fps.MVc * fps.MVc / (double)mvcount)) / (double)mvcount;
            fps.mv_in_out_count = (double)sum_in_vectors / (double)(mvcount * 2);

            fps.pcnt_motion = 1.0 * (double)mvcount / cpi->common.MBs;
        }

        
        
        fps.duration = cpi->source->ts_end
                       - cpi->source->ts_start;

        
        memcpy(cpi->twopass.this_frame_stats,
               &fps,
               sizeof(FIRSTPASS_STATS));
        output_stats(cpi, cpi->output_pkt_list, cpi->twopass.this_frame_stats);
        accumulate_stats(cpi->twopass.total_stats, &fps);
    }

    
    if ((cm->current_video_frame > 0) &&
        (cpi->twopass.this_frame_stats->pcnt_inter > 0.20) &&
        ((cpi->twopass.this_frame_stats->intra_error / cpi->twopass.this_frame_stats->coded_error) > 2.0))
    {
        vp8_yv12_copy_frame_ptr(lst_yv12, gld_yv12);
    }

    
    vp8_swap_yv12_buffer(lst_yv12, new_yv12);
    vp8_yv12_extend_frame_borders(lst_yv12);

    
    if (cm->current_video_frame == 0)
    {
        vp8_yv12_copy_frame_ptr(lst_yv12, gld_yv12);
    }


    
    if (0)
    {
        char filename[512];
        FILE *recon_file;
        sprintf(filename, "enc%04d.yuv", (int) cm->current_video_frame);

        if (cm->current_video_frame == 0)
            recon_file = fopen(filename, "wb");
        else
            recon_file = fopen(filename, "ab");

        if(fwrite(lst_yv12->buffer_alloc, lst_yv12->frame_size, 1, recon_file));
        fclose(recon_file);
    }

    cm->current_video_frame++;

}
extern const int vp8_bits_per_mb[2][QINDEX_RANGE];

#define BASE_ERRPERMB   150
static int estimate_max_q(VP8_COMP *cpi, double section_err, int section_target_bandwitdh)
{
    int Q;
    int num_mbs = cpi->common.MBs;
    int target_norm_bits_per_mb;

    double err_per_mb = section_err / num_mbs;
    double correction_factor;
    double corr_high;
    double speed_correction = 1.0;
    double rolling_ratio;

    double pow_highq = 0.90;
    double pow_lowq = 0.40;

    if (section_target_bandwitdh <= 0)
        return cpi->twopass.maxq_max_limit;          

    target_norm_bits_per_mb = (section_target_bandwitdh < (1 << 20)) ? (512 * section_target_bandwitdh) / num_mbs : 512 * (section_target_bandwitdh / num_mbs);

    
    if ((cpi->rolling_target_bits > 0.0) && (cpi->active_worst_quality < cpi->worst_quality))
    {
        rolling_ratio = (double)cpi->rolling_actual_bits / (double)cpi->rolling_target_bits;

        
        if (rolling_ratio < 0.95)
            
            cpi->twopass.est_max_qcorrection_factor -= 0.005;
        
        else if (rolling_ratio > 1.05)
            cpi->twopass.est_max_qcorrection_factor += 0.005;

        

        cpi->twopass.est_max_qcorrection_factor =
            (cpi->twopass.est_max_qcorrection_factor < 0.1)
                ? 0.1
                : (cpi->twopass.est_max_qcorrection_factor > 10.0)
                    ? 10.0 : cpi->twopass.est_max_qcorrection_factor;
    }

    
    if ((cpi->compressor_speed == 3) || (cpi->compressor_speed == 1))
    {
        if (cpi->oxcf.cpu_used <= 5)
            speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
        else
            speed_correction = 1.25;
    }

    
    corr_high = pow(err_per_mb / BASE_ERRPERMB, pow_highq);
    corr_high = (corr_high < 0.05)
                    ? 0.05 : (corr_high > 5.0) ? 5.0 : corr_high;

    
    
    for (Q = cpi->twopass.maxq_min_limit; Q < cpi->twopass.maxq_max_limit; Q++)
    {
        int bits_per_mb_at_this_q;

        if (Q < 50)
        {
            correction_factor = pow(err_per_mb / BASE_ERRPERMB, (pow_lowq + Q * 0.01));
            correction_factor = (correction_factor < 0.05) ? 0.05 : (correction_factor > 5.0) ? 5.0 : correction_factor;
        }
        else
            correction_factor = corr_high;

        bits_per_mb_at_this_q = (int)(.5 + correction_factor
            * speed_correction * cpi->twopass.est_max_qcorrection_factor
            * cpi->twopass.section_max_qfactor
            * (double)vp8_bits_per_mb[INTER_FRAME][Q] / 1.0);

        if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
            break;
    }

    
    if ( (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY) &&
         (Q < cpi->cq_target_quality) )
         
    {
        Q = cpi->cq_target_quality;
        
    }

    
    
    
    if ( (cpi->ni_frames >
                  ((unsigned int)cpi->twopass.total_stats->count >> 8)) &&
         (cpi->ni_frames > 150) )
    {
        cpi->twopass.maxq_max_limit = ((cpi->ni_av_qi + 32) < cpi->worst_quality)
                                  ? (cpi->ni_av_qi + 32) : cpi->worst_quality;
        cpi->twopass.maxq_min_limit = ((cpi->ni_av_qi - 32) > cpi->best_quality)
                                  ? (cpi->ni_av_qi - 32) : cpi->best_quality;
    }

    return Q;
}
static int estimate_q(VP8_COMP *cpi, double section_err, int section_target_bandwitdh)
{
    int Q;
    int num_mbs = cpi->common.MBs;
    int target_norm_bits_per_mb;

    double err_per_mb = section_err / num_mbs;
    double correction_factor;
    double corr_high;
    double speed_correction = 1.0;
    double pow_highq = 0.90;
    double pow_lowq = 0.40;

    target_norm_bits_per_mb = (section_target_bandwitdh < (1 << 20)) ? (512 * section_target_bandwitdh) / num_mbs : 512 * (section_target_bandwitdh / num_mbs);

    
    if ((cpi->compressor_speed == 3) || (cpi->compressor_speed == 1))
    {
        if (cpi->oxcf.cpu_used <= 5)
            speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
        else
            speed_correction = 1.25;
    }

    
    corr_high = pow(err_per_mb / BASE_ERRPERMB, pow_highq);
    corr_high = (corr_high < 0.05) ? 0.05 : (corr_high > 5.0) ? 5.0 : corr_high;

    
    for (Q = 0; Q < MAXQ; Q++)
    {
        int bits_per_mb_at_this_q;

        if (Q < 50)
        {
            correction_factor = pow(err_per_mb / BASE_ERRPERMB, (pow_lowq + Q * 0.01));
            correction_factor = (correction_factor < 0.05) ? 0.05 : (correction_factor > 5.0) ? 5.0 : correction_factor;
        }
        else
            correction_factor = corr_high;

        bits_per_mb_at_this_q = (int)(.5 + correction_factor * speed_correction * cpi->twopass.est_max_qcorrection_factor * (double)vp8_bits_per_mb[INTER_FRAME][Q] / 1.0);

        if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
            break;
    }

    return Q;
}


static int estimate_kf_group_q(VP8_COMP *cpi, double section_err, int section_target_bandwitdh, double group_iiratio)
{
    int Q;
    int num_mbs = cpi->common.MBs;
    int target_norm_bits_per_mb = (512 * section_target_bandwitdh) / num_mbs;
    int bits_per_mb_at_this_q;

    double err_per_mb = section_err / num_mbs;
    double err_correction_factor;
    double corr_high;
    double speed_correction = 1.0;
    double current_spend_ratio = 1.0;

    double pow_highq = (POW1 < 0.6) ? POW1 + 0.3 : 0.90;
    double pow_lowq = (POW1 < 0.7) ? POW1 + 0.1 : 0.80;

    double iiratio_correction_factor = 1.0;

    double combined_correction_factor;

    
    if (target_norm_bits_per_mb <= 0)
        return MAXQ * 2;

    
    
    if (cpi->long_rolling_target_bits <= 0)
        current_spend_ratio = 10.0;
    else
    {
        current_spend_ratio = (double)cpi->long_rolling_actual_bits / (double)cpi->long_rolling_target_bits;
        current_spend_ratio = (current_spend_ratio > 10.0) ? 10.0 : (current_spend_ratio < 0.1) ? 0.1 : current_spend_ratio;
    }

    
    
    iiratio_correction_factor = 1.0 - ((group_iiratio - 6.0) * 0.1);

    if (iiratio_correction_factor < 0.5)
        iiratio_correction_factor = 0.5;

    
    if ((cpi->compressor_speed == 3) || (cpi->compressor_speed == 1))
    {
        if (cpi->oxcf.cpu_used <= 5)
            speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
        else
            speed_correction = 1.25;
    }

    
    combined_correction_factor = speed_correction * iiratio_correction_factor * current_spend_ratio;

    
    corr_high = pow(err_per_mb / BASE_ERRPERMB, pow_highq);
    corr_high = (corr_high < 0.05) ? 0.05 : (corr_high > 5.0) ? 5.0 : corr_high;

    
    for (Q = 0; Q < MAXQ; Q++)
    {
        
        if (Q < 20)
        {
            err_correction_factor = pow(err_per_mb / BASE_ERRPERMB, (pow_lowq + Q * 0.01));
            err_correction_factor = (err_correction_factor < 0.05) ? 0.05 : (err_correction_factor > 5.0) ? 5.0 : err_correction_factor;
        }
        else
            err_correction_factor = corr_high;

        bits_per_mb_at_this_q = (int)(.5 + err_correction_factor * combined_correction_factor * (double)vp8_bits_per_mb[INTER_FRAME][Q]);

        if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
            break;
    }

    
    while ((bits_per_mb_at_this_q > target_norm_bits_per_mb)  && (Q < (MAXQ * 2)))
    {

        bits_per_mb_at_this_q = (int)(0.96 * bits_per_mb_at_this_q);
        Q++;
    }

    if (0)
    {
        FILE *f = fopen("estkf_q.stt", "a");
        fprintf(f, "%8d %8d %8d %8.2f %8.3f %8.2f %8.3f %8.3f %8.3f %8d\n", cpi->common.current_video_frame, bits_per_mb_at_this_q,
                target_norm_bits_per_mb, err_per_mb, err_correction_factor,
                current_spend_ratio, group_iiratio, iiratio_correction_factor,
                (double)cpi->buffer_level / (double)cpi->oxcf.optimal_buffer_level, Q);
        fclose(f);
    }

    return Q;
}



static int estimate_cq(VP8_COMP *cpi, double section_err, int section_target_bandwitdh)
{
    int Q;
    int num_mbs = cpi->common.MBs;
    int target_norm_bits_per_mb;

    double err_per_mb = section_err / num_mbs;
    double correction_factor;
    double corr_high;
    double speed_correction = 1.0;
    double pow_highq = 0.90;
    double pow_lowq = 0.40;
    double clip_iiratio;
    double clip_iifactor;

    target_norm_bits_per_mb = (section_target_bandwitdh < (1 << 20))
                              ? (512 * section_target_bandwitdh) / num_mbs
                              : 512 * (section_target_bandwitdh / num_mbs);

    
    
    if ((cpi->compressor_speed == 3) || (cpi->compressor_speed == 1))
    {
        if (cpi->oxcf.cpu_used <= 5)
            speed_correction = 1.04 + (cpi->oxcf.cpu_used * 0.04);
        else
            speed_correction = 1.25;
    }
    
    clip_iiratio = cpi->twopass.total_stats->intra_error /
                   DOUBLE_DIVIDE_CHECK(cpi->twopass.total_stats->coded_error);
    clip_iifactor = 1.0 - ((clip_iiratio - 10.0) * 0.025);
    if (clip_iifactor < 0.80)
        clip_iifactor = 0.80;

    
    corr_high = pow(err_per_mb / BASE_ERRPERMB, pow_highq);
    corr_high = (corr_high < 0.05) ? 0.05 : (corr_high > 5.0) ? 5.0 : corr_high;

    
    for (Q = 0; Q < MAXQ; Q++)
    {
        int bits_per_mb_at_this_q;

        if (Q < 50)
        {
            correction_factor =
                pow( err_per_mb / BASE_ERRPERMB, (pow_lowq + Q * 0.01));

            correction_factor = (correction_factor < 0.05) ? 0.05
                                    : (correction_factor > 5.0) ? 5.0
                                        : correction_factor;
        }
        else
            correction_factor = corr_high;

        bits_per_mb_at_this_q =
            (int)( .5 + correction_factor *
                        speed_correction *
                        clip_iifactor *
                        (double)vp8_bits_per_mb[INTER_FRAME][Q] / 1.0);

        if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
            break;
    }

    return cq_level[Q];
}

extern void vp8_new_frame_rate(VP8_COMP *cpi, double framerate);

void vp8_init_second_pass(VP8_COMP *cpi)
{
    FIRSTPASS_STATS this_frame;
    FIRSTPASS_STATS *start_pos;

    double two_pass_min_rate = (double)(cpi->oxcf.target_bandwidth * cpi->oxcf.two_pass_vbrmin_section / 100);

    zero_stats(cpi->twopass.total_stats);

    if (!cpi->twopass.stats_in_end)
        return;

    *cpi->twopass.total_stats = *cpi->twopass.stats_in_end;

    cpi->twopass.total_error_left = cpi->twopass.total_stats->ssim_weighted_pred_err;
    cpi->twopass.total_intra_error_left = cpi->twopass.total_stats->intra_error;
    cpi->twopass.total_coded_error_left = cpi->twopass.total_stats->coded_error;
    cpi->twopass.start_tot_err_left = cpi->twopass.total_error_left;

    
    

    
    
    
    
    
    vp8_new_frame_rate(cpi, 10000000.0 * cpi->twopass.total_stats->count / cpi->twopass.total_stats->duration);

    cpi->output_frame_rate = cpi->oxcf.frame_rate;
    cpi->twopass.bits_left = (int64_t)(cpi->twopass.total_stats->duration * cpi->oxcf.target_bandwidth / 10000000.0) ;
    cpi->twopass.bits_left -= (int64_t)(cpi->twopass.total_stats->duration * two_pass_min_rate / 10000000.0);
    cpi->twopass.clip_bits_total = cpi->twopass.bits_left;

    
    
    
    
    cpi->twopass.kf_intra_err_min = KF_MB_INTRA_MIN * cpi->common.MBs;
    cpi->twopass.gf_intra_err_min = GF_MB_INTRA_MIN * cpi->common.MBs;

    avg_stats(cpi->twopass.total_stats);

    
    {
        double sum_iiratio = 0.0;
        double IIRatio;

        start_pos = cpi->twopass.stats_in;               

        while (input_stats(cpi, &this_frame) != EOF)
        {
            IIRatio = this_frame.intra_error / DOUBLE_DIVIDE_CHECK(this_frame.coded_error);
            IIRatio = (IIRatio < 1.0) ? 1.0 : (IIRatio > 20.0) ? 20.0 : IIRatio;
            sum_iiratio += IIRatio;
        }

        cpi->twopass.avg_iiratio = sum_iiratio / DOUBLE_DIVIDE_CHECK((double)cpi->twopass.total_stats->count);

        
        reset_fpf_position(cpi, start_pos);
    }

    
    
    {
        start_pos = cpi->twopass.stats_in;               

        cpi->twopass.modified_error_total = 0.0;
        cpi->twopass.modified_error_used = 0.0;

        while (input_stats(cpi, &this_frame) != EOF)
        {
            cpi->twopass.modified_error_total += calculate_modified_err(cpi, &this_frame);
        }
        cpi->twopass.modified_error_left = cpi->twopass.modified_error_total;

        reset_fpf_position(cpi, start_pos);            

    }
}

void vp8_end_second_pass(VP8_COMP *cpi)
{
}



static double get_prediction_decay_rate(VP8_COMP *cpi, FIRSTPASS_STATS *next_frame)
{
    double prediction_decay_rate;
    double motion_decay;
    double motion_pct = next_frame->pcnt_motion;

    
    prediction_decay_rate = next_frame->pcnt_inter;

    
    motion_decay = (1.0 - (motion_pct / 20.0));
    if (motion_decay < prediction_decay_rate)
        prediction_decay_rate = motion_decay;

    
    {
        double this_mv_rabs;
        double this_mv_cabs;
        double distance_factor;

        this_mv_rabs = fabs(next_frame->mvr_abs * motion_pct);
        this_mv_cabs = fabs(next_frame->mvc_abs * motion_pct);

        distance_factor = sqrt((this_mv_rabs * this_mv_rabs) +
                               (this_mv_cabs * this_mv_cabs)) / 250.0;
        distance_factor = ((distance_factor > 1.0)
                                ? 0.0 : (1.0 - distance_factor));
        if (distance_factor < prediction_decay_rate)
            prediction_decay_rate = distance_factor;
    }

    return prediction_decay_rate;
}




static int detect_transition_to_still(
    VP8_COMP *cpi,
    int frame_interval,
    int still_interval,
    double loop_decay_rate,
    double decay_accumulator )
{
    BOOL trans_to_still = FALSE;

    
    
    
    if ( (frame_interval > MIN_GF_INTERVAL) &&
         (loop_decay_rate >= 0.999) &&
         (decay_accumulator < 0.9) )
    {
        int j;
        FIRSTPASS_STATS * position = cpi->twopass.stats_in;
        FIRSTPASS_STATS tmp_next_frame;
        double decay_rate;

        
        
        for ( j = 0; j < still_interval; j++ )
        {
            if (EOF == input_stats(cpi, &tmp_next_frame))
                break;

            decay_rate = get_prediction_decay_rate(cpi, &tmp_next_frame);
            if ( decay_rate < 0.999 )
                break;
        }
        
        reset_fpf_position(cpi, position);

        
        if ( j == still_interval )
            trans_to_still = TRUE;
    }

    return trans_to_still;
}




static BOOL detect_flash( VP8_COMP *cpi, int offset )
{
    FIRSTPASS_STATS next_frame;

    BOOL flash_detected = FALSE;

    
    
    if ( read_frame_stats(cpi, &next_frame, offset) != EOF )
    {
        
        
        
        
        
        if ( (next_frame.pcnt_second_ref > next_frame.pcnt_inter) &&
             (next_frame.pcnt_second_ref >= 0.5 ) )
        {
            flash_detected = TRUE;

            








        }
    }

    return flash_detected;
}


static void accumulate_frame_motion_stats(
    VP8_COMP *cpi,
    FIRSTPASS_STATS * this_frame,
    double * this_frame_mv_in_out,
    double * mv_in_out_accumulator,
    double * abs_mv_in_out_accumulator,
    double * mv_ratio_accumulator )
{
    
    double this_frame_mvr_ratio;
    double this_frame_mvc_ratio;
    double motion_pct;

    
    motion_pct = this_frame->pcnt_motion;

    
    *this_frame_mv_in_out = this_frame->mv_in_out_count * motion_pct;
    *mv_in_out_accumulator += this_frame->mv_in_out_count * motion_pct;
    *abs_mv_in_out_accumulator +=
        fabs(this_frame->mv_in_out_count * motion_pct);

    
    
    if (motion_pct > 0.05)
    {
        this_frame_mvr_ratio = fabs(this_frame->mvr_abs) /
                               DOUBLE_DIVIDE_CHECK(fabs(this_frame->MVr));

        this_frame_mvc_ratio = fabs(this_frame->mvc_abs) /
                               DOUBLE_DIVIDE_CHECK(fabs(this_frame->MVc));

         *mv_ratio_accumulator +=
            (this_frame_mvr_ratio < this_frame->mvr_abs)
                ? (this_frame_mvr_ratio * motion_pct)
                : this_frame->mvr_abs * motion_pct;

        *mv_ratio_accumulator +=
            (this_frame_mvc_ratio < this_frame->mvc_abs)
                ? (this_frame_mvc_ratio * motion_pct)
                : this_frame->mvc_abs * motion_pct;

    }
}


static double calc_frame_boost(
    VP8_COMP *cpi,
    FIRSTPASS_STATS * this_frame,
    double this_frame_mv_in_out )
{
    double frame_boost;

    
    if (this_frame->intra_error > cpi->twopass.gf_intra_err_min)
        frame_boost = (IIFACTOR * this_frame->intra_error /
                      DOUBLE_DIVIDE_CHECK(this_frame->coded_error));
    else
        frame_boost = (IIFACTOR * cpi->twopass.gf_intra_err_min /
                      DOUBLE_DIVIDE_CHECK(this_frame->coded_error));

    
    
    
    
    if (this_frame_mv_in_out > 0.0)
        frame_boost += frame_boost * (this_frame_mv_in_out * 2.0);
    
    else
        frame_boost += frame_boost * (this_frame_mv_in_out / 2.0);

    
    if (frame_boost > GF_RMAX)
        frame_boost = GF_RMAX;

    return frame_boost;
}

#if NEW_BOOST
static int calc_arf_boost(
    VP8_COMP *cpi,
    int offset,
    int f_frames,
    int b_frames,
    int *f_boost,
    int *b_boost )
{
    FIRSTPASS_STATS this_frame;

    int i;
    double boost_score = 0.0;
    double fwd_boost_score = 0.0;
    double mv_ratio_accumulator = 0.0;
    double decay_accumulator = 1.0;
    double this_frame_mv_in_out = 0.0;
    double mv_in_out_accumulator = 0.0;
    double abs_mv_in_out_accumulator = 0.0;
    double r;
    BOOL flash_detected = FALSE;

    
    for ( i = 0; i < f_frames; i++ )
    {
        if ( read_frame_stats(cpi, &this_frame, (i+offset)) == EOF )
            break;

        
        accumulate_frame_motion_stats( cpi, &this_frame,
            &this_frame_mv_in_out, &mv_in_out_accumulator,
            &abs_mv_in_out_accumulator, &mv_ratio_accumulator );

        
        r = calc_frame_boost( cpi, &this_frame, this_frame_mv_in_out );

        
        
        flash_detected = detect_flash(cpi, (i+offset)) ||
                         detect_flash(cpi, (i+offset+1));

        
        if ( !flash_detected )
        {
            decay_accumulator =
                decay_accumulator *
                get_prediction_decay_rate(cpi, &this_frame);
            decay_accumulator =
                decay_accumulator < 0.1 ? 0.1 : decay_accumulator;
        }
        boost_score += (decay_accumulator * r);

        
        if  ( (!flash_detected) &&
              ((mv_ratio_accumulator > 100.0) ||
               (abs_mv_in_out_accumulator > 3.0) ||
               (mv_in_out_accumulator < -2.0) ) )
        {
            break;
        }
    }

    *f_boost = (int)(boost_score * 100.0) >> 4;

    
    boost_score = 0.0;
    mv_ratio_accumulator = 0.0;
    decay_accumulator = 1.0;
    this_frame_mv_in_out = 0.0;
    mv_in_out_accumulator = 0.0;
    abs_mv_in_out_accumulator = 0.0;

    
    for ( i = -1; i >= -b_frames; i-- )
    {
        if ( read_frame_stats(cpi, &this_frame, (i+offset)) == EOF )
            break;

        
        accumulate_frame_motion_stats( cpi, &this_frame,
            &this_frame_mv_in_out, &mv_in_out_accumulator,
            &abs_mv_in_out_accumulator, &mv_ratio_accumulator );

        
        r = calc_frame_boost( cpi, &this_frame, this_frame_mv_in_out );

        
        
        flash_detected = detect_flash(cpi, (i+offset)) ||
                         detect_flash(cpi, (i+offset+1));

        
        if ( !flash_detected )
        {
            decay_accumulator =
                decay_accumulator *
                get_prediction_decay_rate(cpi, &this_frame);
            decay_accumulator =
                decay_accumulator < 0.1 ? 0.1 : decay_accumulator;
        }

        boost_score += (decay_accumulator * r);

        
        if  ( (!flash_detected) &&
              ((mv_ratio_accumulator > 100.0) ||
               (abs_mv_in_out_accumulator > 3.0) ||
               (mv_in_out_accumulator < -2.0) ) )
        {
            break;
        }
    }
    *b_boost = (int)(boost_score * 100.0) >> 4;

    return (*f_boost + *b_boost);
}
#endif


static void define_gf_group(VP8_COMP *cpi, FIRSTPASS_STATS *this_frame)
{
    FIRSTPASS_STATS next_frame;
    FIRSTPASS_STATS *start_pos;
    int i;
    double r;
    double boost_score = 0.0;
    double old_boost_score = 0.0;
    double gf_group_err = 0.0;
    double gf_first_frame_err = 0.0;
    double mod_frame_err = 0.0;

    double mv_ratio_accumulator = 0.0;
    double decay_accumulator = 1.0;

    double loop_decay_rate = 1.00;          

    double this_frame_mv_in_out = 0.0;
    double mv_in_out_accumulator = 0.0;
    double abs_mv_in_out_accumulator = 0.0;
    double mod_err_per_mb_accumulator = 0.0;

    int max_bits = frame_max_bits(cpi);     

    unsigned int allow_alt_ref =
                    cpi->oxcf.play_alternate && cpi->oxcf.lag_in_frames;

    int alt_boost = 0;
    int f_boost = 0;
    int b_boost = 0;
    BOOL flash_detected;

    cpi->twopass.gf_group_bits = 0;
    cpi->twopass.gf_decay_rate = 0;

    vp8_clear_system_state();  

    start_pos = cpi->twopass.stats_in;

    vpx_memset(&next_frame, 0, sizeof(next_frame)); 

    
    mod_frame_err = calculate_modified_err(cpi, this_frame);

    
    
    gf_first_frame_err = mod_frame_err;

    
    
    
    if (cpi->common.frame_type == KEY_FRAME)
        gf_group_err -= gf_first_frame_err;

    
    
    
    i = 0;

    while (((i < cpi->twopass.static_scene_max_gf_interval) ||
            ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL)) &&
           (i < cpi->twopass.frames_to_key))
    {
        i++;    

        
        mod_frame_err = calculate_modified_err(cpi, this_frame);

        gf_group_err += mod_frame_err;

        mod_err_per_mb_accumulator +=
            mod_frame_err / DOUBLE_DIVIDE_CHECK((double)cpi->common.MBs);

        if (EOF == input_stats(cpi, &next_frame))
            break;

        
        
        flash_detected = detect_flash(cpi, 0);

        
        accumulate_frame_motion_stats( cpi, &next_frame,
            &this_frame_mv_in_out, &mv_in_out_accumulator,
            &abs_mv_in_out_accumulator, &mv_ratio_accumulator );

        
        r = calc_frame_boost( cpi, &next_frame, this_frame_mv_in_out );

        
        if ( !flash_detected )
        {
            loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);
            decay_accumulator = decay_accumulator * loop_decay_rate;
            decay_accumulator =
                decay_accumulator < 0.1 ? 0.1 : decay_accumulator;
        }
        boost_score += (decay_accumulator * r);

        
        
        if ( detect_transition_to_still( cpi, i, 5,
                                         loop_decay_rate,
                                         decay_accumulator ) )
        {
            allow_alt_ref = FALSE;
            boost_score = old_boost_score;
            break;
        }

        
        if  (
            
            (i >= cpi->max_gf_interval && (decay_accumulator < 0.995)) ||
            (
                
                (i > MIN_GF_INTERVAL) &&
                
                ((cpi->twopass.frames_to_key - i) >= MIN_GF_INTERVAL) &&
                ((boost_score > 20.0) || (next_frame.pcnt_inter < 0.75)) &&
                (!flash_detected) &&
                ((mv_ratio_accumulator > 100.0) ||
                 (abs_mv_in_out_accumulator > 3.0) ||
                 (mv_in_out_accumulator < -2.0) ||
                 ((boost_score - old_boost_score) < 2.0))
            ) )
        {
            boost_score = old_boost_score;
            break;
        }

        vpx_memcpy(this_frame, &next_frame, sizeof(*this_frame));

        old_boost_score = boost_score;
    }

    cpi->twopass.gf_decay_rate =
        (i > 0) ? (int)(100.0 * (1.0 - decay_accumulator)) / i : 0;

    
    if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
    {
        double max_boost;

        
        if (cpi->drop_frames_allowed)
        {
            int df_buffer_level = cpi->oxcf.drop_frames_water_mark *
                                  (cpi->oxcf.optimal_buffer_level / 100);

            if (cpi->buffer_level > df_buffer_level)
                max_boost = ((double)((cpi->buffer_level - df_buffer_level) * 2 / 3) * 16.0) / DOUBLE_DIVIDE_CHECK((double)cpi->av_per_frame_bandwidth);
            else
                max_boost = 0.0;
        }
        else if (cpi->buffer_level > 0)
        {
            max_boost = ((double)(cpi->buffer_level * 2 / 3) * 16.0) / DOUBLE_DIVIDE_CHECK((double)cpi->av_per_frame_bandwidth);
        }
        else
        {
            max_boost = 0.0;
        }

        if (boost_score > max_boost)
            boost_score = max_boost;
    }

    
    if ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL)
    {
        while (i < cpi->twopass.frames_to_key)
        {
            i++;

            if (EOF == input_stats(cpi, this_frame))
                break;

            if (i < cpi->twopass.frames_to_key)
            {
                mod_frame_err = calculate_modified_err(cpi, this_frame);
                gf_group_err += mod_frame_err;
            }
        }
    }

    cpi->gfu_boost = (int)(boost_score * 100.0) >> 4;

#if NEW_BOOST
    
    alt_boost = calc_arf_boost( cpi, 0, (i-1), (i-1), &f_boost, &b_boost );
#endif

    
    if (allow_alt_ref &&
        (i >= MIN_GF_INTERVAL) &&
        
        (i <= (cpi->twopass.frames_to_key - MIN_GF_INTERVAL)) &&
#if NEW_BOOST
        ((next_frame.pcnt_inter > 0.75) ||
         (next_frame.pcnt_second_ref > 0.5)) &&
        ((mv_in_out_accumulator / (double)i > -0.2) ||
         (mv_in_out_accumulator > -2.0)) &&
        (b_boost > 100) &&
        (f_boost > 100) )
#else
        (next_frame.pcnt_inter > 0.75) &&
        ((mv_in_out_accumulator / (double)i > -0.2) ||
         (mv_in_out_accumulator > -2.0)) &&
        (cpi->gfu_boost > 100) &&
        (cpi->twopass.gf_decay_rate <=
            (ARF_DECAY_THRESH + (cpi->gfu_boost / 200))) )
#endif
    {
        int Boost;
        int allocation_chunks;
        int Q = (cpi->oxcf.fixed_q < 0)
                ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;
        int tmp_q;
        int arf_frame_bits = 0;
        int group_bits;

#if NEW_BOOST
        cpi->gfu_boost = alt_boost;
#endif

        
        if ((cpi->twopass.kf_group_bits > 0) &&
            (cpi->twopass.kf_group_error_left > 0))
        {
            group_bits = (int)((double)cpi->twopass.kf_group_bits *
                (gf_group_err / (double)cpi->twopass.kf_group_error_left));
        }
        else
            group_bits = 0;

        
#if NEW_BOOST
        Boost = (alt_boost * GFQ_ADJUSTMENT) / 100;
#else
        Boost = (cpi->gfu_boost * 3 * GFQ_ADJUSTMENT) / (2 * 100);
#endif
        Boost += (i * 50);

        
        if (Boost > ((cpi->baseline_gf_interval + 1) * 200))
            Boost = ((cpi->baseline_gf_interval + 1) * 200);
        else if (Boost < 125)
            Boost = 125;

        allocation_chunks = (i * 100) + Boost;

        
        while (Boost > 1000)
        {
            Boost /= 2;
            allocation_chunks /= 2;
        }

        
        
        arf_frame_bits = (int)((double)Boost * (group_bits /
                               (double)allocation_chunks));

        
        
        tmp_q = estimate_q(cpi, mod_frame_err, (int)arf_frame_bits);

        
        
        if (tmp_q < cpi->worst_quality)
        {
            int half_gf_int;
            int frames_after_arf;
            int frames_bwd = cpi->oxcf.arnr_max_frames - 1;
            int frames_fwd = cpi->oxcf.arnr_max_frames - 1;

            cpi->source_alt_ref_pending = TRUE;

            
            
            
            
            
            

            
            
            

            
            
            
            
            cpi->baseline_gf_interval = i;

            
            
            
            
            
            
            half_gf_int = cpi->baseline_gf_interval >> 1;
            frames_after_arf = cpi->twopass.total_stats->count -
                               this_frame->frame - 1;

            switch (cpi->oxcf.arnr_type)
            {
            case 1: 
                frames_fwd = 0;
                if (frames_bwd > half_gf_int)
                    frames_bwd = half_gf_int;
                break;

            case 2: 
                if (frames_fwd > half_gf_int)
                    frames_fwd = half_gf_int;
                if (frames_fwd > frames_after_arf)
                    frames_fwd = frames_after_arf;
                frames_bwd = 0;
                break;

            case 3: 
            default:
                frames_fwd >>= 1;
                if (frames_fwd > frames_after_arf)
                    frames_fwd = frames_after_arf;
                if (frames_fwd > half_gf_int)
                    frames_fwd = half_gf_int;

                frames_bwd = frames_fwd;

                
                
                if (frames_bwd < half_gf_int)
                    frames_bwd += (cpi->oxcf.arnr_max_frames+1) & 0x1;
                break;
            }

            cpi->active_arnr_frames = frames_bwd + 1 + frames_fwd;
        }
        else
        {
            cpi->source_alt_ref_pending = FALSE;
            cpi->baseline_gf_interval = i;
        }
    }
    else
    {
        cpi->source_alt_ref_pending = FALSE;
        cpi->baseline_gf_interval = i;
    }

    
    
    
    
    
    
    if (cpi->twopass.frames_to_key >= (int)(cpi->twopass.total_stats->count -
                                            cpi->common.current_video_frame))
    {
        cpi->twopass.kf_group_bits =
            (cpi->twopass.bits_left > 0) ? cpi->twopass.bits_left : 0;
    }

    
    if ((cpi->twopass.kf_group_bits > 0) &&
        (cpi->twopass.kf_group_error_left > 0))
    {
        cpi->twopass.gf_group_bits =
            (int)((double)cpi->twopass.kf_group_bits *
                  (gf_group_err / (double)cpi->twopass.kf_group_error_left));
    }
    else
        cpi->twopass.gf_group_bits = 0;

    cpi->twopass.gf_group_bits =
        (cpi->twopass.gf_group_bits < 0)
            ? 0
            : (cpi->twopass.gf_group_bits > cpi->twopass.kf_group_bits)
                ? cpi->twopass.kf_group_bits : cpi->twopass.gf_group_bits;

    
    
    if (cpi->twopass.gf_group_bits > max_bits * cpi->baseline_gf_interval)
        cpi->twopass.gf_group_bits = max_bits * cpi->baseline_gf_interval;

    
    reset_fpf_position(cpi, start_pos);

    
    cpi->twopass.modified_error_used += gf_group_err;

    
    for (i = 0; i <= (cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME); i++) {
        int Boost;
        int allocation_chunks;
        int Q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME] : cpi->oxcf.fixed_q;
        int gf_bits;

        
        if (cpi->source_alt_ref_pending && i == 0)
        {
#if NEW_BOOST
            Boost = (alt_boost * GFQ_ADJUSTMENT) / 100;
#else
            Boost = (cpi->gfu_boost * 3 * GFQ_ADJUSTMENT) / (2 * 100);
#endif
            Boost += (cpi->baseline_gf_interval * 50);

            
            if (Boost > ((cpi->baseline_gf_interval + 1) * 200))
                Boost = ((cpi->baseline_gf_interval + 1) * 200);
            else if (Boost < 125)
                Boost = 125;

            allocation_chunks =
                ((cpi->baseline_gf_interval + 1) * 100) + Boost;
        }
        
        else
        {
            
            Boost = (cpi->gfu_boost * GFQ_ADJUSTMENT) / 100;

            
            if (Boost > (cpi->baseline_gf_interval * 150))
                Boost = (cpi->baseline_gf_interval * 150);
            else if (Boost < 125)
                Boost = 125;

            allocation_chunks =
                (cpi->baseline_gf_interval * 100) + (Boost - 100);
        }

        
        while (Boost > 1000)
        {
            Boost /= 2;
            allocation_chunks /= 2;
        }

        
        
        gf_bits = (int)((double)Boost *
                        (cpi->twopass.gf_group_bits /
                         (double)allocation_chunks));

        
        
        
        if (mod_frame_err < gf_group_err / (double)cpi->baseline_gf_interval)
        {
            double  alt_gf_grp_bits;
            int     alt_gf_bits;

            alt_gf_grp_bits =
                (double)cpi->twopass.kf_group_bits  *
                (mod_frame_err * (double)cpi->baseline_gf_interval) /
                DOUBLE_DIVIDE_CHECK((double)cpi->twopass.kf_group_error_left);

            alt_gf_bits = (int)((double)Boost * (alt_gf_grp_bits /
                                                 (double)allocation_chunks));

            if (gf_bits > alt_gf_bits)
            {
                gf_bits = alt_gf_bits;
            }
        }
        
        
        
        else
        {
            int alt_gf_bits =
                (int)((double)cpi->twopass.kf_group_bits *
                      mod_frame_err /
                      DOUBLE_DIVIDE_CHECK((double)cpi->twopass.kf_group_error_left));

            if (alt_gf_bits > gf_bits)
            {
                gf_bits = alt_gf_bits;
            }
        }

        
        if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
        {
            if (cpi->twopass.gf_bits > (cpi->buffer_level >> 1))
                cpi->twopass.gf_bits = cpi->buffer_level >> 1;
        }

        
        if (gf_bits < 0)
            gf_bits = 0;

        gf_bits += cpi->min_frame_bandwidth;                     

        if (i == 0)
        {
            cpi->twopass.gf_bits = gf_bits;
        }
        if (i == 1 || (!cpi->source_alt_ref_pending && (cpi->common.frame_type != KEY_FRAME)))
        {
            cpi->per_frame_bandwidth = gf_bits;                 
        }
    }

    {
        
        cpi->twopass.kf_group_error_left -= gf_group_err;
        cpi->twopass.kf_group_bits -= cpi->twopass.gf_group_bits;

        if (cpi->twopass.kf_group_bits < 0)
            cpi->twopass.kf_group_bits = 0;

        
        
        if (!cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME)
            cpi->twopass.gf_group_error_left = gf_group_err - gf_first_frame_err;
        else
            cpi->twopass.gf_group_error_left = gf_group_err;

        cpi->twopass.gf_group_bits -= cpi->twopass.gf_bits - cpi->min_frame_bandwidth;

        if (cpi->twopass.gf_group_bits < 0)
            cpi->twopass.gf_group_bits = 0;

        {
#if NEW_BOOST
            int boost = (cpi->source_alt_ref_pending)
                        ? b_boost : cpi->gfu_boost;
#else
            int boost = cpi->gfu_boost;
#endif
            
            if ((boost > 150) && (cpi->baseline_gf_interval > 5))
            {
                int pct_extra = (boost - 100) / 50;
                pct_extra = (pct_extra > 10) ? 10 : pct_extra;

                cpi->twopass.mid_gf_extra_bits =
                    (cpi->twopass.gf_group_bits * pct_extra) / 100;
                cpi->twopass.gf_group_bits -= cpi->twopass.mid_gf_extra_bits;
            }
            else
                cpi->twopass.mid_gf_extra_bits = 0;
        }
    }

    
    if (cpi->common.frame_type != KEY_FRAME)
    {
        FIRSTPASS_STATS sectionstats;
        double Ratio;

        zero_stats(&sectionstats);
        reset_fpf_position(cpi, start_pos);

        for (i = 0 ; i < cpi->baseline_gf_interval ; i++)
        {
            input_stats(cpi, &next_frame);
            accumulate_stats(&sectionstats, &next_frame);
        }

        avg_stats(&sectionstats);

        cpi->twopass.section_intra_rating =
            sectionstats.intra_error /
            DOUBLE_DIVIDE_CHECK(sectionstats.coded_error);

        Ratio = sectionstats.intra_error / DOUBLE_DIVIDE_CHECK(sectionstats.coded_error);
        
        
        cpi->twopass.section_max_qfactor = 1.0 - ((Ratio - 10.0) * 0.025);

        if (cpi->twopass.section_max_qfactor < 0.80)
            cpi->twopass.section_max_qfactor = 0.80;

        
        
        

        reset_fpf_position(cpi, start_pos);
    }
}


static void assign_std_frame_bits(VP8_COMP *cpi, FIRSTPASS_STATS *this_frame)
{
    int    target_frame_size;                                                             

    double modified_err;
    double err_fraction;                                                                 

    int max_bits = frame_max_bits(cpi);    

    
    modified_err = calculate_modified_err(cpi, this_frame);

    if (cpi->twopass.gf_group_error_left > 0)
        err_fraction = modified_err / cpi->twopass.gf_group_error_left;                              
    else
        err_fraction = 0.0;

    target_frame_size = (int)((double)cpi->twopass.gf_group_bits * err_fraction);                    

    
    if (target_frame_size < 0)
        target_frame_size = 0;
    else
    {
        if (target_frame_size > max_bits)
            target_frame_size = max_bits;

        if (target_frame_size > cpi->twopass.gf_group_bits)
            target_frame_size = cpi->twopass.gf_group_bits;
    }

    cpi->twopass.gf_group_error_left -= modified_err;                                               
    cpi->twopass.gf_group_bits -= target_frame_size;                                                

    if (cpi->twopass.gf_group_bits < 0)
        cpi->twopass.gf_group_bits = 0;

    target_frame_size += cpi->min_frame_bandwidth;                                          

    
    if (cpi->common.frames_since_golden == cpi->baseline_gf_interval / 2)
        target_frame_size += cpi->twopass.mid_gf_extra_bits;

    cpi->per_frame_bandwidth = target_frame_size;                                           
}

void vp8_second_pass(VP8_COMP *cpi)
{
    int tmp_q;
    int frames_left = (int)(cpi->twopass.total_stats->count - cpi->common.current_video_frame);

    FIRSTPASS_STATS this_frame;
    FIRSTPASS_STATS this_frame_copy;

    double this_frame_error;
    double this_frame_intra_error;
    double this_frame_coded_error;

    FIRSTPASS_STATS *start_pos;

    if (!cpi->twopass.stats_in)
    {
        return ;
    }

    vp8_clear_system_state();

    if (EOF == input_stats(cpi, &this_frame))
        return;

    this_frame_error = this_frame.ssim_weighted_pred_err;
    this_frame_intra_error = this_frame.intra_error;
    this_frame_coded_error = this_frame.coded_error;

    start_pos = cpi->twopass.stats_in;

    
    if (cpi->twopass.frames_to_key == 0)
    {
        
        vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
        find_next_key_frame(cpi, &this_frame_copy);

        
        
        
        if (cpi->oxcf.error_resilient_mode)
        {
            cpi->twopass.gf_group_bits = cpi->twopass.kf_group_bits;
            cpi->twopass.gf_group_error_left = cpi->twopass.kf_group_error_left;
            cpi->baseline_gf_interval = cpi->twopass.frames_to_key;
            cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
            cpi->source_alt_ref_pending = FALSE;
        }

    }

    
    if (cpi->frames_till_gf_update_due == 0)
    {
        
        vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
        define_gf_group(cpi, &this_frame_copy);

        
        
        
        if (cpi->source_alt_ref_pending && (cpi->common.frame_type != KEY_FRAME))
        {
            
            int bak = cpi->per_frame_bandwidth;
            vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
            assign_std_frame_bits(cpi, &this_frame_copy);
            cpi->per_frame_bandwidth = bak;
        }
    }

    
    else
    {
        
        
        
        if (cpi->oxcf.error_resilient_mode)
        {
            cpi->frames_till_gf_update_due = cpi->twopass.frames_to_key;

            if (cpi->common.frame_type != KEY_FRAME)
            {
                
                vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
                assign_std_frame_bits(cpi, &this_frame_copy);
            }
        }
        else
        {
            
            vpx_memcpy(&this_frame_copy, &this_frame, sizeof(this_frame));
            assign_std_frame_bits(cpi, &this_frame_copy);
        }
    }

    
    cpi->twopass.this_iiratio = this_frame_intra_error /
                        DOUBLE_DIVIDE_CHECK(this_frame_coded_error);
    {
        FIRSTPASS_STATS next_frame;
        if ( lookup_next_frame_stats(cpi, &next_frame) != EOF )
        {
            cpi->twopass.next_iiratio = next_frame.intra_error /
                                DOUBLE_DIVIDE_CHECK(next_frame.coded_error);
        }
    }

    
    cpi->target_bandwidth = cpi->per_frame_bandwidth * cpi->output_frame_rate;
    if (cpi->target_bandwidth < 0)
        cpi->target_bandwidth = 0;

    if (cpi->common.current_video_frame == 0)
    {
        cpi->twopass.est_max_qcorrection_factor = 1.0;

        
        
        if ( cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY )
        {
            int est_cq;

            est_cq =
                estimate_cq( cpi,
                             (cpi->twopass.total_coded_error_left / frames_left),
                             (int)(cpi->twopass.bits_left / frames_left));

            cpi->cq_target_quality = cpi->oxcf.cq_level;
            if ( est_cq > cpi->cq_target_quality )
                cpi->cq_target_quality = est_cq;
        }

        
        cpi->twopass.maxq_max_limit = cpi->worst_quality;
        cpi->twopass.maxq_min_limit = cpi->best_quality;
        tmp_q = estimate_max_q( cpi,
                                (cpi->twopass.total_coded_error_left / frames_left),
                                (int)(cpi->twopass.bits_left / frames_left));

        
        
        
        
        
        cpi->twopass.maxq_max_limit = ((tmp_q + 32) < cpi->worst_quality)
                                  ? (tmp_q + 32) : cpi->worst_quality;
        cpi->twopass.maxq_min_limit = ((tmp_q - 32) > cpi->best_quality)
                                  ? (tmp_q - 32) : cpi->best_quality;

        cpi->active_worst_quality         = tmp_q;
        cpi->ni_av_qi                     = tmp_q;
    }

    
    
    
    
    else if ( (cpi->common.current_video_frame <
                  (((unsigned int)cpi->twopass.total_stats->count * 255)>>8)) &&
              ((cpi->common.current_video_frame + cpi->baseline_gf_interval) <
                  (unsigned int)cpi->twopass.total_stats->count) )
    {
        if (frames_left < 1)
            frames_left = 1;

        tmp_q = estimate_max_q(cpi, (cpi->twopass.total_coded_error_left / frames_left), (int)(cpi->twopass.bits_left / frames_left));

        
        if (tmp_q > cpi->active_worst_quality)
            cpi->active_worst_quality ++;
        else if (tmp_q < cpi->active_worst_quality)
            cpi->active_worst_quality --;

        cpi->active_worst_quality = ((cpi->active_worst_quality * 3) + tmp_q + 2) / 4;
    }

    cpi->twopass.frames_to_key --;
    cpi->twopass.total_error_left      -= this_frame_error;
    cpi->twopass.total_intra_error_left -= this_frame_intra_error;
    cpi->twopass.total_coded_error_left -= this_frame_coded_error;
}


static BOOL test_candidate_kf(VP8_COMP *cpi,  FIRSTPASS_STATS *last_frame, FIRSTPASS_STATS *this_frame, FIRSTPASS_STATS *next_frame)
{
    BOOL is_viable_kf = FALSE;

    
    
    if ((this_frame->pcnt_second_ref < 0.10) &&
        (next_frame->pcnt_second_ref < 0.10) &&
        ((this_frame->pcnt_inter < 0.05) ||
         (
             ((this_frame->pcnt_inter - this_frame->pcnt_neutral) < .25) &&
             ((this_frame->intra_error / DOUBLE_DIVIDE_CHECK(this_frame->coded_error)) < 2.5) &&
             ((fabs(last_frame->coded_error - this_frame->coded_error) / DOUBLE_DIVIDE_CHECK(this_frame->coded_error) > .40) ||
              (fabs(last_frame->intra_error - this_frame->intra_error) / DOUBLE_DIVIDE_CHECK(this_frame->intra_error) > .40) ||
              ((next_frame->intra_error / DOUBLE_DIVIDE_CHECK(next_frame->coded_error)) > 3.5)
             )
         )
        )
       )
    {
        int i;
        FIRSTPASS_STATS *start_pos;

        FIRSTPASS_STATS local_next_frame;

        double boost_score = 0.0;
        double old_boost_score = 0.0;
        double decay_accumulator = 1.0;
        double next_iiratio;

        vpx_memcpy(&local_next_frame, next_frame, sizeof(*next_frame));

        
        start_pos = cpi->twopass.stats_in;

        
        for (i = 0 ; i < 16; i++)
        {
            next_iiratio = (IIKFACTOR1 * local_next_frame.intra_error / DOUBLE_DIVIDE_CHECK(local_next_frame.coded_error)) ;

            if (next_iiratio > RMAX)
                next_iiratio = RMAX;

            
            if (local_next_frame.pcnt_inter > 0.85)
                decay_accumulator = decay_accumulator * local_next_frame.pcnt_inter;
            else
                decay_accumulator = decay_accumulator * ((0.85 + local_next_frame.pcnt_inter) / 2.0);

            

            
            boost_score += (decay_accumulator * next_iiratio);

            
            if ((local_next_frame.pcnt_inter < 0.05) ||
                (next_iiratio < 1.5) ||
                (((local_next_frame.pcnt_inter -
                   local_next_frame.pcnt_neutral) < 0.20) &&
                 (next_iiratio < 3.0)) ||
                ((boost_score - old_boost_score) < 0.5) ||
                (local_next_frame.intra_error < 200)
               )
            {
                break;
            }

            old_boost_score = boost_score;

            
            if (EOF == input_stats(cpi, &local_next_frame))
                break;
        }

        
        if (boost_score > 5.0 && (i > 3))
            is_viable_kf = TRUE;
        else
        {
            
            reset_fpf_position(cpi, start_pos);

            is_viable_kf = FALSE;
        }
    }

    return is_viable_kf;
}
static void find_next_key_frame(VP8_COMP *cpi, FIRSTPASS_STATS *this_frame)
{
    int i,j;
    FIRSTPASS_STATS last_frame;
    FIRSTPASS_STATS first_frame;
    FIRSTPASS_STATS next_frame;
    FIRSTPASS_STATS *start_position;

    double decay_accumulator = 1.0;
    double boost_score = 0;
    double old_boost_score = 0.0;
    double loop_decay_rate;

    double kf_mod_err = 0.0;
    double kf_group_err = 0.0;
    double kf_group_intra_err = 0.0;
    double kf_group_coded_err = 0.0;
    double recent_loop_decay[8] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};

    vpx_memset(&next_frame, 0, sizeof(next_frame)); 

    vp8_clear_system_state();  
    start_position = cpi->twopass.stats_in;

    cpi->common.frame_type = KEY_FRAME;

    
    cpi->this_key_frame_forced = cpi->next_key_frame_forced;

    
    cpi->source_alt_ref_active = FALSE;

    
    cpi->frames_till_gf_update_due = 0;

    cpi->twopass.frames_to_key = 1;

    
    vpx_memcpy(&first_frame, this_frame, sizeof(*this_frame));

    cpi->twopass.kf_group_bits = 0;        
    cpi->twopass.kf_group_error_left = 0;  

    kf_mod_err = calculate_modified_err(cpi, this_frame);

    
    i = 0;
    while (cpi->twopass.stats_in < cpi->twopass.stats_in_end)
    {
        
        kf_group_err += calculate_modified_err(cpi, this_frame);

        
        
        kf_group_intra_err += this_frame->intra_error;
        kf_group_coded_err += this_frame->coded_error;

        
        vpx_memcpy(&last_frame, this_frame, sizeof(*this_frame));
        input_stats(cpi, this_frame);

        
        if (cpi->oxcf.auto_key
            && lookup_next_frame_stats(cpi, &next_frame) != EOF)
        {
            
            if (test_candidate_kf(cpi, &last_frame, this_frame, &next_frame))
                break;

            
            loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);

            
            
            
            recent_loop_decay[i%8] = loop_decay_rate;
            decay_accumulator = 1.0;
            for (j = 0; j < 8; j++)
            {
                decay_accumulator = decay_accumulator * recent_loop_decay[j];
            }

            
            
            if ( detect_transition_to_still( cpi, i,
                                             (cpi->key_frame_frequency-i),
                                             loop_decay_rate,
                                             decay_accumulator ) )
            {
                break;
            }


            
            cpi->twopass.frames_to_key ++;

            
            
            if (cpi->twopass.frames_to_key >= 2 *(int)cpi->key_frame_frequency)
                break;
        } else
            cpi->twopass.frames_to_key ++;

        i++;
    }

    
    
    
    
    if (cpi->oxcf.auto_key
        && cpi->twopass.frames_to_key > (int)cpi->key_frame_frequency )
    {
        FIRSTPASS_STATS *current_pos = cpi->twopass.stats_in;
        FIRSTPASS_STATS tmp_frame;

        cpi->twopass.frames_to_key /= 2;

        
        vpx_memcpy(&tmp_frame, &first_frame, sizeof(first_frame));

        
        reset_fpf_position(cpi, start_position);

        kf_group_err = 0;
        kf_group_intra_err = 0;
        kf_group_coded_err = 0;

        
        for( i = 0; i < cpi->twopass.frames_to_key; i++ )
        {
            
            kf_group_err += calculate_modified_err(cpi, &tmp_frame);
            kf_group_intra_err += tmp_frame.intra_error;
            kf_group_coded_err += tmp_frame.coded_error;

            
            input_stats(cpi, &tmp_frame);
        }

        
        reset_fpf_position(cpi, current_pos);

        cpi->next_key_frame_forced = TRUE;
    }
    else
        cpi->next_key_frame_forced = FALSE;

    
    if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
    {
        
        kf_group_err += calculate_modified_err(cpi, this_frame);

        
        
        kf_group_intra_err += this_frame->intra_error;
        kf_group_coded_err += this_frame->coded_error;
    }

    
    if ((cpi->twopass.bits_left > 0) && (cpi->twopass.modified_error_left > 0.0))
    {
        
        int max_bits = frame_max_bits(cpi);

        
        int64_t max_grp_bits;

        
        
        cpi->twopass.kf_group_bits = (int64_t)( cpi->twopass.bits_left *
                                          ( kf_group_err /
                                            cpi->twopass.modified_error_left ));

        
        max_grp_bits = (int64_t)max_bits * (int64_t)cpi->twopass.frames_to_key;
        if (cpi->twopass.kf_group_bits > max_grp_bits)
            cpi->twopass.kf_group_bits = max_grp_bits;

        
        if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
        {
            int opt_buffer_lvl = cpi->oxcf.optimal_buffer_level;
            int buffer_lvl = cpi->buffer_level;

            
            
            if (buffer_lvl >= opt_buffer_lvl)
            {
                int high_water_mark = (opt_buffer_lvl +
                                       cpi->oxcf.maximum_buffer_size) >> 1;

                int64_t av_group_bits;

                
                av_group_bits = (int64_t)cpi->av_per_frame_bandwidth *
                                (int64_t)cpi->twopass.frames_to_key;

                
                if (cpi->buffer_level >= high_water_mark)
                {
                    int64_t min_group_bits;

                    min_group_bits = av_group_bits +
                                     (int64_t)(buffer_lvl -
                                                 high_water_mark);

                    if (cpi->twopass.kf_group_bits < min_group_bits)
                        cpi->twopass.kf_group_bits = min_group_bits;
                }
                
                else if (cpi->twopass.kf_group_bits < av_group_bits)
                {
                    int64_t bits_below_av = av_group_bits -
                                              cpi->twopass.kf_group_bits;

                    cpi->twopass.kf_group_bits +=
                       (int64_t)((double)bits_below_av *
                                   (double)(buffer_lvl - opt_buffer_lvl) /
                                   (double)(high_water_mark - opt_buffer_lvl));
                }
            }
        }
    }
    else
        cpi->twopass.kf_group_bits = 0;

    
    reset_fpf_position(cpi, start_position);

    
    decay_accumulator = 1.0;
    boost_score = 0.0;
    loop_decay_rate = 1.00;       

    for (i = 0 ; i < cpi->twopass.frames_to_key ; i++)
    {
        double r;

        if (EOF == input_stats(cpi, &next_frame))
            break;

        if (next_frame.intra_error > cpi->twopass.kf_intra_err_min)
            r = (IIKFACTOR2 * next_frame.intra_error /
                     DOUBLE_DIVIDE_CHECK(next_frame.coded_error));
        else
            r = (IIKFACTOR2 * cpi->twopass.kf_intra_err_min /
                     DOUBLE_DIVIDE_CHECK(next_frame.coded_error));

        if (r > RMAX)
            r = RMAX;

        
        loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);

        decay_accumulator = decay_accumulator * loop_decay_rate;
        decay_accumulator = decay_accumulator < 0.1 ? 0.1 : decay_accumulator;

        boost_score += (decay_accumulator * r);

        if ((i > MIN_GF_INTERVAL) &&
            ((boost_score - old_boost_score) < 1.0))
        {
            break;
        }

        old_boost_score = boost_score;
    }

    if (1)
    {
        FIRSTPASS_STATS sectionstats;
        double Ratio;

        zero_stats(&sectionstats);
        reset_fpf_position(cpi, start_position);

        for (i = 0 ; i < cpi->twopass.frames_to_key ; i++)
        {
            input_stats(cpi, &next_frame);
            accumulate_stats(&sectionstats, &next_frame);
        }

        avg_stats(&sectionstats);

        cpi->twopass.section_intra_rating =
            sectionstats.intra_error
            / DOUBLE_DIVIDE_CHECK(sectionstats.coded_error);

        Ratio = sectionstats.intra_error / DOUBLE_DIVIDE_CHECK(sectionstats.coded_error);
        
        
        cpi->twopass.section_max_qfactor = 1.0 - ((Ratio - 10.0) * 0.025);

        if (cpi->twopass.section_max_qfactor < 0.80)
            cpi->twopass.section_max_qfactor = 0.80;

        
        
        
    }

    
    if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
    {
        double max_boost;

        if (cpi->drop_frames_allowed)
        {
            int df_buffer_level = cpi->oxcf.drop_frames_water_mark * (cpi->oxcf.optimal_buffer_level / 100);

            if (cpi->buffer_level > df_buffer_level)
                max_boost = ((double)((cpi->buffer_level - df_buffer_level) * 2 / 3) * 16.0) / DOUBLE_DIVIDE_CHECK((double)cpi->av_per_frame_bandwidth);
            else
                max_boost = 0.0;
        }
        else if (cpi->buffer_level > 0)
        {
            max_boost = ((double)(cpi->buffer_level * 2 / 3) * 16.0) / DOUBLE_DIVIDE_CHECK((double)cpi->av_per_frame_bandwidth);
        }
        else
        {
            max_boost = 0.0;
        }

        if (boost_score > max_boost)
            boost_score = max_boost;
    }

    
    reset_fpf_position(cpi, start_position);

    
    if (1)
    {
        int kf_boost = boost_score;
        int allocation_chunks;
        int Counter = cpi->twopass.frames_to_key;
        int alt_kf_bits;
        YV12_BUFFER_CONFIG *lst_yv12 = &cpi->common.yv12_fb[cpi->common.lst_fb_idx];
        
#if 0

        while ((kf_boost < 48) && (Counter > 0))
        {
            Counter -= 2;
            kf_boost ++;
        }

#endif

        if (kf_boost < 48)
        {
            kf_boost += ((Counter + 1) >> 1);

            if (kf_boost > 48) kf_boost = 48;
        }

        
        if ((lst_yv12->y_width * lst_yv12->y_height) > (320 * 240))
            kf_boost += 2 * (lst_yv12->y_width * lst_yv12->y_height) / (320 * 240);
        else if ((lst_yv12->y_width * lst_yv12->y_height) < (320 * 240))
            kf_boost -= 4 * (320 * 240) / (lst_yv12->y_width * lst_yv12->y_height);

        kf_boost = (int)((double)kf_boost * 100.0) >> 4;                          

        
        

        if (kf_boost < 250)                                                      
            kf_boost = 250;

        
        
        
        

        allocation_chunks = ((cpi->twopass.frames_to_key - 1) * 100) + kf_boost;           

        
        while (kf_boost > 1000)
        {
            kf_boost /= 2;
            allocation_chunks /= 2;
        }

        cpi->twopass.kf_group_bits = (cpi->twopass.kf_group_bits < 0) ? 0 : cpi->twopass.kf_group_bits;

        
        cpi->twopass.kf_bits  = (int)((double)kf_boost * ((double)cpi->twopass.kf_group_bits / (double)allocation_chunks));

        
        if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
        {
            if (cpi->twopass.kf_bits > ((3 * cpi->buffer_level) >> 2))
                cpi->twopass.kf_bits = (3 * cpi->buffer_level) >> 2;
        }

        
        
        
        
        if (kf_mod_err < kf_group_err / cpi->twopass.frames_to_key)
        {
            double  alt_kf_grp_bits =
                        ((double)cpi->twopass.bits_left *
                         (kf_mod_err * (double)cpi->twopass.frames_to_key) /
                         DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left));

            alt_kf_bits = (int)((double)kf_boost *
                                (alt_kf_grp_bits / (double)allocation_chunks));

            if (cpi->twopass.kf_bits > alt_kf_bits)
            {
                cpi->twopass.kf_bits = alt_kf_bits;
            }
        }
        
        
        
        else
        {
            alt_kf_bits =
                (int)((double)cpi->twopass.bits_left *
                      (kf_mod_err /
                       DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left)));

            if (alt_kf_bits > cpi->twopass.kf_bits)
            {
                cpi->twopass.kf_bits = alt_kf_bits;
            }
        }

        cpi->twopass.kf_group_bits -= cpi->twopass.kf_bits;
        cpi->twopass.kf_bits += cpi->min_frame_bandwidth;                                          

        cpi->per_frame_bandwidth = cpi->twopass.kf_bits;                                           
        cpi->target_bandwidth = cpi->twopass.kf_bits * cpi->output_frame_rate;                      
    }

    
    cpi->twopass.kf_group_error_left = (int)(kf_group_err - kf_mod_err);

    
    
    cpi->twopass.modified_error_left -= kf_group_err;

    if (cpi->oxcf.allow_spatial_resampling)
    {
        int resample_trigger = FALSE;
        int last_kf_resampled = FALSE;
        int kf_q;
        int scale_val = 0;
        int hr, hs, vr, vs;
        int new_width = cpi->oxcf.Width;
        int new_height = cpi->oxcf.Height;

        int projected_buffer_level = cpi->buffer_level;
        int tmp_q;

        double projected_bits_perframe;
        double group_iiratio = (kf_group_intra_err - first_frame.intra_error) / (kf_group_coded_err - first_frame.coded_error);
        double err_per_frame = kf_group_err / cpi->twopass.frames_to_key;
        double bits_per_frame;
        double av_bits_per_frame;
        double effective_size_ratio;

        if ((cpi->common.Width != cpi->oxcf.Width) || (cpi->common.Height != cpi->oxcf.Height))
            last_kf_resampled = TRUE;

        
        cpi->common.horiz_scale = NORMAL;
        cpi->common.vert_scale = NORMAL;

        
        
        av_bits_per_frame = cpi->oxcf.target_bandwidth / DOUBLE_DIVIDE_CHECK((double)cpi->oxcf.frame_rate);
        
        

        
        if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
        {
            bits_per_frame = av_bits_per_frame;
        }

        
        
        else
        {
            bits_per_frame = cpi->twopass.kf_group_bits / cpi->twopass.frames_to_key;     

            if (bits_per_frame < av_bits_per_frame)                      
                bits_per_frame = av_bits_per_frame;
        }

        
        if (bits_per_frame < (cpi->oxcf.target_bandwidth * cpi->oxcf.two_pass_vbrmin_section / 100))
            bits_per_frame = (cpi->oxcf.target_bandwidth * cpi->oxcf.two_pass_vbrmin_section / 100);

        
        kf_q = estimate_kf_group_q(cpi, err_per_frame, bits_per_frame, group_iiratio);

        
        projected_bits_perframe = bits_per_frame;
        tmp_q = kf_q;

        while (tmp_q > cpi->worst_quality)
        {
            projected_bits_perframe *= 1.04;
            tmp_q--;
        }

        
        projected_buffer_level = cpi->buffer_level - (int)((projected_bits_perframe - av_bits_per_frame) * cpi->twopass.frames_to_key);

        if (0)
        {
            FILE *f = fopen("Subsamle.stt", "a");
            fprintf(f, " %8d %8d %8d %8d %12.0f %8d %8d %8d\n",  cpi->common.current_video_frame, kf_q, cpi->common.horiz_scale, cpi->common.vert_scale,  kf_group_err / cpi->twopass.frames_to_key, (int)(cpi->twopass.kf_group_bits / cpi->twopass.frames_to_key), new_height, new_width);
            fclose(f);
        }

        
        if (cpi->oxcf.end_usage == USAGE_STREAM_FROM_SERVER)
        {
            
            
            if ((projected_buffer_level < (cpi->oxcf.resample_down_water_mark * cpi->oxcf.optimal_buffer_level / 100)) ||
                (last_kf_resampled && (projected_buffer_level < (cpi->oxcf.resample_up_water_mark * cpi->oxcf.optimal_buffer_level / 100))))
                
                
                resample_trigger = TRUE;
            else
                resample_trigger = FALSE;
        }
        else
        {
            int64_t clip_bits = (int64_t)(cpi->twopass.total_stats->count * cpi->oxcf.target_bandwidth / DOUBLE_DIVIDE_CHECK((double)cpi->oxcf.frame_rate));
            int64_t over_spend = cpi->oxcf.starting_buffer_level - cpi->buffer_level;

            if ((last_kf_resampled && (kf_q > cpi->worst_quality)) ||                                               
                ((kf_q > cpi->worst_quality) &&                                                                  
                 (over_spend > clip_bits / 20)))                                                               
                resample_trigger = TRUE;
            else
                resample_trigger = FALSE;

        }

        if (resample_trigger)
        {
            while ((kf_q >= cpi->worst_quality) && (scale_val < 6))
            {
                scale_val ++;

                cpi->common.vert_scale   = vscale_lookup[scale_val];
                cpi->common.horiz_scale  = hscale_lookup[scale_val];

                Scale2Ratio(cpi->common.horiz_scale, &hr, &hs);
                Scale2Ratio(cpi->common.vert_scale, &vr, &vs);

                new_width = ((hs - 1) + (cpi->oxcf.Width * hr)) / hs;
                new_height = ((vs - 1) + (cpi->oxcf.Height * vr)) / vs;

                
                
                effective_size_ratio = (double)(new_width * new_height) / (double)(cpi->oxcf.Width * cpi->oxcf.Height);
                effective_size_ratio = (1.0 + (3.0 * effective_size_ratio)) / 4.0;

                
                kf_q = estimate_kf_group_q(cpi, err_per_frame * effective_size_ratio, bits_per_frame, group_iiratio);

                if (0)
                {
                    FILE *f = fopen("Subsamle.stt", "a");
                    fprintf(f, "******** %8d %8d %8d %12.0f %8d %8d %8d\n",  kf_q, cpi->common.horiz_scale, cpi->common.vert_scale,  kf_group_err / cpi->twopass.frames_to_key, (int)(cpi->twopass.kf_group_bits / cpi->twopass.frames_to_key), new_height, new_width);
                    fclose(f);
                }
            }
        }

        if ((cpi->common.Width != new_width) || (cpi->common.Height != new_height))
        {
            cpi->common.Width = new_width;
            cpi->common.Height = new_height;
            vp8_alloc_compressor_data(cpi);
        }
    }
}
