









#include "libyuv/row.h"

#include "libyuv/basic_types.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(YUV_DISABLE_ASM) && (defined(__x86_64__) || defined(__i386__))



#ifdef __APPLE__
#define CONST
#else
#define CONST static const
#endif

#ifdef HAS_ARGBTOYROW_SSSE3


CONST vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

CONST vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

CONST vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};


CONST vec8 kBGRAToY = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

CONST vec8 kBGRAToU = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

CONST vec8 kBGRAToV = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};


CONST vec8 kABGRToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

CONST vec8 kABGRToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

CONST vec8 kABGRToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};

CONST uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

CONST uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};


CONST uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};


CONST uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};


CONST uvec8 kShuffleMaskABGRToARGB = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};


CONST uvec8 kShuffleMaskBGRAToARGB = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};


CONST uvec8 kShuffleMaskRGBAToARGB = {
  1u, 2u, 3u, 0u, 5u, 6u, 7u, 4u, 9u, 10u, 11u, 8u, 13u, 14u, 15u, 12u
};


CONST uvec8 kShuffleMaskARGBToRGBA = {
  3u, 0u, 1u, 2u, 7u, 4u, 5u, 6u, 11u, 8u, 9u, 10u, 15u, 12u, 13u, 14u
};


CONST uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};


CONST uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};

void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movq      (%0),%%xmm0                     \n"
    "lea       0x8(%0),%0                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "por       %%xmm5,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "lea       0x20(%1),%1                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src_y),     
    "+r"(dst_argb),  
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"

  : "+r"(src_abgr),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "m"(kShuffleMaskABGRToARGB)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}

void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_bgra),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "m"(kShuffleMaskBGRAToARGB)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}

void RGBAToARGBRow_SSSE3(const uint8* src_rgba, uint8* dst_argb, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"

  : "+r"(src_rgba),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "m"(kShuffleMaskRGBAToARGB)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}

void ARGBToRGBARow_SSSE3(const uint8* src_argb, uint8* dst_rgba, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"

  : "+r"(src_argb),  
    "+r"(dst_rgba),  
    "+r"(pix)        
  : "m"(kShuffleMaskARGBToRGBA)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}

