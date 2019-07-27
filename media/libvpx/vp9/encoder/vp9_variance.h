









#ifndef VP9_ENCODER_VP9_VARIANCE_H_
#define VP9_ENCODER_VP9_VARIANCE_H_

#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

void variance(const uint8_t *a, int a_stride,
              const uint8_t *b, int b_stride,
              int  w, int  h,
              unsigned int *sse, int *sum);

typedef unsigned int(*vp9_sad_fn_t)(const uint8_t *src_ptr,
                                    int source_stride,
                                    const uint8_t *ref_ptr,
                                    int ref_stride);

typedef unsigned int(*vp9_sad_avg_fn_t)(const uint8_t *src_ptr,
                                        int source_stride,
                                        const uint8_t *ref_ptr,
                                        int ref_stride,
                                        const uint8_t *second_pred);

typedef void (*vp9_sad_multi_fn_t)(const uint8_t *src_ptr,
                                   int source_stride,
                                   const uint8_t *ref_ptr,
                                   int  ref_stride,
                                   unsigned int *sad_array);

typedef void (*vp9_sad_multi_d_fn_t)(const uint8_t *src_ptr,
                                     int source_stride,
                                     const uint8_t* const ref_ptr[],
                                     int  ref_stride, unsigned int *sad_array);

typedef unsigned int (*vp9_variance_fn_t)(const uint8_t *src_ptr,
                                          int source_stride,
                                          const uint8_t *ref_ptr,
                                          int ref_stride,
                                          unsigned int *sse);

typedef unsigned int (*vp9_subpixvariance_fn_t)(const uint8_t *src_ptr,
                                                int source_stride,
                                                int xoffset,
                                                int yoffset,
                                                const uint8_t *ref_ptr,
                                                int Refstride,
                                                unsigned int *sse);

typedef unsigned int (*vp9_subp_avg_variance_fn_t)(const uint8_t *src_ptr,
                                                   int source_stride,
                                                   int xoffset,
                                                   int yoffset,
                                                   const uint8_t *ref_ptr,
                                                   int Refstride,
                                                   unsigned int *sse,
                                                   const uint8_t *second_pred);

typedef struct vp9_variance_vtable {
  vp9_sad_fn_t               sdf;
  vp9_sad_avg_fn_t           sdaf;
  vp9_variance_fn_t          vf;
  vp9_subpixvariance_fn_t    svf;
  vp9_subp_avg_variance_fn_t svaf;
  vp9_sad_multi_fn_t         sdx3f;
  vp9_sad_multi_fn_t         sdx8f;
  vp9_sad_multi_d_fn_t       sdx4df;
} vp9_variance_fn_ptr_t;

void vp9_comp_avg_pred(uint8_t *comp_pred, const uint8_t *pred, int width,
                       int height, const uint8_t *ref, int ref_stride);

#ifdef __cplusplus
}  
#endif

#endif
