










#ifndef VP9_ENCODER_VP9_ENCODEMV_H_
#define VP9_ENCODER_VP9_ENCODEMV_H_

#include "vp9/encoder/vp9_onyx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_entropy_mv_init();

void vp9_write_nmv_probs(VP9_COMMON *cm, int usehp, vp9_writer *w);

void vp9_encode_mv(VP9_COMP *cpi, vp9_writer* w, const MV* mv, const MV* ref,
                   const nmv_context* mvctx, int usehp);

void vp9_build_nmv_cost_table(int *mvjoint,
                              int *mvcost[2],
                              const nmv_context* const mvctx,
                              int usehp,
                              int mvc_flag_v,
                              int mvc_flag_h);

void vp9_update_mv_count(VP9_COMP *cpi, MACROBLOCK *x, int_mv best_ref_mv[2]);

#ifdef __cplusplus
}  
#endif

#endif
