










#ifndef VP8_DECODER_ERROR_CONCEALMENT_H_
#define VP8_DECODER_ERROR_CONCEALMENT_H_

#include "onyxd_int.h"
#include "ec_types.h"

#ifdef __cplusplus
extern "C" {
#endif


int vp8_alloc_overlap_lists(VP8D_COMP *pbi);


void vp8_de_alloc_overlap_lists(VP8D_COMP *pbi);


void vp8_estimate_missing_mvs(VP8D_COMP *pbi);





void vp8_interpolate_motion(MACROBLOCKD *mb,
                            int mb_row, int mb_col,
                            int mb_rows, int mb_cols,
                            int mi_stride);




void vp8_conceal_corrupt_mb(MACROBLOCKD *xd);

#ifdef __cplusplus
}  
#endif

#endif
