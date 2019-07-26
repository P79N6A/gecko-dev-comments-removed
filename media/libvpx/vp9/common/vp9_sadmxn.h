









#ifndef VP9_COMMON_VP9_SADMXN_H_
#define VP9_COMMON_VP9_SADMXN_H_

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"

static INLINE unsigned int sad_mx_n_c(const uint8_t *src_ptr,
                                      int src_stride,
                                      const uint8_t *ref_ptr,
                                      int ref_stride,
                                      int m,
                                      int n) {
  int r, c;
  unsigned int sad = 0;

  for (r = 0; r < n; r++) {
    for (c = 0; c < m; c++) {
      sad += abs(src_ptr[c] - ref_ptr[c]);
    }

    src_ptr += src_stride;
    ref_ptr += ref_stride;
  }

  return sad;
}

#endif  
