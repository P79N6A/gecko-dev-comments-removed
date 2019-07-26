









#include <math.h>
#include <limits.h>
#include <stdio.h>
#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_onyx_int.h"
#include "vp9/encoder/vp9_variance.h"
#include "vp9/encoder/vp9_encodeintra.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/encoder/vp9_firstpass.h"
#include "vpx_scale/vpx_scale.h"
#include "vp9/encoder/vp9_encodeframe.h"
#include "vp9/encoder/vp9_encodemb.h"
#include "vp9/common/vp9_extend.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_scale/yv12config.h"
#include "vp9/encoder/vp9_quantize.h"
#include "vp9/encoder/vp9_rdopt.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_entropymv.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/encoder/vp9_vaq.h"
#include "./vpx_scale_rtcd.h"

#include "vp9/common/vp9_reconinter.h"

#define OUTPUT_FPF 0

#define IIFACTOR   12.5
#define IIKFACTOR1 12.5
#define IIKFACTOR2 15.0
#define RMAX       512.0
#define GF_RMAX    96.0
#define ERR_DIVISOR   150.0
#define MIN_DECAY_FACTOR 0.1

#define KF_MB_INTRA_MIN 150
#define GF_MB_INTRA_MIN 100

#define DOUBLE_DIVIDE_CHECK(x) ((x) < 0 ? (x) - 0.000001 : (x) + 0.000001)

#define POW1 (double)cpi->oxcf.two_pass_vbrbias/100.0
#define POW2 (double)cpi->oxcf.two_pass_vbrbias/100.0

static void swap_yv12(YV12_BUFFER_CONFIG *a, YV12_BUFFER_CONFIG *b) {
  YV12_BUFFER_CONFIG temp = *a;
  *a = *b;
  *b = temp;
}

static void find_next_key_frame(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame);

static int select_cq_level(int qindex) {
  int ret_val = QINDEX_RANGE - 1;
  int i;

  double target_q = (vp9_convert_qindex_to_q(qindex) * 0.5847) + 1.0;

  for (i = 0; i < QINDEX_RANGE; i++) {
    if (target_q <= vp9_convert_qindex_to_q(i)) {
      ret_val = i;
      break;
    }
  }

  return ret_val;
}




static void reset_fpf_position(VP9_COMP *cpi, FIRSTPASS_STATS *position) {
  cpi->twopass.stats_in = position;
}

static int lookup_next_frame_stats(VP9_COMP *cpi, FIRSTPASS_STATS *next_frame) {
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
    return EOF;

  *next_frame = *cpi->twopass.stats_in;
  return 1;
}


static int read_frame_stats(VP9_COMP *cpi,
                            FIRSTPASS_STATS *frame_stats,
                            int offset) {
  FIRSTPASS_STATS *fps_ptr = cpi->twopass.stats_in;

  
  if (offset >= 0) {
    if (&fps_ptr[offset] >= cpi->twopass.stats_in_end)
      return EOF;
  } else if (offset < 0) {
    if (&fps_ptr[offset] < cpi->twopass.stats_in_start)
      return EOF;
  }

  *frame_stats = fps_ptr[offset];
  return 1;
}

static int input_stats(VP9_COMP *cpi, FIRSTPASS_STATS *fps) {
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end)
    return EOF;

  *fps = *cpi->twopass.stats_in;
  cpi->twopass.stats_in =
    (void *)((char *)cpi->twopass.stats_in + sizeof(FIRSTPASS_STATS));
  return 1;
}

static void output_stats(const VP9_COMP            *cpi,
                         struct vpx_codec_pkt_list *pktlist,
                         FIRSTPASS_STATS            *stats) {
  struct vpx_codec_cx_pkt pkt;
  pkt.kind = VPX_CODEC_STATS_PKT;
  pkt.data.twopass_stats.buf = stats;
  pkt.data.twopass_stats.sz = sizeof(FIRSTPASS_STATS);
  vpx_codec_pkt_list_add(pktlist, &pkt);


#if OUTPUT_FPF

  {
    FILE *fpfile;
    fpfile = fopen("firstpass.stt", "a");

    fprintf(stdout, "%12.0f %12.0f %12.0f %12.0f %12.0f %12.4f %12.4f"
            "%12.4f %12.4f %12.4f %12.4f %12.4f %12.4f %12.4f"
            "%12.0f %12.0f %12.4f %12.0f %12.0f %12.4f\n",
            stats->frame,
            stats->intra_error,
            stats->coded_error,
            stats->sr_coded_error,
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
            stats->new_mv_count,
            stats->count,
            stats->duration);
    fclose(fpfile);
  }
#endif
}

static void zero_stats(FIRSTPASS_STATS *section) {
  section->frame      = 0.0;
  section->intra_error = 0.0;
  section->coded_error = 0.0;
  section->sr_coded_error = 0.0;
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
  section->new_mv_count = 0.0;
  section->count      = 0.0;
  section->duration   = 1.0;
}

static void accumulate_stats(FIRSTPASS_STATS *section, FIRSTPASS_STATS *frame) {
  section->frame += frame->frame;
  section->intra_error += frame->intra_error;
  section->coded_error += frame->coded_error;
  section->sr_coded_error += frame->sr_coded_error;
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
  section->new_mv_count += frame->new_mv_count;
  section->count      += frame->count;
  section->duration   += frame->duration;
}

static void subtract_stats(FIRSTPASS_STATS *section, FIRSTPASS_STATS *frame) {
  section->frame -= frame->frame;
  section->intra_error -= frame->intra_error;
  section->coded_error -= frame->coded_error;
  section->sr_coded_error -= frame->sr_coded_error;
  section->ssim_weighted_pred_err -= frame->ssim_weighted_pred_err;
  section->pcnt_inter  -= frame->pcnt_inter;
  section->pcnt_motion -= frame->pcnt_motion;
  section->pcnt_second_ref -= frame->pcnt_second_ref;
  section->pcnt_neutral -= frame->pcnt_neutral;
  section->MVr        -= frame->MVr;
  section->mvr_abs     -= frame->mvr_abs;
  section->MVc        -= frame->MVc;
  section->mvc_abs     -= frame->mvc_abs;
  section->MVrv       -= frame->MVrv;
  section->MVcv       -= frame->MVcv;
  section->mv_in_out_count  -= frame->mv_in_out_count;
  section->new_mv_count -= frame->new_mv_count;
  section->count      -= frame->count;
  section->duration   -= frame->duration;
}

