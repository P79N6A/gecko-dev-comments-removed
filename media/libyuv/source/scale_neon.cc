









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_NEON) && defined(__ARM_NEON__)





void ScaleRowDown2_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst, int dst_width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    
    "vld2.8     {q0, q1}, [%0]!                \n"
    "subs       %2, %2, #16                    \n"  
    "vst1.8     {q1}, [%1]!                    \n"  
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(dst),              
    "+r"(dst_width)         
  :
  : "q0", "q1"              
  );
}


void ScaleRowDown2Box_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst, int dst_width) {
  asm volatile (
    
    "add        %1, %0                         \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0, q1}, [%0]!                \n"  
    "vld1.8     {q2, q3}, [%1]!                \n"  
    "subs       %3, %3, #16                    \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"
    "vpadal.u8  q0, q2                         \n"  
    "vpadal.u8  q1, q3                         \n"
    "vrshrn.u16 d0, q0, #2                     \n"  
    "vrshrn.u16 d1, q1, #2                     \n"
    "vst1.8     {q0}, [%2]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(src_stride),       
    "+r"(dst),              
    "+r"(dst_width)         
  :
  : "q0", "q1", "q2", "q3"     
  );
}

void ScaleRowDown4_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!        \n" 
    "subs       %2, %2, #8                     \n" 
    "vst1.8     {d2}, [%1]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width)         
  :
  : "q0", "q1", "memory", "cc"
  );
}

void ScaleRowDown4Box_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  asm volatile (
    "add        r4, %0, %3                     \n"
    "add        r5, r4, %3                     \n"
    "add        %3, r5, %3                     \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {q0}, [%0]!                    \n"   
    "vld1.8     {q1}, [r4]!                    \n"
    "vld1.8     {q2}, [r5]!                    \n"
    "vld1.8     {q3}, [%3]!                    \n"
    "subs       %2, %2, #4                     \n"
    "vpaddl.u8  q0, q0                         \n"
    "vpadal.u8  q0, q1                         \n"
    "vpadal.u8  q0, q2                         \n"
    "vpadal.u8  q0, q3                         \n"
    "vpaddl.u16 q0, q0                         \n"
    "vrshrn.u32 d0, q0, #4                     \n"   
    "vmovn.u16  d0, q0                         \n"
    "vst1.32    {d0[0]}, [%1]!                 \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width)         
  : "r"(src_stride)         
  : "r4", "r5", "q0", "q1", "q2", "q3", "memory", "cc"
  );
}




void ScaleRowDown34_NEON(const uint8* src_ptr,
                         ptrdiff_t src_stride,
                         uint8* dst_ptr, int dst_width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d1, d2, d3}, [%0]!      \n" 
    "subs       %2, %2, #24                  \n"
    "vmov       d2, d3                       \n" 
    "vst3.8     {d0, d1, d2}, [%1]!          \n"
    "bgt        1b                           \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width)         
  :
  : "d0", "d1", "d2", "d3", "memory", "cc"
  );
}

void ScaleRowDown34_0_Box_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vmov.u8    d24, #3                        \n"
    "add        %3, %0                         \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8       {d0, d1, d2, d3}, [%0]!      \n" 
    "vld4.8       {d4, d5, d6, d7}, [%3]!      \n" 
    "subs         %2, %2, #24                  \n"

    
    
    
    "vmovl.u8     q8, d4                       \n"
    "vmovl.u8     q9, d5                       \n"
    "vmovl.u8     q10, d6                      \n"
    "vmovl.u8     q11, d7                      \n"

    
    "vmlal.u8     q8, d0, d24                  \n"
    "vmlal.u8     q9, d1, d24                  \n"
    "vmlal.u8     q10, d2, d24                 \n"
    "vmlal.u8     q11, d3, d24                 \n"

    
    "vqrshrn.u16  d0, q8, #2                   \n"
    "vqrshrn.u16  d1, q9, #2                   \n"
    "vqrshrn.u16  d2, q10, #2                  \n"
    "vqrshrn.u16  d3, q11, #2                  \n"

    
    "vmovl.u8     q8, d1                       \n"
    "vmlal.u8     q8, d0, d24                  \n"
    "vqrshrn.u16  d0, q8, #2                   \n"

    
    "vrhadd.u8    d1, d1, d2                   \n"

    
    "vmovl.u8     q8, d2                       \n"
    "vmlal.u8     q8, d3, d24                  \n"
    "vqrshrn.u16  d2, q8, #2                   \n"

    "vst3.8       {d0, d1, d2}, [%1]!          \n"

    "bgt          1b                           \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width),        
    "+r"(src_stride)        
  :
  : "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11", "d24", "memory", "cc"
  );
}

