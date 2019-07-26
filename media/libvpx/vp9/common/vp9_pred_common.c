










#include <limits.h>

#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_pred_common.h"
#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_treecoder.h"

static INLINE const MB_MODE_INFO *get_above_mbmi(const MODE_INFO *const above) {
  return (above != NULL) ? &above->mbmi : NULL;
}

static INLINE const MB_MODE_INFO *get_left_mbmi(const MODE_INFO *const left) {
  return (left != NULL) ? &left->mbmi : NULL;
}


unsigned char vp9_get_pred_context_switchable_interp(const MACROBLOCKD *xd) {
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  
  
  
  
  
  const int left_mv_pred = left_in_image ? is_inter_block(&left_mi->mbmi)
                                         : 0;
  const int left_interp = left_in_image && left_mv_pred
                              ? left_mi->mbmi.interp_filter
                              : SWITCHABLE_FILTERS;

  
  const int above_mv_pred = above_in_image ? is_inter_block(&above_mi->mbmi)
                                           : 0;
  const int above_interp = above_in_image && above_mv_pred
                               ? above_mi->mbmi.interp_filter
                               : SWITCHABLE_FILTERS;

  if (left_interp == above_interp)
    return left_interp;
  else if (left_interp == SWITCHABLE_FILTERS &&
           above_interp != SWITCHABLE_FILTERS)
    return above_interp;
  else if (left_interp != SWITCHABLE_FILTERS &&
           above_interp == SWITCHABLE_FILTERS)
    return left_interp;
  else
    return SWITCHABLE_FILTERS;
}

unsigned char vp9_get_pred_context_intra_inter(const MACROBLOCKD *xd) {
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  const int above_intra = above_in_image ? !is_inter_block(above_mbmi) : 1;
  const int left_intra = left_in_image ? !is_inter_block(left_mbmi) : 1;

  
  
  
  
  
  
  
  if (above_in_image && left_in_image)  
    return left_intra && above_intra ? 3
                                     : left_intra || above_intra;
  else if (above_in_image || left_in_image)  
    return 2 * (above_in_image ? above_intra : left_intra);
  else
    return 0;
}

unsigned char vp9_get_pred_context_comp_inter_inter(const VP9_COMMON *cm,
                                                    const MACROBLOCKD *xd) {
  int pred_context;
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  
  
  
  
  if (above_in_image && left_in_image) {  
    if (!has_second_ref(above_mbmi) && !has_second_ref(left_mbmi))
      
      pred_context = (above_mbmi->ref_frame[0] == cm->comp_fixed_ref) ^
                     (left_mbmi->ref_frame[0] == cm->comp_fixed_ref);
    else if (!has_second_ref(above_mbmi))
      
      pred_context = 2 + (above_mbmi->ref_frame[0] == cm->comp_fixed_ref ||
                          !is_inter_block(above_mbmi));
    else if (!has_second_ref(left_mbmi))
      
      pred_context = 2 + (left_mbmi->ref_frame[0] == cm->comp_fixed_ref ||
                          !is_inter_block(left_mbmi));
    else  
      pred_context = 4;
  } else if (above_in_image || left_in_image) {  
    const MB_MODE_INFO *edge_mbmi = above_in_image ? above_mbmi : left_mbmi;

    if (!has_second_ref(edge_mbmi))
      
      pred_context = edge_mbmi->ref_frame[0] == cm->comp_fixed_ref;
    else
      
      pred_context = 3;
  } else {  
    pred_context = 1;
  }
  assert(pred_context >= 0 && pred_context < COMP_INTER_CONTEXTS);
  return pred_context;
}