static void avg_stats(FIRSTPASS_STATS *section) {
  if (section->count < 1.0)
    return;

  section->intra_error /= section->count;
  section->coded_error /= section->count;
  section->sr_coded_error /= section->count;
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



static double calculate_modified_err(VP9_COMP *cpi,
                                     FIRSTPASS_STATS *this_frame) {
  const FIRSTPASS_STATS *const stats = &cpi->twopass.total_stats;
  const double av_err = stats->ssim_weighted_pred_err / stats->count;
  const double this_err = this_frame->ssim_weighted_pred_err;
  return av_err * pow(this_err / DOUBLE_DIVIDE_CHECK(av_err),
                      this_err > av_err ? POW1 : POW2);
}

static const double weight_table[256] = {
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.020000,
  0.020000, 0.020000, 0.020000, 0.020000, 0.020000, 0.031250, 0.062500,
  0.093750, 0.125000, 0.156250, 0.187500, 0.218750, 0.250000, 0.281250,
  0.312500, 0.343750, 0.375000, 0.406250, 0.437500, 0.468750, 0.500000,
  0.531250, 0.562500, 0.593750, 0.625000, 0.656250, 0.687500, 0.718750,
  0.750000, 0.781250, 0.812500, 0.843750, 0.875000, 0.906250, 0.937500,
  0.968750, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
  1.000000, 1.000000, 1.000000, 1.000000
};

static double simple_weight(YV12_BUFFER_CONFIG *source) {
  int i, j;

  uint8_t *src = source->y_buffer;
  double sum_weights = 0.0;

  
  
  i = source->y_height;
  do {
    j = source->y_width;
    do {
      sum_weights += weight_table[ *src];
      src++;
    } while (--j);
    src -= source->y_width;
    src += source->y_stride;
  } while (--i);

  sum_weights /= (source->y_height * source->y_width);

  return sum_weights;
}



static int frame_max_bits(VP9_COMP *cpi) {
  
  
  
  
  const double max_bits = (1.0 * cpi->twopass.bits_left /
      (cpi->twopass.total_stats.count - cpi->common.current_video_frame)) *
      (cpi->oxcf.two_pass_vbrmax_section / 100.0);

  
  return MAX((int)max_bits, 0);
}

void vp9_init_first_pass(VP9_COMP *cpi) {
  zero_stats(&cpi->twopass.total_stats);
}

void vp9_end_first_pass(VP9_COMP *cpi) {
  output_stats(cpi, cpi->output_pkt_list, &cpi->twopass.total_stats);
}

static void zz_motion_search(VP9_COMP *cpi, MACROBLOCK *x,
                             YV12_BUFFER_CONFIG *recon_buffer,
                             int *best_motion_err, int recon_yoffset) {
  MACROBLOCKD *const xd = &x->e_mbd;

  
  xd->plane[0].pre[0].buf = recon_buffer->y_buffer + recon_yoffset;

  switch (xd->mi_8x8[0]->mbmi.sb_type) {
    case BLOCK_8X8:
      vp9_mse8x8(x->plane[0].src.buf, x->plane[0].src.stride,
                 xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                 (unsigned int *)(best_motion_err));
      break;
    case BLOCK_16X8:
      vp9_mse16x8(x->plane[0].src.buf, x->plane[0].src.stride,
                  xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                  (unsigned int *)(best_motion_err));
      break;
    case BLOCK_8X16:
      vp9_mse8x16(x->plane[0].src.buf, x->plane[0].src.stride,
                  xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                  (unsigned int *)(best_motion_err));
      break;
    default:
      vp9_mse16x16(x->plane[0].src.buf, x->plane[0].src.stride,
                   xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                   (unsigned int *)(best_motion_err));
      break;
  }
}

static void first_pass_motion_search(VP9_COMP *cpi, MACROBLOCK *x,
                                     int_mv *ref_mv, MV *best_mv,
                                     YV12_BUFFER_CONFIG *recon_buffer,
                                     int *best_motion_err, int recon_yoffset) {
  MACROBLOCKD *const xd = &x->e_mbd;
  int num00;

  int_mv tmp_mv;
  int_mv ref_mv_full;

  int tmp_err;
  int step_param = 3;
  int further_steps = (MAX_MVSEARCH_STEPS - 1) - step_param;
  int n;
  vp9_variance_fn_ptr_t v_fn_ptr =
      cpi->fn_ptr[xd->mi_8x8[0]->mbmi.sb_type];
  int new_mv_mode_penalty = 256;

  int sr = 0;
  int quart_frm = MIN(cpi->common.width, cpi->common.height);

  
  
  while ((quart_frm << sr) < MAX_FULL_PEL_VAL)
    sr++;
  if (sr)
    sr--;

  step_param    += sr;
  further_steps -= sr;

  
  switch (xd->mi_8x8[0]->mbmi.sb_type) {
    case BLOCK_8X8:
      v_fn_ptr.vf = vp9_mse8x8;
      break;
    case BLOCK_16X8:
      v_fn_ptr.vf = vp9_mse16x8;
      break;
    case BLOCK_8X16:
      v_fn_ptr.vf = vp9_mse8x16;
      break;
    default:
      v_fn_ptr.vf = vp9_mse16x16;
      break;
  }

  
  xd->plane[0].pre[0].buf = recon_buffer->y_buffer + recon_yoffset;

  
  tmp_mv.as_int = 0;
  ref_mv_full.as_mv.col = ref_mv->as_mv.col >> 3;
  ref_mv_full.as_mv.row = ref_mv->as_mv.row >> 3;
  tmp_err = cpi->diamond_search_sad(x, &ref_mv_full, &tmp_mv, step_param,
                                    x->sadperbit16, &num00, &v_fn_ptr,
                                    x->nmvjointcost,
                                    x->mvcost, ref_mv);
  if (tmp_err < INT_MAX - new_mv_mode_penalty)
    tmp_err += new_mv_mode_penalty;

  if (tmp_err < *best_motion_err) {
    *best_motion_err = tmp_err;
    best_mv->row = tmp_mv.as_mv.row;
    best_mv->col = tmp_mv.as_mv.col;
  }

  
  n = num00;
  num00 = 0;

  while (n < further_steps) {
    n++;

    if (num00) {
      num00--;
    } else {
      tmp_err = cpi->diamond_search_sad(x, &ref_mv_full, &tmp_mv,
                                        step_param + n, x->sadperbit16,
                                        &num00, &v_fn_ptr,
                                        x->nmvjointcost,
                                        x->mvcost, ref_mv);
      if (tmp_err < INT_MAX - new_mv_mode_penalty)
        tmp_err += new_mv_mode_penalty;

      if (tmp_err < *best_motion_err) {
        *best_motion_err = tmp_err;
        best_mv->row = tmp_mv.as_mv.row;
        best_mv->col = tmp_mv.as_mv.col;
      }
    }
  }
}

void vp9_first_pass(VP9_COMP *cpi) {
  int mb_row, mb_col;
  MACROBLOCK *const x = &cpi->mb;
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &x->e_mbd;
  TileInfo tile;
  struct macroblock_plane *const p = x->plane;
  struct macroblockd_plane *const pd = xd->plane;
  PICK_MODE_CONTEXT *ctx = &x->sb64_context;
  int i;

  int recon_yoffset, recon_uvoffset;
  const int lst_yv12_idx = cm->ref_frame_map[cpi->lst_fb_idx];
  const int gld_yv12_idx = cm->ref_frame_map[cpi->gld_fb_idx];
  YV12_BUFFER_CONFIG *const lst_yv12 = &cm->yv12_fb[lst_yv12_idx];
  YV12_BUFFER_CONFIG *const gld_yv12 = &cm->yv12_fb[gld_yv12_idx];
  YV12_BUFFER_CONFIG *const new_yv12 = get_frame_new_buffer(cm);
  const int recon_y_stride = lst_yv12->y_stride;
  const int recon_uv_stride = lst_yv12->uv_stride;
  int64_t intra_error = 0;
  int64_t coded_error = 0;
  int64_t sr_coded_error = 0;

  int sum_mvr = 0, sum_mvc = 0;
  int sum_mvr_abs = 0, sum_mvc_abs = 0;
  int sum_mvrs = 0, sum_mvcs = 0;
  int mvcount = 0;
  int intercount = 0;
  int second_ref_count = 0;
  int intrapenalty = 256;
  int neutral_count = 0;
  int new_mv_count = 0;
  int sum_in_vectors = 0;
  uint32_t lastmv_as_int = 0;

  int_mv zero_ref_mv;

  zero_ref_mv.as_int = 0;

  vp9_clear_system_state();  

  vp9_setup_src_planes(x, cpi->Source, 0, 0);
  setup_pre_planes(xd, 0, lst_yv12, 0, 0, NULL);
  setup_dst_planes(xd, new_yv12, 0, 0);

  xd->mi_8x8 = cm->mi_grid_visible;
  
  xd->mi_8x8[0] = cm->mi;

  setup_block_dptrs(&x->e_mbd, cm->subsampling_x, cm->subsampling_y);

  vp9_frame_init_quantizer(cpi);

  for (i = 0; i < MAX_MB_PLANE; ++i) {
    p[i].coeff = ctx->coeff_pbuf[i][1];
    pd[i].qcoeff = ctx->qcoeff_pbuf[i][1];
    pd[i].dqcoeff = ctx->dqcoeff_pbuf[i][1];
    pd[i].eobs = ctx->eobs_pbuf[i][1];
  }
  x->skip_recode = 0;


  
  
  
  {
    vp9_init_mv_probs(cm);
    vp9_initialize_rd_consts(cpi);
  }

  
  vp9_tile_init(&tile, cm, 0, 0);

  
  for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
    int_mv best_ref_mv;

    best_ref_mv.as_int = 0;

    
    xd->up_available = (mb_row != 0);
    recon_yoffset = (mb_row * recon_y_stride * 16);
    recon_uvoffset = (mb_row * recon_uv_stride * 8);

    
    
    x->mv_row_min = -((mb_row * 16) + BORDER_MV_PIXELS_B16);
    x->mv_row_max = ((cm->mb_rows - 1 - mb_row) * 16)
                    + BORDER_MV_PIXELS_B16;

    
    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      int this_error;
      int gf_motion_error = INT_MAX;
      int use_dc_pred = (mb_col || mb_row) && (!mb_col || !mb_row);
      double error_weight;

      vp9_clear_system_state();  
      error_weight = 1.0;  

      xd->plane[0].dst.buf = new_yv12->y_buffer + recon_yoffset;
      xd->plane[1].dst.buf = new_yv12->u_buffer + recon_uvoffset;
      xd->plane[2].dst.buf = new_yv12->v_buffer + recon_uvoffset;
      xd->left_available = (mb_col != 0);

      if (mb_col * 2 + 1 < cm->mi_cols) {
        if (mb_row * 2 + 1 < cm->mi_rows) {
          xd->mi_8x8[0]->mbmi.sb_type = BLOCK_16X16;
        } else {
          xd->mi_8x8[0]->mbmi.sb_type = BLOCK_16X8;
        }
      } else {
        if (mb_row * 2 + 1 < cm->mi_rows) {
          xd->mi_8x8[0]->mbmi.sb_type = BLOCK_8X16;
        } else {
          xd->mi_8x8[0]->mbmi.sb_type = BLOCK_8X8;
        }
      }
      xd->mi_8x8[0]->mbmi.ref_frame[0] = INTRA_FRAME;
      set_mi_row_col(xd, &tile,
                     mb_row << 1,
                     num_8x8_blocks_high_lookup[xd->mi_8x8[0]->mbmi.sb_type],
                     mb_col << 1,
                     num_8x8_blocks_wide_lookup[xd->mi_8x8[0]->mbmi.sb_type],
                     cm->mi_rows, cm->mi_cols);

      if (cpi->oxcf.aq_mode == VARIANCE_AQ) {
        int energy = vp9_block_energy(cpi, x, xd->mi_8x8[0]->mbmi.sb_type);
        error_weight = vp9_vaq_inv_q_ratio(energy);
      }

      
      this_error = vp9_encode_intra(x, use_dc_pred);
      if (cpi->oxcf.aq_mode == VARIANCE_AQ) {
        vp9_clear_system_state();  
        this_error *= error_weight;
      }

      
      
      
      
      
      
      
      this_error += intrapenalty;

      
      intra_error += (int64_t)this_error;

      
      
      x->mv_col_min = -((mb_col * 16) + BORDER_MV_PIXELS_B16);
      x->mv_col_max = ((cm->mb_cols - 1 - mb_col) * 16)
                      + BORDER_MV_PIXELS_B16;

      
      if (cm->current_video_frame > 0) {
        int tmp_err;
        int motion_error = INT_MAX;
        int_mv mv, tmp_mv;

        
        zz_motion_search(cpi, x, lst_yv12, &motion_error, recon_yoffset);
        mv.as_int = tmp_mv.as_int = 0;

        
        
        first_pass_motion_search(cpi, x, &best_ref_mv,
                                 &mv.as_mv, lst_yv12,
                                 &motion_error, recon_yoffset);
        if (cpi->oxcf.aq_mode == VARIANCE_AQ) {
          vp9_clear_system_state();  
          motion_error *= error_weight;
        }

        
        
        if (best_ref_mv.as_int) {
          tmp_err = INT_MAX;
          first_pass_motion_search(cpi, x, &zero_ref_mv, &tmp_mv.as_mv,
                                   lst_yv12, &tmp_err, recon_yoffset);
          if (cpi->oxcf.aq_mode == VARIANCE_AQ) {
            vp9_clear_system_state();  
            tmp_err *= error_weight;
          }

          if (tmp_err < motion_error) {
            motion_error = tmp_err;
            mv.as_int = tmp_mv.as_int;
          }
        }

        
        if (cm->current_video_frame > 1) {
          
          zz_motion_search(cpi, x, gld_yv12,
                           &gf_motion_error, recon_yoffset);

          first_pass_motion_search(cpi, x, &zero_ref_mv,
                                   &tmp_mv.as_mv, gld_yv12,
                                   &gf_motion_error, recon_yoffset);
          if (cpi->oxcf.aq_mode == VARIANCE_AQ) {
            vp9_clear_system_state();  
            gf_motion_error *= error_weight;
          }

          if ((gf_motion_error < motion_error) &&
              (gf_motion_error < this_error)) {
            second_ref_count++;
          }

          
          xd->plane[0].pre[0].buf = lst_yv12->y_buffer + recon_yoffset;
          xd->plane[1].pre[0].buf = lst_yv12->u_buffer + recon_uvoffset;
          xd->plane[2].pre[0].buf = lst_yv12->v_buffer + recon_uvoffset;

          
          
          
          
          if (gf_motion_error < this_error)
            sr_coded_error += gf_motion_error;
          else
            sr_coded_error += this_error;
        } else {
          sr_coded_error += motion_error;
        }
        
        best_ref_mv.as_int = 0;

        if (motion_error <= this_error) {
          
          
          
          
          if ((((this_error - intrapenalty) * 9) <=
               (motion_error * 10)) &&
              (this_error < (2 * intrapenalty))) {
            neutral_count++;
          }

          mv.as_mv.row *= 8;
          mv.as_mv.col *= 8;
          this_error = motion_error;
          vp9_set_mbmode_and_mvs(x, NEWMV, &mv);
          xd->mi_8x8[0]->mbmi.tx_size = TX_4X4;
          xd->mi_8x8[0]->mbmi.ref_frame[0] = LAST_FRAME;
          xd->mi_8x8[0]->mbmi.ref_frame[1] = NONE;
          vp9_build_inter_predictors_sby(xd, mb_row << 1,
                                         mb_col << 1,
                                         xd->mi_8x8[0]->mbmi.sb_type);
          vp9_encode_sby(x, xd->mi_8x8[0]->mbmi.sb_type);
          sum_mvr += mv.as_mv.row;
          sum_mvr_abs += abs(mv.as_mv.row);
          sum_mvc += mv.as_mv.col;
          sum_mvc_abs += abs(mv.as_mv.col);
          sum_mvrs += mv.as_mv.row * mv.as_mv.row;
          sum_mvcs += mv.as_mv.col * mv.as_mv.col;
          intercount++;

          best_ref_mv.as_int = mv.as_int;

          
          if (mv.as_int) {
            mvcount++;

            
            if (mv.as_int != lastmv_as_int)
              new_mv_count++;
            lastmv_as_int = mv.as_int;

            
            if (mb_row < cm->mb_rows / 2) {
              if (mv.as_mv.row > 0)
                sum_in_vectors--;
              else if (mv.as_mv.row < 0)
                sum_in_vectors++;
            } else if (mb_row > cm->mb_rows / 2) {
              if (mv.as_mv.row > 0)
                sum_in_vectors++;
              else if (mv.as_mv.row < 0)
                sum_in_vectors--;
            }

            
            if (mb_col < cm->mb_cols / 2) {
              if (mv.as_mv.col > 0)
                sum_in_vectors--;
              else if (mv.as_mv.col < 0)
                sum_in_vectors++;
            } else if (mb_col > cm->mb_cols / 2) {
              if (mv.as_mv.col > 0)
                sum_in_vectors++;
              else if (mv.as_mv.col < 0)
                sum_in_vectors--;
            }
          }
        }
      } else {
        sr_coded_error += (int64_t)this_error;
      }
      coded_error += (int64_t)this_error;

      
      x->plane[0].src.buf += 16;
      x->plane[1].src.buf += 8;
      x->plane[2].src.buf += 8;

      recon_yoffset += 16;
      recon_uvoffset += 8;
    }

    
    x->plane[0].src.buf += 16 * x->plane[0].src.stride - 16 * cm->mb_cols;
    x->plane[1].src.buf += 8 * x->plane[1].src.stride - 8 * cm->mb_cols;
    x->plane[2].src.buf += 8 * x->plane[1].src.stride - 8 * cm->mb_cols;

    vp9_clear_system_state();  
  }

  vp9_clear_system_state();  
  {
    double weight = 0.0;

    FIRSTPASS_STATS fps;

    fps.frame      = cm->current_video_frame;
    fps.intra_error = (double)(intra_error >> 8);
    fps.coded_error = (double)(coded_error >> 8);
    fps.sr_coded_error = (double)(sr_coded_error >> 8);
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
    fps.new_mv_count = 0.0;
    fps.count      = 1.0;

    fps.pcnt_inter   = 1.0 * (double)intercount / cm->MBs;
    fps.pcnt_second_ref = 1.0 * (double)second_ref_count / cm->MBs;
    fps.pcnt_neutral = 1.0 * (double)neutral_count / cm->MBs;

    if (mvcount > 0) {
      fps.MVr = (double)sum_mvr / (double)mvcount;
      fps.mvr_abs = (double)sum_mvr_abs / (double)mvcount;
      fps.MVc = (double)sum_mvc / (double)mvcount;
      fps.mvc_abs = (double)sum_mvc_abs / (double)mvcount;
      fps.MVrv = ((double)sum_mvrs - (fps.MVr * fps.MVr / (double)mvcount)) /
                 (double)mvcount;
      fps.MVcv = ((double)sum_mvcs - (fps.MVc * fps.MVc / (double)mvcount)) /
                 (double)mvcount;
      fps.mv_in_out_count = (double)sum_in_vectors / (double)(mvcount * 2);
      fps.new_mv_count = new_mv_count;

      fps.pcnt_motion = 1.0 * (double)mvcount / cpi->common.MBs;
    }

    
    
    
    fps.duration = (double)(cpi->source->ts_end
                            - cpi->source->ts_start);

    
    cpi->twopass.this_frame_stats = fps;
    output_stats(cpi, cpi->output_pkt_list, &cpi->twopass.this_frame_stats);
    accumulate_stats(&cpi->twopass.total_stats, &fps);
  }

  
  
  if ((cpi->twopass.sr_update_lag > 3) ||
      ((cm->current_video_frame > 0) &&
       (cpi->twopass.this_frame_stats.pcnt_inter > 0.20) &&
       ((cpi->twopass.this_frame_stats.intra_error /
         DOUBLE_DIVIDE_CHECK(cpi->twopass.this_frame_stats.coded_error)) >
        2.0))) {
    vp8_yv12_copy_frame(lst_yv12, gld_yv12);
    cpi->twopass.sr_update_lag = 1;
  } else {
    cpi->twopass.sr_update_lag++;
  }
  
  swap_yv12(lst_yv12, new_yv12);

  vp9_extend_frame_borders(lst_yv12, cm->subsampling_x, cm->subsampling_y);

  
  
  if (cm->current_video_frame == 0)
    vp8_yv12_copy_frame(lst_yv12, gld_yv12);

  
  if (0) {
    char filename[512];
    FILE *recon_file;
    snprintf(filename, sizeof(filename), "enc%04d.yuv",
             (int)cm->current_video_frame);

    if (cm->current_video_frame == 0)
      recon_file = fopen(filename, "wb");
    else
      recon_file = fopen(filename, "ab");

    (void)fwrite(lst_yv12->buffer_alloc, lst_yv12->frame_size, 1, recon_file);
    fclose(recon_file);
  }

  cm->current_video_frame++;
}







