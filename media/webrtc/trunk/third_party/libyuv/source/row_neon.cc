









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_NEON) && defined(__ARM_NEON__)


#define READYUV422                                                             \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vld1.32    {d2[0]}, [%1]!                 \n"                             \
    "vld1.32    {d2[1]}, [%2]!                 \n"


#define READYUV411                                                             \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vld1.16    {d2[0]}, [%1]!                 \n"                             \
    "vld1.16    {d2[1]}, [%2]!                 \n"                             \
    "vmov.u8    d3, d2                         \n"                             \
    "vzip.u8    d2, d3                         \n"


#define READYUV444                                                             \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vld1.8     {d2}, [%1]!                    \n"                             \
    "vld1.8     {d3}, [%2]!                    \n"                             \
    "vpaddl.u8  q1, q1                         \n"                             \
    "vrshrn.u16 d2, q1, #1                     \n"


#define READYUV400                                                             \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vmov.u8    d2, #128                       \n"


#define READNV12                                                               \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vld1.8     {d2}, [%1]!                    \n"                             \
    "vmov.u8    d3, d2                         \n"/* split odd/even uv apart */\
    "vuzp.u8    d2, d3                         \n"                             \
    "vtrn.u32   d2, d3                         \n"


#define READNV21                                                               \
    "vld1.8     {d0}, [%0]!                    \n"                             \
    "vld1.8     {d2}, [%1]!                    \n"                             \
    "vmov.u8    d3, d2                         \n"/* split odd/even uv apart */\
    "vuzp.u8    d3, d2                         \n"                             \
    "vtrn.u32   d2, d3                         \n"


#define READYUY2                                                               \
    "vld2.8     {d0, d2}, [%0]!                \n"                             \
    "vmov.u8    d3, d2                         \n"                             \
    "vuzp.u8    d2, d3                         \n"                             \
    "vtrn.u32   d2, d3                         \n"


#define READUYVY                                                               \
    "vld2.8     {d2, d3}, [%0]!                \n"                             \
    "vmov.u8    d0, d3                         \n"                             \
    "vmov.u8    d3, d2                         \n"                             \
    "vuzp.u8    d2, d3                         \n"                             \
    "vtrn.u32   d2, d3                         \n"

#define YUV422TORGB                                                            \
    "veor.u8    d2, d26                        \n"/*subtract 128 from u and v*/\
    "vmull.s8   q8, d2, d24                    \n"/*  u/v B/R component      */\
    "vmull.s8   q9, d2, d25                    \n"/*  u/v G component        */\
    "vmov.u8    d1, #0                         \n"/*  split odd/even y apart */\
    "vtrn.u8    d0, d1                         \n"                             \
    "vsub.s16   q0, q0, q15                    \n"/*  offset y               */\
    "vmul.s16   q0, q0, q14                    \n"                             \
    "vadd.s16   d18, d19                       \n"                             \
    "vqadd.s16  d20, d0, d16                   \n" /* B */                     \
    "vqadd.s16  d21, d1, d16                   \n"                             \
    "vqadd.s16  d22, d0, d17                   \n" /* R */                     \
    "vqadd.s16  d23, d1, d17                   \n"                             \
    "vqadd.s16  d16, d0, d18                   \n" /* G */                     \
    "vqadd.s16  d17, d1, d18                   \n"                             \
    "vqshrun.s16 d0, q10, #6                   \n" /* B */                     \
    "vqshrun.s16 d1, q11, #6                   \n" /* G */                     \
    "vqshrun.s16 d2, q8, #6                    \n" /* R */                     \
    "vmovl.u8   q10, d0                        \n"/*  set up for reinterleave*/\
    "vmovl.u8   q11, d1                        \n"                             \
    "vmovl.u8   q8, d2                         \n"                             \
    "vtrn.u8    d20, d21                       \n"                             \
    "vtrn.u8    d22, d23                       \n"                             \
    "vtrn.u8    d16, d17                       \n"                             \
    "vmov.u8    d21, d16                       \n"

static vec8 kUVToRB  = { 127, 127, 127, 127, 102, 102, 102, 102,
                         0, 0, 0, 0, 0, 0, 0, 0 };
static vec8 kUVToG = { -25, -25, -25, -25, -52, -52, -52, -52,
                       0, 0, 0, 0, 0, 0, 0, 0 };

