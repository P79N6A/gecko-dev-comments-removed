









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_X86) && (defined(__x86_64__) || defined(__i386__))

#if defined(HAS_ARGBTOYROW_SSSE3) || defined(HAS_ARGBGRAYROW_SSSE3)


static vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};


static vec8 kARGBToYJ = {
  15, 75, 38, 0, 15, 75, 38, 0, 15, 75, 38, 0, 15, 75, 38, 0
};
#endif  

#if defined(HAS_ARGBTOYROW_SSSE3) || defined(HAS_I422TOARGBROW_SSSE3)

static vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

static vec8 kARGBToUJ = {
  127, -84, -43, 0, 127, -84, -43, 0, 127, -84, -43, 0, 127, -84, -43, 0
};

static vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

static vec8 kARGBToVJ = {
  -20, -107, 127, 0, -20, -107, 127, 0, -20, -107, 127, 0, -20, -107, 127, 0
};


static vec8 kBGRAToY = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

static vec8 kBGRAToU = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

static vec8 kBGRAToV = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};


static vec8 kABGRToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

static vec8 kABGRToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

static vec8 kABGRToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};


static vec8 kRGBAToY = {
  0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33
};

static vec8 kRGBAToU = {
  0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38
};

static vec8 kRGBAToV = {
  0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112
};

static uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

static vec16 kAddYJ64 = {
  64, 64, 64, 64, 64, 64, 64, 64
};

static uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

static uvec16 kAddUVJ128 = {
  0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u
};
#endif  

#ifdef HAS_RGB24TOARGBROW_SSSE3


static uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};


static uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};


static uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};


static uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};


static uvec8 kShuffleMaskARGBToRGB24_0 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 128u, 128u, 128u, 128u, 10u, 12u, 13u, 14u
};


static uvec8 kShuffleMaskARGBToRAW_0 = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 128u, 128u, 128u, 128u, 8u, 14u, 13u, 12u
};
#endif  

#if defined(TESTING) && defined(__x86_64__)
void TestRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  asm volatile (
    ".p2align  5                               \n"
    "mov       %%eax,%%eax                     \n"
    "mov       %%ebx,%%ebx                     \n"
    "mov       %%ecx,%%ecx                     \n"
    "mov       %%edx,%%edx                     \n"
    "mov       %%esi,%%esi                     \n"
    "mov       %%edi,%%edi                     \n"
    "mov       %%ebp,%%ebp                     \n"
    "mov       %%esp,%%esp                     \n"
    ".p2align  5                               \n"
    "mov       %%r8d,%%r8d                     \n"
    "mov       %%r9d,%%r9d                     \n"
    "mov       %%r10d,%%r10d                   \n"
    "mov       %%r11d,%%r11d                   \n"
    "mov       %%r12d,%%r12d                   \n"
    "mov       %%r13d,%%r13d                   \n"
    "mov       %%r14d,%%r14d                   \n"
    "mov       %%r15d,%%r15d                   \n"
    ".p2align  5                               \n"
    "lea       (%%rax),%%eax                   \n"
    "lea       (%%rbx),%%ebx                   \n"
    "lea       (%%rcx),%%ecx                   \n"
    "lea       (%%rdx),%%edx                   \n"
    "lea       (%%rsi),%%esi                   \n"
    "lea       (%%rdi),%%edi                   \n"
    "lea       (%%rbp),%%ebp                   \n"
    "lea       (%%rsp),%%esp                   \n"
    ".p2align  5                               \n"
    "lea       (%%r8),%%r8d                    \n"
    "lea       (%%r9),%%r9d                    \n"
    "lea       (%%r10),%%r10d                  \n"
    "lea       (%%r11),%%r11d                  \n"
    "lea       (%%r12),%%r12d                  \n"
    "lea       (%%r13),%%r13d                  \n"
    "lea       (%%r14),%%r14d                  \n"
    "lea       (%%r15),%%r15d                  \n"

    ".p2align  5                               \n"
    "lea       0x10(%%rax),%%eax               \n"
    "lea       0x10(%%rbx),%%ebx               \n"
    "lea       0x10(%%rcx),%%ecx               \n"
    "lea       0x10(%%rdx),%%edx               \n"
    "lea       0x10(%%rsi),%%esi               \n"
    "lea       0x10(%%rdi),%%edi               \n"
    "lea       0x10(%%rbp),%%ebp               \n"
    "lea       0x10(%%rsp),%%esp               \n"
    ".p2align  5                               \n"
    "lea       0x10(%%r8),%%r8d                \n"
    "lea       0x10(%%r9),%%r9d                \n"
    "lea       0x10(%%r10),%%r10d              \n"
    "lea       0x10(%%r11),%%r11d              \n"
    "lea       0x10(%%r12),%%r12d              \n"
    "lea       0x10(%%r13),%%r13d              \n"
    "lea       0x10(%%r14),%%r14d              \n"
    "lea       0x10(%%r15),%%r15d              \n"

    ".p2align  5                               \n"
    "add       0x10,%%eax                      \n"
    "add       0x10,%%ebx                      \n"
    "add       0x10,%%ecx                      \n"
    "add       0x10,%%edx                      \n"
    "add       0x10,%%esi                      \n"
    "add       0x10,%%edi                      \n"
    "add       0x10,%%ebp                      \n"
    "add       0x10,%%esp                      \n"
    ".p2align  5                               \n"
    "add       0x10,%%r8d                      \n"
    "add       0x10,%%r9d                      \n"
    "add       0x10,%%r10d                     \n"
    "add       0x10,%%r11d                     \n"
    "add       0x10,%%r12d                     \n"
    "add       0x10,%%r13d                     \n"
    "add       0x10,%%r14d                     \n"
    "add       0x10,%%r15d                     \n"

    ".p2align  2                               \n"
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
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
#endif  

#ifdef HAS_I400TOARGBROW_SSE2
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "por       %%xmm5,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
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

void I400ToARGBRow_Unaligned_SSE2(const uint8* src_y, uint8* dst_argb,
                                  int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "por       %%xmm5,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
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
#endif  

#ifdef HAS_RGB24TOARGBROW_SSSE3
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"  
    "pslld     $0x18,%%xmm5                    \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x30,0) ",%0           \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2," MEMACCESS2(0x20,1) "   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3," MEMACCESS2(0x30,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x30,0) ",%0           \n"
    "movdqa    %%xmm3,%%xmm2                   \n"
    "palignr   $0x8,%%xmm1,%%xmm2              \n"
    "pshufb    %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqa    %%xmm2," MEMACCESS2(0x20,1) "   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "por       %%xmm5,%%xmm1                   \n"
    "palignr   $0x4,%%xmm3,%%xmm3              \n"
    "pshufb    %%xmm4,%%xmm3                   \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "por       %%xmm5,%%xmm3                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm3," MEMACCESS2(0x30,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
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
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm1,0x00,1,0,2)           
    MEMOPMEM(movdqa,xmm2,0x10,1,0,2)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
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
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm1,0x00,1,0,2)           
    MEMOPMEM(movdqa,xmm2,0x10,1,0,2)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
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
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,2)           
    MEMOPMEM(movdqa,xmm1,0x10,1,0,2)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(pix)   
  :
  : "memory", "cc", "eax"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToRGB24Row_SSSE3(const uint8* src, uint8* dst, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm6                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "movdqu    %%xmm2," MEMACCESS2(0x20,1) "   \n"
    "lea       " MEMLEA(0x30,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "por       %%xmm5,%%xmm1                   \n"
    "psrldq    $0x8,%%xmm2                     \n"
    "pslldq    $0x4,%%xmm3                     \n"
    "por       %%xmm3,%%xmm2                   \n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "movdqu    %%xmm2," MEMACCESS2(0x20,1) "   \n"
    "lea       " MEMLEA(0x30,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
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
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
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
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMACCESS2(0x8,1) ",%1        \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm3,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm1                   \n"
    "psrlq     $0x4,%%xmm0                     \n"
    "psrlq     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
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
#endif  

#ifdef HAS_ARGBTOYROW_SSSE3
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
#endif  

#ifdef HAS_ARGBTOYJROW_SSSE3
void ARGBToYJRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "paddw     %%xmm5,%%xmm0                   \n"
    "paddw     %%xmm5,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kARGBToYJ),  
    "m"(kAddYJ64)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void ARGBToYJRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "paddw     %%xmm5,%%xmm0                   \n"
    "paddw     %%xmm5,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kARGBToYJ),  
    "m"(kAddYJ64)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBTOUVROW_SSSE3





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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_argb)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}


void ARGBToUVJRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToUJ),  
    "m"(kARGBToVJ),  
    "m"(kAddUVJ128)  
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "paddw     %%xmm5,%%xmm0                   \n"
    "paddw     %%xmm5,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_argb)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_argb)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ARGBToUVJRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                  uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kARGBToUJ),         
    "m"(kARGBToVJ),         
    "m"(kAddUVJ128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "paddw     %%xmm5,%%xmm0                   \n"
    "paddw     %%xmm5,%%xmm1                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm1                     \n"
    "packsswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_argb))
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ARGBToUV444Row_SSSE3(const uint8* src_argb, uint8* dst_u, uint8* dst_v,
                          int width) {
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "pmaddubsw %%xmm3,%%xmm0                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),        
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6"
#endif
  );
}

void ARGBToUV444Row_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_u,
                                    uint8* dst_v, int width) {
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "pmaddubsw %%xmm3,%%xmm0                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "pmaddubsw %%xmm3,%%xmm2                   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm6,%%xmm2                   \n"
    "psraw     $0x8,%%xmm0                     \n"
    "psraw     $0x8,%%xmm2                     \n"
    "packsswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),        
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6"
#endif
  );
}

