










#include "onyxd_int.h"
#include "header.h"
#include "reconintra.h"
#include "reconintra4x4.h"
#include "recon.h"
#include "reconinter.h"
#include "dequantize.h"
#include "detokenize.h"
#include "invtrans.h"
#include "alloccommon.h"
#include "entropymode.h"
#include "quant_common.h"
#include "vpx_scale/vpxscale.h"
#include "vpx_scale/yv12extend.h"
#include "setupintrarecon.h"

#include "decodemv.h"
#include "extend.h"
#include "vpx_mem/vpx_mem.h"
#include "idct.h"
#include "dequantize.h"
#include "predictdc.h"
#include "threading.h"
#include "decoderthreading.h"
#include "dboolhuff.h"

#include <assert.h>
#include <stdio.h>

void vp8cx_init_de_quantizer(VP8D_COMP *pbi)
{
    int i;
    int Q;
    VP8_COMMON *const pc = & pbi->common;

    for (Q = 0; Q < QINDEX_RANGE; Q++)
    {
        pc->Y1dequant[Q][0] = (short)vp8_dc_quant(Q, pc->y1dc_delta_q);
        pc->Y2dequant[Q][0] = (short)vp8_dc2quant(Q, pc->y2dc_delta_q);
        pc->UVdequant[Q][0] = (short)vp8_dc_uv_quant(Q, pc->uvdc_delta_q);

        
        for (i = 1; i < 16; i++)
        {
            int rc = vp8_default_zig_zag1d[i];

            pc->Y1dequant[Q][rc] = (short)vp8_ac_yquant(Q);
            pc->Y2dequant[Q][rc] = (short)vp8_ac2quant(Q, pc->y2ac_delta_q);
            pc->UVdequant[Q][rc] = (short)vp8_ac_uv_quant(Q, pc->uvac_delta_q);
        }
    }
}

void mb_init_dequantizer(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int i;
    int QIndex;
    MB_MODE_INFO *mbmi = &xd->mode_info_context->mbmi;
    VP8_COMMON *const pc = & pbi->common;

    
    if (xd->segmentation_enabled)
    {
        
        if (xd->mb_segement_abs_delta == SEGMENT_ABSDATA)
            QIndex = xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];

        
        else
        {
            QIndex = pc->base_qindex + xd->segment_feature_data[MB_LVL_ALT_Q][mbmi->segment_id];
            QIndex = (QIndex >= 0) ? ((QIndex <= MAXQ) ? QIndex : MAXQ) : 0;    
        }
    }
    else
        QIndex = pc->base_qindex;

    
    for (i = 0; i < 16; i++)
    {
        xd->block[i].dequant = pc->Y1dequant[QIndex];
    }

    for (i = 16; i < 24; i++)
    {
        xd->block[i].dequant = pc->UVdequant[QIndex];
    }

    xd->block[24].dequant = pc->Y2dequant[QIndex];

}

#if CONFIG_RUNTIME_CPU_DETECT
#define RTCD_VTABLE(x) (&(pbi)->common.rtcd.x)
#else
#define RTCD_VTABLE(x) NULL
#endif