void I444ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV444
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I411ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV411
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToBGRARow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_bgra,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d19, #255                      \n"
    "vst4.8     {d19, d20, d21, d22}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_bgra),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToABGRRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_abgr,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_abgr),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToRGBARow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_rgba,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d19, #255                      \n"
    "vst4.8     {d19, d20, d21, d22}, [%3]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_u),     
      "+r"(src_v),     
      "+r"(dst_rgba),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToRGB24Row_NEON(const uint8* src_y,
                         const uint8* src_u,
                         const uint8* src_v,
                         uint8* dst_rgb24,
                         int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vst3.8     {d20, d21, d22}, [%3]!         \n"
    "bgt        1b                             \n"
    : "+r"(src_y),      
      "+r"(src_u),      
      "+r"(src_v),      
      "+r"(dst_rgb24),  
      "+r"(width)       
    : "r"(&kUVToRB),    
      "r"(&kUVToG)      
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I422ToRAWRow_NEON(const uint8* src_y,
                       const uint8* src_u,
                       const uint8* src_v,
                       uint8* dst_raw,
                       int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vswp.u8    d20, d22                       \n"
    "vst3.8     {d20, d21, d22}, [%3]!         \n"
    "bgt        1b                             \n"
    : "+r"(src_y),    
      "+r"(src_u),    
      "+r"(src_v),    
      "+r"(dst_raw),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

#define ARGBTORGB565                                                           \
    "vshr.u8    d20, d20, #3                   \n"  /* B                    */ \
    "vshr.u8    d21, d21, #2                   \n"  /* G                    */ \
    "vshr.u8    d22, d22, #3                   \n"  /* R                    */ \
    "vmovl.u8   q8, d20                        \n"  /* B                    */ \
    "vmovl.u8   q9, d21                        \n"  /* G                    */ \
    "vmovl.u8   q10, d22                       \n"  /* R                    */ \
    "vshl.u16   q9, q9, #5                     \n"  /* G                    */ \
    "vshl.u16   q10, q10, #11                  \n"  /* R                    */ \
    "vorr       q0, q8, q9                     \n"  /* BG                   */ \
    "vorr       q0, q0, q10                    \n"  /* BGR                  */

void I422ToRGB565Row_NEON(const uint8* src_y,
                          const uint8* src_u,
                          const uint8* src_v,
                          uint8* dst_rgb565,
                          int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    ARGBTORGB565
    "vst1.8     {q0}, [%3]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_y),    
      "+r"(src_u),    
      "+r"(src_v),    
      "+r"(dst_rgb565),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

#define ARGBTOARGB1555                                                         \
    "vshr.u8    q10, q10, #3                   \n"  /* B                    */ \
    "vshr.u8    d22, d22, #3                   \n"  /* R                    */ \
    "vshr.u8    d23, d23, #7                   \n"  /* A                    */ \
    "vmovl.u8   q8, d20                        \n"  /* B                    */ \
    "vmovl.u8   q9, d21                        \n"  /* G                    */ \
    "vmovl.u8   q10, d22                       \n"  /* R                    */ \
    "vmovl.u8   q11, d23                       \n"  /* A                    */ \
    "vshl.u16   q9, q9, #5                     \n"  /* G                    */ \
    "vshl.u16   q10, q10, #10                  \n"  /* R                    */ \
    "vshl.u16   q11, q11, #15                  \n"  /* A                    */ \
    "vorr       q0, q8, q9                     \n"  /* BG                   */ \
    "vorr       q1, q10, q11                   \n"  /* RA                   */ \
    "vorr       q0, q0, q1                     \n"  /* BGRA                 */

void I422ToARGB1555Row_NEON(const uint8* src_y,
                            const uint8* src_u,
                            const uint8* src_v,
                            uint8* dst_argb1555,
                            int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    ARGBTOARGB1555
    "vst1.8     {q0}, [%3]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_y),    
      "+r"(src_u),    
      "+r"(src_v),    
      "+r"(dst_argb1555),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

#define ARGBTOARGB4444                                                         \
    "vshr.u8    d20, d20, #4                   \n"  /* B                    */ \
    "vbic.32    d21, d21, d4                   \n"  /* G                    */ \
    "vshr.u8    d22, d22, #4                   \n"  /* R                    */ \
    "vbic.32    d23, d23, d4                   \n"  /* A                    */ \
    "vorr       d0, d20, d21                   \n"  /* BG                   */ \
    "vorr       d1, d22, d23                   \n"  /* RA                   */ \
    "vzip.u8    d0, d1                         \n"  /* BGRA                 */

void I422ToARGB4444Row_NEON(const uint8* src_y,
                            const uint8* src_u,
                            const uint8* src_v,
                            uint8* dst_argb4444,
                            int width) {
  asm volatile (
    "vld1.8     {d24}, [%5]                    \n"
    "vld1.8     {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    "vmov.u8    d4, #0x0f                      \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV422
    YUV422TORGB
    "subs       %4, %4, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    ARGBTOARGB4444
    "vst1.8     {q0}, [%3]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_y),    
      "+r"(src_u),    
      "+r"(src_v),    
      "+r"(dst_argb4444),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void YToARGBRow_NEON(const uint8* src_y,
                     uint8* dst_argb,
                     int width) {
  asm volatile (
    "vld1.8     {d24}, [%3]                    \n"
    "vld1.8     {d25}, [%4]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUV400
    YUV422TORGB
    "subs       %2, %2, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%1]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void I400ToARGBRow_NEON(const uint8* src_y,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    ".p2align   2                              \n"
    "vmov.u8    d23, #255                      \n"
  "1:                                          \n"
    "vld1.8     {d20}, [%0]!                   \n"
    "vmov       d21, d20                       \n"
    "vmov       d22, d20                       \n"
    "subs       %2, %2, #8                     \n"
    "vst4.8     {d20, d21, d22, d23}, [%1]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(dst_argb),  
      "+r"(width)      
    :
    : "cc", "memory", "d20", "d21", "d22", "d23"
  );
}

void NV12ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_uv,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%4]                    \n"
    "vld1.8     {d25}, [%5]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READNV12
    YUV422TORGB
    "subs       %3, %3, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%2]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_uv),    
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void NV21ToARGBRow_NEON(const uint8* src_y,
                        const uint8* src_uv,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%4]                    \n"
    "vld1.8     {d25}, [%5]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READNV21
    YUV422TORGB
    "subs       %3, %3, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%2]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_uv),    
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void NV12ToRGB565Row_NEON(const uint8* src_y,
                          const uint8* src_uv,
                          uint8* dst_rgb565,
                          int width) {
  asm volatile (
    "vld1.8     {d24}, [%4]                    \n"
    "vld1.8     {d25}, [%5]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READNV12
    YUV422TORGB
    "subs       %3, %3, #8                     \n"
    ARGBTORGB565
    "vst1.8     {q0}, [%2]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_uv),    
      "+r"(dst_rgb565),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void NV21ToRGB565Row_NEON(const uint8* src_y,
                          const uint8* src_uv,
                          uint8* dst_rgb565,
                          int width) {
  asm volatile (
    "vld1.8     {d24}, [%4]                    \n"
    "vld1.8     {d25}, [%5]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READNV21
    YUV422TORGB
    "subs       %3, %3, #8                     \n"
    ARGBTORGB565
    "vst1.8     {q0}, [%2]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_y),     
      "+r"(src_uv),    
      "+r"(dst_rgb565),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void YUY2ToARGBRow_NEON(const uint8* src_yuy2,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%3]                    \n"
    "vld1.8     {d25}, [%4]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READYUY2
    YUV422TORGB
    "subs       %2, %2, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%1]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_yuy2),  
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void UYVYToARGBRow_NEON(const uint8* src_uyvy,
                        uint8* dst_argb,
                        int width) {
  asm volatile (
    "vld1.8     {d24}, [%3]                    \n"
    "vld1.8     {d25}, [%4]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    READUYVY
    YUV422TORGB
    "subs       %2, %2, #8                     \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%1]!    \n"
    "bgt        1b                             \n"
    : "+r"(src_uyvy),  
      "+r"(dst_argb),  
      "+r"(width)      
    : "r"(&kUVToRB),   
      "r"(&kUVToG)     
    : "cc", "memory", "q0", "q1", "q2", "q3",
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void SplitUVRow_NEON(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                     int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {q0, q1}, [%0]!                \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.8     {q0}, [%1]!                    \n"  
    "vst1.8     {q1}, [%2]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_uv),  
      "+r"(dst_u),   
      "+r"(dst_v),   
      "+r"(width)    
    :                       
    : "cc", "memory", "q0", "q1"  
  );
}


void MergeUVRow_NEON(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                     int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "vld1.8     {q1}, [%1]!                    \n"  
    "subs       %3, %3, #16                    \n"  
    "vst2.u8    {q0, q1}, [%2]!                \n"  
    "bgt        1b                             \n"
    :
      "+r"(src_u),   
      "+r"(src_v),   
      "+r"(dst_uv),  
      "+r"(width)    
    :                       
    : "cc", "memory", "q0", "q1"  
  );
}


void CopyRow_NEON(const uint8* src, uint8* dst, int count) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #32                    \n"  
    "vst1.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(count)  
  :                     
  : "cc", "memory", "q0", "q1"  
  );
}


void SetRow_NEON(uint8* dst, uint32 v32, int count) {
  asm volatile (
    "vdup.u32  q0, %2                          \n"  
    "1:                                        \n"
    "subs      %1, %1, #16                     \n"  
    "vst1.8    {q0}, [%0]!                     \n"  
    "bgt       1b                              \n"
  : "+r"(dst),   
    "+r"(count)  
  : "r"(v32)     
  : "cc", "memory", "q0"
  );
}



void ARGBSetRows_NEON(uint8* dst, uint32 v32, int width,
                      int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    SetRow_NEON(dst, v32, width << 2);
    dst += dst_stride;
  }
}

void MirrorRow_NEON(const uint8* src, uint8* dst, int width) {
  asm volatile (
    
    "mov        r3, #-16                       \n"
    "add        %0, %0, %2                     \n"
    "sub        %0, #16                        \n"

    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0], r3                 \n"  
    "subs       %2, #16                        \n"  
    "vrev64.8   q0, q0                         \n"
    "vst1.8     {d1}, [%1]!                    \n"  
    "vst1.8     {d0}, [%1]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "cc", "memory", "r3", "q0"
  );
}

void MirrorUVRow_NEON(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                      int width) {
  asm volatile (
    
    "mov        r12, #-16                      \n"
    "add        %0, %0, %3, lsl #1             \n"
    "sub        %0, #16                        \n"

    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {d0, d1}, [%0], r12            \n"  
    "subs       %3, #8                         \n"  
    "vrev64.8   q0, q0                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "vst1.8     {d1}, [%2]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_uv),  
    "+r"(dst_u),   
    "+r"(dst_v),   
    "+r"(width)    
  :
  : "cc", "memory", "r12", "q0"
  );
}

void ARGBMirrorRow_NEON(const uint8* src, uint8* dst, int width) {
  asm volatile (
    
    "mov        r3, #-16                       \n"
    "add        %0, %0, %2, lsl #2             \n"
    "sub        %0, #16                        \n"

    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0], r3                 \n"  
    "subs       %2, #4                         \n"  
    "vrev64.32  q0, q0                         \n"
    "vst1.8     {d1}, [%1]!                    \n"  
    "vst1.8     {d0}, [%1]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "cc", "memory", "r3", "q0"
  );
}

void RGB24ToARGBRow_NEON(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  asm volatile (
    "vmov.u8    d4, #255                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d1, d2, d3}, [%0]!            \n"  
    "subs       %2, %2, #8                     \n"  
    "vst4.8     {d1, d2, d3, d4}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb24),  
    "+r"(dst_argb),   
    "+r"(pix)         
  :
  : "cc", "memory", "d1", "d2", "d3", "d4"  
  );
}

void RAWToARGBRow_NEON(const uint8* src_raw, uint8* dst_argb, int pix) {
  asm volatile (
    "vmov.u8    d4, #255                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d1, d2, d3}, [%0]!            \n"  
    "subs       %2, %2, #8                     \n"  
    "vswp.u8    d1, d3                         \n"  
    "vst4.8     {d1, d2, d3, d4}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_raw),   
    "+r"(dst_argb),  
    "+r"(pix)        
  :
  : "cc", "memory", "d1", "d2", "d3", "d4"  
  );
}

#define RGB565TOARGB                                                           \
    "vshrn.u16  d6, q0, #5                     \n"  /* G xxGGGGGG           */ \
    "vuzp.u8    d0, d1                         \n"  /* d0 xxxBBBBB RRRRRxxx */ \
    "vshl.u8    d6, d6, #2                     \n"  /* G GGGGGG00 upper 6   */ \
    "vshr.u8    d1, d1, #3                     \n"  /* R 000RRRRR lower 5   */ \
    "vshl.u8    q0, q0, #3                     \n"  /* B,R BBBBB000 upper 5 */ \
    "vshr.u8    q2, q0, #5                     \n"  /* B,R 00000BBB lower 3 */ \
    "vorr.u8    d0, d0, d4                     \n"  /* B                    */ \
    "vshr.u8    d4, d6, #6                     \n"  /* G 000000GG lower 2   */ \
    "vorr.u8    d2, d1, d5                     \n"  /* R                    */ \
    "vorr.u8    d1, d4, d6                     \n"  /* G                    */

void RGB565ToARGBRow_NEON(const uint8* src_rgb565, uint8* dst_argb, int pix) {
  asm volatile (
    "vmov.u8    d3, #255                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    RGB565TOARGB
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb565),  
    "+r"(dst_argb),    
    "+r"(pix)          
  :
  : "cc", "memory", "q0", "q1", "q2", "q3"  
  );
}

#define ARGB1555TOARGB                                                         \
    "vshrn.u16  d7, q0, #8                     \n"  /* A Arrrrrxx           */ \
    "vshr.u8    d6, d7, #2                     \n"  /* R xxxRRRRR           */ \
    "vshrn.u16  d5, q0, #5                     \n"  /* G xxxGGGGG           */ \
    "vmovn.u16  d4, q0                         \n"  /* B xxxBBBBB           */ \
    "vshr.u8    d7, d7, #7                     \n"  /* A 0000000A           */ \
    "vneg.s8    d7, d7                         \n"  /* A AAAAAAAA upper 8   */ \
    "vshl.u8    d6, d6, #3                     \n"  /* R RRRRR000 upper 5   */ \
    "vshr.u8    q1, q3, #5                     \n"  /* R,A 00000RRR lower 3 */ \
    "vshl.u8    q0, q2, #3                     \n"  /* B,G BBBBB000 upper 5 */ \
    "vshr.u8    q2, q0, #5                     \n"  /* B,G 00000BBB lower 3 */ \
    "vorr.u8    q1, q1, q3                     \n"  /* R,A                  */ \
    "vorr.u8    q0, q0, q2                     \n"  /* B,G                  */ \


#define RGB555TOARGB                                                           \
    "vshrn.u16  d6, q0, #5                     \n"  /* G xxxGGGGG           */ \
    "vuzp.u8    d0, d1                         \n"  /* d0 xxxBBBBB xRRRRRxx */ \
    "vshl.u8    d6, d6, #3                     \n"  /* G GGGGG000 upper 5   */ \
    "vshr.u8    d1, d1, #2                     \n"  /* R 00xRRRRR lower 5   */ \
    "vshl.u8    q0, q0, #3                     \n"  /* B,R BBBBB000 upper 5 */ \
    "vshr.u8    q2, q0, #5                     \n"  /* B,R 00000BBB lower 3 */ \
    "vorr.u8    d0, d0, d4                     \n"  /* B                    */ \
    "vshr.u8    d4, d6, #5                     \n"  /* G 00000GGG lower 3   */ \
    "vorr.u8    d2, d1, d5                     \n"  /* R                    */ \
    "vorr.u8    d1, d4, d6                     \n"  /* G                    */

void ARGB1555ToARGBRow_NEON(const uint8* src_argb1555, uint8* dst_argb,
                            int pix) {
  asm volatile (
    "vmov.u8    d3, #255                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGB1555TOARGB
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb1555),  
    "+r"(dst_argb),    
    "+r"(pix)          
  :
  : "cc", "memory", "q0", "q1", "q2", "q3"  
  );
}

#define ARGB4444TOARGB                                                         \
    "vuzp.u8    d0, d1                         \n"  /* d0 BG, d1 RA         */ \
    "vshl.u8    q2, q0, #4                     \n"  /* B,R BBBB0000         */ \
    "vshr.u8    q1, q0, #4                     \n"  /* G,A 0000GGGG         */ \
    "vshr.u8    q0, q2, #4                     \n"  /* B,R 0000BBBB         */ \
    "vorr.u8    q0, q0, q2                     \n"  /* B,R BBBBBBBB         */ \
    "vshl.u8    q2, q1, #4                     \n"  /* G,A GGGG0000         */ \
    "vorr.u8    q1, q1, q2                     \n"  /* G,A GGGGGGGG         */ \
    "vswp.u8    d1, d2                         \n"  /* B,R,G,A -> B,G,R,A   */

void ARGB4444ToARGBRow_NEON(const uint8* src_argb4444, uint8* dst_argb,
                            int pix) {
  asm volatile (
    "vmov.u8    d3, #255                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGB4444TOARGB
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb4444),  
    "+r"(dst_argb),    
    "+r"(pix)          
  :
  : "cc", "memory", "q0", "q1", "q2"  
  );
}

