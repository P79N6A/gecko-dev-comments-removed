









#ifndef VP9_DECODER_VP9_READ_BIT_BUFFER_H_
#define VP9_DECODER_VP9_READ_BIT_BUFFER_H_

#include <limits.h>

#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vp9_rb_error_handler)(void *data);

struct vp9_read_bit_buffer {
  const uint8_t *bit_buffer;
  const uint8_t *bit_buffer_end;
  size_t bit_offset;

  void *error_handler_data;
  vp9_rb_error_handler error_handler;
};

size_t vp9_rb_bytes_read(struct vp9_read_bit_buffer *rb);

int vp9_rb_read_bit(struct vp9_read_bit_buffer *rb);

int vp9_rb_read_literal(struct vp9_read_bit_buffer *rb, int bits);

int vp9_rb_read_signed_literal(struct vp9_read_bit_buffer *rb, int bits);

#ifdef __cplusplus
}  
#endif

#endif
