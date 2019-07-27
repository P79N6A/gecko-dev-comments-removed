









#include "./vp9_rtcd.h"
#include "vp9/common/vp9_common.h"

void vp9_idct16x16_256_add_neon_pass1(const int16_t *input,
                                      int16_t *output,
                                      int output_stride);
void vp9_idct16x16_256_add_neon_pass2(const int16_t *src,
                                      int16_t *output,
                                      int16_t *pass1Output,
                                      int16_t skip_adding,
                                      uint8_t *dest,
                                      int dest_stride);
void vp9_idct16x16_10_add_neon_pass1(const int16_t *input,
                                     int16_t *output,
                                     int output_stride);
void vp9_idct16x16_10_add_neon_pass2(const int16_t *src,
                                     int16_t *output,
                                     int16_t *pass1Output,
                                     int16_t skip_adding,
                                     uint8_t *dest,
                                     int dest_stride);

#if HAVE_NEON_ASM

extern void vp9_push_neon(int64_t *store);
extern void vp9_pop_neon(int64_t *store);
#endif  

void vp9_idct16x16_256_add_neon(const int16_t *input,
                                uint8_t *dest, int dest_stride) {
#if HAVE_NEON_ASM
  int64_t store_reg[8];
#endif
  int16_t pass1_output[16*16] = {0};
  int16_t row_idct_output[16*16] = {0};

#if HAVE_NEON_ASM
  
  vp9_push_neon(store_reg);
#endif

  
  
  
  vp9_idct16x16_256_add_neon_pass1(input, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(input+1,
                                     row_idct_output,
                                     pass1_output,
                                     0,
                                     dest,
                                     dest_stride);

  
  
  
  vp9_idct16x16_256_add_neon_pass1(input+8*16, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(input+8*16+1,
                                     row_idct_output+8,
                                     pass1_output,
                                     0,
                                     dest,
                                     dest_stride);

  
  
  
  vp9_idct16x16_256_add_neon_pass1(row_idct_output, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(row_idct_output+1,
                                     row_idct_output,
                                     pass1_output,
                                     1,
                                     dest,
                                     dest_stride);

  
  
  
  vp9_idct16x16_256_add_neon_pass1(row_idct_output+8*16, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(row_idct_output+8*16+1,
                                     row_idct_output+8,
                                     pass1_output,
                                     1,
                                     dest+8,
                                     dest_stride);

#if HAVE_NEON_ASM
  
  vp9_pop_neon(store_reg);
#endif

  return;
}

void vp9_idct16x16_10_add_neon(const int16_t *input,
                               uint8_t *dest, int dest_stride) {
#if HAVE_NEON_ASM
  int64_t store_reg[8];
#endif
  int16_t pass1_output[16*16] = {0};
  int16_t row_idct_output[16*16] = {0};

#if HAVE_NEON_ASM
  
  vp9_push_neon(store_reg);
#endif

  
  
  
  vp9_idct16x16_10_add_neon_pass1(input, pass1_output, 8);

  
  
  
  vp9_idct16x16_10_add_neon_pass2(input+1,
                                        row_idct_output,
                                        pass1_output,
                                        0,
                                        dest,
                                        dest_stride);

  

  
  
  
  vp9_idct16x16_256_add_neon_pass1(row_idct_output, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(row_idct_output+1,
                                     row_idct_output,
                                     pass1_output,
                                     1,
                                     dest,
                                     dest_stride);

  
  
  
  vp9_idct16x16_256_add_neon_pass1(row_idct_output+8*16, pass1_output, 8);

  
  
  
  vp9_idct16x16_256_add_neon_pass2(row_idct_output+8*16+1,
                                     row_idct_output+8,
                                     pass1_output,
                                     1,
                                     dest+8,
                                     dest_stride);

#if HAVE_NEON_ASM
  
  vp9_pop_neon(store_reg);
#endif

  return;
}