unsigned char vp9_get_pred_context_comp_ref_p(const VP9_COMMON *cm,
                                              const MACROBLOCKD *xd) {
  int pred_context;
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  const int above_intra = above_in_image ? !is_inter_block(above_mbmi) : 1;
  const int left_intra = left_in_image ? !is_inter_block(left_mbmi) : 1;
  
  
  
  
  const int fix_ref_idx = cm->ref_frame_sign_bias[cm->comp_fixed_ref];
  const int var_ref_idx = !fix_ref_idx;

  if (above_in_image && left_in_image) {  
    if (above_intra && left_intra) {  
      pred_context = 2;
    } else if (above_intra || left_intra) {  
      const MB_MODE_INFO *edge_mbmi = above_intra ? left_mbmi : above_mbmi;

      if (!has_second_ref(edge_mbmi))  
        pred_context = 1 + 2 * (edge_mbmi->ref_frame[0] != cm->comp_var_ref[1]);
      else  
        pred_context = 1 + 2 * (edge_mbmi->ref_frame[var_ref_idx]
                                    != cm->comp_var_ref[1]);
    } else {  
      const int l_sg = !has_second_ref(left_mbmi);
      const int a_sg = !has_second_ref(above_mbmi);
      MV_REFERENCE_FRAME vrfa = a_sg ? above_mbmi->ref_frame[0]
                                     : above_mbmi->ref_frame[var_ref_idx];
      MV_REFERENCE_FRAME vrfl = l_sg ? left_mbmi->ref_frame[0]
                                     : left_mbmi->ref_frame[var_ref_idx];

      if (vrfa == vrfl && cm->comp_var_ref[1] == vrfa) {
        pred_context = 0;
      } else if (l_sg && a_sg) {  
        if ((vrfa == cm->comp_fixed_ref && vrfl == cm->comp_var_ref[0]) ||
            (vrfl == cm->comp_fixed_ref && vrfa == cm->comp_var_ref[0]))
          pred_context = 4;
        else if (vrfa == vrfl)
          pred_context = 3;
        else
          pred_context = 1;
      } else if (l_sg || a_sg) {  
        MV_REFERENCE_FRAME vrfc = l_sg ? vrfa : vrfl;
        MV_REFERENCE_FRAME rfs = a_sg ? vrfa : vrfl;
        if (vrfc == cm->comp_var_ref[1] && rfs != cm->comp_var_ref[1])
          pred_context = 1;
        else if (rfs == cm->comp_var_ref[1] && vrfc != cm->comp_var_ref[1])
          pred_context = 2;
        else
          pred_context = 4;
      } else if (vrfa == vrfl) {  
        pred_context = 4;
      } else {
        pred_context = 2;
      }
    }
  } else if (above_in_image || left_in_image) {  
    const MB_MODE_INFO *edge_mbmi = above_in_image ? above_mbmi : left_mbmi;

    if (!is_inter_block(edge_mbmi)) {
      pred_context = 2;
    } else {
      if (has_second_ref(edge_mbmi))
        pred_context = 4 * (edge_mbmi->ref_frame[var_ref_idx]
                              != cm->comp_var_ref[1]);
      else
        pred_context = 3 * (edge_mbmi->ref_frame[0] != cm->comp_var_ref[1]);
    }
  } else {  
    pred_context = 2;
  }
  assert(pred_context >= 0 && pred_context < REF_CONTEXTS);

  return pred_context;
}
unsigned char vp9_get_pred_context_single_ref_p1(const MACROBLOCKD *xd) {
  int pred_context;
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  const int above_intra = above_in_image ? !is_inter_block(above_mbmi) : 1;
  const int left_intra = left_in_image ? !is_inter_block(left_mbmi) : 1;
  
  
  
  
  if (above_in_image && left_in_image) {  
    if (above_intra && left_intra) {  
      pred_context = 2;
    } else if (above_intra || left_intra) {  
      const MB_MODE_INFO *edge_mbmi = above_intra ? left_mbmi : above_mbmi;
      if (!has_second_ref(edge_mbmi))
        pred_context = 4 * (edge_mbmi->ref_frame[0] == LAST_FRAME);
      else
        pred_context = 1 + (edge_mbmi->ref_frame[0] == LAST_FRAME ||
                            edge_mbmi->ref_frame[1] == LAST_FRAME);
    } else {  
      if (!has_second_ref(above_mbmi) && !has_second_ref(left_mbmi)) {
        pred_context = 2 * (above_mbmi->ref_frame[0] == LAST_FRAME) +
                       2 * (left_mbmi->ref_frame[0] == LAST_FRAME);
      } else if (has_second_ref(above_mbmi) && has_second_ref(left_mbmi)) {
        pred_context = 1 + (above_mbmi->ref_frame[0] == LAST_FRAME ||
                            above_mbmi->ref_frame[1] == LAST_FRAME ||
                            left_mbmi->ref_frame[0] == LAST_FRAME ||
                            left_mbmi->ref_frame[1] == LAST_FRAME);
      } else {
        const MV_REFERENCE_FRAME rfs = !has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[0] : left_mbmi->ref_frame[0];
        const MV_REFERENCE_FRAME crf1 = has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[0] : left_mbmi->ref_frame[0];
        const MV_REFERENCE_FRAME crf2 = has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[1] : left_mbmi->ref_frame[1];

        if (rfs == LAST_FRAME)
          pred_context = 3 + (crf1 == LAST_FRAME || crf2 == LAST_FRAME);
        else
          pred_context = crf1 == LAST_FRAME || crf2 == LAST_FRAME;
      }
    }
  } else if (above_in_image || left_in_image) {  
    const MB_MODE_INFO *edge_mbmi = above_in_image ? above_mbmi : left_mbmi;
    if (!is_inter_block(edge_mbmi)) {  
      pred_context = 2;
    } else {  
      if (!has_second_ref(edge_mbmi))
        pred_context = 4 * (edge_mbmi->ref_frame[0] == LAST_FRAME);
      else
        pred_context = 1 + (edge_mbmi->ref_frame[0] == LAST_FRAME ||
                            edge_mbmi->ref_frame[1] == LAST_FRAME);
    }
  } else {  
    pred_context = 2;
  }

  assert(pred_context >= 0 && pred_context < REF_CONTEXTS);
  return pred_context;
}

