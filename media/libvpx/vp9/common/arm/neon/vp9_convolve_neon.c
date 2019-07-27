









#include "./vp9_rtcd.h"
#include "vp9/common/vp9_common.h"
#include "vpx_ports/mem.h"

void vp9_convolve8_neon(const uint8_t *src, ptrdiff_t src_stride,
                        uint8_t *dst, ptrdiff_t dst_stride,
                        const int16_t *filter_x, int x_step_q4,
                        const int16_t *filter_y, int y_step_q4,
                        int w, int h) {
  


  DECLARE_ALIGNED_ARRAY(8, uint8_t, temp, 64 * 72);

  
  int intermediate_height = h + 7;

  if (x_step_q4 != 16 || y_step_q4 != 16)
    return vp9_convolve8_c(src, src_stride,
                           dst, dst_stride,
                           filter_x, x_step_q4,
                           filter_y, y_step_q4,
                           w, h);

  




  vp9_convolve8_horiz_neon(src - src_stride * 3, src_stride,
                           temp, 64,
                           filter_x, x_step_q4, filter_y, y_step_q4,
                           w, intermediate_height);

  
  vp9_convolve8_vert_neon(temp + 64 * 3, 64,
                          dst, dst_stride,
                          filter_x, x_step_q4, filter_y, y_step_q4,
                          w, h);
}

void vp9_convolve8_avg_neon(const uint8_t *src, ptrdiff_t src_stride,
                            uint8_t *dst, ptrdiff_t dst_stride,
                            const int16_t *filter_x, int x_step_q4,
                            const int16_t *filter_y, int y_step_q4,
                            int w, int h) {
  DECLARE_ALIGNED_ARRAY(8, uint8_t, temp, 64 * 72);
  int intermediate_height = h + 7;

  if (x_step_q4 != 16 || y_step_q4 != 16)
    return vp9_convolve8_avg_c(src, src_stride,
                               dst, dst_stride,
                               filter_x, x_step_q4,
                               filter_y, y_step_q4,
                               w, h);

  


  vp9_convolve8_horiz_neon(src - src_stride * 3, src_stride,
                           temp, 64,
                           filter_x, x_step_q4, filter_y, y_step_q4,
                           w, intermediate_height);
  vp9_convolve8_avg_vert_neon(temp + 64 * 3,
                              64, dst, dst_stride,
                              filter_x, x_step_q4, filter_y, y_step_q4,
                              w, h);
}
