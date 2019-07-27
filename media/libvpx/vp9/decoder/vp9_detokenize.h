










#ifndef VP9_DECODER_VP9_DETOKENIZE_H_
#define VP9_DECODER_VP9_DETOKENIZE_H_

#include "vp9/decoder/vp9_onyxd_int.h"
#include "vp9/decoder/vp9_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

int vp9_decode_block_tokens(VP9_COMMON *cm, MACROBLOCKD *xd,
                            int plane, int block, BLOCK_SIZE plane_bsize,
                            int x, int y, TX_SIZE tx_size, vp9_reader *r);

#ifdef __cplusplus
}  
#endif

#endif