unsigned char vp9_get_pred_context_single_ref_p2(const MACROBLOCKD *xd) {
  int pred_context;
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  const int above_intra = above_in_image ? !is_inter_block(above_mbmi) : 1;
  const int left_intra = left_in_image ? !is_inter_block(left_mbmi) : 1;

  
  
  
  
  if (above_in_image && left_in_image) {  
    if (above_intra && left_intra) {  
      pred_context = 2;
    } else if (above_intra || left_intra) {  
      const MB_MODE_INFO *edge_mbmi = above_intra ? left_mbmi : above_mbmi;
      if (!has_second_ref(edge_mbmi)) {
        if (edge_mbmi->ref_frame[0] == LAST_FRAME)
          pred_context = 3;
        else
          pred_context = 4 * (edge_mbmi->ref_frame[0] == GOLDEN_FRAME);
      } else {
        pred_context = 1 + 2 * (edge_mbmi->ref_frame[0] == GOLDEN_FRAME ||
                                edge_mbmi->ref_frame[1] == GOLDEN_FRAME);
      }
    } else {  
      if (!has_second_ref(above_mbmi) && !has_second_ref(left_mbmi)) {
        if (above_mbmi->ref_frame[0] == LAST_FRAME &&
            left_mbmi->ref_frame[0] == LAST_FRAME) {
          pred_context = 3;
        } else if (above_mbmi->ref_frame[0] == LAST_FRAME ||
                   left_mbmi->ref_frame[0] == LAST_FRAME) {
          const MB_MODE_INFO *edge_mbmi =
              above_mbmi->ref_frame[0] == LAST_FRAME ? left_mbmi : above_mbmi;

          pred_context = 4 * (edge_mbmi->ref_frame[0] == GOLDEN_FRAME);
        } else {
          pred_context = 2 * (above_mbmi->ref_frame[0] == GOLDEN_FRAME) +
                         2 * (left_mbmi->ref_frame[0] == GOLDEN_FRAME);
        }
      } else if (has_second_ref(above_mbmi) && has_second_ref(left_mbmi)) {
        if (above_mbmi->ref_frame[0] == left_mbmi->ref_frame[0] &&
            above_mbmi->ref_frame[1] == left_mbmi->ref_frame[1])
          pred_context = 3 * (above_mbmi->ref_frame[0] == GOLDEN_FRAME ||
                              above_mbmi->ref_frame[1] == GOLDEN_FRAME ||
                              left_mbmi->ref_frame[0] == GOLDEN_FRAME ||
                              left_mbmi->ref_frame[1] == GOLDEN_FRAME);
        else
          pred_context = 2;
      } else {
        const MV_REFERENCE_FRAME rfs = !has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[0] : left_mbmi->ref_frame[0];
        const MV_REFERENCE_FRAME crf1 = has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[0] : left_mbmi->ref_frame[0];
        const MV_REFERENCE_FRAME crf2 = has_second_ref(above_mbmi) ?
                  above_mbmi->ref_frame[1] : left_mbmi->ref_frame[1];

        if (rfs == GOLDEN_FRAME)
          pred_context = 3 + (crf1 == GOLDEN_FRAME || crf2 == GOLDEN_FRAME);
        else if (rfs == ALTREF_FRAME)
          pred_context = crf1 == GOLDEN_FRAME || crf2 == GOLDEN_FRAME;
        else
          pred_context = 1 + 2 * (crf1 == GOLDEN_FRAME || crf2 == GOLDEN_FRAME);
      }
    }
  } else if (above_in_image || left_in_image) {  
    const MB_MODE_INFO *edge_mbmi = above_in_image ? above_mbmi : left_mbmi;

    if (!is_inter_block(edge_mbmi) ||
        (edge_mbmi->ref_frame[0] == LAST_FRAME && !has_second_ref(edge_mbmi)))
      pred_context = 2;
    else if (!has_second_ref(edge_mbmi))
      pred_context = 4 * (edge_mbmi->ref_frame[0] == GOLDEN_FRAME);
    else
      pred_context = 3 * (edge_mbmi->ref_frame[0] == GOLDEN_FRAME ||
                          edge_mbmi->ref_frame[1] == GOLDEN_FRAME);
  } else {  
    pred_context = 2;
  }
  assert(pred_context >= 0 && pred_context < REF_CONTEXTS);
  return pred_context;
}




