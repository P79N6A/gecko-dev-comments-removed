









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) || defined(__i386__))


static uvec8 kShuf0 =
  { 0, 1, 3, 4, 5, 7, 8, 9, 128, 128, 128, 128, 128, 128, 128, 128 };


static uvec8 kShuf1 =
  { 3, 4, 5, 7, 8, 9, 11, 12, 128, 128, 128, 128, 128, 128, 128, 128 };


static uvec8 kShuf2 =
  { 5, 7, 8, 9, 11, 12, 13, 15, 128, 128, 128, 128, 128, 128, 128, 128 };


static uvec8 kShuf01 =
  { 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10 };


static uvec8 kShuf11 =
  { 2, 3, 4, 5, 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 12, 13 };


static uvec8 kShuf21 =
  { 5, 6, 6, 7, 8, 9, 9, 10, 10, 11, 12, 13, 13, 14, 14, 15 };


static uvec8 kMadd01 =
  { 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2 };


static uvec8 kMadd11 =
  { 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1 };


static uvec8 kMadd21 =
  { 2, 2, 1, 3, 3, 1, 2, 2, 1, 3, 3, 1, 2, 2, 1, 3 };


static vec16 kRound34 =
  { 2, 2, 2, 2, 2, 2, 2, 2 };

static uvec8 kShuf38a =
  { 0, 3, 6, 8, 11, 14, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 };

static uvec8 kShuf38b =
  { 128, 128, 128, 128, 128, 128, 0, 3, 6, 8, 11, 14, 128, 128, 128, 128 };


static uvec8 kShufAc =
  { 0, 1, 6, 7, 12, 13, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128 };


static uvec8 kShufAc3 =
  { 128, 128, 128, 128, 128, 128, 0, 1, 6, 7, 12, 13, 128, 128, 128, 128 };


static uvec16 kScaleAc33 =
  { 65536 / 9, 65536 / 9, 65536 / 6, 65536 / 9, 65536 / 9, 65536 / 6, 0, 0 };


static uvec8 kShufAb0 =
  { 0, 128, 3, 128, 6, 128, 8, 128, 11, 128, 14, 128, 128, 128, 128, 128 };


static uvec8 kShufAb1 =
  { 1, 128, 4, 128, 7, 128, 9, 128, 12, 128, 15, 128, 128, 128, 128, 128 };


static uvec8 kShufAb2 =
  { 2, 128, 5, 128, 128, 128, 10, 128, 13, 128, 128, 128, 128, 128, 128, 128 };


static uvec16 kScaleAb2 =
  { 65536 / 3, 65536 / 3, 65536 / 2, 65536 / 3, 65536 / 3, 65536 / 2, 0, 0 };





