









#ifndef VP9_COMMON_VP9_FRAME_BUFFERS_H_
#define VP9_COMMON_VP9_FRAME_BUFFERS_H_

#include "vpx/vpx_frame_buffer.h"
#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InternalFrameBuffer {
  uint8_t *data;
  size_t size;
  int in_use;
} InternalFrameBuffer;

typedef struct InternalFrameBufferList {
  int num_internal_frame_buffers;
  InternalFrameBuffer *int_fb;
} InternalFrameBufferList;


int vp9_alloc_internal_frame_buffers(InternalFrameBufferList *list);


void vp9_free_internal_frame_buffers(InternalFrameBufferList *list);





int vp9_get_frame_buffer(void *cb_priv, size_t min_size,
                         vpx_codec_frame_buffer_t *fb);



int vp9_release_frame_buffer(void *cb_priv, vpx_codec_frame_buffer_t *fb);

#ifdef __cplusplus
}  
#endif

#endif
