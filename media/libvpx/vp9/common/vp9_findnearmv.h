










#ifndef VP9_COMMON_VP9_FINDNEARMV_H_
#define VP9_COMMON_VP9_FINDNEARMV_H_

#include "vp9/common/vp9_mv.h"
#include "vp9/common/vp9_blockd.h"
#include "vp9/common/vp9_treecoder.h"
#include "vp9/common/vp9_onyxc_int.h"

#define LEFT_TOP_MARGIN     ((VP9BORDERINPIXELS - VP9_INTERP_EXTEND) << 3)
#define RIGHT_BOTTOM_MARGIN ((VP9BORDERINPIXELS - VP9_INTERP_EXTEND) << 3)




void vp9_find_best_ref_mvs(MACROBLOCKD *xd, int allow_hp,
                           int_mv *mvlist, int_mv *nearest, int_mv *near);


static void clamp_mv2(MV *mv, const MACROBLOCKD *xd) {
  clamp_mv(mv, xd->mb_to_left_edge - LEFT_TOP_MARGIN,
               xd->mb_to_right_edge + RIGHT_BOTTOM_MARGIN,
               xd->mb_to_top_edge - LEFT_TOP_MARGIN,
               xd->mb_to_bottom_edge + RIGHT_BOTTOM_MARGIN);
}

void vp9_append_sub8x8_mvs_for_idx(VP9_COMMON *cm, MACROBLOCKD *xd,
                                   const TileInfo *const tile,
                                   int_mv *dst_nearest,
                                   int_mv *dst_near,
                                   int block_idx, int ref_idx,
                                   int mi_row, int mi_col);

static MB_PREDICTION_MODE left_block_mode(const MODE_INFO *cur_mi,
                                          const MODE_INFO *left_mi, int b) {
  if (b == 0 || b == 2) {
    if (!left_mi || is_inter_block(&left_mi->mbmi))
      return DC_PRED;

    return left_mi->mbmi.sb_type < BLOCK_8X8 ? left_mi->bmi[b + 1].as_mode
                                             : left_mi->mbmi.mode;
  } else {
    assert(b == 1 || b == 3);
    return cur_mi->bmi[b - 1].as_mode;
  }
}

static MB_PREDICTION_MODE above_block_mode(const MODE_INFO *cur_mi,
                                           const MODE_INFO *above_mi, int b) {
  if (b == 0 || b == 1) {
    if (!above_mi || is_inter_block(&above_mi->mbmi))
      return DC_PRED;

    return above_mi->mbmi.sb_type < BLOCK_8X8 ? above_mi->bmi[b + 2].as_mode
                                              : above_mi->mbmi.mode;
  } else {
    assert(b == 2 || b == 3);
    return cur_mi->bmi[b - 2].as_mode;
  }
}

#endif  