void ScaleRowDown34_1_Box_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vmov.u8    d24, #3                        \n"
    "add        %3, %0                         \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8       {d0, d1, d2, d3}, [%0]!      \n" 
    "vld4.8       {d4, d5, d6, d7}, [%3]!      \n" 
    "subs         %2, %2, #24                  \n"
    
    "vrhadd.u8    q0, q0, q2                   \n"
    "vrhadd.u8    q1, q1, q3                   \n"

    
    "vmovl.u8     q3, d1                       \n"
    "vmlal.u8     q3, d0, d24                  \n"
    "vqrshrn.u16  d0, q3, #2                   \n"

    
    "vrhadd.u8    d1, d1, d2                   \n"

    
    "vmovl.u8     q3, d2                       \n"
    "vmlal.u8     q3, d3, d24                  \n"
    "vqrshrn.u16  d2, q3, #2                   \n"

    "vst3.8       {d0, d1, d2}, [%1]!          \n"
    "bgt          1b                           \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width),        
    "+r"(src_stride)        
  :
  : "r4", "q0", "q1", "q2", "q3", "d24", "memory", "cc"
  );
}

#define HAS_SCALEROWDOWN38_NEON
static uvec8 kShuf38 =
  { 0, 3, 6, 8, 11, 14, 16, 19, 22, 24, 27, 30, 0, 0, 0, 0 };
static uvec8 kShuf38_2 =
  { 0, 8, 16, 2, 10, 17, 4, 12, 18, 6, 14, 19, 0, 0, 0, 0 };
static vec16 kMult38_Div6 =
  { 65536 / 12, 65536 / 12, 65536 / 12, 65536 / 12,
    65536 / 12, 65536 / 12, 65536 / 12, 65536 / 12 };
static vec16 kMult38_Div9 =
  { 65536 / 18, 65536 / 18, 65536 / 18, 65536 / 18,
    65536 / 18, 65536 / 18, 65536 / 18, 65536 / 18 };


void ScaleRowDown38_NEON(const uint8* src_ptr,
                         ptrdiff_t src_stride,
                         uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.8     {q3}, [%3]                     \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0, d1, d2, d3}, [%0]!        \n"
    "subs       %2, %2, #12                    \n"
    "vtbl.u8    d4, {d0, d1, d2, d3}, d6       \n"
    "vtbl.u8    d5, {d0, d1, d2, d3}, d7       \n"
    "vst1.8     {d4}, [%1]!                    \n"
    "vst1.32    {d5[0]}, [%1]!                 \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width)         
  : "r"(&kShuf38)           
  : "d0", "d1", "d2", "d3", "d4", "d5", "memory", "cc"
  );
}