void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"  
    "pslld     $0x18,%%xmm5                    \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm3                 \n"
    "lea       0x30(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3,0x30(%1)                 \n"
    "lea       0x40(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_rgb24),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "m"(kShuffleMaskRGB24ToARGB)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"  
    "pslld     $0x18,%%xmm5                    \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm3                 \n"
    "lea       0x30(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3,0x30(%1)                 \n"
    "lea       0x40(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_raw),   
    "+r"(dst_argb),  
    "+r"(pix)        
  : "m"(kShuffleMaskRAWToARGB)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void RGB565ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0x1080108,%%eax                \n"
    "movd      %%eax,%%xmm5                    \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "mov       $0x20802080,%%eax               \n"
    "movd      %%eax,%%xmm6                    \n"
    "pshufd    $0x0,%%xmm6,%%xmm6              \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psllw     $0xb,%%xmm3                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psllw     $0xa,%%xmm4                     \n"
    "psrlw     $0x5,%%xmm4                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psllw     $0x8,%%xmm7                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "psllw     $0xb,%%xmm2                     \n"
    "pmulhuw   %%xmm5,%%xmm1                   \n"
    "pmulhuw   %%xmm5,%%xmm2                   \n"
    "psllw     $0x8,%%xmm1                     \n"
    "por       %%xmm2,%%xmm1                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "pmulhuw   %%xmm6,%%xmm0                   \n"
    "por       %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpckhbw %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm1,(%1,%0,2)                \n"
    "movdqa    %%xmm2,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGB1555ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0x1080108,%%eax                \n"
    "movd      %%eax,%%xmm5                    \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "mov       $0x42004200,%%eax               \n"
    "movd      %%eax,%%xmm6                    \n"
    "pshufd    $0x0,%%xmm6,%%xmm6              \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psllw     $0xb,%%xmm3                     \n"
    "movdqa    %%xmm3,%%xmm4                   \n"
    "psrlw     $0x6,%%xmm4                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psllw     $0x8,%%xmm7                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "psllw     $0x1,%%xmm1                     \n"
    "psllw     $0xb,%%xmm2                     \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "pmulhuw   %%xmm5,%%xmm2                   \n"
    "pmulhuw   %%xmm5,%%xmm1                   \n"
    "psllw     $0x8,%%xmm1                     \n"
    "por       %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "psraw     $0x8,%%xmm2                     \n"
    "pmulhuw   %%xmm6,%%xmm0                   \n"
    "pand      %%xmm7,%%xmm2                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpckhbw %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm1,(%1,%0,2)                \n"
    "movdqa    %%xmm2,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGB4444ToARGBRow_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "mov       $0xf0f0f0f,%%eax                \n"
    "movd      %%eax,%%xmm4                    \n"
    "pshufd    $0x0,%%xmm4,%%xmm4              \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "pslld     $0x4,%%xmm5                     \n"
    "sub       %0,%1                           \n"
    "sub       %0,%1                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "psllw     $0x4,%%xmm1                     \n"
    "psrlw     $0x4,%%xmm3                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm0                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1,%0,2)                \n"
    "movdqa    %%xmm1,0x10(%1,%0,2)            \n"
    "lea       0x10(%0),%0                     \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToRGB24Row_SSSE3(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm6                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "lea       0x40(%0),%0                     \n"
    "pshufb    %%xmm6,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "pshufb    %%xmm6,%%xmm2                   \n"
    "pshufb    %%xmm6,%%xmm3                   \n"
    "movdqa    %%xmm1,%%xmm4                   \n"
    "psrldq    $0x4,%%xmm1                     \n"
    "pslldq    $0xc,%%xmm4                     \n"
    "movdqa    %%xmm2,%%xmm5                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pslldq    $0x8,%%xmm5                     \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "lea       0x30(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  : "m"(kShuffleMaskARGBToRGB24)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void ARGBToRAWRow_SSSE3(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm6                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "lea       0x40(%0),%0                     \n"
    "pshufb    %%xmm6,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "pshufb    %%xmm6,%%xmm2                   \n"
    "pshufb    %%xmm6,%%xmm3                   \n"
    "movdqa    %%xmm1,%%xmm4                   \n"
    "psrldq    $0x4,%%xmm1                     \n"
    "pslldq    $0xc,%%xmm4                     \n"
    "movdqa    %%xmm2,%%xmm5                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pslldq    $0x8,%%xmm5                     \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqa    %%xmm1,0x10(%1)                 \n"
    "movdqa    %%xmm2,0x20(%1)                 \n"
    "lea       0x30(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  : "m"(kShuffleMaskARGBToRAW)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void ARGBToRGB565Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "psrld     $0x1b,%%xmm3                    \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psrld     $0x1a,%%xmm4                    \n"
    "pslld     $0x5,%%xmm4                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0xb,%%xmm5                     \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "pslld     $0x8,%%xmm0                     \n"
    "psrld     $0x3,%%xmm1                     \n"
    "psrld     $0x5,%%xmm2                     \n"
    "psrad     $0x10,%%xmm0                    \n"
    "pand      %%xmm3,%%xmm1                   \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "por       %%xmm2,%%xmm1                   \n"
    "por       %%xmm1,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToARGB1555Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psrld     $0x1b,%%xmm4                    \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "pslld     $0x5,%%xmm5                     \n"
    "movdqa    %%xmm4,%%xmm6                   \n"
    "pslld     $0xa,%%xmm6                     \n"
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "pslld     $0xf,%%xmm7                     \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm3                   \n"
    "psrad     $0x10,%%xmm0                    \n"
    "psrld     $0x3,%%xmm1                     \n"
    "psrld     $0x6,%%xmm2                     \n"
    "psrld     $0x9,%%xmm3                     \n"
    "pand      %%xmm7,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm2                   \n"
    "pand      %%xmm6,%%xmm3                   \n"
    "por       %%xmm1,%%xmm0                   \n"
    "por       %%xmm3,%%xmm2                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}

void ARGBToARGB4444Row_SSE2(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psllw     $0xc,%%xmm4                     \n"
    "movdqa    %%xmm4,%%xmm3                   \n"
    "psrlw     $0x8,%%xmm3                     \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm3,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm1                   \n"
    "psrlq     $0x4,%%xmm0                     \n"
    "psrlq     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "lea       0x10(%0),%0                     \n"
    "movq      %%xmm0,(%1)                     \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x4,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}

void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kARGBToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kARGBToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}






void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToU),  
    "m"(kARGBToV),  
    "m"(kAddUV128)  
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_argb))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ARGBToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToU),         
    "m"(kARGBToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_argb))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void BGRAToYRow_SSSE3(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_bgra),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kBGRAToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void BGRAToYRow_Unaligned_SSSE3(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_bgra),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kBGRAToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void BGRAToUVRow_SSSE3(const uint8* src_bgra0, int src_stride_bgra,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kBGRAToU),         
    "m"(kBGRAToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_bgra))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void BGRAToUVRow_Unaligned_SSSE3(const uint8* src_bgra0, int src_stride_bgra,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kBGRAToU),         
    "m"(kBGRAToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_bgra))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ABGRToYRow_SSSE3(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_abgr),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kABGRToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ABGRToYRow_Unaligned_SSSE3(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm3                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       0x40(%0),%0                     \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_abgr),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kABGRToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ABGRToUVRow_SSSE3(const uint8* src_abgr0, int src_stride_abgr,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kABGRToU),         
    "m"(kABGRToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm6                 \n"
    "pavgb     (%0,%4,1),%%xmm0                \n"
    "pavgb     0x10(%0,%4,1),%%xmm1            \n"
    "pavgb     0x20(%0,%4,1),%%xmm2            \n"
    "pavgb     0x30(%0,%4,1),%%xmm6            \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_abgr))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ABGRToUVRow_Unaligned_SSSE3(const uint8* src_abgr0, int src_stride_abgr,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kABGRToU),         
    "m"(kABGRToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    0x20(%0),%%xmm2                 \n"
    "movdqu    0x30(%0),%%xmm6                 \n"
    "movdqu    (%0,%4,1),%%xmm7                \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqu    0x10(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm1                   \n"
    "movdqu    0x20(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqu    0x30(%0,%4,1),%%xmm7            \n"
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       0x40(%0),%0                     \n"
    "movdqa    %%xmm0,%%xmm7                   \n"
    "shufps    $0x88,%%xmm1,%%xmm0             \n"
    "shufps    $0xdd,%%xmm1,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm7                   \n"
    "shufps    $0x88,%%xmm6,%%xmm2             \n"
    "shufps    $0xdd,%%xmm6,%%xmm7             \n"
    "pavgb     %%xmm7,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm6                   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm2,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0,(%1)                     \n"
    "movhps    %%xmm0,(%1,%2,1)                \n"
    "lea       0x8(%1),%1                      \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"(static_cast<intptr_t>(src_stride_abgr))
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_I422TOARGBROW_SSSE3
#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */


#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

struct {
  vec8 kUVToB;  
  vec8 kUVToG;  
  vec8 kUVToR;  
  vec16 kUVBiasB;  
  vec16 kUVBiasG;  
  vec16 kUVBiasR;  
  vec16 kYSub16;  
  vec16 kYToRgb;  
  vec8 kVUToB;  
  vec8 kVUToG;  
  vec8 kVUToR;  
} CONST SIMD_ALIGNED(kYuvConstants) = {
  { UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB },
  { UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG },
  { UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR },
  { BB, BB, BB, BB, BB, BB, BB, BB },
  { BG, BG, BG, BG, BG, BG, BG, BG },
  { BR, BR, BR, BR, BR, BR, BR, BR },
  { 16, 16, 16, 16, 16, 16, 16, 16 },
  { YG, YG, YG, YG, YG, YG, YG, YG },
  { VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB },
  { VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG },
  { VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR }
};



#define READYUV444                                                             \
    "movq       (%[u_buf]),%%xmm0              \n"                             \
    "movq       (%[u_buf],%[v_buf],1),%%xmm1   \n"                             \
    "lea        0x8(%[u_buf]),%[u_buf]         \n"                             \
    "punpcklbw  %%xmm1,%%xmm0                  \n"                             \


#define READYUV422                                                             \
    "movd       (%[u_buf]),%%xmm0              \n"                             \
    "movd       (%[u_buf],%[v_buf],1),%%xmm1   \n"                             \
    "lea        0x4(%[u_buf]),%[u_buf]         \n"                             \
    "punpcklbw  %%xmm1,%%xmm0                  \n"                             \
    "punpcklwd  %%xmm0,%%xmm0                  \n"                             \


#define READYUV411                                                             \
    "movd       (%[u_buf]),%%xmm0              \n"                             \
    "movd       (%[u_buf],%[v_buf],1),%%xmm1   \n"                             \
    "lea        0x2(%[u_buf]),%[u_buf]         \n"                             \
    "punpcklbw  %%xmm1,%%xmm0                  \n"                             \
    "punpcklwd  %%xmm0,%%xmm0                  \n"                             \
    "punpckldq  %%xmm0,%%xmm0                  \n"                             \


#define READNV12                                                               \
    "movq       (%[uv_buf]),%%xmm0             \n"                             \
    "lea        0x8(%[uv_buf]),%[uv_buf]       \n"                             \
    "punpcklwd  %%xmm0,%%xmm0                  \n"                             \


#define YUVTORGB                                                               \
    "movdqa     %%xmm0,%%xmm1                  \n"                             \
    "movdqa     %%xmm0,%%xmm2                  \n"                             \
    "pmaddubsw  (%[kYuvConstants]),%%xmm0      \n"                             \
    "pmaddubsw  16(%[kYuvConstants]),%%xmm1    \n"                             \
    "pmaddubsw  32(%[kYuvConstants]),%%xmm2    \n"                             \
    "psubw      48(%[kYuvConstants]),%%xmm0    \n"                             \
    "psubw      64(%[kYuvConstants]),%%xmm1    \n"                             \
    "psubw      80(%[kYuvConstants]),%%xmm2    \n"                             \
    "movq       (%[y_buf]),%%xmm3              \n"                             \
    "lea        0x8(%[y_buf]),%[y_buf]         \n"                             \
    "punpcklbw  %%xmm4,%%xmm3                  \n"                             \
    "psubsw     96(%[kYuvConstants]),%%xmm3    \n"                             \
    "pmullw     112(%[kYuvConstants]),%%xmm3   \n"                             \
    "paddsw     %%xmm3,%%xmm0                  \n"                             \
    "paddsw     %%xmm3,%%xmm1                  \n"                             \
    "paddsw     %%xmm3,%%xmm2                  \n"                             \
    "psraw      $0x6,%%xmm0                    \n"                             \
    "psraw      $0x6,%%xmm1                    \n"                             \
    "psraw      $0x6,%%xmm2                    \n"                             \
    "packuswb   %%xmm0,%%xmm0                  \n"                             \
    "packuswb   %%xmm1,%%xmm1                  \n"                             \
    "packuswb   %%xmm2,%%xmm2                  \n"                             \


#define YVUTORGB                                                               \
    "movdqa     %%xmm0,%%xmm1                  \n"                             \
    "movdqa     %%xmm0,%%xmm2                  \n"                             \
    "pmaddubsw  128(%[kYuvConstants]),%%xmm0   \n"                             \
    "pmaddubsw  144(%[kYuvConstants]),%%xmm1   \n"                             \
    "pmaddubsw  160(%[kYuvConstants]),%%xmm2   \n"                             \
    "psubw      48(%[kYuvConstants]),%%xmm0    \n"                             \
    "psubw      64(%[kYuvConstants]),%%xmm1    \n"                             \
    "psubw      80(%[kYuvConstants]),%%xmm2    \n"                             \
    "movq       (%[y_buf]),%%xmm3              \n"                             \
    "lea        0x8(%[y_buf]),%[y_buf]         \n"                             \
    "punpcklbw  %%xmm4,%%xmm3                  \n"                             \
    "psubsw     96(%[kYuvConstants]),%%xmm3    \n"                             \
    "pmullw     112(%[kYuvConstants]),%%xmm3   \n"                             \
    "paddsw     %%xmm3,%%xmm0                  \n"                             \
    "paddsw     %%xmm3,%%xmm1                  \n"                             \
    "paddsw     %%xmm3,%%xmm2                  \n"                             \
    "psraw      $0x6,%%xmm0                    \n"                             \
    "psraw      $0x6,%%xmm1                    \n"                             \
    "psraw      $0x6,%%xmm2                    \n"                             \
    "packuswb   %%xmm0,%%xmm0                  \n"                             \
    "packuswb   %%xmm1,%%xmm1                  \n"                             \
    "packuswb   %%xmm2,%%xmm2                  \n"                             \

void OMITFP I444ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* argb_buf,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV444
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* argb_buf,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I411ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* argb_buf,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV411
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV12ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* uv_buf,
                                uint8* argb_buf,
                                int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READNV12
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV21ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* vu_buf,
                                uint8* argb_buf,
                                int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READNV12
    YVUTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(vu_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I444ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* argb_buf,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV444
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* argb_buf,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I411ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* argb_buf,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV411
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV12ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* uv_buf,
                                          uint8* argb_buf,
                                          int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READNV12
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV21ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* vu_buf,
                                          uint8* argb_buf,
                                          int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READNV12
    YVUTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(vu_buf),    
    [argb_buf]"+r"(argb_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToBGRARow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* bgra_buf,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm5,(%[argb_buf])            \n"
    "movdqa    %%xmm0,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(bgra_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToABGRRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* abgr_buf,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,(%[argb_buf])            \n"
    "movdqa    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(abgr_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* bgra_buf,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm5,(%[argb_buf])            \n"
    "movdqu    %%xmm0,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(bgra_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* abgr_buf,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqu    %%xmm2,(%[argb_buf])            \n"
    "movdqu    %%xmm1,0x10(%[argb_buf])        \n"
    "lea       0x20(%[argb_buf]),%[argb_buf]   \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [argb_buf]"+r"(abgr_buf),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_YTOARGBROW_SSE2
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "mov       $0x10001000,%%eax               \n"
    "movd      %%eax,%%xmm3                    \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "mov       $0x012a012a,%%eax               \n"
    "movd      %%eax,%%xmm2                    \n"
    "pshufd    $0x0,%%xmm2,%%xmm2              \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    
    "movq      (%0),%%xmm0                     \n"
    "lea       0x8(%0),%0                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "psubusw   %%xmm3,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"

    
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "por       %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "movdqa    %%xmm1,16(%1)                   \n"
    "lea       32(%1),%1                       \n"

    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),    
    "+r"(rgb_buf),  
    "+rm"(width)    
  :
  : "memory", "cc", "eax"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}
#endif  

#ifdef HAS_MIRRORROW_SSSE3

CONST uvec8 kShuffleMirror = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "lea       -0x10(%0),%0                    \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0,%2),%%xmm0                  \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(temp_width)  
  : "m"(kShuffleMirror) 
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_MIRRORROW_SSE2
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "lea       -0x10(%0),%0                    \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0,%2),%%xmm0                  \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "psllw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "pshuflw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufd    $0x4e,%%xmm0,%%xmm0             \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(temp_width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_MIRRORROW_UV_SSSE3

CONST uvec8 kShuffleMirrorUV = {
  14u, 12u, 10u, 8u, 6u, 4u, 2u, 0u, 15u, 13u, 11u, 9u, 7u, 5u, 3u, 1u
};
void MirrorRowUV_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
                       int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "movdqa    %4,%%xmm1                       \n"
    "lea       -16(%0,%3,2),%0                 \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "lea       -16(%0),%0                      \n"
    "pshufb    %%xmm1,%%xmm0                   \n"
    "sub       $8,%3                           \n"
    "movlpd    %%xmm0,(%1)                     \n"
    "movhpd    %%xmm0,(%1,%2)                  \n"
    "lea       8(%1),%1                        \n"
    "jg        1b                              \n"
  : "+r"(src),      
    "+r"(dst_u),    
    "+r"(dst_v),    
    "+r"(temp_width)  
  : "m"(kShuffleMirrorUV)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_ARGBMIRRORROW_SSSE3

CONST uvec8 kARGBShuffleMirror = {
  12u, 13u, 14u, 15u, 8u, 9u, 10u, 11u, 4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u
};

void ARGBMirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = static_cast<intptr_t>(width);
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "lea       -0x10(%0),%0                    \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0,%2,4),%%xmm0                \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(temp_width)  
  : "m"(kARGBShuffleMirror)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_SPLITUV_SSE2
void SplitUV_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb    %%xmm5,%%xmm5                    \n"
    "psrlw      $0x8,%%xmm5                      \n"
    "sub        %1,%2                            \n"
    ".p2align  4                               \n"
  "1:                                            \n"
    "movdqa     (%0),%%xmm0                      \n"
    "movdqa     0x10(%0),%%xmm1                  \n"
    "lea        0x20(%0),%0                      \n"
    "movdqa     %%xmm0,%%xmm2                    \n"
    "movdqa     %%xmm1,%%xmm3                    \n"
    "pand       %%xmm5,%%xmm0                    \n"
    "pand       %%xmm5,%%xmm1                    \n"
    "packuswb   %%xmm1,%%xmm0                    \n"
    "psrlw      $0x8,%%xmm2                      \n"
    "psrlw      $0x8,%%xmm3                      \n"
    "packuswb   %%xmm3,%%xmm2                    \n"
    "movdqa     %%xmm0,(%1)                      \n"
    "movdqa     %%xmm2,(%1,%2)                   \n"
    "lea        0x10(%1),%1                      \n"
    "sub        $0x10,%3                         \n"
    "jg         1b                               \n"
  : "+r"(src_uv),     
    "+r"(dst_u),      
    "+r"(dst_v),      
    "+r"(pix)         
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_COPYROW_SSE2
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  asm volatile (
    "sub        %0,%1                          \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    %%xmm0,(%0,%1)                  \n"
    "movdqa    %%xmm1,0x10(%0,%1)              \n"
    "lea       0x20(%0),%0                     \n"
    "sub       $0x20,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(count)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_COPYROW_X86
void CopyRow_X86(const uint8* src, uint8* dst, int width) {
  size_t width_tmp = static_cast<size_t>(width);
  asm volatile (
    "shr       $0x2,%2                         \n"
    "rep movsl                                 \n"
  : "+S"(src),  
    "+D"(dst),  
    "+c"(width_tmp) 
  :
  : "memory", "cc"
  );
}
#endif  

#ifdef HAS_YUY2TOYROW_SSE2
void YUY2ToYRow_SSE2(const uint8* src_yuy2, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void YUY2ToUVRow_SSE2(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    (%0,%4,1),%%xmm2                \n"
    "movdqa    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"(static_cast<intptr_t>(stride_yuy2))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void YUY2ToUV422Row_SSE2(const uint8* src_yuy2,
                         uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void YUY2ToUVRow_Unaligned_SSE2(const uint8* src_yuy2,
                                int stride_yuy2,
                                uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    (%0,%4,1),%%xmm2                \n"
    "movdqu    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"(static_cast<intptr_t>(stride_yuy2))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void YUY2ToUV422Row_Unaligned_SSE2(const uint8* src_yuy2,
                                   uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void UYVYToYRow_SSE2(const uint8* src_uyvy, uint8* dst_y, int pix) {
  asm volatile (
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void UYVYToUVRow_SSE2(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    (%0,%4,1),%%xmm2                \n"
    "movdqa    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"(static_cast<intptr_t>(stride_uyvy))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void UYVYToUV422Row_SSE2(const uint8* src_uyvy,
                         uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  asm volatile (
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),  
    "+r"(dst_y),     
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}

void UYVYToUVRow_Unaligned_SSE2(const uint8* src_uyvy, int stride_uyvy,
                                uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "movdqu    (%0,%4,1),%%xmm2                \n"
    "movdqu    0x10(%0,%4,1),%%xmm3            \n"
    "lea       0x20(%0),%0                     \n"
    "pavgb     %%xmm2,%%xmm0                   \n"
    "pavgb     %%xmm3,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"(static_cast<intptr_t>(stride_uyvy))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void UYVYToUV422Row_Unaligned_SSE2(const uint8* src_uyvy,
                                   uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    "sub       %1,%2                           \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqu    (%0),%%xmm0                     \n"
    "movdqu    0x10(%0),%%xmm1                 \n"
    "lea       0x20(%0),%0                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0,(%1)                     \n"
    "movq      %%xmm1,(%1,%2)                  \n"
    "lea       0x8(%1),%1                      \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBBLENDROW_SSE2

void ARGBBlendRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                       uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0xf,%%xmm7                     \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x8,%%xmm6                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psllw     $0x8,%%xmm5                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "sub       $0x1,%3                         \n"
    "je        91f                             \n"
    "jl        99f                             \n"

    
  "10:                                         \n"
    "test      $0xf,%2                         \n"
    "je        19f                             \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      (%1),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      (%1),%%xmm1                     \n"
    "lea       0x4(%1),%1                      \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jge       10b                             \n"

  "19:                                         \n"
    "add       $1-4,%3                         \n"
    "jl        49f                             \n"

    
    ".p2align  2                               \n"
  "41:                                         \n"
    "movdqu    (%0),%%xmm3                     \n"
    "lea       0x10(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    (%1),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    (%1),%%xmm1                     \n"
    "lea       0x10(%1),%1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,(%2)                     \n"
    "lea       0x10(%2),%2                     \n"
    "jge       41b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        99f                             \n"

    
  "91:                                         \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      (%1),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      (%1),%%xmm1                     \n"
    "lea       0x4(%1),%1                      \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jge       91b                             \n"
  "99:                                         \n"
  : "+r"(src_argb0),    
    "+r"(src_argb1),    
    "+r"(dst_argb),     
    "+r"(width)         
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_ARGBBLENDROW_SSSE3

CONST uvec8 kShuffleAlpha = {
  3u, 0x80, 3u, 0x80, 7u, 0x80, 7u, 0x80,
  11u, 0x80, 11u, 0x80, 15u, 0x80, 15u, 0x80
};











void ARGBBlendRow_SSSE3(const uint8* src_argb0, const uint8* src_argb1,
                        uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm7,%%xmm7                   \n"
    "psrlw     $0xf,%%xmm7                     \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrlw     $0x8,%%xmm6                     \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psllw     $0x8,%%xmm5                     \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "sub       $0x1,%3                         \n"
    "je        91f                             \n"
    "jl        99f                             \n"

    
  "10:                                         \n"
    "test      $0xf,%2                         \n"
    "je        19f                             \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      (%1),%%xmm2                     \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      (%1),%%xmm1                     \n"
    "lea       0x4(%1),%1                      \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jge       10b                             \n"

  "19:                                         \n"
    "add       $1-4,%3                         \n"
    "jl        49f                             \n"
    "test      $0xf,%0                         \n"
    "jne       41f                             \n"
    "test      $0xf,%1                         \n"
    "jne       41f                             \n"

    
    ".p2align  2                               \n"
  "40:                                         \n"
    "movdqa    (%0),%%xmm3                     \n"
    "lea       0x10(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqa    (%1),%%xmm2                     \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqa    (%1),%%xmm1                     \n"
    "lea       0x10(%1),%1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,(%2)                     \n"
    "lea       0x10(%2),%2                     \n"
    "jge       40b                             \n"
    "jmp       49f                             \n"

    
    ".p2align  2                               \n"
  "41:                                         \n"
    "movdqu    (%0),%%xmm3                     \n"
    "lea       0x10(%0),%0                     \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    (%1),%%xmm2                     \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    (%1),%%xmm1                     \n"
    "lea       0x10(%1),%1                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0,(%2)                     \n"
    "lea       0x10(%2),%2                     \n"
    "jge       41b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        99f                             \n"

    
  "91:                                         \n"
    "movd      (%0),%%xmm3                     \n"
    "lea       0x4(%0),%0                      \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      (%1),%%xmm2                     \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      (%1),%%xmm1                     \n"
    "lea       0x4(%1),%1                      \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jge       91b                             \n"
  "99:                                         \n"
  : "+r"(src_argb0),    
    "+r"(src_argb1),    
    "+r"(dst_argb),     
    "+r"(width)         
  : "m"(kShuffleAlpha)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_ARGBATTENUATE_SSE2


void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrld     $0x8,%%xmm5                     \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "pshufhw   $0xff,%%xmm0,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "pshufhw   $0xff,%%xmm1,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+r"(width)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBATTENUATEROW_SSSE3

CONST uvec8 kShuffleAlpha0 = {
  3u, 3u, 3u, 3u, 3u, 3u, 128u, 128u, 7u, 7u, 7u, 7u, 7u, 7u, 128u, 128u,
};
CONST uvec8 kShuffleAlpha1 = {
  11u, 11u, 11u, 11u, 11u, 11u, 128u, 128u,
  15u, 15u, 15u, 15u, 15u, 15u, 128u, 128u,
};


void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "pslld     $0x18,%%xmm3                    \n"
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "punpcklbw %%xmm1,%%xmm1                   \n"
    "pmulhuw   %%xmm1,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "punpckhbw %%xmm2,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "pand      %%xmm3,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+r"(width)        
  : "m"(kShuffleAlpha0),  
    "m"(kShuffleAlpha1)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBUNATTENUATEROW_SSE2


void ARGBUnattenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  uintptr_t alpha = 0;
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movzb     0x3(%0),%3                      \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movd      0x0(%4,%3,4),%%xmm2             \n"
    "movzb     0x7(%0),%3                      \n"
    "movd      0x0(%4,%3,4),%%xmm3             \n"
    "pshuflw   $0xc0,%%xmm2,%%xmm2             \n"
    "pshuflw   $0xc0,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "movzb     0xb(%0),%3                      \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "movd      0x0(%4,%3,4),%%xmm2             \n"
    "movzb     0xf(%0),%3                      \n"
    "movd      0x0(%4,%3,4),%%xmm3             \n"
    "pshuflw   $0xc0,%%xmm2,%%xmm2             \n"
    "pshuflw   $0xc0,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+r"(width),       
    "+r"(alpha)        
  : "r"(fixed_invtbl8)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBGRAYROW_SSSE3

CONST vec8 kARGBToGray = {
  14, 76, 38, 0, 14, 76, 38, 0, 14, 76, 38, 0, 14, 76, 38, 0
};


void ARGBGrayRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "movdqa    %3,%%xmm4                       \n"
    "sub       %0,%1                           \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movdqa    (%0),%%xmm2                     \n"
    "movdqa    0x10(%0),%%xmm3                 \n"
    "psrld     $0x18,%%xmm2                    \n"
    "psrld     $0x18,%%xmm3                    \n"
    "packuswb  %%xmm3,%%xmm2                   \n"
    "packuswb  %%xmm2,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm3                   \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "punpcklbw %%xmm2,%%xmm3                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm3,%%xmm0                   \n"
    "punpckhwd %%xmm3,%%xmm1                   \n"
    "sub       $0x8,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "movdqa    %%xmm1,0x10(%0,%1,1)            \n"
    "lea       0x20(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),   
    "+r"(dst_argb),   
    "+r"(width)       
  : "m"(kARGBToGray)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSEPIAROW_SSSE3




CONST vec8 kARGBToSepiaB = {
  17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0
};

CONST vec8 kARGBToSepiaG = {
  22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0
};

CONST vec8 kARGBToSepiaR = {
  24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0
};


void ARGBSepiaRow_SSSE3(uint8* dst_argb, int width) {
  asm volatile (
    "movdqa    %2,%%xmm2                       \n"
    "movdqa    %3,%%xmm3                       \n"
    "movdqa    %4,%%xmm4                       \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm6                 \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "pmaddubsw %%xmm2,%%xmm6                   \n"
    "phaddw    %%xmm6,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movdqa    (%0),%%xmm5                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "pmaddubsw %%xmm3,%%xmm5                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm5                   \n"
    "psrlw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    (%0),%%xmm5                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "pmaddubsw %%xmm4,%%xmm5                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm5                   \n"
    "psrlw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "movdqa    (%0),%%xmm6                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "psrld     $0x18,%%xmm6                    \n"
    "psrld     $0x18,%%xmm1                    \n"
    "packuswb  %%xmm1,%%xmm6                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "punpcklbw %%xmm6,%%xmm5                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm5,%%xmm0                   \n"
    "punpckhwd %%xmm5,%%xmm1                   \n"
    "sub       $0x8,%1                         \n"
    "movdqa    %%xmm0,(%0)                     \n"
    "movdqa    %%xmm1,0x10(%0)                 \n"
    "lea       0x20(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(dst_argb),      
    "+r"(width)          
  : "m"(kARGBToSepiaB),  
    "m"(kARGBToSepiaG),  
    "m"(kARGBToSepiaR)   
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}
#endif  

#ifdef HAS_ARGBCOLORMATRIXROW_SSSE3


void ARGBColorMatrixRow_SSSE3(uint8* dst_argb, const int8* matrix_argb,
                              int width) {
  asm volatile (
    "movd      (%2),%%xmm2                     \n"
    "movd      0x4(%2),%%xmm3                  \n"
    "movd      0x8(%2),%%xmm4                  \n"
    "pshufd    $0x0,%%xmm2,%%xmm2              \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "pshufd    $0x0,%%xmm4,%%xmm4              \n"

    
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm6                 \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "pmaddubsw %%xmm2,%%xmm6                   \n"
    "movdqa    (%0),%%xmm5                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "pmaddubsw %%xmm3,%%xmm5                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "phaddsw   %%xmm6,%%xmm0                   \n"
    "phaddsw   %%xmm1,%%xmm5                   \n"
    "psraw     $0x7,%%xmm0                     \n"
    "psraw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    (%0),%%xmm5                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "pmaddubsw %%xmm4,%%xmm5                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "phaddsw   %%xmm1,%%xmm5                   \n"
    "psraw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "movdqa    (%0),%%xmm6                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "psrld     $0x18,%%xmm6                    \n"
    "psrld     $0x18,%%xmm1                    \n"
    "packuswb  %%xmm1,%%xmm6                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm6,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm0                   \n"
    "punpckhwd %%xmm5,%%xmm1                   \n"
    "sub       $0x8,%1                         \n"
    "movdqa    %%xmm0,(%0)                     \n"
    "movdqa    %%xmm1,0x10(%0)                 \n"
    "lea       0x20(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(dst_argb),      
    "+r"(width)          
  : "r"(matrix_argb)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}
#endif  

#ifdef HAS_ARGBQUANTIZEROW_SSE2


void ARGBQuantizeRow_SSE2(uint8* dst_argb, int scale, int interval_size,
                          int interval_offset, int width) {
  asm volatile (
    "movd      %2,%%xmm2                       \n"
    "movd      %3,%%xmm3                       \n"
    "movd      %4,%%xmm4                       \n"
    "pshuflw   $0x40,%%xmm2,%%xmm2             \n"
    "pshufd    $0x44,%%xmm2,%%xmm2             \n"
    "pshuflw   $0x40,%%xmm3,%%xmm3             \n"
    "pshufd    $0x44,%%xmm3,%%xmm3             \n"
    "pshuflw   $0x40,%%xmm4,%%xmm4             \n"
    "pshufd    $0x44,%%xmm4,%%xmm4             \n"
    "pxor      %%xmm5,%%xmm5                   \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "pslld     $0x18,%%xmm6                    \n"

    
    ".p2align  2                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    (%0),%%xmm1                     \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "pmullw    %%xmm3,%%xmm0                   \n"
    "movdqa    (%0),%%xmm7                     \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "pand      %%xmm6,%%xmm7                   \n"
    "paddw     %%xmm4,%%xmm0                   \n"
    "paddw     %%xmm4,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm7,%%xmm0                   \n"
    "sub       $0x4,%1                         \n"
    "movdqa    %%xmm0,(%0)                     \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(dst_argb),       
    "+r"(width)           
  : "r"(scale),           
    "r"(interval_size),   
    "r"(interval_offset)  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_COMPUTECUMULATIVESUMROW_SSE2


void ComputeCumulativeSumRow_SSE2(const uint8* row, int32* cumsum,
                                  const int32* previous_cumsum, int width) {
  asm volatile (
    "sub       %1,%2                           \n"
    "pxor      %%xmm0,%%xmm0                   \n"
    "pxor      %%xmm1,%%xmm1                   \n"
    "sub       $0x4,%3                         \n"
    "jl        49f                             \n"
    "test      $0xf,%1                         \n"
    "jne       49f                             \n"

  
    ".p2align  2                               \n"
  "40:                                         \n"
    "movdqu    (%0),%%xmm2                     \n"
    "lea       0x10(%0),%0                     \n"
    "movdqa    %%xmm2,%%xmm4                   \n"
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "punpcklwd %%xmm1,%%xmm2                   \n"
    "punpckhwd %%xmm1,%%xmm3                   \n"
    "punpckhbw %%xmm1,%%xmm4                   \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "punpcklwd %%xmm1,%%xmm4                   \n"
    "punpckhwd %%xmm1,%%xmm5                   \n"
    "paddd     %%xmm2,%%xmm0                   \n"
    "movdqa    (%1,%2,1),%%xmm2                \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "movdqa    0x10(%1,%2,1),%%xmm3            \n"
    "paddd     %%xmm0,%%xmm3                   \n"
    "paddd     %%xmm4,%%xmm0                   \n"
    "movdqa    0x20(%1,%2,1),%%xmm4            \n"
    "paddd     %%xmm0,%%xmm4                   \n"
    "paddd     %%xmm5,%%xmm0                   \n"
    "movdqa    0x30(%1,%2,1),%%xmm5            \n"
    "paddd     %%xmm0,%%xmm5                   \n"
    "movdqa    %%xmm2,(%1)                     \n"
    "movdqa    %%xmm3,0x10(%1)                 \n"
    "movdqa    %%xmm4,0x20(%1)                 \n"
    "movdqa    %%xmm5,0x30(%1)                 \n"
    "lea       0x40(%1),%1                     \n"
    "sub       $0x4,%3                         \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        19f                             \n"

  
    ".p2align  2                               \n"
  "10:                                         \n"
    "movd      (%0),%%xmm2                     \n"
    "lea       0x4(%0),%0                      \n"
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklwd %%xmm1,%%xmm2                   \n"
    "paddd     %%xmm2,%%xmm0                   \n"
    "movdqu    (%1,%2,1),%%xmm2                \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "movdqu    %%xmm2,(%1)                     \n"
    "lea       0x10(%1),%1                     \n"
    "sub       $0x1,%3                         \n"
    "jge       10b                             \n"

  "19:                                         \n"
  : "+r"(row),  
    "+r"(cumsum),  
    "+r"(previous_cumsum),  
    "+r"(width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_CUMULATIVESUMTOAVERAGE_SSE2
void CumulativeSumToAverage_SSE2(const int32* topleft, const int32* botleft,
                                 int width, int area, uint8* dst, int count) {
  asm volatile (
    "movd      %5,%%xmm4                       \n"
    "cvtdq2ps  %%xmm4,%%xmm4                   \n"
    "rcpss     %%xmm4,%%xmm4                   \n"
    "pshufd    $0x0,%%xmm4,%%xmm4              \n"
    "sub       $0x4,%3                         \n"
    "jl        49f                             \n"

  
    ".p2align  2                               \n"
  "40:                                         \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    0x10(%0),%%xmm1                 \n"
    "movdqa    0x20(%0),%%xmm2                 \n"
    "movdqa    0x30(%0),%%xmm3                 \n"
    "psubd     (%0,%4,4),%%xmm0                \n"
    "psubd     0x10(%0,%4,4),%%xmm1            \n"
    "psubd     0x20(%0,%4,4),%%xmm2            \n"
    "psubd     0x30(%0,%4,4),%%xmm3            \n"
    "lea       0x40(%0),%0                     \n"
    "psubd     (%1),%%xmm0                     \n"
    "psubd     0x10(%1),%%xmm1                 \n"
    "psubd     0x20(%1),%%xmm2                 \n"
    "psubd     0x30(%1),%%xmm3                 \n"
    "paddd     (%1,%4,4),%%xmm0                \n"
    "paddd     0x10(%1,%4,4),%%xmm1            \n"
    "paddd     0x20(%1,%4,4),%%xmm2            \n"
    "paddd     0x30(%1,%4,4),%%xmm3            \n"
    "lea       0x40(%1),%1                     \n"
    "cvtdq2ps  %%xmm0,%%xmm0                   \n"
    "cvtdq2ps  %%xmm1,%%xmm1                   \n"
    "mulps     %%xmm4,%%xmm0                   \n"
    "mulps     %%xmm4,%%xmm1                   \n"
    "cvtdq2ps  %%xmm2,%%xmm2                   \n"
    "cvtdq2ps  %%xmm3,%%xmm3                   \n"
    "mulps     %%xmm4,%%xmm2                   \n"
    "mulps     %%xmm4,%%xmm3                   \n"
    "cvtps2dq  %%xmm0,%%xmm0                   \n"
    "cvtps2dq  %%xmm1,%%xmm1                   \n"
    "cvtps2dq  %%xmm2,%%xmm2                   \n"
    "cvtps2dq  %%xmm3,%%xmm3                   \n"
    "packssdw  %%xmm1,%%xmm0                   \n"
    "packssdw  %%xmm3,%%xmm2                   \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "movdqu    %%xmm0,(%2)                     \n"
    "lea       0x10(%2),%2                     \n"
    "sub       $0x4,%3                         \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        19f                             \n"

  
    ".p2align  2                               \n"
  "10:                                         \n"
    "movdqa    (%0),%%xmm0                     \n"
    "psubd     (%0,%4,4),%%xmm0                \n"
    "lea       0x10(%0),%0                     \n"
    "psubd     (%1),%%xmm0                     \n"
    "paddd     (%1,%4,4),%%xmm0                \n"
    "lea       0x10(%1),%1                     \n"
    "cvtdq2ps  %%xmm0,%%xmm0                   \n"
    "mulps     %%xmm4,%%xmm0                   \n"
    "cvtps2dq  %%xmm0,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "sub       $0x1,%3                         \n"
    "jge       10b                             \n"
  "19:                                         \n"
  : "+r"(topleft),  
    "+r"(botleft),  
    "+r"(dst),      
    "+rm"(count)    
  : "r"(static_cast<intptr_t>(width)),  
    "rm"(area)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4"
#endif
  );
}
#endif  
#ifdef HAS_ARGBSHADE_SSE2


void ARGBShadeRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width,
                       uint32 value) {
  asm volatile (
    "movd      %3,%%xmm2                       \n"
    "sub       %0,%1                           \n"
    "punpcklbw %%xmm2,%%xmm2                   \n"
    "punpcklqdq %%xmm2,%%xmm2                  \n"

    
    ".p2align  2                               \n"
  "1:                                          \n"
    "movdqa    (%0),%%xmm0                     \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%0,%1,1)                \n"
    "lea       0x10(%0),%0                     \n"
    "jg        1b                              \n"
  : "+r"(src_argb),       
    "+r"(dst_argb),       
    "+r"(width)           
  : "r"(value)            
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
  );
}
#endif  

#ifdef HAS_ARGBAFFINEROW_SSE2






LIBYUV_API
void ARGBAffineRow_SSE2(const uint8* src_argb, int src_argb_stride,
                        uint8* dst_argb, const float* uv_dudv, int width) {
  intptr_t src_argb_stride_temp = src_argb_stride;
  intptr_t temp = 0;
  asm volatile (
    "movq      (%3),%%xmm2                     \n"
    "movq      0x8(%3),%%xmm7                  \n"
    "shl       $0x10,%1                        \n"
    "add       $0x4,%1                         \n"
    "movd      %1,%%xmm5                       \n"
    "sub       $0x4,%4                         \n"
    "jl        49f                             \n"

    "pshufd    $0x44,%%xmm7,%%xmm7             \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "movdqa    %%xmm2,%%xmm0                   \n"
    "addps     %%xmm7,%%xmm0                   \n"
    "movlhps   %%xmm0,%%xmm2                   \n"
    "movdqa    %%xmm7,%%xmm4                   \n"
    "addps     %%xmm4,%%xmm4                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "addps     %%xmm4,%%xmm3                   \n"
    "addps     %%xmm4,%%xmm4                   \n"

  
    ".p2align  4                               \n"
  "40:                                         \n"
    "cvttps2dq %%xmm2,%%xmm0                   \n"
    "cvttps2dq %%xmm3,%%xmm1                   \n"
    "packssdw  %%xmm1,%%xmm0                   \n"
    "pmaddwd   %%xmm5,%%xmm0                   \n"
#if defined(__x86_64__)
    "movd      %%xmm0,%1                       \n"
    "mov       %1,%5                           \n"
    "and       $0x0fffffff,%1                  \n"
    "shr       $32,%5                          \n"
    "pshufd    $0xEE,%%xmm0,%%xmm0             \n"
#else
    "movd      %%xmm0,%1                       \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
    "movd      %%xmm0,%5                       \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
#endif
    "movd      (%0,%1,1),%%xmm1                \n"
    "movd      (%0,%5,1),%%xmm6                \n"
    "punpckldq %%xmm6,%%xmm1                   \n"
    "addps     %%xmm4,%%xmm2                   \n"
    "movq      %%xmm1,(%2)                     \n"
#if defined(__x86_64__)
    "movd      %%xmm0,%1                       \n"
    "mov       %1,%5                           \n"
    "and       $0x0fffffff,%1                  \n"
    "shr       $32,%5                          \n"
#else
    "movd      %%xmm0,%1                       \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
    "movd      %%xmm0,%5                       \n"
#endif
    "movd      (%0,%1,1),%%xmm0                \n"
    "movd      (%0,%5,1),%%xmm6                \n"
    "punpckldq %%xmm6,%%xmm0                   \n"
    "addps     %%xmm4,%%xmm3                   \n"
    "sub       $0x4,%4                         \n"
    "movq      %%xmm0,0x08(%2)                 \n"
    "lea       0x10(%2),%2                     \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%4                         \n"
    "jl        19f                             \n"

  
    ".p2align  4                               \n"
  "10:                                         \n"
    "cvttps2dq %%xmm2,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "pmaddwd   %%xmm5,%%xmm0                   \n"
    "addps     %%xmm7,%%xmm2                   \n"
    "movd      %%xmm0,%1                       \n"
#if defined(__x86_64__)
    "and       $0x0fffffff,%1                  \n"
#endif
    "movd      (%0,%1,1),%%xmm0                \n"
    "sub       $0x1,%4                         \n"
    "movd      %%xmm0,(%2)                     \n"
    "lea       0x4(%2),%2                      \n"
    "jge       10b                             \n"
  "19:                                         \n"
  : "+r"(src_argb),  
    "+r"(src_argb_stride_temp),  
    "+r"(dst_argb),  
    "+r"(uv_dudv),   
    "+rm"(width),    
    "+r"(temp)   
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  


void ARGBInterpolateRow_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                              ptrdiff_t src_stride, int dst_width,
                              int source_y_fraction) {
  asm volatile (
    "sub       %1,%0                           \n"
    "shr       %3                              \n"
    "cmp       $0x0,%3                         \n"
    "je        2f                              \n"
    "cmp       $0x40,%3                        \n"
    "je        3f                              \n"
    "movd      %3,%%xmm0                       \n"
    "neg       %3                              \n"
    "add       $0x80,%3                        \n"
    "movd      %3,%%xmm5                       \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm5                   \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    ".p2align  4                               \n"
  "1:                                          \n"
    "movdqa    (%1),%%xmm0                     \n"
    "movdqa    (%1,%4,1),%%xmm2                \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm0                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "pmaddubsw %%xmm5,%%xmm0                   \n"
    "pmaddubsw %%xmm5,%%xmm1                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%1,%0,1)                \n"
    "lea       0x10(%1),%1                     \n"
    "jg        1b                              \n"
    "jmp       4f                              \n"
    ".p2align  4                               \n"
  "2:                                          \n"
    "movdqa    (%1),%%xmm0                     \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%1,%0,1)                \n"
    "lea       0x10(%1),%1                     \n"
    "jg        2b                              \n"
    "jmp       4f                              \n"
    ".p2align  4                               \n"
  "3:                                          \n"
    "movdqa    (%1),%%xmm0                     \n"
    "pavgb     (%1,%4,1),%%xmm0                \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0,(%1,%0,1)                \n"
    "lea       0x10(%1),%1                     \n"
    "jg        3b                              \n"
  "4:                                          \n"
    ".p2align  4                               \n"
  : "+r"(dst_ptr),     
    "+r"(src_ptr),     
    "+r"(dst_width),   
    "+r"(source_y_fraction)  
  : "r"(static_cast<intptr_t>(src_stride))  
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm5"
#endif
  );
}

#endif  

#ifdef __cplusplus
}  
}  
#endif
