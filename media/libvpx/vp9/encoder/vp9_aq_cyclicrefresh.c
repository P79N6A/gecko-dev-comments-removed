









#include <limits.h>
#include <math.h>

#include "vp9/encoder/vp9_aq_cyclicrefresh.h"

#include "vp9/common/vp9_seg_common.h"

#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/encoder/vp9_segmentation.h"

struct CYCLIC_REFRESH {
  
  
  int max_sbs_perframe;
  
  int max_qdelta_perc;
  
  BLOCK_SIZE min_block_size;
  
  int sb_index;
  
  int time_for_refresh;
  
  int num_seg_blocks;
  
  int actual_seg_bits;
  
  int rdmult;
  
  signed char *map;
  
  int64_t projected_rate_sb;
  int64_t projected_dist_sb;
  
  int64_t thresh_rate_sb;
  int64_t thresh_dist_sb;
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
  
  
  
  
  
  const float factor  = 0.5;
  const int number_blocks = cm->mi_rows  * cm->mi_cols;
  
  
  
  
  if (rc->avg_frame_bandwidth < factor * number_blocks ||
      number_blocks / 64 < 5)
    return 0;
  else
    return 1;
}





static int candidate_refresh_aq(const CYCLIC_REFRESH *cr,
                                const MB_MODE_INFO *mbmi,
                                BLOCK_SIZE bsize, int use_rd) {
  if (use_rd) {
    
    
    if (cr->projected_rate_sb < cr->thresh_rate_sb)
      return 1;
    
    
    
    
    
    else if (bsize < cr->min_block_size ||
             (mbmi->mv[0].as_int != 0 &&
              cr->projected_dist_sb > cr->thresh_dist_sb) ||
             !is_inter_block(mbmi))
      return 0;
    else
      return 1;
  } else {
    
    if (bsize < cr->min_block_size ||
        mbmi->mv[0].as_int != 0 ||
        !is_inter_block(mbmi))
      return 0;
    else
      return 1;
  }
}




