









#ifndef VPX_VPX_FRAME_BUFFER_H_
#define VPX_VPX_FRAME_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "./vpx_integer.h"



#define VPX_MAXIMUM_WORK_BUFFERS 1



#define VP9_MAXIMUM_REF_BUFFERS 8





typedef struct vpx_codec_frame_buffer {
  uint8_t *data;  
  size_t size;  
  void *priv;  
} vpx_codec_frame_buffer_t;



















typedef int (*vpx_get_frame_buffer_cb_fn_t)(
    void *priv, size_t min_size, vpx_codec_frame_buffer_t *fb);











typedef int (*vpx_release_frame_buffer_cb_fn_t)(
    void *priv, vpx_codec_frame_buffer_t *fb);

#ifdef __cplusplus
}  
#endif

#endif
