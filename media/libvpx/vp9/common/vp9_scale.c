









#include "./vp9_rtcd.h"
#include "vp9/common/vp9_filter.h"
#include "vp9/common/vp9_scale.h"

static INLINE int scaled_x(int val, const struct scale_factors_common *sfc) {
  return val * sfc->x_scale_fp >> REF_SCALE_SHIFT;
}

static INLINE int scaled_y(int val, const struct scale_factors_common *sfc) {
  return val * sfc->y_scale_fp >> REF_SCALE_SHIFT;
}

static int unscaled_value(int val, const struct scale_factors_common *sfc) {
  (void) sfc;
  return val;
}

static MV32 scaled_mv(const MV *mv, const struct scale_factors *scale) {
  const MV32 res = {
    scaled_y(mv->row, scale->sfc) + scale->y_offset_q4,
    scaled_x(mv->col, scale->sfc) + scale->x_offset_q4
  };
  return res;
}

static MV32 unscaled_mv(const MV *mv, const struct scale_factors *scale) {
  const MV32 res = {
    mv->row,
    mv->col
  };
  return res;
}

static void set_offsets_with_scaling(struct scale_factors *scale,
                                     int row, int col) {
  scale->x_offset_q4 = scaled_x(col << SUBPEL_BITS, scale->sfc) & SUBPEL_MASK;
  scale->y_offset_q4 = scaled_y(row << SUBPEL_BITS, scale->sfc) & SUBPEL_MASK;
}

static void set_offsets_without_scaling(struct scale_factors *scale,
                                        int row, int col) {
  scale->x_offset_q4 = 0;
  scale->y_offset_q4 = 0;
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

void vp9_setup_scale_factors_for_frame(struct scale_factors *scale,
                                       struct scale_factors_common *scale_comm,
                                       int other_w, int other_h,
                                       int this_w, int this_h) {
  if (!check_scale_factors(other_w, other_h, this_w, this_h)) {
    scale_comm->x_scale_fp = REF_INVALID_SCALE;
    scale_comm->y_scale_fp = REF_INVALID_SCALE;
    return;
  }

  scale_comm->x_scale_fp = get_fixed_point_scale_factor(other_w, this_w);
  scale_comm->y_scale_fp = get_fixed_point_scale_factor(other_h, this_h);
  scale_comm->x_step_q4 = scaled_x(16, scale_comm);
  scale_comm->y_step_q4 = scaled_y(16, scale_comm);

  if (vp9_is_scaled(scale_comm)) {
    scale_comm->scale_value_x = scaled_x;
    scale_comm->scale_value_y = scaled_y;
    scale_comm->set_scaled_offsets = set_offsets_with_scaling;
    scale_comm->scale_mv = scaled_mv;
  } else {
    scale_comm->scale_value_x = unscaled_value;
    scale_comm->scale_value_y = unscaled_value;
    scale_comm->set_scaled_offsets = set_offsets_without_scaling;
    scale_comm->scale_mv = unscaled_mv;
  }

  
  
  
  
  
  
  if (scale_comm->x_step_q4 == 16) {
    if (scale_comm->y_step_q4 == 16) {
      
      scale_comm->predict[0][0][0] = vp9_convolve_copy;
      scale_comm->predict[0][0][1] = vp9_convolve_avg;
      scale_comm->predict[0][1][0] = vp9_convolve8_vert;
      scale_comm->predict[0][1][1] = vp9_convolve8_avg_vert;
      scale_comm->predict[1][0][0] = vp9_convolve8_horiz;
      scale_comm->predict[1][0][1] = vp9_convolve8_avg_horiz;
    } else {
      
      scale_comm->predict[0][0][0] = vp9_convolve8_vert;
      scale_comm->predict[0][0][1] = vp9_convolve8_avg_vert;
      scale_comm->predict[0][1][0] = vp9_convolve8_vert;
      scale_comm->predict[0][1][1] = vp9_convolve8_avg_vert;
      scale_comm->predict[1][0][0] = vp9_convolve8;
      scale_comm->predict[1][0][1] = vp9_convolve8_avg;
    }
  } else {
    if (scale_comm->y_step_q4 == 16) {
      
      scale_comm->predict[0][0][0] = vp9_convolve8_horiz;
      scale_comm->predict[0][0][1] = vp9_convolve8_avg_horiz;
      scale_comm->predict[0][1][0] = vp9_convolve8;
      scale_comm->predict[0][1][1] = vp9_convolve8_avg;
      scale_comm->predict[1][0][0] = vp9_convolve8_horiz;
      scale_comm->predict[1][0][1] = vp9_convolve8_avg_horiz;
    } else {
      
      scale_comm->predict[0][0][0] = vp9_convolve8;
      scale_comm->predict[0][0][1] = vp9_convolve8_avg;
      scale_comm->predict[0][1][0] = vp9_convolve8;
      scale_comm->predict[0][1][1] = vp9_convolve8_avg;
      scale_comm->predict[1][0][0] = vp9_convolve8;
      scale_comm->predict[1][0][1] = vp9_convolve8_avg;
    }
  }
  
  scale_comm->predict[1][1][0] = vp9_convolve8;
  scale_comm->predict[1][1][1] = vp9_convolve8_avg;

  scale->sfc = scale_comm;
  scale->x_offset_q4 = 0;  
  scale->y_offset_q4 = 0;  
}