void OMITFP ScaleRowDown38_3_Box_NEON(const uint8* src_ptr,
                                      ptrdiff_t src_stride,
                                      uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.16    {q13}, [%4]                    \n"
    "vld1.8     {q14}, [%5]                    \n"
    "vld1.8     {q15}, [%6]                    \n"
    "add        r4, %0, %3, lsl #1             \n"
    "add        %3, %0                         \n"
    ".p2align   2                              \n"
  "1:                                          \n"

    
    
    
    
    "vld4.8       {d0, d1, d2, d3}, [%0]!      \n"
    "vld4.8       {d4, d5, d6, d7}, [%3]!      \n"
    "vld4.8       {d16, d17, d18, d19}, [r4]!  \n"
    "subs         %2, %2, #12                  \n"

    
    
    
    
    "vtrn.u8      d0, d1                       \n"
    "vtrn.u8      d4, d5                       \n"
    "vtrn.u8      d16, d17                     \n"

    
    
    "vtrn.u8      d2, d3                       \n"
    "vtrn.u8      d6, d7                       \n"
    "vtrn.u8      d18, d19                     \n"

    
    
    "vpaddl.u8    q0, q0                       \n"
    "vpaddl.u8    q2, q2                       \n"
    "vpaddl.u8    q8, q8                       \n"

    
    "vpaddl.u8    d3, d3                       \n"
    "vpaddl.u8    d7, d7                       \n"
    "vpaddl.u8    d19, d19                     \n"

    
    "vadd.u16     q0, q2                       \n"
    "vadd.u16     q0, q8                       \n"
    "vadd.u16     d4, d3, d7                   \n"
    "vadd.u16     d4, d19                      \n"

    
    
    
    "vqrdmulh.s16 q2, q2, q13                  \n"
    "vmovn.u16    d4, q2                       \n"

    
    
    
    
    
    
    "vmovl.u8     q1, d2                       \n"
    "vmovl.u8     q3, d6                       \n"
    "vmovl.u8     q9, d18                      \n"

    
    "vadd.u16     q1, q3                       \n"
    "vadd.u16     q1, q9                       \n"

    
    
    "vtrn.u32     d2, d3                       \n"

    
    
    "vtrn.u16     d2, d3                       \n"

    
    "vadd.u16     q0, q1                       \n"

    
    
    
    "vqrdmulh.s16 q0, q0, q15                  \n"

    
    
    "vmov.u8      d2, d4                       \n"

    "vtbl.u8      d3, {d0, d1, d2}, d28        \n"
    "vtbl.u8      d4, {d0, d1, d2}, d29        \n"

    "vst1.8       {d3}, [%1]!                  \n"
    "vst1.32      {d4[0]}, [%1]!               \n"
    "bgt          1b                           \n"
  : "+r"(src_ptr),          
    "+r"(dst_ptr),          
    "+r"(dst_width),        
    "+r"(src_stride)        
  : "r"(&kMult38_Div6),     
    "r"(&kShuf38_2),        
    "r"(&kMult38_Div9)      
  : "r4", "q0", "q1", "q2", "q3", "q8", "q9",
    "q13", "q14", "q15", "memory", "cc"
  );
}


void ScaleRowDown38_2_Box_NEON(const uint8* src_ptr,
                               ptrdiff_t src_stride,
                               uint8* dst_ptr, int dst_width) {
  asm volatile (
    "vld1.16    {q13}, [%4]                    \n"
    "vld1.8     {q14}, [%5]                    \n"
    "add        %3, %0                         \n"
    ".p2align   2                              \n"
  "1:                                          \n"

    
    
    
    
    "vld4.8       {d0, d1, d2, d3}, [%0]!      \n"
    "vld4.8       {d4, d5, d6, d7}, [%3]!      \n"
    "subs         %2, %2, #12                  \n"

    
    
    
    
    "vtrn.u8      d0, d1                       \n"
    "vtrn.u8      d4, d5                       \n"

    
    
    "vtrn.u8      d2, d3                       \n"
    "vtrn.u8      d6, d7                       \n"

    
    
    "vpaddl.u8    q0, q0                       \n"
    "vpaddl.u8    q2, q2                       \n"

    
    "vpaddl.u8    d3, d3                       \n"
    "vpaddl.u8    d7, d7                       \n"

    
    "vadd.u16     q0, q2                       \n"
    "vadd.u16     d4, d3, d7                   \n"

    
    "vqrshrn.u16  d4, q2, #2                   \n"

    
    
    
    
    
    
    "vmovl.u8     q1, d2                       \n"
    "vmovl.u8     q3, d6                       \n"

    
    "vadd.u16     q1, q3                       \n"

    
    
    "vtrn.u32     d2, d3                       \n"

    
    
    "vtrn.u16     d2, d3                       \n"

    
    "vadd.u16     q0, q1                       \n"

    
    
    
    "vqrdmulh.s16 q0, q0, q13                  \n"

    
    
    "vmov.u8      d2, d4                       \n"

    "vtbl.u8      d3, {d0, d1, d2}, d28        \n"
    "vtbl.u8      d4, {d0, d1, d2}, d29        \n"

    "vst1.8       {d3}, [%1]!                  \n"
    "vst1.32      {d4[0]}, [%1]!               \n"
    "bgt          1b                           \n"
  : "+r"(src_ptr),       
    "+r"(dst_ptr),       
    "+r"(dst_width),     
    "+r"(src_stride)     
  : "r"(&kMult38_Div6),  
    "r"(&kShuf38_2)      
  : "q0", "q1", "q2", "q3", "q13", "q14", "memory", "cc"
  );
}


