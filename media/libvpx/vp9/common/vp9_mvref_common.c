










#include "vp9/common/vp9_mvref_common.h"

#define MVREF_NEIGHBOURS 8

typedef struct position {
  int row;
  int col;
} POSITION;

typedef enum {
  BOTH_ZERO = 0,
  ZERO_PLUS_PREDICTED = 1,
  BOTH_PREDICTED = 2,
  NEW_PLUS_NON_INTRA = 3,
  BOTH_NEW = 4,
  INTRA_PLUS_NON_INTRA = 5,
  BOTH_INTRA = 6,
  INVALID_CASE = 9
} motion_vector_context;






static const int mode_2_counter[MB_MODE_COUNT] = {
  9,  
  9,  
  9,  
  9,  
  9,  
  9,  
  9,  
  9,  
  9,  
  9,  
  0,  
  0,  
  3,  
  1,  
};




static const int counter_to_context[19] = {
  BOTH_PREDICTED,  
  NEW_PLUS_NON_INTRA,  
  BOTH_NEW,  
  ZERO_PLUS_PREDICTED,  
  NEW_PLUS_NON_INTRA,  
  INVALID_CASE,  
  BOTH_ZERO,  
  INVALID_CASE,  
  INVALID_CASE,  
  INTRA_PLUS_NON_INTRA,  
  INTRA_PLUS_NON_INTRA,  
  INVALID_CASE,  
  INTRA_PLUS_NON_INTRA,  
  INVALID_CASE,  
  INVALID_CASE,  
  INVALID_CASE,  
  INVALID_CASE,  
  INVALID_CASE,  
  BOTH_INTRA  
};