static double bitcost(double prob) {
  return -(log(prob) / log(2.0));
}

static int64_t estimate_modemvcost(VP9_COMP *cpi,
                                     FIRSTPASS_STATS *fpstats) {
#if 0
  int mv_cost;
  int mode_cost;

  double av_pct_inter = fpstats->pcnt_inter / fpstats->count;
  double av_pct_motion = fpstats->pcnt_motion / fpstats->count;
  double av_intra = (1.0 - av_pct_inter);

  double zz_cost;
  double motion_cost;
  double intra_cost;

  zz_cost = bitcost(av_pct_inter - av_pct_motion);
  motion_cost = bitcost(av_pct_motion);
  intra_cost = bitcost(av_intra);

  
  
  mv_cost = ((int)(fpstats->new_mv_count / fpstats->count) * 8) << 9;

  
  
  mode_cost =
    (int)((((av_pct_inter - av_pct_motion) * zz_cost) +
           (av_pct_motion * motion_cost) +
           (av_intra * intra_cost)) * cpi->common.MBs) << 9;

  
  
#endif
  return 0;
}

static double calc_correction_factor(double err_per_mb,
                                     double err_divisor,
                                     double pt_low,
                                     double pt_high,
                                     int q) {
  const double error_term = err_per_mb / err_divisor;

  
  const double power_term = MIN(vp9_convert_qindex_to_q(q) * 0.01 + pt_low,
                                pt_high);

  
  if (power_term < 1.0)
    assert(error_term >= 0.0);

  return fclamp(pow(error_term, power_term), 0.05, 5.0);
}





static void adjust_maxq_qrange(VP9_COMP *cpi) {
  int i;
  
  double q = cpi->avg_q * 2.0;
  cpi->twopass.maxq_max_limit = cpi->worst_quality;
  for (i = cpi->best_quality; i <= cpi->worst_quality; i++) {
    cpi->twopass.maxq_max_limit = i;
    if (vp9_convert_qindex_to_q(i) >= q)
      break;
  }

  
  q = cpi->avg_q * 0.5;
  cpi->twopass.maxq_min_limit = cpi->best_quality;
  for (i = cpi->worst_quality; i >= cpi->best_quality; i--) {
    cpi->twopass.maxq_min_limit = i;
    if (vp9_convert_qindex_to_q(i) <= q)
      break;
  }
}

