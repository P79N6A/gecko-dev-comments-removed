









#ifndef VP9_ENCODER_VP9_PICKMODE_H_
#define VP9_ENCODER_VP9_PICKMODE_H_

#include "vp9/encoder/vp9_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_pick_intra_mode(VP9_COMP *cpi, MACROBLOCK *x, RD_COST *rd_cost,
                         BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx);

void vp9_pick_inter_mode(VP9_COMP *cpi, MACROBLOCK *x,
                         TileDataEnc *tile_data,
                         int mi_row, int mi_col, RD_COST *rd_cost,
                         BLOCK_SIZE bsize,
                         PICK_MODE_CONTEXT *ctx);

void vp9_pick_inter_mode_sub8x8(VP9_COMP *cpi, MACROBLOCK *x,
                                TileDataEnc *tile_data,
                                int mi_row, int mi_col, RD_COST *rd_cost,
                                BLOCK_SIZE bsize,
                                PICK_MODE_CONTEXT *ctx);

#ifdef __cplusplus
}  
#endif

#endif
