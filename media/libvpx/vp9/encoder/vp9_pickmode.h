









#ifndef VP9_ENCODER_VP9_PICKMODE_H_
#define VP9_ENCODER_VP9_PICKMODE_H_

#include "vp9/encoder/vp9_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_pick_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                         const struct TileInfo *const tile,
                         int mi_row, int mi_col,
                         int *returnrate,
                         int64_t *returndistortion,
                         BLOCK_SIZE bsize,
                         PICK_MODE_CONTEXT *ctx);

#ifdef __cplusplus
}  
#endif

#endif