void ARGBToRGB24Row_NEON(const uint8* src_argb, uint8* dst_rgb24, int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d1, d2, d3, d4}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vst3.8     {d1, d2, d3}, [%1]!            \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_rgb24),  
    "+r"(pix)         
  :
  : "cc", "memory", "d1", "d2", "d3", "d4"  
  );
}

void ARGBToRAWRow_NEON(const uint8* src_argb, uint8* dst_raw, int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d1, d2, d3, d4}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vswp.u8    d1, d3                         \n"  
    "vst3.8     {d1, d2, d3}, [%1]!            \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_raw),   
    "+r"(pix)        
  :
  : "cc", "memory", "d1", "d2", "d3", "d4"  
  );
}

void YUY2ToYRow_NEON(const uint8* src_yuy2, uint8* dst_y, int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {q0, q1}, [%0]!                \n"  
    "subs       %2, %2, #16                    \n"  
    "vst1.8     {q0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1"  
  );
}

void UYVYToYRow_NEON(const uint8* src_uyvy, uint8* dst_y, int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {q0, q1}, [%0]!                \n"  
    "subs       %2, %2, #16                    \n"  
    "vst1.8     {q1}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1"  
  );
}

void YUY2ToUV422Row_NEON(const uint8* src_yuy2, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.8     {d1}, [%1]!                    \n"  
    "vst1.8     {d3}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3"  
  );
}

void UYVYToUV422Row_NEON(const uint8* src_uyvy, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.8     {d0}, [%1]!                    \n"  
    "vst1.8     {d2}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3"  
  );
}

void YUY2ToUVRow_NEON(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %4, %4, #16                    \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "vrhadd.u8  d1, d1, d5                     \n"  
    "vrhadd.u8  d3, d3, d7                     \n"  
    "vst1.8     {d1}, [%2]!                    \n"  
    "vst1.8     {d3}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),     
    "+r"(stride_yuy2),  
    "+r"(dst_u),        
    "+r"(dst_v),        
    "+r"(pix)           
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"  
  );
}

void UYVYToUVRow_NEON(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %4, %4, #16                    \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "vrhadd.u8  d0, d0, d4                     \n"  
    "vrhadd.u8  d2, d2, d6                     \n"  
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d2}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),     
    "+r"(stride_uyvy),  
    "+r"(dst_u),        
    "+r"(dst_v),        
    "+r"(pix)           
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"  
  );
}

void HalfRow_NEON(const uint8* src_uv, int src_uv_stride,
                  uint8* dst_uv, int pix) {
  asm volatile (
    
    "add        %1, %0                         \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %3, %3, #16                    \n"  
    "vld1.8     {q1}, [%1]!                    \n"  
    "vrhadd.u8  q0, q1                         \n"  
    "vst1.8     {q0}, [%2]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_uv),         
    "+r"(src_uv_stride),  
    "+r"(dst_uv),         
    "+r"(pix)             
  :
  : "cc", "memory", "q0", "q1"  
  );
}


