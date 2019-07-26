










#ifndef __INC_PICKINTER_H
#define __INC_PICKINTER_H
#include "vpx_config.h"
#include "vp8/common/onyxc_int.h"

extern void vp8_pick_inter_mode(VP8_COMP *cpi, MACROBLOCK *x, int recon_yoffset,
                                int recon_uvoffset, int *returnrate,
                                int *returndistortion, int *returnintra,
                                int mb_row, int mb_col);
extern void vp8_pick_intra_mode(MACROBLOCK *x, int *rate);

extern int vp8_get_inter_mbpred_error(MACROBLOCK *mb,
                                      const vp8_variance_fn_ptr_t *vfp,
                                      unsigned int *sse,
                                      int_mv this_mv);
#endif
