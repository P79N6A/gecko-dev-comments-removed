









#include "./vp9_rtcd.h"
#include "vp9/common/vp9_filter.h"
#include "vp9/common/vp9_scale.h"

static INLINE int scaled_x(int val, const struct scale_factors *sf) {
  return val * sf->x_scale_fp >> REF_SCALE_SHIFT;
}

static INLINE int scaled_y(int val, const struct scale_factors *sf) {
  return val * sf->y_scale_fp >> REF_SCALE_SHIFT;
}

static int unscaled_value(int val, const struct scale_factors *sf) {
  (void) sf;
  return val;
}

static int get_fixed_point_scale_factor(int other_size, int this_size) {
  
  
  
  
  return (other_size << REF_SCALE_SHIFT) / this_size;
}

static int check_scale_factors(int other_w, int other_h,
                               int this_w, int this_h) {
  return 2 * this_w >= other_w &&
         2 * this_h >= other_h &&
         this_w <= 16 * other_w &&
         this_h <= 16 * other_h;
}

MV32 vp9_scale_mv(const MV *mv, int x, int y, const struct scale_factors *sf) {
  const int x_off_q4 = scaled_x(x << SUBPEL_BITS, sf) & SUBPEL_MASK;
  const int y_off_q4 = scaled_y(y << SUBPEL_BITS, sf) & SUBPEL_MASK;
  const MV32 res = {
    scaled_y(mv->row, sf) + y_off_q4,
    scaled_x(mv->col, sf) + x_off_q4
  };
  return res;
}

void vp9_setup_scale_factors_for_frame(struct scale_factors *sf,
                                       int other_w, int other_h,
                                       int this_w, int this_h) {
  if (!check_scale_factors(other_w, other_h, this_w, this_h)) {
    sf->x_scale_fp = REF_INVALID_SCALE;
    sf->y_scale_fp = REF_INVALID_SCALE;
    return;
  }

  sf->x_scale_fp = get_fixed_point_scale_factor(other_w, this_w);
  sf->y_scale_fp = get_fixed_point_scale_factor(other_h, this_h);
  sf->x_step_q4 = scaled_x(16, sf);
  sf->y_step_q4 = scaled_y(16, sf);

  if (vp9_is_scaled(sf)) {
    sf->scale_value_x = scaled_x;
    sf->scale_value_y = scaled_y;
  } else {
    sf->scale_value_x = unscaled_value;
    sf->scale_value_y = unscaled_value;
  }

  
  
  
  
  
  
  if (sf->x_step_q4 == 16) {
    if (sf->y_step_q4 == 16) {
      
      sf->predict[0][0][0] = vp9_convolve_copy;
      sf->predict[0][0][1] = vp9_convolve_avg;
      sf->predict[0][1][0] = vp9_convolve8_vert;
      sf->predict[0][1][1] = vp9_convolve8_avg_vert;
      sf->predict[1][0][0] = vp9_convolve8_horiz;
      sf->predict[1][0][1] = vp9_convolve8_avg_horiz;
    } else {
      
      sf->predict[0][0][0] = vp9_convolve8_vert;
      sf->predict[0][0][1] = vp9_convolve8_avg_vert;
      sf->predict[0][1][0] = vp9_convolve8_vert;
      sf->predict[0][1][1] = vp9_convolve8_avg_vert;
      sf->predict[1][0][0] = vp9_convolve8;
      sf->predict[1][0][1] = vp9_convolve8_avg;
    }
  } else {
    if (sf->y_step_q4 == 16) {
      
      sf->predict[0][0][0] = vp9_convolve8_horiz;
      sf->predict[0][0][1] = vp9_convolve8_avg_horiz;
      sf->predict[0][1][0] = vp9_convolve8;
      sf->predict[0][1][1] = vp9_convolve8_avg;
      sf->predict[1][0][0] = vp9_convolve8_horiz;
      sf->predict[1][0][1] = vp9_convolve8_avg_horiz;
    } else {
      
      sf->predict[0][0][0] = vp9_convolve8;
      sf->predict[0][0][1] = vp9_convolve8_avg;
      sf->predict[0][1][0] = vp9_convolve8;
      sf->predict[0][1][1] = vp9_convolve8_avg;
      sf->predict[1][0][0] = vp9_convolve8;
      sf->predict[1][0][1] = vp9_convolve8_avg;
    }
  }
  
  sf->predict[1][1][0] = vp9_convolve8;
  sf->predict[1][1][1] = vp9_convolve8_avg;
}