void vp9_cyclic_refresh_update_segment(VP9_COMP *const cpi,
                                       MB_MODE_INFO *const mbmi,
                                       int mi_row, int mi_col,
                                       BLOCK_SIZE bsize, int use_rd) {
  const VP9_COMMON *const cm = &cpi->common;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  const int bw = num_8x8_blocks_wide_lookup[bsize];
  const int bh = num_8x8_blocks_high_lookup[bsize];
  const int xmis = MIN(cm->mi_cols - mi_col, bw);
  const int ymis = MIN(cm->mi_rows - mi_row, bh);
  const int block_index = mi_row * cm->mi_cols + mi_col;
  const int refresh_this_block = cpi->mb.in_static_area ||
                                 candidate_refresh_aq(cr, mbmi, bsize, use_rd);
  
  int new_map_value = cr->map[block_index];
  int x = 0; int y = 0;

  
  if (mbmi->segment_id > 0 && !refresh_this_block)
    mbmi->segment_id = 0;

  
  
  
  
  if (mbmi->segment_id == 1) {
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
  
  
  if (mbmi->segment_id)
    cr->num_seg_blocks += xmis * ymis;
}


void vp9_cyclic_refresh_setup(VP9_COMP *const cpi) {
  VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *const rc = &cpi->rc;
  CYCLIC_REFRESH *const cr = cpi->cyclic_refresh;
  struct segmentation *const seg = &cm->seg;
  unsigned char *const seg_map = cpi->segmentation_map;
  const int apply_cyclic_refresh  = apply_cyclic_refresh_bitrate(cm, rc);
  
  if (!apply_cyclic_refresh ||
      (cm->frame_type == KEY_FRAME) ||
      (cpi->svc.temporal_layer_id > 0)) {
    
    vpx_memset(seg_map, 0, cm->mi_rows * cm->mi_cols);
    vp9_disable_segmentation(&cm->seg);
    if (cm->frame_type == KEY_FRAME)
      cr->sb_index = 0;
    return;
  } else {
    int qindex_delta = 0;
    int i, block_count, bl_index, sb_rows, sb_cols, sbs_in_frame;
    int xmis, ymis, x, y, qindex2;

    
    const float rate_ratio_qdelta = 2.0;
    const double q = vp9_convert_qindex_to_q(cm->base_qindex, cm->bit_depth);
    vp9_clear_system_state();
    
    cr->max_sbs_perframe = 10;
    cr->max_qdelta_perc = 50;
    cr->min_block_size = BLOCK_8X8;
    cr->time_for_refresh = 1;
    
    cr->thresh_rate_sb = (rc->sb64_target_rate * 256) >> 2;
    
    cr->thresh_dist_sb = 8 * (int)(q * q);
    if (cpi->sf.use_nonrd_pick_mode) {
      
      
      cr->thresh_rate_sb = (rc->sb64_target_rate * 256) >> 3;
      cr->thresh_dist_sb = 4 * (int)(q * q);
    }

    cr->num_seg_blocks = 0;
    
    
    vpx_memset(seg_map, 0, cm->mi_rows * cm->mi_cols);
    vp9_enable_segmentation(&cm->seg);
    vp9_clearall_segfeatures(seg);
    
    seg->abs_delta = SEGMENT_DELTADATA;

    
    
    
    
    
    

    
    vp9_disable_segfeature(seg, 0, SEG_LVL_ALT_Q);
    
    vp9_enable_segfeature(seg, 1, SEG_LVL_ALT_Q);

    
    qindex_delta = vp9_compute_qdelta_by_rate(rc, cm->frame_type,
                                              cm->base_qindex,
                                              rate_ratio_qdelta,
                                              cm->bit_depth);
    
    
    if (-qindex_delta > cr->max_qdelta_perc * cm->base_qindex / 100)
      qindex_delta = -cr->max_qdelta_perc * cm->base_qindex / 100;

    
    qindex2 = clamp(cm->base_qindex + cm->y_dc_delta_q + qindex_delta, 0, MAXQ);
    cr->rdmult = vp9_compute_rd_mult(cpi, qindex2);

    vp9_set_segdata(seg, 1, SEG_LVL_ALT_Q, qindex_delta);

    sb_cols = (cm->mi_cols + MI_BLOCK_SIZE - 1) / MI_BLOCK_SIZE;
    sb_rows = (cm->mi_rows + MI_BLOCK_SIZE - 1) / MI_BLOCK_SIZE;
    sbs_in_frame = sb_cols * sb_rows;
    
    block_count = cr->max_sbs_perframe * sbs_in_frame / 100;
    
    
    
    assert(cr->sb_index < sbs_in_frame);
    i = cr->sb_index;
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
            seg_map[bl_index2] = 1;
            sum_map++;
          } else if (cr->map[bl_index2] < 0) {
            cr->map[bl_index2]++;
          }
        }
      }
      
      
      if (sum_map > 0 && sum_map < xmis * ymis) {
        const int new_value = (sum_map >= xmis * ymis / 2);
        for (y = 0; y < ymis; y++)
          for (x = 0; x < xmis; x++)
            seg_map[bl_index + y * cm->mi_cols + x] = new_value;
      }
      i++;
      if (i == sbs_in_frame) {
        i = 0;
      }
      if (sum_map >= xmis * ymis /2)
        block_count--;
    } while (block_count && i != cr->sb_index);
    cr->sb_index = i;
  }
}

void vp9_cyclic_refresh_set_rate_and_dist_sb(CYCLIC_REFRESH *cr,
                                             int64_t rate_sb, int64_t dist_sb) {
  cr->projected_rate_sb = rate_sb;
  cr->projected_dist_sb = dist_sb;
}

int vp9_cyclic_refresh_get_rdmult(const CYCLIC_REFRESH *cr) {
  return cr->rdmult;
}
