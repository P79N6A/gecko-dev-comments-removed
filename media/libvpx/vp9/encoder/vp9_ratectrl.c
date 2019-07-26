










#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_common.h"
#include "vp9/encoder/vp9_ratectrl.h"
#include "vp9/common/vp9_entropymode.h"
#include "vpx_mem/vpx_mem.h"
#include "vp9/common/vp9_systemdependent.h"
#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_seg_common.h"

#define MIN_BPB_FACTOR 0.005
#define MAX_BPB_FACTOR 50


#define BPER_MB_NORMBITS    9

static const unsigned int prior_key_frame_weight[KEY_FRAME_CONTEXT] =
    { 1, 2, 3, 4, 5 };




double vp9_convert_qindex_to_q(int qindex) {
  
  return vp9_ac_quant(qindex, 0) / 4.0;
}

int vp9_gfboost_qadjust(int qindex) {
  const double q = vp9_convert_qindex_to_q(qindex);
  return (int)((0.00000828 * q * q * q) +
               (-0.0055 * q * q) +
               (1.32 * q) + 79.3);
}

static int kfboost_qadjust(int qindex) {
  const double q = vp9_convert_qindex_to_q(qindex);
  return (int)((0.00000973 * q * q * q) +
               (-0.00613 * q * q) +
               (1.316 * q) + 121.2);
}

int vp9_bits_per_mb(FRAME_TYPE frame_type, int qindex,
                    double correction_factor) {
  const double q = vp9_convert_qindex_to_q(qindex);
  int enumerator = frame_type == KEY_FRAME ? 3300000 : 2250000;

  
  enumerator += (int)(enumerator * q) >> 12;
  return (int)(0.5 + (enumerator * correction_factor / q));
}

void vp9_save_coding_context(VP9_COMP *cpi) {
  CODING_CONTEXT *const cc = &cpi->coding_context;
  VP9_COMMON *cm = &cpi->common;

  
  
  
  
  vp9_copy(cc->nmvjointcost,  cpi->mb.nmvjointcost);
  vp9_copy(cc->nmvcosts,  cpi->mb.nmvcosts);
  vp9_copy(cc->nmvcosts_hp,  cpi->mb.nmvcosts_hp);

  vp9_copy(cc->segment_pred_probs, cm->seg.pred_probs);

  vpx_memcpy(cpi->coding_context.last_frame_seg_map_copy,
             cm->last_frame_seg_map, (cm->mi_rows * cm->mi_cols));

  vp9_copy(cc->last_ref_lf_deltas, cm->lf.last_ref_deltas);
  vp9_copy(cc->last_mode_lf_deltas, cm->lf.last_mode_deltas);

  cc->fc = cm->fc;
}

void vp9_restore_coding_context(VP9_COMP *cpi) {
  CODING_CONTEXT *const cc = &cpi->coding_context;
  VP9_COMMON *cm = &cpi->common;

  
  
  vp9_copy(cpi->mb.nmvjointcost, cc->nmvjointcost);
  vp9_copy(cpi->mb.nmvcosts, cc->nmvcosts);
  vp9_copy(cpi->mb.nmvcosts_hp, cc->nmvcosts_hp);

  vp9_copy(cm->seg.pred_probs, cc->segment_pred_probs);

  vpx_memcpy(cm->last_frame_seg_map,
             cpi->coding_context.last_frame_seg_map_copy,
             (cm->mi_rows * cm->mi_cols));

  vp9_copy(cm->lf.last_ref_deltas, cc->last_ref_lf_deltas);
  vp9_copy(cm->lf.last_mode_deltas, cc->last_mode_lf_deltas);

  cm->fc = cc->fc;
}

void vp9_setup_key_frame(VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;

  vp9_setup_past_independence(cm);

  
  cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
  
  cpi->refresh_golden_frame = 1;
  cpi->refresh_alt_ref_frame = 1;
}

void vp9_setup_inter_frame(VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;
  if (cm->error_resilient_mode || cm->intra_only)
    vp9_setup_past_independence(cm);

  assert(cm->frame_context_idx < NUM_FRAME_CONTEXTS);
  cm->fc = cm->frame_contexts[cm->frame_context_idx];
}

static int estimate_bits_at_q(int frame_kind, int q, int mbs,
                              double correction_factor) {
  const int bpm = (int)(vp9_bits_per_mb(frame_kind, q, correction_factor));

  
  
  
  return (mbs > (1 << 11)) ? (bpm >> BPER_MB_NORMBITS) * mbs
                           : (bpm * mbs) >> BPER_MB_NORMBITS;
}


