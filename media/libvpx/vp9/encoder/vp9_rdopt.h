









#ifndef VP9_ENCODER_VP9_RDOPT_H_
#define VP9_ENCODER_VP9_RDOPT_H_

#include "vp9/common/vp9_blockd.h"

#include "vp9/encoder/vp9_block.h"
#include "vp9/encoder/vp9_context_tree.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TileInfo;
struct VP9_COMP;
struct macroblock;
struct RD_COST;

void vp9_rd_pick_intra_mode_sb(struct VP9_COMP *cpi, struct macroblock *x,
                               struct RD_COST *rd_cost, BLOCK_SIZE bsize,
                               PICK_MODE_CONTEXT *ctx, int64_t best_rd);

void vp9_rd_pick_inter_mode_sb(struct VP9_COMP *cpi,
                               struct TileDataEnc *tile_data,
                               struct macroblock *x,
                               int mi_row, int mi_col,
                               struct RD_COST *rd_cost,
                               BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
                               int64_t best_rd_so_far);

void vp9_rd_pick_inter_mode_sb_seg_skip(struct VP9_COMP *cpi,
                                        struct TileDataEnc *tile_data,
                                        struct macroblock *x,
                                        struct RD_COST *rd_cost,
                                        BLOCK_SIZE bsize,
                                        PICK_MODE_CONTEXT *ctx,
                                        int64_t best_rd_so_far);

void vp9_rd_pick_inter_mode_sub8x8(struct VP9_COMP *cpi,
                                   struct TileDataEnc *tile_data,
                                   struct macroblock *x,
                                   int mi_row, int mi_col,
                                   struct RD_COST *rd_cost,
                                   BLOCK_SIZE bsize, PICK_MODE_CONTEXT *ctx,
                                   int64_t best_rd_so_far);
#ifdef __cplusplus
}  
#endif

#endif
