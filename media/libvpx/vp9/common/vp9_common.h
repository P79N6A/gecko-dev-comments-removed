









#ifndef VP9_COMMON_VP9_COMMON_H_
#define VP9_COMMON_VP9_COMMON_H_



#include <assert.h>

#include "./vpx_config.h"
#include "vpx_mem/vpx_mem.h"
#include "vpx/vpx_integer.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define ROUND_POWER_OF_TWO(value, n) \
    (((value) + (1 << ((n) - 1))) >> (n))

#define ALIGN_POWER_OF_TWO(value, n) \
    (((value) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))


#define vp9_copy(dest, src) {            \
    assert(sizeof(dest) == sizeof(src)); \
    vpx_memcpy(dest, src, sizeof(src));  \
  }


#define vp9_copy_array(dest, src, n) {       \
    assert(sizeof(*dest) == sizeof(*src));   \
    vpx_memcpy(dest, src, n * sizeof(*src)); \
  }

#define vp9_zero(dest) vpx_memset(&dest, 0, sizeof(dest))
#define vp9_zero_array(dest, n) vpx_memset(dest, 0, n * sizeof(*dest))

static INLINE uint8_t clip_pixel(int val) {
  return (val > 255) ? 255u : (val < 0) ? 0u : val;
}

static INLINE int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

static INLINE double fclamp(double value, double low, double high) {
  return value < low ? low : (value > high ? high : value);
}

static int get_unsigned_bits(unsigned int num_values) {
  int cat = 0;
  if (num_values <= 1)
    return 0;
  num_values--;
  while (num_values > 0) {
    cat++;
    num_values >>= 1;
  }
  return cat;
}

#if CONFIG_DEBUG
#define CHECK_MEM_ERROR(cm, lval, expr) do { \
  lval = (expr); \
  if (!lval) \
    vpx_internal_error(&cm->error, VPX_CODEC_MEM_ERROR, \
                       "Failed to allocate "#lval" at %s:%d", \
                       __FILE__, __LINE__); \
  } while (0)
#else
#define CHECK_MEM_ERROR(cm, lval, expr) do { \
  lval = (expr); \
  if (!lval) \
    vpx_internal_error(&cm->error, VPX_CODEC_MEM_ERROR, \
                       "Failed to allocate "#lval); \
  } while (0)
#endif

#define VP9_SYNC_CODE_0 0x49
#define VP9_SYNC_CODE_1 0x83
#define VP9_SYNC_CODE_2 0x42

#define VP9_FRAME_MARKER 0x2


#endif  
