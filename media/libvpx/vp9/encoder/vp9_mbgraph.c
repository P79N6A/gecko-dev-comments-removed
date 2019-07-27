









#include <limits.h>

#include "vpx_mem/vpx_mem.h"
#include "vp9/encoder/vp9_rdopt.h"
#include "vp9/encoder/vp9_segmentation.h"
#include "vp9/encoder/vp9_mcomp.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_reconinter.h"
#include "vp9/common/vp9_reconintra.h"
#include "vp9/common/vp9_systemdependent.h"



static unsigned int do_16x16_motion_iteration(VP9_COMP *cpi,
                                              const MV *ref_mv,
                                              MV *dst_mv,
                                              int mb_row,
                                              int mb_col) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  vp9_variance_fn_ptr_t v_fn_ptr = cpi->fn_ptr[BLOCK_16X16];

  const int tmp_col_min = x->mv_col_min;
  const int tmp_col_max = x->mv_col_max;
  const int tmp_row_min = x->mv_row_min;
  const int tmp_row_max = x->mv_row_max;
  MV ref_full;

  
  int step_param = cpi->sf.reduce_first_step_size +
      (cpi->speed < 8 ? (cpi->speed > 5 ? 1 : 0) : 2);
  step_param = MIN(step_param, (cpi->sf.max_step_search_steps - 2));

  vp9_set_mv_search_range(x, ref_mv);

  ref_full.col = ref_mv->col >> 3;
  ref_full.row = ref_mv->row >> 3;

  
  vp9_hex_search(x, &ref_full, step_param, x->errorperbit, 0, &v_fn_ptr, 0,
                 ref_mv, dst_mv);

  
  
  {
    int distortion;
    unsigned int sse;
    cpi->find_fractional_mv_step(
        x, dst_mv, ref_mv, cpi->common.allow_high_precision_mv, x->errorperbit,
        &v_fn_ptr, 0, cpi->sf.subpel_iters_per_step, NULL, NULL, &distortion,
        &sse);
  }

  vp9_set_mbmode_and_mvs(xd, NEWMV, dst_mv);
  vp9_build_inter_predictors_sby(xd, mb_row, mb_col, BLOCK_16X16);

  
  x->mv_col_min = tmp_col_min;
  x->mv_col_max = tmp_col_max;
  x->mv_row_min = tmp_row_min;
  x->mv_row_max = tmp_row_max;

  return vp9_sad16x16(x->plane[0].src.buf, x->plane[0].src.stride,
          xd->plane[0].dst.buf, xd->plane[0].dst.stride,
          INT_MAX);
}

static int do_16x16_motion_search(VP9_COMP *cpi, const int_mv *ref_mv,
                                  int_mv *dst_mv, int mb_row, int mb_col) {
  MACROBLOCK *const x = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  unsigned int err, tmp_err;
  int_mv tmp_mv;

  
  
  err = vp9_sad16x16(x->plane[0].src.buf, x->plane[0].src.stride,
                     xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                     INT_MAX);
  dst_mv->as_int = 0;

  
  
  tmp_err = do_16x16_motion_iteration(cpi, &ref_mv->as_mv, &tmp_mv.as_mv,
                                      mb_row, mb_col);
  if (tmp_err < err) {
    err = tmp_err;
    dst_mv->as_int = tmp_mv.as_int;
  }

  
  
  if (ref_mv->as_int) {
    unsigned int tmp_err;
    int_mv zero_ref_mv, tmp_mv;

    zero_ref_mv.as_int = 0;
    tmp_err = do_16x16_motion_iteration(cpi, &zero_ref_mv.as_mv, &tmp_mv.as_mv,
                                        mb_row, mb_col);
    if (tmp_err < err) {
      dst_mv->as_int = tmp_mv.as_int;
      err = tmp_err;
    }
  }

  return err;
}

static int do_16x16_zerozero_search(VP9_COMP *cpi, int_mv *dst_mv) {
  MACROBLOCK *const x = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  unsigned int err;

  
  
  err = vp9_sad16x16(x->plane[0].src.buf, x->plane[0].src.stride,
                     xd->plane[0].pre[0].buf, xd->plane[0].pre[0].stride,
                     INT_MAX);

  dst_mv->as_int = 0;

  return err;
}
static int find_best_16x16_intra(VP9_COMP *cpi,
                                 int mb_y_offset,
                                 MB_PREDICTION_MODE *pbest_mode) {
  MACROBLOCK   *const x  = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  MB_PREDICTION_MODE best_mode = -1, mode;
  unsigned int best_err = INT_MAX;

  
  
  for (mode = DC_PRED; mode <= TM_PRED; mode++) {
    unsigned int err;

    xd->mi_8x8[0]->mbmi.mode = mode;
    vp9_predict_intra_block(xd, 0, 2, TX_16X16, mode,
                            x->plane[0].src.buf, x->plane[0].src.stride,
                            xd->plane[0].dst.buf, xd->plane[0].dst.stride,
                            0, 0, 0);
    err = vp9_sad16x16(x->plane[0].src.buf, x->plane[0].src.stride,
                       xd->plane[0].dst.buf, xd->plane[0].dst.stride, best_err);

    
    if (err < best_err) {
      best_err  = err;
      best_mode = mode;
    }
  }

  if (pbest_mode)
    *pbest_mode = best_mode;

  return best_err;
}