void ARGBToBayerRow_NEON(const uint8* src_argb, uint8* dst_bayer,
                         uint32 selector, int pix) {
  asm volatile (
    "vmov.u32   d6[0], %3                      \n"  
  "1:                                          \n"
    "vld1.8     {q0, q1}, [%0]!                \n"  
    "subs       %2, %2, #8                     \n"  
    "vtbl.8     d4, {d0, d1}, d6               \n"  
    "vtbl.8     d5, {d2, d3}, d6               \n"  
    "vtrn.u32   d4, d5                         \n"  
    "vst1.8     {d4}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_bayer),  
    "+r"(pix)         
  : "r"(selector)     
  : "cc", "memory", "q0", "q1", "q2", "q3"  
  );
}


void ARGBToBayerGGRow_NEON(const uint8* src_argb, uint8* dst_bayer,
                           uint32 , int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vst1.8     {d1}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_bayer),  
    "+r"(pix)         
  :
  : "cc", "memory", "q0", "q1"  
  );
}


void ARGBShuffleRow_NEON(const uint8* src_argb, uint8* dst_argb,
                         const uint8* shuffler, int pix) {
  asm volatile (
    "vld1.8     {q2}, [%3]                     \n"  
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #4                     \n"  
    "vtbl.8     d2, {d0, d1}, d4               \n"  
    "vtbl.8     d3, {d0, d1}, d5               \n"  
    "vst1.8     {q1}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "r"(shuffler)    
  : "cc", "memory", "q0", "q1", "q2"  
  );
}

void I422ToYUY2Row_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_yuy2, int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {d0, d2}, [%0]!                \n"  
    "vld1.8     {d1}, [%1]!                    \n"  
    "vld1.8     {d3}, [%2]!                    \n"  
    "subs       %4, %4, #16                    \n"  
    "vst4.8     {d0, d1, d2, d3}, [%3]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_y),     
    "+r"(src_u),     
    "+r"(src_v),     
    "+r"(dst_yuy2),  
    "+r"(width)      
  :
  : "cc", "memory", "d0", "d1", "d2", "d3"
  );
}

void I422ToUYVYRow_NEON(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_uyvy, int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld2.8     {d1, d3}, [%0]!                \n"  
    "vld1.8     {d0}, [%1]!                    \n"  
    "vld1.8     {d2}, [%2]!                    \n"  
    "subs       %4, %4, #16                    \n"  
    "vst4.8     {d0, d1, d2, d3}, [%3]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_y),     
    "+r"(src_u),     
    "+r"(src_v),     
    "+r"(dst_uyvy),  
    "+r"(width)      
  :
  : "cc", "memory", "d0", "d1", "d2", "d3"
  );
}

void ARGBToRGB565Row_NEON(const uint8* src_argb, uint8* dst_rgb565, int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d20, d21, d22, d23}, [%0]!    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGBTORGB565
    "vst1.8     {q0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_rgb565),  
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q8", "q9", "q10", "q11"
  );
}

void ARGBToARGB1555Row_NEON(const uint8* src_argb, uint8* dst_argb1555,
                            int pix) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d20, d21, d22, d23}, [%0]!    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGBTOARGB1555
    "vst1.8     {q0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_argb1555),  
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q8", "q9", "q10", "q11"
  );
}

void ARGBToARGB4444Row_NEON(const uint8* src_argb, uint8* dst_argb4444,
                            int pix) {
  asm volatile (
    "vmov.u8    d4, #0x0f                      \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d20, d21, d22, d23}, [%0]!    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGBTOARGB4444
    "vst1.8     {q0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),      
    "+r"(dst_argb4444),  
    "+r"(pix)            
  :
  : "cc", "memory", "q0", "q8", "q9", "q10", "q11"
  );
}

void ARGBToYRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #13                       \n"  
    "vmov.u8    d25, #65                       \n"  
    "vmov.u8    d26, #33                       \n"  
    "vmov.u8    d27, #16                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vqadd.u8   d0, d27                        \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q12", "q13"
  );
}

void ARGBToYJRow_NEON(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #15                       \n"  
    "vmov.u8    d25, #75                       \n"  
    "vmov.u8    d26, #38                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q12", "q13"
  );
}


void ARGBToUV444Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    "vmov.u8    d24, #112                      \n"  
    "vmov.u8    d25, #74                       \n"  
    "vmov.u8    d26, #38                       \n"  
    "vmov.u8    d27, #18                       \n"  
    "vmov.u8    d28, #94                       \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlsl.u8   q2, d1, d25                    \n"  
    "vmlsl.u8   q2, d2, d26                    \n"  
    "vadd.u16   q2, q2, q15                    \n"  

    "vmull.u8   q3, d2, d24                    \n"  
    "vmlsl.u8   q3, d1, d28                    \n"  
    "vmlsl.u8   q3, d0, d27                    \n"  
    "vadd.u16   q3, q3, q15                    \n"  

    "vqshrn.u16  d0, q2, #8                    \n"  
    "vqshrn.u16  d1, q3, #8                    \n"  

    "vst1.8     {d0}, [%1]!                    \n"  
    "vst1.8     {d1}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q12", "q13", "q14", "q15"
  );
}


void ARGBToUV422Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  

    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  

    "subs       %3, %3, #16                    \n"  
    "vmul.s16   q8, q0, q10                    \n"  
    "vmls.s16   q8, q1, q11                    \n"  
    "vmls.s16   q8, q2, q12                    \n"  
    "vadd.u16   q8, q8, q15                    \n"  

    "vmul.s16   q9, q2, q10                    \n"  
    "vmls.s16   q9, q1, q14                    \n"  
    "vmls.s16   q9, q0, q13                    \n"  
    "vadd.u16   q9, q9, q15                    \n"  

    "vqshrn.u16  d0, q8, #8                    \n"  
    "vqshrn.u16  d1, q9, #8                    \n"  

    "vst1.8     {d0}, [%1]!                    \n"  
    "vst1.8     {d1}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void ARGBToUV411Row_NEON(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%0]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%0]!     \n"  
    "vpaddl.u8  q4, q4                         \n"  
    "vpaddl.u8  q5, q5                         \n"  
    "vpaddl.u8  q6, q6                         \n"  

    "vpadd.u16  d0, d0, d1                     \n"  
    "vpadd.u16  d1, d8, d9                     \n"  
    "vpadd.u16  d2, d2, d3                     \n"  
    "vpadd.u16  d3, d10, d11                   \n"  
    "vpadd.u16  d4, d4, d5                     \n"  
    "vpadd.u16  d5, d12, d13                   \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %3, %3, #32                    \n"  
    "vmul.s16   q8, q0, q10                    \n"  
    "vmls.s16   q8, q1, q11                    \n"  
    "vmls.s16   q8, q2, q12                    \n"  
    "vadd.u16   q8, q8, q15                    \n"  
    "vmul.s16   q9, q2, q10                    \n"  
    "vmls.s16   q9, q1, q14                    \n"  
    "vmls.s16   q9, q0, q13                    \n"  
    "vadd.u16   q9, q9, q15                    \n"  
    "vqshrn.u16  d0, q8, #8                    \n"  
    "vqshrn.u16  d1, q9, #8                    \n"  
    "vst1.8     {d0}, [%1]!                    \n"  
    "vst1.8     {d1}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


#define RGBTOUV(QB, QG, QR) \
    "vmul.s16   q8, " #QB ", q10               \n"  /* B                    */ \
    "vmls.s16   q8, " #QG ", q11               \n"  /* G                    */ \
    "vmls.s16   q8, " #QR ", q12               \n"  /* R                    */ \
    "vadd.u16   q8, q8, q15                    \n"  /* +128 -> unsigned     */ \
    "vmul.s16   q9, " #QR ", q10               \n"  /* R                    */ \
    "vmls.s16   q9, " #QG ", q14               \n"  /* G                    */ \
    "vmls.s16   q9, " #QB ", q13               \n"  /* B                    */ \
    "vadd.u16   q9, q9, q15                    \n"  /* +128 -> unsigned     */ \
    "vqshrn.u16  d0, q8, #8                    \n"  /* 16 bit to 8 bit U    */ \
    "vqshrn.u16  d1, q9, #8                    \n"  /* 16 bit to 8 bit V    */


