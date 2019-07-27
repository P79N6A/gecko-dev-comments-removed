









#ifndef VP9_COMMON_VP9_FILTER_H_
#define VP9_COMMON_VP9_FILTER_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "vpx_ports/mem.h"


#ifdef __cplusplus
extern "C" {
#endif

#define FILTER_BITS 7

#define SUBPEL_BITS 4
#define SUBPEL_MASK ((1 << SUBPEL_BITS) - 1)
#define SUBPEL_SHIFTS (1 << SUBPEL_BITS)
#define SUBPEL_TAPS 8

typedef enum {
  EIGHTTAP = 0,
  EIGHTTAP_SMOOTH = 1,
  EIGHTTAP_SHARP = 2,
  SWITCHABLE_FILTERS = 3, 
  BILINEAR = 3,
  
  
  SWITCHABLE_FILTER_CONTEXTS = SWITCHABLE_FILTERS + 1,
  SWITCHABLE = 4  
} INTERP_FILTER;

typedef int16_t InterpKernel[SUBPEL_TAPS];

const InterpKernel *vp9_get_interp_kernel(INTERP_FILTER filter);

DECLARE_ALIGNED(256, extern const InterpKernel,
                vp9_bilinear_filters[SUBPEL_SHIFTS]);



#define BILINEAR_FILTERS_2TAP(x) \
  (vp9_bilinear_filters[(x)] + SUBPEL_TAPS/2 - 1)

#ifdef __cplusplus
}  
#endif

#endif