static void skip_recon_mb(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    if (xd->frame_type == KEY_FRAME  ||  xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {

        vp8_build_intra_predictors_mbuv_s(xd);
        RECON_INVOKE(&pbi->common.rtcd.recon,
                     build_intra_predictors_mby_s)(xd);
    }
    else
    {
        vp8_build_inter_predictors_mb_s(xd);
    }
}

static void clamp_mv_to_umv_border(MV *mv, const MACROBLOCKD *xd)
{
    








    if (mv->col < (xd->mb_to_left_edge - (19 << 3)))
        mv->col = xd->mb_to_left_edge - (16 << 3);
    else if (mv->col > xd->mb_to_right_edge + (18 << 3))
        mv->col = xd->mb_to_right_edge + (16 << 3);

    if (mv->row < (xd->mb_to_top_edge - (19 << 3)))
        mv->row = xd->mb_to_top_edge - (16 << 3);
    else if (mv->row > xd->mb_to_bottom_edge + (18 << 3))
        mv->row = xd->mb_to_bottom_edge + (16 << 3);
}


static void clamp_uvmv_to_umv_border(MV *mv, const MACROBLOCKD *xd)
{
    mv->col = (2*mv->col < (xd->mb_to_left_edge - (19 << 3))) ? (xd->mb_to_left_edge - (16 << 3)) >> 1 : mv->col;
    mv->col = (2*mv->col > xd->mb_to_right_edge + (18 << 3)) ? (xd->mb_to_right_edge + (16 << 3)) >> 1 : mv->col;

    mv->row = (2*mv->row < (xd->mb_to_top_edge - (19 << 3))) ? (xd->mb_to_top_edge - (16 << 3)) >> 1 : mv->row;
    mv->row = (2*mv->row > xd->mb_to_bottom_edge + (18 << 3)) ? (xd->mb_to_bottom_edge + (16 << 3)) >> 1 : mv->row;
}

void clamp_mvs(MACROBLOCKD *xd)
{
    if (xd->mode_info_context->mbmi.mode == SPLITMV)
    {
        int i;

        for (i=0; i<16; i++)
            clamp_mv_to_umv_border(&xd->block[i].bmi.mv.as_mv, xd);
        for (i=16; i<24; i++)
            clamp_uvmv_to_umv_border(&xd->block[i].bmi.mv.as_mv, xd);
    }
    else
    {
        clamp_mv_to_umv_border(&xd->mode_info_context->mbmi.mv.as_mv, xd);
        clamp_uvmv_to_umv_border(&xd->block[16].bmi.mv.as_mv, xd);
    }

}

void vp8_decode_macroblock(VP8D_COMP *pbi, MACROBLOCKD *xd)
{
    int eobtotal = 0;
    int i, do_clamp = xd->mode_info_context->mbmi.need_to_clamp_mvs;

    if (xd->mode_info_context->mbmi.mb_skip_coeff)
    {
        vp8_reset_mb_tokens_context(xd);
    }
    else
    {
        eobtotal = vp8_decode_mb_tokens(pbi, xd);
    }

    
    if (do_clamp)
    {
        clamp_mvs(xd);
    }

    xd->mode_info_context->mbmi.dc_diff = 1;

    if (xd->mode_info_context->mbmi.mode != B_PRED && xd->mode_info_context->mbmi.mode != SPLITMV && eobtotal == 0)
    {
        xd->mode_info_context->mbmi.dc_diff = 0;
        skip_recon_mb(pbi, xd);
        return;
    }

    if (xd->segmentation_enabled)
        mb_init_dequantizer(pbi, xd);

    
    if (xd->frame_type == KEY_FRAME  ||  xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME)
    {
        vp8_build_intra_predictors_mbuv(xd);

        if (xd->mode_info_context->mbmi.mode != B_PRED)
        {
            RECON_INVOKE(&pbi->common.rtcd.recon,
                         build_intra_predictors_mby)(xd);
        } else {
            vp8_intra_prediction_down_copy(xd);
        }
    }
    else
    {
        vp8_build_inter_predictors_mb(xd);
    }

    
    if (xd->mode_info_context->mbmi.mode != B_PRED && xd->mode_info_context->mbmi.mode != SPLITMV)
    {
        BLOCKD *b = &xd->block[24];
        DEQUANT_INVOKE(&pbi->dequant, block)(b);

        
        if (xd->eobs[24] > 1)
        {
            IDCT_INVOKE(RTCD_VTABLE(idct), iwalsh16)(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
            ((int *)b->qcoeff)[1] = 0;
            ((int *)b->qcoeff)[2] = 0;
            ((int *)b->qcoeff)[3] = 0;
            ((int *)b->qcoeff)[4] = 0;
            ((int *)b->qcoeff)[5] = 0;
            ((int *)b->qcoeff)[6] = 0;
            ((int *)b->qcoeff)[7] = 0;
        }
        else
        {
            IDCT_INVOKE(RTCD_VTABLE(idct), iwalsh1)(&b->dqcoeff[0], b->diff);
            ((int *)b->qcoeff)[0] = 0;
        }

        DEQUANT_INVOKE (&pbi->dequant, dc_idct_add_y_block)
                        (xd->qcoeff, xd->block[0].dequant,
                         xd->predictor, xd->dst.y_buffer,
                         xd->dst.y_stride, xd->eobs, xd->block[24].diff);
    }
    else if ((xd->frame_type == KEY_FRAME  ||  xd->mode_info_context->mbmi.ref_frame == INTRA_FRAME) && xd->mode_info_context->mbmi.mode == B_PRED)
    {
        for (i = 0; i < 16; i++)
        {

            BLOCKD *b = &xd->block[i];
            vp8_predict_intra4x4(b, b->bmi.mode, b->predictor);

            if (xd->eobs[i] > 1)
            {
                DEQUANT_INVOKE(&pbi->dequant, idct_add)
                    (b->qcoeff, b->dequant,  b->predictor,
                    *(b->base_dst) + b->dst, 16, b->dst_stride);
            }
            else
            {
                IDCT_INVOKE(RTCD_VTABLE(idct), idct1_scalar_add)
                    (b->qcoeff[0] * b->dequant[0], b->predictor,
                    *(b->base_dst) + b->dst, 16, b->dst_stride);
                ((int *)b->qcoeff)[0] = 0;
            }
        }

    }
    else
    {
        DEQUANT_INVOKE (&pbi->dequant, idct_add_y_block)
                        (xd->qcoeff, xd->block[0].dequant,
                         xd->predictor, xd->dst.y_buffer,
                         xd->dst.y_stride, xd->eobs);
    }

    DEQUANT_INVOKE (&pbi->dequant, idct_add_uv_block)
                    (xd->qcoeff+16*16, xd->block[16].dequant,
                     xd->predictor+16*16, xd->dst.u_buffer, xd->dst.v_buffer,
                     xd->dst.uv_stride, xd->eobs+16);
}


static int get_delta_q(vp8_reader *bc, int prev, int *q_update)
{
    int ret_val = 0;

    if (vp8_read_bit(bc))
    {
        ret_val = vp8_read_literal(bc, 4);

        if (vp8_read_bit(bc))
            ret_val = -ret_val;
    }

    
    if (ret_val != prev)
        *q_update = 1;

    return ret_val;
}

#ifdef PACKET_TESTING
#include <stdio.h>
FILE *vpxlog = 0;
#endif



void vp8_decode_mb_row(VP8D_COMP *pbi,
                       VP8_COMMON *pc,
                       int mb_row,
                       MACROBLOCKD *xd)
{

    int i;
    int recon_yoffset, recon_uvoffset;
    int mb_col;
    int ref_fb_idx = pc->lst_fb_idx;
    int dst_fb_idx = pc->new_fb_idx;
    int recon_y_stride = pc->yv12_fb[ref_fb_idx].y_stride;
    int recon_uv_stride = pc->yv12_fb[ref_fb_idx].uv_stride;

    vpx_memset(&pc->left_context, 0, sizeof(pc->left_context));
    recon_yoffset = mb_row * recon_y_stride * 16;
    recon_uvoffset = mb_row * recon_uv_stride * 8;
    

    xd->above_context = pc->above_context;
    xd->up_available = (mb_row != 0);

    xd->mb_to_top_edge = -((mb_row * 16)) << 3;
    xd->mb_to_bottom_edge = ((pc->mb_rows - 1 - mb_row) * 16) << 3;

    for (mb_col = 0; mb_col < pc->mb_cols; mb_col++)
    {

        if (xd->mode_info_context->mbmi.mode == SPLITMV || xd->mode_info_context->mbmi.mode == B_PRED)
        {
            for (i = 0; i < 16; i++)
            {
                BLOCKD *d = &xd->block[i];
                vpx_memcpy(&d->bmi, &xd->mode_info_context->bmi[i], sizeof(B_MODE_INFO));
            }
        }

        


        xd->mb_to_left_edge = -((mb_col * 16) << 3);
        xd->mb_to_right_edge = ((pc->mb_cols - 1 - mb_col) * 16) << 3;

        xd->dst.y_buffer = pc->yv12_fb[dst_fb_idx].y_buffer + recon_yoffset;
        xd->dst.u_buffer = pc->yv12_fb[dst_fb_idx].u_buffer + recon_uvoffset;
        xd->dst.v_buffer = pc->yv12_fb[dst_fb_idx].v_buffer + recon_uvoffset;

        xd->left_available = (mb_col != 0);

        
        if (xd->mode_info_context->mbmi.ref_frame == LAST_FRAME)
            ref_fb_idx = pc->lst_fb_idx;
        else if (xd->mode_info_context->mbmi.ref_frame == GOLDEN_FRAME)
            ref_fb_idx = pc->gld_fb_idx;
        else
            ref_fb_idx = pc->alt_fb_idx;

        xd->pre.y_buffer = pc->yv12_fb[ref_fb_idx].y_buffer + recon_yoffset;
        xd->pre.u_buffer = pc->yv12_fb[ref_fb_idx].u_buffer + recon_uvoffset;
        xd->pre.v_buffer = pc->yv12_fb[ref_fb_idx].v_buffer + recon_uvoffset;

        vp8_build_uvmvs(xd, pc->full_pixel);

        





        vp8_decode_macroblock(pbi, xd);


        recon_yoffset += 16;
        recon_uvoffset += 8;

        ++xd->mode_info_context;  

        xd->above_context++;

    }

    
    vp8_extend_mb_row(
        &pc->yv12_fb[dst_fb_idx],
        xd->dst.y_buffer + 16, xd->dst.u_buffer + 8, xd->dst.v_buffer + 8
    );

    ++xd->mode_info_context;      
}


static unsigned int read_partition_size(const unsigned char *cx_size)
{
    const unsigned int size =
        cx_size[0] + (cx_size[1] << 8) + (cx_size[2] << 16);
    return size;
}


static void setup_token_decoder(VP8D_COMP *pbi,
                                const unsigned char *cx_data)
{
    int num_part;
    int i;
    VP8_COMMON          *pc = &pbi->common;
    const unsigned char *user_data_end = pbi->Source + pbi->source_sz;
    vp8_reader          *bool_decoder;
    const unsigned char *partition;

    
    pc->multi_token_partition = (TOKEN_PARTITION)vp8_read_literal(&pbi->bc, 2);
    num_part = 1 << pc->multi_token_partition;

    
    partition = cx_data;
    bool_decoder = &pbi->bc2;

    if (num_part > 1)
    {
        CHECK_MEM_ERROR(pbi->mbc, vpx_malloc(num_part * sizeof(vp8_reader)));
        bool_decoder = pbi->mbc;
        partition += 3 * (num_part - 1);
    }

    for (i = 0; i < num_part; i++)
    {
        const unsigned char *partition_size_ptr = cx_data + i * 3;
        ptrdiff_t            partition_size;

        


        if (i < num_part - 1)
        {
            partition_size = read_partition_size(partition_size_ptr);
        }
        else
        {
            partition_size = user_data_end - partition;
        }

        if (user_data_end - partition < partition_size)
            vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                               "Truncated packet or corrupt partition "
                               "%d length", i + 1);

        if (vp8dx_start_decode(bool_decoder, IF_RTCD(&pbi->dboolhuff),
                               partition, partition_size))
            vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                               "Failed to allocate bool decoder %d", i + 1);

        
        partition += partition_size;
        bool_decoder++;
    }

    
    if (pbi->decoding_thread_count > num_part - 1)
        pbi->decoding_thread_count = num_part - 1;
}