void ARGBToUVRow_NEON(const uint8* src_argb, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  
    "vpadal.u8  q0, q4                         \n"  
    "vpadal.u8  q1, q5                         \n"  
    "vpadal.u8  q2, q6                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q0, q1, q2)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(src_stride_argb),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void ARGBToUVJRow_NEON(const uint8* src_argb, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #127 / 2                  \n"  
    "vmov.s16   q11, #84 / 2                   \n"  
    "vmov.s16   q12, #43 / 2                   \n"  
    "vmov.s16   q13, #20 / 2                   \n"  
    "vmov.s16   q14, #107 / 2                  \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  
    "vpadal.u8  q0, q4                         \n"  
    "vpadal.u8  q1, q5                         \n"  
    "vpadal.u8  q2, q6                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q0, q1, q2)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(src_stride_argb),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void BGRAToUVRow_NEON(const uint8* src_bgra, int src_stride_bgra,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q3, q3                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  
    "vpadal.u8  q3, q7                         \n"  
    "vpadal.u8  q2, q6                         \n"  
    "vpadal.u8  q1, q5                         \n"  

    "vrshr.u16  q1, q1, #1                     \n"  
    "vrshr.u16  q2, q2, #1                     \n"
    "vrshr.u16  q3, q3, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q3, q2, q1)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_bgra),  
    "+r"(src_stride_bgra),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void ABGRToUVRow_NEON(const uint8* src_abgr, int src_stride_abgr,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  
    "vpadal.u8  q2, q6                         \n"  
    "vpadal.u8  q1, q5                         \n"  
    "vpadal.u8  q0, q4                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q2, q1, q0)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_abgr),  
    "+r"(src_stride_abgr),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void RGBAToUVRow_NEON(const uint8* src_rgba, int src_stride_rgba,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "vpaddl.u8  q0, q1                         \n"  
    "vpaddl.u8  q1, q2                         \n"  
    "vpaddl.u8  q2, q3                         \n"  
    "vld4.8     {d8, d10, d12, d14}, [%1]!     \n"  
    "vld4.8     {d9, d11, d13, d15}, [%1]!     \n"  
    "vpadal.u8  q0, q5                         \n"  
    "vpadal.u8  q1, q6                         \n"  
    "vpadal.u8  q2, q7                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q0, q1, q2)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgba),  
    "+r"(src_stride_rgba),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void RGB24ToUVRow_NEON(const uint8* src_rgb24, int src_stride_rgb24,
                       uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d0, d2, d4}, [%0]!            \n"  
    "vld3.8     {d1, d3, d5}, [%0]!            \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vld3.8     {d8, d10, d12}, [%1]!          \n"  
    "vld3.8     {d9, d11, d13}, [%1]!          \n"  
    "vpadal.u8  q0, q4                         \n"  
    "vpadal.u8  q1, q5                         \n"  
    "vpadal.u8  q2, q6                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q0, q1, q2)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb24),  
    "+r"(src_stride_rgb24),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void RAWToUVRow_NEON(const uint8* src_raw, int src_stride_raw,
                     uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d0, d2, d4}, [%0]!            \n"  
    "vld3.8     {d1, d3, d5}, [%0]!            \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vld3.8     {d8, d10, d12}, [%1]!          \n"  
    "vld3.8     {d9, d11, d13}, [%1]!          \n"  
    "vpadal.u8  q2, q6                         \n"  
    "vpadal.u8  q1, q5                         \n"  
    "vpadal.u8  q0, q4                         \n"  

    "vrshr.u16  q0, q0, #1                     \n"  
    "vrshr.u16  q1, q1, #1                     \n"
    "vrshr.u16  q2, q2, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    RGBTOUV(q2, q1, q0)
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_raw),  
    "+r"(src_stride_raw),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void RGB565ToUVRow_NEON(const uint8* src_rgb565, int src_stride_rgb565,
                        uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    RGB565TOARGB
    "vpaddl.u8  d8, d0                         \n"  
    "vpaddl.u8  d10, d1                        \n"  
    "vpaddl.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%0]!                    \n"  
    RGB565TOARGB
    "vpaddl.u8  d9, d0                         \n"  
    "vpaddl.u8  d11, d1                        \n"  
    "vpaddl.u8  d13, d2                        \n"  

    "vld1.8     {q0}, [%1]!                    \n"  
    RGB565TOARGB
    "vpadal.u8  d8, d0                         \n"  
    "vpadal.u8  d10, d1                        \n"  
    "vpadal.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%1]!                    \n"  
    RGB565TOARGB
    "vpadal.u8  d9, d0                         \n"  
    "vpadal.u8  d11, d1                        \n"  
    "vpadal.u8  d13, d2                        \n"  

    "vrshr.u16  q4, q4, #1                     \n"  
    "vrshr.u16  q5, q5, #1                     \n"
    "vrshr.u16  q6, q6, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    "vmul.s16   q8, q4, q10                    \n"  
    "vmls.s16   q8, q5, q11                    \n"  
    "vmls.s16   q8, q6, q12                    \n"  
    "vadd.u16   q8, q8, q15                    \n"  
    "vmul.s16   q9, q6, q10                    \n"  
    "vmls.s16   q9, q5, q14                    \n"  
    "vmls.s16   q9, q4, q13                    \n"  
    "vadd.u16   q9, q9, q15                    \n"  
    "vqshrn.u16  d0, q8, #8                    \n"  
    "vqshrn.u16  d1, q9, #8                    \n"  
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb565),  
    "+r"(src_stride_rgb565),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void ARGB1555ToUVRow_NEON(const uint8* src_argb1555, int src_stride_argb1555,
                        uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    RGB555TOARGB
    "vpaddl.u8  d8, d0                         \n"  
    "vpaddl.u8  d10, d1                        \n"  
    "vpaddl.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%0]!                    \n"  
    RGB555TOARGB
    "vpaddl.u8  d9, d0                         \n"  
    "vpaddl.u8  d11, d1                        \n"  
    "vpaddl.u8  d13, d2                        \n"  

    "vld1.8     {q0}, [%1]!                    \n"  
    RGB555TOARGB
    "vpadal.u8  d8, d0                         \n"  
    "vpadal.u8  d10, d1                        \n"  
    "vpadal.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%1]!                    \n"  
    RGB555TOARGB
    "vpadal.u8  d9, d0                         \n"  
    "vpadal.u8  d11, d1                        \n"  
    "vpadal.u8  d13, d2                        \n"  

    "vrshr.u16  q4, q4, #1                     \n"  
    "vrshr.u16  q5, q5, #1                     \n"
    "vrshr.u16  q6, q6, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    "vmul.s16   q8, q4, q10                    \n"  
    "vmls.s16   q8, q5, q11                    \n"  
    "vmls.s16   q8, q6, q12                    \n"  
    "vadd.u16   q8, q8, q15                    \n"  
    "vmul.s16   q9, q6, q10                    \n"  
    "vmls.s16   q9, q5, q14                    \n"  
    "vmls.s16   q9, q4, q13                    \n"  
    "vadd.u16   q9, q9, q15                    \n"  
    "vqshrn.u16  d0, q8, #8                    \n"  
    "vqshrn.u16  d1, q9, #8                    \n"  
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb1555),  
    "+r"(src_stride_argb1555),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


void ARGB4444ToUVRow_NEON(const uint8* src_argb4444, int src_stride_argb4444,
                          uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "add        %1, %0, %1                     \n"  
    "vmov.s16   q10, #112 / 2                  \n"  
    "vmov.s16   q11, #74 / 2                   \n"  
    "vmov.s16   q12, #38 / 2                   \n"  
    "vmov.s16   q13, #18 / 2                   \n"  
    "vmov.s16   q14, #94 / 2                   \n"  
    "vmov.u16   q15, #0x8080                   \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    ARGB4444TOARGB
    "vpaddl.u8  d8, d0                         \n"  
    "vpaddl.u8  d10, d1                        \n"  
    "vpaddl.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%0]!                    \n"  
    ARGB4444TOARGB
    "vpaddl.u8  d9, d0                         \n"  
    "vpaddl.u8  d11, d1                        \n"  
    "vpaddl.u8  d13, d2                        \n"  

    "vld1.8     {q0}, [%1]!                    \n"  
    ARGB4444TOARGB
    "vpadal.u8  d8, d0                         \n"  
    "vpadal.u8  d10, d1                        \n"  
    "vpadal.u8  d12, d2                        \n"  
    "vld1.8     {q0}, [%1]!                    \n"  
    ARGB4444TOARGB
    "vpadal.u8  d9, d0                         \n"  
    "vpadal.u8  d11, d1                        \n"  
    "vpadal.u8  d13, d2                        \n"  

    "vrshr.u16  q4, q4, #1                     \n"  
    "vrshr.u16  q5, q5, #1                     \n"
    "vrshr.u16  q6, q6, #1                     \n"

    "subs       %4, %4, #16                    \n"  
    "vmul.s16   q8, q4, q10                    \n"  
    "vmls.s16   q8, q5, q11                    \n"  
    "vmls.s16   q8, q6, q12                    \n"  
    "vadd.u16   q8, q8, q15                    \n"  
    "vmul.s16   q9, q6, q10                    \n"  
    "vmls.s16   q9, q5, q14                    \n"  
    "vmls.s16   q9, q4, q13                    \n"  
    "vadd.u16   q9, q9, q15                    \n"  
    "vqshrn.u16  d0, q8, #8                    \n"  
    "vqshrn.u16  d1, q9, #8                    \n"  
    "vst1.8     {d0}, [%2]!                    \n"  
    "vst1.8     {d1}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb4444),  
    "+r"(src_stride_argb4444),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7",
    "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
  );
}

void RGB565ToYRow_NEON(const uint8* src_rgb565, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #13                       \n"  
    "vmov.u8    d25, #65                       \n"  
    "vmov.u8    d26, #33                       \n"  
    "vmov.u8    d27, #16                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    RGB565TOARGB
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vqadd.u8   d0, d27                        \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb565),  
    "+r"(dst_y),       
    "+r"(pix)          
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q12", "q13"
  );
}

