









#include <assert.h>
#include <limits.h>

#include "./vpx_scale_rtcd.h"

#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/mem.h"

#include "vp9/common/vp9_loopfilter.h"
#include "vp9/common/vp9_onyxc_int.h"
#include "vp9/common/vp9_quant_common.h"

#include "vp9/encoder/vp9_encoder.h"
#include "vp9/encoder/vp9_picklpf.h"
#include "vp9/encoder/vp9_quantize.h"

static int get_max_filter_level(const VP9_COMP *cpi) {
  if (cpi->oxcf.pass == 2) {
    return cpi->twopass.section_intra_rating > 8 ? MAX_LOOP_FILTER * 3 / 4
                                                 : MAX_LOOP_FILTER;
  } else {
    return MAX_LOOP_FILTER;
  }
}


static int64_t try_filter_frame(const YV12_BUFFER_CONFIG *sd,
                                VP9_COMP *const cpi,
                                int filt_level, int partial_frame) {
  VP9_COMMON *const cm = &cpi->common;
  int64_t filt_err;

  if (cpi->num_workers > 1)
    vp9_loop_filter_frame_mt(cm->frame_to_show, cm, cpi->td.mb.e_mbd.plane,
                             filt_level, 1, partial_frame,
                             cpi->workers, cpi->num_workers, &cpi->lf_row_sync);
  else
    vp9_loop_filter_frame(cm->frame_to_show, cm, &cpi->td.mb.e_mbd, filt_level,
                          1, partial_frame);

#if CONFIG_VP9_HIGHBITDEPTH
  if (cm->use_highbitdepth) {
    filt_err = vp9_highbd_get_y_sse(sd, cm->frame_to_show);
  } else {
    filt_err = vp9_get_y_sse(sd, cm->frame_to_show);
  }
#else
  filt_err = vp9_get_y_sse(sd, cm->frame_to_show);
#endif  

  
  vpx_yv12_copy_y(&cpi->last_frame_uf, cm->frame_to_show);

  return filt_err;
}

static int search_filter_level(const YV12_BUFFER_CONFIG *sd, VP9_COMP *cpi,
                               int partial_frame) {
  const VP9_COMMON *const cm = &cpi->common;
  const struct loopfilter *const lf = &cm->lf;
  const int min_filter_level = 0;
  const int max_filter_level = get_max_filter_level(cpi);
  int filt_direction = 0;
  int64_t best_err;
  int filt_best;

  
  
  int filt_mid = clamp(lf->filter_level, min_filter_level, max_filter_level);
  int filter_step = filt_mid < 16 ? 4 : filt_mid / 4;
  
  int64_t ss_err[MAX_LOOP_FILTER + 1];

  
  memset(ss_err, 0xFF, sizeof(ss_err));

  
  vpx_yv12_copy_y(cm->frame_to_show, &cpi->last_frame_uf);

  best_err = try_filter_frame(sd, cpi, filt_mid, partial_frame);
  filt_best = filt_mid;
  ss_err[filt_mid] = best_err;

  while (filter_step > 0) {
    const int filt_high = MIN(filt_mid + filter_step, max_filter_level);
    const int filt_low = MAX(filt_mid - filter_step, min_filter_level);

    
    int64_t bias = (best_err >> (15 - (filt_mid / 8))) * filter_step;

    if ((cpi->oxcf.pass == 2) && (cpi->twopass.section_intra_rating < 20))
      bias = (bias * cpi->twopass.section_intra_rating) / 20;

    
    if (cm->tx_mode != ONLY_4X4)
      bias >>= 1;

    if (filt_direction <= 0 && filt_low != filt_mid) {
      
      if (ss_err[filt_low] < 0) {
        ss_err[filt_low] = try_filter_frame(sd, cpi, filt_low, partial_frame);
      }
      
      
      if ((ss_err[filt_low] - bias) < best_err) {
        
        if (ss_err[filt_low] < best_err)
          best_err = ss_err[filt_low];

        filt_best = filt_low;
      }
    }

    
    if (filt_direction >= 0 && filt_high != filt_mid) {
      if (ss_err[filt_high] < 0) {
        ss_err[filt_high] = try_filter_frame(sd, cpi, filt_high, partial_frame);
      }
      
      if (ss_err[filt_high] < (best_err - bias)) {
        best_err = ss_err[filt_high];
        filt_best = filt_high;
      }
    }

    
    if (filt_best == filt_mid) {
      filter_step /= 2;
      filt_direction = 0;
    } else {
      filt_direction = (filt_best < filt_mid) ? -1 : 1;
      filt_mid = filt_best;
    }
  }

  return filt_best;
}

void vp9_pick_filter_level(const YV12_BUFFER_CONFIG *sd, VP9_COMP *cpi,
                           LPF_PICK_METHOD method) {
  VP9_COMMON *const cm = &cpi->common;
  struct loopfilter *const lf = &cm->lf;

  lf->sharpness_level = cm->frame_type == KEY_FRAME ? 0
                                                    : cpi->oxcf.sharpness;

  if (method == LPF_PICK_MINIMAL_LPF && lf->filter_level) {
      lf->filter_level = 0;
  } else if (method >= LPF_PICK_FROM_Q) {
    const int min_filter_level = 0;
    const int max_filter_level = get_max_filter_level(cpi);
    const int q = vp9_ac_quant(cm->base_qindex, 0, cm->bit_depth);
    
    
#if CONFIG_VP9_HIGHBITDEPTH
    int filt_guess;
    switch (cm->bit_depth) {
      case VPX_BITS_8:
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 1015158, 18);
        break;
      case VPX_BITS_10:
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 4060632, 20);
        break;
      case VPX_BITS_12:
        filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 16242526, 22);
        break;
      default:
        assert(0 && "bit_depth should be VPX_BITS_8, VPX_BITS_10 "
                    "or VPX_BITS_12");
        return;
    }
#else
    int filt_guess = ROUND_POWER_OF_TWO(q * 20723 + 1015158, 18);
#endif  
    if (cm->frame_type == KEY_FRAME)
      filt_guess -= 4;
    lf->filter_level = clamp(filt_guess, min_filter_level, max_filter_level);
  } else {
    lf->filter_level = search_filter_level(sd, cpi,
                                           method == LPF_PICK_FROM_SUBIMAGE);
  }
}
