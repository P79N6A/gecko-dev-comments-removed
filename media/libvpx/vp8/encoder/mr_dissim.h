










#ifndef VP8_ENCODER_MR_DISSIM_H_
#define VP8_ENCODER_MR_DISSIM_H_
#include "vpx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void vp8_cal_low_res_mb_cols(VP8_COMP *cpi);
extern void vp8_cal_dissimilarity(VP8_COMP *cpi);
extern void vp8_store_drop_frame_info(VP8_COMP *cpi);

#ifdef __cplusplus
}  
#endif

#endif
