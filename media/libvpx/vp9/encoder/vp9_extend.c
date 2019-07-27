









#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/mem.h"

#include "vp9/common/vp9_common.h"
#include "vp9/encoder/vp9_extend.h"

static void copy_and_extend_plane(const uint8_t *src, int src_pitch,
                                  uint8_t *dst, int dst_pitch,
                                  int w, int h,
                                  int extend_top, int extend_left,
                                  int extend_bottom, int extend_right) {
  int i, linesize;

  
  const uint8_t *src_ptr1 = src;
  const uint8_t *src_ptr2 = src + w - 1;
  uint8_t *dst_ptr1 = dst - extend_left;
  uint8_t *dst_ptr2 = dst + w;

  for (i = 0; i < h; i++) {
    memset(dst_ptr1, src_ptr1[0], extend_left);
    memcpy(dst_ptr1 + extend_left, src_ptr1, w);
    memset(dst_ptr2, src_ptr2[0], extend_right);
    src_ptr1 += src_pitch;
    src_ptr2 += src_pitch;
    dst_ptr1 += dst_pitch;
    dst_ptr2 += dst_pitch;
  }

  
  
  src_ptr1 = dst - extend_left;
  src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
  dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
  dst_ptr2 = dst + dst_pitch * (h) - extend_left;
  linesize = extend_left + extend_right + w;

  for (i = 0; i < extend_top; i++) {
    memcpy(dst_ptr1, src_ptr1, linesize);
    dst_ptr1 += dst_pitch;
  }

  for (i = 0; i < extend_bottom; i++) {
    memcpy(dst_ptr2, src_ptr2, linesize);
    dst_ptr2 += dst_pitch;
  }
}

#if CONFIG_VP9_HIGHBITDEPTH
static void highbd_copy_and_extend_plane(const uint8_t *src8, int src_pitch,
                                         uint8_t *dst8, int dst_pitch,
                                         int w, int h,
                                         int extend_top, int extend_left,
                                         int extend_bottom, int extend_right) {
  int i, linesize;
  uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  uint16_t *dst = CONVERT_TO_SHORTPTR(dst8);

  
  const uint16_t *src_ptr1 = src;
  const uint16_t *src_ptr2 = src + w - 1;
  uint16_t *dst_ptr1 = dst - extend_left;
  uint16_t *dst_ptr2 = dst + w;

  for (i = 0; i < h; i++) {
    vpx_memset16(dst_ptr1, src_ptr1[0], extend_left);
    memcpy(dst_ptr1 + extend_left, src_ptr1, w * sizeof(uint16_t));
    vpx_memset16(dst_ptr2, src_ptr2[0], extend_right);
    src_ptr1 += src_pitch;
    src_ptr2 += src_pitch;
    dst_ptr1 += dst_pitch;
    dst_ptr2 += dst_pitch;
  }

  
  
  src_ptr1 = dst - extend_left;
  src_ptr2 = dst + dst_pitch * (h - 1) - extend_left;
  dst_ptr1 = dst + dst_pitch * (-extend_top) - extend_left;
  dst_ptr2 = dst + dst_pitch * (h) - extend_left;
  linesize = extend_left + extend_right + w;

  for (i = 0; i < extend_top; i++) {
    memcpy(dst_ptr1, src_ptr1, linesize * sizeof(uint16_t));
    dst_ptr1 += dst_pitch;
  }

  for (i = 0; i < extend_bottom; i++) {
    memcpy(dst_ptr2, src_ptr2, linesize * sizeof(uint16_t));
    dst_ptr2 += dst_pitch;
  }
}
#endif  