static int estimate_max_q(VP9_COMP *cpi,
                          FIRSTPASS_STATS *fpstats,
                          int section_target_bandwitdh) {
  int q;
  int num_mbs = cpi->common.MBs;
  int target_norm_bits_per_mb;

  double section_err = fpstats->coded_error / fpstats->count;
  double sr_correction;
  double err_per_mb = section_err / num_mbs;
  double err_correction_factor;
  double speed_correction = 1.0;

  if (section_target_bandwitdh <= 0)
    return cpi->twopass.maxq_max_limit;          

  target_norm_bits_per_mb = section_target_bandwitdh < (1 << 20)
                              ? (512 * section_target_bandwitdh) / num_mbs
                              : 512 * (section_target_bandwitdh / num_mbs);

  
  
  if (fpstats->sr_coded_error > fpstats->coded_error) {
    double sr_err_diff = (fpstats->sr_coded_error - fpstats->coded_error) /
                             (fpstats->count * cpi->common.MBs);
    sr_correction = fclamp(pow(sr_err_diff / 32.0, 0.25), 0.75, 1.25);
  } else {
    sr_correction = 0.75;
  }

  
  
  if (cpi->rolling_target_bits > 0 &&
      cpi->active_worst_quality < cpi->worst_quality) {
    double rolling_ratio = (double)cpi->rolling_actual_bits /
                               (double)cpi->rolling_target_bits;

    if (rolling_ratio < 0.95)
      cpi->twopass.est_max_qcorrection_factor -= 0.005;
    else if (rolling_ratio > 1.05)
      cpi->twopass.est_max_qcorrection_factor += 0.005;

    cpi->twopass.est_max_qcorrection_factor = fclamp(
        cpi->twopass.est_max_qcorrection_factor, 0.1, 10.0);
  }

  
  
  
  
  if (cpi->compressor_speed == 1)
    speed_correction = cpi->oxcf.cpu_used <= 5 ?
                          1.04 + (0 * 0.04) :
                          1.25;

  
  
  for (q = cpi->twopass.maxq_min_limit; q < cpi->twopass.maxq_max_limit; q++) {
    int bits_per_mb_at_this_q;

    err_correction_factor = calc_correction_factor(err_per_mb,
                                                   ERR_DIVISOR, 0.4, 0.90, q) *
                                sr_correction * speed_correction *
                                cpi->twopass.est_max_qcorrection_factor;

    bits_per_mb_at_this_q = vp9_bits_per_mb(INTER_FRAME, q,
                                            err_correction_factor);

    if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
      break;
  }

  
  if (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY &&
      q < cpi->cq_target_quality)
    q = cpi->cq_target_quality;

  
  
  
  
  if (cpi->ni_frames > ((int)cpi->twopass.total_stats.count >> 8) &&
      cpi->ni_frames > 25)
    adjust_maxq_qrange(cpi);

  return q;
}



static int estimate_cq(VP9_COMP *cpi,
                       FIRSTPASS_STATS *fpstats,
                       int section_target_bandwitdh) {
  int q;
  int num_mbs = cpi->common.MBs;
  int target_norm_bits_per_mb;

  double section_err = (fpstats->coded_error / fpstats->count);
  double err_per_mb = section_err / num_mbs;
  double err_correction_factor;
  double sr_err_diff;
  double sr_correction;
  double speed_correction = 1.0;
  double clip_iiratio;
  double clip_iifactor;

  target_norm_bits_per_mb = (section_target_bandwitdh < (1 << 20))
                            ? (512 * section_target_bandwitdh) / num_mbs
                            : 512 * (section_target_bandwitdh / num_mbs);


  
  
  if (cpi->compressor_speed == 1) {
    if (cpi->oxcf.cpu_used <= 5)
      speed_correction = 1.04 + ( 0 * 0.04);
    else
      speed_correction = 1.25;
  }

  
  
  if (fpstats->sr_coded_error > fpstats->coded_error) {
    sr_err_diff =
      (fpstats->sr_coded_error - fpstats->coded_error) /
      (fpstats->count * cpi->common.MBs);
    sr_correction = (sr_err_diff / 32.0);
    sr_correction = pow(sr_correction, 0.25);
    if (sr_correction < 0.75)
      sr_correction = 0.75;
    else if (sr_correction > 1.25)
      sr_correction = 1.25;
  } else {
    sr_correction = 0.75;
  }

  
  clip_iiratio = cpi->twopass.total_stats.intra_error /
                 DOUBLE_DIVIDE_CHECK(cpi->twopass.total_stats.coded_error);
  clip_iifactor = 1.0 - ((clip_iiratio - 10.0) * 0.025);
  if (clip_iifactor < 0.80)
    clip_iifactor = 0.80;

  
  for (q = 0; q < MAXQ; q++) {
    int bits_per_mb_at_this_q;

    
    err_correction_factor =
      calc_correction_factor(err_per_mb, 100.0, 0.4, 0.90, q) *
      sr_correction * speed_correction * clip_iifactor;

    bits_per_mb_at_this_q =
      vp9_bits_per_mb(INTER_FRAME, q, err_correction_factor);

    if (bits_per_mb_at_this_q <= target_norm_bits_per_mb)
      break;
  }

  
  q = select_cq_level(q);
  if (q >= cpi->worst_quality)
    q = cpi->worst_quality - 1;
  if (q < cpi->best_quality)
    q = cpi->best_quality;

  return q;
}

extern void vp9_new_framerate(VP9_COMP *cpi, double framerate);

void vp9_init_second_pass(VP9_COMP *cpi) {
  FIRSTPASS_STATS this_frame;
  FIRSTPASS_STATS *start_pos;

  double lower_bounds_min_rate = FRAME_OVERHEAD_BITS * cpi->oxcf.framerate;
  double two_pass_min_rate = (double)(cpi->oxcf.target_bandwidth *
                                      cpi->oxcf.two_pass_vbrmin_section / 100);

  if (two_pass_min_rate < lower_bounds_min_rate)
    two_pass_min_rate = lower_bounds_min_rate;

  zero_stats(&cpi->twopass.total_stats);
  zero_stats(&cpi->twopass.total_left_stats);

  if (!cpi->twopass.stats_in_end)
    return;

  cpi->twopass.total_stats = *cpi->twopass.stats_in_end;
  cpi->twopass.total_left_stats = cpi->twopass.total_stats;

  
  
  
  
  
  vp9_new_framerate(cpi, 10000000.0 * cpi->twopass.total_stats.count /
                       cpi->twopass.total_stats.duration);

  cpi->output_framerate = cpi->oxcf.framerate;
  cpi->twopass.bits_left = (int64_t)(cpi->twopass.total_stats.duration *
                                     cpi->oxcf.target_bandwidth / 10000000.0);
  cpi->twopass.bits_left -= (int64_t)(cpi->twopass.total_stats.duration *
                                      two_pass_min_rate / 10000000.0);

  
  
  
  
  cpi->twopass.kf_intra_err_min = KF_MB_INTRA_MIN * cpi->common.MBs;
  cpi->twopass.gf_intra_err_min = GF_MB_INTRA_MIN * cpi->common.MBs;

  
  cpi->twopass.sr_update_lag = 1;

  
  
  {
    double sum_iiratio = 0.0;
    double IIRatio;

    start_pos = cpi->twopass.stats_in;  

    while (input_stats(cpi, &this_frame) != EOF) {
      IIRatio = this_frame.intra_error
                / DOUBLE_DIVIDE_CHECK(this_frame.coded_error);
      IIRatio = (IIRatio < 1.0) ? 1.0 : (IIRatio > 20.0) ? 20.0 : IIRatio;
      sum_iiratio += IIRatio;
    }

    cpi->twopass.avg_iiratio = sum_iiratio /
        DOUBLE_DIVIDE_CHECK((double)cpi->twopass.total_stats.count);

    
    reset_fpf_position(cpi, start_pos);
  }

  
  
  {
    start_pos = cpi->twopass.stats_in;  

    cpi->twopass.modified_error_total = 0.0;
    cpi->twopass.modified_error_used = 0.0;

    while (input_stats(cpi, &this_frame) != EOF) {
      cpi->twopass.modified_error_total +=
          calculate_modified_err(cpi, &this_frame);
    }
    cpi->twopass.modified_error_left = cpi->twopass.modified_error_total;

    reset_fpf_position(cpi, start_pos);  
  }
}

void vp9_end_second_pass(VP9_COMP *cpi) {
}



static double get_prediction_decay_rate(VP9_COMP *cpi,
                                        FIRSTPASS_STATS *next_frame) {
  double prediction_decay_rate;
  double second_ref_decay;
  double mb_sr_err_diff;

  
  prediction_decay_rate = next_frame->pcnt_inter;

  
  
  mb_sr_err_diff = (next_frame->sr_coded_error - next_frame->coded_error) /
                   cpi->common.MBs;
  if (mb_sr_err_diff <= 512.0) {
    second_ref_decay = 1.0 - (mb_sr_err_diff / 512.0);
    second_ref_decay = pow(second_ref_decay, 0.5);
    if (second_ref_decay < 0.85)
      second_ref_decay = 0.85;
    else if (second_ref_decay > 1.0)
      second_ref_decay = 1.0;
  } else {
    second_ref_decay = 0.85;
  }

  if (second_ref_decay < prediction_decay_rate)
    prediction_decay_rate = second_ref_decay;

  return prediction_decay_rate;
}




static int detect_transition_to_still(
  VP9_COMP *cpi,
  int frame_interval,
  int still_interval,
  double loop_decay_rate,
  double last_decay_rate) {
  int trans_to_still = 0;

  
  
  
  if (frame_interval > MIN_GF_INTERVAL &&
      loop_decay_rate >= 0.999 &&
      last_decay_rate < 0.9) {
    int j;
    FIRSTPASS_STATS *position = cpi->twopass.stats_in;
    FIRSTPASS_STATS tmp_next_frame;
    double zz_inter;

    
    
    for (j = 0; j < still_interval; j++) {
      if (EOF == input_stats(cpi, &tmp_next_frame))
        break;

      zz_inter =
        (tmp_next_frame.pcnt_inter - tmp_next_frame.pcnt_motion);
      if (zz_inter < 0.999)
        break;
    }
    
    reset_fpf_position(cpi, position);

    
    if (j == still_interval)
      trans_to_still = 1;
  }

  return trans_to_still;
}




