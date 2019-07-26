









#ifndef VP9_COMMON_VP9_FILTER_H_
#define VP9_COMMON_VP9_FILTER_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"

#define FILTER_BITS 7

#define SUBPEL_BITS 4
#define SUBPEL_MASK ((1 << SUBPEL_BITS) - 1)
#define SUBPEL_SHIFTS (1 << SUBPEL_BITS)
#define SUBPEL_TAPS 8

typedef enum {
  EIGHTTAP = 0,
  EIGHTTAP_SMOOTH = 1,
  EIGHTTAP_SHARP = 2,
  BILINEAR = 3,
  SWITCHABLE = 4  
} INTERPOLATION_TYPE;

typedef int16_t subpel_kernel[SUBPEL_TAPS];

struct subpix_fn_table {
  const subpel_kernel *filter_x;
  const subpel_kernel *filter_y;
};

const subpel_kernel *vp9_get_filter_kernel(INTERPOLATION_TYPE type);

extern const subpel_kernel vp9_bilinear_filters[SUBPEL_SHIFTS];
extern const subpel_kernel vp9_sub_pel_filters_8[SUBPEL_SHIFTS];
extern const subpel_kernel vp9_sub_pel_filters_8s[SUBPEL_SHIFTS];
extern const subpel_kernel vp9_sub_pel_filters_8lp[SUBPEL_SHIFTS];



#define BILINEAR_FILTERS_2TAP(x) \
  (vp9_bilinear_filters[(x)] + SUBPEL_TAPS/2 - 1)

#endif  
