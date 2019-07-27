








#ifndef VP9_COMMON_VP9_CONVOLVE_H_
#define VP9_COMMON_VP9_CONVOLVE_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*convolve_fn_t)(const uint8_t *src, ptrdiff_t src_stride,
                              uint8_t *dst, ptrdiff_t dst_stride,
                              const int16_t *filter_x, int x_step_q4,
                              const int16_t *filter_y, int y_step_q4,
                              int w, int h);

#ifdef __cplusplus
}  
#endif

#endif