void ScaleRowDown2_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void ScaleRowDown2Linear_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                              uint8* dst_ptr, int dst_width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10, 0) ",%%xmm1  \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm3                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "pavgw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void ScaleRowDown2Box_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    MEMOPREG(movdqa,0x00,0,3,1,xmm2)           
    BUNDLEALIGN
    MEMOPREG(movdqa,0x10,0,3,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm3                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "pavgw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  : "r"((intptr_t)(src_stride))   
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void ScaleRowDown2_Unaligned_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                                  uint8* dst_ptr, int dst_width) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void ScaleRowDown2Linear_Unaligned_SSE2(const uint8* src_ptr,
                                        ptrdiff_t src_stride,
                                        uint8* dst_ptr, int dst_width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"

    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm3                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "pavgw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void ScaleRowDown2Box_Unaligned_SSE2(const uint8* src_ptr,
                                     ptrdiff_t src_stride,
                                     uint8* dst_ptr, int dst_width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"

    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    MEMOPREG(movdqu,0x00,0,3,1,xmm2)           
    BUNDLEALIGN
    MEMOPREG(movdqu,0x10,0,3,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm3                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "pavgw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  : "r"((intptr_t)(src_stride))   
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void ScaleRowDown4_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrld     $0x18,%%xmm5                    \n"
    "pslld     $0x10,%%xmm5                    \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void ScaleRowDown4Box_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  intptr_t stridex3 = 0;
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0x8,%%xmm7                     \n"
    "lea       " MEMLEA4(0x00,4,4,2) ",%3      \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    MEMOPREG(movdqa,0x00,0,4,1,xmm2)           
    BUNDLEALIGN
    MEMOPREG(movdqa,0x10,0,4,1,xmm3)           
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    MEMOPREG(movdqa,0x00,0,4,2,xmm2)           
    BUNDLEALIGN
    MEMOPREG(movdqa,0x10,0,4,2,xmm3)           
    MEMOPREG(movdqa,0x00,0,3,1,xmm4)           
    MEMOPREG(movdqa,0x10,0,3,1,xmm5)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm4,%%xmm2                   \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm5,%%xmm3                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "pand      %%xmm7,%%xmm2                   \n"
    "pand      %%xmm7,%%xmm3                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "pavgw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "pand      %%xmm7,%%xmm2                   \n"
    "pavgw     %%xmm2,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),     
    "+r"(dst_ptr),     
    "+r"(dst_width),   
    "+r"(stridex3)     
  : "r"((intptr_t)(src_stride))    
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm7"
#endif
  );
}

void ScaleRowDown34_SSSE3(const uint8* src_ptr, ptrdiff_t src_stride,
                          uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %0,%%xmm3                       \n"
    "movdqa    %1,%%xmm4                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kShuf0),  
    "m"(kShuf1),  
    "m"(kShuf2)   
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm2   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "palignr   $0x8,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm3,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "pshufb    %%xmm5,%%xmm2                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "movq      %%xmm1," MEMACCESS2(0x8,1) "    \n"
    "movq      %%xmm2," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x18,1) ",%1           \n"
    "sub       $0x18,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),   
    "+r"(dst_ptr),   
    "+r"(dst_width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ScaleRowDown34_1_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %0,%%xmm2                       \n"  
    "movdqa    %1,%%xmm3                       \n"  
    "movdqa    %2,%%xmm4                       \n"  
  :
  : "m"(kShuf01),  
    "m"(kShuf11),  
    "m"(kShuf21)   
  );
  asm volatile (
    "movdqa    %0,%%xmm5                       \n"  
    "movdqa    %1,%%xmm0                       \n"  
    "movdqa    %2,%%xmm1                       \n"  
  :
  : "m"(kMadd01),  
    "m"(kMadd11),  
    "m"(kRound34)  
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm6         \n"
    MEMOPREG(movdqa,0x00,0,3,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm5,%%xmm6                   \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS(1) "         \n"
    "movdqu    " MEMACCESS2(0x8,0) ",%%xmm6    \n"
    MEMOPREG(movdqu,0x8,0,3,1,xmm7)            
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm3,%%xmm6                   \n"
    "pmaddubsw %%xmm0,%%xmm6                   \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS2(0x8,1) "    \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqa,0x10,0,3,1,xmm7)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm4,%%xmm6                   \n"
    "pmaddubsw %4,%%xmm6                       \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x18,1) ",%1           \n"
    "sub       $0x18,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),   
    "+r"(dst_ptr),   
    "+r"(dst_width)  
  : "r"((intptr_t)(src_stride)),  
    "m"(kMadd21)     
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ScaleRowDown34_0_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %0,%%xmm2                       \n"  
    "movdqa    %1,%%xmm3                       \n"  
    "movdqa    %2,%%xmm4                       \n"  
  :
  : "m"(kShuf01),  
    "m"(kShuf11),  
    "m"(kShuf21)   
  );
  asm volatile (
    "movdqa    %0,%%xmm5                       \n"  
    "movdqa    %1,%%xmm0                       \n"  
    "movdqa    %2,%%xmm1                       \n"  
  :
  : "m"(kMadd01),  
    "m"(kMadd11),  
    "m"(kRound34)  
  );

  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm6         \n"
    MEMOPREG(movdqa,0x00,0,3,1,xmm7)           
    "pavgb     %%xmm6,%%xmm7                   \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm5,%%xmm6                   \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS(1) "         \n"
    "movdqu    " MEMACCESS2(0x8,0) ",%%xmm6    \n"
    MEMOPREG(movdqu,0x8,0,3,1,xmm7)            
    "pavgb     %%xmm6,%%xmm7                   \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm3,%%xmm6                   \n"
    "pmaddubsw %%xmm0,%%xmm6                   \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS2(0x8,1) "    \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm6   \n"
    MEMOPREG(movdqa,0x10,0,3,1,xmm7)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm6,%%xmm7                   \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "pshufb    %%xmm4,%%xmm6                   \n"
    "pmaddubsw %4,%%xmm6                       \n"
    "paddsw    %%xmm1,%%xmm6                   \n"
    "psrlw     $0x2,%%xmm6                     \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movq      %%xmm6," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x18,1) ",%1           \n"
    "sub       $0x18,%2                        \n"
    "jg        1b                              \n"
    : "+r"(src_ptr),   
      "+r"(dst_ptr),   
      "+r"(dst_width)  
    : "r"((intptr_t)(src_stride)),  
      "m"(kMadd21)     
    : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ScaleRowDown38_SSSE3(const uint8* src_ptr, ptrdiff_t src_stride,
                          uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "movhlps   %%xmm0,%%xmm1                   \n"
    "movd      %%xmm1," MEMACCESS2(0x8,1) "    \n"
    "lea       " MEMLEA(0xc,1) ",%1            \n"
    "sub       $0xc,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),   
    "+r"(dst_ptr),   
    "+r"(dst_width)  
  : "m"(kShuf38a),   
    "m"(kShuf38b)    
  : "memory", "cc"
#if defined(__SSE2__)
      , "xmm0", "xmm1", "xmm4", "xmm5"
#endif
  );
}

