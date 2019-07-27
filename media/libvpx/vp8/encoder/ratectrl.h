










#ifndef VP8_ENCODER_RATECTRL_H_
#define VP8_ENCODER_RATECTRL_H_

#include "onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void vp8_save_coding_context(VP8_COMP *cpi);
extern void vp8_restore_coding_context(VP8_COMP *cpi);

extern void vp8_setup_key_frame(VP8_COMP *cpi);
extern void vp8_update_rate_correction_factors(VP8_COMP *cpi, int damp_var);
extern int vp8_regulate_q(VP8_COMP *cpi, int target_bits_per_frame);
extern void vp8_adjust_key_frame_context(VP8_COMP *cpi);
extern void vp8_compute_frame_size_bounds(VP8_COMP *cpi, int *frame_under_shoot_limit, int *frame_over_shoot_limit);


extern int vp8_pick_frame_size(VP8_COMP *cpi);

extern int vp8_drop_encodedframe_overshoot(VP8_COMP *cpi, int Q);

#ifdef __cplusplus
}  
#endif

#endif