static const POSITION mv_ref_blocks[BLOCK_SIZES][MVREF_NEIGHBOURS] = {
  
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  
  {{-1, 0}, {0, -1}, {-1, -1}, {-2, 0}, {0, -2}, {-2, -1}, {-1, -2}, {-2, -2}},
  
  {{0, -1}, {-1, 0}, {1, -1}, {-1, -1}, {0, -2}, {-2, 0}, {-2, -1}, {-1, -2}},
  
  {{-1, 0}, {0, -1}, {-1, 1}, {-1, -1}, {-2, 0}, {0, -2}, {-1, -2}, {-2, -1}},
  
  {{-1, 0}, {0, -1}, {-1, 1}, {1, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  
  {{0, -1}, {-1, 0}, {2, -1}, {-1, -1}, {-1, 1}, {0, -3}, {-3, 0}, {-3, -3}},
  
  {{-1, 0}, {0, -1}, {-1, 2}, {-1, -1}, {1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  
  {{-1, 1}, {1, -1}, {-1, 2}, {2, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-3, -3}},
  
  {{0, -1}, {-1, 0}, {4, -1}, {-1, 2}, {-1, -1}, {0, -3}, {-3, 0}, {2, -1}},
  
  {{-1, 0}, {0, -1}, {-1, 4}, {2, -1}, {-1, -1}, {-3, 0}, {0, -3}, {-1, 2}},
  
  {{-1, 3}, {3, -1}, {-1, 4}, {4, -1}, {-1, -1}, {-1, 0}, {0, -1}, {-1, 6}}
};

static const int idx_n_column_to_subblock[4][2] = {
  {1, 2},
  {1, 3},
  {3, 2},
  {3, 3}
};


#define MV_BORDER (16 << 3)  // Allow 16 pels in 1/8th pel units

static void clamp_mv_ref(MV *mv, const MACROBLOCKD *xd) {
  clamp_mv(mv, xd->mb_to_left_edge - MV_BORDER,
               xd->mb_to_right_edge + MV_BORDER,
               xd->mb_to_top_edge - MV_BORDER,
               xd->mb_to_bottom_edge + MV_BORDER);
}



static INLINE int_mv get_sub_block_mv(const MODE_INFO *candidate, int which_mv,
                                      int search_col, int block_idx) {
  return block_idx >= 0 && candidate->mbmi.sb_type < BLOCK_8X8
          ? candidate->bmi[idx_n_column_to_subblock[block_idx][search_col == 0]]
              .as_mv[which_mv]
          : candidate->mbmi.mv[which_mv];
}



static INLINE int_mv scale_mv(const MB_MODE_INFO *mbmi, int ref,
                              const MV_REFERENCE_FRAME this_ref_frame,
                              const int *ref_sign_bias) {
  int_mv mv = mbmi->mv[ref];
  if (ref_sign_bias[mbmi->ref_frame[ref]] != ref_sign_bias[this_ref_frame]) {
    mv.as_mv.row *= -1;
    mv.as_mv.col *= -1;
  }
  return mv;
}




#define ADD_MV_REF_LIST(MV) \
  do { \
    if (refmv_count) { \
      if ((MV).as_int != mv_ref_list[0].as_int) { \
        mv_ref_list[refmv_count] = (MV); \
        goto Done; \
      } \
    } else { \
      mv_ref_list[refmv_count++] = (MV); \
    } \
  } while (0)



#define IF_DIFF_REF_FRAME_ADD_MV(CANDIDATE) \
  do { \
    if ((CANDIDATE)->ref_frame[0] != ref_frame) \
      ADD_MV_REF_LIST(scale_mv((CANDIDATE), 0, ref_frame, ref_sign_bias)); \
    if ((CANDIDATE)->ref_frame[1] != ref_frame && \
        has_second_ref(CANDIDATE) && \
        (CANDIDATE)->mv[1].as_int != (CANDIDATE)->mv[0].as_int) \
      ADD_MV_REF_LIST(scale_mv((CANDIDATE), 1, ref_frame, ref_sign_bias)); \
  } while (0)




static INLINE int is_inside(const TileInfo *const tile,
                            int mi_col, int mi_row, int mi_rows,
                            const POSITION *mi_pos) {
  return !(mi_row + mi_pos->row < 0 ||
           mi_col + mi_pos->col < tile->mi_col_start ||
           mi_row + mi_pos->row >= mi_rows ||
           mi_col + mi_pos->col >= tile->mi_col_end);
}



void vp9_find_mv_refs_idx(const VP9_COMMON *cm, const MACROBLOCKD *xd,
                          const TileInfo *const tile,
                          MODE_INFO *mi, const MODE_INFO *prev_mi,
                          MV_REFERENCE_FRAME ref_frame,
                          int_mv *mv_ref_list,
                          int block_idx,
                          int mi_row, int mi_col) {
  const int *ref_sign_bias = cm->ref_frame_sign_bias;
  int i, refmv_count = 0;
  const POSITION *const mv_ref_search = mv_ref_blocks[mi->mbmi.sb_type];
  const MB_MODE_INFO *const prev_mbmi = prev_mi ? &prev_mi->mbmi : NULL;
  int different_ref_found = 0;
  int context_counter = 0;

  
  vpx_memset(mv_ref_list, 0, sizeof(*mv_ref_list) * MAX_MV_REF_CANDIDATES);

  
  
  
  for (i = 0; i < 2; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MODE_INFO *const candidate_mi = xd->mi_8x8[mv_ref->col + mv_ref->row
                                                   * xd->mode_info_stride];
      const MB_MODE_INFO *const candidate = &candidate_mi->mbmi;
      
      context_counter += mode_2_counter[candidate->mode];

      
      if (candidate->ref_frame[0] == ref_frame) {
        ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 0,
                                         mv_ref->col, block_idx));
        different_ref_found = candidate->ref_frame[1] != ref_frame;
      } else {
        if (candidate->ref_frame[1] == ref_frame)
          
          ADD_MV_REF_LIST(get_sub_block_mv(candidate_mi, 1,
                                           mv_ref->col, block_idx));
        different_ref_found = 1;
      }
    }
  }

  
  
  
  for (; i < MVREF_NEIGHBOURS; ++i) {
    const POSITION *const mv_ref = &mv_ref_search[i];
    if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
      const MB_MODE_INFO *const candidate = &xd->mi_8x8[mv_ref->col +
                                            mv_ref->row
                                            * xd->mode_info_stride]->mbmi;

      if (candidate->ref_frame[0] == ref_frame) {
        ADD_MV_REF_LIST(candidate->mv[0]);
        different_ref_found = candidate->ref_frame[1] != ref_frame;
      } else {
        if (candidate->ref_frame[1] == ref_frame)
          ADD_MV_REF_LIST(candidate->mv[1]);
        different_ref_found = 1;
      }
    }
  }

  
  if (prev_mbmi) {
    if (prev_mbmi->ref_frame[0] == ref_frame)
      ADD_MV_REF_LIST(prev_mbmi->mv[0]);
    else if (prev_mbmi->ref_frame[1] == ref_frame)
      ADD_MV_REF_LIST(prev_mbmi->mv[1]);
  }

  
  
  
  if (different_ref_found) {
    for (i = 0; i < MVREF_NEIGHBOURS; ++i) {
      const POSITION *mv_ref = &mv_ref_search[i];
      if (is_inside(tile, mi_col, mi_row, cm->mi_rows, mv_ref)) {
        const MB_MODE_INFO *const candidate = &xd->mi_8x8[mv_ref->col +
                                                          mv_ref->row
                                              * xd->mode_info_stride]->mbmi;

        
        if (is_inter_block(candidate))
          IF_DIFF_REF_FRAME_ADD_MV(candidate);
      }
    }
  }

  
  if (prev_mbmi && is_inter_block(prev_mbmi))
    IF_DIFF_REF_FRAME_ADD_MV(prev_mbmi);

 Done:

  mi->mbmi.mode_context[ref_frame] = counter_to_context[context_counter];

  
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i)
    clamp_mv_ref(&mv_ref_list[i].as_mv, xd);
}

