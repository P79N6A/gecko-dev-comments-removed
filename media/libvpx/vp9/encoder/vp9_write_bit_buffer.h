









#ifndef VP9_ENCODER_VP9_WRITE_BIT_BUFFER_H_
#define VP9_ENCODER_VP9_WRITE_BIT_BUFFER_H_

#include <limits.h>

#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vp9_write_bit_buffer {
  uint8_t *bit_buffer;
  size_t bit_offset;
};

static size_t vp9_rb_bytes_written(struct vp9_write_bit_buffer *wb) {
  return wb->bit_offset / CHAR_BIT + (wb->bit_offset % CHAR_BIT > 0);
}

static void vp9_wb_write_bit(struct vp9_write_bit_buffer *wb, int bit) {
  const int off = wb->bit_offset;
  const int p = off / CHAR_BIT;
  const int q = CHAR_BIT - 1 - off % CHAR_BIT;
  if (q == CHAR_BIT -1) {
    wb->bit_buffer[p] = bit << q;
  } else {
    wb->bit_buffer[p] &= ~(1 << q);
    wb->bit_buffer[p] |= bit << q;
  }
  wb->bit_offset = off + 1;
}

static void vp9_wb_write_literal(struct vp9_write_bit_buffer *wb,
                              int data, int bits) {
  int bit;
  for (bit = bits - 1; bit >= 0; bit--)
    vp9_wb_write_bit(wb, (data >> bit) & 1);
}


#ifdef __cplusplus
}  
#endif

#endif
