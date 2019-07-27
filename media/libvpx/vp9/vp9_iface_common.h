








#ifndef VP9_VP9_IFACE_COMMON_H_
#define VP9_VP9_IFACE_COMMON_H_

static void yuvconfig2image(vpx_image_t *img, const YV12_BUFFER_CONFIG  *yv12,
                            void *user_priv) {
  



  const int ss_x = yv12->uv_crop_width < yv12->y_crop_width;
  const int ss_y = yv12->uv_crop_height < yv12->y_crop_height;
  int bps;
  if (!ss_y) {
    if (!ss_x) {
      img->fmt = VPX_IMG_FMT_I444;
      bps = 24;
    } else {
      img->fmt = VPX_IMG_FMT_I422;
      bps = 16;
    }
  } else {
    img->fmt = VPX_IMG_FMT_I420;
    bps = 12;
  }
  img->bit_depth = 8;
  img->w = yv12->y_stride;
  img->h = ALIGN_POWER_OF_TWO(yv12->y_height + 2 * VP9_ENC_BORDER_IN_PIXELS, 3);
  img->d_w = yv12->y_crop_width;
  img->d_h = yv12->y_crop_height;
  img->x_chroma_shift = ss_x;
  img->y_chroma_shift = ss_y;
  img->planes[VPX_PLANE_Y] = yv12->y_buffer;
  img->planes[VPX_PLANE_U] = yv12->u_buffer;
  img->planes[VPX_PLANE_V] = yv12->v_buffer;
  img->planes[VPX_PLANE_ALPHA] = NULL;
  img->stride[VPX_PLANE_Y] = yv12->y_stride;
  img->stride[VPX_PLANE_U] = yv12->uv_stride;
  img->stride[VPX_PLANE_V] = yv12->uv_stride;
  img->stride[VPX_PLANE_ALPHA] = yv12->y_stride;
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

  yv12->y_stride = img->stride[VPX_PLANE_Y];
  yv12->uv_stride = img->stride[VPX_PLANE_U];

  yv12->border  = (img->stride[VPX_PLANE_Y] - img->w) / 2;
  return VPX_CODEC_OK;
}

#endif  