static void lower_mv_precision(MV *mv, int allow_hp) {
  const int use_hp = allow_hp && vp9_use_mv_hp(mv);
  if (!use_hp) {
    if (mv->row & 1)
      mv->row += (mv->row > 0 ? -1 : 1);
    if (mv->col & 1)
      mv->col += (mv->col > 0 ? -1 : 1);
  }
}


void vp9_find_best_ref_mvs(MACROBLOCKD *xd, int allow_hp,
                           int_mv *mvlist, int_mv *nearest, int_mv *near) {
  int i;
  
  for (i = 0; i < MAX_MV_REF_CANDIDATES; ++i) {
    lower_mv_precision(&mvlist[i].as_mv, allow_hp);
    clamp_mv2(&mvlist[i].as_mv, xd);
  }
  *nearest = mvlist[0];
  *near = mvlist[1];
}

void vp9_append_sub8x8_mvs_for_idx(VP9_COMMON *cm, MACROBLOCKD *xd,
                                   const TileInfo *const tile,
                                   int block, int ref, int mi_row, int mi_col,
                                   int_mv *nearest, int_mv *near) {
  int_mv mv_list[MAX_MV_REF_CANDIDATES];
  MODE_INFO *const mi = xd->mi_8x8[0];
  b_mode_info *bmi = mi->bmi;
  int n;

  assert(MAX_MV_REF_CANDIDATES == 2);

  vp9_find_mv_refs_idx(cm, xd, tile, mi, xd->last_mi, mi->mbmi.ref_frame[ref],
                       mv_list, block, mi_row, mi_col);

  near->as_int = 0;
  switch (block) {
    case 0:
      nearest->as_int = mv_list[0].as_int;
      near->as_int = mv_list[1].as_int;
      break;
    case 1:
    case 2:
      nearest->as_int = bmi[0].as_mv[ref].as_int;
      for (n = 0; n < MAX_MV_REF_CANDIDATES; ++n)
        if (nearest->as_int != mv_list[n].as_int) {
          near->as_int = mv_list[n].as_int;
          break;
        }
      break;
    case 3: {
      int_mv candidates[2 + MAX_MV_REF_CANDIDATES];
      candidates[0] = bmi[1].as_mv[ref];
      candidates[1] = bmi[0].as_mv[ref];
      candidates[2] = mv_list[0];
      candidates[3] = mv_list[1];

      nearest->as_int = bmi[2].as_mv[ref].as_int;
      for (n = 0; n < 2 + MAX_MV_REF_CANDIDATES; ++n)
        if (nearest->as_int != candidates[n].as_int) {
          near->as_int = candidates[n].as_int;
          break;
        }
      break;
    }
    default:
      assert("Invalid block index.");
  }
}
