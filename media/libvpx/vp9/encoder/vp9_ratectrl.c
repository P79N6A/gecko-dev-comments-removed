









#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vpx_mem/vpx_mem.h"

#include "vp9/common/vp9_alloccommon.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_entropymode.h"
#include "vp9/common/vp9_quant_common.h"
#include "vp9/common/vp9_seg_common.h"
#include "vp9/common/vp9_systemdependent.h"

#include "vp9/encoder/vp9_encodemv.h"
#include "vp9/encoder/vp9_ratectrl.h"



#define MAX_MB_RATE 250
#define MAXRATE_1080P 2025000

#define DEFAULT_KF_BOOST 2000
#define DEFAULT_GF_BOOST 2000

#define LIMIT_QRANGE_FOR_ALTREF_AND_KEY 1

#define MIN_BPB_FACTOR 0.005
#define MAX_BPB_FACTOR 50

#define FRAME_OVERHEAD_BITS 200


static int kf_low_motion_minq[QINDEX_RANGE];
static int kf_high_motion_minq[QINDEX_RANGE];
static int arfgf_low_motion_minq[QINDEX_RANGE];
static int arfgf_high_motion_minq[QINDEX_RANGE];
static int inter_minq[QINDEX_RANGE];
static int rtc_minq[QINDEX_RANGE];
static int gf_high = 2000;
static int gf_low = 400;
static int kf_high = 5000;
static int kf_low = 400;





static int get_minq_index(double maxq, double x3, double x2, double x1) {
  int i;
  const double minqtarget = MIN(((x3 * maxq + x2) * maxq + x1) * maxq,
                                maxq);

  
  
  if (minqtarget <= 2.0)
    return 0;

  for (i = 0; i < QINDEX_RANGE; i++)
    if (minqtarget <= vp9_convert_qindex_to_q(i))
      return i;

  return QINDEX_RANGE - 1;
}

void vp9_rc_init_minq_luts() {
  int i;

  for (i = 0; i < QINDEX_RANGE; i++) {
    const double maxq = vp9_convert_qindex_to_q(i);
    kf_low_motion_minq[i] = get_minq_index(maxq, 0.000001, -0.0004, 0.125);
    kf_high_motion_minq[i] = get_minq_index(maxq, 0.000002, -0.0012, 0.50);
    arfgf_low_motion_minq[i] = get_minq_index(maxq, 0.0000015, -0.0009, 0.30);
    arfgf_high_motion_minq[i] = get_minq_index(maxq, 0.0000021, -0.00125, 0.50);
    inter_minq[i] = get_minq_index(maxq, 0.00000271, -0.00113, 0.90);
    rtc_minq[i] = get_minq_index(maxq, 0.00000271, -0.00113, 0.70);
  }
}




double vp9_convert_qindex_to_q(int qindex) {
  
  return vp9_ac_quant(qindex, 0) / 4.0;
}

int vp9_rc_bits_per_mb(FRAME_TYPE frame_type, int qindex,
                       double correction_factor) {
  const double q = vp9_convert_qindex_to_q(qindex);
  int enumerator = frame_type == KEY_FRAME ? 3300000 : 2250000;

  
  enumerator += (int)(enumerator * q) >> 12;
  return (int)(enumerator * correction_factor / q);
}

static int estimate_bits_at_q(FRAME_TYPE frame_type, int q, int mbs,
                              double correction_factor) {
  const int bpm = (int)(vp9_rc_bits_per_mb(frame_type, q, correction_factor));
  return ((uint64_t)bpm * mbs) >> BPER_MB_NORMBITS;
}

int vp9_rc_clamp_pframe_target_size(const VP9_COMP *const cpi, int target) {
  const RATE_CONTROL *rc = &cpi->rc;
  const int min_frame_target = MAX(rc->min_frame_bandwidth,
                                   rc->avg_frame_bandwidth >> 5);
  if (target < min_frame_target)
    target = min_frame_target;
  if (cpi->refresh_golden_frame && rc->is_src_frame_alt_ref) {
    
    
    
    
    target = min_frame_target;
  }
  
  if (target > rc->max_frame_bandwidth)
    target = rc->max_frame_bandwidth;
  return target;
}

int vp9_rc_clamp_iframe_target_size(const VP9_COMP *const cpi, int target) {
  const RATE_CONTROL *rc = &cpi->rc;
  const VP9EncoderConfig *oxcf = &cpi->oxcf;
  if (oxcf->rc_max_intra_bitrate_pct) {
    const int max_rate = rc->avg_frame_bandwidth *
                             oxcf->rc_max_intra_bitrate_pct / 100;
    target = MIN(target, max_rate);
  }
  if (target > rc->max_frame_bandwidth)
    target = rc->max_frame_bandwidth;
  return target;
}



static void update_layer_buffer_level(SVC *svc, int encoded_frame_size) {
  int temporal_layer = 0;
  int current_temporal_layer = svc->temporal_layer_id;
  for (temporal_layer = current_temporal_layer + 1;
      temporal_layer < svc->number_temporal_layers; ++temporal_layer) {
    LAYER_CONTEXT *lc = &svc->layer_context[temporal_layer];
    RATE_CONTROL *lrc = &lc->rc;
    int bits_off_for_this_layer = (int)(lc->target_bandwidth / lc->framerate -
        encoded_frame_size);
    lrc->bits_off_target += bits_off_for_this_layer;

    
    lrc->bits_off_target = MIN(lrc->bits_off_target, lrc->maximum_buffer_size);
    lrc->buffer_level = lrc->bits_off_target;
  }
}


static void update_buffer_level(VP9_COMP *cpi, int encoded_frame_size) {
  const VP9_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;

  
  if (!cm->show_frame) {
    rc->bits_off_target -= encoded_frame_size;
  } else {
    rc->bits_off_target += rc->avg_frame_bandwidth - encoded_frame_size;
  }

  
  rc->bits_off_target = MIN(rc->bits_off_target, rc->maximum_buffer_size);
  rc->buffer_level = rc->bits_off_target;

  if (cpi->use_svc && cpi->oxcf.rc_mode == VPX_CBR) {
    update_layer_buffer_level(&cpi->svc, encoded_frame_size);
  }
}

