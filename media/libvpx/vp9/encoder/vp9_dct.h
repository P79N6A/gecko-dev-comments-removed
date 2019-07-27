









#ifndef VP9_ENCODER_VP9_DCT_H_
#define VP9_ENCODER_VP9_DCT_H_

#include "vp9/common/vp9_idct.h"

#ifdef __cplusplus
extern "C" {
#endif

void vp9_highbd_fdct4x4_c(const int16_t *input, tran_low_t *output, int stride);
void vp9_highbd_fdct8x8_c(const int16_t *input, tran_low_t *output, int stride);
void vp9_highbd_fdct16x16_c(const int16_t *input, tran_low_t *output,
                            int stride);
void vp9_highbd_fdct32x32_c(const int16_t *input, tran_low_t *out, int stride);
void vp9_highbd_fdct32x32_rd_c(const int16_t *input, tran_low_t *out,
                               int stride);

void vp9_fdct4(const tran_low_t *input, tran_low_t *output);
void vp9_fadst4(const tran_low_t *input, tran_low_t *output);
void vp9_fdct8(const tran_low_t *input, tran_low_t *output);
void vp9_fadst8(const tran_low_t *input, tran_low_t *output);
void vp9_fdct16(const tran_low_t in[16], tran_low_t out[16]);
void vp9_fadst16(const tran_low_t *input, tran_low_t *output);
void vp9_fdct32(const tran_high_t *input, tran_high_t *output, int round);

static const transform_2d FHT_4[] = {
  { vp9_fdct4,  vp9_fdct4  },  
  { vp9_fadst4, vp9_fdct4  },  
  { vp9_fdct4,  vp9_fadst4 },  
  { vp9_fadst4, vp9_fadst4 }   
};

static const transform_2d FHT_8[] = {
  { vp9_fdct8,  vp9_fdct8  },  
  { vp9_fadst8, vp9_fdct8  },  
  { vp9_fdct8,  vp9_fadst8 },  
  { vp9_fadst8, vp9_fadst8 }   
};

static const transform_2d FHT_16[] = {
  { vp9_fdct16,  vp9_fdct16  },  
  { vp9_fadst16, vp9_fdct16  },  
  { vp9_fdct16,  vp9_fadst16 },  
  { vp9_fadst16, vp9_fadst16 }   
};

#ifdef __cplusplus
}  
#endif

#endif