static void update_mbgraph_mb_stats
(
  VP9_COMP *cpi,
  MBGRAPH_MB_STATS *stats,
  YV12_BUFFER_CONFIG *buf,
  int mb_y_offset,
  YV12_BUFFER_CONFIG *golden_ref,
  int_mv *prev_golden_ref_mv,
  int gld_y_offset,
  YV12_BUFFER_CONFIG *alt_ref,
  int_mv *prev_alt_ref_mv,
  int arf_y_offset,
  int mb_row,
  int mb_col
) {
  MACROBLOCK *const x = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  int intra_error;
  VP9_COMMON *cm = &cpi->common;

  
  x->plane[0].src.buf = buf->y_buffer + mb_y_offset;
  x->plane[0].src.stride = buf->y_stride;

  xd->plane[0].dst.buf = get_frame_new_buffer(cm)->y_buffer + mb_y_offset;
  xd->plane[0].dst.stride = get_frame_new_buffer(cm)->y_stride;

  
  intra_error = find_best_16x16_intra(cpi, mb_y_offset,
                                      &stats->ref[INTRA_FRAME].m.mode);
  if (intra_error <= 0)
    intra_error = 1;
  stats->ref[INTRA_FRAME].err = intra_error;

  
  if (golden_ref) {
    int g_motion_error;
    xd->plane[0].pre[0].buf = golden_ref->y_buffer + mb_y_offset;
    xd->plane[0].pre[0].stride = golden_ref->y_stride;
    g_motion_error = do_16x16_motion_search(cpi,
                                            prev_golden_ref_mv,
                                            &stats->ref[GOLDEN_FRAME].m.mv,
                                            mb_row, mb_col);
    stats->ref[GOLDEN_FRAME].err = g_motion_error;
  } else {
    stats->ref[GOLDEN_FRAME].err = INT_MAX;
    stats->ref[GOLDEN_FRAME].m.mv.as_int = 0;
  }

  
  
  if (alt_ref) {
    int a_motion_error;
    xd->plane[0].pre[0].buf = alt_ref->y_buffer + mb_y_offset;
    xd->plane[0].pre[0].stride = alt_ref->y_stride;
    a_motion_error = do_16x16_zerozero_search(cpi,
                                              &stats->ref[ALTREF_FRAME].m.mv);

    stats->ref[ALTREF_FRAME].err = a_motion_error;
  } else {
    stats->ref[ALTREF_FRAME].err = INT_MAX;
    stats->ref[ALTREF_FRAME].m.mv.as_int = 0;
  }
}

static void update_mbgraph_frame_stats(VP9_COMP *cpi,
                                       MBGRAPH_FRAME_STATS *stats,
                                       YV12_BUFFER_CONFIG *buf,
                                       YV12_BUFFER_CONFIG *golden_ref,
                                       YV12_BUFFER_CONFIG *alt_ref) {
  MACROBLOCK *const x = &cpi->mb;
  MACROBLOCKD *const xd = &x->e_mbd;
  VP9_COMMON *const cm = &cpi->common;

  int mb_col, mb_row, offset = 0;
  int mb_y_offset = 0, arf_y_offset = 0, gld_y_offset = 0;
  int_mv arf_top_mv, gld_top_mv;
  MODE_INFO mi_local = { { 0 } };

  
  
  arf_top_mv.as_int = 0;
  gld_top_mv.as_int = 0;
  x->mv_row_min     = -BORDER_MV_PIXELS_B16;
  x->mv_row_max     = (cm->mb_rows - 1) * 8 + BORDER_MV_PIXELS_B16;
  xd->up_available  = 0;
  xd->plane[0].dst.stride  = buf->y_stride;
  xd->plane[0].pre[0].stride  = buf->y_stride;
  xd->plane[1].dst.stride = buf->uv_stride;
  xd->mi_8x8[0] = &mi_local;
  mi_local.mbmi.sb_type = BLOCK_16X16;
  mi_local.mbmi.ref_frame[0] = LAST_FRAME;
  mi_local.mbmi.ref_frame[1] = NONE;

  for (mb_row = 0; mb_row < cm->mb_rows; mb_row++) {
    int_mv arf_left_mv, gld_left_mv;
    int mb_y_in_offset  = mb_y_offset;
    int arf_y_in_offset = arf_y_offset;
    int gld_y_in_offset = gld_y_offset;

    
    
    arf_left_mv.as_int = arf_top_mv.as_int;
    gld_left_mv.as_int = gld_top_mv.as_int;
    x->mv_col_min      = -BORDER_MV_PIXELS_B16;
    x->mv_col_max      = (cm->mb_cols - 1) * 8 + BORDER_MV_PIXELS_B16;
    xd->left_available = 0;

    for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
      MBGRAPH_MB_STATS *mb_stats = &stats->mb_stats[offset + mb_col];

      update_mbgraph_mb_stats(cpi, mb_stats, buf, mb_y_in_offset,
                              golden_ref, &gld_left_mv, gld_y_in_offset,
                              alt_ref,    &arf_left_mv, arf_y_in_offset,
                              mb_row, mb_col);
      arf_left_mv.as_int = mb_stats->ref[ALTREF_FRAME].m.mv.as_int;
      gld_left_mv.as_int = mb_stats->ref[GOLDEN_FRAME].m.mv.as_int;
      if (mb_col == 0) {
        arf_top_mv.as_int = arf_left_mv.as_int;
        gld_top_mv.as_int = gld_left_mv.as_int;
      }
      xd->left_available = 1;
      mb_y_in_offset    += 16;
      gld_y_in_offset   += 16;
      arf_y_in_offset   += 16;
      x->mv_col_min     -= 16;
      x->mv_col_max     -= 16;
    }
    xd->up_available = 1;
    mb_y_offset     += buf->y_stride * 16;
    gld_y_offset    += golden_ref->y_stride * 16;
    if (alt_ref)
      arf_y_offset    += alt_ref->y_stride * 16;
    x->mv_row_min   -= 16;
    x->mv_row_max   -= 16;
    offset          += cm->mb_cols;
  }
}


