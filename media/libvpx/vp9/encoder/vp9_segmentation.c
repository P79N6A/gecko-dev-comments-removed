










#include <limits.h>
#include "vpx_mem/vpx_mem.h"
#include "vp9/encoder/vp9_segmentation.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_tile_common.h"

void vp9_enable_segmentation(VP9_PTR ptr) {
  VP9_COMP *cpi = (VP9_COMP *)ptr;
  struct segmentation *const seg =  &cpi->common.seg;

  seg->enabled = 1;
  seg->update_map = 1;
  seg->update_data = 1;
}

void vp9_disable_segmentation(VP9_PTR ptr) {
  VP9_COMP *cpi = (VP9_COMP *)ptr;
  struct segmentation *const seg =  &cpi->common.seg;
  seg->enabled = 0;
}

void vp9_set_segmentation_map(VP9_PTR ptr,
                              unsigned char *segmentation_map) {
  VP9_COMP *cpi = (VP9_COMP *)ptr;
  struct segmentation *const seg = &cpi->common.seg;

  
  vpx_memcpy(cpi->segmentation_map, segmentation_map,
             (cpi->common.mi_rows * cpi->common.mi_cols));

  
  seg->update_map = 1;
  seg->update_data = 1;
}

void vp9_set_segment_data(VP9_PTR ptr,
                          signed char *feature_data,
                          unsigned char abs_delta) {
  VP9_COMP *cpi = (VP9_COMP *)ptr;
  struct segmentation *const seg = &cpi->common.seg;

  seg->abs_delta = abs_delta;

  vpx_memcpy(seg->feature_data, feature_data, sizeof(seg->feature_data));

  
  
  
}
void vp9_disable_segfeature(struct segmentation *seg, int segment_id,
                            SEG_LVL_FEATURES feature_id) {
  seg->feature_mask[segment_id] &= ~(1 << feature_id);
}

void vp9_clear_segdata(struct segmentation *seg, int segment_id,
                       SEG_LVL_FEATURES feature_id) {
  seg->feature_data[segment_id][feature_id] = 0;
}


static void calc_segtree_probs(int *segcounts, vp9_prob *segment_tree_probs) {
  
  const int c01 = segcounts[0] + segcounts[1];
  const int c23 = segcounts[2] + segcounts[3];
  const int c45 = segcounts[4] + segcounts[5];
  const int c67 = segcounts[6] + segcounts[7];

  segment_tree_probs[0] = get_binary_prob(c01 + c23, c45 + c67);
  segment_tree_probs[1] = get_binary_prob(c01, c23);
  segment_tree_probs[2] = get_binary_prob(c45, c67);
  segment_tree_probs[3] = get_binary_prob(segcounts[0], segcounts[1]);
  segment_tree_probs[4] = get_binary_prob(segcounts[2], segcounts[3]);
  segment_tree_probs[5] = get_binary_prob(segcounts[4], segcounts[5]);
  segment_tree_probs[6] = get_binary_prob(segcounts[6], segcounts[7]);
}


static int cost_segmap(int *segcounts, vp9_prob *probs) {
  const int c01 = segcounts[0] + segcounts[1];
  const int c23 = segcounts[2] + segcounts[3];
  const int c45 = segcounts[4] + segcounts[5];
  const int c67 = segcounts[6] + segcounts[7];
  const int c0123 = c01 + c23;
  const int c4567 = c45 + c67;

  
  int cost = c0123 * vp9_cost_zero(probs[0]) +
             c4567 * vp9_cost_one(probs[0]);

  
  if (c0123 > 0) {
    cost += c01 * vp9_cost_zero(probs[1]) +
            c23 * vp9_cost_one(probs[1]);

    if (c01 > 0)
      cost += segcounts[0] * vp9_cost_zero(probs[3]) +
              segcounts[1] * vp9_cost_one(probs[3]);
    if (c23 > 0)
      cost += segcounts[2] * vp9_cost_zero(probs[4]) +
              segcounts[3] * vp9_cost_one(probs[4]);
  }

  if (c4567 > 0) {
    cost += c45 * vp9_cost_zero(probs[2]) +
            c67 * vp9_cost_one(probs[2]);

    if (c45 > 0)
      cost += segcounts[4] * vp9_cost_zero(probs[5]) +
              segcounts[5] * vp9_cost_one(probs[5]);
    if (c67 > 0)
      cost += segcounts[6] * vp9_cost_zero(probs[6]) +
              segcounts[7] * vp9_cost_one(probs[6]);
  }

  return cost;
}

