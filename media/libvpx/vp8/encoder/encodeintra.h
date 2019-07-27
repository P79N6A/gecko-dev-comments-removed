










#ifndef VP8_ENCODER_ENCODEINTRA_H_
#define VP8_ENCODER_ENCODEINTRA_H_
#include "onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

int vp8_encode_intra(VP8_COMP *cpi, MACROBLOCK *x, int use_dc_pred);
void vp8_encode_intra16x16mby(MACROBLOCK *x);
void vp8_encode_intra16x16mbuv(MACROBLOCK *x);
void vp8_encode_intra4x4mby(MACROBLOCK *mb);
void vp8_encode_intra4x4block(MACROBLOCK *x, int ib);
#ifdef __cplusplus
}  
#endif

#endif
