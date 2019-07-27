









#ifndef VP9_ENCODER_VP9_RESIZE_H_
#define VP9_ENCODER_VP9_RESIZE_H_

#include <stdio.h>
#include "vpx/vpx_integer.h"

void vp9_resize_plane(const uint8_t *const input,
                      int height,
                      int width,
                      int in_stride,
                      uint8_t *output,
                      int height2,
                      int width2,
                      int out_stride);
void vp9_resize_frame420(const uint8_t *const y,
                         int y_stride,
                         const uint8_t *const u,
                         const uint8_t *const v,
                         int uv_stride,
                         int height,
                         int width,
                         uint8_t *oy,
                         int oy_stride,
                         uint8_t *ou,
                         uint8_t *ov,
                         int ouv_stride,
                         int oheight,
                         int owidth);
void vp9_resize_frame422(const uint8_t *const y,
                         int y_stride,
                         const uint8_t *const u,
                         const uint8_t *const v,
                         int uv_stride,
                         int height,
                         int width,
                         uint8_t *oy,
                         int oy_stride,
                         uint8_t *ou,
                         uint8_t *ov,
                         int ouv_stride,
                         int oheight,
                         int owidth);
void vp9_resize_frame444(const uint8_t *const y,
                         int y_stride,
                         const uint8_t *const u,
                         const uint8_t *const v,
                         int uv_stride,
                         int height,
                         int width,
                         uint8_t *oy,
                         int oy_stride,
                         uint8_t *ou,
                         uint8_t *ov,
                         int ouv_stride,
                         int oheight,
                         int owidth);

#endif    
