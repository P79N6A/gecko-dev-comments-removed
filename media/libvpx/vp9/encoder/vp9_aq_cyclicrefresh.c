









#include <limits.h>
#include <math.h>

#include "vp9/encoder/vp9_aq_cyclicrefresh.h"

#include "vp9/common/vp9_seg_common.h"

#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/encoder/vp9_segmentation.h"

struct CYCLIC_REFRESH {
  
  
  int percent_refresh;
  
  int max_qdelta_perc;
  
  int sb_index;
  
  
  
  int time_for_refresh;
  
  int target_num_seg_blocks;
  
  int actual_num_seg1_blocks;
  int actual_num_seg2_blocks;
  
  int rdmult;
  
  signed char *map;
  
  
  int64_t thresh_rate_sb;
  int64_t thresh_dist_sb;
  
  
  int16_t motion_thresh;
  
  double rate_ratio_qdelta;
  double low_content_avg;
  int qindex_delta_seg1;
  int qindex_delta_seg2;
};

CYCLIC_REFRESH *vp9_cyclic_refresh_alloc(int mi_rows, int mi_cols) {
  CYCLIC_REFRESH *const cr = vpx_calloc(1, sizeof(*cr));
  if (cr == NULL)
    return NULL;

  cr->map = vpx_calloc(mi_rows * mi_cols, sizeof(*cr->map));
  if (cr->map == NULL) {
    vpx_free(cr);
    return NULL;
  }

  return cr;
}

void vp9_cyclic_refresh_free(CYCLIC_REFRESH *cr) {
  vpx_free(cr->map);
  vpx_free(cr);
}


static int apply_cyclic_refresh_bitrate(const VP9_COMMON *cm,
                                        const RATE_CONTROL *rc) {
  
  
  
  
  
  const float factor = 0.25;
  const int number_blocks = cm->mi_rows  * cm->mi_cols;
  
  
  
  
  if (rc->avg_frame_bandwidth < factor * number_blocks ||
      number_blocks / 64 < 5)
    return 0;
  else
    return 1;
}





static int candidate_refresh_aq(const CYCLIC_REFRESH *cr,
                                const MB_MODE_INFO *mbmi,
                                int64_t rate,
                                int64_t dist,
                                int bsize) {
  MV mv = mbmi->mv[0].as_mv;
  
  
  
  
  
  if (dist > cr->thresh_dist_sb &&
      (mv.row > cr->motion_thresh || mv.row < -cr->motion_thresh ||
       mv.col > cr->motion_thresh || mv.col < -cr->motion_thresh ||
       !is_inter_block(mbmi)))
    return CR_SEGMENT_ID_BASE;
  else  if (bsize >= BLOCK_16X16 &&
            rate < cr->thresh_rate_sb &&
            is_inter_block(mbmi) &&
            mbmi->mv[0].as_int == 0)
    
    return CR_SEGMENT_ID_BOOST2;
  else
    return CR_SEGMENT_ID_BOOST1;
}


static int compute_deltaq(const VP9_COMP *cpi, int q, double rate_factor) {
  const CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  const RATE_CONTROL *const rc = &cpi->rc;
  int deltaq = vp9_compute_qdelta_by_rate(rc, cpi->common.frame_type,
                                          q, rate_factor,
                                          cpi->common.bit_depth);
  if ((-deltaq) > cr->max_qdelta_perc * q / 100) {
    deltaq = -cr->max_qdelta_perc * q / 100;
  }
  return deltaq;
}





int vp9_cyclic_refresh_estimate_bits_at_q(const VP9_COMP *cpi,
                                          double correction_factor) {
  const VP9_COMMON *const cm = &cpi->common;
  const CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  int estimated_bits;
  int mbs = cm->MBs;
  int num8x8bl = mbs << 2;
  
  
  double weight_segment1 = (double)cr->actual_num_seg1_blocks / num8x8bl;
  double weight_segment2 = (double)cr->actual_num_seg2_blocks / num8x8bl;
  
  estimated_bits = (int)((1.0 - weight_segment1 - weight_segment2) *
      vp9_estimate_bits_at_q(cm->frame_type, cm->base_qindex, mbs,
                             correction_factor, cm->bit_depth) +
                             weight_segment1 *
      vp9_estimate_bits_at_q(cm->frame_type,
                             cm->base_qindex + cr->qindex_delta_seg1, mbs,
                             correction_factor, cm->bit_depth) +
                             weight_segment2 *
      vp9_estimate_bits_at_q(cm->frame_type,
                             cm->base_qindex + cr->qindex_delta_seg2, mbs,
                             correction_factor, cm->bit_depth));
  return estimated_bits;
}






