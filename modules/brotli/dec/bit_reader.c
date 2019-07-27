
















#include <assert.h>
#include <stdlib.h>

#include "./bit_reader.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int BrotliInitBitReader(BrotliBitReader* const br, BrotliInput input) {
  size_t i;
  assert(br != NULL);

  br->buf_ptr_ = br->buf_;
  br->input_ = input;
  br->val_ = 0;
  br->pos_ = 0;
  br->bit_pos_ = 0;
  br->bit_end_pos_ = 0;
  br->eos_ = 0;
  if (!BrotliReadMoreInput(br)) {
    return 0;
  }
  for (i = 0; i < sizeof(br->val_); ++i) {
    br->val_ |= ((uint64_t)br->buf_[br->pos_]) << (8 * i);
    ++br->pos_;
  }
  return (br->bit_end_pos_ > 0);
}

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif
