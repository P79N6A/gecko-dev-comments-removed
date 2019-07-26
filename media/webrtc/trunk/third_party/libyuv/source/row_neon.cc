









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(YUV_DISABLE_ASM) && defined(__ARM_NEON__)



#define YUV422TORGB                                                            \
    "vld1.u8    {d0}, [%0]!                    \n"                             \
    "vld1.u32   {d2[0]}, [%1]!                 \n"                             \
    "vld1.u32   {d2[1]}, [%2]!                 \n"                             \
    "veor.u8    d2, d26                        \n"/*subtract 128 from u and v*/\
    "vmull.s8   q8, d2, d24                    \n"/*  u/v B/R component      */\
    "vmull.s8   q9, d2, d25                    \n"/*  u/v G component        */\
    "vmov.u8    d1, #0                         \n"/*  split odd/even y apart */\
    "vtrn.u8    d0, d1                         \n"                             \
    "vsub.s16   q0, q0, q15                    \n"/*  offset y               */\
    "vmul.s16   q0, q0, q14                    \n"                             \
    "vadd.s16   d18, d19                       \n"                             \
    "vqadd.s16  d20, d0, d16                   \n"                             \
    "vqadd.s16  d21, d1, d16                   \n"                             \
    "vqadd.s16  d22, d0, d17                   \n"                             \
    "vqadd.s16  d23, d1, d17                   \n"                             \
    "vqadd.s16  d16, d0, d18                   \n"                             \
    "vqadd.s16  d17, d1, d18                   \n"                             \
    "vqrshrun.s16 d0, q10, #6                  \n"                             \
    "vqrshrun.s16 d1, q11, #6                  \n"                             \
    "vqrshrun.s16 d2, q8, #6                   \n"                             \
    "vmovl.u8   q10, d0                        \n"/*  set up for reinterleave*/\
    "vmovl.u8   q11, d1                        \n"                             \
    "vmovl.u8   q8, d2                         \n"                             \
    "vtrn.u8    d20, d21                       \n"                             \
    "vtrn.u8    d22, d23                       \n"                             \
    "vtrn.u8    d16, d17                       \n"                             \

#if defined(HAS_I422TOARGBROW_NEON) || defined(HAS_I422TOBGRAROW_NEON) ||      \
    defined(HAS_I422TOABGRROW_NEON) || defined(HAS_I422TORGBAROW_NEON)
static const vec8 kUVToRB  = { 127, 127, 127, 127, 102, 102, 102, 102,
                               0, 0, 0, 0, 0, 0, 0, 0 };
static const vec8 kUVToG = { -25, -25, -25, -25, -52, -52, -52, -52,
                             0, 0, 0, 0, 0, 0, 0, 0 };
#endif

#ifdef HAS_I422TOARGBROW_NEON
void I422ToARGBRow_NEON(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
    YUV422TORGB
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bgt        1b                             \n"
    : "+r"(y_buf),    
      "+r"(u_buf),    
      "+r"(v_buf),    
      "+r"(rgb_buf),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}
#endif  

#ifdef HAS_I422TOBGRAROW_NEON
void I422ToBGRARow_NEON(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
    YUV422TORGB
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d19, #255                      \n"
    "vst4.8     {d19, d20, d21, d22}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bgt        1b                             \n"
    : "+r"(y_buf),    
      "+r"(u_buf),    
      "+r"(v_buf),    
      "+r"(rgb_buf),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}
#endif  

#ifdef HAS_I422TOABGRROW_NEON
void I422ToABGRRow_NEON(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
    YUV422TORGB
    "vswp.u8    d20, d22                       \n"
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d23, #255                      \n"
    "vst4.8     {d20, d21, d22, d23}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bgt        1b                             \n"
    : "+r"(y_buf),    
      "+r"(u_buf),    
      "+r"(v_buf),    
      "+r"(rgb_buf),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}
#endif  