static int detect_flash(VP9_COMP *cpi, int offset) {
  FIRSTPASS_STATS next_frame;

  int flash_detected = 0;

  
  
  if (read_frame_stats(cpi, &next_frame, offset) != EOF) {
    
    
    
    
    
    if (next_frame.pcnt_second_ref > next_frame.pcnt_inter &&
        next_frame.pcnt_second_ref >= 0.5)
      flash_detected = 1;
  }

  return flash_detected;
}


static void accumulate_frame_motion_stats(
  FIRSTPASS_STATS *this_frame,
  double *this_frame_mv_in_out,
  double *mv_in_out_accumulator,
  double *abs_mv_in_out_accumulator,
  double *mv_ratio_accumulator) {
  
  double this_frame_mvr_ratio;
  double this_frame_mvc_ratio;
  double motion_pct;

  
  motion_pct = this_frame->pcnt_motion;

  
  *this_frame_mv_in_out = this_frame->mv_in_out_count * motion_pct;
  *mv_in_out_accumulator += this_frame->mv_in_out_count * motion_pct;
  *abs_mv_in_out_accumulator +=
    fabs(this_frame->mv_in_out_count * motion_pct);

  
  
  if (motion_pct > 0.05) {
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
  VP9_COMP *cpi,
  FIRSTPASS_STATS *this_frame,
  double this_frame_mv_in_out) {
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

static int calc_arf_boost(VP9_COMP *cpi, int offset,
                          int f_frames, int b_frames,
                          int *f_boost, int *b_boost) {
  FIRSTPASS_STATS this_frame;

  int i;
  double boost_score = 0.0;
  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;
  int arf_boost;
  int flash_detected = 0;

  
  for (i = 0; i < f_frames; i++) {
    if (read_frame_stats(cpi, &this_frame, (i + offset)) == EOF)
      break;

    
    accumulate_frame_motion_stats(&this_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator,
                                  &mv_ratio_accumulator);

    
    
    flash_detected = detect_flash(cpi, (i + offset)) ||
                     detect_flash(cpi, (i + offset + 1));

    
    if (!flash_detected) {
      decay_accumulator *= get_prediction_decay_rate(cpi, &this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                          ? MIN_DECAY_FACTOR : decay_accumulator;
    }

    boost_score += (decay_accumulator *
                    calc_frame_boost(cpi, &this_frame, this_frame_mv_in_out));
  }

  *f_boost = (int)boost_score;

  
  boost_score = 0.0;
  mv_ratio_accumulator = 0.0;
  decay_accumulator = 1.0;
  this_frame_mv_in_out = 0.0;
  mv_in_out_accumulator = 0.0;
  abs_mv_in_out_accumulator = 0.0;

  
  for (i = -1; i >= -b_frames; i--) {
    if (read_frame_stats(cpi, &this_frame, (i + offset)) == EOF)
      break;

    
    accumulate_frame_motion_stats(&this_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator,
                                  &mv_ratio_accumulator);

    
    
    flash_detected = detect_flash(cpi, (i + offset)) ||
                     detect_flash(cpi, (i + offset + 1));

    
    if (!flash_detected) {
      decay_accumulator *= get_prediction_decay_rate(cpi, &this_frame);
      decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                              ? MIN_DECAY_FACTOR : decay_accumulator;
    }

    boost_score += (decay_accumulator *
                    calc_frame_boost(cpi, &this_frame, this_frame_mv_in_out));
  }
  *b_boost = (int)boost_score;

  arf_boost = (*f_boost + *b_boost);
  if (arf_boost < ((b_frames + f_frames) * 20))
    arf_boost = ((b_frames + f_frames) * 20);

  return arf_boost;
}

#if CONFIG_MULTIPLE_ARF





static void schedule_frames(VP9_COMP *cpi, const int start, const int end,
                            const int arf_idx, const int gf_or_arf_group,
                            const int level) {
  int i, abs_end, half_range;
  int *cfo = cpi->frame_coding_order;
  int idx = cpi->new_frame_coding_order_period;

  
  assert(start >= 0);

  

  
  if (gf_or_arf_group == 0) {
    assert(end >= start);
    for (i = start; i <= end; ++i) {
      cfo[idx] = i;
      cpi->arf_buffer_idx[idx] = arf_idx;
      cpi->arf_weight[idx] = -1;
      ++idx;
    }
    cpi->new_frame_coding_order_period = idx;
    return;
  }

  
  
  if (end < 0) {
    
    
    cfo[idx] = end;
    
    cpi->arf_buffer_idx[idx] = (arf_idx > 2) ? (arf_idx - 1) : 2;
    cpi->arf_weight[idx] = level;
    ++idx;
    abs_end = -end;
  } else {
    abs_end = end;
  }

  half_range = (abs_end - start) >> 1;

  
  
  if ((start + MIN_GF_INTERVAL) >= (abs_end - MIN_GF_INTERVAL)) {
    
    
    for (i = start; i <= abs_end; ++i) {
      cfo[idx] = i;
      cpi->arf_buffer_idx[idx] = arf_idx;
      cpi->arf_weight[idx] = -1;
      ++idx;
    }
    cpi->new_frame_coding_order_period = idx;
  } else {
    
    cpi->new_frame_coding_order_period = idx;
    schedule_frames(cpi, start, -(start + half_range), arf_idx + 1,
                    gf_or_arf_group, level + 1);
    schedule_frames(cpi, start + half_range + 1, abs_end, arf_idx,
                    gf_or_arf_group, level + 1);
  }
}

#define FIXED_ARF_GROUP_SIZE 16

void define_fixed_arf_period(VP9_COMP *cpi) {
  int i;
  int max_level = INT_MIN;

  assert(cpi->multi_arf_enabled);
  assert(cpi->oxcf.lag_in_frames >= FIXED_ARF_GROUP_SIZE);

  
  
  cpi->this_frame_weight = cpi->arf_weight[cpi->sequence_number];
  assert(cpi->this_frame_weight >= 0);

  
  cpi->new_frame_coding_order_period = 0;
  cpi->next_frame_in_order = 0;
  cpi->arf_buffered = 0;
  vp9_zero(cpi->frame_coding_order);
  vp9_zero(cpi->arf_buffer_idx);
  vpx_memset(cpi->arf_weight, -1, sizeof(cpi->arf_weight));

  if (cpi->twopass.frames_to_key <= (FIXED_ARF_GROUP_SIZE + 8)) {
    
    cpi->source_alt_ref_pending = 0;
    cpi->baseline_gf_interval = cpi->twopass.frames_to_key;
    schedule_frames(cpi, 0, (cpi->baseline_gf_interval - 1), 2, 0, 0);
  } else {
    
    cpi->source_alt_ref_pending = 1;
    cpi->baseline_gf_interval = FIXED_ARF_GROUP_SIZE;
    schedule_frames(cpi, 0, -(cpi->baseline_gf_interval - 1), 2, 1, 0);
  }

  
  for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
    if (cpi->arf_weight[i] > max_level) {
      max_level = cpi->arf_weight[i];
    }
  }
  ++max_level;
  for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
    if (cpi->arf_weight[i] == -1) {
      cpi->arf_weight[i] = max_level;
    }
  }
  cpi->max_arf_level = max_level;
#if 0
  printf("\nSchedule: ");
  for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
    printf("%4d ", cpi->frame_coding_order[i]);
  }
  printf("\n");
  printf("ARFref:   ");
  for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
    printf("%4d ", cpi->arf_buffer_idx[i]);
  }
  printf("\n");
  printf("Weight:   ");
  for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
    printf("%4d ", cpi->arf_weight[i]);
  }
  printf("\n");
#endif
}
#endif