void ScaleRowDown38_2_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %0,%%xmm2                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm4                       \n"
    "movdqa    %3,%%xmm5                       \n"
  :
  : "m"(kShufAb0),   
    "m"(kShufAb1),   
    "m"(kShufAb2),   
    "m"(kScaleAb2)   
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(pavgb,0x00,0,3,1,xmm0)            
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pshufb    %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm6                   \n"
    "pshufb    %%xmm3,%%xmm6                   \n"
    "paddusw   %%xmm6,%%xmm1                   \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "paddusw   %%xmm0,%%xmm1                   \n"
    "pmulhuw   %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "sub       $0x6,%2                         \n"
    "movd      %%xmm1," MEMACCESS(1) "         \n"
    "psrlq     $0x10,%%xmm1                    \n"
    "movd      %%xmm1," MEMACCESS2(0x2,1) "    \n"
    "lea       " MEMLEA(0x6,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),     
    "+r"(dst_ptr),     
    "+r"(dst_width)    
  : "r"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void ScaleRowDown38_3_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  asm volatile (
    "movdqa    %0,%%xmm2                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm4                       \n"
    "pxor      %%xmm5,%%xmm5                   \n"
  :
  : "m"(kShufAc),    
    "m"(kShufAc3),   
    "m"(kScaleAc33)  
  );
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,0,3,1,xmm6)           
    "movhlps   %%xmm0,%%xmm1                   \n"
    "movhlps   %%xmm6,%%xmm7                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm6                   \n"
    "punpcklbw %%xmm5,%%xmm7                   \n"
    "paddusw   %%xmm6,%%xmm0                   \n"
    "paddusw   %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqa,0x00,0,3,2,xmm6)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movhlps   %%xmm6,%%xmm7                   \n"
    "punpcklbw %%xmm5,%%xmm6                   \n"
    "punpcklbw %%xmm5,%%xmm7                   \n"
    "paddusw   %%xmm6,%%xmm0                   \n"
    "paddusw   %%xmm7,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm6                   \n"
    "psrldq    $0x2,%%xmm0                     \n"
    "paddusw   %%xmm0,%%xmm6                   \n"
    "psrldq    $0x2,%%xmm0                     \n"
    "paddusw   %%xmm0,%%xmm6                   \n"
    "pshufb    %%xmm2,%%xmm6                   \n"
    "movdqa    %%xmm1,%%xmm7                   \n"
    "psrldq    $0x2,%%xmm1                     \n"
    "paddusw   %%xmm1,%%xmm7                   \n"
    "psrldq    $0x2,%%xmm1                     \n"
    "paddusw   %%xmm1,%%xmm7                   \n"
    "pshufb    %%xmm3,%%xmm7                   \n"
    "paddusw   %%xmm7,%%xmm6                   \n"
    "pmulhuw   %%xmm4,%%xmm6                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "sub       $0x6,%2                         \n"
    "movd      %%xmm6," MEMACCESS(1) "         \n"
    "psrlq     $0x10,%%xmm6                    \n"
    "movd      %%xmm6," MEMACCESS2(0x2,1) "    \n"
    "lea       " MEMLEA(0x6,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),    
    "+r"(dst_ptr),    
    "+r"(dst_width)   
  : "r"((intptr_t)(src_stride))   
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ScaleAddRows_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                       uint16* dst_ptr, int src_width, int src_height) {
  int tmp_height = 0;
  intptr_t tmp_src = 0;
  asm volatile (
    "pxor      %%xmm4,%%xmm4                   \n"
    "sub       $0x1,%5                         \n"

    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "mov       %0,%3                           \n"
    "add       %6,%0                           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm4,%%xmm0                   \n"
    "punpckhbw %%xmm4,%%xmm1                   \n"
    "mov       %5,%2                           \n"
    "test      %2,%2                           \n"
    "je        3f                              \n"

    LABELALIGN
  "2:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm2         \n"
    "add       %6,%0                           \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "punpcklbw %%xmm4,%%xmm2                   \n"
    "punpckhbw %%xmm4,%%xmm3                   \n"
    "paddusw   %%xmm2,%%xmm0                   \n"
    "paddusw   %%xmm3,%%xmm1                   \n"
    "sub       $0x1,%2                         \n"
    "jg        2b                              \n"

    LABELALIGN
  "3:                                          \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x10,3) ",%0           \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "sub       $0x10,%4                        \n"
    "jg        1b                              \n"
  : "+r"(src_ptr),     
    "+r"(dst_ptr),     
    "+r"(tmp_height),  
    "+r"(tmp_src),     
    "+r"(src_width),   
    "+rm"(src_height)  
  : "rm"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}


void ScaleFilterCols_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                           int dst_width, int x, int dx) {
  intptr_t x0 = 0, x1 = 0, temp_pixel = 0;
  asm volatile (
    "movd      %6,%%xmm2                       \n"
    "movd      %7,%%xmm3                       \n"
    "movl      $0x04040000,%k2                 \n"
    "movd      %k2,%%xmm5                      \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x9,%%xmm6                     \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "subl      $0x2,%5                         \n"
    "jl        29f                             \n"
    "movdqa    %%xmm2,%%xmm0                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "punpckldq %%xmm0,%%xmm2                   \n"
    "punpckldq %%xmm3,%%xmm3                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"

    LABELALIGN
  "2:                                          \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm3,%%xmm2                   \n"
    MEMOPARG(movzwl,0x00,1,3,1,k2)             
    "movd      %k2,%%xmm0                      \n"
    "psrlw     $0x9,%%xmm1                     \n"
    BUNDLEALIGN
    MEMOPARG(movzwl,0x00,1,4,1,k2)             
    "movd      %k2,%%xmm4                      \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "punpcklwd %%xmm4,%%xmm0                   \n"
    "pxor      %%xmm6,%%xmm1                   \n"
    "pmaddubsw %%xmm1,%%xmm0                   \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0,%k2                      \n"
    "mov       %w2," MEMACCESS(0) "            \n"
    "lea       " MEMLEA(0x2,0) ",%0            \n"
    "sub       $0x2,%5                         \n"
    "jge       2b                              \n"

    LABELALIGN
  "29:                                         \n"
    "addl      $0x1,%5                         \n"
    "jl        99f                             \n"
    MEMOPARG(movzwl,0x00,1,3,1,k2)             
    "movd      %k2,%%xmm0                      \n"
    "psrlw     $0x9,%%xmm2                     \n"
    "pshufb    %%xmm5,%%xmm2                   \n"
    "pxor      %%xmm6,%%xmm2                   \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0,%k2                      \n"
    "mov       %b2," MEMACCESS(0) "            \n"
  "99:                                         \n"
  : "+r"(dst_ptr),     
    "+r"(src_ptr),     
    "+a"(temp_pixel),  
    "+r"(x0),          
    "+r"(x1),          
    "+rm"(dst_width)   
  : "rm"(x),           
    "rm"(dx)           
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}



void ScaleColsUp2_SSE2(uint8* dst_ptr, const uint8* src_ptr,
                       int dst_width, int x, int dx) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "sub       $0x20,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(0) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,0) "   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "jg        1b                              \n"

  : "+r"(dst_ptr),     
    "+r"(src_ptr),     
    "+r"(dst_width)    
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void ScaleARGBRowDown2_SSE2(const uint8* src_argb,
                            ptrdiff_t src_stride,
                            uint8* dst_argb, int dst_width) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "shufps    $0xdd,%%xmm1,%%xmm0             \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(dst_width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void ScaleARGBRowDown2Linear_SSE2(const uint8* src_argb,
                                  ptrdiff_t src_stride,
                                  uint8* dst_argb, int dst_width) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm2             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(dst_width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void ScaleARGBRowDown2Box_SSE2(const uint8* src_argb,
                               ptrdiff_t src_stride,
                               uint8* dst_argb, int dst_width) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    BUNDLEALIGN
    MEMOPREG(movdqa,0x00,0,3,1,xmm2)           
    MEMOPREG(movdqa,0x10,0,3,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm2             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),   
    "+r"(dst_argb),   
    "+r"(dst_width)   
  : "r"((intptr_t)(src_stride))   
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}



