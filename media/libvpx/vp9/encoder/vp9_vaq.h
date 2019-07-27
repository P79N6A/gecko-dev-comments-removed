










#ifndef VP9_ENCODER_VP9_VAQ_H_
#define VP9_ENCODER_VP9_VAQ_H_

#include "vp9/encoder/vp9_onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int vp9_vaq_segment_id(int energy);
double vp9_vaq_rdmult_ratio(int energy);
double vp9_vaq_inv_q_ratio(int energy);

void vp9_vaq_init();
void vp9_vaq_frame_setup(VP9_COMP *cpi);

int vp9_block_energy(VP9_COMP *cpi, MACROBLOCK *x, BLOCK_SIZE bs);

#ifdef __cplusplus
}  
#endif

#endif
