









#include "libyuv/row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_NEON) && defined(__ARM_NEON__)
static uvec8 kVTbl4x4Transpose =
  { 0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15 };

void TransposeWx8_NEON(const uint8* src, int src_stride,
                       uint8* dst, int dst_stride,
                       int width) {
  asm volatile (
    
    
    
    "sub         %4, #8                        \n"

    
    ".p2align  2                               \n"
    "1:                                        \n"
      "mov         r9, %0                      \n"

      "vld1.8      {d0}, [r9], %1              \n"
      "vld1.8      {d1}, [r9], %1              \n"
      "vld1.8      {d2}, [r9], %1              \n"
      "vld1.8      {d3}, [r9], %1              \n"
      "vld1.8      {d4}, [r9], %1              \n"
      "vld1.8      {d5}, [r9], %1              \n"
      "vld1.8      {d6}, [r9], %1              \n"
      "vld1.8      {d7}, [r9]                  \n"

      "vtrn.8      d1, d0                      \n"
      "vtrn.8      d3, d2                      \n"
      "vtrn.8      d5, d4                      \n"
      "vtrn.8      d7, d6                      \n"

      "vtrn.16     d1, d3                      \n"
      "vtrn.16     d0, d2                      \n"
      "vtrn.16     d5, d7                      \n"
      "vtrn.16     d4, d6                      \n"

      "vtrn.32     d1, d5                      \n"
      "vtrn.32     d0, d4                      \n"
      "vtrn.32     d3, d7                      \n"
      "vtrn.32     d2, d6                      \n"

      "vrev16.8    q0, q0                      \n"
      "vrev16.8    q1, q1                      \n"
      "vrev16.8    q2, q2                      \n"
      "vrev16.8    q3, q3                      \n"

      "mov         r9, %2                      \n"

      "vst1.8      {d1}, [r9], %3              \n"
      "vst1.8      {d0}, [r9], %3              \n"
      "vst1.8      {d3}, [r9], %3              \n"
      "vst1.8      {d2}, [r9], %3              \n"
      "vst1.8      {d5}, [r9], %3              \n"
      "vst1.8      {d4}, [r9], %3              \n"
      "vst1.8      {d7}, [r9], %3              \n"
      "vst1.8      {d6}, [r9]                  \n"

      "add         %0, #8                      \n"  
      "add         %2, %2, %3, lsl #3          \n"  
      "subs        %4,  #8                     \n"  
      "bge         1b                          \n"

    
    
    "adds        %4, #8                        \n"
    "beq         4f                            \n"

    
    "cmp         %4, #2                        \n"
    "blt         3f                            \n"

    "cmp         %4, #4                        \n"
    "blt         2f                            \n"

    
    "mov         r9, %0                        \n"
    "vld1.32     {d0[0]}, [r9], %1             \n"
    "vld1.32     {d0[1]}, [r9], %1             \n"
    "vld1.32     {d1[0]}, [r9], %1             \n"
    "vld1.32     {d1[1]}, [r9], %1             \n"
    "vld1.32     {d2[0]}, [r9], %1             \n"
    "vld1.32     {d2[1]}, [r9], %1             \n"
    "vld1.32     {d3[0]}, [r9], %1             \n"
    "vld1.32     {d3[1]}, [r9]                 \n"

    "mov         r9, %2                        \n"

    "vld1.8      {q3}, [%5]                    \n"

    "vtbl.8      d4, {d0, d1}, d6              \n"
    "vtbl.8      d5, {d0, d1}, d7              \n"
    "vtbl.8      d0, {d2, d3}, d6              \n"
    "vtbl.8      d1, {d2, d3}, d7              \n"

    
    
    "vst1.32     {d4[0]}, [r9], %3             \n"
    "vst1.32     {d4[1]}, [r9], %3             \n"
    "vst1.32     {d5[0]}, [r9], %3             \n"
    "vst1.32     {d5[1]}, [r9]                 \n"

    "add         r9, %2, #4                    \n"
    "vst1.32     {d0[0]}, [r9], %3             \n"
    "vst1.32     {d0[1]}, [r9], %3             \n"
    "vst1.32     {d1[0]}, [r9], %3             \n"
    "vst1.32     {d1[1]}, [r9]                 \n"

    "add         %0, #4                        \n"  
    "add         %2, %2, %3, lsl #2            \n"  
    "subs        %4,  #4                       \n"  
    "beq         4f                            \n"

    
    
    "cmp         %4, #2                        \n"
    "blt         3f                            \n"

    
    "2:                                        \n"
    "mov         r9, %0                        \n"
    "vld1.16     {d0[0]}, [r9], %1             \n"
    "vld1.16     {d1[0]}, [r9], %1             \n"
    "vld1.16     {d0[1]}, [r9], %1             \n"
    "vld1.16     {d1[1]}, [r9], %1             \n"
    "vld1.16     {d0[2]}, [r9], %1             \n"
    "vld1.16     {d1[2]}, [r9], %1             \n"
    "vld1.16     {d0[3]}, [r9], %1             \n"
    "vld1.16     {d1[3]}, [r9]                 \n"

    "vtrn.8      d0, d1                        \n"

    "mov         r9, %2                        \n"

    "vst1.64     {d0}, [r9], %3                \n"
    "vst1.64     {d1}, [r9]                    \n"

    "add         %0, #2                        \n"  
    "add         %2, %2, %3, lsl #1            \n"  
    "subs        %4,  #2                       \n"  
    "beq         4f                            \n"

    
    "3:                                        \n"
    "vld1.8      {d0[0]}, [%0], %1             \n"
    "vld1.8      {d0[1]}, [%0], %1             \n"
    "vld1.8      {d0[2]}, [%0], %1             \n"
    "vld1.8      {d0[3]}, [%0], %1             \n"
    "vld1.8      {d0[4]}, [%0], %1             \n"
    "vld1.8      {d0[5]}, [%0], %1             \n"
    "vld1.8      {d0[6]}, [%0], %1             \n"
    "vld1.8      {d0[7]}, [%0]                 \n"

    "vst1.64     {d0}, [%2]                    \n"

    "4:                                        \n"

    : "+r"(src),               
      "+r"(src_stride),        
      "+r"(dst),               
      "+r"(dst_stride),        
      "+r"(width)              
    : "r"(&kVTbl4x4Transpose)  
    : "memory", "cc", "r9", "q0", "q1", "q2", "q3"
  );
}