void ScaleARGBRowDownEven_SSE2(const uint8* src_argb, ptrdiff_t src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width) {
  intptr_t src_stepx_x4 = (intptr_t)(src_stepx);
  intptr_t src_stepx_x12 = 0;
  asm volatile (
    "lea       " MEMLEA3(0x00,1,4) ",%1        \n"
    "lea       " MEMLEA4(0x00,1,1,2) ",%4      \n"
    LABELALIGN
  "1:                                          \n"
    "movd      " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movd,0x00,0,1,1,xmm1)             
    "punpckldq %%xmm1,%%xmm0                   \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,0,1,2,xmm2)             
    MEMOPREG(movd,0x00,0,4,1,xmm3)             
    "lea       " MEMLEA4(0x00,0,1,4) ",%0      \n"
    "punpckldq %%xmm3,%%xmm2                   \n"
    "punpcklqdq %%xmm2,%%xmm0                  \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),      
    "+r"(src_stepx_x4),  
    "+r"(dst_argb),      
    "+r"(dst_width),     
    "+r"(src_stepx_x12)  
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}



void ScaleARGBRowDownEvenBox_SSE2(const uint8* src_argb,
                                  ptrdiff_t src_stride, int src_stepx,
                                  uint8* dst_argb, int dst_width) {
  intptr_t src_stepx_x4 = (intptr_t)(src_stepx);
  intptr_t src_stepx_x12 = 0;
  intptr_t row1 = (intptr_t)(src_stride);
  asm volatile (
    "lea       " MEMLEA3(0x00,1,4) ",%1        \n"
    "lea       " MEMLEA4(0x00,1,1,2) ",%4      \n"
    "lea       " MEMLEA4(0x00,0,5,1) ",%5      \n"

    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movhps,0x00,0,1,1,xmm0)           
    MEMOPREG(movq,0x00,0,1,2,xmm1)             
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,0,4,1,xmm1)           
    "lea       " MEMLEA4(0x00,0,1,4) ",%0      \n"
    "movq      " MEMACCESS(5) ",%%xmm2         \n"
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,5,1,1,xmm2)           
    MEMOPREG(movq,0x00,5,1,2,xmm3)             
    MEMOPREG(movhps,0x00,5,4,1,xmm3)           
    "lea       " MEMLEA4(0x00,5,1,4) ",%5      \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm2             \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),       
    "+r"(src_stepx_x4),   
    "+r"(dst_argb),       
    "+rm"(dst_width),     
    "+r"(src_stepx_x12),  
    "+r"(row1)            
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3"
#endif
  );
}

