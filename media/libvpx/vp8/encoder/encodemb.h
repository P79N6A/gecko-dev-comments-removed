










#ifndef VP8_ENCODER_ENCODEMB_H_
#define VP8_ENCODER_ENCODEMB_H_

#include "onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif
void vp8_encode_inter16x16(MACROBLOCK *x);

void vp8_build_dcblock(MACROBLOCK *b);
void vp8_transform_mb(MACROBLOCK *mb);
void vp8_transform_mbuv(MACROBLOCK *x);
void vp8_transform_intra_mby(MACROBLOCK *x);

void vp8_optimize_mby(MACROBLOCK *x);
void vp8_optimize_mbuv(MACROBLOCK *x);
void vp8_encode_inter16x16y(MACROBLOCK *x);
#ifdef __cplusplus
}  
#endif

#endif