static uvec8 kVTbl4x4TransposeDi =
  { 0,  8,  1,  9,  2, 10,  3, 11,  4, 12,  5, 13,  6, 14,  7, 15 };

void TransposeUVWx8_NEON(const uint8* src, int src_stride,
                         uint8* dst_a, int dst_stride_a,
                         uint8* dst_b, int dst_stride_b,
                         int width) {
  asm volatile (
    
    
    
    "sub         %6, #8                        \n"

    
    ".p2align  2                               \n"
    "1:                                        \n"
      "mov         r9, %0                      \n"

      "vld2.8      {d0,  d1},  [r9], %1        \n"
      "vld2.8      {d2,  d3},  [r9], %1        \n"
      "vld2.8      {d4,  d5},  [r9], %1        \n"
      "vld2.8      {d6,  d7},  [r9], %1        \n"
      "vld2.8      {d16, d17}, [r9], %1        \n"
      "vld2.8      {d18, d19}, [r9], %1        \n"
      "vld2.8      {d20, d21}, [r9], %1        \n"
      "vld2.8      {d22, d23}, [r9]            \n"

      "vtrn.8      q1, q0                      \n"
      "vtrn.8      q3, q2                      \n"
      "vtrn.8      q9, q8                      \n"
      "vtrn.8      q11, q10                    \n"

      "vtrn.16     q1, q3                      \n"
      "vtrn.16     q0, q2                      \n"
      "vtrn.16     q9, q11                     \n"
      "vtrn.16     q8, q10                     \n"

      "vtrn.32     q1, q9                      \n"
      "vtrn.32     q0, q8                      \n"
      "vtrn.32     q3, q11                     \n"
      "vtrn.32     q2, q10                     \n"

      "vrev16.8    q0, q0                      \n"
      "vrev16.8    q1, q1                      \n"
      "vrev16.8    q2, q2                      \n"
      "vrev16.8    q3, q3                      \n"
      "vrev16.8    q8, q8                      \n"
      "vrev16.8    q9, q9                      \n"
      "vrev16.8    q10, q10                    \n"
      "vrev16.8    q11, q11                    \n"

      "mov         r9, %2                      \n"

      "vst1.8      {d2},  [r9], %3             \n"
      "vst1.8      {d0},  [r9], %3             \n"
      "vst1.8      {d6},  [r9], %3             \n"
      "vst1.8      {d4},  [r9], %3             \n"
      "vst1.8      {d18}, [r9], %3             \n"
      "vst1.8      {d16}, [r9], %3             \n"
      "vst1.8      {d22}, [r9], %3             \n"
      "vst1.8      {d20}, [r9]                 \n"

      "mov         r9, %4                      \n"

      "vst1.8      {d3},  [r9], %5             \n"
      "vst1.8      {d1},  [r9], %5             \n"
      "vst1.8      {d7},  [r9], %5             \n"
      "vst1.8      {d5},  [r9], %5             \n"
      "vst1.8      {d19}, [r9], %5             \n"
      "vst1.8      {d17}, [r9], %5             \n"
      "vst1.8      {d23}, [r9], %5             \n"
      "vst1.8      {d21}, [r9]                 \n"

      "add         %0, #8*2                    \n"  
      "add         %2, %2, %3, lsl #3          \n"  
      "add         %4, %4, %5, lsl #3          \n"  
      "subs        %6,  #8                     \n"  
      "bge         1b                          \n"

    
    
    "adds        %6, #8                        \n"
    "beq         4f                            \n"

    
    "cmp         %6, #2                        \n"
    "blt         3f                            \n"

    "cmp         %6, #4                        \n"
    "blt         2f                            \n"

    
    
    "mov         r9, %0                        \n"
    "vld1.64     {d0}, [r9], %1                \n"
    "vld1.64     {d1}, [r9], %1                \n"
    "vld1.64     {d2}, [r9], %1                \n"
    "vld1.64     {d3}, [r9], %1                \n"
    "vld1.64     {d4}, [r9], %1                \n"
    "vld1.64     {d5}, [r9], %1                \n"
    "vld1.64     {d6}, [r9], %1                \n"
    "vld1.64     {d7}, [r9]                    \n"

    "vld1.8      {q15}, [%7]                   \n"

    "vtrn.8      q0, q1                        \n"
    "vtrn.8      q2, q3                        \n"

    "vtbl.8      d16, {d0, d1}, d30            \n"
    "vtbl.8      d17, {d0, d1}, d31            \n"
    "vtbl.8      d18, {d2, d3}, d30            \n"
    "vtbl.8      d19, {d2, d3}, d31            \n"
    "vtbl.8      d20, {d4, d5}, d30            \n"
    "vtbl.8      d21, {d4, d5}, d31            \n"
    "vtbl.8      d22, {d6, d7}, d30            \n"
    "vtbl.8      d23, {d6, d7}, d31            \n"

    "mov         r9, %2                        \n"

    "vst1.32     {d16[0]},  [r9], %3           \n"
    "vst1.32     {d16[1]},  [r9], %3           \n"
    "vst1.32     {d17[0]},  [r9], %3           \n"
    "vst1.32     {d17[1]},  [r9], %3           \n"

    "add         r9, %2, #4                    \n"
    "vst1.32     {d20[0]}, [r9], %3            \n"
    "vst1.32     {d20[1]}, [r9], %3            \n"
    "vst1.32     {d21[0]}, [r9], %3            \n"
    "vst1.32     {d21[1]}, [r9]                \n"

    "mov         r9, %4                        \n"

    "vst1.32     {d18[0]}, [r9], %5            \n"
    "vst1.32     {d18[1]}, [r9], %5            \n"
    "vst1.32     {d19[0]}, [r9], %5            \n"
    "vst1.32     {d19[1]}, [r9], %5            \n"

    "add         r9, %4, #4                    \n"
    "vst1.32     {d22[0]},  [r9], %5           \n"
    "vst1.32     {d22[1]},  [r9], %5           \n"
    "vst1.32     {d23[0]},  [r9], %5           \n"
    "vst1.32     {d23[1]},  [r9]               \n"

    "add         %0, #4*2                      \n"  
    "add         %2, %2, %3, lsl #2            \n"  
    "add         %4, %4, %5, lsl #2            \n"  
    "subs        %6,  #4                       \n"  
    "beq         4f                            \n"

    
    
    "cmp         %6, #2                        \n"
    "blt         3f                            \n"

    
    "2:                                        \n"
    "mov         r9, %0                        \n"
    "vld2.16     {d0[0], d2[0]}, [r9], %1      \n"
    "vld2.16     {d1[0], d3[0]}, [r9], %1      \n"
    "vld2.16     {d0[1], d2[1]}, [r9], %1      \n"
    "vld2.16     {d1[1], d3[1]}, [r9], %1      \n"
    "vld2.16     {d0[2], d2[2]}, [r9], %1      \n"
    "vld2.16     {d1[2], d3[2]}, [r9], %1      \n"
    "vld2.16     {d0[3], d2[3]}, [r9], %1      \n"
    "vld2.16     {d1[3], d3[3]}, [r9]          \n"

    "vtrn.8      d0, d1                        \n"
    "vtrn.8      d2, d3                        \n"

    "mov         r9, %2                        \n"

    "vst1.64     {d0}, [r9], %3                \n"
    "vst1.64     {d2}, [r9]                    \n"

    "mov         r9, %4                        \n"

    "vst1.64     {d1}, [r9], %5                \n"
    "vst1.64     {d3}, [r9]                    \n"

    "add         %0, #2*2                      \n"  
    "add         %2, %2, %3, lsl #1            \n"  
    "add         %4, %4, %5, lsl #1            \n"  
    "subs        %6,  #2                       \n"  
    "beq         4f                            \n"

    
    "3:                                        \n"
    "vld2.8      {d0[0], d1[0]}, [%0], %1      \n"
    "vld2.8      {d0[1], d1[1]}, [%0], %1      \n"
    "vld2.8      {d0[2], d1[2]}, [%0], %1      \n"
    "vld2.8      {d0[3], d1[3]}, [%0], %1      \n"
    "vld2.8      {d0[4], d1[4]}, [%0], %1      \n"
    "vld2.8      {d0[5], d1[5]}, [%0], %1      \n"
    "vld2.8      {d0[6], d1[6]}, [%0], %1      \n"
    "vld2.8      {d0[7], d1[7]}, [%0]          \n"

    "vst1.64     {d0}, [%2]                    \n"
    "vst1.64     {d1}, [%4]                    \n"

    "4:                                        \n"

    : "+r"(src),                 
      "+r"(src_stride),          
      "+r"(dst_a),               
      "+r"(dst_stride_a),        
      "+r"(dst_b),               
      "+r"(dst_stride_b),        
      "+r"(width)                
    : "r"(&kVTbl4x4TransposeDi)  
    : "memory", "cc", "r9",
      "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11"
  );
}
#endif

#ifdef __cplusplus
}  
}  
#endif
