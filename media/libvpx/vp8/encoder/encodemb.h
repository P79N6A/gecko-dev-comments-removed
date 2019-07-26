










#ifndef __INC_ENCODEMB_H
#define __INC_ENCODEMB_H

#include "onyx_int.h"
void vp8_encode_inter16x16(MACROBLOCK *x);

void vp8_build_dcblock(MACROBLOCK *b);
void vp8_transform_mb(MACROBLOCK *mb);
void vp8_transform_mbuv(MACROBLOCK *x);
void vp8_transform_intra_mby(MACROBLOCK *x);

void vp8_optimize_mby(MACROBLOCK *x);
void vp8_optimize_mbuv(MACROBLOCK *x);
void vp8_encode_inter16x16y(MACROBLOCK *x);
#endif
