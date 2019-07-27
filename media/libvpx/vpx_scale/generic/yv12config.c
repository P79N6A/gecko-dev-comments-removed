









#include <assert.h>

#include "vpx_scale/yv12config.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx_ports/mem.h"








#define yv12_align_addr(addr, align) \
    (void*)(((size_t)(addr) + ((align) - 1)) & (size_t)-(align))

int
vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf) {
  if (ybf) {
    
    
    if (ybf->buffer_alloc_sz > 0) {
      vpx_free(ybf->buffer_alloc);
    }

    


    memset(ybf, 0, sizeof(YV12_BUFFER_CONFIG));
  } else {
    return -1;
  }

  return 0;
}

int vp8_yv12_realloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                                  int width, int height, int border) {
  if (ybf) {
    int aligned_width = (width + 15) & ~15;
    int aligned_height = (height + 15) & ~15;
    int y_stride = ((aligned_width + 2 * border) + 31) & ~31;
    int yplane_size = (aligned_height + 2 * border) * y_stride;
    int uv_width = aligned_width >> 1;
    int uv_height = aligned_height >> 1;
    

    int uv_stride = y_stride >> 1;
    int uvplane_size = (uv_height + border) * uv_stride;
    const int frame_size = yplane_size + 2 * uvplane_size;

    if (!ybf->buffer_alloc) {
      ybf->buffer_alloc = (uint8_t *)vpx_memalign(32, frame_size);
      ybf->buffer_alloc_sz = frame_size;
    }

    if (!ybf->buffer_alloc || ybf->buffer_alloc_sz < frame_size)
      return -1;

    




    if (border & 0x1f)
      return -3;

    ybf->y_crop_width = width;
    ybf->y_crop_height = height;
    ybf->y_width  = aligned_width;
    ybf->y_height = aligned_height;
    ybf->y_stride = y_stride;

    ybf->uv_crop_width = (width + 1) / 2;
    ybf->uv_crop_height = (height + 1) / 2;
    ybf->uv_width = uv_width;
    ybf->uv_height = uv_height;
    ybf->uv_stride = uv_stride;

    ybf->alpha_width = 0;
    ybf->alpha_height = 0;
    ybf->alpha_stride = 0;

    ybf->border = border;
    ybf->frame_size = frame_size;

    ybf->y_buffer = ybf->buffer_alloc + (border * y_stride) + border;
    ybf->u_buffer = ybf->buffer_alloc + yplane_size + (border / 2  * uv_stride) + border / 2;
    ybf->v_buffer = ybf->buffer_alloc + yplane_size + uvplane_size + (border / 2  * uv_stride) + border / 2;
    ybf->alpha_buffer = NULL;

    ybf->corrupted = 0; 
    return 0;
  }
  return -2;
}

int vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                                int width, int height, int border) {
  if (ybf) {
    vp8_yv12_de_alloc_frame_buffer(ybf);
    return vp8_yv12_realloc_frame_buffer(ybf, width, height, border);
  }
  return -2;
}

#if CONFIG_VP9


int vp9_free_frame_buffer(YV12_BUFFER_CONFIG *ybf) {
  if (ybf) {
    if (ybf->buffer_alloc_sz > 0) {
      vpx_free(ybf->buffer_alloc);
    }

    


    memset(ybf, 0, sizeof(YV12_BUFFER_CONFIG));
  } else {
    return -1;
  }

  return 0;
}

int vp9_realloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                             int width, int height,
                             int ss_x, int ss_y,
#if CONFIG_VP9_HIGHBITDEPTH
                             int use_highbitdepth,
