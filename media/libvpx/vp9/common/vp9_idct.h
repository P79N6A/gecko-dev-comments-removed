









#ifndef VP9_COMMON_VP9_IDCT_H_
#define VP9_COMMON_VP9_IDCT_H_

#include <assert.h>

#include "./vpx_config.h"
#include "vpx/vpx_integer.h"
#include "vp9/common/vp9_common.h"
#include "vp9/common/vp9_enums.h"

#ifdef __cplusplus
extern "C" {
#endif



#define DCT_CONST_BITS 14
#define DCT_CONST_ROUNDING  (1 << (DCT_CONST_BITS - 1))

#define UNIT_QUANT_SHIFT 2
#define UNIT_QUANT_FACTOR (1 << UNIT_QUANT_SHIFT)

#define pair_set_epi16(a, b) \
  _mm_set_epi16(b, a, b, a, b, a, b, a)

#define dual_set_epi16(a, b) \
  _mm_set_epi16(b, b, b, b, a, a, a, a)




#if CONFIG_VP9_HIGHBITDEPTH
typedef int64_t tran_high_t;
typedef int32_t tran_low_t;
#else
typedef int32_t tran_high_t;
typedef int16_t tran_low_t;
#endif






static const tran_high_t cospi_1_64  = 16364;
static const tran_high_t cospi_2_64  = 16305;
static const tran_high_t cospi_3_64  = 16207;
static const tran_high_t cospi_4_64  = 16069;
static const tran_high_t cospi_5_64  = 15893;
static const tran_high_t cospi_6_64  = 15679;
static const tran_high_t cospi_7_64  = 15426;
static const tran_high_t cospi_8_64  = 15137;
static const tran_high_t cospi_9_64  = 14811;
static const tran_high_t cospi_10_64 = 14449;
static const tran_high_t cospi_11_64 = 14053;
static const tran_high_t cospi_12_64 = 13623;
static const tran_high_t cospi_13_64 = 13160;
static const tran_high_t cospi_14_64 = 12665;
static const tran_high_t cospi_15_64 = 12140;
static const tran_high_t cospi_16_64 = 11585;
static const tran_high_t cospi_17_64 = 11003;
static const tran_high_t cospi_18_64 = 10394;
static const tran_high_t cospi_19_64 = 9760;
static const tran_high_t cospi_20_64 = 9102;
static const tran_high_t cospi_21_64 = 8423;
static const tran_high_t cospi_22_64 = 7723;
static const tran_high_t cospi_23_64 = 7005;
static const tran_high_t cospi_24_64 = 6270;
static const tran_high_t cospi_25_64 = 5520;
static const tran_high_t cospi_26_64 = 4756;
static const tran_high_t cospi_27_64 = 3981;
static const tran_high_t cospi_28_64 = 3196;
static const tran_high_t cospi_29_64 = 2404;
static const tran_high_t cospi_30_64 = 1606;
static const tran_high_t cospi_31_64 = 804;


static const tran_high_t sinpi_1_9 = 5283;
static const tran_high_t sinpi_2_9 = 9929;
static const tran_high_t sinpi_3_9 = 13377;
static const tran_high_t sinpi_4_9 = 15212;

static INLINE tran_low_t dct_const_round_shift(tran_high_t input) {
  tran_high_t rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
#if CONFIG_VP9_HIGHBITDEPTH
  
  
  
  
  
#elif CONFIG_COEFFICIENT_RANGE_CHECKING
  
  
  
  
  
  
  assert(INT16_MIN <= rv);
  assert(rv <= INT16_MAX);
#endif
  return (tran_low_t)rv;
}

typedef void (*transform_1d)(const tran_low_t*, tran_low_t*);

typedef struct {
  transform_1d cols, rows;  
} transform_2d;

#if CONFIG_VP9_HIGHBITDEPTH
typedef void (*high_transform_1d)(const tran_low_t*, tran_low_t*, int bd);

typedef struct {
  high_transform_1d cols, rows;  
} high_transform_2d;
#endif  

void vp9_iwht4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                     int eob);
void vp9_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                     int eob);
void vp9_idct8x8_add(const tran_low_t *input, uint8_t *dest, int stride,
                     int eob);
void vp9_idct16x16_add(const tran_low_t *input, uint8_t *dest, int stride, int
                       eob);
void vp9_idct32x32_add(const tran_low_t *input, uint8_t *dest, int stride,
                       int eob);

void vp9_iht4x4_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest,
                    int stride, int eob);
void vp9_iht8x8_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest,
                    int stride, int eob);
void vp9_iht16x16_add(TX_TYPE tx_type, const tran_low_t *input, uint8_t *dest,
                      int stride, int eob);

#if CONFIG_VP9_HIGHBITDEPTH
void vp9_high_iwht4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                          int eob, int bd);
void vp9_high_idct4x4_add(const tran_low_t *input, uint8_t *dest, int stride,
                          int eob, int bd);
void vp9_high_idct8x8_add(const tran_low_t *input, uint8_t *dest, int stride,
                          int eob, int bd);
void vp9_high_idct16x16_add(const tran_low_t *input, uint8_t *dest, int stride,
                            int eob, int bd);
void vp9_high_idct32x32_add(const tran_low_t *input, uint8_t *dest, int stride,
                            int eob, int bd);
void vp9_high_iht4x4_add(TX_TYPE tx_type, const tran_low_t *input,
                         uint8_t *dest, int stride, int eob, int bd);
void vp9_high_iht8x8_add(TX_TYPE tx_type, const tran_low_t *input,
                         uint8_t *dest, int stride, int eob, int bd);
void vp9_high_iht16x16_add(TX_TYPE tx_type, const tran_low_t *input,
                           uint8_t *dest, int stride, int eob, int bd);
#endif  
#ifdef __cplusplus
}  
#endif

#endif