void ARGBToUV422Row_SSSE3(const uint8* src_argb0,
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ARGBToUV422Row_Unaligned_SSSE3(const uint8* src_argb0,
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void BGRAToYRow_SSSE3(const uint8* src_bgra, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_bgra)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_bgra0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_bgra)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void ABGRToYRow_SSSE3(const uint8* src_abgr, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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

void RGBAToYRow_SSSE3(const uint8* src_rgba, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kRGBAToY),   
    "m"(kAddY16)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void RGBAToYRow_Unaligned_SSSE3(const uint8* src_rgba, uint8* dst_y, int pix) {
  asm volatile (
    "movdqa    %4,%%xmm5                       \n"
    "movdqa    %3,%%xmm4                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm2                   \n"
    "pmaddubsw %%xmm4,%%xmm3                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "phaddw    %%xmm3,%%xmm2                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm2                     \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "paddb     %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_rgba),  
    "+r"(dst_y),     
    "+r"(pix)        
  : "m"(kRGBAToY),   
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_abgr)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_abgr0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_abgr)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void RGBAToUVRow_SSSE3(const uint8* src_rgba0, int src_stride_rgba,
                       uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         
    "m"(kRGBAToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(pavgb,0x00,0,4,1,xmm0)            
    MEMOPREG(pavgb,0x10,0,4,1,xmm1)            
    MEMOPREG(pavgb,0x20,0,4,1,xmm2)            
    MEMOPREG(pavgb,0x30,0,4,1,xmm6)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_rgba0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_rgba))
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}

void RGBAToUVRow_Unaligned_SSSE3(const uint8* src_rgba0, int src_stride_rgba,
                                 uint8* dst_u, uint8* dst_v, int width) {
  asm volatile (
    "movdqa    %0,%%xmm4                       \n"
    "movdqa    %1,%%xmm3                       \n"
    "movdqa    %2,%%xmm5                       \n"
  :
  : "m"(kRGBAToU),         
    "m"(kRGBAToV),         
    "m"(kAddUV128)         
  );
  asm volatile (
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqu    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqu    " MEMACCESS2(0x30,0) ",%%xmm6   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm0                   \n"
    MEMOPREG(movdqu,0x10,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm1                   \n"
    MEMOPREG(movdqu,0x20,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm2                   \n"
    MEMOPREG(movdqu,0x30,0,4,1,xmm7)           
    "pavgb     %%xmm7,%%xmm6                   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
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
    "movlps    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhps,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_rgba0),       
    "+r"(dst_u),           
    "+r"(dst_v),           
    "+rm"(width)           
  : "r"((intptr_t)(src_stride_rgba)) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_I422TOARGBROW_SSSE3
#define UB 127 /* min(63,(int8)(2.018 * 64)) */
#define UG -25 /* (int8)(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* (int8)(-0.813 * 64 - 0.5) */
#define VR 102 /* (int8)(1.596 * 64 + 0.5) */


#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

#define YG 74 /* (int8)(1.164 * 64 + 0.5) */

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
} static SIMD_ALIGNED(kYuvConstants) = {
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
    "movq       " MEMACCESS([u_buf]) ",%%xmm0                   \n"            \
    BUNDLEALIGN                                                                \
    MEMOPREG(movq, 0x00, [u_buf], [v_buf], 1, xmm1)                            \
    "lea        " MEMLEA(0x8, [u_buf]) ",%[u_buf]               \n"            \
    "punpcklbw  %%xmm1,%%xmm0                                   \n"


#define READYUV422                                                             \
    "movd       " MEMACCESS([u_buf]) ",%%xmm0                   \n"            \
    BUNDLEALIGN                                                                \
    MEMOPREG(movd, 0x00, [u_buf], [v_buf], 1, xmm1)                            \
    "lea        " MEMLEA(0x4, [u_buf]) ",%[u_buf]               \n"            \
    "punpcklbw  %%xmm1,%%xmm0                                   \n"            \
    "punpcklwd  %%xmm0,%%xmm0                                   \n"


#define READYUV411                                                             \
    "movd       " MEMACCESS([u_buf]) ",%%xmm0                   \n"            \
    BUNDLEALIGN                                                                \
    MEMOPREG(movd, 0x00, [u_buf], [v_buf], 1, xmm1)                            \
    "lea        " MEMLEA(0x2, [u_buf]) ",%[u_buf]               \n"            \
    "punpcklbw  %%xmm1,%%xmm0                                   \n"            \
    "punpcklwd  %%xmm0,%%xmm0                                   \n"            \
    "punpckldq  %%xmm0,%%xmm0                                   \n"


#define READNV12                                                               \
    "movq       " MEMACCESS([uv_buf]) ",%%xmm0                  \n"            \
    "lea        " MEMLEA(0x8, [uv_buf]) ",%[uv_buf]             \n"            \
    "punpcklwd  %%xmm0,%%xmm0                                   \n"


#define YUVTORGB                                                               \
    "movdqa     %%xmm0,%%xmm1                                   \n"            \
    "movdqa     %%xmm0,%%xmm2                                   \n"            \
    "pmaddubsw  " MEMACCESS([kYuvConstants]) ",%%xmm0           \n"            \
    "pmaddubsw  " MEMACCESS2(16, [kYuvConstants]) ",%%xmm1      \n"            \
    "pmaddubsw  " MEMACCESS2(32, [kYuvConstants]) ",%%xmm2      \n"            \
    "psubw      " MEMACCESS2(48, [kYuvConstants]) ",%%xmm0      \n"            \
    "psubw      " MEMACCESS2(64, [kYuvConstants]) ",%%xmm1      \n"            \
    "psubw      " MEMACCESS2(80, [kYuvConstants]) ",%%xmm2      \n"            \
    "movq       " MEMACCESS([y_buf]) ",%%xmm3                   \n"            \
    "lea        " MEMLEA(0x8, [y_buf]) ",%[y_buf]               \n"            \
    "punpcklbw  %%xmm4,%%xmm3                                   \n"            \
    "psubsw     " MEMACCESS2(96, [kYuvConstants]) ",%%xmm3      \n"            \
    "pmullw     " MEMACCESS2(112, [kYuvConstants]) ",%%xmm3     \n"            \
    "paddsw     %%xmm3,%%xmm0                                   \n"            \
    "paddsw     %%xmm3,%%xmm1                                   \n"            \
    "paddsw     %%xmm3,%%xmm2                                   \n"            \
    "psraw      $0x6,%%xmm0                                     \n"            \
    "psraw      $0x6,%%xmm1                                     \n"            \
    "psraw      $0x6,%%xmm2                                     \n"            \
    "packuswb   %%xmm0,%%xmm0                                   \n"            \
    "packuswb   %%xmm1,%%xmm1                                   \n"            \
    "packuswb   %%xmm2,%%xmm2                                   \n"


#define YVUTORGB                                                               \
    "movdqa     %%xmm0,%%xmm1                                   \n"            \
    "movdqa     %%xmm0,%%xmm2                                   \n"            \
    "pmaddubsw  " MEMACCESS2(128, [kYuvConstants]) ",%%xmm0     \n"            \
    "pmaddubsw  " MEMACCESS2(144, [kYuvConstants]) ",%%xmm1     \n"            \
    "pmaddubsw  " MEMACCESS2(160, [kYuvConstants]) ",%%xmm2     \n"            \
    "psubw      " MEMACCESS2(48, [kYuvConstants]) ",%%xmm0      \n"            \
    "psubw      " MEMACCESS2(64, [kYuvConstants]) ",%%xmm1      \n"            \
    "psubw      " MEMACCESS2(80, [kYuvConstants]) ",%%xmm2      \n"            \
    "movq       " MEMACCESS([y_buf]) ",%%xmm3                   \n"            \
    "lea        " MEMLEA(0x8, [y_buf]) ",%[y_buf]               \n"            \
    "punpcklbw  %%xmm4,%%xmm3                                   \n"            \
    "psubsw     " MEMACCESS2(96, [kYuvConstants]) ",%%xmm3      \n"            \
    "pmullw     " MEMACCESS2(112, [kYuvConstants]) ",%%xmm3     \n"            \
    "paddsw     %%xmm3,%%xmm0                                   \n"            \
    "paddsw     %%xmm3,%%xmm1                                   \n"            \
    "paddsw     %%xmm3,%%xmm2                                   \n"            \
    "psraw      $0x6,%%xmm0                                     \n"            \
    "psraw      $0x6,%%xmm1                                     \n"            \
    "psraw      $0x6,%%xmm2                                     \n"            \
    "packuswb   %%xmm0,%%xmm0                                   \n"            \
    "packuswb   %%xmm1,%%xmm1                                   \n"            \
    "packuswb   %%xmm2,%%xmm2                                   \n"

void OMITFP I444ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* dst_argb,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV444
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS([dst_argb]) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "   \n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb]  \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToRGB24Row_SSSE3(const uint8* y_buf,
                                 const uint8* u_buf,
                                 const uint8* v_buf,
                                 uint8* dst_rgb24,
                                 int width) {

#if defined(__i386__)
  asm volatile (
    "movdqa    %[kShuffleMaskARGBToRGB24_0],%%xmm5 \n"
    "movdqa    %[kShuffleMaskARGBToRGB24],%%xmm6   \n"
  :: [kShuffleMaskARGBToRGB24_0]"m"(kShuffleMaskARGBToRGB24_0),
    [kShuffleMaskARGBToRGB24]"m"(kShuffleMaskARGBToRGB24));
#endif

  asm volatile (
#if !defined(__i386__)
    "movdqa    %[kShuffleMaskARGBToRGB24_0],%%xmm5 \n"
    "movdqa    %[kShuffleMaskARGBToRGB24],%%xmm6   \n"
#endif
    "sub       %[u_buf],%[v_buf]               \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm2,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "movq      %%xmm0," MEMACCESS([dst_rgb24]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x8,[dst_rgb24]) "\n"
    "lea       " MEMLEA(0x18,[dst_rgb24]) ",%[dst_rgb24] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_rgb24]"+r"(dst_rgb24),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB)
#if !defined(__i386__)
    , [kShuffleMaskARGBToRGB24_0]"m"(kShuffleMaskARGBToRGB24_0),
    [kShuffleMaskARGBToRGB24]"m"(kShuffleMaskARGBToRGB24)
#endif
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void OMITFP I422ToRAWRow_SSSE3(const uint8* y_buf,
                               const uint8* u_buf,
                               const uint8* v_buf,
                               uint8* dst_raw,
                               int width) {

#if defined(__i386__)
  asm volatile (
    "movdqa    %[kShuffleMaskARGBToRAW_0],%%xmm5 \n"
    "movdqa    %[kShuffleMaskARGBToRAW],%%xmm6   \n"
  :: [kShuffleMaskARGBToRAW_0]"m"(kShuffleMaskARGBToRAW_0),
    [kShuffleMaskARGBToRAW]"m"(kShuffleMaskARGBToRAW));
#endif

  asm volatile (
#if !defined(__i386__)
    "movdqa    %[kShuffleMaskARGBToRAW_0],%%xmm5 \n"
    "movdqa    %[kShuffleMaskARGBToRAW],%%xmm6   \n"
#endif
    "sub       %[u_buf],%[v_buf]               \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm2,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm6,%%xmm1                   \n"
    "palignr   $0xc,%%xmm0,%%xmm1              \n"
    "movq      %%xmm0," MEMACCESS([dst_raw]) " \n"
    "movdqu    %%xmm1," MEMACCESS2(0x8,[dst_raw]) "\n"
    "lea       " MEMLEA(0x18,[dst_raw]) ",%[dst_raw] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_raw]"+r"(dst_raw),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB)
#if !defined(__i386__)
    , [kShuffleMaskARGBToRAW_0]"m"(kShuffleMaskARGBToRAW_0),
    [kShuffleMaskARGBToRAW]"m"(kShuffleMaskARGBToRAW)
#endif
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}

void OMITFP I422ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* dst_argb,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I411ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* dst_argb,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV411
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV12ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* uv_buf,
                                uint8* dst_argb,
                                int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READNV12
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
  
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV21ToARGBRow_SSSE3(const uint8* y_buf,
                                const uint8* uv_buf,
                                uint8* dst_argb,
                                int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READNV12
    YVUTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [dst_argb]"+r"(dst_argb),  
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
                                          uint8* dst_argb,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV444
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* dst_argb,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I411ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* dst_argb,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV411
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV12ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* uv_buf,
                                          uint8* dst_argb,
                                          int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READNV12
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [dst_argb]"+r"(dst_argb),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
  
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP NV21ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* uv_buf,
                                          uint8* dst_argb,
                                          int width) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READNV12
    YVUTORGB
    "punpcklbw %%xmm1,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm0                   \n"
    "punpckhwd %%xmm2,%%xmm1                   \n"
    "movdqu    %%xmm0," MEMACCESS([dst_argb]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_argb]) "\n"
    "lea       " MEMLEA(0x20,[dst_argb]) ",%[dst_argb] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [uv_buf]"+r"(uv_buf),    
    [dst_argb]"+r"(dst_argb),  
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
                                uint8* dst_bgra,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm5," MEMACCESS([dst_bgra]) "\n"
    "movdqa    %%xmm0," MEMACCESS2(0x10,[dst_bgra]) "\n"
    "lea       " MEMLEA(0x20,[dst_bgra]) ",%[dst_bgra] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_bgra]"+r"(dst_bgra),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToABGRRow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* dst_abgr,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2," MEMACCESS([dst_abgr]) "\n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,[dst_abgr]) "\n"
    "lea       " MEMLEA(0x20,[dst_abgr]) ",%[dst_abgr] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_abgr]"+r"(dst_abgr),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToRGBARow_SSSE3(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* dst_rgba,
                                int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm2,%%xmm1                   \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm5," MEMACCESS([dst_rgba]) "\n"
    "movdqa    %%xmm0," MEMACCESS2(0x10,[dst_rgba]) "\n"
    "lea       " MEMLEA(0x20,[dst_rgba]) ",%[dst_rgba] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_rgba]"+r"(dst_rgba),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* dst_bgra,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm5," MEMACCESS([dst_bgra]) "\n"
    "movdqu    %%xmm0," MEMACCESS2(0x10,[dst_bgra]) "\n"
    "lea       " MEMLEA(0x20,[dst_bgra]) ",%[dst_bgra] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_bgra]"+r"(dst_bgra),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* dst_abgr,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm2                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "movdqu    %%xmm2," MEMACCESS([dst_abgr]) "\n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,[dst_abgr]) "\n"
    "lea       " MEMLEA(0x20,[dst_abgr]) ",%[dst_abgr] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_abgr]"+r"(dst_abgr),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

void OMITFP I422ToRGBARow_Unaligned_SSSE3(const uint8* y_buf,
                                          const uint8* u_buf,
                                          const uint8* v_buf,
                                          uint8* dst_rgba,
                                          int width) {
  asm volatile (
    "sub       %[u_buf],%[v_buf]               \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pxor      %%xmm4,%%xmm4                   \n"
    LABELALIGN
  "1:                                          \n"
    READYUV422
    YUVTORGB
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm2,%%xmm1                   \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "movdqa    %%xmm5,%%xmm0                   \n"
    "punpcklwd %%xmm1,%%xmm5                   \n"
    "punpckhwd %%xmm1,%%xmm0                   \n"
    "movdqu    %%xmm5," MEMACCESS([dst_rgba]) "\n"
    "movdqu    %%xmm0," MEMACCESS2(0x10,[dst_rgba]) "\n"
    "lea       " MEMLEA(0x20,[dst_rgba]) ",%[dst_rgba] \n"
    "sub       $0x8,%[width]                   \n"
    "jg        1b                              \n"
  : [y_buf]"+r"(y_buf),    
    [u_buf]"+r"(u_buf),    
    [v_buf]"+r"(v_buf),    
    [dst_rgba]"+r"(dst_rgba),  
    [width]"+rm"(width)    
  : [kYuvConstants]"r"(&kYuvConstants.kUVToB) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}

#endif  

#ifdef HAS_YTOARGBROW_SSE2
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* dst_argb,
                     int width) {
  asm volatile (
    "pxor      %%xmm5,%%xmm5                   \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "mov       $0x00100010,%%eax               \n"
    "movd      %%eax,%%xmm3                    \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "mov       $0x004a004a,%%eax               \n"
    "movd      %%eax,%%xmm2                    \n"
    "pshufd    $0x0,%%xmm2,%%xmm2              \n"
    LABELALIGN
  "1:                                          \n"
    
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "psubusw   %%xmm3,%%xmm0                   \n"
    "pmullw    %%xmm2,%%xmm0                   \n"
    "psrlw     $6, %%xmm0                      \n"
    "packuswb  %%xmm0,%%xmm0                   \n"

    
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm0,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm1                   \n"
    "por       %%xmm4,%%xmm0                   \n"
    "por       %%xmm4,%%xmm1                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"

    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(y_buf),     
    "+r"(dst_argb),  
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

static uvec8 kShuffleMirror = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = (intptr_t)(width);
  asm volatile (
    "movdqa    %3,%%xmm5                       \n"
    "lea       " MEMLEA(-0x10,0) ",%0          \n"
    LABELALIGN
  "1:                                          \n"
    MEMOPREG(movdqa,0x00,0,2,1,xmm0)           
    "pshufb    %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(temp_width)  
  : "m"(kShuffleMirror) 
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_MIRRORROW_SSE2
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = (intptr_t)(width);
  asm volatile (
    "lea       " MEMLEA(-0x10,0) ",%0          \n"
    LABELALIGN
  "1:                                          \n"
    MEMOPREG(movdqu,0x00,0,2,1,xmm0)           
    "movdqa    %%xmm0,%%xmm1                   \n"
    "psllw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm1,%%xmm0                   \n"
    "pshuflw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufd    $0x4e,%%xmm0,%%xmm0             \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1)",%1            \n"
    "jg        1b                              \n"
  : "+r"(src),  
    "+r"(dst),  
    "+r"(temp_width)  
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
#endif  

#ifdef HAS_MIRRORROW_UV_SSSE3

static uvec8 kShuffleMirrorUV = {
  14u, 12u, 10u, 8u, 6u, 4u, 2u, 0u, 15u, 13u, 11u, 9u, 7u, 5u, 3u, 1u
};
void MirrorUVRow_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
                       int width) {
  intptr_t temp_width = (intptr_t)(width);
  asm volatile (
    "movdqa    %4,%%xmm1                       \n"
    "lea       " MEMLEA4(-0x10,0,3,2) ",%0       \n"
    "sub       %1,%2                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(-0x10,0) ",%0            \n"
    "pshufb    %%xmm1,%%xmm0                   \n"
    "sub       $8,%3                           \n"
    "movlpd    %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movhpd,xmm0,0x00,1,2,1)           
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src),      
    "+r"(dst_u),    
    "+r"(dst_v),    
    "+r"(temp_width)  
  : "m"(kShuffleMirrorUV)  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_ARGBMIRRORROW_SSSE3

static uvec8 kARGBShuffleMirror = {
  12u, 13u, 14u, 15u, 8u, 9u, 10u, 11u, 4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u
};

void ARGBMirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  intptr_t temp_width = (intptr_t)(width);
  asm volatile (
    "lea       " MEMLEA4(-0x10,0,2,4) ",%0     \n"
    "movdqa    %3,%%xmm5                       \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "lea       " MEMLEA(-0x10,0) ",%0          \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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

#ifdef HAS_SPLITUVROW_SSE2
void SplitUVRow_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  asm volatile (
    "pcmpeqb    %%xmm5,%%xmm5                    \n"
    "psrlw      $0x8,%%xmm5                      \n"
    "sub        %1,%2                            \n"
    LABELALIGN
  "1:                                            \n"
    "movdqa     " MEMACCESS(0) ",%%xmm0          \n"
    "movdqa     " MEMACCESS2(0x10,0) ",%%xmm1    \n"
    "lea        " MEMLEA(0x20,0) ",%0            \n"
    "movdqa     %%xmm0,%%xmm2                    \n"
    "movdqa     %%xmm1,%%xmm3                    \n"
    "pand       %%xmm5,%%xmm0                    \n"
    "pand       %%xmm5,%%xmm1                    \n"
    "packuswb   %%xmm1,%%xmm0                    \n"
    "psrlw      $0x8,%%xmm2                      \n"
    "psrlw      $0x8,%%xmm3                      \n"
    "packuswb   %%xmm3,%%xmm2                    \n"
    "movdqa     %%xmm0," MEMACCESS(1) "          \n"
    MEMOPMEM(movdqa,xmm2,0x00,1,2,1)             
    "lea        " MEMLEA(0x10,1) ",%1            \n"
    "sub        $0x10,%3                         \n"
    "jg         1b                               \n"
  : "+r"(src_uv),     
    "+r"(dst_u),      
    "+r"(dst_v),      
    "+r"(pix)         
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}

void SplitUVRow_Unaligned_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                               int pix) {
  asm volatile (
    "pcmpeqb    %%xmm5,%%xmm5                    \n"
    "psrlw      $0x8,%%xmm5                      \n"
    "sub        %1,%2                            \n"
    LABELALIGN
  "1:                                            \n"
    "movdqu     " MEMACCESS(0) ",%%xmm0          \n"
    "movdqu     " MEMACCESS2(0x10,0) ",%%xmm1    \n"
    "lea        " MEMLEA(0x20,0) ",%0            \n"
    "movdqa     %%xmm0,%%xmm2                    \n"
    "movdqa     %%xmm1,%%xmm3                    \n"
    "pand       %%xmm5,%%xmm0                    \n"
    "pand       %%xmm5,%%xmm1                    \n"
    "packuswb   %%xmm1,%%xmm0                    \n"
    "psrlw      $0x8,%%xmm2                      \n"
    "psrlw      $0x8,%%xmm3                      \n"
    "packuswb   %%xmm3,%%xmm2                    \n"
    "movdqu     %%xmm0," MEMACCESS(1) "          \n"
    MEMOPMEM(movdqu,xmm2,0x00,1,2,1)             
    "lea        " MEMLEA(0x10,1) ",%1            \n"
    "sub        $0x10,%3                         \n"
    "jg         1b                               \n"
  : "+r"(src_uv),     
    "+r"(dst_u),      
    "+r"(dst_v),      
    "+r"(pix)         
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_MERGEUVROW_SSE2
void MergeUVRow_SSE2(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                     int width) {
  asm volatile (
    "sub       %0,%1                             \n"
    LABELALIGN
  "1:                                            \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0           \n"
    MEMOPREG(movdqa,0x00,0,1,1,xmm1)             
    "lea       " MEMLEA(0x10,0) ",%0             \n"
    "movdqa    %%xmm0,%%xmm2                     \n"
    "punpcklbw %%xmm1,%%xmm0                     \n"
    "punpckhbw %%xmm1,%%xmm2                     \n"
    "movdqa    %%xmm0," MEMACCESS(2) "           \n"
    "movdqa    %%xmm2," MEMACCESS2(0x10,2) "     \n"
    "lea       " MEMLEA(0x20,2) ",%2             \n"
    "sub       $0x10,%3                          \n"
    "jg        1b                                \n"
  : "+r"(src_u),     
    "+r"(src_v),     
    "+r"(dst_uv),    
    "+r"(width)      
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
  );
}

void MergeUVRow_Unaligned_SSE2(const uint8* src_u, const uint8* src_v,
                               uint8* dst_uv, int width) {
  asm volatile (
    "sub       %0,%1                             \n"
    LABELALIGN
  "1:                                            \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0           \n"
    MEMOPREG(movdqu,0x00,0,1,1,xmm1)             
    "lea       " MEMLEA(0x10,0) ",%0             \n"
    "movdqa    %%xmm0,%%xmm2                     \n"
    "punpcklbw %%xmm1,%%xmm0                     \n"
    "punpckhbw %%xmm1,%%xmm2                     \n"
    "movdqu    %%xmm0," MEMACCESS(2) "           \n"
    "movdqu    %%xmm2," MEMACCESS2(0x10,2) "     \n"
    "lea       " MEMLEA(0x20,2) ",%2             \n"
    "sub       $0x10,%3                          \n"
    "jg        1b                                \n"
  : "+r"(src_u),     
    "+r"(src_v),     
    "+r"(dst_uv),    
    "+r"(width)      
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
  );
}
#endif  

#ifdef HAS_COPYROW_SSE2
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
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
  size_t width_tmp = (size_t)(width);
  asm volatile (
    "shr       $0x2,%2                         \n"
    "rep movsl " MEMMOVESTRING(0,1) "          \n"
  : "+S"(src),  
    "+D"(dst),  
    "+c"(width_tmp) 
  :
  : "memory", "cc"
  );
}
#endif  

#ifdef HAS_COPYROW_ERMS

void CopyRow_ERMS(const uint8* src, uint8* dst, int width) {
  size_t width_tmp = (size_t)(width);
  asm volatile (
    "rep movsb " MEMMOVESTRING(0,1) "          \n"
  : "+S"(src),  
    "+D"(dst),  
    "+c"(width_tmp) 
  :
  : "memory", "cc"
  );
}
#endif  

#ifdef HAS_ARGBCOPYALPHAROW_SSE2

void ARGBCopyAlphaRow_SSE2(const uint8* src, uint8* dst, int width) {
  asm volatile (
    "pcmpeqb   %%xmm0,%%xmm0                   \n"
    "pslld     $0x18,%%xmm0                    \n"
    "pcmpeqb   %%xmm1,%%xmm1                   \n"
    "psrld     $0x8,%%xmm1                     \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm2         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "movdqa    " MEMACCESS(1) ",%%xmm4         \n"
    "movdqa    " MEMACCESS2(0x10,1) ",%%xmm5   \n"
    "pand      %%xmm0,%%xmm2                   \n"
    "pand      %%xmm0,%%xmm3                   \n"
    "pand      %%xmm1,%%xmm4                   \n"
    "pand      %%xmm1,%%xmm5                   \n"
    "por       %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm3                   \n"
    "movdqa    %%xmm2," MEMACCESS(1) "         \n"
    "movdqa    %%xmm3," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBCOPYALPHAROW_AVX2

void ARGBCopyAlphaRow_AVX2(const uint8* src, uint8* dst, int width) {
  asm volatile (
    "vpcmpeqb  %%ymm0,%%ymm0,%%ymm0            \n"
    "vpsrld    $0x8,%%ymm0,%%ymm0              \n"
    LABELALIGN
  "1:                                          \n"
    "vmovdqu   " MEMACCESS(0) ",%%ymm1         \n"
    "vmovdqu   " MEMACCESS2(0x20,0) ",%%ymm2   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "vpblendvb %%ymm0," MEMACCESS(1) ",%%ymm1,%%ymm1        \n"
    "vpblendvb %%ymm0," MEMACCESS2(0x20,1) ",%%ymm2,%%ymm2  \n"
    "vmovdqu   %%ymm1," MEMACCESS(1) "         \n"
    "vmovdqu   %%ymm2," MEMACCESS2(0x20,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
    "vzeroupper                                \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
  );
}
#endif  

#ifdef HAS_ARGBCOPYYTOALPHAROW_SSE2

void ARGBCopyYToAlphaRow_SSE2(const uint8* src, uint8* dst, int width) {
  asm volatile (
    "pcmpeqb   %%xmm0,%%xmm0                   \n"
    "pslld     $0x18,%%xmm0                    \n"
    "pcmpeqb   %%xmm1,%%xmm1                   \n"
    "psrld     $0x8,%%xmm1                     \n"
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm2         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "punpcklbw %%xmm2,%%xmm2                   \n"
    "punpckhwd %%xmm2,%%xmm3                   \n"
    "punpcklwd %%xmm2,%%xmm2                   \n"
    "movdqa    " MEMACCESS(1) ",%%xmm4         \n"
    "movdqa    " MEMACCESS2(0x10,1) ",%%xmm5   \n"
    "pand      %%xmm0,%%xmm2                   \n"
    "pand      %%xmm0,%%xmm3                   \n"
    "pand      %%xmm1,%%xmm4                   \n"
    "pand      %%xmm1,%%xmm5                   \n"
    "por       %%xmm4,%%xmm2                   \n"
    "por       %%xmm5,%%xmm3                   \n"
    "movdqa    %%xmm2," MEMACCESS(1) "         \n"
    "movdqa    %%xmm3," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "sub       $0x8,%2                         \n"
    "jg        1b                              \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBCOPYYTOALPHAROW_AVX2

void ARGBCopyYToAlphaRow_AVX2(const uint8* src, uint8* dst, int width) {
  asm volatile (
    "vpcmpeqb  %%ymm0,%%ymm0,%%ymm0            \n"
    "vpsrld    $0x8,%%ymm0,%%ymm0              \n"
    LABELALIGN
  "1:                                          \n"
    "vpmovzxbd " MEMACCESS(0) ",%%ymm1         \n"
    "vpmovzxbd " MEMACCESS2(0x8,0) ",%%ymm2    \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "vpslld    $0x18,%%ymm1,%%ymm1             \n"
    "vpslld    $0x18,%%ymm2,%%ymm2             \n"
    "vpblendvb %%ymm0," MEMACCESS(1) ",%%ymm1,%%ymm1        \n"
    "vpblendvb %%ymm0," MEMACCESS2(0x20,1) ",%%ymm2,%%ymm2  \n"
    "vmovdqu   %%ymm1," MEMACCESS(1) "         \n"
    "vmovdqu   %%ymm2," MEMACCESS2(0x20,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
    "sub       $0x10,%2                        \n"
    "jg        1b                              \n"
    "vzeroupper                                \n"
  : "+r"(src),   
    "+r"(dst),   
    "+r"(width)  
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2"
#endif
  );
}
#endif  

#ifdef HAS_SETROW_X86
void SetRow_X86(uint8* dst, uint32 v32, int width) {
  size_t width_tmp = (size_t)(width);
  asm volatile (
    "shr       $0x2,%1                         \n"
    "rep stosl " MEMSTORESTRING(eax,0) "       \n"
    : "+D"(dst),       
      "+c"(width_tmp)  
    : "a"(v32)         
    : "memory", "cc");
}

void ARGBSetRows_X86(uint8* dst, uint32 v32, int width,
                   int dst_stride, int height) {
  for (int y = 0; y < height; ++y) {
    size_t width_tmp = (size_t)(width);
    uint32* d = (uint32*)(dst);
    asm volatile (
      "rep stosl " MEMSTORESTRING(eax,0) "     \n"
      : "+D"(d),         
        "+c"(width_tmp)  
      : "a"(v32)         
      : "memory", "cc");
    dst += dst_stride;
  }
}
#endif  

#ifdef HAS_YUY2TOYROW_SSE2
void YUY2ToYRow_SSE2(const uint8* src_yuy2, uint8* dst_y, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrlw     $0x8,%%xmm5                     \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    BUNDLEALIGN
    MEMOPREG(movdqa,0x00,0,4,1,xmm2)           
    MEMOPREG(movdqa,0x10,0,4,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"((intptr_t)(stride_yuy2))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm2)           
    MEMOPREG(movdqu,0x10,0,4,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"((intptr_t)(stride_yuy2))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_yuy2),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void UYVYToYRow_SSE2(const uint8* src_uyvy, uint8* dst_y, int pix) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    BUNDLEALIGN
    MEMOPREG(movdqa,0x00,0,4,1,xmm2)           
    MEMOPREG(movdqa,0x10,0,4,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"((intptr_t)(stride_uyvy))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  asm volatile (
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    BUNDLEALIGN
    MEMOPREG(movdqu,0x00,0,4,1,xmm2)           
    MEMOPREG(movdqu,0x10,0,4,1,xmm3)           
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  : "r"((intptr_t)(stride_uyvy))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm1,0x00,1,2,1)             
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "sub       $0x10,%3                        \n"
    "jg        1b                              \n"
  : "+r"(src_uyvy),    
    "+r"(dst_u),       
    "+r"(dst_v),       
    "+r"(pix)          
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    "movd      " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      " MEMACCESS(1) ",%%xmm2         \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x4,1) ",%1            \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x4,2) ",%2            \n"
    "jge       10b                             \n"

  "19:                                         \n"
    "add       $1-4,%3                         \n"
    "jl        49f                             \n"

    
    LABELALIGN
  "41:                                         \n"
    "movdqu    " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    " MEMACCESS(1) ",%%xmm2         \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jge       41b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        99f                             \n"

    
  "91:                                         \n"
    "movd      " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      " MEMACCESS(1) ",%%xmm2         \n"
    "psrlw     $0x8,%%xmm3                     \n"
    "pshufhw   $0xf5,%%xmm3,%%xmm3             \n"
    "pshuflw   $0xf5,%%xmm3,%%xmm3             \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x4,1) ",%1            \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x4,2) ",%2            \n"
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

static uvec8 kShuffleAlpha = {
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
    "movd      " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      " MEMACCESS(1) ",%%xmm2         \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x4,1) ",%1            \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x4,2) ",%2            \n"
    "jge       10b                             \n"

  "19:                                         \n"
    "add       $1-4,%3                         \n"
    "jl        49f                             \n"
    "test      $0xf,%0                         \n"
    "jne       41f                             \n"
    "test      $0xf,%1                         \n"
    "jne       41f                             \n"

    
    LABELALIGN
  "40:                                         \n"
    "movdqa    " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqa    " MEMACCESS(1) ",%%xmm2         \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqa    " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jge       40b                             \n"
    "jmp       49f                             \n"

    
    LABELALIGN
  "41:                                         \n"
    "movdqu    " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movdqu    " MEMACCESS(1) ",%%xmm2         \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jge       41b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        99f                             \n"

    
  "91:                                         \n"
    "movd      " MEMACCESS(0) ",%%xmm3         \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "movdqa    %%xmm3,%%xmm0                   \n"
    "pxor      %%xmm4,%%xmm3                   \n"
    "movd      " MEMACCESS(1) ",%%xmm2         \n"
    "pshufb    %4,%%xmm3                       \n"
    "pand      %%xmm6,%%xmm2                   \n"
    "paddw     %%xmm7,%%xmm3                   \n"
    "pmullw    %%xmm3,%%xmm2                   \n"
    "movd      " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x4,1) ",%1            \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "por       %%xmm4,%%xmm0                   \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm2                     \n"
    "paddusb   %%xmm2,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x1,%3                         \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x4,2) ",%2            \n"
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

#ifdef HAS_ARGBATTENUATEROW_SSE2


void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "pslld     $0x18,%%xmm4                    \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrld     $0x8,%%xmm5                     \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "pshufhw   $0xff,%%xmm0,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm1         \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "pshufhw   $0xff,%%xmm1,%%xmm2             \n"
    "pshuflw   $0xff,%%xmm2,%%xmm2             \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "pand      %%xmm4,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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

static uvec8 kShuffleAlpha0 = {
  3u, 3u, 3u, 3u, 3u, 3u, 128u, 128u, 7u, 7u, 7u, 7u, 7u, 7u, 128u, 128u,
};
static uvec8 kShuffleAlpha1 = {
  11u, 11u, 11u, 11u, 11u, 11u, 128u, 128u,
  15u, 15u, 15u, 15u, 15u, 15u, 128u, 128u,
};


void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "pcmpeqb   %%xmm3,%%xmm3                   \n"
    "pslld     $0x18,%%xmm3                    \n"
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "pshufb    %%xmm4,%%xmm0                   \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "punpcklbw %%xmm1,%%xmm1                   \n"
    "pmulhuw   %%xmm1,%%xmm0                   \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "movdqu    " MEMACCESS(0) ",%%xmm2         \n"
    "punpckhbw %%xmm2,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "movdqu    " MEMACCESS(0) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "pand      %%xmm3,%%xmm2                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm2,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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
    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movzb     " MEMACCESS2(0x03,0) ",%3       \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    MEMOPREG(movd,0x00,4,3,4,xmm2)             
    "movzb     " MEMACCESS2(0x07,0) ",%3       \n"
    MEMOPREG(movd,0x00,4,3,4,xmm3)             
    "pshuflw   $0x40,%%xmm2,%%xmm2             \n"
    "pshuflw   $0x40,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqu    " MEMACCESS(0) ",%%xmm1         \n"
    "movzb     " MEMACCESS2(0x0b,0) ",%3       \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,4,3,4,xmm2)             
    "movzb     " MEMACCESS2(0x0f,0) ",%3       \n"
    MEMOPREG(movd,0x00,4,3,4,xmm3)             
    "pshuflw   $0x40,%%xmm2,%%xmm2             \n"
    "pshuflw   $0x40,%%xmm3,%%xmm3             \n"
    "movlhps   %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+r"(width),       
    "+r"(alpha)        
  : "r"(fixed_invtbl8)  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBGRAYROW_SSSE3

void ARGBGrayRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  asm volatile (
    "movdqa    %3,%%xmm4                       \n"
    "movdqa    %4,%%xmm5                       \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "pmaddubsw %%xmm4,%%xmm0                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm0                   \n"
    "paddw     %%xmm5,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm2         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm3   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),   
    "+r"(dst_argb),   
    "+r"(width)       
  : "m"(kARGBToYJ),   
    "m"(kAddYJ64)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSEPIAROW_SSSE3




static vec8 kARGBToSepiaB = {
  17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0
};

static vec8 kARGBToSepiaG = {
  22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0
};

static vec8 kARGBToSepiaR = {
  24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0
};


void ARGBSepiaRow_SSSE3(uint8* dst_argb, int width) {
  asm volatile (
    "movdqa    %2,%%xmm2                       \n"
    "movdqa    %3,%%xmm3                       \n"
    "movdqa    %4,%%xmm4                       \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm6   \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "pmaddubsw %%xmm2,%%xmm6                   \n"
    "phaddw    %%xmm6,%%xmm0                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm5         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "pmaddubsw %%xmm3,%%xmm5                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm5                   \n"
    "psrlw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm5         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "pmaddubsw %%xmm4,%%xmm5                   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "phaddw    %%xmm1,%%xmm5                   \n"
    "psrlw     $0x7,%%xmm5                     \n"
    "packuswb  %%xmm5,%%xmm5                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm6         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "psrld     $0x18,%%xmm6                    \n"
    "psrld     $0x18,%%xmm1                    \n"
    "packuswb  %%xmm1,%%xmm6                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "punpcklbw %%xmm6,%%xmm5                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklwd %%xmm5,%%xmm0                   \n"
    "punpckhwd %%xmm5,%%xmm1                   \n"
    "sub       $0x8,%1                         \n"
    "movdqa    %%xmm0," MEMACCESS(0) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,0) "   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
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


void ARGBColorMatrixRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                              const int8* matrix_argb, int width) {
  asm volatile (
    "movdqu    " MEMACCESS(3) ",%%xmm5         \n"
    "pshufd    $0x00,%%xmm5,%%xmm2             \n"
    "pshufd    $0x55,%%xmm5,%%xmm3             \n"
    "pshufd    $0xaa,%%xmm5,%%xmm4             \n"
    "pshufd    $0xff,%%xmm5,%%xmm5             \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm7   \n"
    "pmaddubsw %%xmm2,%%xmm0                   \n"
    "pmaddubsw %%xmm2,%%xmm7                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm6         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "pmaddubsw %%xmm3,%%xmm6                   \n"
    "pmaddubsw %%xmm3,%%xmm1                   \n"
    "phaddsw   %%xmm7,%%xmm0                   \n"
    "phaddsw   %%xmm1,%%xmm6                   \n"
    "psraw     $0x6,%%xmm0                     \n"
    "psraw     $0x6,%%xmm6                     \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "punpcklbw %%xmm6,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm1         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm7   \n"
    "pmaddubsw %%xmm4,%%xmm1                   \n"
    "pmaddubsw %%xmm4,%%xmm7                   \n"
    "phaddsw   %%xmm7,%%xmm1                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm6         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm7   \n"
    "pmaddubsw %%xmm5,%%xmm6                   \n"
    "pmaddubsw %%xmm5,%%xmm7                   \n"
    "phaddsw   %%xmm7,%%xmm6                   \n"
    "psraw     $0x6,%%xmm1                     \n"
    "psraw     $0x6,%%xmm6                     \n"
    "packuswb  %%xmm1,%%xmm1                   \n"
    "packuswb  %%xmm6,%%xmm6                   \n"
    "punpcklbw %%xmm6,%%xmm1                   \n"
    "movdqa    %%xmm0,%%xmm6                   \n"
    "punpcklwd %%xmm1,%%xmm0                   \n"
    "punpckhwd %%xmm1,%%xmm6                   \n"
    "sub       $0x8,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm6," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),      
    "+r"(dst_argb),      
    "+r"(width)          
  : "r"(matrix_argb)     
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
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

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm1         \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "pmullw    %%xmm3,%%xmm0                   \n"
    "movdqa    " MEMACCESS(0) ",%%xmm7         \n"
    "pmullw    %%xmm3,%%xmm1                   \n"
    "pand      %%xmm6,%%xmm7                   \n"
    "paddw     %%xmm4,%%xmm0                   \n"
    "paddw     %%xmm4,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "por       %%xmm7,%%xmm0                   \n"
    "sub       $0x4,%1                         \n"
    "movdqa    %%xmm0," MEMACCESS(0) "         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
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

#ifdef HAS_ARGBSHADEROW_SSE2


void ARGBShadeRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width,
                       uint32 value) {
  asm volatile (
    "movd      %3,%%xmm2                       \n"
    "punpcklbw %%xmm2,%%xmm2                   \n"
    "punpcklqdq %%xmm2,%%xmm2                  \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "pmulhuw   %%xmm2,%%xmm1                   \n"
    "psrlw     $0x8,%%xmm0                     \n"
    "psrlw     $0x8,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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

#ifdef HAS_ARGBMULTIPLYROW_SSE2

void ARGBMultiplyRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  asm volatile (
    "pxor      %%xmm5,%%xmm5                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqu    " MEMACCESS(1) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "movdqu    %%xmm0,%%xmm1                   \n"
    "movdqu    %%xmm2,%%xmm3                   \n"
    "punpcklbw %%xmm0,%%xmm0                   \n"
    "punpckhbw %%xmm1,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "punpckhbw %%xmm5,%%xmm3                   \n"
    "pmulhuw   %%xmm2,%%xmm0                   \n"
    "pmulhuw   %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBADDROW_SSE2

void ARGBAddRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                     uint8* dst_argb, int width) {
  asm volatile (
    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSUBTRACTROW_SSE2

void ARGBSubtractRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  asm volatile (
    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "psubusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_argb0),  
    "+r"(src_argb1),  
    "+r"(dst_argb),   
    "+r"(width)       
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1"
#endif
  );
}
#endif  

#ifdef HAS_SOBELXROW_SSE2




void SobelXRow_SSE2(const uint8* src_y0, const uint8* src_y1,
                    const uint8* src_y2, uint8* dst_sobelx, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "sub       %0,%2                           \n"
    "sub       %0,%3                           \n"
    "pxor      %%xmm5,%%xmm5                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "movq      " MEMACCESS2(0x2,0) ",%%xmm1    \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "psubw     %%xmm1,%%xmm0                   \n"
    BUNDLEALIGN
    MEMOPREG(movq,0x00,0,1,1,xmm1)             
    MEMOPREG(movq,0x02,0,1,1,xmm2)             
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "psubw     %%xmm2,%%xmm1                   \n"
    BUNDLEALIGN
    MEMOPREG(movq,0x00,0,2,1,xmm2)             
    MEMOPREG(movq,0x02,0,2,1,xmm3)             
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm3                   \n"
    "psubw     %%xmm3,%%xmm2                   \n"
    "paddw     %%xmm2,%%xmm0                   \n"
    "paddw     %%xmm1,%%xmm0                   \n"
    "paddw     %%xmm1,%%xmm0                   \n"
    "pxor      %%xmm1,%%xmm1                   \n"
    "psubw     %%xmm0,%%xmm1                   \n"
    "pmaxsw    %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "sub       $0x8,%4                         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm0,0x00,0,3,1)             
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "jg        1b                              \n"
  : "+r"(src_y0),      
    "+r"(src_y1),      
    "+r"(src_y2),      
    "+r"(dst_sobelx),  
    "+r"(width)        
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_SOBELYROW_SSE2




void SobelYRow_SSE2(const uint8* src_y0, const uint8* src_y1,
                    uint8* dst_sobely, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "sub       %0,%2                           \n"
    "pxor      %%xmm5,%%xmm5                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movq,0x00,0,1,1,xmm1)             
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "psubw     %%xmm1,%%xmm0                   \n"
    BUNDLEALIGN
    "movq      " MEMACCESS2(0x1,0) ",%%xmm1    \n"
    MEMOPREG(movq,0x01,0,1,1,xmm2)             
    "punpcklbw %%xmm5,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "psubw     %%xmm2,%%xmm1                   \n"
    BUNDLEALIGN
    "movq      " MEMACCESS2(0x2,0) ",%%xmm2    \n"
    MEMOPREG(movq,0x02,0,1,1,xmm3)             
    "punpcklbw %%xmm5,%%xmm2                   \n"
    "punpcklbw %%xmm5,%%xmm3                   \n"
    "psubw     %%xmm3,%%xmm2                   \n"
    "paddw     %%xmm2,%%xmm0                   \n"
    "paddw     %%xmm1,%%xmm0                   \n"
    "paddw     %%xmm1,%%xmm0                   \n"
    "pxor      %%xmm1,%%xmm1                   \n"
    "psubw     %%xmm0,%%xmm1                   \n"
    "pmaxsw    %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "sub       $0x8,%3                         \n"
    BUNDLEALIGN
    MEMOPMEM(movq,xmm0,0x00,0,2,1)             
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "jg        1b                              \n"
  : "+r"(src_y0),      
    "+r"(src_y1),      
    "+r"(dst_sobely),  
    "+r"(width)        
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_SOBELROW_SSE2





void SobelRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                   uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,0,1,1,xmm1)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "punpcklbw %%xmm0,%%xmm2                   \n"
    "punpckhbw %%xmm0,%%xmm0                   \n"
    "movdqa    %%xmm2,%%xmm1                   \n"
    "punpcklwd %%xmm2,%%xmm1                   \n"
    "punpckhwd %%xmm2,%%xmm2                   \n"
    "por       %%xmm5,%%xmm1                   \n"
    "por       %%xmm5,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm3                   \n"
    "punpcklwd %%xmm0,%%xmm3                   \n"
    "punpckhwd %%xmm0,%%xmm0                   \n"
    "por       %%xmm5,%%xmm3                   \n"
    "por       %%xmm5,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movdqa    %%xmm1," MEMACCESS(2) "         \n"
    "movdqa    %%xmm2," MEMACCESS2(0x10,2) "   \n"
    "movdqa    %%xmm3," MEMACCESS2(0x20,2) "   \n"
    "movdqa    %%xmm0," MEMACCESS2(0x30,2) "   \n"
    "lea       " MEMLEA(0x40,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_argb),    
    "+r"(width)        
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_SOBELTOPLANEROW_SSE2

void SobelToPlaneRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                          uint8* dst_y, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "pslld     $0x18,%%xmm5                    \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,0,1,1,xmm1)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "paddusb   %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%3                        \n"
    "movdqa    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_y),       
    "+r"(width)        
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
#endif  

#ifdef HAS_SOBELXYROW_SSE2





void SobelXYRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                     uint8* dst_argb, int width) {
  asm volatile (
    "sub       %0,%1                           \n"
    "pcmpeqb   %%xmm5,%%xmm5                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,0,1,1,xmm1)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm2                   \n"
    "paddusb   %%xmm1,%%xmm2                   \n"
    "movdqa    %%xmm0,%%xmm3                   \n"
    "punpcklbw %%xmm5,%%xmm3                   \n"
    "punpckhbw %%xmm5,%%xmm0                   \n"
    "movdqa    %%xmm1,%%xmm4                   \n"
    "punpcklbw %%xmm2,%%xmm4                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "movdqa    %%xmm4,%%xmm6                   \n"
    "punpcklwd %%xmm3,%%xmm6                   \n"
    "punpckhwd %%xmm3,%%xmm4                   \n"
    "movdqa    %%xmm1,%%xmm7                   \n"
    "punpcklwd %%xmm0,%%xmm7                   \n"
    "punpckhwd %%xmm0,%%xmm1                   \n"
    "sub       $0x10,%3                        \n"
    "movdqa    %%xmm6," MEMACCESS(2) "         \n"
    "movdqa    %%xmm4," MEMACCESS2(0x10,2) "   \n"
    "movdqa    %%xmm7," MEMACCESS2(0x20,2) "   \n"
    "movdqa    %%xmm1," MEMACCESS2(0x30,2) "   \n"
    "lea       " MEMLEA(0x40,2) ",%2           \n"
    "jg        1b                              \n"
  : "+r"(src_sobelx),  
    "+r"(src_sobely),  
    "+r"(dst_argb),    
    "+r"(width)        
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
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
    "pxor      %%xmm0,%%xmm0                   \n"
    "pxor      %%xmm1,%%xmm1                   \n"
    "sub       $0x4,%3                         \n"
    "jl        49f                             \n"
    "test      $0xf,%1                         \n"
    "jne       49f                             \n"

  
    LABELALIGN
  "40:                                         \n"
    "movdqu    " MEMACCESS(0) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
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
    "movdqa    " MEMACCESS(2) ",%%xmm2         \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "paddd     %%xmm3,%%xmm0                   \n"
    "movdqa    " MEMACCESS2(0x10,2) ",%%xmm3   \n"
    "paddd     %%xmm0,%%xmm3                   \n"
    "paddd     %%xmm4,%%xmm0                   \n"
    "movdqa    " MEMACCESS2(0x20,2) ",%%xmm4   \n"
    "paddd     %%xmm0,%%xmm4                   \n"
    "paddd     %%xmm5,%%xmm0                   \n"
    "movdqa    " MEMACCESS2(0x30,2) ",%%xmm5   \n"
    "lea       " MEMLEA(0x40,2) ",%2           \n"
    "paddd     %%xmm0,%%xmm5                   \n"
    "movdqa    %%xmm2," MEMACCESS(1) "         \n"
    "movdqa    %%xmm3," MEMACCESS2(0x10,1) "   \n"
    "movdqa    %%xmm4," MEMACCESS2(0x20,1) "   \n"
    "movdqa    %%xmm5," MEMACCESS2(0x30,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
    "sub       $0x4,%3                         \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        19f                             \n"

  
    LABELALIGN
  "10:                                         \n"
    "movd      " MEMACCESS(0) ",%%xmm2         \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "punpcklbw %%xmm1,%%xmm2                   \n"
    "punpcklwd %%xmm1,%%xmm2                   \n"
    "paddd     %%xmm2,%%xmm0                   \n"
    "movdqu    " MEMACCESS(2) ",%%xmm2         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "paddd     %%xmm0,%%xmm2                   \n"
    "movdqu    %%xmm2," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
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

#ifdef HAS_CUMULATIVESUMTOAVERAGEROW_SSE2
void CumulativeSumToAverageRow_SSE2(const int32* topleft, const int32* botleft,
                                    int width, int area, uint8* dst,
                                    int count) {
  asm volatile (
    "movd      %5,%%xmm5                       \n"
    "cvtdq2ps  %%xmm5,%%xmm5                   \n"
    "rcpss     %%xmm5,%%xmm4                   \n"
    "pshufd    $0x0,%%xmm4,%%xmm4              \n"
    "sub       $0x4,%3                         \n"
    "jl        49f                             \n"
    "cmpl      $0x80,%5                        \n"
    "ja        40f                             \n"

    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "pcmpeqb   %%xmm6,%%xmm6                   \n"
    "psrld     $0x10,%%xmm6                    \n"
    "cvtdq2ps  %%xmm6,%%xmm6                   \n"
    "addps     %%xmm6,%%xmm5                   \n"
    "mulps     %%xmm4,%%xmm5                   \n"
    "cvtps2dq  %%xmm5,%%xmm5                   \n"
    "packssdw  %%xmm5,%%xmm5                   \n"

  
    LABELALIGN
  "4:                                         \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    BUNDLEALIGN
    MEMOPREG(psubd,0x00,0,4,4,xmm0)            
    MEMOPREG(psubd,0x10,0,4,4,xmm1)            
    MEMOPREG(psubd,0x20,0,4,4,xmm2)            
    MEMOPREG(psubd,0x30,0,4,4,xmm3)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "psubd     " MEMACCESS(1) ",%%xmm0         \n"
    "psubd     " MEMACCESS2(0x10,1) ",%%xmm1   \n"
    "psubd     " MEMACCESS2(0x20,1) ",%%xmm2   \n"
    "psubd     " MEMACCESS2(0x30,1) ",%%xmm3   \n"
    BUNDLEALIGN
    MEMOPREG(paddd,0x00,1,4,4,xmm0)            
    MEMOPREG(paddd,0x10,1,4,4,xmm1)            
    MEMOPREG(paddd,0x20,1,4,4,xmm2)            
    MEMOPREG(paddd,0x30,1,4,4,xmm3)            
    "lea       " MEMLEA(0x40,1) ",%1           \n"
    "packssdw  %%xmm1,%%xmm0                   \n"
    "packssdw  %%xmm3,%%xmm2                   \n"
    "pmulhuw   %%xmm5,%%xmm0                   \n"
    "pmulhuw   %%xmm5,%%xmm2                   \n"
    "packuswb  %%xmm2,%%xmm0                   \n"
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "sub       $0x4,%3                         \n"
    "jge       4b                              \n"
    "jmp       49f                             \n"

  
    LABELALIGN
  "40:                                         \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "movdqa    " MEMACCESS2(0x20,0) ",%%xmm2   \n"
    "movdqa    " MEMACCESS2(0x30,0) ",%%xmm3   \n"
    BUNDLEALIGN
    MEMOPREG(psubd,0x00,0,4,4,xmm0)            
    MEMOPREG(psubd,0x10,0,4,4,xmm1)            
    MEMOPREG(psubd,0x20,0,4,4,xmm2)            
    MEMOPREG(psubd,0x30,0,4,4,xmm3)            
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "psubd     " MEMACCESS(1) ",%%xmm0         \n"
    "psubd     " MEMACCESS2(0x10,1) ",%%xmm1   \n"
    "psubd     " MEMACCESS2(0x20,1) ",%%xmm2   \n"
    "psubd     " MEMACCESS2(0x30,1) ",%%xmm3   \n"
    BUNDLEALIGN
    MEMOPREG(paddd,0x00,1,4,4,xmm0)            
    MEMOPREG(paddd,0x10,1,4,4,xmm1)            
    MEMOPREG(paddd,0x20,1,4,4,xmm2)            
    MEMOPREG(paddd,0x30,1,4,4,xmm3)            
    "lea       " MEMLEA(0x40,1) ",%1           \n"
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
    "movdqu    %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "sub       $0x4,%3                         \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%3                         \n"
    "jl        19f                             \n"

  
    LABELALIGN
  "10:                                         \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(psubd,0x00,0,4,4,xmm0)            
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "psubd     " MEMACCESS(1) ",%%xmm0         \n"
    BUNDLEALIGN
    MEMOPREG(paddd,0x00,1,4,4,xmm0)            
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "cvtdq2ps  %%xmm0,%%xmm0                   \n"
    "mulps     %%xmm4,%%xmm0                   \n"
    "cvtps2dq  %%xmm0,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x4,2) ",%2            \n"
    "sub       $0x1,%3                         \n"
    "jge       10b                             \n"
  "19:                                         \n"
  : "+r"(topleft),  
    "+r"(botleft),  
    "+r"(dst),      
    "+rm"(count)    
  : "r"((intptr_t)(width)),  
    "rm"(area)     
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}
#endif  

#ifdef HAS_ARGBAFFINEROW_SSE2

LIBYUV_API
void ARGBAffineRow_SSE2(const uint8* src_argb, int src_argb_stride,
                        uint8* dst_argb, const float* src_dudv, int width) {
  intptr_t src_argb_stride_temp = src_argb_stride;
  intptr_t temp = 0;
  asm volatile (
    "movq      " MEMACCESS(3) ",%%xmm2         \n"
    "movq      " MEMACCESS2(0x08,3) ",%%xmm7   \n"
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

  
    LABELALIGN
  "40:                                         \n"
    "cvttps2dq %%xmm2,%%xmm0                   \n"  
    "cvttps2dq %%xmm3,%%xmm1                   \n"  
    "packssdw  %%xmm1,%%xmm0                   \n"  
    "pmaddwd   %%xmm5,%%xmm0                   \n"  
    "movd      %%xmm0,%k1                      \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
    "movd      %%xmm0,%k5                      \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,0,1,1,xmm1)             
    MEMOPREG(movd,0x00,0,5,1,xmm6)             
    "punpckldq %%xmm6,%%xmm1                   \n"
    "addps     %%xmm4,%%xmm2                   \n"
    "movq      %%xmm1," MEMACCESS(2) "         \n"
    "movd      %%xmm0,%k1                      \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"
    "movd      %%xmm0,%k5                      \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,0,1,1,xmm0)             
    MEMOPREG(movd,0x00,0,5,1,xmm6)             
    "punpckldq %%xmm6,%%xmm0                   \n"
    "addps     %%xmm4,%%xmm3                   \n"
    "sub       $0x4,%4                         \n"
    "movq      %%xmm0," MEMACCESS2(0x08,2) "   \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "jge       40b                             \n"

  "49:                                         \n"
    "add       $0x3,%4                         \n"
    "jl        19f                             \n"

  
    LABELALIGN
  "10:                                         \n"
    "cvttps2dq %%xmm2,%%xmm0                   \n"
    "packssdw  %%xmm0,%%xmm0                   \n"
    "pmaddwd   %%xmm5,%%xmm0                   \n"
    "addps     %%xmm7,%%xmm2                   \n"
    "movd      %%xmm0,%k1                      \n"
    BUNDLEALIGN
    MEMOPREG(movd,0x00,0,1,1,xmm0)             
    "sub       $0x1,%4                         \n"
    "movd      %%xmm0," MEMACCESS(2) "         \n"
    "lea       " MEMLEA(0x04,2) ",%2           \n"
    "jge       10b                             \n"
  "19:                                         \n"
  : "+r"(src_argb),  
    "+r"(src_argb_stride_temp),  
    "+r"(dst_argb),  
    "+r"(src_dudv),  
    "+rm"(width),    
    "+r"(temp)   
  :
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_INTERPOLATEROW_SSSE3

void InterpolateRow_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                          ptrdiff_t src_stride, int dst_width,
                          int source_y_fraction) {
  asm volatile (
    "sub       %1,%0                           \n"
    "shr       %3                              \n"
    "cmp       $0x0,%3                         \n"
    "je        100f                            \n"
    "cmp       $0x20,%3                        \n"
    "je        75f                             \n"
    "cmp       $0x40,%3                        \n"
    "je        50f                             \n"
    "cmp       $0x60,%3                        \n"
    "je        25f                             \n"

    "movd      %3,%%xmm0                       \n"
    "neg       %3                              \n"
    "add       $0x80,%3                        \n"
    "movd      %3,%%xmm5                       \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm5                   \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm2)
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm0                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "pmaddubsw %%xmm5,%%xmm0                   \n"
    "pmaddubsw %%xmm5,%%xmm1                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "25:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm1)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        25b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "50:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm1)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        50b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "75:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm1         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm0)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        75b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "100:                                        \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    "sub       $0x10,%2                        \n"
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        100b                            \n"

  "99:                                         \n"
  : "+r"(dst_ptr),    
    "+r"(src_ptr),    
    "+r"(dst_width),  
    "+r"(source_y_fraction)  
  : "r"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_INTERPOLATEROW_SSE2

void InterpolateRow_SSE2(uint8* dst_ptr, const uint8* src_ptr,
                         ptrdiff_t src_stride, int dst_width,
                         int source_y_fraction) {
  asm volatile (
    "sub       %1,%0                           \n"
    "shr       %3                              \n"
    "cmp       $0x0,%3                         \n"
    "je        100f                            \n"
    "cmp       $0x20,%3                        \n"
    "je        75f                             \n"
    "cmp       $0x40,%3                        \n"
    "je        50f                             \n"
    "cmp       $0x60,%3                        \n"
    "je        25f                             \n"

    "movd      %3,%%xmm0                       \n"
    "neg       %3                              \n"
    "add       $0x80,%3                        \n"
    "movd      %3,%%xmm5                       \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm5                   \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "pxor      %%xmm4,%%xmm4                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm2)           
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm2,%%xmm3                   \n"
    "punpcklbw %%xmm4,%%xmm2                   \n"
    "punpckhbw %%xmm4,%%xmm3                   \n"
    "punpcklbw %%xmm4,%%xmm0                   \n"
    "punpckhbw %%xmm4,%%xmm1                   \n"
    "psubw     %%xmm0,%%xmm2                   \n"
    "psubw     %%xmm1,%%xmm3                   \n"
    "paddw     %%xmm2,%%xmm2                   \n"
    "paddw     %%xmm3,%%xmm3                   \n"
    "pmulhw    %%xmm5,%%xmm2                   \n"
    "pmulhw    %%xmm5,%%xmm3                   \n"
    "paddw     %%xmm2,%%xmm0                   \n"
    "paddw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "25:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm1)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        25b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "50:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm1)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        50b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "75:                                         \n"
    "movdqa    " MEMACCESS(1) ",%%xmm1         \n"
    MEMOPREG(movdqa,0x00,1,4,1,xmm0)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        75b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "100:                                        \n"
    "movdqa    " MEMACCESS(1) ",%%xmm0         \n"
    "sub       $0x10,%2                        \n"
    MEMOPMEM(movdqa,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        100b                            \n"

  "99:                                         \n"
  : "+r"(dst_ptr),    
    "+r"(src_ptr),    
    "+r"(dst_width),  
    "+r"(source_y_fraction)  
  : "r"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_INTERPOLATEROW_SSSE3

void InterpolateRow_Unaligned_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                                    ptrdiff_t src_stride, int dst_width,
                                    int source_y_fraction) {
  asm volatile (
    "sub       %1,%0                           \n"
    "shr       %3                              \n"
    "cmp       $0x0,%3                         \n"
    "je        100f                            \n"
    "cmp       $0x20,%3                        \n"
    "je        75f                             \n"
    "cmp       $0x40,%3                        \n"
    "je        50f                             \n"
    "cmp       $0x60,%3                        \n"
    "je        25f                             \n"

    "movd      %3,%%xmm0                       \n"
    "neg       %3                              \n"
    "add       $0x80,%3                        \n"
    "movd      %3,%%xmm5                       \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm5                   \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm2)
    "movdqu    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm2,%%xmm0                   \n"
    "punpckhbw %%xmm2,%%xmm1                   \n"
    "pmaddubsw %%xmm5,%%xmm0                   \n"
    "pmaddubsw %%xmm5,%%xmm1                   \n"
    "psrlw     $0x7,%%xmm0                     \n"
    "psrlw     $0x7,%%xmm1                     \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "25:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm1)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        25b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "50:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm1)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        50b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "75:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm0)
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        75b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "100:                                        \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    "sub       $0x10,%2                        \n"
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        100b                            \n"

  "99:                                         \n"
  : "+r"(dst_ptr),    
    "+r"(src_ptr),    
    "+r"(dst_width),  
    "+r"(source_y_fraction)  
  : "r"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm5"
#endif
  );
}
#endif   

#ifdef HAS_INTERPOLATEROW_SSE2

void InterpolateRow_Unaligned_SSE2(uint8* dst_ptr, const uint8* src_ptr,
                                   ptrdiff_t src_stride, int dst_width,
                                   int source_y_fraction) {
  asm volatile (
    "sub       %1,%0                           \n"
    "shr       %3                              \n"
    "cmp       $0x0,%3                         \n"
    "je        100f                            \n"
    "cmp       $0x20,%3                        \n"
    "je        75f                             \n"
    "cmp       $0x40,%3                        \n"
    "je        50f                             \n"
    "cmp       $0x60,%3                        \n"
    "je        25f                             \n"

    "movd      %3,%%xmm0                       \n"
    "neg       %3                              \n"
    "add       $0x80,%3                        \n"
    "movd      %3,%%xmm5                       \n"
    "punpcklbw %%xmm0,%%xmm5                   \n"
    "punpcklwd %%xmm5,%%xmm5                   \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    "pxor      %%xmm4,%%xmm4                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm2)           
    "movdqu    %%xmm0,%%xmm1                   \n"
    "movdqu    %%xmm2,%%xmm3                   \n"
    "punpcklbw %%xmm4,%%xmm2                   \n"
    "punpckhbw %%xmm4,%%xmm3                   \n"
    "punpcklbw %%xmm4,%%xmm0                   \n"
    "punpckhbw %%xmm4,%%xmm1                   \n"
    "psubw     %%xmm0,%%xmm2                   \n"
    "psubw     %%xmm1,%%xmm3                   \n"
    "paddw     %%xmm2,%%xmm2                   \n"
    "paddw     %%xmm3,%%xmm3                   \n"
    "pmulhw    %%xmm5,%%xmm2                   \n"
    "pmulhw    %%xmm5,%%xmm3                   \n"
    "paddw     %%xmm2,%%xmm0                   \n"
    "paddw     %%xmm3,%%xmm1                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        1b                              \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "25:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm1)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        25b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "50:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm1)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        50b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "75:                                         \n"
    "movdqu    " MEMACCESS(1) ",%%xmm1         \n"
    MEMOPREG(movdqu,0x00,1,4,1,xmm0)           
    "pavgb     %%xmm1,%%xmm0                   \n"
    "pavgb     %%xmm1,%%xmm0                   \n"
    "sub       $0x10,%2                        \n"
    BUNDLEALIGN
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        75b                             \n"
    "jmp       99f                             \n"

    
    LABELALIGN
  "100:                                        \n"
    "movdqu    " MEMACCESS(1) ",%%xmm0         \n"
    "sub       $0x10,%2                        \n"
    MEMOPMEM(movdqu,xmm0,0x00,1,0,1)           
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        100b                            \n"

  "99:                                         \n"
  : "+r"(dst_ptr),    
    "+r"(src_ptr),    
    "+r"(dst_width),  
    "+r"(source_y_fraction)  
  : "r"((intptr_t)(src_stride))  
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_HALFROW_SSE2
void HalfRow_SSE2(const uint8* src_uv, int src_uv_stride,
                  uint8* dst_uv, int pix) {
  asm volatile (
    "sub       %0,%1                           \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    MEMOPREG(pavgb,0x00,0,3,1,xmm0)            
    "sub       $0x10,%2                        \n"
    MEMOPMEM(movdqa,xmm0,0x00,0,1,1)           
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "jg        1b                              \n"
  : "+r"(src_uv),  
    "+r"(dst_uv),  
    "+r"(pix)      
  : "r"((intptr_t)(src_uv_stride))  
  : "memory", "cc"
#if defined(__SSE2__)
      , "xmm0"
#endif
  );
}
#endif  

#ifdef HAS_ARGBTOBAYERROW_SSSE3
void ARGBToBayerRow_SSSE3(const uint8* src_argb, uint8* dst_bayer,
                          uint32 selector, int pix) {
  asm volatile (
    
    "movd      %3,%%xmm5                       \n"
    "pshufd    $0x0,%%xmm5,%%xmm5              \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "punpckldq %%xmm1,%%xmm0                   \n"
    "sub       $0x8,%2                         \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_bayer), 
    "+r"(pix)        
  : "g"(selector)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBTOBAYERGGROW_SSE2
void ARGBToBayerGGRow_SSE2(const uint8* src_argb, uint8* dst_bayer,
                           uint32 selector, int pix) {
  asm volatile (
    "pcmpeqb   %%xmm5,%%xmm5                   \n"
    "psrld     $0x18,%%xmm5                    \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "psrld     $0x8,%%xmm0                     \n"
    "psrld     $0x8,%%xmm1                     \n"
    "pand      %%xmm5,%%xmm0                   \n"
    "pand      %%xmm5,%%xmm1                   \n"
    "packssdw  %%xmm1,%%xmm0                   \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x8,%2                         \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_bayer), 
    "+r"(pix)        
  :
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSHUFFLEROW_SSSE3

void ARGBShuffleRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                          const uint8* shuffler, int pix) {
  asm volatile (
    "movdqa    " MEMACCESS(3) ",%%xmm5         \n"
    LABELALIGN
  "1:                                          \n"
    "movdqa    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqa    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "sub       $0x8,%2                         \n"
    "movdqa    %%xmm0," MEMACCESS(1) "         \n"
    "movdqa    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "r"(shuffler)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}

void ARGBShuffleRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_argb,
                                    const uint8* shuffler, int pix) {
  asm volatile (
    "movdqa    " MEMACCESS(3) ",%%xmm5         \n"
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "movdqu    " MEMACCESS2(0x10,0) ",%%xmm1   \n"
    "lea       " MEMLEA(0x20,0) ",%0           \n"
    "pshufb    %%xmm5,%%xmm0                   \n"
    "pshufb    %%xmm5,%%xmm1                   \n"
    "sub       $0x8,%2                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,1) "   \n"
    "lea       " MEMLEA(0x20,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "r"(shuffler)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSHUFFLEROW_AVX2

void ARGBShuffleRow_AVX2(const uint8* src_argb, uint8* dst_argb,
                         const uint8* shuffler, int pix) {
  asm volatile (
    "vbroadcastf128 " MEMACCESS(3) ",%%ymm5    \n"
    LABELALIGN
  "1:                                          \n"
    "vmovdqu   " MEMACCESS(0) ",%%ymm0         \n"
    "vmovdqu   " MEMACCESS2(0x20,0) ",%%ymm1   \n"
    "lea       " MEMLEA(0x40,0) ",%0           \n"
    "vpshufb   %%ymm5,%%ymm0,%%ymm0            \n"
    "vpshufb   %%ymm5,%%ymm1,%%ymm1            \n"
    "sub       $0x10,%2                        \n"
    "vmovdqu   %%ymm0," MEMACCESS(1) "         \n"
    "vmovdqu   %%ymm1," MEMACCESS2(0x20,1) "   \n"
    "lea       " MEMLEA(0x40,1) ",%1           \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(pix)        
  : "r"(shuffler)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_ARGBSHUFFLEROW_SSE2

void ARGBShuffleRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                         const uint8* shuffler, int pix) {
  uintptr_t pixel_temp = 0u;
  asm volatile (
    "pxor      %%xmm5,%%xmm5                   \n"
    "mov       " MEMACCESS(4) ",%k2            \n"
    "cmp       $0x3000102,%k2                  \n"
    "je        3012f                           \n"
    "cmp       $0x10203,%k2                    \n"
    "je        123f                            \n"
    "cmp       $0x30201,%k2                    \n"
    "je        321f                            \n"
    "cmp       $0x2010003,%k2                  \n"
    "je        2103f                           \n"

    LABELALIGN
  "1:                                          \n"
    "movzb     " MEMACCESS(4) ",%2             \n"
    MEMOPARG(movzb,0x00,0,2,1,2) "             \n"  
    "mov       %b2," MEMACCESS(1) "            \n"
    "movzb     " MEMACCESS2(0x1,4) ",%2        \n"
    MEMOPARG(movzb,0x00,0,2,1,2) "             \n"  
    "mov       %b2," MEMACCESS2(0x1,1) "       \n"
    BUNDLEALIGN
    "movzb     " MEMACCESS2(0x2,4) ",%2        \n"
    MEMOPARG(movzb,0x00,0,2,1,2) "             \n"  
    "mov       %b2," MEMACCESS2(0x2,1) "       \n"
    "movzb     " MEMACCESS2(0x3,4) ",%2        \n"
    MEMOPARG(movzb,0x00,0,2,1,2) "             \n"  
    "mov       %b2," MEMACCESS2(0x3,1) "       \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    "lea       " MEMLEA(0x4,1) ",%1            \n"
    "sub       $0x1,%3                         \n"
    "jg        1b                              \n"
    "jmp       99f                             \n"

    LABELALIGN
  "123:                                        \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pshufhw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshuflw   $0x1b,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x1b,%%xmm1,%%xmm1             \n"
    "pshuflw   $0x1b,%%xmm1,%%xmm1             \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        123b                            \n"
    "jmp       99f                             \n"

    LABELALIGN
  "321:                                        \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pshufhw   $0x39,%%xmm0,%%xmm0             \n"
    "pshuflw   $0x39,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x39,%%xmm1,%%xmm1             \n"
    "pshuflw   $0x39,%%xmm1,%%xmm1             \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        321b                            \n"
    "jmp       99f                             \n"

    LABELALIGN
  "2103:                                       \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pshufhw   $0x93,%%xmm0,%%xmm0             \n"
    "pshuflw   $0x93,%%xmm0,%%xmm0             \n"
    "pshufhw   $0x93,%%xmm1,%%xmm1             \n"
    "pshuflw   $0x93,%%xmm1,%%xmm1             \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        2103b                           \n"
    "jmp       99f                             \n"

    LABELALIGN
  "3012:                                       \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x10,0) ",%0           \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "punpcklbw %%xmm5,%%xmm0                   \n"
    "punpckhbw %%xmm5,%%xmm1                   \n"
    "pshufhw   $0xc6,%%xmm0,%%xmm0             \n"
    "pshuflw   $0xc6,%%xmm0,%%xmm0             \n"
    "pshufhw   $0xc6,%%xmm1,%%xmm1             \n"
    "pshuflw   $0xc6,%%xmm1,%%xmm1             \n"
    "packuswb  %%xmm1,%%xmm0                   \n"
    "sub       $0x4,%3                         \n"
    "movdqu    %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x10,1) ",%1           \n"
    "jg        3012b                           \n"

  "99:                                         \n"
  : "+r"(src_argb),    
    "+r"(dst_argb),    
    "+d"(pixel_temp),  
    "+r"(pix)         
  : "r"(shuffler)      
  : "memory", "cc"
#if defined(__native_client__) && defined(__x86_64__)
    , "r14"
#endif
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm5"
#endif
  );
}
#endif  

#ifdef HAS_I422TOYUY2ROW_SSE2
void I422ToYUY2Row_SSE2(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_frame, int width) {
 asm volatile (
    "sub       %1,%2                             \n"
    LABELALIGN
  "1:                                            \n"
    "movq      " MEMACCESS(1) ",%%xmm2           \n"
    MEMOPREG(movq,0x00,1,2,1,xmm3)               
    "lea       " MEMLEA(0x8,1) ",%1              \n"
    "punpcklbw %%xmm3,%%xmm2                     \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0           \n"
    "lea       " MEMLEA(0x10,0) ",%0             \n"
    "movdqa    %%xmm0,%%xmm1                     \n"
    "punpcklbw %%xmm2,%%xmm0                     \n"
    "punpckhbw %%xmm2,%%xmm1                     \n"
    "movdqu    %%xmm0," MEMACCESS(3) "           \n"
    "movdqu    %%xmm1," MEMACCESS2(0x10,3) "     \n"
    "lea       " MEMLEA(0x20,3) ",%3             \n"
    "sub       $0x10,%4                          \n"
    "jg         1b                               \n"
    : "+r"(src_y),  
      "+r"(src_u),  
      "+r"(src_v),  
      "+r"(dst_frame),  
      "+rm"(width)  
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
#endif  

#ifdef HAS_I422TOUYVYROW_SSE2
void I422ToUYVYRow_SSE2(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_frame, int width) {
 asm volatile (
    "sub        %1,%2                            \n"
    LABELALIGN
  "1:                                            \n"
    "movq      " MEMACCESS(1) ",%%xmm2           \n"
    MEMOPREG(movq,0x00,1,2,1,xmm3)               
    "lea       " MEMLEA(0x8,1) ",%1              \n"
    "punpcklbw %%xmm3,%%xmm2                     \n"
    "movdqu    " MEMACCESS(0) ",%%xmm0           \n"
    "movdqa    %%xmm2,%%xmm1                     \n"
    "lea       " MEMLEA(0x10,0) ",%0             \n"
    "punpcklbw %%xmm0,%%xmm1                     \n"
    "punpckhbw %%xmm0,%%xmm2                     \n"
    "movdqu    %%xmm1," MEMACCESS(3) "           \n"
    "movdqu    %%xmm2," MEMACCESS2(0x10,3) "     \n"
    "lea       " MEMLEA(0x20,3) ",%3             \n"
    "sub       $0x10,%4                          \n"
    "jg         1b                               \n"
    : "+r"(src_y),  
      "+r"(src_u),  
      "+r"(src_v),  
      "+r"(dst_frame),  
      "+rm"(width)  
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
#endif  

#ifdef HAS_ARGBPOLYNOMIALROW_SSE2
void ARGBPolynomialRow_SSE2(const uint8* src_argb,
                            uint8* dst_argb, const float* poly,
                            int width) {
  asm volatile (
    "pxor      %%xmm3,%%xmm3                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movq      " MEMACCESS(0) ",%%xmm0         \n"
    "lea       " MEMLEA(0x8,0) ",%0            \n"
    "punpcklbw %%xmm3,%%xmm0                   \n"
    "movdqa    %%xmm0,%%xmm4                   \n"
    "punpcklwd %%xmm3,%%xmm0                   \n"
    "punpckhwd %%xmm3,%%xmm4                   \n"
    "cvtdq2ps  %%xmm0,%%xmm0                   \n"
    "cvtdq2ps  %%xmm4,%%xmm4                   \n"
    "movdqa    %%xmm0,%%xmm1                   \n"
    "movdqa    %%xmm4,%%xmm5                   \n"
    "mulps     " MEMACCESS2(0x10,3) ",%%xmm0   \n"
    "mulps     " MEMACCESS2(0x10,3) ",%%xmm4   \n"
    "addps     " MEMACCESS(3) ",%%xmm0         \n"
    "addps     " MEMACCESS(3) ",%%xmm4         \n"
    "movdqa    %%xmm1,%%xmm2                   \n"
    "movdqa    %%xmm5,%%xmm6                   \n"
    "mulps     %%xmm1,%%xmm2                   \n"
    "mulps     %%xmm5,%%xmm6                   \n"
    "mulps     %%xmm2,%%xmm1                   \n"
    "mulps     %%xmm6,%%xmm5                   \n"
    "mulps     " MEMACCESS2(0x20,3) ",%%xmm2   \n"
    "mulps     " MEMACCESS2(0x20,3) ",%%xmm6   \n"
    "mulps     " MEMACCESS2(0x30,3) ",%%xmm1   \n"
    "mulps     " MEMACCESS2(0x30,3) ",%%xmm5   \n"
    "addps     %%xmm2,%%xmm0                   \n"
    "addps     %%xmm6,%%xmm4                   \n"
    "addps     %%xmm1,%%xmm0                   \n"
    "addps     %%xmm5,%%xmm4                   \n"
    "cvttps2dq %%xmm0,%%xmm0                   \n"
    "cvttps2dq %%xmm4,%%xmm4                   \n"
    "packuswb  %%xmm4,%%xmm0                   \n"
    "packuswb  %%xmm0,%%xmm0                   \n"
    "sub       $0x2,%2                         \n"
    "movq      %%xmm0," MEMACCESS(1) "         \n"
    "lea       " MEMLEA(0x8,1) ",%1            \n"
    "jg        1b                              \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(width)      
  : "r"(poly)        
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
#endif
  );
}
#endif  

#ifdef HAS_ARGBPOLYNOMIALROW_AVX2
void ARGBPolynomialRow_AVX2(const uint8* src_argb,
                            uint8* dst_argb, const float* poly,
                            int width) {
  asm volatile (
    "vbroadcastf128 " MEMACCESS(3) ",%%ymm4     \n"
    "vbroadcastf128 " MEMACCESS2(0x10,3) ",%%ymm5 \n"
    "vbroadcastf128 " MEMACCESS2(0x20,3) ",%%ymm6 \n"
    "vbroadcastf128 " MEMACCESS2(0x30,3) ",%%ymm7 \n"

    
    LABELALIGN
  "1:                                          \n"
    "vpmovzxbd   " MEMACCESS(0) ",%%ymm0       \n"  
    "lea         " MEMLEA(0x8,0) ",%0          \n"
    "vcvtdq2ps   %%ymm0,%%ymm0                 \n"  
    "vmulps      %%ymm0,%%ymm0,%%ymm2          \n"  
    "vmulps      %%ymm7,%%ymm0,%%ymm3          \n"  
    "vfmadd132ps %%ymm5,%%ymm4,%%ymm0          \n"  
    "vfmadd231ps %%ymm6,%%ymm2,%%ymm0          \n"  
    "vfmadd231ps %%ymm3,%%ymm2,%%ymm0          \n"  
    "vcvttps2dq  %%ymm0,%%ymm0                 \n"
    "vpackusdw   %%ymm0,%%ymm0,%%ymm0          \n"
    "vpermq      $0xd8,%%ymm0,%%ymm0           \n"
    "vpackuswb   %%xmm0,%%xmm0,%%xmm0          \n"
    "sub         $0x2,%2                       \n"
    "vmovq       %%xmm0," MEMACCESS(1) "       \n"
    "lea         " MEMLEA(0x8,1) ",%1          \n"
    "jg          1b                            \n"
    "vzeroupper                                \n"
  : "+r"(src_argb),  
    "+r"(dst_argb),  
    "+r"(width)      
  : "r"(poly)        
  : "memory", "cc"
#if defined(__SSE2__)

    , "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
#endif
  );
}
#endif  

#ifdef HAS_ARGBCOLORTABLEROW_X86

void ARGBColorTableRow_X86(uint8* dst_argb, const uint8* table_argb,
                           int width) {
  uintptr_t pixel_temp = 0u;
  asm volatile (
    
    LABELALIGN
  "1:                                          \n"
    "movzb     " MEMACCESS(0) ",%1             \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    MEMOPARG(movzb,0x00,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x4,0) "      \n"
    "movzb     " MEMACCESS2(-0x3,0) ",%1       \n"
    MEMOPARG(movzb,0x01,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x3,0) "      \n"
    "movzb     " MEMACCESS2(-0x2,0) ",%1       \n"
    MEMOPARG(movzb,0x02,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x2,0) "      \n"
    "movzb     " MEMACCESS2(-0x1,0) ",%1       \n"
    MEMOPARG(movzb,0x03,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x1,0) "      \n"
    "dec       %2                              \n"
    "jg        1b                              \n"
  : "+r"(dst_argb),   
    "+d"(pixel_temp), 
    "+r"(width)       
  : "r"(table_argb)   
  : "memory", "cc");
}
#endif  

#ifdef HAS_RGBCOLORTABLEROW_X86

void RGBColorTableRow_X86(uint8* dst_argb, const uint8* table_argb, int width) {
  uintptr_t pixel_temp = 0u;
  asm volatile (
    
    LABELALIGN
  "1:                                          \n"
    "movzb     " MEMACCESS(0) ",%1             \n"
    "lea       " MEMLEA(0x4,0) ",%0            \n"
    MEMOPARG(movzb,0x00,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x4,0) "      \n"
    "movzb     " MEMACCESS2(-0x3,0) ",%1       \n"
    MEMOPARG(movzb,0x01,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x3,0) "      \n"
    "movzb     " MEMACCESS2(-0x2,0) ",%1       \n"
    MEMOPARG(movzb,0x02,3,1,4,1) "             \n"  
    "mov       %b1," MEMACCESS2(-0x2,0) "      \n"
    "dec       %2                              \n"
    "jg        1b                              \n"
  : "+r"(dst_argb),   
    "+d"(pixel_temp), 
    "+r"(width)       
  : "r"(table_argb)   
  : "memory", "cc");
}
#endif  

#ifdef HAS_ARGBLUMACOLORTABLEROW_SSSE3

void ARGBLumaColorTableRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                                 int width,
                                 const uint8* luma, uint32 lumacoeff) {
  uintptr_t pixel_temp = 0u;
  uintptr_t table_temp = 0u;
  asm volatile (
    "movd      %6,%%xmm3                       \n"
    "pshufd    $0x0,%%xmm3,%%xmm3              \n"
    "pcmpeqb   %%xmm4,%%xmm4                   \n"
    "psllw     $0x8,%%xmm4                     \n"
    "pxor      %%xmm5,%%xmm5                   \n"

    
    LABELALIGN
  "1:                                          \n"
    "movdqu    " MEMACCESS(2) ",%%xmm0         \n"
    "pmaddubsw %%xmm3,%%xmm0                   \n"
    "phaddw    %%xmm0,%%xmm0                   \n"
    "pand      %%xmm4,%%xmm0                   \n"
    "punpcklwd %%xmm5,%%xmm0                   \n"
    "movd      %%xmm0,%k1                      \n"  
    "add       %5,%1                           \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"

    "movzb     " MEMACCESS(2) ",%0             \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS(3) "            \n"
    "movzb     " MEMACCESS2(0x1,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x1,3) "       \n"
    "movzb     " MEMACCESS2(0x2,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x2,3) "       \n"
    "movzb     " MEMACCESS2(0x3,2) ",%0        \n"
    "mov       %b0," MEMACCESS2(0x3,3) "       \n"

    "movd      %%xmm0,%k1                      \n"  
    "add       %5,%1                           \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"

    "movzb     " MEMACCESS2(0x4,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x4,3) "       \n"
    BUNDLEALIGN
    "movzb     " MEMACCESS2(0x5,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x5,3) "       \n"
    "movzb     " MEMACCESS2(0x6,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x6,3) "       \n"
    "movzb     " MEMACCESS2(0x7,2) ",%0        \n"
    "mov       %b0," MEMACCESS2(0x7,3) "       \n"

    "movd      %%xmm0,%k1                      \n"  
    "add       %5,%1                           \n"
    "pshufd    $0x39,%%xmm0,%%xmm0             \n"

    "movzb     " MEMACCESS2(0x8,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x8,3) "       \n"
    "movzb     " MEMACCESS2(0x9,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0x9,3) "       \n"
    "movzb     " MEMACCESS2(0xa,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0xa,3) "       \n"
    "movzb     " MEMACCESS2(0xb,2) ",%0        \n"
    "mov       %b0," MEMACCESS2(0xb,3) "       \n"

    "movd      %%xmm0,%k1                      \n"  
    "add       %5,%1                           \n"

    "movzb     " MEMACCESS2(0xc,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0xc,3) "       \n"
    "movzb     " MEMACCESS2(0xd,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0xd,3) "       \n"
    "movzb     " MEMACCESS2(0xe,2) ",%0        \n"
    MEMOPARG(movzb,0x00,1,0,1,0) "             \n"  
    "mov       %b0," MEMACCESS2(0xe,3) "       \n"
    "movzb     " MEMACCESS2(0xf,2) ",%0        \n"
    "mov       %b0," MEMACCESS2(0xf,3) "       \n"
    "sub       $0x4,%4                         \n"
    "lea       " MEMLEA(0x10,2) ",%2           \n"
    "lea       " MEMLEA(0x10,3) ",%3           \n"
    "jg        1b                              \n"
  : "+d"(pixel_temp),  
    "+a"(table_temp),  
    "+r"(src_argb),    
    "+r"(dst_argb),    
    "+rm"(width)       
  : "r"(luma),         
    "rm"(lumacoeff)    
  : "memory", "cc"
#if defined(__SSE2__)
    , "xmm0", "xmm3", "xmm4", "xmm5"
#endif
  );
}
#endif  

#endif

#ifdef __cplusplus
}  
}  
#endif
