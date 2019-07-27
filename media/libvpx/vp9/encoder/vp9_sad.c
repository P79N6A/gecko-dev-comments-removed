









#include <stdlib.h>

#include "./vp9_rtcd.h"
#include "./vpx_config.h"

#include "vpx/vpx_integer.h"
#if CONFIG_VP9_HIGHBITDEPTH
#include "vp9/common/vp9_common.h"
#endif
#include "vp9/encoder/vp9_variance.h"

static INLINE unsigned int sad(const uint8_t *a, int a_stride,
                               const uint8_t *b, int b_stride,
                               int width, int height) {
  int y, x;
  unsigned int sad = 0;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

#define sadMxN(m, n) \
unsigned int vp9_sad##m##x##n##_c(const uint8_t *src, int src_stride, \
                                  const uint8_t *ref, int ref_stride) { \
  return sad(src, src_stride, ref, ref_stride, m, n); \
} \
unsigned int vp9_sad##m##x##n##_avg_c(const uint8_t *src, int src_stride, \
                                      const uint8_t *ref, int ref_stride, \
                                      const uint8_t *second_pred) { \
  uint8_t comp_pred[m * n]; \
  vp9_comp_avg_pred(comp_pred, second_pred, m, n, ref, ref_stride); \
  return sad(src, src_stride, comp_pred, m, m, n); \
}

#define sadMxNxK(m, n, k) \
void vp9_sad##m##x##n##x##k##_c(const uint8_t *src, int src_stride, \
                                const uint8_t *ref, int ref_stride, \
                                unsigned int *sads) { \
  int i; \
  for (i = 0; i < k; ++i) \
    sads[i] = vp9_sad##m##x##n##_c(src, src_stride, &ref[i], ref_stride); \
}

#define sadMxNx4D(m, n) \
void vp9_sad##m##x##n##x4d_c(const uint8_t *src, int src_stride, \
                             const uint8_t *const refs[], int ref_stride, \
                             unsigned int *sads) { \
  int i; \
  for (i = 0; i < 4; ++i) \
    sads[i] = vp9_sad##m##x##n##_c(src, src_stride, refs[i], ref_stride); \
}


sadMxN(64, 64)
sadMxNxK(64, 64, 3)
sadMxNxK(64, 64, 8)
sadMxNx4D(64, 64)


sadMxN(64, 32)
sadMxNx4D(64, 32)


sadMxN(32, 64)
sadMxNx4D(32, 64)


sadMxN(32, 32)
sadMxNxK(32, 32, 3)
sadMxNxK(32, 32, 8)
sadMxNx4D(32, 32)


sadMxN(32, 16)
sadMxNx4D(32, 16)


sadMxN(16, 32)
sadMxNx4D(16, 32)


sadMxN(16, 16)
sadMxNxK(16, 16, 3)
sadMxNxK(16, 16, 8)
sadMxNx4D(16, 16)


sadMxN(16, 8)
sadMxNxK(16, 8, 3)
sadMxNxK(16, 8, 8)
sadMxNx4D(16, 8)


sadMxN(8, 16)
sadMxNxK(8, 16, 3)
sadMxNxK(8, 16, 8)
sadMxNx4D(8, 16)


sadMxN(8, 8)
sadMxNxK(8, 8, 3)
sadMxNxK(8, 8, 8)
sadMxNx4D(8, 8)


sadMxN(8, 4)
sadMxNxK(8, 4, 8)
sadMxNx4D(8, 4)


sadMxN(4, 8)
sadMxNxK(4, 8, 8)
sadMxNx4D(4, 8)


sadMxN(4, 4)
sadMxNxK(4, 4, 3)
sadMxNxK(4, 4, 8)
sadMxNx4D(4, 4)