void vp9_rc_init(const VP9EncoderConfig *oxcf, int pass, RATE_CONTROL *rc) {
  int i;

  if (pass == 0 && oxcf->rc_mode == VPX_CBR) {
    rc->avg_frame_qindex[KEY_FRAME] = oxcf->worst_allowed_q;
    rc->avg_frame_qindex[INTER_FRAME] = oxcf->worst_allowed_q;
  } else {
    rc->avg_frame_qindex[KEY_FRAME] = (oxcf->worst_allowed_q +
                                           oxcf->best_allowed_q) / 2;
    rc->avg_frame_qindex[INTER_FRAME] = (oxcf->worst_allowed_q +
                                           oxcf->best_allowed_q) / 2;
  }

  rc->last_q[KEY_FRAME] = oxcf->best_allowed_q;
  rc->last_q[INTER_FRAME] = oxcf->best_allowed_q;

  rc->buffer_level =    rc->starting_buffer_level;
  rc->bits_off_target = rc->starting_buffer_level;

  rc->rolling_target_bits      = rc->avg_frame_bandwidth;
  rc->rolling_actual_bits      = rc->avg_frame_bandwidth;
  rc->long_rolling_target_bits = rc->avg_frame_bandwidth;
  rc->long_rolling_actual_bits = rc->avg_frame_bandwidth;

  rc->total_actual_bits = 0;
  rc->total_target_bits = 0;
  rc->total_target_vs_actual = 0;

  rc->baseline_gf_interval = DEFAULT_GF_INTERVAL;
  rc->frames_since_key = 8;  
  rc->this_key_frame_forced = 0;
  rc->next_key_frame_forced = 0;
  rc->source_alt_ref_pending = 0;
  rc->source_alt_ref_active = 0;

  rc->frames_till_gf_update_due = 0;

  rc->ni_av_qi = oxcf->worst_allowed_q;
  rc->ni_tot_qi = 0;
  rc->ni_frames = 0;

  rc->tot_q = 0.0;
  rc->avg_q = vp9_convert_qindex_to_q(oxcf->worst_allowed_q);

  for (i = 0; i < RATE_FACTOR_LEVELS; ++i) {
    rc->rate_correction_factors[i] = 1.0;
  }
}

int vp9_rc_drop_frame(VP9_COMP *cpi) {
  const VP9EncoderConfig *oxcf = &cpi->oxcf;
  RATE_CONTROL *const rc = &cpi->rc;

  if (!oxcf->drop_frames_water_mark) {
    return 0;
  } else {
    if (rc->buffer_level < 0) {
      
      return 1;
    } else {
      
      
      int drop_mark = (int)(oxcf->drop_frames_water_mark *
          rc->optimal_buffer_level / 100);
      if ((rc->buffer_level > drop_mark) &&
          (rc->decimation_factor > 0)) {
        --rc->decimation_factor;
      } else if (rc->buffer_level <= drop_mark &&
          rc->decimation_factor == 0) {
        rc->decimation_factor = 1;
      }
      if (rc->decimation_factor > 0) {
        if (rc->decimation_count > 0) {
          --rc->decimation_count;
          return 1;
        } else {
          rc->decimation_count = rc->decimation_factor;
          return 0;
        }
      } else {
        rc->decimation_count = 0;
        return 0;
      }
    }
  }
}

static double get_rate_correction_factor(const VP9_COMP *cpi) {
  const RATE_CONTROL *const rc = &cpi->rc;

  if (cpi->common.frame_type == KEY_FRAME) {
    return rc->rate_correction_factors[KF_STD];
  } else if (cpi->oxcf.pass == 2) {
    RATE_FACTOR_LEVEL rf_lvl =
      cpi->twopass.gf_group.rf_level[cpi->twopass.gf_group.index];
    return rc->rate_correction_factors[rf_lvl];
  } else {
    if ((cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame) &&
        !rc->is_src_frame_alt_ref &&
        !(cpi->use_svc && cpi->oxcf.rc_mode == VPX_CBR))
      return rc->rate_correction_factors[GF_ARF_STD];
    else
      return rc->rate_correction_factors[INTER_NORMAL];
  }
}

static void set_rate_correction_factor(VP9_COMP *cpi, double factor) {
  RATE_CONTROL *const rc = &cpi->rc;

  if (cpi->common.frame_type == KEY_FRAME) {
    rc->rate_correction_factors[KF_STD] = factor;
  } else if (cpi->oxcf.pass == 2) {
    RATE_FACTOR_LEVEL rf_lvl =
      cpi->twopass.gf_group.rf_level[cpi->twopass.gf_group.index];
    rc->rate_correction_factors[rf_lvl] = factor;
  } else {
    if ((cpi->refresh_alt_ref_frame || cpi->refresh_golden_frame) &&
        !rc->is_src_frame_alt_ref &&
        !(cpi->use_svc && cpi->oxcf.rc_mode == VPX_CBR))
      rc->rate_correction_factors[GF_ARF_STD] = factor;
    else
      rc->rate_correction_factors[INTER_NORMAL] = factor;
  }
}

void vp9_rc_update_rate_correction_factors(VP9_COMP *cpi, int damp_var) {
  const VP9_COMMON *const cm = &cpi->common;
  int correction_factor = 100;
  double rate_correction_factor = get_rate_correction_factor(cpi);
  double adjustment_limit;

  int projected_size_based_on_q = 0;

  
  if (cpi->rc.is_src_frame_alt_ref)
    return;

  
  vp9_clear_system_state();

  
  
  
  projected_size_based_on_q = estimate_bits_at_q(cm->frame_type,
                                                 cm->base_qindex, cm->MBs,
                                                 rate_correction_factor);
  
  if (projected_size_based_on_q > 0)
    correction_factor = (100 * cpi->rc.projected_frame_size) /
                            projected_size_based_on_q;

  
  
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
    
    correction_factor = (int)(100 + ((correction_factor - 100) *
                                  adjustment_limit));
    rate_correction_factor = (rate_correction_factor * correction_factor) / 100;

    
    if (rate_correction_factor > MAX_BPB_FACTOR)
      rate_correction_factor = MAX_BPB_FACTOR;
  } else if (correction_factor < 99) {
    
    correction_factor = (int)(100 - ((100 - correction_factor) *
                                  adjustment_limit));
    rate_correction_factor = (rate_correction_factor * correction_factor) / 100;

    
    if (rate_correction_factor < MIN_BPB_FACTOR)
      rate_correction_factor = MIN_BPB_FACTOR;
  }

  set_rate_correction_factor(cpi, rate_correction_factor);
}