void ARGB1555ToYRow_NEON(const uint8* src_argb1555, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #13                       \n"  
    "vmov.u8    d25, #65                       \n"  
    "vmov.u8    d26, #33                       \n"  
    "vmov.u8    d27, #16                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGB1555TOARGB
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vqadd.u8   d0, d27                        \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb1555),  
    "+r"(dst_y),         
    "+r"(pix)            
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q12", "q13"
  );
}

void ARGB4444ToYRow_NEON(const uint8* src_argb4444, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d24, #13                       \n"  
    "vmov.u8    d25, #65                       \n"  
    "vmov.u8    d26, #33                       \n"  
    "vmov.u8    d27, #16                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "subs       %2, %2, #8                     \n"  
    ARGB4444TOARGB
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vqadd.u8   d0, d27                        \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb4444),  
    "+r"(dst_y),         
    "+r"(pix)            
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q12", "q13"
  );
}

void BGRAToYRow_NEON(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #33                        \n"  
    "vmov.u8    d5, #65                        \n"  
    "vmov.u8    d6, #13                        \n"  
    "vmov.u8    d7, #16                        \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q8, d1, d4                     \n"  
    "vmlal.u8   q8, d2, d5                     \n"  
    "vmlal.u8   q8, d3, d6                     \n"  
    "vqrshrun.s16 d0, q8, #7                   \n"  
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_bgra),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}

void ABGRToYRow_NEON(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #33                        \n"  
    "vmov.u8    d5, #65                        \n"  
    "vmov.u8    d6, #13                        \n"  
    "vmov.u8    d7, #16                        \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q8, d0, d4                     \n"  
    "vmlal.u8   q8, d1, d5                     \n"  
    "vmlal.u8   q8, d2, d6                     \n"  
    "vqrshrun.s16 d0, q8, #7                   \n"  
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_abgr),  
    "+r"(dst_y),  
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}

void RGBAToYRow_NEON(const uint8* src_rgba, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #13                        \n"  
    "vmov.u8    d5, #65                        \n"  
    "vmov.u8    d6, #33                        \n"  
    "vmov.u8    d7, #16                        \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q8, d1, d4                     \n"  
    "vmlal.u8   q8, d2, d5                     \n"  
    "vmlal.u8   q8, d3, d6                     \n"  
    "vqrshrun.s16 d0, q8, #7                   \n"  
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgba),  
    "+r"(dst_y),  
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}

void RGB24ToYRow_NEON(const uint8* src_rgb24, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #13                        \n"  
    "vmov.u8    d5, #65                        \n"  
    "vmov.u8    d6, #33                        \n"  
    "vmov.u8    d7, #16                        \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d0, d1, d2}, [%0]!            \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q8, d0, d4                     \n"  
    "vmlal.u8   q8, d1, d5                     \n"  
    "vmlal.u8   q8, d2, d6                     \n"  
    "vqrshrun.s16 d0, q8, #7                   \n"  
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb24),  
    "+r"(dst_y),  
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}

void RAWToYRow_NEON(const uint8* src_raw, uint8* dst_y, int pix) {
  asm volatile (
    "vmov.u8    d4, #33                        \n"  
    "vmov.u8    d5, #65                        \n"  
    "vmov.u8    d6, #13                        \n"  
    "vmov.u8    d7, #16                        \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld3.8     {d0, d1, d2}, [%0]!            \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q8, d0, d4                     \n"  
    "vmlal.u8   q8, d1, d5                     \n"  
    "vmlal.u8   q8, d2, d6                     \n"  
    "vqrshrun.s16 d0, q8, #7                   \n"  
    "vqadd.u8   d0, d7                         \n"
    "vst1.8     {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_raw),  
    "+r"(dst_y),  
    "+r"(pix)        
  :
  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "q8"
  );
}


void InterpolateRow_NEON(uint8* dst_ptr,
                         const uint8* src_ptr, ptrdiff_t src_stride,
                         int dst_width, int source_y_fraction) {
  asm volatile (
    "cmp        %4, #0                         \n"
    "beq        100f                           \n"
    "add        %2, %1                         \n"
    "cmp        %4, #64                        \n"
    "beq        75f                            \n"
    "cmp        %4, #128                       \n"
    "beq        50f                            \n"
    "cmp        %4, #192                       \n"
    "beq        25f                            \n"

    "vdup.8     d5, %4                         \n"
    "rsb        %4, #256                       \n"
    "vdup.8     d4, %4                         \n"
    
  "1:                                          \n"
    "vld1.8     {q0}, [%1]!                    \n"
    "vld1.8     {q1}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"
    "vmull.u8   q13, d0, d4                    \n"
    "vmull.u8   q14, d1, d4                    \n"
    "vmlal.u8   q13, d2, d5                    \n"
    "vmlal.u8   q14, d3, d5                    \n"
    "vrshrn.u16 d0, q13, #8                    \n"
    "vrshrn.u16 d1, q14, #8                    \n"
    "vst1.8     {q0}, [%0]!                    \n"
    "bgt        1b                             \n"
    "b          99f                            \n"

    
  "25:                                         \n"
    "vld1.8     {q0}, [%1]!                    \n"
    "vld1.8     {q1}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"
    "vrhadd.u8  q0, q1                         \n"
    "vrhadd.u8  q0, q1                         \n"
    "vst1.8     {q0}, [%0]!                    \n"
    "bgt        25b                            \n"
    "b          99f                            \n"

    
  "50:                                         \n"
    "vld1.8     {q0}, [%1]!                    \n"
    "vld1.8     {q1}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"
    "vrhadd.u8  q0, q1                         \n"
    "vst1.8     {q0}, [%0]!                    \n"
    "bgt        50b                            \n"
    "b          99f                            \n"

    
  "75:                                         \n"
    "vld1.8     {q1}, [%1]!                    \n"
    "vld1.8     {q0}, [%2]!                    \n"
    "subs       %3, %3, #16                    \n"
    "vrhadd.u8  q0, q1                         \n"
    "vrhadd.u8  q0, q1                         \n"
    "vst1.8     {q0}, [%0]!                    \n"
    "bgt        75b                            \n"
    "b          99f                            \n"

    
  "100:                                        \n"
    "vld1.8     {q0}, [%1]!                    \n"
    "subs       %3, %3, #16                    \n"
    "vst1.8     {q0}, [%0]!                    \n"
    "bgt        100b                           \n"

  "99:                                         \n"
  : "+r"(dst_ptr),          
    "+r"(src_ptr),          
    "+r"(src_stride),       
    "+r"(dst_width),        
    "+r"(source_y_fraction) 
  :
  : "cc", "memory", "q0", "q1", "d4", "d5", "q13", "q14"
  );
}