int vp9_cyclic_refresh_rc_bits_per_mb(const VP9_COMP *cpi, int i,
                                      double correction_factor) {
  const VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  int bits_per_mb;
  int num8x8bl = cm->MBs << 2;
  
  
  double weight_segment = (double)((cr->target_num_seg_blocks +
      cr->actual_num_seg1_blocks + cr->actual_num_seg2_blocks) >> 1) /
      num8x8bl;
  
  int deltaq = compute_deltaq(cpi, i, cr->rate_ratio_qdelta);
  
  bits_per_mb = (int)((1.0 - weight_segment) *
      vp9_rc_bits_per_mb(cm->frame_type, i, correction_factor, cm->bit_depth) +
      weight_segment *
      vp9_rc_bits_per_mb(cm->frame_type, i + deltaq, correction_factor,
                         cm->bit_depth));
  return bits_per_mb;
}




void vp9_cyclic_refresh_update_segment(VP9_COMP *const cpi,
                                       MB_MODE_INFO *const mbmi,
                                       int mi_row, int mi_col,
                                       BLOCK_SIZE bsize,
                                       int64_t rate,
                                       int64_t dist,
                                       int skip) {
  const VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  const int bw = num_8x8_blocks_wide_lookup[bsize];
  const int bh = num_8x8_blocks_high_lookup[bsize];
  const int xmis = MIN(cm->mi_cols - mi_col, bw);
  const int ymis = MIN(cm->mi_rows - mi_row, bh);
  const int block_index = mi_row * cm->mi_cols + mi_col;
  const int refresh_this_block = candidate_refresh_aq(cr, mbmi, rate, dist,
                                                      bsize);
  
  int new_map_value = cr->map[block_index];
  int x = 0; int y = 0;

  
  
  if (cyclic_refresh_segment_id_boosted(mbmi->segment_id)) {
    mbmi->segment_id = refresh_this_block;
    
    if (skip)
      mbmi->segment_id = CR_SEGMENT_ID_BASE;
  }

  
  
  
  
  if (cyclic_refresh_segment_id_boosted(mbmi->segment_id)) {
    new_map_value = -cr->time_for_refresh;
  } else if (refresh_this_block) {
    
    
    
    if (cr->map[block_index] == 1)
      new_map_value = 0;
  } else {
    
    new_map_value = 1;
  }

  
  
  for (y = 0; y < ymis; y++)
    for (x = 0; x < xmis; x++) {
      cr->map[block_index + y * cm->mi_cols + x] = new_map_value;
      cpi->segmentation_map[block_index + y * cm->mi_cols + x] =
          mbmi->segment_id;
    }
}


void vp9_cyclic_refresh_postencode(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  unsigned char *const seg_map = cpi->segmentation_map;
  int mi_row, mi_col;
  cr->actual_num_seg1_blocks = 0;
  cr->actual_num_seg2_blocks = 0;
  for (mi_row = 0; mi_row < cm->mi_rows; mi_row++)
    for (mi_col = 0; mi_col < cm->mi_cols; mi_col++) {
      if (cyclic_refresh_segment_id(
          seg_map[mi_row * cm->mi_cols + mi_col]) == CR_SEGMENT_ID_BOOST1)
        cr->actual_num_seg1_blocks++;
      else if (cyclic_refresh_segment_id(
          seg_map[mi_row * cm->mi_cols + mi_col]) == CR_SEGMENT_ID_BOOST2)
        cr->actual_num_seg2_blocks++;
    }
}


void vp9_cyclic_refresh_set_golden_update(VP9_COMP *const cpi) {
  RATE_CONTROL *const rc = &cpi->rc;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  
  
  
  if (cr->percent_refresh > 0)
    rc->baseline_gf_interval = 4 * (100 / cr->percent_refresh);
  else
    rc->baseline_gf_interval = 40;
}





