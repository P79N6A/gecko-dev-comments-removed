









#include <limits.h>
#include <math.h>

#include "vp9/common/vp9_seg_common.h"

#include "vp9/encoder/vp9_segmentation.h"

#define AQ_C_SEGMENTS  3
#define AQ_C_STRENGTHS  3
static const int aq_c_active_segments[AQ_C_STRENGTHS] = {1, 2, 3};
static const double aq_c_q_adj_factor[AQ_C_STRENGTHS][AQ_C_SEGMENTS] =
  {{1.0, 1.0, 1.0}, {1.0, 2.0, 1.0}, {1.0, 1.5, 2.5}};
static const double aq_c_transitions[AQ_C_STRENGTHS][AQ_C_SEGMENTS] =
  {{1.0, 1.0, 1.0}, {1.0, 0.25, 0.0}, {1.0, 0.5, 0.25}};

static int get_aq_c_strength(int q_index, vpx_bit_depth_t bit_depth) {
  
  const int base_quant = vp9_ac_quant(q_index, 0, bit_depth) / 4;
  return (base_quant > 20) + (base_quant > 45);
}

void vp9_setup_in_frame_q_adj(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  struct segmentation *const seg = &cm->seg;

  
  vp9_clear_system_state();

  if (cm->frame_type == KEY_FRAME ||
      cpi->refresh_alt_ref_frame ||
      (cpi->refresh_golden_frame && !cpi->rc.is_src_frame_alt_ref)) {
    int segment;
    const int aq_strength = get_aq_c_strength(cm->base_qindex, cm->bit_depth);
    const int active_segments = aq_c_active_segments[aq_strength];

    
    vpx_memset(cpi->segmentation_map, 0, cm->mi_rows * cm->mi_cols);

    
    vpx_memset(cpi->complexity_map, 0, cm->mi_rows * cm->mi_cols);

    vp9_clearall_segfeatures(seg);

    
    
    if (cpi->rc.sb64_target_rate < 256) {
      vp9_disable_segmentation(seg);
      return;
    }

    vp9_enable_segmentation(seg);

    
    seg->abs_delta = SEGMENT_DELTADATA;

    
    vp9_disable_segfeature(seg, 0, SEG_LVL_ALT_Q);

    
    for (segment = 1; segment < active_segments; ++segment) {
      int qindex_delta =
          vp9_compute_qdelta_by_rate(&cpi->rc, cm->frame_type, cm->base_qindex,
                                     aq_c_q_adj_factor[aq_strength][segment],
                                     cm->bit_depth);

      
      
      
      
      if ((cm->base_qindex != 0) && ((cm->base_qindex + qindex_delta) == 0)) {
        qindex_delta = -cm->base_qindex + 1;
      }
      if ((cm->base_qindex + qindex_delta) > 0) {
        vp9_enable_segfeature(seg, segment, SEG_LVL_ALT_Q);
        vp9_set_segdata(seg, segment, SEG_LVL_ALT_Q, qindex_delta);
      }
    }
  }
}







void vp9_select_in_frame_q_segment(VP9_COMP *cpi,
                                   int mi_row, int mi_col,
                                   int output_enabled, int projected_rate) {
  VP9_COMMON *const cm = &cpi->common;

  const int mi_offset = mi_row * cm->mi_cols + mi_col;
  const int bw = num_8x8_blocks_wide_lookup[BLOCK_64X64];
  const int bh = num_8x8_blocks_high_lookup[BLOCK_64X64];
  const int xmis = MIN(cm->mi_cols - mi_col, bw);
  const int ymis = MIN(cm->mi_rows - mi_row, bh);
  int complexity_metric = 64;
  int x, y;

  unsigned char segment;

  if (!output_enabled) {
    segment = 0;
  } else {
    
    
    const int target_rate = (cpi->rc.sb64_target_rate * xmis * ymis * 256) /
                            (bw * bh);
    const int aq_strength = get_aq_c_strength(cm->base_qindex, cm->bit_depth);
    const int active_segments = aq_c_active_segments[aq_strength];

    
    
    
    
    
    
    segment = active_segments - 1;
    while (segment > 0) {
      if (projected_rate <
          (target_rate * aq_c_transitions[aq_strength][segment])) {
        break;
      }
      --segment;
    }

    if (target_rate > 0) {
      complexity_metric =
        clamp((int)((projected_rate * 64) / target_rate), 16, 255);
    }
  }

  
  for (y = 0; y < ymis; y++) {
    for (x = 0; x < xmis; x++) {
      cpi->segmentation_map[mi_offset + y * cm->mi_cols + x] = segment;
      cpi->complexity_map[mi_offset + y * cm->mi_cols + x] =
        (unsigned char)complexity_metric;
    }
  }
}