static void count_segs(VP9_COMP *cpi, const TileInfo *const tile,
                       MODE_INFO **mi_8x8,
                       int *no_pred_segcounts,
                       int (*temporal_predictor_count)[2],
                       int *t_unpred_seg_counts,
                       int bw, int bh, int mi_row, int mi_col) {
  VP9_COMMON *const cm = &cpi->common;
  MACROBLOCKD *const xd = &cpi->mb.e_mbd;
  int segment_id;

  if (mi_row >= cm->mi_rows || mi_col >= cm->mi_cols)
    return;

  xd->mi_8x8 = mi_8x8;
  segment_id = xd->mi_8x8[0]->mbmi.segment_id;

  set_mi_row_col(xd, tile, mi_row, bh, mi_col, bw, cm->mi_rows, cm->mi_cols);

  
  no_pred_segcounts[segment_id]++;

  
  if (cm->frame_type != KEY_FRAME) {
    const BLOCK_SIZE bsize = mi_8x8[0]->mbmi.sb_type;
    
    const int pred_segment_id = vp9_get_segment_id(cm, cm->last_frame_seg_map,
                                                   bsize, mi_row, mi_col);
    const int pred_flag = pred_segment_id == segment_id;
    const int pred_context = vp9_get_pred_context_seg_id(xd);

    
    
    xd->mi_8x8[0]->mbmi.seg_id_predicted = pred_flag;
    temporal_predictor_count[pred_context][pred_flag]++;

    if (!pred_flag)
      
      t_unpred_seg_counts[segment_id]++;
  }
}

static void count_segs_sb(VP9_COMP *cpi, const TileInfo *const tile,
                          MODE_INFO **mi_8x8,
                          int *no_pred_segcounts,
                          int (*temporal_predictor_count)[2],
                          int *t_unpred_seg_counts,
                          int mi_row, int mi_col,
                          BLOCK_SIZE bsize) {
  const VP9_COMMON *const cm = &cpi->common;
  const int mis = cm->mode_info_stride;
  int bw, bh;
  const int bs = num_8x8_blocks_wide_lookup[bsize], hbs = bs / 2;

  if (mi_row >= cm->mi_rows || mi_col >= cm->mi_cols)
    return;

  bw = num_8x8_blocks_wide_lookup[mi_8x8[0]->mbmi.sb_type];
  bh = num_8x8_blocks_high_lookup[mi_8x8[0]->mbmi.sb_type];

  if (bw == bs && bh == bs) {
    count_segs(cpi, tile, mi_8x8, no_pred_segcounts, temporal_predictor_count,
               t_unpred_seg_counts, bs, bs, mi_row, mi_col);
  } else if (bw == bs && bh < bs) {
    count_segs(cpi, tile, mi_8x8, no_pred_segcounts, temporal_predictor_count,
               t_unpred_seg_counts, bs, hbs, mi_row, mi_col);
    count_segs(cpi, tile, mi_8x8 + hbs * mis, no_pred_segcounts,
               temporal_predictor_count, t_unpred_seg_counts, bs, hbs,
               mi_row + hbs, mi_col);
  } else if (bw < bs && bh == bs) {
    count_segs(cpi, tile, mi_8x8, no_pred_segcounts, temporal_predictor_count,
               t_unpred_seg_counts, hbs, bs, mi_row, mi_col);
    count_segs(cpi, tile, mi_8x8 + hbs,
               no_pred_segcounts, temporal_predictor_count, t_unpred_seg_counts,
               hbs, bs, mi_row, mi_col + hbs);
  } else {
    const BLOCK_SIZE subsize = subsize_lookup[PARTITION_SPLIT][bsize];
    int n;

    assert(bw < bs && bh < bs);

    for (n = 0; n < 4; n++) {
      const int mi_dc = hbs * (n & 1);
      const int mi_dr = hbs * (n >> 1);

      count_segs_sb(cpi, tile, &mi_8x8[mi_dr * mis + mi_dc],
                    no_pred_segcounts, temporal_predictor_count,
                    t_unpred_seg_counts,
                    mi_row + mi_dr, mi_col + mi_dc, subsize);
    }
  }
}

