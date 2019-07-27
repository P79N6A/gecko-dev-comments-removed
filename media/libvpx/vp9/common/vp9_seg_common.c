









#include <assert.h>

#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_quant_common.h"

static const int seg_feature_data_signed[SEG_LVL_MAX] = { 1, 1, 0, 0 };

static const int seg_feature_data_max[SEG_LVL_MAX] = {
  MAXQ, MAX_LOOP_FILTER, 3, 0 };






int vp9_segfeature_active(const struct segmentation *seg, int segment_id,
                          SEG_LVL_FEATURES feature_id) {
  return seg->enabled &&
         (seg->feature_mask[segment_id] & (1 << feature_id));
}

void vp9_clearall_segfeatures(struct segmentation *seg) {
  vp9_zero(seg->feature_data);
  vp9_zero(seg->feature_mask);
}

void vp9_enable_segfeature(struct segmentation *seg, int segment_id,
                           SEG_LVL_FEATURES feature_id) {
  seg->feature_mask[segment_id] |= 1 << feature_id;
}

int vp9_seg_feature_data_max(SEG_LVL_FEATURES feature_id) {
  return seg_feature_data_max[feature_id];
}

int vp9_is_segfeature_signed(SEG_LVL_FEATURES feature_id) {
  return seg_feature_data_signed[feature_id];
}

void vp9_set_segdata(struct segmentation *seg, int segment_id,
                     SEG_LVL_FEATURES feature_id, int seg_data) {
  assert(seg_data <= seg_feature_data_max[feature_id]);
  if (seg_data < 0) {
    assert(seg_feature_data_signed[feature_id]);
    assert(-seg_data <= seg_feature_data_max[feature_id]);
  }

  seg->feature_data[segment_id][feature_id] = seg_data;
}

int vp9_get_segdata(const struct segmentation *seg, int segment_id,
                    SEG_LVL_FEATURES feature_id) {
  return seg->feature_data[segment_id][feature_id];
}


const vp9_tree_index vp9_segment_tree[TREE_SIZE(MAX_SEGMENTS)] = {
  2,  4,  6,  8, 10, 12,
  0, -1, -2, -3, -4, -5, -6, -7
};



