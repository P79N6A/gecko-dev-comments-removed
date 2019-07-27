










#ifndef VP9_ENCODER_VP9_AQ_COMPLEXITY_H_
#define VP9_ENCODER_VP9_AQ_COMPLEXITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vp9/common/vp9_enums.h"

struct VP9_COMP;
struct macroblock;


void vp9_caq_select_segment(struct VP9_COMP *cpi, struct macroblock *,
                            BLOCK_SIZE bs,
                            int mi_row, int mi_col, int projected_rate);



void vp9_setup_in_frame_q_adj(struct VP9_COMP *cpi);

#ifdef __cplusplus
}  
#endif

#endif