#if CONFIG_VP9_HIGHBITDEPTH
static INLINE unsigned int highbd_sad(const uint8_t *a8, int a_stride,
                                      const uint8_t *b8, int b_stride,
                                      int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *a = CONVERT_TO_SHORTPTR(a8);
  const uint16_t *b = CONVERT_TO_SHORTPTR(b8);
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

static INLINE unsigned int highbd_sadb(const uint8_t *a8, int a_stride,
                                       const uint16_t *b, int b_stride,
                                       int width, int height) {
  int y, x;
  unsigned int sad = 0;
  const uint16_t *a = CONVERT_TO_SHORTPTR(a8);
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++)
      sad += abs(a[x] - b[x]);

    a += a_stride;
    b += b_stride;
  }
  return sad;
}

#define highbd_sadMxN(m, n) \
unsigned int vp9_highbd_sad##m##x##n##_c(const uint8_t *src, int src_stride, \
                                         const uint8_t *ref, int ref_stride) { \
  return highbd_sad(src, src_stride, ref, ref_stride, m, n); \
} \
unsigned int vp9_highbd_sad##m##x##n##_avg_c(const uint8_t *src, \
                                             int src_stride, \
                                             const uint8_t *ref, \
                                             int ref_stride, \
                                             const uint8_t *second_pred) { \
  uint16_t comp_pred[m * n]; \
  vp9_highbd_comp_avg_pred(comp_pred, second_pred, m, n, ref, ref_stride); \
  return highbd_sadb(src, src_stride, comp_pred, m, m, n); \
}

#define highbd_sadMxNxK(m, n, k) \
void vp9_highbd_sad##m##x##n##x##k##_c(const uint8_t *src, int src_stride, \
                                       const uint8_t *ref, int ref_stride, \
                                       unsigned int *sads) { \
  int i; \
  for (i = 0; i < k; ++i) { \
    sads[i] = vp9_highbd_sad##m##x##n##_c(src, src_stride, &ref[i], \
                                          ref_stride); \
  } \
}

#define highbd_sadMxNx4D(m, n) \
void vp9_highbd_sad##m##x##n##x4d_c(const uint8_t *src, int src_stride, \
                                    const uint8_t *const refs[], \
                                    int ref_stride, unsigned int *sads) { \
  int i; \
  for (i = 0; i < 4; ++i) { \
    sads[i] = vp9_highbd_sad##m##x##n##_c(src, src_stride, refs[i], \
                                          ref_stride); \
  }  \
}


highbd_sadMxN(64, 64)
highbd_sadMxNxK(64, 64, 3)
highbd_sadMxNxK(64, 64, 8)
highbd_sadMxNx4D(64, 64)


highbd_sadMxN(64, 32)
highbd_sadMxNx4D(64, 32)


highbd_sadMxN(32, 64)
highbd_sadMxNx4D(32, 64)


highbd_sadMxN(32, 32)
highbd_sadMxNxK(32, 32, 3)
highbd_sadMxNxK(32, 32, 8)
highbd_sadMxNx4D(32, 32)


highbd_sadMxN(32, 16)
highbd_sadMxNx4D(32, 16)


highbd_sadMxN(16, 32)
highbd_sadMxNx4D(16, 32)


highbd_sadMxN(16, 16)
highbd_sadMxNxK(16, 16, 3)
highbd_sadMxNxK(16, 16, 8)
highbd_sadMxNx4D(16, 16)


highbd_sadMxN(16, 8)
highbd_sadMxNxK(16, 8, 3)
highbd_sadMxNxK(16, 8, 8)
highbd_sadMxNx4D(16, 8)


highbd_sadMxN(8, 16)
highbd_sadMxNxK(8, 16, 3)
highbd_sadMxNxK(8, 16, 8)
highbd_sadMxNx4D(8, 16)


highbd_sadMxN(8, 8)
highbd_sadMxNxK(8, 8, 3)
highbd_sadMxNxK(8, 8, 8)
highbd_sadMxNx4D(8, 8)


highbd_sadMxN(8, 4)
highbd_sadMxNxK(8, 4, 8)
highbd_sadMxNx4D(8, 4)


highbd_sadMxN(4, 8)
highbd_sadMxNxK(4, 8, 8)
highbd_sadMxNx4D(4, 8)


highbd_sadMxN(4, 4)
highbd_sadMxNxK(4, 4, 3)
highbd_sadMxNxK(4, 4, 8)
highbd_sadMxNx4D(4, 4)

#endif  
