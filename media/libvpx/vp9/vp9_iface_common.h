








#ifndef VP9_VP9_IFACE_COMMON_H_
#define VP9_VP9_IFACE_COMMON_H_

#include "vpx_ports/mem.h"

static void yuvconfig2image(vpx_image_t *img, const YV12_BUFFER_CONFIG  *yv12,
                            void *user_priv) {
  



  int bps;
  if (!yv12->subsampling_y) {
    if (!yv12->subsampling_x) {
      img->fmt = VPX_IMG_FMT_I444;
      bps = 24;
    } else {
      img->fmt = VPX_IMG_FMT_I422;
      bps = 16;
    }
  } else {
    if (!yv12->subsampling_x) {
      img->fmt = VPX_IMG_FMT_I440;
      bps = 16;
    } else {
      img->fmt = VPX_IMG_FMT_I420;
      bps = 12;
    }
  }
  img->cs = yv12->color_space;
  img->bit_depth = 8;
  img->w = yv12->y_stride;
  img->h = ALIGN_POWER_OF_TWO(yv12->y_height + 2 * VP9_ENC_BORDER_IN_PIXELS, 3);
  img->d_w = yv12->y_crop_width;
  img->d_h = yv12->y_crop_height;
  img->x_chroma_shift = yv12->subsampling_x;
  img->y_chroma_shift = yv12->subsampling_y;
  img->planes[VPX_PLANE_Y] = yv12->y_buffer;
  img->planes[VPX_PLANE_U] = yv12->u_buffer;
  img->planes[VPX_PLANE_V] = yv12->v_buffer;
  img->planes[VPX_PLANE_ALPHA] = NULL;
  img->stride[VPX_PLANE_Y] = yv12->y_stride;
  img->stride[VPX_PLANE_U] = yv12->uv_stride;
  img->stride[VPX_PLANE_V] = yv12->uv_stride;
  img->stride[VPX_PLANE_ALPHA] = yv12->y_stride;
#if CONFIG_VP9_HIGHBITDEPTH
  if (yv12->flags & YV12_FLAG_HIGHBITDEPTH) {
    
    
    img->fmt |= VPX_IMG_FMT_HIGHBITDEPTH;
    img->bit_depth = yv12->bit_depth;
    img->planes[VPX_PLANE_Y] = (uint8_t*)CONVERT_TO_SHORTPTR(yv12->y_buffer);
    img->planes[VPX_PLANE_U] = (uint8_t*)CONVERT_TO_SHORTPTR(yv12->u_buffer);
    img->planes[VPX_PLANE_V] = (uint8_t*)CONVERT_TO_SHORTPTR(yv12->v_buffer);
    img->planes[VPX_PLANE_ALPHA] = NULL;
    img->stride[VPX_PLANE_Y] = 2 * yv12->y_stride;
    img->stride[VPX_PLANE_U] = 2 * yv12->uv_stride;
    img->stride[VPX_PLANE_V] = 2 * yv12->uv_stride;
    img->stride[VPX_PLANE_ALPHA] = 2 * yv12->y_stride;
  }
#endif  
  img->bps = bps;
  img->user_priv = user_priv;
  img->img_data = yv12->buffer_alloc;
  img->img_data_owner = 0;
  img->self_allocd = 0;
}

static vpx_codec_err_t image2yuvconfig(const vpx_image_t *img,
                                       YV12_BUFFER_CONFIG *yv12) {
  yv12->y_buffer = img->planes[VPX_PLANE_Y];
  yv12->u_buffer = img->planes[VPX_PLANE_U];
  yv12->v_buffer = img->planes[VPX_PLANE_V];

  yv12->y_crop_width  = img->d_w;
  yv12->y_crop_height = img->d_h;
  yv12->y_width  = img->d_w;
  yv12->y_height = img->d_h;

  yv12->uv_width = img->x_chroma_shift == 1 ? (1 + yv12->y_width) / 2
                                            : yv12->y_width;
  yv12->uv_height = img->y_chroma_shift == 1 ? (1 + yv12->y_height) / 2
                                             : yv12->y_height;
  yv12->uv_crop_width = yv12->uv_width;
  yv12->uv_crop_height = yv12->uv_height;

  yv12->y_stride = img->stride[VPX_PLANE_Y];
  yv12->uv_stride = img->stride[VPX_PLANE_U];
  yv12->color_space = img->cs;

#if CONFIG_VP9_HIGHBITDEPTH
  if (img->fmt & VPX_IMG_FMT_HIGHBITDEPTH) {
    
    
    
    
    
    
    
    
    
    
    yv12->y_buffer = CONVERT_TO_BYTEPTR(yv12->y_buffer);
    yv12->u_buffer = CONVERT_TO_BYTEPTR(yv12->u_buffer);
    yv12->v_buffer = CONVERT_TO_BYTEPTR(yv12->v_buffer);
    yv12->y_stride >>= 1;
    yv12->uv_stride >>= 1;
    yv12->flags = YV12_FLAG_HIGHBITDEPTH;
  } else {
    yv12->flags = 0;
  }
  yv12->border  = (yv12->y_stride - img->w) / 2;
#else
  yv12->border  = (img->stride[VPX_PLANE_Y] - img->w) / 2;
#endif  
  yv12->subsampling_x = img->x_chroma_shift;
  yv12->subsampling_y = img->y_chroma_shift;
  return VPX_CODEC_OK;
}

#endif  
