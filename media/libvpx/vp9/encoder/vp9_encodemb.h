









#ifndef VP9_ENCODER_VP9_ENCODEMB_H_
#define VP9_ENCODER_VP9_ENCODEMB_H_

#include "./vpx_config.h"
#include "vp9/encoder/vp9_block.h"

#ifdef __cplusplus
extern "C" {
#endif

struct encode_b_args {
  MACROBLOCK *x;
  struct optimize_ctx *ctx;
  int8_t *skip;
};
void vp9_encode_sb(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_encode_sby_pass1(MACROBLOCK *x, BLOCK_SIZE bsize);
void vp9_xform_quant_fp(MACROBLOCK *x, int plane, int block,
                        BLOCK_SIZE plane_bsize, TX_SIZE tx_size);
void vp9_xform_quant_dc(MACROBLOCK *x, int plane, int block,
                        BLOCK_SIZE plane_bsize, TX_SIZE tx_size);
void vp9_xform_quant(MACROBLOCK *x, int plane, int block,
                     BLOCK_SIZE plane_bsize, TX_SIZE tx_size);

void vp9_subtract_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);

void vp9_encode_block_intra(int plane, int block, BLOCK_SIZE plane_bsize,
                            TX_SIZE tx_size, void *arg);

void vp9_encode_intra_block_plane(MACROBLOCK *x, BLOCK_SIZE bsize, int plane);

#ifdef __cplusplus
}  
#endif

#endif
