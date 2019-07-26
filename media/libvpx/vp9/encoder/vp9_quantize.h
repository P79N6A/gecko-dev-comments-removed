









#ifndef VP9_ENCODER_VP9_QUANTIZE_H_
#define VP9_ENCODER_VP9_QUANTIZE_H_

#include "vp9/encoder/vp9_block.h"

void vp9_regular_quantize_b_4x4(MACROBLOCK *x, int y_blocks, int b_idx,
                                const int16_t *scan, const int16_t *iscan);

struct VP9_COMP;

void vp9_set_quantizer(struct VP9_COMP *cpi, int q);

void vp9_frame_init_quantizer(struct VP9_COMP *cpi);

void vp9_update_zbin_extra(struct VP9_COMP *cpi, MACROBLOCK *x);

void vp9_mb_init_quantizer(struct VP9_COMP *cpi, MACROBLOCK *x);

void vp9_init_quantizer(struct VP9_COMP *cpi);

#endif  
