









#ifndef VPX_SCALE_YV12CONFIG_H_
#define VPX_SCALE_YV12CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "vpx/vpx_frame_buffer.h"
#include "vpx/vpx_integer.h"

#define VP8BORDERINPIXELS           32
#define VP9INNERBORDERINPIXELS      96
#define VP9_INTERP_EXTEND           4
#define VP9_ENC_BORDER_IN_PIXELS    160
#define VP9_DEC_BORDER_IN_PIXELS    32

  typedef struct yv12_buffer_config {
    int   y_width;
    int   y_height;
    int   y_crop_width;
    int   y_crop_height;
    int   y_stride;
    

    int   uv_width;
    int   uv_height;
    int   uv_crop_width;
    int   uv_crop_height;
    int   uv_stride;
    

    int   alpha_width;
    int   alpha_height;
    int   alpha_stride;

    uint8_t *y_buffer;
    uint8_t *u_buffer;
    uint8_t *v_buffer;
    uint8_t *alpha_buffer;

    uint8_t *buffer_alloc;
    int buffer_alloc_sz;
    int border;
    int frame_size;

    int corrupted;
    int flags;
  } YV12_BUFFER_CONFIG;

  int vp8_yv12_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                                  int width, int height, int border);
  int vp8_yv12_realloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                                    int width, int height, int border);
  int vp8_yv12_de_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf);

  int vp9_alloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                             int width, int height, int ss_x, int ss_y,
                             int border);

  
  
  
  
  
  
  int vp9_realloc_frame_buffer(YV12_BUFFER_CONFIG *ybf,
                               int width, int height, int ss_x, int ss_y,
                               int border,
                               vpx_codec_frame_buffer_t *fb,
                               vpx_get_frame_buffer_cb_fn_t cb,
                               void *cb_priv);
  int vp9_free_frame_buffer(YV12_BUFFER_CONFIG *ybf);

#ifdef __cplusplus
}
#endif

#endif