static void calc_iframe_target_size(VP9_COMP *cpi) {
  
  int target;

  
  vp9_clear_system_state();  

  
  target = cpi->per_frame_bandwidth;

  if (cpi->oxcf.rc_max_intra_bitrate_pct) {
    int max_rate = cpi->per_frame_bandwidth
                 * cpi->oxcf.rc_max_intra_bitrate_pct / 100;

    if (target > max_rate)
      target = max_rate;
  }

  cpi->this_frame_target = target;
}







static void calc_gf_params(VP9_COMP *cpi) {
  
  cpi->frames_till_gf_update_due = cpi->baseline_gf_interval;
}


static void calc_pframe_target_size(VP9_COMP *cpi) {
  const int min_frame_target = MAX(cpi->min_frame_bandwidth,
                                   cpi->av_per_frame_bandwidth >> 5);
  if (cpi->refresh_alt_ref_frame) {
    
    
    cpi->per_frame_bandwidth = cpi->twopass.gf_bits;
    cpi->this_frame_target = cpi->per_frame_bandwidth;
  } else {
    
    cpi->this_frame_target = cpi->per_frame_bandwidth;
  }

  
  
  
  
  
  
  if (cpi->this_frame_target < min_frame_target)
    cpi->this_frame_target = min_frame_target;

  if (!cpi->refresh_alt_ref_frame)
    
    cpi->inter_frame_target = cpi->this_frame_target;

  
  if (cpi->frames_till_gf_update_due == 0) {
    const int q = (cpi->oxcf.fixed_q < 0) ? cpi->last_q[INTER_FRAME]
                                          : cpi->oxcf.fixed_q;

    cpi->refresh_golden_frame = 1;

    calc_gf_params(cpi);

    
    
    
    if (!cpi->source_alt_ref_active) {
      if (cpi->oxcf.fixed_q < 0) {
        
        
        cpi->this_frame_target = cpi->per_frame_bandwidth;
      } else {
        cpi->this_frame_target =
          (estimate_bits_at_q(1, q, cpi->common.MBs, 1.0)
           * cpi->last_boost) / 100;
      }
    } else {
      
      
      
      
      cpi->this_frame_target = 0;
    }
  }
}


void vp9_update_rate_correction_factors(VP9_COMP *cpi, int damp_var) {
  const int q = cpi->common.base_qindex;
  int correction_factor = 100;
  double rate_correction_factor;
  double adjustment_limit;

  int projected_size_based_on_q = 0;

  
  vp9_clear_system_state();  

  if (cpi->common.frame_type == KEY_FRAME) {
    rate_correction_factor = cpi->key_frame_rate_correction_factor;
  } else {
    if (cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame)
      rate_correction_factor = cpi->gf_rate_correction_factor;
    else
      rate_correction_factor = cpi->rate_correction_factor;
  }

  
  
  
  projected_size_based_on_q = estimate_bits_at_q(cpi->common.frame_type, q,
                                                 cpi->common.MBs,
                                                 rate_correction_factor);

  
  if (projected_size_based_on_q > 0)
    correction_factor =
        (100 * cpi->projected_frame_size) / projected_size_based_on_q;

  
  
  switch (damp_var) {
    case 0:
      adjustment_limit = 0.75;
      break;
    case 1:
      adjustment_limit = 0.375;
      break;
    case 2:
    default:
      adjustment_limit = 0.25;
      break;
  }

  
  if (correction_factor > 102) {
    
    correction_factor =
        (int)(100 + ((correction_factor - 100) * adjustment_limit));
    rate_correction_factor =
        ((rate_correction_factor * correction_factor) / 100);

    
    if (rate_correction_factor > MAX_BPB_FACTOR)
      rate_correction_factor = MAX_BPB_FACTOR;
  } else if (correction_factor < 99) {
    
    correction_factor =
        (int)(100 - ((100 - correction_factor) * adjustment_limit));
    rate_correction_factor =
        ((rate_correction_factor * correction_factor) / 100);

    
    if (rate_correction_factor < MIN_BPB_FACTOR)
      rate_correction_factor = MIN_BPB_FACTOR;
  }

  if (cpi->common.frame_type == KEY_FRAME) {
    cpi->key_frame_rate_correction_factor = rate_correction_factor;
  } else {
    if (cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame)
      cpi->gf_rate_correction_factor = rate_correction_factor;
    else
      cpi->rate_correction_factor = rate_correction_factor;
  }
}


