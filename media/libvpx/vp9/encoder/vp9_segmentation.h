










#ifndef VP9_ENCODER_VP9_SEGMENTATION_H_
#define VP9_ENCODER_VP9_SEGMENTATION_H_

#include "vp9/common/vp9_blockd.h"
#include "vp9/encoder/vp9_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_enable_segmentation(struct segmentation *seg);
void vp9_disable_segmentation(struct segmentation *seg);

void vp9_disable_segfeature(struct segmentation *seg,
                            int segment_id,
                            SEG_LVL_FEATURES feature_id);
void vp9_clear_segdata(struct segmentation *seg,
                       int segment_id,
                       SEG_LVL_FEATURES feature_id);











void vp9_set_segment_data(struct segmentation *seg, signed char *feature_data,
                          unsigned char abs_delta);

void vp9_choose_segmap_coding_method(VP9_COMMON *cm, MACROBLOCKD *xd);

void vp9_reset_segment_features(struct segmentation *seg);

#ifdef __cplusplus
}  
#endif

#endif
