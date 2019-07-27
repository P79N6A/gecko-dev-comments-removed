









#ifndef VP9_ENCODER_VP9_PICKMODE_H_
#define VP9_ENCODER_VP9_PICKMODE_H_

#include "vp9/encoder/vp9_onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

int64_t vp9_pick_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                            const struct TileInfo *const tile,
                            int mi_row, int mi_col,
                            int *returnrate,
                            int64_t *returndistortion,
                            BLOCK_SIZE bsize);

#ifdef __cplusplus
}  
#endif

#endif