static void define_gf_group(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  FIRSTPASS_STATS next_frame = { 0 };
  FIRSTPASS_STATS *start_pos;
  int i;
  double boost_score = 0.0;
  double old_boost_score = 0.0;
  double gf_group_err = 0.0;
  double gf_first_frame_err = 0.0;
  double mod_frame_err = 0.0;

  double mv_ratio_accumulator = 0.0;
  double decay_accumulator = 1.0;
  double zero_motion_accumulator = 1.0;

  double loop_decay_rate = 1.00;          
  double last_loop_decay_rate = 1.00;

  double this_frame_mv_in_out = 0.0;
  double mv_in_out_accumulator = 0.0;
  double abs_mv_in_out_accumulator = 0.0;
  double mv_ratio_accumulator_thresh;
  int max_bits = frame_max_bits(cpi);     

  unsigned int allow_alt_ref =
    cpi->oxcf.play_alternate && cpi->oxcf.lag_in_frames;

  int f_boost = 0;
  int b_boost = 0;
  int flash_detected;
  int active_max_gf_interval;

  cpi->twopass.gf_group_bits = 0;

  vp9_clear_system_state();  

  start_pos = cpi->twopass.stats_in;

  
  mod_frame_err = calculate_modified_err(cpi, this_frame);

  
  
  gf_first_frame_err = mod_frame_err;

  
  
  
  if (cpi->common.frame_type == KEY_FRAME)
    gf_group_err -= gf_first_frame_err;

  
  mv_ratio_accumulator_thresh = (cpi->common.width + cpi->common.height) / 10.0;

  
  
  
  
  
  
  active_max_gf_interval =
    12 + ((int)vp9_convert_qindex_to_q(cpi->active_worst_quality) >> 5);

  if (active_max_gf_interval > cpi->max_gf_interval)
    active_max_gf_interval = cpi->max_gf_interval;

  i = 0;
  while (((i < cpi->twopass.static_scene_max_gf_interval) ||
          ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL)) &&
         (i < cpi->twopass.frames_to_key)) {
    i++;    

    
    mod_frame_err = calculate_modified_err(cpi, this_frame);
    gf_group_err += mod_frame_err;

    if (EOF == input_stats(cpi, &next_frame))
      break;

    
    
    flash_detected = detect_flash(cpi, 0);

    
    accumulate_frame_motion_stats(&next_frame,
                                  &this_frame_mv_in_out, &mv_in_out_accumulator,
                                  &abs_mv_in_out_accumulator,
                                  &mv_ratio_accumulator);

    
    if (!flash_detected) {
      last_loop_decay_rate = loop_decay_rate;
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);
      decay_accumulator = decay_accumulator * loop_decay_rate;

      
      if ((next_frame.pcnt_inter - next_frame.pcnt_motion) <
          zero_motion_accumulator) {
        zero_motion_accumulator =
          (next_frame.pcnt_inter - next_frame.pcnt_motion);
      }

      
      
      if (detect_transition_to_still(cpi, i, 5, loop_decay_rate,
                                     last_loop_decay_rate)) {
        allow_alt_ref = 0;
        break;
      }
    }

    
    boost_score +=
      (decay_accumulator *
       calc_frame_boost(cpi, &next_frame, this_frame_mv_in_out));

    
    if (
      
      (i >= active_max_gf_interval && (zero_motion_accumulator < 0.995)) ||
      (
        
        (i > MIN_GF_INTERVAL) &&
        
        ((cpi->twopass.frames_to_key - i) >= MIN_GF_INTERVAL) &&
        ((boost_score > 125.0) || (next_frame.pcnt_inter < 0.75)) &&
        (!flash_detected) &&
        ((mv_ratio_accumulator > mv_ratio_accumulator_thresh) ||
         (abs_mv_in_out_accumulator > 3.0) ||
         (mv_in_out_accumulator < -2.0) ||
         ((boost_score - old_boost_score) < IIFACTOR)))) {
      boost_score = old_boost_score;
      break;
    }

    *this_frame = next_frame;

    old_boost_score = boost_score;
  }

  cpi->gf_zeromotion_pct = (int)(zero_motion_accumulator * 1000.0);

  
  if ((cpi->twopass.frames_to_key - i) < MIN_GF_INTERVAL) {
    while (i < cpi->twopass.frames_to_key) {
      i++;

      if (EOF == input_stats(cpi, this_frame))
        break;

      if (i < cpi->twopass.frames_to_key) {
        mod_frame_err = calculate_modified_err(cpi, this_frame);
        gf_group_err += mod_frame_err;
      }
    }
  }

  
  cpi->baseline_gf_interval = i;

#if CONFIG_MULTIPLE_ARF
  if (cpi->multi_arf_enabled) {
    
    cpi->new_frame_coding_order_period = 0;
    cpi->next_frame_in_order = 0;
    cpi->arf_buffered = 0;
    vp9_zero(cpi->frame_coding_order);
    vp9_zero(cpi->arf_buffer_idx);
    vpx_memset(cpi->arf_weight, -1, sizeof(cpi->arf_weight));
  }
#endif

  
  if (allow_alt_ref &&
      (i < cpi->oxcf.lag_in_frames) &&
      (i >= MIN_GF_INTERVAL) &&
      
      (i <= (cpi->twopass.frames_to_key - MIN_GF_INTERVAL)) &&
      ((next_frame.pcnt_inter > 0.75) ||
       (next_frame.pcnt_second_ref > 0.5)) &&
      ((mv_in_out_accumulator / (double)i > -0.2) ||
       (mv_in_out_accumulator > -2.0)) &&
      (boost_score > 100)) {
    
    cpi->gfu_boost = calc_arf_boost(cpi, 0, (i - 1), (i - 1), &f_boost,
                                    &b_boost);
    cpi->source_alt_ref_pending = 1;

#if CONFIG_MULTIPLE_ARF
    
    if (cpi->multi_arf_enabled) {
      schedule_frames(cpi, 0, -(cpi->baseline_gf_interval - 1), 2, 1, 0);
    }
#endif
  } else {
    cpi->gfu_boost = (int)boost_score;
    cpi->source_alt_ref_pending = 0;
#if CONFIG_MULTIPLE_ARF
    
    if (cpi->multi_arf_enabled) {
      schedule_frames(cpi, 0, cpi->baseline_gf_interval - 1, 2, 0, 0);
      assert(cpi->new_frame_coding_order_period == cpi->baseline_gf_interval);
    }
#endif
  }

#if CONFIG_MULTIPLE_ARF
  if (cpi->multi_arf_enabled && (cpi->common.frame_type != KEY_FRAME)) {
    int max_level = INT_MIN;
    
    for (i = 0; i < cpi->frame_coding_order_period; ++i) {
      if (cpi->arf_weight[i] > max_level) {
        max_level = cpi->arf_weight[i];
      }
    }
    ++max_level;
    for (i = 0; i < cpi->frame_coding_order_period; ++i) {
      if (cpi->arf_weight[i] == -1) {
        cpi->arf_weight[i] = max_level;
      }
    }
    cpi->max_arf_level = max_level;
  }
#if 0
  if (cpi->multi_arf_enabled) {
    printf("\nSchedule: ");
    for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
      printf("%4d ", cpi->frame_coding_order[i]);
    }
    printf("\n");
    printf("ARFref:   ");
    for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
      printf("%4d ", cpi->arf_buffer_idx[i]);
    }
    printf("\n");
    printf("Weight:   ");
    for (i = 0; i < cpi->new_frame_coding_order_period; ++i) {
      printf("%4d ", cpi->arf_weight[i]);
    }
    printf("\n");
  }
#endif
#endif

  
  
  
  
  
  
  if (cpi->twopass.frames_to_key >= (int)(cpi->twopass.total_stats.count -
                                          cpi->common.current_video_frame)) {
    cpi->twopass.kf_group_bits =
      (cpi->twopass.bits_left > 0) ? cpi->twopass.bits_left : 0;
  }

  
  if ((cpi->twopass.kf_group_bits > 0) &&
      (cpi->twopass.kf_group_error_left > 0)) {
    cpi->twopass.gf_group_bits =
      (int64_t)(cpi->twopass.kf_group_bits *
                (gf_group_err / cpi->twopass.kf_group_error_left));
  } else {
    cpi->twopass.gf_group_bits = 0;
  }
  cpi->twopass.gf_group_bits =
    (cpi->twopass.gf_group_bits < 0)
    ? 0
    : (cpi->twopass.gf_group_bits > cpi->twopass.kf_group_bits)
    ? cpi->twopass.kf_group_bits : cpi->twopass.gf_group_bits;

  
  
  if (cpi->twopass.gf_group_bits >
      (int64_t)max_bits * cpi->baseline_gf_interval)
    cpi->twopass.gf_group_bits = (int64_t)max_bits * cpi->baseline_gf_interval;

  
  reset_fpf_position(cpi, start_pos);

  
  cpi->twopass.modified_error_used += gf_group_err;

  
  for (i = 0;
      i <= (cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME);
      ++i) {
    int allocation_chunks;
    int q = cpi->oxcf.fixed_q < 0 ? cpi->last_q[INTER_FRAME]
                                  : cpi->oxcf.fixed_q;
    int gf_bits;

    int boost = (cpi->gfu_boost * vp9_gfboost_qadjust(q)) / 100;

    
    boost = clamp(boost, 125, (cpi->baseline_gf_interval + 1) * 200);

    if (cpi->source_alt_ref_pending && i == 0)
      allocation_chunks = ((cpi->baseline_gf_interval + 1) * 100) + boost;
    else
      allocation_chunks = (cpi->baseline_gf_interval * 100) + (boost - 100);

    
    if (boost > 1023) {
      int divisor = boost >> 10;
      boost /= divisor;
      allocation_chunks /= divisor;
    }

    
    
    gf_bits = (int)((double)boost * (cpi->twopass.gf_group_bits /
                                       (double)allocation_chunks));

    
    
    
    if (mod_frame_err < gf_group_err / (double)cpi->baseline_gf_interval) {
      double alt_gf_grp_bits =
        (double)cpi->twopass.kf_group_bits  *
        (mod_frame_err * (double)cpi->baseline_gf_interval) /
        DOUBLE_DIVIDE_CHECK(cpi->twopass.kf_group_error_left);

      int alt_gf_bits = (int)((double)boost * (alt_gf_grp_bits /
                                           (double)allocation_chunks));

      if (gf_bits > alt_gf_bits)
        gf_bits = alt_gf_bits;
    } else {
      
      
      
      int alt_gf_bits = (int)((double)cpi->twopass.kf_group_bits *
                        mod_frame_err /
                        DOUBLE_DIVIDE_CHECK(cpi->twopass.kf_group_error_left));

      if (alt_gf_bits > gf_bits)
        gf_bits = alt_gf_bits;
    }

    
    if (gf_bits < 0)
      gf_bits = 0;

    
    gf_bits += cpi->min_frame_bandwidth;

    if (i == 0) {
      cpi->twopass.gf_bits = gf_bits;
    }
    if (i == 1 || (!cpi->source_alt_ref_pending
        && (cpi->common.frame_type != KEY_FRAME))) {
      
      cpi->per_frame_bandwidth = gf_bits;
    }
  }

  {
    
    cpi->twopass.kf_group_error_left -= (int64_t)gf_group_err;
    cpi->twopass.kf_group_bits -= cpi->twopass.gf_group_bits;

    if (cpi->twopass.kf_group_bits < 0)
      cpi->twopass.kf_group_bits = 0;

    
    
    
    
    if (!cpi->source_alt_ref_pending && cpi->common.frame_type != KEY_FRAME)
      cpi->twopass.gf_group_error_left = (int64_t)(gf_group_err
                                                   - gf_first_frame_err);
    else
      cpi->twopass.gf_group_error_left = (int64_t)gf_group_err;

    cpi->twopass.gf_group_bits -= cpi->twopass.gf_bits
        - cpi->min_frame_bandwidth;

    if (cpi->twopass.gf_group_bits < 0)
      cpi->twopass.gf_group_bits = 0;

    
    
    
    if (cpi->baseline_gf_interval >= 3) {
      const int boost = cpi->source_alt_ref_pending ? b_boost : cpi->gfu_boost;

      if (boost >= 150) {
        int alt_extra_bits;
        int pct_extra = (boost - 100) / 50;
        pct_extra = (pct_extra > 20) ? 20 : pct_extra;

        alt_extra_bits = (int)((cpi->twopass.gf_group_bits * pct_extra) / 100);
        cpi->twopass.gf_group_bits -= alt_extra_bits;
      }
    }
  }

  if (cpi->common.frame_type != KEY_FRAME) {
    FIRSTPASS_STATS sectionstats;

    zero_stats(&sectionstats);
    reset_fpf_position(cpi, start_pos);

    for (i = 0; i < cpi->baseline_gf_interval; i++) {
      input_stats(cpi, &next_frame);
      accumulate_stats(&sectionstats, &next_frame);
    }

    avg_stats(&sectionstats);

    cpi->twopass.section_intra_rating = (int)
      (sectionstats.intra_error /
      DOUBLE_DIVIDE_CHECK(sectionstats.coded_error));

    reset_fpf_position(cpi, start_pos);
  }
}