static void stop_token_decoder(VP8D_COMP *pbi)
{
    int i;
    VP8_COMMON *pc = &pbi->common;

    if (pc->multi_token_partition != ONE_PARTITION)
        vpx_free(pbi->mbc);
}

static void init_frame(VP8D_COMP *pbi)
{
    VP8_COMMON *const pc = & pbi->common;
    MACROBLOCKD *const xd  = & pbi->mb;

    if (pc->frame_type == KEY_FRAME)
    {
        
        vpx_memcpy(pc->fc.mvc, vp8_default_mv_context, sizeof(vp8_default_mv_context));

        vp8_init_mbmode_probs(pc);

        vp8_default_coef_probs(pc);
        vp8_kf_default_bmode_probs(pc->kf_bmode_prob);

        
        vpx_memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));
        xd->mb_segement_abs_delta = SEGMENT_DELTADATA;

        
        vpx_memset(xd->ref_lf_deltas, 0, sizeof(xd->ref_lf_deltas));
        vpx_memset(xd->mode_lf_deltas, 0, sizeof(xd->mode_lf_deltas));

        
        pc->refresh_golden_frame = 1;
        pc->refresh_alt_ref_frame = 1;
        pc->copy_buffer_to_gf = 0;
        pc->copy_buffer_to_arf = 0;

        


        pc->ref_frame_sign_bias[GOLDEN_FRAME] = 0;
        pc->ref_frame_sign_bias[ALTREF_FRAME] = 0;
    }
    else
    {
        if (!pc->use_bilinear_mc_filter)
            pc->mcomp_filter_type = SIXTAP;
        else
            pc->mcomp_filter_type = BILINEAR;

        
        if (pc->mcomp_filter_type == SIXTAP)
        {
            xd->subpixel_predict      = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap4x4);
            xd->subpixel_predict8x4   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x4);
            xd->subpixel_predict8x8   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap8x8);
            xd->subpixel_predict16x16 = SUBPIX_INVOKE(RTCD_VTABLE(subpix), sixtap16x16);
        }
        else
        {
            xd->subpixel_predict      = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear4x4);
            xd->subpixel_predict8x4   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x4);
            xd->subpixel_predict8x8   = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear8x8);
            xd->subpixel_predict16x16 = SUBPIX_INVOKE(RTCD_VTABLE(subpix), bilinear16x16);
        }
    }

    xd->left_context = &pc->left_context;
    xd->mode_info_context = pc->mi;
    xd->frame_type = pc->frame_type;
    xd->mode_info_context->mbmi.mode = DC_PRED;
    xd->mode_info_stride = pc->mode_info_stride;
}