int vp9_regulate_q(VP9_COMP *cpi, int target_bits_per_frame) {
  int q = cpi->active_worst_quality;

  int i;
  int last_error = INT_MAX;
  int target_bits_per_mb;
  int bits_per_mb_at_this_q;
  double correction_factor;

  
  if (cpi->common.frame_type == KEY_FRAME) {
    correction_factor = cpi->key_frame_rate_correction_factor;
  } else {
    if (cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame)
      correction_factor = cpi->gf_rate_correction_factor;
    else
      correction_factor = cpi->rate_correction_factor;
  }

  
  
  if (target_bits_per_frame >= (INT_MAX >> BPER_MB_NORMBITS))
    target_bits_per_mb =
        (target_bits_per_frame / cpi->common.MBs)
        << BPER_MB_NORMBITS;  
  else
    target_bits_per_mb =
        (target_bits_per_frame << BPER_MB_NORMBITS) / cpi->common.MBs;

  i = cpi->active_best_quality;

  do {
    bits_per_mb_at_this_q = (int)vp9_bits_per_mb(cpi->common.frame_type, i,
                                                 correction_factor);

    if (bits_per_mb_at_this_q <= target_bits_per_mb) {
      if ((target_bits_per_mb - bits_per_mb_at_this_q) <= last_error)
        q = i;
      else
        q = i - 1;

      break;
    } else {
      last_error = bits_per_mb_at_this_q - target_bits_per_mb;
    }
  } while (++i <= cpi->active_worst_quality);

  return q;
}


static int estimate_keyframe_frequency(VP9_COMP *cpi) {
  int i;

  
  int av_key_frame_frequency = 0;

  


  if (cpi->key_frame_count == 1) {
    


    int key_freq = cpi->oxcf.key_freq > 0 ? cpi->oxcf.key_freq : 1;
    av_key_frame_frequency = (int)cpi->output_framerate * 2;

    if (cpi->oxcf.auto_key && av_key_frame_frequency > key_freq)
      av_key_frame_frequency = cpi->oxcf.key_freq;

    cpi->prior_key_frame_distance[KEY_FRAME_CONTEXT - 1]
      = av_key_frame_frequency;
  } else {
    unsigned int total_weight = 0;
    int last_kf_interval =
      (cpi->frames_since_key > 0) ? cpi->frames_since_key : 1;

    


    for (i = 0; i < KEY_FRAME_CONTEXT; i++) {
      if (i < KEY_FRAME_CONTEXT - 1)
        cpi->prior_key_frame_distance[i]
          = cpi->prior_key_frame_distance[i + 1];
      else
        cpi->prior_key_frame_distance[i] = last_kf_interval;

      av_key_frame_frequency += prior_key_frame_weight[i]
                                * cpi->prior_key_frame_distance[i];
      total_weight += prior_key_frame_weight[i];
    }

    av_key_frame_frequency /= total_weight;
  }
  return av_key_frame_frequency;
}


void vp9_adjust_key_frame_context(VP9_COMP *cpi) {
  
  vp9_clear_system_state();

  cpi->frames_since_key = 0;
  cpi->key_frame_count++;
}


void vp9_compute_frame_size_bounds(VP9_COMP *cpi, int *frame_under_shoot_limit,
                                   int *frame_over_shoot_limit) {
  
  if (cpi->oxcf.fixed_q >= 0) {
    
    *frame_under_shoot_limit = 0;
    *frame_over_shoot_limit  = INT_MAX;
  } else {
    if (cpi->common.frame_type == KEY_FRAME) {
      *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
      *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
    } else {
      if (cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame) {
        *frame_over_shoot_limit  = cpi->this_frame_target * 9 / 8;
        *frame_under_shoot_limit = cpi->this_frame_target * 7 / 8;
      } else {
        
        if (cpi->oxcf.end_usage == USAGE_CONSTRAINED_QUALITY) {
          *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
          *frame_under_shoot_limit = cpi->this_frame_target * 2 / 8;
        } else {
          *frame_over_shoot_limit  = cpi->this_frame_target * 11 / 8;
          *frame_under_shoot_limit = cpi->this_frame_target * 5 / 8;
        }
      }
    }

    
    
    
    *frame_over_shoot_limit += 200;
    *frame_under_shoot_limit -= 200;
    if (*frame_under_shoot_limit < 0)
      *frame_under_shoot_limit = 0;
  }
}



int vp9_pick_frame_size(VP9_COMP *cpi) {
  VP9_COMMON *cm = &cpi->common;

  if (cm->frame_type == KEY_FRAME)
    calc_iframe_target_size(cpi);
  else
    calc_pframe_target_size(cpi);

  return 1;
}
