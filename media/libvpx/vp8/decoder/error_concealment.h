










#ifndef ERROR_CONCEALMENT_H
#define ERROR_CONCEALMENT_H

#include "onyxd_int.h"
#include "ec_types.h"


int vp8_alloc_overlap_lists(VP8D_COMP *pbi);


void vp8_de_alloc_overlap_lists(VP8D_COMP *pbi);


void vp8_estimate_missing_mvs(VP8D_COMP *pbi);





void vp8_interpolate_motion(MACROBLOCKD *mb,
                            int mb_row, int mb_col,
                            int mb_rows, int mb_cols,
                            int mi_stride);




void vp8_conceal_corrupt_mb(MACROBLOCKD *xd);

#endif