static void assign_std_frame_bits(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  int target_frame_size;

  double modified_err;
  double err_fraction;

  
  int max_bits = frame_max_bits(cpi);

  
  modified_err = calculate_modified_err(cpi, this_frame);

  if (cpi->twopass.gf_group_error_left > 0)
    
    err_fraction = modified_err / cpi->twopass.gf_group_error_left;
  else
    err_fraction = 0.0;

  
  target_frame_size = (int)((double)cpi->twopass.gf_group_bits * err_fraction);

  
  
  if (target_frame_size < 0) {
    target_frame_size = 0;
  } else {
    if (target_frame_size > max_bits)
      target_frame_size = max_bits;

    if (target_frame_size > cpi->twopass.gf_group_bits)
      target_frame_size = (int)cpi->twopass.gf_group_bits;
  }

  
  cpi->twopass.gf_group_error_left -= (int64_t)modified_err;
  cpi->twopass.gf_group_bits -= target_frame_size;

  if (cpi->twopass.gf_group_bits < 0)
    cpi->twopass.gf_group_bits = 0;

  
  target_frame_size += cpi->min_frame_bandwidth;

  
  cpi->per_frame_bandwidth = target_frame_size;
}


static int adjust_active_maxq(int old_maxqi, int new_maxqi) {
  int i;
  const double old_q = vp9_convert_qindex_to_q(old_maxqi);
  const double new_q = vp9_convert_qindex_to_q(new_maxqi);
  const double target_q = ((old_q * 7.0) + new_q) / 8.0;

  if (target_q > old_q) {
    for (i = old_maxqi; i <= new_maxqi; i++)
      if (vp9_convert_qindex_to_q(i) >= target_q)
        return i;
  } else {
    for (i = old_maxqi; i >= new_maxqi; i--)
      if (vp9_convert_qindex_to_q(i) <= target_q)
        return i;
  }

  return new_maxqi;
}

void vp9_second_pass(VP9_COMP *cpi) {
  int tmp_q;
  int frames_left = (int)(cpi->twopass.total_stats.count -
                          cpi->common.current_video_frame);

  FIRSTPASS_STATS this_frame;
  FIRSTPASS_STATS this_frame_copy;

  double this_frame_intra_error;
  double this_frame_coded_error;

  if (!cpi->twopass.stats_in)
    return;

  vp9_clear_system_state();

  if (cpi->oxcf.end_usage == USAGE_CONSTANT_QUALITY) {
    cpi->active_worst_quality = cpi->oxcf.cq_level;
  } else {
    
    if (cpi->common.current_video_frame == 0) {
      int section_target_bandwidth =
          (int)(cpi->twopass.bits_left / frames_left);
      cpi->twopass.est_max_qcorrection_factor = 1.0;

      
      
      
      











      
      cpi->twopass.maxq_max_limit = cpi->worst_quality;
      cpi->twopass.maxq_min_limit = cpi->best_quality;

      tmp_q = estimate_max_q(cpi, &cpi->twopass.total_left_stats,
                             section_target_bandwidth);

      cpi->active_worst_quality = tmp_q;
      cpi->ni_av_qi = tmp_q;
      cpi->avg_q = vp9_convert_qindex_to_q(tmp_q);

      
      
      
      
      
      adjust_maxq_qrange(cpi);
    }

    
    
    
    
    else if ((cpi->common.current_video_frame <
              (((unsigned int)cpi->twopass.total_stats.count * 255) >> 8)) &&
             ((cpi->common.current_video_frame + cpi->baseline_gf_interval) <
              (unsigned int)cpi->twopass.total_stats.count)) {
      int section_target_bandwidth =
          (int)(cpi->twopass.bits_left / frames_left);
      if (frames_left < 1)
        frames_left = 1;

      tmp_q = estimate_max_q(
          cpi,
          &cpi->twopass.total_left_stats,
          section_target_bandwidth);

      
      cpi->active_worst_quality =
          adjust_active_maxq(cpi->active_worst_quality, tmp_q);
    }
  }
  vp9_zero(this_frame);
  if (EOF == input_stats(cpi, &this_frame))
    return;

  this_frame_intra_error = this_frame.intra_error;
  this_frame_coded_error = this_frame.coded_error;

  
  if (cpi->twopass.frames_to_key == 0) {
    
    this_frame_copy = this_frame;
    find_next_key_frame(cpi, &this_frame_copy);
  }

  
  if (cpi->frames_till_gf_update_due == 0) {
    
    this_frame_copy = this_frame;

    cpi->gf_zeromotion_pct = 0;

#if CONFIG_MULTIPLE_ARF
    if (cpi->multi_arf_enabled) {
      define_fixed_arf_period(cpi);
    } else {
#endif
      define_gf_group(cpi, &this_frame_copy);
#if CONFIG_MULTIPLE_ARF
    }
#endif

    if (cpi->gf_zeromotion_pct > 995) {
      
      
      if (!cpi->common.show_frame)
        cpi->enable_encode_breakout = 0;
      else
        cpi->enable_encode_breakout = 2;
    }

    
    
    
    
    
    
    if (cpi->source_alt_ref_pending && (cpi->common.frame_type != KEY_FRAME)) {
      
      
      int bak = cpi->per_frame_bandwidth;
      this_frame_copy = this_frame;
      assign_std_frame_bits(cpi, &this_frame_copy);
      cpi->per_frame_bandwidth = bak;
    }
  } else {
    
    
    this_frame_copy =  this_frame;
    assign_std_frame_bits(cpi, &this_frame_copy);
  }

  
  cpi->twopass.this_iiratio = (int)(this_frame_intra_error /
                              DOUBLE_DIVIDE_CHECK(this_frame_coded_error));
  {
    FIRSTPASS_STATS next_frame;
    if (lookup_next_frame_stats(cpi, &next_frame) != EOF) {
      cpi->twopass.next_iiratio = (int)(next_frame.intra_error /
                                  DOUBLE_DIVIDE_CHECK(next_frame.coded_error));
    }
  }

  
  cpi->target_bandwidth = (int)(cpi->per_frame_bandwidth
                                * cpi->output_framerate);
  if (cpi->target_bandwidth < 0)
    cpi->target_bandwidth = 0;

  cpi->twopass.frames_to_key--;

  
  subtract_stats(&cpi->twopass.total_left_stats, &this_frame);
}

