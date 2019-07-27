









#ifndef VP8_DECODER_DETOKENIZE_H_
#define VP8_DECODER_DETOKENIZE_H_

#include "onyxd_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp8_reset_mb_tokens_context(MACROBLOCKD *x);
int vp8_decode_mb_tokens(VP8D_COMP *, MACROBLOCKD *);

#ifdef __cplusplus
}  
#endif

#endif