void vp9_cyclic_refresh_check_golden_update(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  int mi_row, mi_col;
  double fraction_low = 0.0;
  int low_content_frame = 0;

  MODE_INFO **mi = cm->mi_grid_visible;
  RATE_CONTROL *const rc = &cpi->rc;
  const int rows = cm->mi_rows, cols = cm->mi_cols;
  int cnt1 = 0, cnt2 = 0;
  int force_gf_refresh = 0;

  for (mi_row = 0; mi_row < rows; mi_row++) {
    for (mi_col = 0; mi_col < cols; mi_col++) {
      int16_t abs_mvr = mi[0]->mbmi.mv[0].as_mv.row >= 0 ?
          mi[0]->mbmi.mv[0].as_mv.row : -1 * mi[0]->mbmi.mv[0].as_mv.row;
      int16_t abs_mvc = mi[0]->mbmi.mv[0].as_mv.col >= 0 ?
          mi[0]->mbmi.mv[0].as_mv.col : -1 * mi[0]->mbmi.mv[0].as_mv.col;

      
      if (abs_mvr <= 16 && abs_mvc <= 16) {
        cnt1++;
        if (abs_mvr == 0 && abs_mvc == 0)
          cnt2++;
      }
      mi++;

      
      if (cr->map[mi_row * cols + mi_col] < 1)
        low_content_frame++;
    }
    mi += 8;
  }

  
  
  
  if (cnt1 * 10 > (70 * rows * cols) && cnt2 * 20 < cnt1) {
    vp9_cyclic_refresh_set_golden_update(cpi);
    rc->frames_till_gf_update_due = rc->baseline_gf_interval;

    if (rc->frames_till_gf_update_due > rc->frames_to_key)
      rc->frames_till_gf_update_due = rc->frames_to_key;
    cpi->refresh_golden_frame = 1;
    force_gf_refresh = 1;
  }

  fraction_low =
      (double)low_content_frame / (rows * cols);
  
  cr->low_content_avg = (fraction_low + 3 * cr->low_content_avg) / 4;
  if (!force_gf_refresh && cpi->refresh_golden_frame == 1) {
    
    
    
    if (fraction_low < 0.8 || cr->low_content_avg < 0.7)
      cpi->refresh_golden_frame = 0;
    
    cr->low_content_avg = fraction_low;
  }
}







static void cyclic_refresh_update_map(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  unsigned char *const seg_map = cpi->segmentation_map;
  int i, block_count, bl_index, sb_rows, sb_cols, sbs_in_frame;
  int xmis, ymis, x, y;
  memset(seg_map, CR_SEGMENT_ID_BASE, cm->mi_rows * cm->mi_cols);
  sb_cols = (cm->mi_cols + MI_BLOCK_SIZE - 1) / MI_BLOCK_SIZE;
  sb_rows = (cm->mi_rows + MI_BLOCK_SIZE - 1) / MI_BLOCK_SIZE;
  sbs_in_frame = sb_cols * sb_rows;
  
  block_count = cr->percent_refresh * cm->mi_rows * cm->mi_cols / 100;
  
  
  
  assert(cr->sb_index < sbs_in_frame);
  i = cr->sb_index;
  cr->target_num_seg_blocks = 0;
  do {
    int sum_map = 0;
    
    int sb_row_index = (i / sb_cols);
    int sb_col_index = i - sb_row_index * sb_cols;
    int mi_row = sb_row_index * MI_BLOCK_SIZE;
    int mi_col = sb_col_index * MI_BLOCK_SIZE;
    assert(mi_row >= 0 && mi_row < cm->mi_rows);
    assert(mi_col >= 0 && mi_col < cm->mi_cols);
    bl_index = mi_row * cm->mi_cols + mi_col;
    
    xmis = MIN(cm->mi_cols - mi_col,
               num_8x8_blocks_wide_lookup[BLOCK_64X64]);
    ymis = MIN(cm->mi_rows - mi_row,
               num_8x8_blocks_high_lookup[BLOCK_64X64]);
    for (y = 0; y < ymis; y++) {
      for (x = 0; x < xmis; x++) {
        const int bl_index2 = bl_index + y * cm->mi_cols + x;
        
        
        
        if (cr->map[bl_index2] == 0) {
          sum_map++;
        } else if (cr->map[bl_index2] < 0) {
          cr->map[bl_index2]++;
        }
      }
    }
    
    
    if (sum_map >= xmis * ymis / 2) {
      for (y = 0; y < ymis; y++)
        for (x = 0; x < xmis; x++) {
          seg_map[bl_index + y * cm->mi_cols + x] = CR_SEGMENT_ID_BOOST1;
        }
      cr->target_num_seg_blocks += xmis * ymis;
    }
    i++;
    if (i == sbs_in_frame) {
      i = 0;
    }
  } while (cr->target_num_seg_blocks < block_count && i != cr->sb_index);
  cr->sb_index = i;
}


