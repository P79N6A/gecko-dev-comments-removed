









#include "denoising.h"

#include "vp8/common/reconinter.h"
#include "vpx/vpx_integer.h"
#include "vpx_mem/vpx_mem.h"
#include "vp8_rtcd.h"

static const unsigned int NOISE_MOTION_THRESHOLD = 25 * 25;



static const unsigned int SSE_DIFF_THRESHOLD = 16 * 16 * 20;
static const unsigned int SSE_THRESHOLD = 16 * 16 * 40;






























int vp8_denoiser_filter_c(YV12_BUFFER_CONFIG *mc_running_avg,
                          YV12_BUFFER_CONFIG *running_avg, MACROBLOCK *signal,
                          unsigned int motion_magnitude, int y_offset,
                          int uv_offset)
{
    unsigned char *sig = signal->thismb;
    int sig_stride = 16;
    unsigned char *mc_running_avg_y = mc_running_avg->y_buffer + y_offset;
    int mc_avg_y_stride = mc_running_avg->y_stride;
    unsigned char *running_avg_y = running_avg->y_buffer + y_offset;
    int avg_y_stride = running_avg->y_stride;
    int r, c, i;
    int sum_diff = 0;
    int adj_val[3] = {3, 4, 6};

    

    if (motion_magnitude <= MOTION_MAGNITUDE_THRESHOLD)
    {
        for (i = 0; i < 3; i++)
            adj_val[i] += 1;
    }

    for (r = 0; r < 16; ++r)
    {
        for (c = 0; c < 16; ++c)
        {
            int diff = 0;
            int adjustment = 0;
            int absdiff = 0;

            diff = mc_running_avg_y[c] - sig[c];
            absdiff = abs(diff);

            
            if (absdiff <= 3)
            {
                running_avg_y[c] = mc_running_avg_y[c];
                sum_diff += diff;
            }
            else
            {
                if (absdiff >= 4 && absdiff <= 7)
                    adjustment = adj_val[0];
                else if (absdiff >= 8 && absdiff <= 15)
                    adjustment = adj_val[1];
                else
                    adjustment = adj_val[2];

                if (diff > 0)
                {
                    if ((sig[c] + adjustment) > 255)
                        running_avg_y[c] = 255;
                    else
                        running_avg_y[c] = sig[c] + adjustment;

                    sum_diff += adjustment;
                }
                else
                {
                    if ((sig[c] - adjustment) < 0)
                        running_avg_y[c] = 0;
                    else
                        running_avg_y[c] = sig[c] - adjustment;

                    sum_diff -= adjustment;
                }
            }
        }

        
        sig += sig_stride;
        mc_running_avg_y += mc_avg_y_stride;
        running_avg_y += avg_y_stride;
    }

    if (abs(sum_diff) > SUM_DIFF_THRESHOLD)
        return COPY_BLOCK;

    vp8_copy_mem16x16(running_avg->y_buffer + y_offset, avg_y_stride,
                      signal->thismb, sig_stride);
    return FILTER_BLOCK;
}

int vp8_denoiser_allocate(VP8_DENOISER *denoiser, int width, int height)
{
    int i;
    assert(denoiser);

    for (i = 0; i < MAX_REF_FRAMES; i++)
    {
        denoiser->yv12_running_avg[i].flags = 0;

        if (vp8_yv12_alloc_frame_buffer(&(denoiser->yv12_running_avg[i]), width,
                                        height, VP8BORDERINPIXELS)
            < 0)
        {
            vp8_denoiser_free(denoiser);
            return 1;
        }
        vpx_memset(denoiser->yv12_running_avg[i].buffer_alloc, 0,
                   denoiser->yv12_running_avg[i].frame_size);

    }
    denoiser->yv12_mc_running_avg.flags = 0;

    if (vp8_yv12_alloc_frame_buffer(&(denoiser->yv12_mc_running_avg), width,
                                   height, VP8BORDERINPIXELS) < 0)
    {
        vp8_denoiser_free(denoiser);
        return 1;
    }

    vpx_memset(denoiser->yv12_mc_running_avg.buffer_alloc, 0,
               denoiser->yv12_mc_running_avg.frame_size);
    return 0;
}

void vp8_denoiser_free(VP8_DENOISER *denoiser)
{
    int i;
    assert(denoiser);

    for (i = 0; i < MAX_REF_FRAMES ; i++)
    {
        vp8_yv12_de_alloc_frame_buffer(&denoiser->yv12_running_avg[i]);
    }
    vp8_yv12_de_alloc_frame_buffer(&denoiser->yv12_mc_running_avg);
}


