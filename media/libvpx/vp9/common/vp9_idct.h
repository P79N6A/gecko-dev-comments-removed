









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

#define pair_set_epi32(a, b) \
  _mm_set_epi32(b, a, b, a)






static const int cospi_1_64  = 16364;
static const int cospi_2_64  = 16305;
static const int cospi_3_64  = 16207;
static const int cospi_4_64  = 16069;
static const int cospi_5_64  = 15893;
static const int cospi_6_64  = 15679;
static const int cospi_7_64  = 15426;
static const int cospi_8_64  = 15137;
static const int cospi_9_64  = 14811;
static const int cospi_10_64 = 14449;
static const int cospi_11_64 = 14053;
static const int cospi_12_64 = 13623;
static const int cospi_13_64 = 13160;
static const int cospi_14_64 = 12665;
static const int cospi_15_64 = 12140;
static const int cospi_16_64 = 11585;
static const int cospi_17_64 = 11003;
static const int cospi_18_64 = 10394;
static const int cospi_19_64 = 9760;
static const int cospi_20_64 = 9102;
static const int cospi_21_64 = 8423;
static const int cospi_22_64 = 7723;
static const int cospi_23_64 = 7005;
static const int cospi_24_64 = 6270;
static const int cospi_25_64 = 5520;
static const int cospi_26_64 = 4756;
static const int cospi_27_64 = 3981;
static const int cospi_28_64 = 3196;
static const int cospi_29_64 = 2404;
static const int cospi_30_64 = 1606;
static const int cospi_31_64 = 804;


static const int sinpi_1_9 = 5283;
static const int sinpi_2_9 = 9929;
static const int sinpi_3_9 = 13377;
static const int sinpi_4_9 = 15212;

static INLINE int dct_const_round_shift(int input) {
  int rv = ROUND_POWER_OF_TWO(input, DCT_CONST_BITS);
  return (int16_t)rv;
}

typedef void (*transform_1d)(const int16_t*, int16_t*);

typedef struct {
  transform_1d cols, rows;  
} transform_2d;

void vp9_iwht4x4_add(const int16_t *input, uint8_t *dest, int stride, int eob);

void vp9_idct4x4_add(const int16_t *input, uint8_t *dest, int stride, int eob);
void vp9_idct8x8_add(const int16_t *input, uint8_t *dest, int stride, int eob);
void vp9_idct16x16_add(const int16_t *input, uint8_t *dest, int stride, int
                       eob);
void vp9_idct32x32_add(const int16_t *input, uint8_t *dest, int stride,
                       int eob);

void vp9_iht4x4_add(TX_TYPE tx_type, const int16_t *input, uint8_t *dest,
                    int stride, int eob);
void vp9_iht8x8_add(TX_TYPE tx_type, const int16_t *input, uint8_t *dest,
                    int stride, int eob);
void vp9_iht16x16_add(TX_TYPE tx_type, const int16_t *input, uint8_t *dest,
                      int stride, int eob);


#ifdef __cplusplus
}  
#endif

#endif
