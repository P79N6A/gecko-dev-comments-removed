










#ifndef VP9_ENCODER_VP9_AQ_VARIANCE_H_
#define VP9_ENCODER_VP9_AQ_VARIANCE_H_

#include "vp9/encoder/vp9_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int vp9_vaq_segment_id(int energy);
void vp9_vaq_frame_setup(VP9_COMP *cpi);

int vp9_block_energy(VP9_COMP *cpi, MACROBLOCK *x, BLOCK_SIZE bs);
double vp9_log_block_var(VP9_COMP *cpi, MACROBLOCK *x, BLOCK_SIZE bs);

#ifdef __cplusplus
}  
#endif

#endif
