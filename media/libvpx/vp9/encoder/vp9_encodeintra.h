









#ifndef VP9_ENCODER_VP9_ENCODEINTRA_H_
#define VP9_ENCODER_VP9_ENCODEINTRA_H_

#include "vp9/encoder/vp9_onyx_int.h"

int vp9_encode_intra(MACROBLOCK *x, int use_16x16_pred);
void vp9_encode_block_intra(int plane, int block, BLOCK_SIZE plane_bsize,
                            TX_SIZE tx_size, void *arg);

#endif  
