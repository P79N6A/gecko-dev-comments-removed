










#ifndef VP9_ENCODER_VP9_AQ_CYCLICREFRESH_H_
#define VP9_ENCODER_VP9_AQ_CYCLICREFRESH_H_

#include "vp9/common/vp9_blockd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VP9_COMP;

struct CYCLIC_REFRESH;
typedef struct CYCLIC_REFRESH CYCLIC_REFRESH;

CYCLIC_REFRESH *vp9_cyclic_refresh_alloc(int mi_rows, int mi_cols);

void vp9_cyclic_refresh_free(CYCLIC_REFRESH *cr);




void vp9_cyclic_refresh_update_segment(struct VP9_COMP *const cpi,
                                       MB_MODE_INFO *const mbmi,
                                       int mi_row, int mi_col,
                                       BLOCK_SIZE bsize, int use_rd);


void vp9_cyclic_refresh_setup(struct VP9_COMP *const cpi);

void vp9_cyclic_refresh_set_rate_and_dist_sb(CYCLIC_REFRESH *cr,
                                             int64_t rate_sb, int64_t dist_sb);

int vp9_cyclic_refresh_get_rdmult(const CYCLIC_REFRESH *cr);

#ifdef __cplusplus
}  
#endif

#endif