static int test_candidate_kf(VP9_COMP *cpi,
                             FIRSTPASS_STATS *last_frame,
                             FIRSTPASS_STATS *this_frame,
                             FIRSTPASS_STATS *next_frame) {
  int is_viable_kf = 0;

  
  
  if ((this_frame->pcnt_second_ref < 0.10) &&
      (next_frame->pcnt_second_ref < 0.10) &&
      ((this_frame->pcnt_inter < 0.05) ||
       (((this_frame->pcnt_inter - this_frame->pcnt_neutral) < .35) &&
        ((this_frame->intra_error /
          DOUBLE_DIVIDE_CHECK(this_frame->coded_error)) < 2.5) &&
        ((fabs(last_frame->coded_error - this_frame->coded_error) /
              DOUBLE_DIVIDE_CHECK(this_frame->coded_error) >
          .40) ||
         (fabs(last_frame->intra_error - this_frame->intra_error) /
              DOUBLE_DIVIDE_CHECK(this_frame->intra_error) >
          .40) ||
         ((next_frame->intra_error /
           DOUBLE_DIVIDE_CHECK(next_frame->coded_error)) > 3.5))))) {
    int i;
    FIRSTPASS_STATS *start_pos;

    FIRSTPASS_STATS local_next_frame;

    double boost_score = 0.0;
    double old_boost_score = 0.0;
    double decay_accumulator = 1.0;
    double next_iiratio;

    local_next_frame = *next_frame;

    
    start_pos = cpi->twopass.stats_in;

    
    for (i = 0; i < 16; i++) {
      next_iiratio = (IIKFACTOR1 * local_next_frame.intra_error /
                      DOUBLE_DIVIDE_CHECK(local_next_frame.coded_error));

      if (next_iiratio > RMAX)
        next_iiratio = RMAX;

      
      if (local_next_frame.pcnt_inter > 0.85)
        decay_accumulator = decay_accumulator * local_next_frame.pcnt_inter;
      else
        decay_accumulator =
            decay_accumulator * ((0.85 + local_next_frame.pcnt_inter) / 2.0);

      

      
      boost_score += (decay_accumulator * next_iiratio);

      
      if ((local_next_frame.pcnt_inter < 0.05) ||
          (next_iiratio < 1.5) ||
          (((local_next_frame.pcnt_inter -
             local_next_frame.pcnt_neutral) < 0.20) &&
           (next_iiratio < 3.0)) ||
          ((boost_score - old_boost_score) < 3.0) ||
          (local_next_frame.intra_error < 200)
         ) {
        break;
      }

      old_boost_score = boost_score;

      
      if (EOF == input_stats(cpi, &local_next_frame))
        break;
    }

    
    
    if (boost_score > 30.0 && (i > 3)) {
      is_viable_kf = 1;
    } else {
      
      reset_fpf_position(cpi, start_pos);

      is_viable_kf = 0;
    }
  }

  return is_viable_kf;
}
static void find_next_key_frame(VP9_COMP *cpi, FIRSTPASS_STATS *this_frame) {
  int i, j;
  FIRSTPASS_STATS last_frame;
  FIRSTPASS_STATS first_frame;
  FIRSTPASS_STATS next_frame;
  FIRSTPASS_STATS *start_position;

  double decay_accumulator = 1.0;
  double zero_motion_accumulator = 1.0;
  double boost_score = 0;
  double loop_decay_rate;

  double kf_mod_err = 0.0;
  double kf_group_err = 0.0;
  double kf_group_intra_err = 0.0;
  double kf_group_coded_err = 0.0;
  double recent_loop_decay[8] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

  vp9_zero(next_frame);

  vp9_clear_system_state();  
  start_position = cpi->twopass.stats_in;

  cpi->common.frame_type = KEY_FRAME;

  
  cpi->this_key_frame_forced = cpi->next_key_frame_forced;

  
  cpi->source_alt_ref_active = 0;

  
  cpi->frames_till_gf_update_due = 0;

  cpi->twopass.frames_to_key = 1;

  
  first_frame = *this_frame;

  cpi->twopass.kf_group_bits = 0;        
  cpi->twopass.kf_group_error_left = 0;  

  kf_mod_err = calculate_modified_err(cpi, this_frame);

  
  i = 0;
  while (cpi->twopass.stats_in < cpi->twopass.stats_in_end) {
    
    kf_group_err += calculate_modified_err(cpi, this_frame);

    
    
    
    kf_group_intra_err += this_frame->intra_error;
    kf_group_coded_err += this_frame->coded_error;

    
    last_frame = *this_frame;
    input_stats(cpi, this_frame);

    
    if (cpi->oxcf.auto_key
        && lookup_next_frame_stats(cpi, &next_frame) != EOF) {
      
      if (test_candidate_kf(cpi, &last_frame, this_frame, &next_frame))
        break;


      
      loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);

      
      
      
      recent_loop_decay[i % 8] = loop_decay_rate;
      decay_accumulator = 1.0;
      for (j = 0; j < 8; j++)
        decay_accumulator *= recent_loop_decay[j];

      
      
      if (detect_transition_to_still(cpi, i, cpi->key_frame_frequency - i,
                                     loop_decay_rate, decay_accumulator))
        break;

      
      cpi->twopass.frames_to_key++;

      
      
      if (cpi->twopass.frames_to_key >= 2 * (int)cpi->key_frame_frequency)
        break;
    } else {
      cpi->twopass.frames_to_key++;
    }
    i++;
  }

  
  
  
  
  if (cpi->oxcf.auto_key
      && cpi->twopass.frames_to_key > (int)cpi->key_frame_frequency) {
    FIRSTPASS_STATS *current_pos = cpi->twopass.stats_in;
    FIRSTPASS_STATS tmp_frame;

    cpi->twopass.frames_to_key /= 2;

    
    tmp_frame = first_frame;

    
    reset_fpf_position(cpi, start_position);

    kf_group_err = 0;
    kf_group_intra_err = 0;
    kf_group_coded_err = 0;

    
    for (i = 0; i < cpi->twopass.frames_to_key; i++) {
      
      kf_group_err += calculate_modified_err(cpi, &tmp_frame);
      kf_group_intra_err += tmp_frame.intra_error;
      kf_group_coded_err += tmp_frame.coded_error;

      
      input_stats(cpi, &tmp_frame);
    }

    
    reset_fpf_position(cpi, current_pos);

    cpi->next_key_frame_forced = 1;
  } else {
    cpi->next_key_frame_forced = 0;
  }
  
  if (cpi->twopass.stats_in >= cpi->twopass.stats_in_end) {
    
    kf_group_err += calculate_modified_err(cpi, this_frame);

    
    
    
    kf_group_intra_err += this_frame->intra_error;
    kf_group_coded_err += this_frame->coded_error;
  }

  
  if ((cpi->twopass.bits_left > 0) &&
      (cpi->twopass.modified_error_left > 0.0)) {
    
    int max_bits = frame_max_bits(cpi);

    
    int64_t max_grp_bits;

    
    
    cpi->twopass.kf_group_bits = (int64_t)(cpi->twopass.bits_left *
                                           (kf_group_err /
                                            cpi->twopass.modified_error_left));

    
    max_grp_bits = (int64_t)max_bits * (int64_t)cpi->twopass.frames_to_key;
    if (cpi->twopass.kf_group_bits > max_grp_bits)
      cpi->twopass.kf_group_bits = max_grp_bits;
  } else {
    cpi->twopass.kf_group_bits = 0;
  }
  
  reset_fpf_position(cpi, start_position);

  
  
  decay_accumulator = 1.0;
  boost_score = 0.0;
  loop_decay_rate = 1.00;       

  
  for (i = 0; i < cpi->twopass.frames_to_key; i++) {
    double r;

    if (EOF == input_stats(cpi, &next_frame))
      break;

    
    if ((next_frame.pcnt_inter - next_frame.pcnt_motion) <
        zero_motion_accumulator) {
      zero_motion_accumulator =
        (next_frame.pcnt_inter - next_frame.pcnt_motion);
    }

    
    if (i <= (cpi->max_gf_interval * 2)) {
      if (next_frame.intra_error > cpi->twopass.kf_intra_err_min)
        r = (IIKFACTOR2 * next_frame.intra_error /
             DOUBLE_DIVIDE_CHECK(next_frame.coded_error));
      else
        r = (IIKFACTOR2 * cpi->twopass.kf_intra_err_min /
             DOUBLE_DIVIDE_CHECK(next_frame.coded_error));

      if (r > RMAX)
        r = RMAX;

      
      if (!detect_flash(cpi, 0)) {
        loop_decay_rate = get_prediction_decay_rate(cpi, &next_frame);
        decay_accumulator = decay_accumulator * loop_decay_rate;
        decay_accumulator = decay_accumulator < MIN_DECAY_FACTOR
                              ? MIN_DECAY_FACTOR : decay_accumulator;
      }

      boost_score += (decay_accumulator * r);
    }
  }

  {
    FIRSTPASS_STATS sectionstats;

    zero_stats(&sectionstats);
    reset_fpf_position(cpi, start_position);

    for (i = 0; i < cpi->twopass.frames_to_key; i++) {
      input_stats(cpi, &next_frame);
      accumulate_stats(&sectionstats, &next_frame);
    }

    avg_stats(&sectionstats);

    cpi->twopass.section_intra_rating = (int)
      (sectionstats.intra_error
      / DOUBLE_DIVIDE_CHECK(sectionstats.coded_error));
  }

  
  reset_fpf_position(cpi, start_position);

  
  if (1) {
    int kf_boost = (int)boost_score;
    int allocation_chunks;
    int alt_kf_bits;

    if (kf_boost < (cpi->twopass.frames_to_key * 3))
      kf_boost = (cpi->twopass.frames_to_key * 3);

    if (kf_boost < 300)  
      kf_boost = 300;

    
    
    cpi->kf_boost = kf_boost;
    cpi->kf_zeromotion_pct = (int)(zero_motion_accumulator * 100.0);

    
    
    
    
    
    
    
    
    
    
    
    
    if (zero_motion_accumulator >= 0.99) {
      allocation_chunks =
        ((cpi->twopass.frames_to_key - 1) * 10) + kf_boost;
    } else {
      allocation_chunks =
        ((cpi->twopass.frames_to_key - 1) * 100) + kf_boost;
    }

    
    if (kf_boost > 1028) {
      int divisor = kf_boost >> 10;
      kf_boost /= divisor;
      allocation_chunks /= divisor;
    }

    cpi->twopass.kf_group_bits =
        (cpi->twopass.kf_group_bits < 0) ? 0 : cpi->twopass.kf_group_bits;

    
    cpi->twopass.kf_bits =
        (int)((double)kf_boost *
              ((double)cpi->twopass.kf_group_bits / (double)allocation_chunks));

    
    
    
    
    if (kf_mod_err < kf_group_err / cpi->twopass.frames_to_key) {
      double  alt_kf_grp_bits =
        ((double)cpi->twopass.bits_left *
         (kf_mod_err * (double)cpi->twopass.frames_to_key) /
         DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left));

      alt_kf_bits = (int)((double)kf_boost *
                          (alt_kf_grp_bits / (double)allocation_chunks));

      if (cpi->twopass.kf_bits > alt_kf_bits) {
        cpi->twopass.kf_bits = alt_kf_bits;
      }
    } else {
    
    
    
      alt_kf_bits =
        (int)((double)cpi->twopass.bits_left *
              (kf_mod_err /
               DOUBLE_DIVIDE_CHECK(cpi->twopass.modified_error_left)));

      if (alt_kf_bits > cpi->twopass.kf_bits) {
        cpi->twopass.kf_bits = alt_kf_bits;
      }
    }

    cpi->twopass.kf_group_bits -= cpi->twopass.kf_bits;
    
    cpi->twopass.kf_bits += cpi->min_frame_bandwidth;

    
    cpi->per_frame_bandwidth = cpi->twopass.kf_bits;
    
    cpi->target_bandwidth = (int)(cpi->twopass.kf_bits *
                                  cpi->output_framerate);
  }

  
  cpi->twopass.kf_group_error_left = (int)(kf_group_err - kf_mod_err);

  
  
  
  cpi->twopass.modified_error_left -= kf_group_err;
}