#ifdef HAS_I422TORGBAROW_NEON
void I422ToRGBARow_NEON(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width) {
  asm volatile (
    "vld1.u8    {d24}, [%5]                    \n"
    "vld1.u8    {d25}, [%6]                    \n"
    "vmov.u8    d26, #128                      \n"
    "vmov.u16   q14, #74                       \n"
    "vmov.u16   q15, #16                       \n"
  "1:                                          \n"
    YUV422TORGB
    "vmov.u8    d21, d16                       \n"
    "vmov.u8    d19, #255                      \n"
    "vst4.8     {d19, d20, d21, d22}, [%3]!    \n"
    "subs       %4, %4, #8                     \n"
    "bgt        1b                             \n"
    : "+r"(y_buf),    
      "+r"(u_buf),    
      "+r"(v_buf),    
      "+r"(rgb_buf),  
      "+r"(width)     
    : "r"(&kUVToRB),  
      "r"(&kUVToG)    
    : "cc", "memory", "q0", "q1", "q2", "q3", "q8", "q9",
                      "q10", "q11", "q12", "q13", "q14", "q15"
  );
}
#endif  

#ifdef HAS_SPLITUV_NEON


void SplitUV_NEON(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
  "1:                                          \n"
    "vld2.u8    {q0, q1}, [%0]!                \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.u8    {q0}, [%1]!                    \n"  
    "vst1.u8    {q1}, [%2]!                    \n"  
    "bgt        1b                             \n"
    : "+r"(src_uv),  
      "+r"(dst_u),   
      "+r"(dst_v),   
      "+r"(width)    
    :                       
    : "memory", "cc", "q0", "q1"  
  );
}
#endif  

#ifdef HAS_COPYROW_NEON

void CopyRow_NEON(const uint8* src, uint8* dst, int count) {
  asm volatile (
  "1:                                          \n"
    "pld        [%0, #0xC0]                    \n"  
    "vldm       %0!,{q0, q1, q2, q3}              \n"  
    "subs       %2, %2, #64                    \n"  
    "vstm       %1!,{q0, q1, q2, q3}              \n"  
    "bgt        1b                             \n"
    : "+r"(src),   
      "+r"(dst),   
      "+r"(count)  
    :                     
    : "memory", "cc", "q0", "q1", "q2", "q3"  
  );
}
#endif  

#ifdef HAS_MIRRORROW_NEON
void MirrorRow_NEON(const uint8* src, uint8* dst, int width) {
  asm volatile (
    
    "add         %1, %2                        \n"
    
    "lsrs        r3, %2, #4                    \n"
    
    
    
    
    
    
    "mov         r3, #-24                      \n"
    "beq         2f                            \n"

    
    
    "sub         %1, #16                       \n"
    
    
    
    
    "sub         %2, #16                       \n"

    
    
    
    
    "1:                                        \n"
      "vld1.8      {q0}, [%0]!                 \n"  
      "vrev64.8    q0, q0                      \n"
      "vst1.8      {d1}, [%1]!                 \n"
      "vst1.8      {d0}, [%1], r3              \n"  
      "subs        %2, #16                     \n"
    "bge         1b                            \n"

    
    
    "adds        %2, #16                       \n"
    "beq         5f                            \n"
    "add         %1, #16                       \n"
  "2:                                          \n"
    "mov         r3, #-3                       \n"
    "sub         %1, #2                        \n"
    "subs        %2, #2                        \n"
    
    
    "blt         4f                            \n"



  "3:                                          \n"
    "vld2.8      {d0[0], d1[0]}, [%0]!         \n"  
    "vst1.8      {d1[0]}, [%1]!                \n"
    "vst1.8      {d0[0]}, [%1], r3             \n"  
    "subs        %2, #2                        \n"
    "bge         3b                            \n"

    "adds        %2, #2                        \n"
    "beq         5f                            \n"
  "4:                                          \n"
    "add         %1, #1                        \n"
    "vld1.8      {d0[0]}, [%0]                 \n"
    "vst1.8      {d0[0]}, [%1]                 \n"
  "5:                                          \n"
    : "+r"(src),   
      "+r"(dst),   
      "+r"(width)  
    :
    : "memory", "cc", "r3", "q0"
  );
}
#endif  