int vp8_decode_frame(VP8D_COMP *pbi)
{
    vp8_reader *const bc = & pbi->bc;
    VP8_COMMON *const pc = & pbi->common;
    MACROBLOCKD *const xd  = & pbi->mb;
    const unsigned char *data = (const unsigned char *)pbi->Source;
    const unsigned char *const data_end = data + pbi->source_sz;
    ptrdiff_t first_partition_length_in_bytes;

    int mb_row;
    int i, j, k, l;
    const int *const mb_feature_data_bits = vp8_mb_feature_data_bits;

    if (data_end - data < 3)
        vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                           "Truncated packet");
    pc->frame_type = (FRAME_TYPE)(data[0] & 1);
    pc->version = (data[0] >> 1) & 7;
    pc->show_frame = (data[0] >> 4) & 1;
    first_partition_length_in_bytes =
        (data[0] | (data[1] << 8) | (data[2] << 16)) >> 5;
    data += 3;

    if (data_end - data < first_partition_length_in_bytes)
        vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                           "Truncated packet or corrupt partition 0 length");
    vp8_setup_version(pc);

    if (pc->frame_type == KEY_FRAME)
    {
        const int Width = pc->Width;
        const int Height = pc->Height;

        
        if (data[0] != 0x9d || data[1] != 0x01 || data[2] != 0x2a)
            vpx_internal_error(&pc->error, VPX_CODEC_UNSUP_BITSTREAM,
                               "Invalid frame sync code");

        pc->Width = (data[3] | (data[4] << 8)) & 0x3fff;
        pc->horiz_scale = data[4] >> 6;
        pc->Height = (data[5] | (data[6] << 8)) & 0x3fff;
        pc->vert_scale = data[6] >> 6;
        data += 7;

        if (Width != pc->Width  ||  Height != pc->Height)
        {
            int prev_mb_rows = pc->mb_rows;

            if (pc->Width <= 0)
            {
                pc->Width = Width;
                vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                                   "Invalid frame width");
            }

            if (pc->Height <= 0)
            {
                pc->Height = Height;
                vpx_internal_error(&pc->error, VPX_CODEC_CORRUPT_FRAME,
                                   "Invalid frame height");
            }

            if (vp8_alloc_frame_buffers(pc, pc->Width, pc->Height))
                vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                                   "Failed to allocate frame buffers");