void ScaleARGBCols_SSE2(uint8* dst_argb, const uint8* src_argb,
                        int dst_width, int x, int dx) {
  intptr_t x0 = 0, x1 = 0;
  asm volatile (
    "movd      %5,%%xmm2                       \n"
    "movd      %6,%%xmm3                       \n"
    "pshufd    $0x0,%%xmm2,%%xmm2              \n"
    "pshufd    $0x11,%%xmm3,%%xmm0             \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pshufd    $0x5,%%xmm3,%%xmm0              \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "pextrw    $0x1,%%xmm2,%k0                 \n"
    "pextrw    $0x3,%%xmm2,%k1                 \n"
    "cmp       $0x0,%4                         \n"
    "jl        99f                             \n"
    "sub       $0x4,%4                         \n"
    "jl        49f                             \n"

    LABELALIGN
  "40:                                         \n"
    MEMOPREG(movd,0x00,3,0,4,xmm0)             
    MEMOPREG(movd,0x00,3,1,4,xmm1)             
    "pextrw    $0x5,%%xmm2,%k0                 \n"
    "pextrw    $0x7,%%xmm2,%k1                 \n"
    "paddd     %%xmm3,%%xmm2                   \n"
    "punpckldq %%xmm1,%%xmm0                   \n"
    MEMOPREG(movd,0x00,3,0,4,xmm1)             
    MEMOPREG(movd,0x00,3,1,4,xmm4)             
    "pextrw    $0x1,%%xmm2,%k0                 \n"
    "pextrw    $0x3,%%xmm2,%k1                 \n"
    "punpckldq %%xmm4,%%xmm1                   \n"
    "punpcklqdq %%xmm1,%%xmm0                  \n"
    "sub       $0x4,%4                         \n"
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "test      $0x2,%4                         \n"
    "je        29f                             \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,3,0,4,xmm0)             
    MEMOPREG(movd,0x00,3,1,4,xmm1)             
    "pextrw    $0x5,%%xmm2,%k0                 \n"
    "punpckldq %%xmm1,%%xmm0                   \n"
    "movq      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x8,2) ",%2            \n"
  "29:                                         \n"
    "test      $0x1,%4                         \n"
    "je        99f                             \n"
    MEMOPREG(movd,0x00,3,0,4,xmm0)             
    "movd      %%xmm0," MEMACCESS(2) "         \n"
  "99:                                         \n"
  : "+a"(x0),          
    "+d"(x1),          
    "+r"(dst_argb),    
    "+r"(src_argb),    
    "+r"(dst_width)    
  : "rm"(x),           
    "rm"(dx)           
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}



void ScaleARGBColsUp2_SSE2(uint8* dst_argb, const uint8* src_argb,
                           int dst_width, int x, int dx) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpckldq %%xmm0,%%xmm0                   \n"
    "punpckhdq %%xmm1,%%xmm1                   \n"
    "sub       $0x8,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(0) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,0) "   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "jg        1b                              \n"

  : "+r"(dst_argb),    
    "+r"(src_argb),    
    "+r"(dst_width)    
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}