void ScaleFilterRows_NEON(uint8* dst_ptr,
                          const uint8* src_ptr, ptrdiff_t src_stride,
                          int dst_width, int source_y_fraction) {
  asm volatile (
    "cmp          %4, #0                       \n"
    "beq          100f                         \n"
    "add          %2, %1                       \n"
    "cmp          %4, #64                      \n"
    "beq          75f                          \n"
    "cmp          %4, #128                     \n"
    "beq          50f                          \n"
    "cmp          %4, #192                     \n"
    "beq          25f                          \n"

    "vdup.8       d5, %4                       \n"
    "rsb          %4, #256                     \n"
    "vdup.8       d4, %4                       \n"
    
  "1:                                          \n"
    "vld1.8       {q0}, [%1]!                  \n"
    "vld1.8       {q1}, [%2]!                  \n"
    "subs         %3, %3, #16                  \n"
    "vmull.u8     q13, d0, d4                  \n"
    "vmull.u8     q14, d1, d4                  \n"
    "vmlal.u8     q13, d2, d5                  \n"
    "vmlal.u8     q14, d3, d5                  \n"
    "vrshrn.u16   d0, q13, #8                  \n"
    "vrshrn.u16   d1, q14, #8                  \n"
    "vst1.8       {q0}, [%0]!                  \n"
    "bgt          1b                           \n"
    "b            99f                          \n"

    
  "25:                                         \n"
    "vld1.8       {q0}, [%1]!                  \n"
    "vld1.8       {q1}, [%2]!                  \n"
    "subs         %3, %3, #16                  \n"
    "vrhadd.u8    q0, q1                       \n"
    "vrhadd.u8    q0, q1                       \n"
    "vst1.8       {q0}, [%0]!                  \n"
    "bgt          25b                          \n"
    "b            99f                          \n"

    
  "50:                                         \n"
    "vld1.8       {q0}, [%1]!                  \n"
    "vld1.8       {q1}, [%2]!                  \n"
    "subs         %3, %3, #16                  \n"
    "vrhadd.u8    q0, q1                       \n"
    "vst1.8       {q0}, [%0]!                  \n"
    "bgt          50b                          \n"
    "b            99f                          \n"

    
  "75:                                         \n"
    "vld1.8       {q1}, [%1]!                  \n"
    "vld1.8       {q0}, [%2]!                  \n"
    "subs         %3, %3, #16                  \n"
    "vrhadd.u8    q0, q1                       \n"
    "vrhadd.u8    q0, q1                       \n"
    "vst1.8       {q0}, [%0]!                  \n"
    "bgt          75b                          \n"
    "b            99f                          \n"

    
  "100:                                        \n"
    "vld1.8       {q0}, [%1]!                  \n"
    "subs         %3, %3, #16                  \n"
    "vst1.8       {q0}, [%0]!                  \n"
    "bgt          100b                         \n"

  "99:                                         \n"
    "vst1.8       {d1[7]}, [%0]                \n"
  : "+r"(dst_ptr),          
    "+r"(src_ptr),          
    "+r"(src_stride),       
    "+r"(dst_width),        
    "+r"(source_y_fraction) 
  :
  : "q0", "q1", "d4", "d5", "q13", "q14", "memory", "cc"
  );
}

void ScaleARGBRowDown2_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                            uint8* dst, int dst_width) {
  asm volatile (
    ".p2align   2                              \n"
  "1:                                          \n"
    
    "vld2.32    {q0, q1}, [%0]!                \n"
    "vld2.32    {q2, q3}, [%0]!                \n"
    "subs       %2, %2, #8                     \n"  
    "vst1.8     {q1}, [%1]!                    \n"  
    "vst1.8     {q3}, [%1]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(dst),              
    "+r"(dst_width)         
  :
  : "memory", "cc", "q0", "q1", "q2", "q3"  
  );
}