#ifdef HAS_MIRRORROWUV_NEON
void MirrorRowUV_NEON(const uint8* src, uint8* dst_a, uint8* dst_b, int width) {
  asm volatile (
    
    "add         %1, %3                        \n"  
    "add         %2, %3                        \n"  
    
    
    
    "lsrs        r12, %3, #3                   \n"
    "beq         2f                            \n"
    
    "mov         r12, #-8                      \n"
    
    
    "sub         %1, #8                        \n"
    "sub         %2, #8                        \n"
    
    
    
    
    "sub         %3, #8                        \n"

    
    "1:                                        \n"
      "vld2.8      {d0, d1}, [%0]!             \n"  
      "vrev64.8    q0, q0                      \n"
      "vst1.8      {d0}, [%1], r12             \n"  
      "vst1.8      {d1}, [%2], r12             \n"  
      "subs        %3, #8                      \n"
      "bge         1b                          \n"

    
    
    "adds        %3, #8                        \n"
    "beq         4f                            \n"
    "add         %1, #8                        \n"
    "add         %2, #8                        \n"
  "2:                                          \n"
    "mov         r12, #-1                      \n"
    "sub         %1, #1                        \n"
    "sub         %2, #1                        \n"
  "3:                                          \n"
      "vld2.8      {d0[0], d1[0]}, [%0]!       \n"  
      "vst1.8      {d0[0]}, [%1], r12          \n"  
      "vst1.8      {d1[0]}, [%2], r12          \n"  
      "subs        %3, %3, #1                  \n"
      "bgt         3b                          \n"
  "4:                                          \n"
    : "+r"(src),    
      "+r"(dst_a),  
      "+r"(dst_b),  
      "+r"(width)   
    :
    : "memory", "cc", "r12", "q0"
  );
}
#endif  

#ifdef HAS_BGRATOARGBROW_NEON
void BGRAToARGBRow_NEON(const uint8* src_bgra, uint8* dst_argb, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vswp.u8    d1, d2                         \n"  
    "vswp.u8    d0, d3                         \n"  
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_bgra),  
    "+r"(dst_argb),  
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3"  
  );
}
#endif  

#ifdef HAS_ABGRTOARGBROW_NEON
void ABGRToARGBRow_NEON(const uint8* src_abgr, uint8* dst_argb, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vswp.u8    d0, d2                         \n"  
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_abgr),  
    "+r"(dst_argb),  
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3"  
  );
}
#endif  

