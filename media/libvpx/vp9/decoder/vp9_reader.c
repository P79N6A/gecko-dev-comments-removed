









#include "vpx_ports/mem.h"
#include "vpx_mem/vpx_mem.h"

#include "vp9/decoder/vp9_reader.h"




#define LOTS_OF_BITS 0x40000000

int vp9_reader_init(vp9_reader *r,
                    const uint8_t *buffer,
                    size_t size,
                    vpx_decrypt_cb decrypt_cb,
                    void *decrypt_state) {
  if (size && !buffer) {
    return 1;
  } else {
    r->buffer_end = buffer + size;
    r->buffer = buffer;
    r->value = 0;
    r->count = -8;
    r->range = 255;
    r->decrypt_cb = decrypt_cb;
    r->decrypt_state = decrypt_state;
    vp9_reader_fill(r);
    return vp9_read_bit(r) != 0;  
  }
}

void vp9_reader_fill(vp9_reader *r) {
  const uint8_t *const buffer_end = r->buffer_end;
  const uint8_t *buffer = r->buffer;
  const uint8_t *buffer_start = buffer;
  BD_VALUE value = r->value;
  int count = r->count;
  int shift = BD_VALUE_SIZE - CHAR_BIT - (count + CHAR_BIT);
  int loop_end = 0;
  const size_t bytes_left = buffer_end - buffer;
  const size_t bits_left = bytes_left * CHAR_BIT;
  const int x = (int)(shift + CHAR_BIT - bits_left);

  if (r->decrypt_cb) {
    size_t n = MIN(sizeof(r->clear_buffer), bytes_left);
    r->decrypt_cb(r->decrypt_state, buffer, r->clear_buffer, (int)n);
    buffer = r->clear_buffer;
    buffer_start = r->clear_buffer;
  }

  if (x >= 0) {
    count += LOTS_OF_BITS;
    loop_end = x;
  }

  if (x < 0 || bits_left) {
    while (shift >= loop_end) {
      count += CHAR_BIT;
      value |= (BD_VALUE)*buffer++ << shift;
      shift -= CHAR_BIT;
    }
  }

  
  
  
  r->buffer += buffer - buffer_start;
  r->value = value;
  r->count = count;
}

const uint8_t *vp9_reader_find_end(vp9_reader *r) {
  
  while (r->count > CHAR_BIT && r->count < BD_VALUE_SIZE) {
    r->count -= CHAR_BIT;
    r->buffer--;
  }
  return r->buffer;
}

int vp9_reader_has_error(vp9_reader *r) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  return r->count > BD_VALUE_SIZE && r->count < LOTS_OF_BITS;
}