void vp9_copy_and_extend_frame(const YV12_BUFFER_CONFIG *src,
                               YV12_BUFFER_CONFIG *dst) {
  
  
  const int et_y = 16;
  const int el_y = 16;
  
  
  
  const int er_y = MAX(src->y_width + 16, ALIGN_POWER_OF_TWO(src->y_width, 6))
      - src->y_crop_width;
  const int eb_y = MAX(src->y_height + 16, ALIGN_POWER_OF_TWO(src->y_height, 6))
      - src->y_crop_height;
  const int uv_width_subsampling = (src->uv_width != src->y_width);
  const int uv_height_subsampling = (src->uv_height != src->y_height);
  const int et_uv = et_y >> uv_height_subsampling;
  const int el_uv = el_y >> uv_width_subsampling;
  const int eb_uv = eb_y >> uv_height_subsampling;
  const int er_uv = er_y >> uv_width_subsampling;

#if CONFIG_VP9_HIGHBITDEPTH
  if (src->flags & YV12_FLAG_HIGHBITDEPTH) {
    highbd_copy_and_extend_plane(src->y_buffer, src->y_stride,
                                 dst->y_buffer, dst->y_stride,
                                 src->y_crop_width, src->y_crop_height,
                                 et_y, el_y, eb_y, er_y);

    highbd_copy_and_extend_plane(src->u_buffer, src->uv_stride,
                                 dst->u_buffer, dst->uv_stride,
                                 src->uv_crop_width, src->uv_crop_height,
                                 et_uv, el_uv, eb_uv, er_uv);

    highbd_copy_and_extend_plane(src->v_buffer, src->uv_stride,
                                 dst->v_buffer, dst->uv_stride,
                                 src->uv_crop_width, src->uv_crop_height,
                                 et_uv, el_uv, eb_uv, er_uv);
    return;
  }
#endif  

  copy_and_extend_plane(src->y_buffer, src->y_stride,
                        dst->y_buffer, dst->y_stride,
                        src->y_crop_width, src->y_crop_height,
                        et_y, el_y, eb_y, er_y);

  copy_and_extend_plane(src->u_buffer, src->uv_stride,
                        dst->u_buffer, dst->uv_stride,
                        src->uv_crop_width, src->uv_crop_height,
                        et_uv, el_uv, eb_uv, er_uv);

  copy_and_extend_plane(src->v_buffer, src->uv_stride,
                        dst->v_buffer, dst->uv_stride,
                        src->uv_crop_width, src->uv_crop_height,
                        et_uv, el_uv, eb_uv, er_uv);
}

void vp9_copy_and_extend_frame_with_rect(const YV12_BUFFER_CONFIG *src,
                                         YV12_BUFFER_CONFIG *dst,
                                         int srcy, int srcx,
                                         int srch, int srcw) {
  
  const int et_y = srcy ? 0 : dst->border;
  const int el_y = srcx ? 0 : dst->border;
  const int eb_y = srcy + srch != src->y_height ? 0 :
                      dst->border + dst->y_height - src->y_height;
  const int er_y = srcx + srcw != src->y_width ? 0 :
                      dst->border + dst->y_width - src->y_width;
  const int src_y_offset = srcy * src->y_stride + srcx;
  const int dst_y_offset = srcy * dst->y_stride + srcx;

  const int et_uv = ROUND_POWER_OF_TWO(et_y, 1);
  const int el_uv = ROUND_POWER_OF_TWO(el_y, 1);
  const int eb_uv = ROUND_POWER_OF_TWO(eb_y, 1);
  const int er_uv = ROUND_POWER_OF_TWO(er_y, 1);
  const int src_uv_offset = ((srcy * src->uv_stride) >> 1) + (srcx >> 1);
  const int dst_uv_offset = ((srcy * dst->uv_stride) >> 1) + (srcx >> 1);
  const int srch_uv = ROUND_POWER_OF_TWO(srch, 1);
  const int srcw_uv = ROUND_POWER_OF_TWO(srcw, 1);

  copy_and_extend_plane(src->y_buffer + src_y_offset, src->y_stride,
                        dst->y_buffer + dst_y_offset, dst->y_stride,
                        srcw, srch,
                        et_y, el_y, eb_y, er_y);

  copy_and_extend_plane(src->u_buffer + src_uv_offset, src->uv_stride,
                        dst->u_buffer + dst_uv_offset, dst->uv_stride,
                        srcw_uv, srch_uv,
                        et_uv, el_uv, eb_uv, er_uv);

  copy_and_extend_plane(src->v_buffer + src_uv_offset, src->uv_stride,
                        dst->v_buffer + dst_uv_offset, dst->uv_stride,
                        srcw_uv, srch_uv,
                        et_uv, el_uv, eb_uv, er_uv);
}