void vp9_cyclic_refresh_update_parameters(VP9_COMP *const cpi) {
  const RATE_CONTROL *const rc = &cpi->rc;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  cr->percent_refresh = 10;
  
  
  
  if (rc->frames_since_key <  40)
    cr->rate_ratio_qdelta = 3.0;
  else
    cr->rate_ratio_qdelta = 2.0;
}


void vp9_cyclic_refresh_setup(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *const rc = &cpi->rc;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  struct segmentation *const seg = &cm->seg;
  const int apply_cyclic_refresh  = apply_cyclic_refresh_bitrate(cm, rc);
  if (cm->current_video_frame == 0)
    cr->low_content_avg = 0.0;
  
  if (!apply_cyclic_refresh ||
      (cm->frame_type == KEY_FRAME) ||
      (cpi->svc.temporal_layer_id > 0) ||
      (cpi->svc.spatial_layer_id > 0)) {
    
    unsigned char *const seg_map = cpi->segmentation_map;
    memset(seg_map, 0, cm->mi_rows * cm->mi_cols);
    vp9_disable_segmentation(&cm->seg);
    if (cm->frame_type == KEY_FRAME)
      cr->sb_index = 0;
    return;
  } else {
    int qindex_delta = 0;
    int qindex2;
    const double q = vp9_convert_qindex_to_q(cm->base_qindex, cm->bit_depth);
    vp9_clear_system_state();
    cr->max_qdelta_perc = 50;
    cr->time_for_refresh = 0;
    
    
    cr->thresh_rate_sb = ((int64_t)(rc->sb64_target_rate) << 8) << 2;
    
    
    
    cr->thresh_dist_sb = ((int64_t)(q * q)) << 2;
    cr->motion_thresh = 32;
    
    
    vp9_enable_segmentation(&cm->seg);
    vp9_clearall_segfeatures(seg);
    
    seg->abs_delta = SEGMENT_DELTADATA;

    
    
    
    
    
    

    
    vp9_disable_segfeature(seg, CR_SEGMENT_ID_BASE, SEG_LVL_ALT_Q);
    
    vp9_enable_segfeature(seg, CR_SEGMENT_ID_BOOST1, SEG_LVL_ALT_Q);
    
    vp9_enable_segfeature(seg, CR_SEGMENT_ID_BOOST2, SEG_LVL_ALT_Q);

    
    qindex_delta = compute_deltaq(cpi, cm->base_qindex, cr->rate_ratio_qdelta);
    cr->qindex_delta_seg1 = qindex_delta;

    
    qindex2 = clamp(cm->base_qindex + cm->y_dc_delta_q + qindex_delta, 0, MAXQ);

    cr->rdmult = vp9_compute_rd_mult(cpi, qindex2);

    vp9_set_segdata(seg, CR_SEGMENT_ID_BOOST1, SEG_LVL_ALT_Q, qindex_delta);

    
    qindex_delta = compute_deltaq(cpi, cm->base_qindex,
                                  MIN(CR_MAX_RATE_TARGET_RATIO,
                                      CR_BOOST2_FAC * cr->rate_ratio_qdelta));
    cr->qindex_delta_seg2 = qindex_delta;
    vp9_set_segdata(seg, CR_SEGMENT_ID_BOOST2, SEG_LVL_ALT_Q, qindex_delta);

    
    cyclic_refresh_update_map(cpi);
  }
}

int vp9_cyclic_refresh_get_rdmult(const CYCLIC_REFRESH *cr) {
  return cr->rdmult;
}