#if CONFIG_MULTITHREAD
            if (pbi->b_multithreaded_rd)
                vp8mt_alloc_temp_buffers(pbi, pc->Width, prev_mb_rows);
#endif
        }
    }

    if (pc->Width == 0 || pc->Height == 0)
    {
        return -1;
    }

    init_frame(pbi);

    if (vp8dx_start_decode(bc, IF_RTCD(&pbi->dboolhuff),
                           data, data_end - data))
        vpx_internal_error(&pc->error, VPX_CODEC_MEM_ERROR,
                           "Failed to allocate bool decoder 0");
    if (pc->frame_type == KEY_FRAME) {
        pc->clr_type    = (YUV_TYPE)vp8_read_bit(bc);
        pc->clamp_type  = (CLAMP_TYPE)vp8_read_bit(bc);
    }

    
    xd->segmentation_enabled = (unsigned char)vp8_read_bit(bc);

    if (xd->segmentation_enabled)
    {
        
        xd->update_mb_segmentation_map = (unsigned char)vp8_read_bit(bc);
        xd->update_mb_segmentation_data = (unsigned char)vp8_read_bit(bc);

        if (xd->update_mb_segmentation_data)
        {
            xd->mb_segement_abs_delta = (unsigned char)vp8_read_bit(bc);

            vpx_memset(xd->segment_feature_data, 0, sizeof(xd->segment_feature_data));

            
            for (i = 0; i < MB_LVL_MAX; i++)
            {
                for (j = 0; j < MAX_MB_SEGMENTS; j++)
                {
                    
                    if (vp8_read_bit(bc))
                    {
                        xd->segment_feature_data[i][j] = (signed char)vp8_read_literal(bc, mb_feature_data_bits[i]);

                        if (vp8_read_bit(bc))
                            xd->segment_feature_data[i][j] = -xd->segment_feature_data[i][j];
                    }
                    else
                        xd->segment_feature_data[i][j] = 0;
                }
            }
        }

        if (xd->update_mb_segmentation_map)
        {
            
            vpx_memset(xd->mb_segment_tree_probs, 255, sizeof(xd->mb_segment_tree_probs));

            
            for (i = 0; i < MB_FEATURE_TREE_PROBS; i++)
            {
                
                if (vp8_read_bit(bc))
                    xd->mb_segment_tree_probs[i] = (vp8_prob)vp8_read_literal(bc, 8);
            }
        }
    }

    
    pc->filter_type = (LOOPFILTERTYPE) vp8_read_bit(bc);
    pc->filter_level = vp8_read_literal(bc, 6);
    pc->sharpness_level = vp8_read_literal(bc, 3);

    
    xd->mode_ref_lf_delta_update = 0;
    xd->mode_ref_lf_delta_enabled = (unsigned char)vp8_read_bit(bc);

    if (xd->mode_ref_lf_delta_enabled)
    {
        
        xd->mode_ref_lf_delta_update = (unsigned char)vp8_read_bit(bc);

        if (xd->mode_ref_lf_delta_update)
        {
            
            for (i = 0; i < MAX_REF_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bc))
                {
                    
                    xd->ref_lf_deltas[i] = (signed char)vp8_read_literal(bc, 6);

                    if (vp8_read_bit(bc))        
                        xd->ref_lf_deltas[i] = xd->ref_lf_deltas[i] * -1;
                }
            }

            
            for (i = 0; i < MAX_MODE_LF_DELTAS; i++)
            {
                if (vp8_read_bit(bc))
                {
                    
                    xd->mode_lf_deltas[i] = (signed char)vp8_read_literal(bc, 6);

                    if (vp8_read_bit(bc))        
                        xd->mode_lf_deltas[i] = xd->mode_lf_deltas[i] * -1;
                }
            }
        }
    }

    setup_token_decoder(pbi, data + first_partition_length_in_bytes);
    xd->current_bc = &pbi->bc2;

    
    {
        int Q, q_update;

        Q = vp8_read_literal(bc, 7);  
        pc->base_qindex = Q;
        q_update = 0;
        pc->y1dc_delta_q = get_delta_q(bc, pc->y1dc_delta_q, &q_update);
        pc->y2dc_delta_q = get_delta_q(bc, pc->y2dc_delta_q, &q_update);
        pc->y2ac_delta_q = get_delta_q(bc, pc->y2ac_delta_q, &q_update);
        pc->uvdc_delta_q = get_delta_q(bc, pc->uvdc_delta_q, &q_update);
        pc->uvac_delta_q = get_delta_q(bc, pc->uvac_delta_q, &q_update);

        if (q_update)
            vp8cx_init_de_quantizer(pbi);

        
        mb_init_dequantizer(pbi, &pbi->mb);
    }

    



    if (pc->frame_type != KEY_FRAME)
    {
        
        pc->refresh_golden_frame = vp8_read_bit(bc);
        pc->refresh_alt_ref_frame = vp8_read_bit(bc);

        
        pc->copy_buffer_to_gf = 0;

        if (!pc->refresh_golden_frame)
            pc->copy_buffer_to_gf = vp8_read_literal(bc, 2);

        pc->copy_buffer_to_arf = 0;

        if (!pc->refresh_alt_ref_frame)
            pc->copy_buffer_to_arf = vp8_read_literal(bc, 2);

        pc->ref_frame_sign_bias[GOLDEN_FRAME] = vp8_read_bit(bc);
        pc->ref_frame_sign_bias[ALTREF_FRAME] = vp8_read_bit(bc);
    }

    pc->refresh_entropy_probs = vp8_read_bit(bc);
    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->lfc, &pc->fc, sizeof(pc->fc));
    }

    pc->refresh_last_frame = pc->frame_type == KEY_FRAME  ||  vp8_read_bit(bc);

    if (0)
    {
        FILE *z = fopen("decodestats.stt", "a");
        fprintf(z, "%6d F:%d,G:%d,A:%d,L:%d,Q:%d\n",
                pc->current_video_frame,
                pc->frame_type,
                pc->refresh_golden_frame,
                pc->refresh_alt_ref_frame,
                pc->refresh_last_frame,
                pc->base_qindex);
        fclose(z);
    }


    {
        

        for (i = 0; i < BLOCK_TYPES; i++)
            for (j = 0; j < COEF_BANDS; j++)
                for (k = 0; k < PREV_COEF_CONTEXTS; k++)
                    for (l = 0; l < MAX_ENTROPY_TOKENS - 1; l++)
                    {

                        vp8_prob *const p = pc->fc.coef_probs [i][j][k] + l;

                        if (vp8_read(bc, vp8_coef_update_probs [i][j][k][l]))
                        {
                            *p = (vp8_prob)vp8_read_literal(bc, 8);

                        }
                    }
    }

    vpx_memcpy(&xd->pre, &pc->yv12_fb[pc->lst_fb_idx], sizeof(YV12_BUFFER_CONFIG));
    vpx_memcpy(&xd->dst, &pc->yv12_fb[pc->new_fb_idx], sizeof(YV12_BUFFER_CONFIG));

    
    if (!(pbi->b_multithreaded_rd) || pc->multi_token_partition == ONE_PARTITION || !(pc->filter_level))
        vp8_setup_intra_recon(&pc->yv12_fb[pc->new_fb_idx]);

    vp8_setup_block_dptrs(xd);

    vp8_build_block_doffsets(xd);

    
    vpx_memset(xd->qcoeff, 0, sizeof(xd->qcoeff));

    
    pc->mb_no_coeff_skip = (int)vp8_read_bit(bc);


    vp8_decode_mode_mvs(pbi);

    vpx_memset(pc->above_context, 0, sizeof(ENTROPY_CONTEXT_PLANES) * pc->mb_cols);

    vpx_memcpy(&xd->block[0].bmi, &xd->mode_info_context->bmi[0], sizeof(B_MODE_INFO));

    if (pbi->b_multithreaded_rd && pc->multi_token_partition != ONE_PARTITION)
    {
        vp8mt_decode_mb_rows(pbi, xd);
        if(pbi->common.filter_level)
        {
             

            pc->last_frame_type = pc->frame_type;
            pc->last_filter_type = pc->filter_type;
            pc->last_sharpness_level = pc->sharpness_level;
        }
        vp8_yv12_extend_frame_borders_ptr(&pc->yv12_fb[pc->new_fb_idx]);    
    }
    else
    {
        int ibc = 0;
        int num_part = 1 << pc->multi_token_partition;

        
        for (mb_row = 0; mb_row < pc->mb_rows; mb_row++)
        {

            if (num_part > 1)
            {
                xd->current_bc = & pbi->mbc[ibc];
                ibc++;

                if (ibc == num_part)
                    ibc = 0;
            }

            vp8_decode_mb_row(pbi, pc, mb_row, xd);
        }
    }


    stop_token_decoder(pbi);

    

    
    if ((pc->frame_type == KEY_FRAME) ||
         pc->refresh_golden_frame || pc->refresh_alt_ref_frame)
    {
        pc->last_kf_gf_q = pc->base_qindex;
    }

    if (pc->refresh_entropy_probs == 0)
    {
        vpx_memcpy(&pc->fc, &pc->lfc, sizeof(pc->fc));
    }

#ifdef PACKET_TESTING
    {
        FILE *f = fopen("decompressor.VP8", "ab");
        unsigned int size = pbi->bc2.pos + pbi->bc.pos + 8;
        fwrite((void *) &size, 4, 1, f);
        fwrite((void *) pbi->Source, size, 1, f);
        fclose(f);
    }
#endif

    return 0;
}
