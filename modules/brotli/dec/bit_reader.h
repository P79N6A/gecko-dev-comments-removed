
















#ifndef BROTLI_DEC_BIT_READER_H_
#define BROTLI_DEC_BIT_READER_H_

#include <string.h>
#include "./streams.h"
#include "./types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define BROTLI_MAX_NUM_BIT_READ   25
#define BROTLI_READ_SIZE          4096
#define BROTLI_IBUF_SIZE          (2 * BROTLI_READ_SIZE + 32)
#define BROTLI_IBUF_MASK          (2 * BROTLI_READ_SIZE - 1)

#define UNALIGNED_COPY64(dst, src) memcpy(dst, src, 8)

static const uint32_t kBitMask[BROTLI_MAX_NUM_BIT_READ] = {
  0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767,
  65535, 131071, 262143, 524287, 1048575, 2097151, 4194303, 8388607, 16777215
};

typedef struct {
  
  
  uint8_t buf_[BROTLI_IBUF_SIZE];
  uint8_t*    buf_ptr_;      
  BrotliInput input_;        
  uint64_t    val_;          
  uint32_t    pos_;          
  uint32_t    bit_pos_;      
  uint32_t    bit_end_pos_;  
  int         eos_;          
} BrotliBitReader;

int BrotliInitBitReader(BrotliBitReader* const br, BrotliInput input);


static BROTLI_INLINE uint32_t BrotliPrefetchBits(BrotliBitReader* const br) {
  return (uint32_t)(br->val_ >> br->bit_pos_);
}



static BROTLI_INLINE void BrotliSetBitPos(BrotliBitReader* const br,
                                          uint32_t val) {
#ifdef BROTLI_DECODE_DEBUG
  uint32_t n_bits = val - br->bit_pos_;
  const uint32_t bval = (uint32_t)(br->val_ >> br->bit_pos_) & kBitMask[n_bits];
  printf("[BrotliReadBits]  %010d %2d  val: %6x\n",
         (br->pos_ << 3) + br->bit_pos_ - 64, n_bits, bval);
#endif
  br->bit_pos_ = val;
}


static BROTLI_INLINE void ShiftBytes(BrotliBitReader* const br) {
  while (br->bit_pos_ >= 8) {
    br->val_ >>= 8;
    br->val_ |= ((uint64_t)br->buf_[br->pos_ & BROTLI_IBUF_MASK]) << 56;
    ++br->pos_;
    br->bit_pos_ -= 8;
    br->bit_end_pos_ -= 8;
  }
}













static BROTLI_INLINE int BrotliReadMoreInput(BrotliBitReader* const br) {
  if (br->bit_end_pos_ > 256) {
    return 1;
  } else if (br->eos_) {
    return br->bit_pos_ <= br->bit_end_pos_;
  } else {
    uint8_t* dst = br->buf_ptr_;
    int bytes_read = BrotliRead(br->input_, dst, BROTLI_READ_SIZE);
    if (bytes_read < 0) {
      return 0;
    }
    if (bytes_read < BROTLI_READ_SIZE) {
      br->eos_ = 1;
      
#if (defined(__x86_64__) || defined(_M_X64))
      *(uint64_t*)(dst + bytes_read) = 0;
      *(uint64_t*)(dst + bytes_read + 8) = 0;
      *(uint64_t*)(dst + bytes_read + 16) = 0;
      *(uint64_t*)(dst + bytes_read + 24) = 0;
#else
      memset(dst + bytes_read, 0, 32);
#endif
    }
    if (dst == br->buf_) {
      
#if (defined(__x86_64__) || defined(_M_X64))
      UNALIGNED_COPY64(br->buf_ + BROTLI_IBUF_SIZE - 32, br->buf_);
      UNALIGNED_COPY64(br->buf_ + BROTLI_IBUF_SIZE - 24, br->buf_ + 8);
      UNALIGNED_COPY64(br->buf_ + BROTLI_IBUF_SIZE - 16, br->buf_ + 16);
      UNALIGNED_COPY64(br->buf_ + BROTLI_IBUF_SIZE - 8, br->buf_ + 24);
#else
      memcpy(br->buf_ + (BROTLI_READ_SIZE << 1), br->buf_, 32);
#endif
      br->buf_ptr_ = br->buf_ + BROTLI_READ_SIZE;
    } else {
      br->buf_ptr_ = br->buf_;
    }
    br->bit_end_pos_ += ((uint32_t)bytes_read << 3);
    return 1;
  }
}


static BROTLI_INLINE void BrotliFillBitWindow(BrotliBitReader* const br) {
  if (br->bit_pos_ >= 40) {
#if (defined(__x86_64__) || defined(_M_X64))
    br->val_ >>= 40;
    
    
    br->val_ |= *(const uint64_t*)(
        br->buf_ + (br->pos_ & BROTLI_IBUF_MASK)) << 24;
    br->pos_ += 5;
    br->bit_pos_ -= 40;
    br->bit_end_pos_ -= 40;
#else
    ShiftBytes(br);
#endif
  }
}


static BROTLI_INLINE uint32_t BrotliReadBits(
    BrotliBitReader* const br, int n_bits) {
  uint32_t val;
  BrotliFillBitWindow(br);
  val = (uint32_t)(br->val_ >> br->bit_pos_) & kBitMask[n_bits];
#ifdef BROTLI_DECODE_DEBUG
  printf("[BrotliReadBits]  %010d %2d  val: %6x\n",
         (br->pos_ << 3) + br->bit_pos_ - 64, n_bits, val);
#endif
  br->bit_pos_ += (uint32_t)n_bits;
  return val;
}

#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