static uvec8 kShuffleColARGB = {
  0u, 4u, 1u, 5u, 2u, 6u, 3u, 7u,  
  8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u  
};


static uvec8 kShuffleFractions = {
  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
};


void ScaleARGBFilterCols_SSSE3(uint8* dst_argb, const uint8* src_argb,
                               int dst_width, int x, int dx) {
  intptr_t x0 = 0, x1 = 0;
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm5                       \n"
  :
  : "m"(kShuffleColARGB),  
    "m"(kShuffleFractions)  
  );

  asm volatile (
    "movd      %5,%%xmm2                       \n"
    "movd      %6,%%xmm3                       \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x9,%%xmm6                     \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "sub       $0x2,%2                         \n"
    "jl        29f                             \n"
    "movdqa    %%xmm2,%%xmm0                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "punpckldq %%xmm0,%%xmm2                   \n"
    "punpckldq %%xmm3,%%xmm3                   \n"
    "paddd     %%xmm3,%%xmm3                   \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"

    LABELALIGN
  "2:                                          \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm3,%%xmm2                   \n"
    MEMOPREG(movq,0x00,1,3,4,xmm0)             
    "psrlw     $0x9,%%xmm1                     \n"
    BUNDLEALIGN
    MEMOPREG(movhps,0x00,1,4,4,xmm0)           
    "pshufb    %%xmm5,%%xmm1                   \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "pxor      %%xmm6,%%xmm1                   \n"
    "pmaddubsw %%xmm1,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "pextrw    $0x1,%%xmm2,%k3                 \n"
    "pextrw    $0x3,%%xmm2,%k4                 \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movq      %%xmm0," MEMACCESS(0) "         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "sub       $0x2,%2                         \n"
    "jge       2b                              \n"

    LABELALIGN
  "29:                                         \n"
    "add       $0x1,%2                         \n"
    "jl        99f                             \n"
    "psrlw     $0x9,%%xmm2                     \n"
    BUNDLEALIGN
    MEMOPREG(movq,0x00,1,3,4,xmm0)             
    "pshufb    %%xmm5,%%xmm2                   \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "pxor      %%xmm6,%%xmm2                   \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0," MEMACCESS(0) "         \n"

    LABELALIGN
  "99:                                         \n"
  : "+r"(dst_argb),    
    "+r"(src_argb),    
    "+rm"(dst_width),  
    "+r"(x0),          
    "+r"(x1)           
  : "rm"(x),           
    "rm"(dx)           
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}


int FixedDiv_X86(int num, int div) {
  asm volatile (
    "cdq                                       \n"
    "shld      $0x10,%%eax,%%edx               \n"
    "shl       $0x10,%%eax                     \n"
    "idiv      %1                              \n"
    "mov       %0, %%eax                       \n"
    : "+a"(num)  
    : "c"(div)   
    : "memory", "cc", "edx"
  );
  return num;
}


int FixedDiv1_X86(int num, int div) {
  asm volatile (
    "cdq                                       \n"
    "shld      $0x10,%%eax,%%edx               \n"
    "shl       $0x10,%%eax                     \n"
    "sub       $0x10001,%%eax                  \n"
    "sbb       $0x0,%%edx                      \n"
    "sub       $0x1,%1                         \n"
    "idiv      %1                              \n"
    "mov       %0, %%eax                       \n"
    : "+a"(num)  
    : "c"(div)   
    : "memory", "cc", "edx"
  );
  return num;
}

#endif  

#ifdef __cplusplus
}  
}  
#endif