void vp9_choose_segmap_coding_method(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  struct segmentation *seg = &cm->seg;

  int no_pred_cost;
  int t_pred_cost = INT_MAX;

  int i, tile_col, mi_row, mi_col;

  int temporal_predictor_count[PREDICTION_PROBS][2] = { { 0 } };
  int no_pred_segcounts[MAX_SEGMENTS] = { 0 };
  int t_unpred_seg_counts[MAX_SEGMENTS] = { 0 };

  vp9_prob no_pred_tree[SEG_TREE_PROBS];
  vp9_prob t_pred_tree[SEG_TREE_PROBS];
  vp9_prob t_nopred_prob[PREDICTION_PROBS];

  const int mis = cm->mode_info_stride;
  MODE_INFO **mi_ptr, **mi;

  
  
  vpx_memset(seg->tree_probs, 255, sizeof(seg->tree_probs));
  vpx_memset(seg->pred_probs, 255, sizeof(seg->pred_probs));

  
  
  for (tile_col = 0; tile_col < 1 << cm->log2_tile_cols; tile_col++) {
    TileInfo tile;

    vp9_tile_init(&tile, cm, 0, tile_col);
    mi_ptr = cm->mi_grid_visible + tile.mi_col_start;
    for (mi_row = 0; mi_row < cm->mi_rows;
         mi_row += 8, mi_ptr += 8 * mis) {
      mi = mi_ptr;
      for (mi_col = tile.mi_col_start; mi_col < tile.mi_col_end;
           mi_col += 8, mi += 8)
        count_segs_sb(cpi, &tile, mi, no_pred_segcounts,
                      temporal_predictor_count, t_unpred_seg_counts,
                      mi_row, mi_col, BLOCK_64X64);
    }
  }

  
  
  calc_segtree_probs(no_pred_segcounts, no_pred_tree);
  no_pred_cost = cost_segmap(no_pred_segcounts, no_pred_tree);

  
  if (!frame_is_intra_only(cm)) {
    
    
    calc_segtree_probs(t_unpred_seg_counts, t_pred_tree);
    t_pred_cost = cost_segmap(t_unpred_seg_counts, t_pred_tree);

    
    for (i = 0; i < PREDICTION_PROBS; i++) {
      const int count0 = temporal_predictor_count[i][0];
      const int count1 = temporal_predictor_count[i][1];

      t_nopred_prob[i] = get_binary_prob(count0, count1);

      
      t_pred_cost += count0 * vp9_cost_zero(t_nopred_prob[i]) +
                     count1 * vp9_cost_one(t_nopred_prob[i]);
    }
  }

  
  if (t_pred_cost < no_pred_cost) {
    seg->temporal_update = 1;
    vpx_memcpy(seg->tree_probs, t_pred_tree, sizeof(t_pred_tree));
    vpx_memcpy(seg->pred_probs, t_nopred_prob, sizeof(t_nopred_prob));
  } else {
    seg->temporal_update = 0;
    vpx_memcpy(seg->tree_probs, no_pred_tree, sizeof(no_pred_tree));
  }
}

void vp9_reset_segment_features(struct segmentation *seg) {
  
  seg->enabled = 0;
  seg->update_map = 0;
  seg->update_data = 0;
  vpx_memset(seg->tree_probs, 255, sizeof(seg->tree_probs));
  vp9_clearall_segfeatures(seg);
}