void ARGBBlendRow_NEON(const uint8* src_argb0, const uint8* src_argb1,
                       uint8* dst_argb, int width) {
  asm volatile (
    "subs       %3, #8                         \n"
    "blt        89f                            \n"
    
  "8:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vmull.u8   q10, d4, d3                    \n"  
    "vmull.u8   q11, d5, d3                    \n"  
    "vmull.u8   q12, d6, d3                    \n"  
    "vqrshrn.u16 d20, q10, #8                  \n"  
    "vqrshrn.u16 d21, q11, #8                  \n"  
    "vqrshrn.u16 d22, q12, #8                  \n"  
    "vqsub.u8   q2, q2, q10                    \n"  
    "vqsub.u8   d6, d6, d22                    \n"  
    "vqadd.u8   q0, q0, q2                     \n"  
    "vqadd.u8   d2, d2, d6                     \n"  
    "vmov.u8    d3, #255                       \n"  
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bge        8b                             \n"

  "89:                                         \n"
    "adds       %3, #8-1                       \n"
    "blt        99f                            \n"

    
  "1:                                          \n"
    "vld4.8     {d0[0],d1[0],d2[0],d3[0]}, [%0]! \n"  
    "vld4.8     {d4[0],d5[0],d6[0],d7[0]}, [%1]! \n"  
    "subs       %3, %3, #1                     \n"  
    "vmull.u8   q10, d4, d3                    \n"  
    "vmull.u8   q11, d5, d3                    \n"  
    "vmull.u8   q12, d6, d3                    \n"  
    "vqrshrn.u16 d20, q10, #8                  \n"  
    "vqrshrn.u16 d21, q11, #8                  \n"  
    "vqrshrn.u16 d22, q12, #8                  \n"  
    "vqsub.u8   q2, q2, q10                    \n"  
    "vqsub.u8   d6, d6, d22                    \n"  
    "vqadd.u8   q0, q0, q2                     \n"  
    "vqadd.u8   d2, d2, d6                     \n"  
    "vmov.u8    d3, #255                       \n"  
    "vst4.8     {d0[0],d1[0],d2[0],d3[0]}, [%2]! \n"  
    "bge        1b                             \n"

  "99:                                         \n"

  : "+r"(src_argb0),    
    "+r"(src_argb1),    
    "+r"(dst_argb),     
    "+r"(width)         
  :
  : "cc", "memory", "q0", "q1", "q2", "q3", "q10", "q11", "q12"
  );
}


void ARGBAttenuateRow_NEON(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q10, d0, d3                    \n"  
    "vmull.u8   q11, d1, d3                    \n"  
    "vmull.u8   q12, d2, d3                    \n"  
    "vqrshrn.u16 d0, q10, #8                   \n"  
    "vqrshrn.u16 d1, q11, #8                   \n"  
    "vqrshrn.u16 d2, q12, #8                   \n"  
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "cc", "memory", "q0", "q1", "q10", "q11", "q12"
  );
}



void ARGBQuantizeRow_NEON(uint8* dst_argb, int scale, int interval_size,
                          int interval_offset, int width) {
  asm volatile (
    "vdup.u16   q8, %2                         \n"
    "vshr.u16   q8, q8, #1                     \n"  
    "vdup.u16   q9, %3                         \n"  
    "vdup.u16   q10, %4                        \n"  

    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]         \n"  
    "subs       %1, %1, #8                     \n"  
    "vmovl.u8   q0, d0                         \n"  
    "vmovl.u8   q1, d2                         \n"
    "vmovl.u8   q2, d4                         \n"
    "vqdmulh.s16 q0, q0, q8                    \n"  
    "vqdmulh.s16 q1, q1, q8                    \n"  
    "vqdmulh.s16 q2, q2, q8                    \n"  
    "vmul.u16   q0, q0, q9                     \n"  
    "vmul.u16   q1, q1, q9                     \n"  
    "vmul.u16   q2, q2, q9                     \n"  
    "vadd.u16   q0, q0, q10                    \n"  
    "vadd.u16   q1, q1, q10                    \n"  
    "vadd.u16   q2, q2, q10                    \n"  
    "vqmovn.u16 d0, q0                         \n"
    "vqmovn.u16 d2, q1                         \n"
    "vqmovn.u16 d4, q2                         \n"
    "vst4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "bgt        1b                             \n"
  : "+r"(dst_argb),       
    "+r"(width)           
  : "r"(scale),           
    "r"(interval_size),   
    "r"(interval_offset)  
  : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9", "q10"
  );
}




void ARGBShadeRow_NEON(const uint8* src_argb, uint8* dst_argb, int width,
                       uint32 value) {
  asm volatile (
    "vdup.u32   q0, %3                         \n"  
    "vzip.u8    d0, d1                         \n"  
    "vshr.u16   q0, q0, #1                     \n"  

    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d20, d22, d24, d26}, [%0]!    \n"  
    "subs       %2, %2, #8                     \n"  
    "vmovl.u8   q10, d20                       \n"  
    "vmovl.u8   q11, d22                       \n"
    "vmovl.u8   q12, d24                       \n"
    "vmovl.u8   q13, d26                       \n"
    "vqrdmulh.s16 q10, q10, d0[0]              \n"  
    "vqrdmulh.s16 q11, q11, d0[1]              \n"  
    "vqrdmulh.s16 q12, q12, d0[2]              \n"  
    "vqrdmulh.s16 q13, q13, d0[3]              \n"  
    "vqmovn.u16 d20, q10                       \n"
    "vqmovn.u16 d22, q11                       \n"
    "vqmovn.u16 d24, q12                       \n"
    "vqmovn.u16 d26, q13                       \n"
    "vst4.8     {d20, d22, d24, d26}, [%1]!    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),       
    "+r"(dst_argb),       
    "+r"(width)           
  : "r"(value)            
  : "cc", "memory", "q0", "q10", "q11", "q12", "q13"
  );
}




void ARGBGrayRow_NEON(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "vmov.u8    d24, #15                       \n"  
    "vmov.u8    d25, #75                       \n"  
    "vmov.u8    d26, #38                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmull.u8   q2, d0, d24                    \n"  
    "vmlal.u8   q2, d1, d25                    \n"  
    "vmlal.u8   q2, d2, d26                    \n"  
    "vqrshrun.s16 d0, q2, #7                   \n"  
    "vmov       d1, d0                         \n"  
    "vmov       d2, d0                         \n"  
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(width)      
  :
  : "cc", "memory", "q0", "q1", "q2", "q12", "q13"
  );
}





void ARGBSepiaRow_NEON(uint8* dst_argb, int width) {
  asm volatile (
    "vmov.u8    d20, #17                       \n"  
    "vmov.u8    d21, #68                       \n"  
    "vmov.u8    d22, #35                       \n"  
    "vmov.u8    d24, #22                       \n"  
    "vmov.u8    d25, #88                       \n"  
    "vmov.u8    d26, #45                       \n"  
    "vmov.u8    d28, #24                       \n"  
    "vmov.u8    d29, #98                       \n"  
    "vmov.u8    d30, #50                       \n"  
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]         \n"  
    "subs       %1, %1, #8                     \n"  
    "vmull.u8   q2, d0, d20                    \n"  
    "vmlal.u8   q2, d1, d21                    \n"  
    "vmlal.u8   q2, d2, d22                    \n"  
    "vmull.u8   q3, d0, d24                    \n"  
    "vmlal.u8   q3, d1, d25                    \n"  
    "vmlal.u8   q3, d2, d26                    \n"  
    "vmull.u8   q8, d0, d28                    \n"  
    "vmlal.u8   q8, d1, d29                    \n"  
    "vmlal.u8   q8, d2, d30                    \n"  
    "vqshrn.u16 d0, q2, #7                     \n"  
    "vqshrn.u16 d1, q3, #7                     \n"  
    "vqshrn.u16 d2, q8, #7                     \n"  
    "vst4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "bgt        1b                             \n"
  : "+r"(dst_argb),  
    "+r"(width)      
  :
  : "cc", "memory", "q0", "q1", "q2", "q3",
    "q10", "q11", "q12", "q13", "q14", "q15"
  );
}




