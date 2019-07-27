









#include <assert.h>
#include "vp9/encoder/vp9_writer.h"
#include "vp9/common/vp9_entropy.h"

void vp9_start_encode(vp9_writer *br, uint8_t *source) {
  br->lowvalue = 0;
  br->range    = 255;
  br->count    = -24;
  br->buffer   = source;
  br->pos      = 0;
  vp9_write_bit(br, 0);
}

void vp9_stop_encode(vp9_writer *br) {
  int i;

  for (i = 0; i < 32; i++)
    vp9_write_bit(br, 0);

  
  if ((br->buffer[br->pos - 1] & 0xe0) == 0xc0)
    br->buffer[br->pos++] = 0;
}

