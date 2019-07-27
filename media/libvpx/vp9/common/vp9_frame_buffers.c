









#include <assert.h>

#include "vp9/common/vp9_frame_buffers.h"
#include "vpx_mem/vpx_mem.h"

int vp9_alloc_internal_frame_buffers(InternalFrameBufferList *list) {
  assert(list != NULL);
  vp9_free_internal_frame_buffers(list);

  list->num_internal_frame_buffers =
      VP9_MAXIMUM_REF_BUFFERS + VPX_MAXIMUM_WORK_BUFFERS;
  list->int_fb = vpx_calloc(list->num_internal_frame_buffers,
                            sizeof(*list->int_fb));
  return (list->int_fb == NULL);
}

void vp9_free_internal_frame_buffers(InternalFrameBufferList *list) {
  int i;

  assert(list != NULL);

  for (i = 0; i < list->num_internal_frame_buffers; ++i) {
    vpx_free(list->int_fb[i].data);
    list->int_fb[i].data = NULL;
  }
  vpx_free(list->int_fb);
  list->int_fb = NULL;
}

int vp9_get_frame_buffer(void *cb_priv, size_t min_size,
                         vpx_codec_frame_buffer_t *fb) {
  int i;
  InternalFrameBufferList *const int_fb_list =
      (InternalFrameBufferList *)cb_priv;
  if (int_fb_list == NULL)
    return -1;

  
  for (i = 0; i < int_fb_list->num_internal_frame_buffers; ++i) {
    if (!int_fb_list->int_fb[i].in_use)
      break;
  }

  if (i == int_fb_list->num_internal_frame_buffers)
    return -1;

  if (int_fb_list->int_fb[i].size < min_size) {
    int_fb_list->int_fb[i].data =
        (uint8_t *)vpx_realloc(int_fb_list->int_fb[i].data, min_size);
    if (!int_fb_list->int_fb[i].data)
      return -1;

    int_fb_list->int_fb[i].size = min_size;
  }

  fb->data = int_fb_list->int_fb[i].data;
  fb->size = int_fb_list->int_fb[i].size;
  int_fb_list->int_fb[i].in_use = 1;

  
  fb->priv = &int_fb_list->int_fb[i];
  return 0;
}

int vp9_release_frame_buffer(void *cb_priv, vpx_codec_frame_buffer_t *fb) {
  InternalFrameBuffer *const int_fb = (InternalFrameBuffer *)fb->priv;
  (void)cb_priv;
  int_fb->in_use = 0;
  return 0;
}