int vp9_rc_regulate_q(const VP9_COMP *cpi, int target_bits_per_frame,
                      int active_best_quality, int active_worst_quality) {
  const VP9_COMMON *const cm = &cpi->common;
  int q = active_worst_quality;
  int last_error = INT_MAX;
  int i, target_bits_per_mb;
  const double correction_factor = get_rate_correction_factor(cpi);

  
  
  target_bits_per_mb =
      ((uint64_t)target_bits_per_frame << BPER_MB_NORMBITS) / cm->MBs;

  i = active_best_quality;

  do {
    const int bits_per_mb_at_this_q = (int)vp9_rc_bits_per_mb(cm->frame_type, i,
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
  } while (++i <= active_worst_quality);

  return q;
}

static int get_active_quality(int q, int gfu_boost, int low, int high,
                              int *low_motion_minq, int *high_motion_minq) {
  if (gfu_boost > high) {
    return low_motion_minq[q];
  } else if (gfu_boost < low) {
    return high_motion_minq[q];
  } else {
    const int gap = high - low;
    const int offset = high - gfu_boost;
    const int qdiff = high_motion_minq[q] - low_motion_minq[q];
    const int adjustment = ((offset * qdiff) + (gap >> 1)) / gap;
    return low_motion_minq[q] + adjustment;
  }
}

static int get_kf_active_quality(const RATE_CONTROL *const rc, int q) {
  return get_active_quality(q, rc->kf_boost, kf_low, kf_high,
                            kf_low_motion_minq, kf_high_motion_minq);
}

static int get_gf_active_quality(const RATE_CONTROL *const rc, int q) {
  return get_active_quality(q, rc->gfu_boost, gf_low, gf_high,
                            arfgf_low_motion_minq, arfgf_high_motion_minq);
}

static int calc_active_worst_quality_one_pass_vbr(const VP9_COMP *cpi) {
  const RATE_CONTROL *const rc = &cpi->rc;
  const unsigned int curr_frame = cpi->common.current_video_frame;
  int active_worst_quality;

  if (cpi->common.frame_type == KEY_FRAME) {
    active_worst_quality = curr_frame == 0 ? rc->worst_quality
                                           : rc->last_q[KEY_FRAME] * 2;
  } else {
    if (!rc->is_src_frame_alt_ref &&
        (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) {
      active_worst_quality =  curr_frame == 1 ? rc->last_q[KEY_FRAME] * 5 / 4
                                              : rc->last_q[INTER_FRAME];
    } else {
      active_worst_quality = curr_frame == 1 ? rc->last_q[KEY_FRAME] * 2
                                             : rc->last_q[INTER_FRAME] * 2;
    }
  }
  return MIN(active_worst_quality, rc->worst_quality);
}


static int calc_active_worst_quality_one_pass_cbr(const VP9_COMP *cpi) {
  
  
  
  
  
  const VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *rc = &cpi->rc;
  
  int64_t critical_level = rc->optimal_buffer_level >> 2;
  int64_t buff_lvl_step = 0;
  int adjustment = 0;
  int active_worst_quality;
  if (cm->frame_type == KEY_FRAME)
    return rc->worst_quality;
  if (cm->current_video_frame > 1)
    active_worst_quality = MIN(rc->worst_quality,
                               rc->avg_frame_qindex[INTER_FRAME] * 5 / 4);
  else
    active_worst_quality = MIN(rc->worst_quality,
                               rc->avg_frame_qindex[KEY_FRAME] * 3 / 2);
  if (rc->buffer_level > rc->optimal_buffer_level) {
    
    
    int max_adjustment_down = active_worst_quality / 3;
    if (max_adjustment_down) {
      buff_lvl_step = ((rc->maximum_buffer_size -
                        rc->optimal_buffer_level) / max_adjustment_down);
      if (buff_lvl_step)
        adjustment = (int)((rc->buffer_level - rc->optimal_buffer_level) /
                            buff_lvl_step);
      active_worst_quality -= adjustment;
    }
  } else if (rc->buffer_level > critical_level) {
    
    if (critical_level) {
      buff_lvl_step = (rc->optimal_buffer_level - critical_level);
      if (buff_lvl_step) {
        adjustment =
            (int)((rc->worst_quality - rc->avg_frame_qindex[INTER_FRAME]) *
                  (rc->optimal_buffer_level - rc->buffer_level) /
                  buff_lvl_step);
      }
      active_worst_quality = rc->avg_frame_qindex[INTER_FRAME] + adjustment;
    }
  } else {
    
    active_worst_quality = rc->worst_quality;
  }
  return active_worst_quality;
}

static int rc_pick_q_and_bounds_one_pass_cbr(const VP9_COMP *cpi,
                                             int *bottom_index,
                                             int *top_index) {
  const VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *const rc = &cpi->rc;
  int active_best_quality;
  int active_worst_quality = calc_active_worst_quality_one_pass_cbr(cpi);
  int q;

  if (frame_is_intra_only(cm)) {
    active_best_quality = rc->best_quality;
    
    
    
    if (rc->this_key_frame_forced) {
      int qindex = rc->last_boosted_qindex;
      double last_boosted_q = vp9_convert_qindex_to_q(qindex);
      int delta_qindex = vp9_compute_qdelta(rc, last_boosted_q,
                                            (last_boosted_q * 0.75));
      active_best_quality = MAX(qindex + delta_qindex, rc->best_quality);
    } else if (cm->current_video_frame > 0) {
      
      double q_adj_factor = 1.0;
      double q_val;

      active_best_quality =
          get_kf_active_quality(rc, rc->avg_frame_qindex[KEY_FRAME]);

      
      if ((cm->width * cm->height) <= (352 * 288)) {
        q_adj_factor -= 0.25;
      }

      
      
      q_val = vp9_convert_qindex_to_q(active_best_quality);
      active_best_quality += vp9_compute_qdelta(rc, q_val,
                                                q_val * q_adj_factor);
    }
  } else if (!rc->is_src_frame_alt_ref &&
             !cpi->use_svc &&
             (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) {
    
    
    
    if (rc->frames_since_key > 1 &&
        rc->avg_frame_qindex[INTER_FRAME] < active_worst_quality) {
      q = rc->avg_frame_qindex[INTER_FRAME];
    } else {
      q = active_worst_quality;
    }
    active_best_quality = get_gf_active_quality(rc, q);
  } else {
    
    if (cm->current_video_frame > 1) {
      if (rc->avg_frame_qindex[INTER_FRAME] < active_worst_quality)
        active_best_quality = rtc_minq[rc->avg_frame_qindex[INTER_FRAME]];
      else
        active_best_quality = rtc_minq[active_worst_quality];
    } else {
      if (rc->avg_frame_qindex[KEY_FRAME] < active_worst_quality)
        active_best_quality = rtc_minq[rc->avg_frame_qindex[KEY_FRAME]];
      else
        active_best_quality = rtc_minq[active_worst_quality];
    }
  }

  
  active_best_quality = clamp(active_best_quality,
                              rc->best_quality, rc->worst_quality);
  active_worst_quality = clamp(active_worst_quality,
                               active_best_quality, rc->worst_quality);

  *top_index = active_worst_quality;
  *bottom_index = active_best_quality;

#if LIMIT_QRANGE_FOR_ALTREF_AND_KEY
  
  if (cm->frame_type == KEY_FRAME &&
      !rc->this_key_frame_forced  &&
      !(cm->current_video_frame == 0)) {
    int qdelta = 0;
    vp9_clear_system_state();
    qdelta = vp9_compute_qdelta_by_rate(&cpi->rc, cm->frame_type,
                                        active_worst_quality, 2.0);
    *top_index = active_worst_quality + qdelta;
    *top_index = (*top_index > *bottom_index) ? *top_index : *bottom_index;
  }
#endif

  
  if (cm->frame_type == KEY_FRAME && rc->this_key_frame_forced) {
    q = rc->last_boosted_qindex;
  } else {
    q = vp9_rc_regulate_q(cpi, rc->this_frame_target,
                          active_best_quality, active_worst_quality);
    if (q > *top_index) {
      
      if (rc->this_frame_target >= rc->max_frame_bandwidth)
        *top_index = q;
      else
        q = *top_index;
    }
  }
  assert(*top_index <= rc->worst_quality &&
         *top_index >= rc->best_quality);
  assert(*bottom_index <= rc->worst_quality &&
         *bottom_index >= rc->best_quality);
  assert(q <= rc->worst_quality && q >= rc->best_quality);
  return q;
}

static int get_active_cq_level(const RATE_CONTROL *rc,
                               const VP9EncoderConfig *const oxcf) {
  static const double cq_adjust_threshold = 0.5;
  int active_cq_level = oxcf->cq_level;
  if (oxcf->rc_mode == VPX_CQ &&
      rc->total_target_bits > 0) {
    const double x = (double)rc->total_actual_bits / rc->total_target_bits;
    if (x < cq_adjust_threshold) {
      active_cq_level = (int)(active_cq_level * x / cq_adjust_threshold);
    }
  }
  return active_cq_level;
}

static int rc_pick_q_and_bounds_one_pass_vbr(const VP9_COMP *cpi,
                                             int *bottom_index,
                                             int *top_index) {
  const VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *const rc = &cpi->rc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  const int cq_level = get_active_cq_level(rc, oxcf);
  int active_best_quality;
  int active_worst_quality = calc_active_worst_quality_one_pass_vbr(cpi);
  int q;

  if (frame_is_intra_only(cm)) {

    
    
    
    if (rc->this_key_frame_forced) {
      int qindex = rc->last_boosted_qindex;
      double last_boosted_q = vp9_convert_qindex_to_q(qindex);
      int delta_qindex = vp9_compute_qdelta(rc, last_boosted_q,
                                            last_boosted_q * 0.75);
      active_best_quality = MAX(qindex + delta_qindex, rc->best_quality);
    } else {
      
      double q_adj_factor = 1.0;
      double q_val;

      active_best_quality =
          get_kf_active_quality(rc, rc->avg_frame_qindex[KEY_FRAME]);

      
      if ((cm->width * cm->height) <= (352 * 288)) {
        q_adj_factor -= 0.25;
      }

      
      
      q_val = vp9_convert_qindex_to_q(active_best_quality);
      active_best_quality += vp9_compute_qdelta(rc, q_val,
                                                q_val * q_adj_factor);
    }
  } else if (!rc->is_src_frame_alt_ref &&
             (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) {
    
    
    
    if (rc->frames_since_key > 1 &&
        rc->avg_frame_qindex[INTER_FRAME] < active_worst_quality) {
      q = rc->avg_frame_qindex[INTER_FRAME];
    } else {
      q = rc->avg_frame_qindex[KEY_FRAME];
    }
    
    if (oxcf->rc_mode == VPX_CQ) {
      if (q < cq_level)
        q = cq_level;

      active_best_quality = get_gf_active_quality(rc, q);

      
      active_best_quality = active_best_quality * 15 / 16;

    } else if (oxcf->rc_mode == VPX_Q) {
      if (!cpi->refresh_alt_ref_frame) {
        active_best_quality = cq_level;
      } else {
        active_best_quality = get_gf_active_quality(rc, q);
      }
    } else {
      active_best_quality = get_gf_active_quality(rc, q);
    }
  } else {
    if (oxcf->rc_mode == VPX_Q) {
      active_best_quality = cq_level;
    } else {
      
      if (cm->current_video_frame > 1)
        active_best_quality = inter_minq[rc->avg_frame_qindex[INTER_FRAME]];
      else
        active_best_quality = inter_minq[rc->avg_frame_qindex[KEY_FRAME]];
      
      
      if ((oxcf->rc_mode == VPX_CQ) &&
          (active_best_quality < cq_level)) {
        active_best_quality = cq_level;
      }
    }
  }

  
  active_best_quality = clamp(active_best_quality,
                              rc->best_quality, rc->worst_quality);
  active_worst_quality = clamp(active_worst_quality,
                               active_best_quality, rc->worst_quality);

  *top_index = active_worst_quality;
  *bottom_index = active_best_quality;

#if LIMIT_QRANGE_FOR_ALTREF_AND_KEY
  {
    int qdelta = 0;
    vp9_clear_system_state();

    
    if (cm->frame_type == KEY_FRAME &&
        !rc->this_key_frame_forced &&
        !(cm->current_video_frame == 0)) {
      qdelta = vp9_compute_qdelta_by_rate(&cpi->rc, cm->frame_type,
                                          active_worst_quality, 2.0);
    } else if (!rc->is_src_frame_alt_ref &&
               (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) {
      qdelta = vp9_compute_qdelta_by_rate(&cpi->rc, cm->frame_type,
                                          active_worst_quality, 1.75);
    }
    *top_index = active_worst_quality + qdelta;
    *top_index = (*top_index > *bottom_index) ? *top_index : *bottom_index;
  }
#endif

  if (oxcf->rc_mode == VPX_Q) {
    q = active_best_quality;
  
  } else if ((cm->frame_type == KEY_FRAME) && rc->this_key_frame_forced) {
    q = rc->last_boosted_qindex;
  } else {
    q = vp9_rc_regulate_q(cpi, rc->this_frame_target,
                          active_best_quality, active_worst_quality);
    if (q > *top_index) {
      
      if (rc->this_frame_target >= rc->max_frame_bandwidth)
        *top_index = q;
      else
        q = *top_index;
    }
  }

  assert(*top_index <= rc->worst_quality &&
         *top_index >= rc->best_quality);
  assert(*bottom_index <= rc->worst_quality &&
         *bottom_index >= rc->best_quality);
  assert(q <= rc->worst_quality && q >= rc->best_quality);
  return q;
}

static int rc_pick_q_and_bounds_two_pass(const VP9_COMP *cpi,
                                         int *bottom_index,
                                         int *top_index) {
  const VP9_COMMON *const cm = &cpi->common;
  const RATE_CONTROL *const rc = &cpi->rc;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  const int cq_level = get_active_cq_level(rc, oxcf);
  int active_best_quality;
  int active_worst_quality = cpi->twopass.active_worst_quality;
  int q;

  if (frame_is_intra_only(cm) || vp9_is_upper_layer_key_frame(cpi)) {
    
    
    
    if (rc->this_key_frame_forced) {
      int qindex = rc->last_boosted_qindex;
      double last_boosted_q = vp9_convert_qindex_to_q(qindex);
      int delta_qindex = vp9_compute_qdelta(rc, last_boosted_q,
                                            last_boosted_q * 0.75);
      active_best_quality = MAX(qindex + delta_qindex, rc->best_quality);
    } else {
      
      double q_adj_factor = 1.0;
      double q_val;
      
      active_best_quality = get_kf_active_quality(rc, active_worst_quality);

      
      if ((cm->width * cm->height) <= (352 * 288)) {
        q_adj_factor -= 0.25;
      }

      
      q_adj_factor += 0.05 - (0.001 * (double)cpi->twopass.kf_zeromotion_pct);

      
      
      q_val = vp9_convert_qindex_to_q(active_best_quality);
      active_best_quality += vp9_compute_qdelta(rc, q_val,
                                                q_val * q_adj_factor);
    }
  } else if (!rc->is_src_frame_alt_ref &&
             (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) {
    
    
    
    if (rc->frames_since_key > 1 &&
        rc->avg_frame_qindex[INTER_FRAME] < active_worst_quality) {
      q = rc->avg_frame_qindex[INTER_FRAME];
    } else {
      q = active_worst_quality;
    }
    
    if (oxcf->rc_mode == VPX_CQ) {
      if (q < cq_level)
        q = cq_level;

      active_best_quality = get_gf_active_quality(rc, q);

      
      active_best_quality = active_best_quality * 15 / 16;

    } else if (oxcf->rc_mode == VPX_Q) {
      if (!cpi->refresh_alt_ref_frame) {
        active_best_quality = cq_level;
      } else {
        active_best_quality = get_gf_active_quality(rc, q);
      }
    } else {
      active_best_quality = get_gf_active_quality(rc, q);
    }
  } else {
    if (oxcf->rc_mode == VPX_Q) {
      active_best_quality = cq_level;
    } else {
      active_best_quality = inter_minq[active_worst_quality];

      
      
      if ((oxcf->rc_mode == VPX_CQ) &&
          (active_best_quality < cq_level)) {
        active_best_quality = cq_level;
      }
    }
  }

  
  active_best_quality = clamp(active_best_quality,
                              rc->best_quality, rc->worst_quality);
  active_worst_quality = clamp(active_worst_quality,
                               active_best_quality, rc->worst_quality);

  *top_index = active_worst_quality;
  *bottom_index = active_best_quality;

#if LIMIT_QRANGE_FOR_ALTREF_AND_KEY
  vp9_clear_system_state();
  {
    const GF_GROUP *const gf_group = &cpi->twopass.gf_group;
    const double rate_factor_deltas[RATE_FACTOR_LEVELS] = {
      1.00,  
      1.00,  
      1.50,  
      1.75,  
      2.00,  
    };
    const double rate_factor =
      rate_factor_deltas[gf_group->rf_level[gf_group->index]];
    int qdelta = vp9_compute_qdelta_by_rate(&cpi->rc, cm->frame_type,
                                            active_worst_quality, rate_factor);
    *top_index = active_worst_quality + qdelta;
    *top_index = (*top_index > *bottom_index) ? *top_index : *bottom_index;
  }
#endif

  if (oxcf->rc_mode == VPX_Q) {
    q = active_best_quality;
  
  } else if ((cm->frame_type == KEY_FRAME) && rc->this_key_frame_forced) {
    q = rc->last_boosted_qindex;
  } else {
    q = vp9_rc_regulate_q(cpi, rc->this_frame_target,
                          active_best_quality, active_worst_quality);
    if (q > *top_index) {
      
      if (rc->this_frame_target >= rc->max_frame_bandwidth)
        *top_index = q;
      else
        q = *top_index;
    }
  }

  assert(*top_index <= rc->worst_quality &&
         *top_index >= rc->best_quality);
  assert(*bottom_index <= rc->worst_quality &&
         *bottom_index >= rc->best_quality);
  assert(q <= rc->worst_quality && q >= rc->best_quality);
  return q;
}

int vp9_rc_pick_q_and_bounds(const VP9_COMP *cpi,
                             int *bottom_index, int *top_index) {
  int q;
  if (cpi->oxcf.pass == 0) {
    if (cpi->oxcf.rc_mode == VPX_CBR)
      q = rc_pick_q_and_bounds_one_pass_cbr(cpi, bottom_index, top_index);
    else
      q = rc_pick_q_and_bounds_one_pass_vbr(cpi, bottom_index, top_index);
  } else {
    q = rc_pick_q_and_bounds_two_pass(cpi, bottom_index, top_index);
  }
  if (cpi->sf.use_nonrd_pick_mode) {
    if (cpi->sf.force_frame_boost == 1)
      q -= cpi->sf.max_delta_qindex;

    if (q < *bottom_index)
      *bottom_index = q;
    else if (q > *top_index)
      *top_index = q;
  }
  return q;
}

void vp9_rc_compute_frame_size_bounds(const VP9_COMP *cpi,
                                      int frame_target,
                                      int *frame_under_shoot_limit,
                                      int *frame_over_shoot_limit) {
  if (cpi->oxcf.rc_mode == VPX_Q) {
    *frame_under_shoot_limit = 0;
    *frame_over_shoot_limit  = INT_MAX;
  } else {
    
    
    const int tolerance = (cpi->sf.recode_tolerance * frame_target) / 100;
    *frame_under_shoot_limit = MAX(frame_target - tolerance - 200, 0);
    *frame_over_shoot_limit = MIN(frame_target + tolerance + 200,
                                  cpi->rc.max_frame_bandwidth);
  }
}

void vp9_rc_set_frame_target(VP9_COMP *cpi, int target) {
  const VP9_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;

  rc->this_frame_target = target;

  
  rc->sb64_target_rate = ((int64_t)rc->this_frame_target * 64 * 64) /
                             (cm->width * cm->height);
}

static void update_alt_ref_frame_stats(VP9_COMP *cpi) {
  
  RATE_CONTROL *const rc = &cpi->rc;
  rc->frames_since_golden = 0;

  
  rc->source_alt_ref_pending = 0;

  
  rc->source_alt_ref_active = 1;
}

static void update_golden_frame_stats(VP9_COMP *cpi) {
  RATE_CONTROL *const rc = &cpi->rc;

  
  if (cpi->refresh_golden_frame) {
    
    rc->frames_since_golden = 0;

    if (cpi->oxcf.pass == 2) {
      if (!rc->source_alt_ref_pending &&
          cpi->twopass.gf_group.rf_level[0] == GF_ARF_STD)
      rc->source_alt_ref_active = 0;
    } else if (!rc->source_alt_ref_pending) {
      rc->source_alt_ref_active = 0;
    }

    
    if (rc->frames_till_gf_update_due > 0)
      rc->frames_till_gf_update_due--;

  } else if (!cpi->refresh_alt_ref_frame) {
    
    if (rc->frames_till_gf_update_due > 0)
      rc->frames_till_gf_update_due--;

    rc->frames_since_golden++;
  }
}

void vp9_rc_postencode_update(VP9_COMP *cpi, uint64_t bytes_used) {
  const VP9_COMMON *const cm = &cpi->common;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  RATE_CONTROL *const rc = &cpi->rc;
  const int qindex = cm->base_qindex;

  
  rc->projected_frame_size = (int)(bytes_used << 3);

  
  vp9_rc_update_rate_correction_factors(
      cpi, (cpi->sf.recode_loop >= ALLOW_RECODE_KFARFGF ||
            oxcf->rc_mode == VPX_CBR) ? 2 : 0);

  
  if (cm->frame_type == KEY_FRAME) {
    rc->last_q[KEY_FRAME] = qindex;
    rc->avg_frame_qindex[KEY_FRAME] =
        ROUND_POWER_OF_TWO(3 * rc->avg_frame_qindex[KEY_FRAME] + qindex, 2);
  } else {
    if (rc->is_src_frame_alt_ref ||
        !(cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame) ||
        (cpi->use_svc && oxcf->rc_mode == VPX_CBR)) {
      rc->last_q[INTER_FRAME] = qindex;
      rc->avg_frame_qindex[INTER_FRAME] =
        ROUND_POWER_OF_TWO(3 * rc->avg_frame_qindex[INTER_FRAME] + qindex, 2);
      rc->ni_frames++;
      rc->tot_q += vp9_convert_qindex_to_q(qindex);
      rc->avg_q = rc->tot_q / rc->ni_frames;
      
      
      rc->ni_tot_qi += qindex;
      rc->ni_av_qi = rc->ni_tot_qi / rc->ni_frames;
    }
  }

  
  
  
  
  
  if ((qindex < rc->last_boosted_qindex) ||
      ((cpi->static_mb_pct < 100) &&
       ((cm->frame_type == KEY_FRAME) || cpi->refresh_alt_ref_frame ||
        (cpi->refresh_golden_frame && !rc->is_src_frame_alt_ref)))) {
    rc->last_boosted_qindex = qindex;
  }

  update_buffer_level(cpi, rc->projected_frame_size);

  
  
  if (cm->frame_type != KEY_FRAME) {
    rc->rolling_target_bits = ROUND_POWER_OF_TWO(
        rc->rolling_target_bits * 3 + rc->this_frame_target, 2);
    rc->rolling_actual_bits = ROUND_POWER_OF_TWO(
        rc->rolling_actual_bits * 3 + rc->projected_frame_size, 2);
    rc->long_rolling_target_bits = ROUND_POWER_OF_TWO(
        rc->long_rolling_target_bits * 31 + rc->this_frame_target, 5);
    rc->long_rolling_actual_bits = ROUND_POWER_OF_TWO(
        rc->long_rolling_actual_bits * 31 + rc->projected_frame_size, 5);
  }

  
  rc->total_actual_bits += rc->projected_frame_size;
  rc->total_target_bits += cm->show_frame ? rc->avg_frame_bandwidth : 0;

  rc->total_target_vs_actual = rc->total_actual_bits - rc->total_target_bits;

  if (is_altref_enabled(cpi) && cpi->refresh_alt_ref_frame &&
      (cm->frame_type != KEY_FRAME))
    
    update_alt_ref_frame_stats(cpi);
  else
    
    update_golden_frame_stats(cpi);

  if (cm->frame_type == KEY_FRAME)
    rc->frames_since_key = 0;
  if (cm->show_frame) {
    rc->frames_since_key++;
    rc->frames_to_key--;
  }
}

void vp9_rc_postencode_update_drop_frame(VP9_COMP *cpi) {
  
  update_buffer_level(cpi, 0);
  cpi->common.last_frame_type = cpi->common.frame_type;
  cpi->rc.frames_since_key++;
  cpi->rc.frames_to_key--;
}


#define USE_ALTREF_FOR_ONE_PASS   1

static int calc_pframe_target_size_one_pass_vbr(const VP9_COMP *const cpi) {
  static const int af_ratio = 10;
  const RATE_CONTROL *const rc = &cpi->rc;
  int target;
#if USE_ALTREF_FOR_ONE_PASS
  target = (!rc->is_src_frame_alt_ref &&
            (cpi->refresh_golden_frame || cpi->refresh_alt_ref_frame)) ?
      (rc->avg_frame_bandwidth * rc->baseline_gf_interval * af_ratio) /
      (rc->baseline_gf_interval + af_ratio - 1) :
      (rc->avg_frame_bandwidth * rc->baseline_gf_interval) /
      (rc->baseline_gf_interval + af_ratio - 1);
#else
  target = rc->avg_frame_bandwidth;
#endif
  return vp9_rc_clamp_pframe_target_size(cpi, target);
}

static int calc_iframe_target_size_one_pass_vbr(const VP9_COMP *const cpi) {
  static const int kf_ratio = 25;
  const RATE_CONTROL *rc = &cpi->rc;
  const int target = rc->avg_frame_bandwidth * kf_ratio;
  return vp9_rc_clamp_iframe_target_size(cpi, target);
}

void vp9_rc_get_one_pass_vbr_params(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;
  int target;
  
  if (!cpi->refresh_alt_ref_frame &&
      (cm->current_video_frame == 0 ||
       (cpi->frame_flags & FRAMEFLAGS_KEY) ||
       rc->frames_to_key == 0 ||
       (cpi->oxcf.auto_key && 0))) {
    cm->frame_type = KEY_FRAME;
    rc->this_key_frame_forced = cm->current_video_frame != 0 &&
                                rc->frames_to_key == 0;
    rc->frames_to_key = cpi->oxcf.key_freq;
    rc->kf_boost = DEFAULT_KF_BOOST;
    rc->source_alt_ref_active = 0;
  } else {
    cm->frame_type = INTER_FRAME;
  }
  if (rc->frames_till_gf_update_due == 0) {
    rc->baseline_gf_interval = DEFAULT_GF_INTERVAL;
    rc->frames_till_gf_update_due = rc->baseline_gf_interval;
    
    if (rc->frames_till_gf_update_due > rc->frames_to_key)
      rc->frames_till_gf_update_due = rc->frames_to_key;
    cpi->refresh_golden_frame = 1;
    rc->source_alt_ref_pending = USE_ALTREF_FOR_ONE_PASS;
    rc->gfu_boost = DEFAULT_GF_BOOST;
  }
  if (cm->frame_type == KEY_FRAME)
    target = calc_iframe_target_size_one_pass_vbr(cpi);
  else
    target = calc_pframe_target_size_one_pass_vbr(cpi);
  vp9_rc_set_frame_target(cpi, target);
}

static int calc_pframe_target_size_one_pass_cbr(const VP9_COMP *cpi) {
  const VP9EncoderConfig *oxcf = &cpi->oxcf;
  const RATE_CONTROL *rc = &cpi->rc;
  const SVC *const svc = &cpi->svc;
  const int64_t diff = rc->optimal_buffer_level - rc->buffer_level;
  const int64_t one_pct_bits = 1 + rc->optimal_buffer_level / 100;
  int min_frame_target = MAX(rc->avg_frame_bandwidth >> 4, FRAME_OVERHEAD_BITS);
  int target = rc->avg_frame_bandwidth;
  if (svc->number_temporal_layers > 1 &&
      oxcf->rc_mode == VPX_CBR) {
    
    
    
    int current_temporal_layer = svc->temporal_layer_id;
    const LAYER_CONTEXT *lc = &svc->layer_context[current_temporal_layer];
    target = lc->avg_frame_size;
    min_frame_target = MAX(lc->avg_frame_size >> 4, FRAME_OVERHEAD_BITS);
  }
  if (diff > 0) {
    
    const int pct_low = (int)MIN(diff / one_pct_bits, oxcf->under_shoot_pct);
    target -= (target * pct_low) / 200;
  } else if (diff < 0) {
    
    const int pct_high = (int)MIN(-diff / one_pct_bits, oxcf->over_shoot_pct);
    target += (target * pct_high) / 200;
  }
  return MAX(min_frame_target, target);
}

static int calc_iframe_target_size_one_pass_cbr(const VP9_COMP *cpi) {
  const RATE_CONTROL *rc = &cpi->rc;
  const VP9EncoderConfig *oxcf = &cpi->oxcf;
  const SVC *const svc = &cpi->svc;
  int target;
  if (cpi->common.current_video_frame == 0) {
    target = ((rc->starting_buffer_level / 2) > INT_MAX)
      ? INT_MAX : (int)(rc->starting_buffer_level / 2);
  } else {
    int kf_boost = 32;
    double framerate = cpi->framerate;
    if (svc->number_temporal_layers > 1 &&
        oxcf->rc_mode == VPX_CBR) {
      
      const LAYER_CONTEXT *lc = &svc->layer_context[svc->temporal_layer_id];
      framerate = lc->framerate;
    }
    kf_boost = MAX(kf_boost, (int)(2 * framerate - 16));
    if (rc->frames_since_key <  framerate / 2) {
      kf_boost = (int)(kf_boost * rc->frames_since_key /
                       (framerate / 2));
    }
    target = ((16 + kf_boost) * rc->avg_frame_bandwidth) >> 4;
  }
  return vp9_rc_clamp_iframe_target_size(cpi, target);
}

void vp9_rc_get_svc_params(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;
  int target = rc->avg_frame_bandwidth;
  if ((cm->current_video_frame == 0) ||
      (cpi->frame_flags & FRAMEFLAGS_KEY) ||
      (cpi->oxcf.auto_key && (rc->frames_since_key %
          cpi->oxcf.key_freq == 0))) {
    cm->frame_type = KEY_FRAME;
    rc->source_alt_ref_active = 0;

    if (is_two_pass_svc(cpi)) {
      cpi->svc.layer_context[cpi->svc.spatial_layer_id].is_key_frame = 1;
      cpi->ref_frame_flags &=
          (~VP9_LAST_FLAG & ~VP9_GOLD_FLAG & ~VP9_ALT_FLAG);
    }

    if (cpi->oxcf.pass == 0 && cpi->oxcf.rc_mode == VPX_CBR) {
      target = calc_iframe_target_size_one_pass_cbr(cpi);
    }
  } else {
    cm->frame_type = INTER_FRAME;

    if (is_two_pass_svc(cpi)) {
      LAYER_CONTEXT *lc = &cpi->svc.layer_context[cpi->svc.spatial_layer_id];
      if (cpi->svc.spatial_layer_id == 0) {
        lc->is_key_frame = 0;
      } else {
        lc->is_key_frame = cpi->svc.layer_context[0].is_key_frame;
        if (lc->is_key_frame)
          cpi->ref_frame_flags &= (~VP9_LAST_FLAG);
      }
      cpi->ref_frame_flags &= (~VP9_ALT_FLAG);
    }

    if (cpi->oxcf.pass == 0 && cpi->oxcf.rc_mode == VPX_CBR) {
      target = calc_pframe_target_size_one_pass_cbr(cpi);
    }
  }
  vp9_rc_set_frame_target(cpi, target);
  rc->frames_till_gf_update_due = INT_MAX;
  rc->baseline_gf_interval = INT_MAX;
}

void vp9_rc_get_one_pass_cbr_params(VP9_COMP *cpi) {
  VP9_COMMON *const cm = &cpi->common;
  RATE_CONTROL *const rc = &cpi->rc;
  int target;
  
  if ((cm->current_video_frame == 0 ||
      (cpi->frame_flags & FRAMEFLAGS_KEY) ||
      rc->frames_to_key == 0 ||
      (cpi->oxcf.auto_key && 0))) {
    cm->frame_type = KEY_FRAME;
    rc->this_key_frame_forced = cm->current_video_frame != 0 &&
                                rc->frames_to_key == 0;
    rc->frames_to_key = cpi->oxcf.key_freq;
    rc->kf_boost = DEFAULT_KF_BOOST;
    rc->source_alt_ref_active = 0;
    target = calc_iframe_target_size_one_pass_cbr(cpi);
  } else {
    cm->frame_type = INTER_FRAME;
    target = calc_pframe_target_size_one_pass_cbr(cpi);
  }
  vp9_rc_set_frame_target(cpi, target);
  
  rc->frames_till_gf_update_due = INT_MAX;
  rc->baseline_gf_interval = INT_MAX;
}

int vp9_compute_qdelta(const RATE_CONTROL *rc, double qstart, double qtarget) {
  int start_index = rc->worst_quality;
  int target_index = rc->worst_quality;
  int i;

  
  for (i = rc->best_quality; i < rc->worst_quality; ++i) {
    start_index = i;
    if (vp9_convert_qindex_to_q(i) >= qstart)
      break;
  }

  
  for (i = rc->best_quality; i < rc->worst_quality; ++i) {
    target_index = i;
    if (vp9_convert_qindex_to_q(i) >= qtarget)
      break;
  }

  return target_index - start_index;
}

int vp9_compute_qdelta_by_rate(const RATE_CONTROL *rc, FRAME_TYPE frame_type,
                               int qindex, double rate_target_ratio) {
  int target_index = rc->worst_quality;
  int i;

  
  const int base_bits_per_mb = vp9_rc_bits_per_mb(frame_type, qindex, 1.0);

  
  const int target_bits_per_mb = (int)(rate_target_ratio * base_bits_per_mb);

  
  for (i = rc->best_quality; i < rc->worst_quality; ++i) {
    target_index = i;
    if (vp9_rc_bits_per_mb(frame_type, i, 1.0) <= target_bits_per_mb )
      break;
  }

  return target_index - qindex;
}

void vp9_rc_set_gf_max_interval(const VP9_COMP *const cpi,
                                RATE_CONTROL *const rc) {
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  
  rc->max_gf_interval = 16;

  
  rc->static_scene_max_gf_interval = oxcf->key_freq >> 1;
  if (rc->static_scene_max_gf_interval > (MAX_LAG_BUFFERS * 2))
    rc->static_scene_max_gf_interval = MAX_LAG_BUFFERS * 2;

  if (is_altref_enabled(cpi)) {
    if (rc->static_scene_max_gf_interval > oxcf->lag_in_frames - 1)
      rc->static_scene_max_gf_interval = oxcf->lag_in_frames - 1;
  }

  if (rc->max_gf_interval > rc->static_scene_max_gf_interval)
    rc->max_gf_interval = rc->static_scene_max_gf_interval;
}

void vp9_rc_update_framerate(VP9_COMP *cpi) {
  const VP9_COMMON *const cm = &cpi->common;
  const VP9EncoderConfig *const oxcf = &cpi->oxcf;
  RATE_CONTROL *const rc = &cpi->rc;
  int vbr_max_bits;

  rc->avg_frame_bandwidth = (int)(oxcf->target_bandwidth / cpi->framerate);
  rc->min_frame_bandwidth = (int)(rc->avg_frame_bandwidth *
                                oxcf->two_pass_vbrmin_section / 100);

  rc->min_frame_bandwidth = MAX(rc->min_frame_bandwidth, FRAME_OVERHEAD_BITS);

  
  
  
  
  
  
  
  vbr_max_bits = (int)(((int64_t)rc->avg_frame_bandwidth *
                     oxcf->two_pass_vbrmax_section) / 100);
  rc->max_frame_bandwidth = MAX(MAX((cm->MBs * MAX_MB_RATE), MAXRATE_1080P),
                                    vbr_max_bits);

  vp9_rc_set_gf_max_interval(cpi, rc);
}