unsigned char vp9_get_pred_context_tx_size(const MACROBLOCKD *xd) {
  const MODE_INFO *const above_mi = get_above_mi(xd);
  const MODE_INFO *const left_mi = get_left_mi(xd);
  const MB_MODE_INFO *const above_mbmi = get_above_mbmi(above_mi);
  const MB_MODE_INFO *const left_mbmi = get_left_mbmi(left_mi);
  const int above_in_image = above_mi != NULL;
  const int left_in_image = left_mi != NULL;
  const int max_tx_size = max_txsize_lookup[xd->mi_8x8[0]->mbmi.sb_type];
  int above_context = max_tx_size;
  int left_context = max_tx_size;

  if (above_in_image)
    above_context = above_mbmi->skip_coeff ? max_tx_size
                                           : above_mbmi->tx_size;

  if (left_in_image)
    left_context = left_mbmi->skip_coeff ? max_tx_size
                                         : left_mbmi->tx_size;

  if (!left_in_image)
    left_context = above_context;

  if (!above_in_image)
    above_context = left_context;

  return above_context + left_context > max_tx_size;
}

void vp9_set_pred_flag_seg_id(MACROBLOCKD *xd, uint8_t pred_flag) {
  xd->mi_8x8[0]->mbmi.seg_id_predicted = pred_flag;
}

int vp9_get_segment_id(VP9_COMMON *cm, const uint8_t *segment_ids,
                       BLOCK_SIZE bsize, int mi_row, int mi_col) {
  const int mi_offset = mi_row * cm->mi_cols + mi_col;
  const int bw = num_8x8_blocks_wide_lookup[bsize];
  const int bh = num_8x8_blocks_high_lookup[bsize];
  const int xmis = MIN(cm->mi_cols - mi_col, bw);
  const int ymis = MIN(cm->mi_rows - mi_row, bh);
  int x, y, segment_id = INT_MAX;

  for (y = 0; y < ymis; y++)
    for (x = 0; x < xmis; x++)
      segment_id = MIN(segment_id,
                       segment_ids[mi_offset + y * cm->mi_cols + x]);

  assert(segment_id >= 0 && segment_id < MAX_SEGMENTS);
  return segment_id;
}