#ifdef HAS_RGBATOARGBROW_NEON
void RGBAToARGBRow_NEON(const uint8* src_rgba, uint8* dst_argb, int pix) {
  asm volatile (
  "1:                                           \n"
    "vld1.8     {d0, d1, d2, d3}, [%0]!         \n"  
    "subs       %2, %2, #8                      \n"  
    "vmov.u8    d4, d0                          \n"  
    "vst4.8     {d1, d2, d3, d4}, [%1]!         \n"  
    "bgt        1b                              \n"
  : "+r"(src_rgba),  
    "+r"(dst_argb),  
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_RGB24TOARGBROW_NEON
void RGB24ToARGBRow_NEON(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  asm volatile (
    "vmov.u8    d4, #255                       \n"  
  "1:                                          \n"
    "vld3.8     {d1, d2, d3}, [%0]!            \n"  
    "subs       %2, %2, #8                     \n"  
    "vst4.8     {d1, d2, d3, d4}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_rgb24),  
    "+r"(dst_argb),   
    "+r"(pix)         
  :
  : "memory", "cc", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_RAWTOARGBROW_NEON
void RAWToARGBRow_NEON(const uint8* src_raw, uint8* dst_argb, int pix) {
  asm volatile (
    "vmov.u8    d4, #255                       \n"  
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
  : "memory", "cc", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_ARGBTORGBAROW_NEON
void ARGBToRGBARow_NEON(const uint8* src_argb, uint8* dst_rgba, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d1, d2, d3, d4}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vmov.u8    d0, d4                         \n"  
    "vst4.8     {d0, d1, d2, d3}, [%1]!        \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_rgba),  
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_ARGBTORGB24ROW_NEON
void ARGBToRGB24Row_NEON(const uint8* src_argb, uint8* dst_rgb24, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d1, d2, d3, d4}, [%0]!        \n"  
    "subs       %2, %2, #8                     \n"  
    "vst3.8     {d1, d2, d3}, [%1]!            \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),   
    "+r"(dst_rgb24),  
    "+r"(pix)         
  :
  : "memory", "cc", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_ARGBTORAWROW_NEON
void ARGBToRAWRow_NEON(const uint8* src_argb, uint8* dst_raw, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d1, d2, d3, d4}, [%0]!        \n"  
    "vswp.u8    d1, d3                         \n"  
    "subs       %2, %2, #8                     \n"  
    "vst3.8     {d1, d2, d3}, [%1]!            \n"  
    "bgt        1b                             \n"
  : "+r"(src_argb),  
    "+r"(dst_raw),   
    "+r"(pix)        
  :
  : "memory", "cc", "d1", "d2", "d3", "d4"  
  );
}
#endif  

#ifdef HAS_YUY2TOYROW_NEON
void YUY2ToYRow_NEON(const uint8* src_yuy2, uint8* dst_y, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld2.u8    {d0, d1}, [%0]!                \n"  
    "subs       %2, %2, #8                     \n"  
    "vst1.u8    {d0}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1"  
  );
}
#endif  

#ifdef HAS_UYVYTOYROW_NEON
void UYVYToYRow_NEON(const uint8* src_uyvy, uint8* dst_y, int pix) {
  asm volatile (
  "1:                                          \n"
    "vld2.u8    {d0, d1}, [%0]!                \n"  
    "subs       %2, %2, #8                     \n"  
    "vst1.u8    {d1}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1"  
  );
}
#endif  

#ifdef HAS_YUY2TOYROW_NEON
void YUY2ToUV422Row_NEON(const uint8* src_yuy2, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.u8    {d1}, [%1]!                    \n"  
    "vst1.u8    {d3}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3"  
  );
}
#endif  

#ifdef HAS_UYVYTOYROW_NEON
void UYVYToUV422Row_NEON(const uint8* src_uyvy, uint8* dst_u, uint8* dst_v,
                         int pix) {
  asm volatile (
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vst1.u8    {d0}, [%1]!                    \n"  
    "vst1.u8    {d2}, [%2]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3"  
  );
}
#endif  

#ifdef HAS_YUY2TOYROW_NEON
void YUY2ToUVRow_NEON(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "adds       %1, %0, %1                     \n"  
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vrhadd.u8  d1, d1, d5                     \n"  
    "vrhadd.u8  d3, d3, d7                     \n"  
    "vst1.u8    {d1}, [%2]!                    \n"  
    "vst1.u8    {d3}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_yuy2),  
    "+r"(stride_yuy2),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"  
  );
}
#endif  

#ifdef HAS_UYVYTOYROW_NEON
void UYVYToUVRow_NEON(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "adds       %1, %0, %1                     \n"  
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n"  
    "vld4.8     {d4, d5, d6, d7}, [%1]!        \n"  
    "subs       %3, %3, #16                    \n"  
    "vrhadd.u8  d0, d0, d4                     \n"  
    "vrhadd.u8  d2, d2, d6                     \n"  
    "vst1.u8    {d0}, [%2]!                    \n"  
    "vst1.u8    {d2}, [%3]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_uyvy),  
    "+r"(stride_uyvy),  
    "+r"(dst_u),     
    "+r"(dst_v),     
    "+r"(pix)        
  :
  : "memory", "cc", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"  
  );
}
#endif  

#endif  

#ifdef __cplusplus
}  
}  
#endif