void ScaleARGBRowDown2Box_NEON(const uint8* src_ptr, ptrdiff_t src_stride,
                               uint8* dst, int dst_width) {
  asm volatile (
    
    "add        %1, %1, %0                     \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld4.8     {d0, d2, d4, d6}, [%0]!        \n"  
    "vld4.8     {d1, d3, d5, d7}, [%0]!        \n"  
    "subs       %3, %3, #8                     \n"  
    "vpaddl.u8  q0, q0                         \n"  
    "vpaddl.u8  q1, q1                         \n"  
    "vpaddl.u8  q2, q2                         \n"  
    "vpaddl.u8  q3, q3                         \n"  
    "vld4.8     {d16, d18, d20, d22}, [%1]!    \n"  
    "vld4.8     {d17, d19, d21, d23}, [%1]!    \n"  
    "vpadal.u8  q0, q8                         \n"  
    "vpadal.u8  q1, q9                         \n"  
    "vpadal.u8  q2, q10                        \n"  
    "vpadal.u8  q3, q11                        \n"  
    "vrshrn.u16 d0, q0, #2                     \n"  
    "vrshrn.u16 d1, q1, #2                     \n"
    "vrshrn.u16 d2, q2, #2                     \n"
    "vrshrn.u16 d3, q3, #2                     \n"
    "vst4.8     {d0, d1, d2, d3}, [%2]!        \n"
    "bgt        1b                             \n"
  : "+r"(src_ptr),          
    "+r"(src_stride),       
    "+r"(dst),              
    "+r"(dst_width)         
  :
  : "memory", "cc", "q0", "q1", "q2", "q3", "q8", "q9", "q10", "q11"
  );
}



void ScaleARGBRowDownEven_NEON(const uint8* src_argb,  ptrdiff_t src_stride,
                               int src_stepx, uint8* dst_argb, int dst_width) {
  asm volatile (
    "mov        r12, %3, lsl #2                \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.32    {d0[0]}, [%0], r12             \n"
    "vld1.32    {d0[1]}, [%0], r12             \n"
    "vld1.32    {d1[0]}, [%0], r12             \n"
    "vld1.32    {d1[1]}, [%0], r12             \n"
    "subs       %2, %2, #4                     \n"  
    "vst1.8     {q0}, [%1]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+r"(dst_width)    
  : "r"(src_stepx)     
  : "memory", "cc", "r12", "q0"
  );
}



void ScaleARGBRowDownEvenBox_NEON(const uint8* src_argb, ptrdiff_t src_stride,
                                  int src_stepx,
                                  uint8* dst_argb, int dst_width) {
  asm volatile (
    "mov        r12, %4, lsl #2                \n"
    "add        %1, %1, %0                     \n"
    ".p2align   2                              \n"
  "1:                                          \n"
    "vld1.8     {d0}, [%0], r12                \n"  
    "vld1.8     {d1}, [%1], r12                \n"
    "vld1.8     {d2}, [%0], r12                \n"
    "vld1.8     {d3}, [%1], r12                \n"
    "vld1.8     {d4}, [%0], r12                \n"
    "vld1.8     {d5}, [%1], r12                \n"
    "vld1.8     {d6}, [%0], r12                \n"
    "vld1.8     {d7}, [%1], r12                \n"
    "vaddl.u8   q0, d0, d1                     \n"
    "vaddl.u8   q1, d2, d3                     \n"
    "vaddl.u8   q2, d4, d5                     \n"
    "vaddl.u8   q3, d6, d7                     \n"
    "vswp.8     d1, d2                         \n"  
    "vswp.8     d5, d6                         \n"  
    "vadd.u16   q0, q0, q1                     \n"  
    "vadd.u16   q2, q2, q3                     \n"  
    "vrshrn.u16 d0, q0, #2                     \n"  
    "vrshrn.u16 d1, q2, #2                     \n"  
    "subs       %3, %3, #4                     \n"  
    "vst1.8     {q0}, [%2]!                    \n"
    "bgt        1b                             \n"
  : "+r"(src_argb),    
    "+r"(src_stride),  
    "+r"(dst_argb),    
    "+r"(dst_width)    
  : "r"(src_stepx)     
  : "memory", "cc", "r12", "q0", "q1", "q2", "q3"
  );
}

#endif  

#ifdef __cplusplus
}  
}  
#endif