#endif
                             int border,
                             int byte_alignment,
                             vpx_codec_frame_buffer_t *fb,
                             vpx_get_frame_buffer_cb_fn_t cb,
                             void *cb_priv) {
  if (ybf) {
    const int vp9_byte_align = (byte_alignment == 0) ? 1 : byte_alignment;
    const int aligned_width = (width + 7) & ~7;
    const int aligned_height = (height + 7) & ~7;
    const int y_stride = ((aligned_width + 2 * border) + 31) & ~31;
    const uint64_t yplane_size = (aligned_height + 2 * border) *
                                 (uint64_t)y_stride + byte_alignment;
    const int uv_width = aligned_width >> ss_x;
    const int uv_height = aligned_height >> ss_y;
    const int uv_stride = y_stride >> ss_x;
    const int uv_border_w = border >> ss_x;
    const int uv_border_h = border >> ss_y;
    const uint64_t uvplane_size = (uv_height + 2 * uv_border_h) *
                                  (uint64_t)uv_stride + byte_alignment;

#if CONFIG_ALPHA
    const int alpha_width = aligned_width;
    const int alpha_height = aligned_height;
    const int alpha_stride = y_stride;
    const int alpha_border_w = border;
    const int alpha_border_h = border;
    const uint64_t alpha_plane_size = (alpha_height + 2 * alpha_border_h) *
                                      (uint64_t)alpha_stride + byte_alignment;
#if CONFIG_VP9_HIGHBITDEPTH
    const uint64_t frame_size = (1 + use_highbitdepth) *
        (yplane_size + 2 * uvplane_size + alpha_plane_size);
#else
    const uint64_t frame_size = yplane_size + 2 * uvplane_size +
                                alpha_plane_size;
#endif  
#else
#if CONFIG_VP9_HIGHBITDEPTH
    const uint64_t frame_size =
        (1 + use_highbitdepth) * (yplane_size + 2 * uvplane_size);
#else
    const uint64_t frame_size = yplane_size + 2 * uvplane_size;
#endif  
#endif  

    uint8_t *buf = NULL;

    if (cb != NULL) {
      const int align_addr_extra_size = 31;
      const uint64_t external_frame_size = frame_size + align_addr_extra_size;

      assert(fb != NULL);

      if (external_frame_size != (size_t)external_frame_size)
        return -1;

      
      if (cb(cb_priv, (size_t)external_frame_size, fb) < 0)
        return -1;

      if (fb->data == NULL || fb->size < external_frame_size)
        return -1;

      ybf->buffer_alloc = (uint8_t *)yv12_align_addr(fb->data, 32);
    } else if (frame_size > (size_t)ybf->buffer_alloc_sz) {
      
      vpx_free(ybf->buffer_alloc);
      ybf->buffer_alloc = NULL;

      if (frame_size != (size_t)frame_size)
        return -1;

      ybf->buffer_alloc = (uint8_t *)vpx_memalign(32, (size_t)frame_size);
      if (!ybf->buffer_alloc)
        return -1;

      ybf->buffer_alloc_sz = (int)frame_size;

      
      
      
      memset(ybf->buffer_alloc, 0, ybf->buffer_alloc_sz);
    }

    




    if (border & 0x1f)
      return -3;

    ybf->y_crop_width = width;
    ybf->y_crop_height = height;
    ybf->y_width  = aligned_width;
    ybf->y_height = aligned_height;
    ybf->y_stride = y_stride;

    ybf->uv_crop_width = (width + ss_x) >> ss_x;
    ybf->uv_crop_height = (height + ss_y) >> ss_y;
    ybf->uv_width = uv_width;
    ybf->uv_height = uv_height;
    ybf->uv_stride = uv_stride;

    ybf->border = border;
    ybf->frame_size = (int)frame_size;
    ybf->subsampling_x = ss_x;
    ybf->subsampling_y = ss_y;

    buf = ybf->buffer_alloc;
#if CONFIG_VP9_HIGHBITDEPTH
    if (use_highbitdepth) {
      
      buf = CONVERT_TO_BYTEPTR(ybf->buffer_alloc);
      ybf->flags = YV12_FLAG_HIGHBITDEPTH;
    } else {
      ybf->flags = 0;
    }
#endif  

    ybf->y_buffer = (uint8_t *)yv12_align_addr(
        buf + (border * y_stride) + border, vp9_byte_align);
    ybf->u_buffer = (uint8_t *)yv12_align_addr(
        buf + yplane_size + (uv_border_h * uv_stride) + uv_border_w,
        vp9_byte_align);
    ybf->v_buffer = (uint8_t *)yv12_align_addr(
        buf + yplane_size + uvplane_size + (uv_border_h * uv_stride) +
        uv_border_w, vp9_byte_align);

#if CONFIG_ALPHA
    ybf->alpha_width = alpha_width;
    ybf->alpha_height = alpha_height;
    ybf->alpha_stride = alpha_stride;
    ybf->alpha_buffer = (uint8_t *)yv12_align_addr(
        buf + yplane_size + 2 * uvplane_size +
        (alpha_border_h * alpha_stride) + alpha_border_w, vp9_byte_align);
#endif
    ybf->corrupted = 0; 
    return 0;
  }
  return -2;
}

int vp9_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                           int width, int height,
                           int ss_x, int ss_y,
#if CONFIG_VP9_HIGHBITDEPTH
                           int use_highbitdepth,
#endif
                           int border,
                           int byte_alignment) {
  if (ybf) {
    vp9_free_frame_buffer(ybf);
    return vp9_realloc_frame_buffer(ybf, width, height, ss_x, ss_y,
#if CONFIG_VP9_HIGHBITDEPTH
                                    use_highbitdepth,
#endif
                                    border, byte_alignment, NULL, NULL, NULL);
  }
  return -2;
}
#endif