void ARGBColorMatrixRow_NEON(const uint8* src_argb, uint8* dst_argb,
                             const int8* matrix_argb, int width) {
  asm volatile (
    "vld1.8     {q2}, [%3]                     \n"  
    "vmovl.s8   q0, d4                         \n"  
    "vmovl.s8   q1, d5                         \n"  

    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d16, d18, d20, d22}, [%0]!    \n"  
    "subs       %2, %2, #8                     \n"  
    "vmovl.u8   q8, d16                        \n"  
    "vmovl.u8   q9, d18                        \n"  
    "vmovl.u8   q10, d20                       \n"  
    "vmovl.u8   q15, d22                       \n"  
    "vmul.s16   q12, q8, d0[0]                 \n"  
    "vmul.s16   q13, q8, d1[0]                 \n"  
    "vmul.s16   q14, q8, d2[0]                 \n"  
    "vmul.s16   q15, q8, d3[0]                 \n"  
    "vmul.s16   q4, q9, d0[1]                  \n"  
    "vmul.s16   q5, q9, d1[1]                  \n"  
    "vmul.s16   q6, q9, d2[1]                  \n"  
    "vmul.s16   q7, q9, d3[1]                  \n"  
    "vqadd.s16  q12, q12, q4                   \n"  
    "vqadd.s16  q13, q13, q5                   \n"  
    "vqadd.s16  q14, q14, q6                   \n"  
    "vqadd.s16  q15, q15, q7                   \n"  
    "vmul.s16   q4, q10, d0[2]                 \n"  
    "vmul.s16   q5, q10, d1[2]                 \n"  
    "vmul.s16   q6, q10, d2[2]                 \n"  
    "vmul.s16   q7, q10, d3[2]                 \n"  
    "vqadd.s16  q12, q12, q4                   \n"  
    "vqadd.s16  q13, q13, q5                   \n"  
    "vqadd.s16  q14, q14, q6                   \n"  
    "vqadd.s16  q15, q15, q7                   \n"  
    "vmul.s16   q4, q15, d0[3]                 \n"  
    "vmul.s16   q5, q15, d1[3]                 \n"  
    "vmul.s16   q6, q15, d2[3]                 \n"  
    "vmul.s16   q7, q15, d3[3]                 \n"  
    "vqadd.s16  q12, q12, q4                   \n"  
    "vqadd.s16  q13, q13, q5                   \n"  
    "vqadd.s16  q14, q14, q6                   \n"  
    "vqadd.s16  q15, q15, q7                   \n"  
    "vqshrun.s16 d16, q12, #6                  \n"  
    "vqshrun.s16 d18, q13, #6                  \n"  
    "vqshrun.s16 d20, q14, #6                  \n"  
    "vqshrun.s16 d22, q15, #6                  \n"  
    "vst4.8     {d16, d18, d20, d22}, [%1]!    \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_argb),   
    "+r"(width)       
  : "r"(matrix_argb)  
  : "cc", "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9",
    "q10", "q11", "q12", "q13", "q14", "q15"
  );
}


#ifdef HAS_ARGBMULTIPLYROW_NEON

void ARGBMultiplyRow_NEON(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  asm volatile (
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%1]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vmull.u8   q0, d0, d1                     \n"  
    "vmull.u8   q1, d2, d3                     \n"  
    "vmull.u8   q2, d4, d5                     \n"  
    "vmull.u8   q3, d6, d7                     \n"  
    "vrshrn.u16 d0, q0, #8                     \n"  
    "vrshrn.u16 d1, q1, #8                     \n"  
    "vrshrn.u16 d2, q2, #8                     \n"  
    "vrshrn.u16 d3, q3, #8                     \n"  
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bgt        1b                             \n"

  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "cc", "memory", "q0", "q1", "q2", "q3"
  );
}
#endif  


void ARGBAddRow_NEON(const uint8* src_argb0, const uint8* src_argb1,
                     uint8* dst_argb, int width) {
  asm volatile (
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vqadd.u8   q0, q0, q2                     \n"  
    "vqadd.u8   q1, q1, q3                     \n"  
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bgt        1b                             \n"

  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "cc", "memory", "q0", "q1", "q2", "q3"
  );
}


void ARGBSubtractRow_NEON(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  asm volatile (
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vqsub.u8   q0, q0, q2                     \n"  
    "vqsub.u8   q1, q1, q3                     \n"  
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bgt        1b                             \n"

  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "cc", "memory", "q0", "q1", "q2", "q3"
  );
}






void SobelRow_NEON(const uint8* src_sobelx, const uint8* src_sobely,
                     uint8* dst_argb, int width) {
  asm volatile (
    "vmov.u8    d3, #255                       \n"  
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0}, [%0]!                    \n"  
    "vld1.8     {d1}, [%1]!                    \n"  
    "subs       %3, %3, #8                     \n"  
    "vqadd.u8   d0, d0, d1                     \n"  
    "vmov.u8    d1, d0                         \n"
    "vmov.u8    d2, d0                         \n"
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_argb),    
    "+r"(width)        
  :
  : "cc", "memory", "q0", "q1"
  );
}


void SobelToPlaneRow_NEON(const uint8* src_sobelx, const uint8* src_sobely,
                          uint8* dst_y, int width) {
  asm volatile (
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"  
    "vld1.8     {q1}, [%1]!                    \n"  
    "subs       %3, %3, #16                    \n"  
    "vqadd.u8   q0, q0, q1                     \n"  
    "vst1.8     {q0}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_y),       
    "+r"(width)        
  :
  : "cc", "memory", "q0", "q1"
  );
}






void SobelXYRow_NEON(const uint8* src_sobelx, const uint8* src_sobely,
                     uint8* dst_argb, int width) {
  asm volatile (
    "vmov.u8    d3, #255                       \n"  
    
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d2}, [%0]!                    \n"  
    "vld1.8     {d0}, [%1]!                    \n"  
    "subs       %3, %3, #8                     \n"  
    "vqadd.u8   d1, d0, d2                     \n"  
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_argb),    
    "+r"(width)        
  :
  : "cc", "memory", "q0", "q1"
  );
}





void SobelXRow_NEON(const uint8* src_y0, const uint8* src_y1,
                    const uint8* src_y2, uint8* dst_sobelx, int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0}, [%0],%5                  \n"  
    "vld1.8     {d1}, [%0],%6                  \n"
    "vsubl.u8   q0, d0, d1                     \n"
    "vld1.8     {d2}, [%1],%5                  \n"  
    "vld1.8     {d3}, [%1],%6                  \n"
    "vsubl.u8   q1, d2, d3                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vld1.8     {d2}, [%2],%5                  \n"  
    "vld1.8     {d3}, [%2],%6                  \n"
    "subs       %4, %4, #8                     \n"  
    "vsubl.u8   q1, d2, d3                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vabs.s16   q0, q0                         \n"
    "vqmovn.u16 d0, q0                         \n"
    "vst1.8     {d0}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_y0),      
    "+r"(src_y1),      
    "+r"(src_y2),      
    "+r"(dst_sobelx),  
    "+r"(width)        
  : "r"(2),            
    "r"(6)             
  : "cc", "memory", "q0", "q1"  
  );
}





void SobelYRow_NEON(const uint8* src_y0, const uint8* src_y1,
                    uint8* dst_sobely, int width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0}, [%0],%4                  \n"  
    "vld1.8     {d1}, [%1],%4                  \n"
    "vsubl.u8   q0, d0, d1                     \n"
    "vld1.8     {d2}, [%0],%4                  \n"  
    "vld1.8     {d3}, [%1],%4                  \n"
    "vsubl.u8   q1, d2, d3                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vld1.8     {d2}, [%0],%5                  \n"  
    "vld1.8     {d3}, [%1],%5                  \n"
    "subs       %3, %3, #8                     \n"  
    "vsubl.u8   q1, d2, d3                     \n"
    "vadd.s16   q0, q0, q1                     \n"
    "vabs.s16   q0, q0                         \n"
    "vqmovn.u16 d0, q0                         \n"
    "vst1.8     {d0}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_y0),      
    "+r"(src_y1),      
    "+r"(dst_sobely),  
    "+r"(width)        
  : "r"(1),            
    "r"(6)             
  : "cc", "memory", "q0", "q1"  
  );
}
#endif  

#ifdef __cplusplus
}  
}  
#endif