void vp8_denoiser_denoise_mb(VP8_DENOISER *denoiser,
                             MACROBLOCK *x,
                             unsigned int best_sse,
                             unsigned int zero_mv_sse,
                             int recon_yoffset,
                             int recon_uvoffset)
{
    int mv_row;
    int mv_col;
    unsigned int motion_magnitude2;

    MV_REFERENCE_FRAME frame = x->best_reference_frame;
    MV_REFERENCE_FRAME zero_frame = x->best_zeromv_reference_frame;

    enum vp8_denoiser_decision decision = FILTER_BLOCK;

    if (zero_frame)
    {
        YV12_BUFFER_CONFIG *src = &denoiser->yv12_running_avg[frame];
        YV12_BUFFER_CONFIG *dst = &denoiser->yv12_mc_running_avg;
        YV12_BUFFER_CONFIG saved_pre,saved_dst;
        MB_MODE_INFO saved_mbmi;
        MACROBLOCKD *filter_xd = &x->e_mbd;
        MB_MODE_INFO *mbmi = &filter_xd->mode_info_context->mbmi;
        int sse_diff = zero_mv_sse - best_sse;

        saved_mbmi = *mbmi;

        
        mbmi->ref_frame = x->best_reference_frame;
        mbmi->mode = x->best_sse_inter_mode;
        mbmi->mv = x->best_sse_mv;
        mbmi->need_to_clamp_mvs = x->need_to_clamp_best_mvs;
        mv_col = x->best_sse_mv.as_mv.col;
        mv_row = x->best_sse_mv.as_mv.row;

        if (frame == INTRA_FRAME ||
            ((unsigned int)(mv_row *mv_row + mv_col *mv_col)
              <= NOISE_MOTION_THRESHOLD &&
             sse_diff < (int)SSE_DIFF_THRESHOLD))
        {
            







            mbmi->ref_frame =
                    x->best_zeromv_reference_frame;

            src = &denoiser->yv12_running_avg[zero_frame];

            mbmi->mode = ZEROMV;
            mbmi->mv.as_int = 0;
            x->best_sse_inter_mode = ZEROMV;
            x->best_sse_mv.as_int = 0;
            best_sse = zero_mv_sse;
        }

        saved_pre = filter_xd->pre;
        saved_dst = filter_xd->dst;

        
        filter_xd->pre.y_buffer = src->y_buffer + recon_yoffset;
        filter_xd->pre.u_buffer = src->u_buffer + recon_uvoffset;
        filter_xd->pre.v_buffer = src->v_buffer + recon_uvoffset;
        
        filter_xd->dst.y_buffer = dst->y_buffer + recon_yoffset;
        filter_xd->dst.u_buffer = dst->u_buffer + recon_uvoffset;
        filter_xd->dst.v_buffer = dst->v_buffer + recon_uvoffset;

        if (!x->skip)
        {
            vp8_build_inter_predictors_mb(filter_xd);
        }
        else
        {
            vp8_build_inter16x16_predictors_mb(filter_xd,
                                               filter_xd->dst.y_buffer,
                                               filter_xd->dst.u_buffer,
                                               filter_xd->dst.v_buffer,
                                               filter_xd->dst.y_stride,
                                               filter_xd->dst.uv_stride);
        }
        filter_xd->pre = saved_pre;
        filter_xd->dst = saved_dst;
        *mbmi = saved_mbmi;

    }

    mv_row = x->best_sse_mv.as_mv.row;
    mv_col = x->best_sse_mv.as_mv.col;
    motion_magnitude2 = mv_row * mv_row + mv_col * mv_col;
    if (best_sse > SSE_THRESHOLD || motion_magnitude2
           > 8 * NOISE_MOTION_THRESHOLD)
    {
        decision = COPY_BLOCK;
    }

    if (decision == FILTER_BLOCK)
    {
        
        decision = vp8_denoiser_filter(&denoiser->yv12_mc_running_avg,
                                       &denoiser->yv12_running_avg[INTRA_FRAME],
                                       x,
                                       motion_magnitude2,
                                       recon_yoffset, recon_uvoffset);
    }
    if (decision == COPY_BLOCK)
    {
        


        vp8_copy_mem16x16(
                x->thismb, 16,
                denoiser->yv12_running_avg[INTRA_FRAME].y_buffer + recon_yoffset,
                denoiser->yv12_running_avg[INTRA_FRAME].y_stride);
    }
}