static void separate_arf_mbs(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  int mb_col, mb_row, offset, i;
  int mi_row, mi_col;
  int ncnt[4] = { 0 };
  int n_frames = cpi->mbgraph_n_frames;

  int *arf_not_zz;

  CHECK_MEM_ERROR(cm, arf_not_zz,
                  vpx_calloc(cm->mb_rows * cm->mb_cols * sizeof(*arf_not_zz),
                             1));

  
  if (n_frames > cpi->rc.frames_till_gf_update_due)
    n_frames = cpi->rc.frames_till_gf_update_due;

  
  for (i = n_frames - 1; i >= 0; i--) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];

    for (offset = 0, mb_row = 0; mb_row < cm->mb_rows;
         offset += cm->mb_cols, mb_row++) {
      for (mb_col = 0; mb_col < cm->mb_cols; mb_col++) {
        MBGRAPH_MB_STATS *mb_stats = &frame_stats->mb_stats[offset + mb_col];

        int altref_err = mb_stats->ref[ALTREF_FRAME].err;
        int intra_err  = mb_stats->ref[INTRA_FRAME ].err;
        int golden_err = mb_stats->ref[GOLDEN_FRAME].err;

        
        if (altref_err > 1000 ||
            altref_err > intra_err ||
            altref_err > golden_err) {
          arf_not_zz[offset + mb_col]++;
        }
      }
    }
  }

  
  
  for (mi_row = 0; mi_row < cm->mi_rows; mi_row++) {
    for (mi_col = 0; mi_col < cm->mi_cols; mi_col++) {
      
      
      if (arf_not_zz[mi_row / 2 * cm->mb_cols + mi_col / 2]) {
        ncnt[0]++;
        cpi->segmentation_map[mi_row * cm->mi_cols + mi_col] = 0;
      } else {
        cpi->segmentation_map[mi_row * cm->mi_cols + mi_col] = 1;
        ncnt[1]++;
      }
    }
  }

  
  
  if (1) {
    
    if (cm->MBs)
      cpi->static_mb_pct = (ncnt[1] * 100) / (cm->mi_rows * cm->mi_cols);

    
    
    else
      cpi->static_mb_pct = 0;

    cpi->seg0_cnt = ncnt[0];
    vp9_enable_segmentation((VP9_PTR)cpi);
  } else {
    cpi->static_mb_pct = 0;
    vp9_disable_segmentation((VP9_PTR)cpi);
  }

  
  vpx_free(arf_not_zz);
}

void vp9_update_mbgraph_stats(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  int i, n_frames = vp9_lookahead_depth(cpi->lookahead);
  YV12_BUFFER_CONFIG *golden_ref = get_ref_frame_buffer(cpi, GOLDEN_FRAME);

  
  
  if (n_frames <= cpi->rc.frames_till_gf_update_due)
    return;

  if (n_frames > MAX_LAG_BUFFERS)
    n_frames = MAX_LAG_BUFFERS;

  cpi->mbgraph_n_frames = n_frames;
  for (i = 0; i < n_frames; i++) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];
    vpx_memset(frame_stats->mb_stats, 0,
               cm->mb_rows * cm->mb_cols *
               sizeof(*cpi->mbgraph_stats[i].mb_stats));
  }

  
  
  
  
  for (i = 0; i < n_frames; i++) {
    MBGRAPH_FRAME_STATS *frame_stats = &cpi->mbgraph_stats[i];
    struct lookahead_entry *q_cur = vp9_lookahead_peek(cpi->lookahead, i);

    assert(q_cur != NULL);

    update_mbgraph_frame_stats(cpi, frame_stats, &q_cur->img,
                               golden_ref, cpi->Source);
  }

  vp9_clear_system_state();  

  separate_arf_mbs(cpi);
}
