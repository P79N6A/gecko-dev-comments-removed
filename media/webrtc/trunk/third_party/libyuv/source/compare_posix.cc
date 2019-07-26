









#include "libyuv/basic_types.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) || defined(__i386__))

uint32 SumSquareError_SSE2(const uint8* src_a, const uint8* src_b, int count) {
  uint32 sse;
  asm volatile (  
    "pxor      %%xmm0,%%xmm0                   \n"
    "pxor      %%xmm5,%%xmm5                   \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10, 0) ",%0          \n"
    "movdqa    " MEMACCESS(1) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10, 1) ",%1          \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm1,%%xmm3                   \n"
    "psubusb   %%xmm2,%%xmm1                   \n"
    "psubusb   %%xmm3,%%xmm2                   \n"
    "por       %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "punpckhbw %%xmm5,%%xmm2                   \n"
    "pmaddwd   %%xmm1,%%xmm1                   \n"
    "pmaddwd   %%xmm2,%%xmm2                   \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "paddd     %%xmm2,%%xmm0                   \n"
    "jg        1b                              \n"

    "pshufd    $0xee,%%xmm0,%%xmm1             \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "pshufd    $0x1,%%xmm0,%%xmm1              \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "movd      %%xmm0,%3                       \n"

  : "+r"(src_a),      
    "+r"(src_b),      
    "+r"(count),      
    "=g"(sse)         
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );  
  return sse;
}

#endif  

#if !defined(LIBYUV_DISABLE_X86) && \
    (defined(__x86_64__) || (defined(__i386__) && !defined(__pic__)))
#define HAS_HASHDJB2_SSE41
static uvec32 kHash16x33 = { 0x92d9e201, 0, 0, 0 };  
static uvec32 kHashMul0 = {
  0x0c3525e1,  
  0xa3476dc1,  
  0x3b4039a1,  
  0x4f5f0981,  
};
static uvec32 kHashMul1 = {
  0x30f35d61,  
  0x855cb541,  
  0x040a9121,  
  0x747c7101,  
};
static uvec32 kHashMul2 = {
  0xec41d4e1,  
  0x4cfa3cc1,  
  0x025528a1,  
  0x00121881,  
};
static uvec32 kHashMul3 = {
  0x00008c61,  
  0x00000441,  
  0x00000021,  
  0x00000001,  
};

uint32 HashDjb2_SSE41(const uint8* src, int count, uint32 seed) {
  uint32 hash;
  asm volatile (  
    "movd      %2,%%xmm0                       \n"
    "pxor      %%xmm7,%%xmm7                   \n"
    "movdqa    %4,%%xmm6                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10, 0) ",%0          \n"
    "pmulld    %%xmm6,%%xmm0                   \n"
    "movdqa    %5,%%xmm5                       \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "punpcklwd %%xmm7,%%xmm3                   \n"
    "pmulld    %%xmm5,%%xmm3                   \n"
    "movdqa    %6,%%xmm5                       \n"
    "movdqa    %%xmm2,%%xmm4                   \n"
    "punpckhwd %%xmm7,%%xmm4                   \n"
    "pmulld    %%xmm5,%%xmm4                   \n"
    "movdqa    %7,%%xmm5                       \n"
    "punpckhbw %%xmm7,%%xmm1                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklwd %%xmm7,%%xmm2                   \n"
    "pmulld    %%xmm5,%%xmm2                   \n"
    "movdqa    %8,%%xmm5                       \n"
    "punpckhwd %%xmm7,%%xmm1                   \n"
    "pmulld    %%xmm5,%%xmm1                   \n"
    "paddd     %%xmm4,%%xmm3                   \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "sub       $0x10,%1                        \n"
    "paddd     %%xmm3,%%xmm1                   \n"
    "pshufd    $0xe,%%xmm1,%%xmm2              \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "pshufd    $0x1,%%xmm1,%%xmm2              \n"
    "paddd     %%xmm2,%%xmm1                   \n"
    "paddd     %%xmm1,%%xmm0                   \n"
    "jg        1b                              \n"
    "movd      %%xmm0,%3                       \n"
  : "+r"(src),        
    "+r"(count),      
    "+rm"(seed),      
    "=g"(hash)        
  : "m"(kHash16x33),  
    "m"(kHashMul0),   
    "m"(kHashMul1),   
    "m"(kHashMul2),   
    "m"(kHashMul3)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );  
  return hash;
}
#endif  

#ifdef __cplusplus
}  
}  
#endif

