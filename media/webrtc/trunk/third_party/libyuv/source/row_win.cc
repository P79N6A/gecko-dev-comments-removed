









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(YUV_DISABLE_ASM) && defined(_M_IX86)

#ifdef HAS_ARGBTOYROW_SSSE3


static const vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};

static const vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

static const vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};


static const vec8 kBGRAToY = {
  0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13
};

static const vec8 kBGRAToU = {
  0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112
};

static const vec8 kBGRAToV = {
  0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18
};


static const vec8 kABGRToY = {
  33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0, 33, 65, 13, 0
};

static const vec8 kABGRToU = {
  -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0, -38, -74, 112, 0
};

static const vec8 kABGRToV = {
  112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0, 112, -94, -18, 0
};


static const vec8 kRGBAToY = {
  0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33
};

static const vec8 kRGBAToU = {
  0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38
};

static const vec8 kRGBAToV = {
  0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112
};

static const uvec8 kAddY16 = {
  16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u, 16u
};

static const uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};


static const uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};


static const uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};


static const uvec8 kShuffleMaskBGRAToARGB = {
  3u, 2u, 1u, 0u, 7u, 6u, 5u, 4u, 11u, 10u, 9u, 8u, 15u, 14u, 13u, 12u
};


static const uvec8 kShuffleMaskABGRToARGB = {
  2u, 1u, 0u, 3u, 6u, 5u, 4u, 7u, 10u, 9u, 8u, 11u, 14u, 13u, 12u, 15u
};


static const uvec8 kShuffleMaskRGBAToARGB = {
  1u, 2u, 3u, 0u, 5u, 6u, 7u, 4u, 9u, 10u, 11u, 8u, 13u, 14u, 15u, 12u
};


static const uvec8 kShuffleMaskARGBToRGBA = {
  3u, 0u, 1u, 2u, 7u, 4u, 5u, 6u, 11u, 8u, 9u, 10u, 15u, 12u, 13u, 14u
};


static const uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};


static const uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};

__declspec(naked) __declspec(align(16))
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        
    mov        edx, [esp + 8]        
    mov        ecx, [esp + 12]       
    pcmpeqb    xmm5, xmm5            
    pslld      xmm5, 24

    align      16
  convertloop:
    movq       xmm0, qword ptr [eax]
    lea        eax,  [eax + 8]
    punpcklbw  xmm0, xmm0
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0
    punpckhwd  xmm1, xmm1
    por        xmm0, xmm5
    por        xmm1, xmm5
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToARGBRow_SSSE3(const uint8* src_bgra, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMaskBGRAToARGB
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToARGBRow_SSSE3(const uint8* src_abgr, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMaskABGRToARGB
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGBAToARGBRow_SSSE3(const uint8* src_rgba, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMaskRGBAToARGB
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRGBARow_SSSE3(const uint8* src_argb, uint8* dst_rgba, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMaskARGBToRGBA
    sub       edx, eax

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [eax + edx], xmm0
    lea       eax, [eax + 16]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm5, xmm5       
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRGB24ToARGB

    align      16
 convertloop:
    movdqu    xmm0, [eax]
    movdqu    xmm1, [eax + 16]
    movdqu    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    
    pshufb    xmm2, xmm4
    por       xmm2, xmm5
    palignr   xmm1, xmm0, 12   
    pshufb    xmm0, xmm4
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm5
    pshufb    xmm1, xmm4
    movdqa    [edx], xmm0
    por       xmm1, xmm5
    palignr   xmm3, xmm3, 4    
    pshufb    xmm3, xmm4
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm5
    sub       ecx, 16
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb,
                        int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm5, xmm5       
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRAWToARGB

    align      16
 convertloop:
    movdqu    xmm0, [eax]
    movdqu    xmm1, [eax + 16]
    movdqu    xmm3, [eax + 32]
    lea       eax, [eax + 48]
    movdqa    xmm2, xmm3
    palignr   xmm2, xmm1, 8    
    pshufb    xmm2, xmm4
    por       xmm2, xmm5
    palignr   xmm1, xmm0, 12   
    pshufb    xmm0, xmm4
    movdqa    [edx + 32], xmm2
    por       xmm0, xmm5
    pshufb    xmm1, xmm4
    movdqa    [edx], xmm0
    por       xmm1, xmm5
    palignr   xmm3, xmm3, 4    
    pshufb    xmm3, xmm4
    movdqa    [edx + 16], xmm1
    por       xmm3, xmm5
    sub       ecx, 16
    movdqa    [edx + 48], xmm3
    lea       edx, [edx + 64]
    jg        convertloop
    ret
  }
}








__declspec(naked) __declspec(align(16))
void RGB565ToARGBRow_SSE2(const uint8* src_rgb565, uint8* dst_argb,
                          int pix) {
__asm {
    mov       eax, 0x01080108  
    movd      xmm5, eax
    pshufd    xmm5, xmm5, 0
    mov       eax, 0x20802080  
    movd      xmm6, eax
    pshufd    xmm6, xmm6, 0
    pcmpeqb   xmm3, xmm3       
    psllw     xmm3, 11
    pcmpeqb   xmm4, xmm4       
    psllw     xmm4, 10
    psrlw     xmm4, 5
    pcmpeqb   xmm7, xmm7       
    psllw     xmm7, 8

    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    sub       edx, eax
    sub       edx, eax

    align      16
 convertloop:
    movdqu    xmm0, [eax]   
    movdqa    xmm1, xmm0
    movdqa    xmm2, xmm0
    pand      xmm1, xmm3    
    psllw     xmm2, 11      
    pmulhuw   xmm1, xmm5    
    pmulhuw   xmm2, xmm5    
    psllw     xmm1, 8
    por       xmm1, xmm2    
    pand      xmm0, xmm4    
    pmulhuw   xmm0, xmm6    
    por       xmm0, xmm7    
    movdqa    xmm2, xmm1
    punpcklbw xmm1, xmm0
    punpckhbw xmm2, xmm0
    movdqa    [eax * 2 + edx], xmm1  
    movdqa    [eax * 2 + edx + 16], xmm2  
    lea       eax, [eax + 16]
    sub       ecx, 8
    jg        convertloop
    ret
  }
}


__declspec(naked) __declspec(align(16))
void ARGB1555ToARGBRow_SSE2(const uint8* src_argb1555, uint8* dst_argb,
                            int pix) {
__asm {
    mov       eax, 0x01080108  
    movd      xmm5, eax
    pshufd    xmm5, xmm5, 0
    mov       eax, 0x42004200  
    movd      xmm6, eax
    pshufd    xmm6, xmm6, 0
    pcmpeqb   xmm3, xmm3       
    psllw     xmm3, 11
    movdqa    xmm4, xmm3       
    psrlw     xmm4, 6
    pcmpeqb   xmm7, xmm7       
    psllw     xmm7, 8

    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    sub       edx, eax
    sub       edx, eax

    align      16
 convertloop:
    movdqu    xmm0, [eax]   
    movdqa    xmm1, xmm0
    movdqa    xmm2, xmm0
    psllw     xmm1, 1       
    psllw     xmm2, 11      
    pand      xmm1, xmm3
    pmulhuw   xmm2, xmm5    
    pmulhuw   xmm1, xmm5    
    psllw     xmm1, 8
    por       xmm1, xmm2    
    movdqa    xmm2, xmm0
    pand      xmm0, xmm4    
    psraw     xmm2, 8       
    pmulhuw   xmm0, xmm6    
    pand      xmm2, xmm7
    por       xmm0, xmm2    
    movdqa    xmm2, xmm1
    punpcklbw xmm1, xmm0
    punpckhbw xmm2, xmm0
    movdqa    [eax * 2 + edx], xmm1  
    movdqa    [eax * 2 + edx + 16], xmm2  
    lea       eax, [eax + 16]
    sub       ecx, 8
    jg        convertloop
    ret
  }
}


__declspec(naked) __declspec(align(16))
void ARGB4444ToARGBRow_SSE2(const uint8* src_argb4444, uint8* dst_argb,
                            int pix) {
__asm {
    mov       eax, 0x0f0f0f0f  
    movd      xmm4, eax
    pshufd    xmm4, xmm4, 0
    movdqa    xmm5, xmm4       
    pslld     xmm5, 4
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    sub       edx, eax
    sub       edx, eax

    align      16
 convertloop:
    movdqu    xmm0, [eax]   
    movdqa    xmm2, xmm0
    pand      xmm0, xmm4    
    pand      xmm2, xmm5    
    movdqa    xmm1, xmm0
    movdqa    xmm3, xmm2
    psllw     xmm1, 4
    psrlw     xmm3, 4
    por       xmm0, xmm1
    por       xmm2, xmm3
    movdqa    xmm1, xmm0
    punpcklbw xmm0, xmm2
    punpckhbw xmm1, xmm2
    movdqa    [eax * 2 + edx], xmm0  
    movdqa    [eax * 2 + edx + 16], xmm1  
    lea       eax, [eax + 16]
    sub       ecx, 8
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRGB24Row_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm6, kShuffleMaskARGBToRGB24

    align      16
 convertloop:
    movdqa    xmm0, [eax]   
    movdqa    xmm1, [eax + 16]
    movdqa    xmm2, [eax + 32]
    movdqa    xmm3, [eax + 48]
    lea       eax, [eax + 64]
    pshufb    xmm0, xmm6    
    pshufb    xmm1, xmm6
    pshufb    xmm2, xmm6
    pshufb    xmm3, xmm6
    movdqa    xmm4, xmm1   
    psrldq    xmm1, 4      
    pslldq    xmm4, 12     
    movdqa    xmm5, xmm2   
    por       xmm0, xmm4   
    pslldq    xmm5, 8      
    movdqa    [edx], xmm0  
    por       xmm1, xmm5   
    psrldq    xmm2, 8      
    pslldq    xmm3, 4      
    por       xmm2, xmm3   
    movdqa    [edx + 16], xmm1   
    movdqa    [edx + 32], xmm2   
    lea       edx, [edx + 48]
    sub       ecx, 16
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRAWRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm6, kShuffleMaskARGBToRAW

    align      16
 convertloop:
    movdqa    xmm0, [eax]   
    movdqa    xmm1, [eax + 16]
    movdqa    xmm2, [eax + 32]
    movdqa    xmm3, [eax + 48]
    lea       eax, [eax + 64]
    pshufb    xmm0, xmm6    
    pshufb    xmm1, xmm6
    pshufb    xmm2, xmm6
    pshufb    xmm3, xmm6
    movdqa    xmm4, xmm1   
    psrldq    xmm1, 4      
    pslldq    xmm4, 12     
    movdqa    xmm5, xmm2   
    por       xmm0, xmm4   
    pslldq    xmm5, 8      
    movdqa    [edx], xmm0  
    por       xmm1, xmm5   
    psrldq    xmm2, 8      
    pslldq    xmm3, 4      
    por       xmm2, xmm3   
    movdqa    [edx + 16], xmm1   
    movdqa    [edx + 32], xmm2   
    lea       edx, [edx + 48]
    sub       ecx, 16
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToRGB565Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm3, xmm3       
    psrld     xmm3, 27
    pcmpeqb   xmm4, xmm4       
    psrld     xmm4, 26
    pslld     xmm4, 5
    pcmpeqb   xmm5, xmm5       
    pslld     xmm5, 11

    align      16
 convertloop:
    movdqa    xmm0, [eax]   
    movdqa    xmm1, xmm0    
    movdqa    xmm2, xmm0    
    pslld     xmm0, 8       
    psrld     xmm1, 3       
    psrld     xmm2, 5       
    psrad     xmm0, 16      
    pand      xmm1, xmm3    
    pand      xmm2, xmm4    
    pand      xmm0, xmm5    
    por       xmm1, xmm2    
    por       xmm0, xmm1    
    packssdw  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  
    lea       edx, [edx + 8]
    sub       ecx, 4
    jg        convertloop
    ret
  }
}


__declspec(naked) __declspec(align(16))
void ARGBToARGB1555Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm4, xmm4       
    psrld     xmm4, 27
    movdqa    xmm5, xmm4       
    pslld     xmm5, 5
    movdqa    xmm6, xmm4       
    pslld     xmm6, 10
    pcmpeqb   xmm7, xmm7       
    pslld     xmm7, 15

    align      16
 convertloop:
    movdqa    xmm0, [eax]   
    movdqa    xmm1, xmm0    
    movdqa    xmm2, xmm0    
    movdqa    xmm3, xmm0    
    psrad     xmm0, 16      
    psrld     xmm1, 3       
    psrld     xmm2, 6       
    psrld     xmm3, 9       
    pand      xmm0, xmm7    
    pand      xmm1, xmm4    
    pand      xmm2, xmm5    
    pand      xmm3, xmm6    
    por       xmm0, xmm1    
    por       xmm2, xmm3    
    por       xmm0, xmm2    
    packssdw  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  
    lea       edx, [edx + 8]
    sub       ecx, 4
    jg        convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToARGB4444Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm4, xmm4       
    psllw     xmm4, 12
    movdqa    xmm3, xmm4       
    psrlw     xmm3, 8

    align      16
 convertloop:
    movdqa    xmm0, [eax]   
    movdqa    xmm1, xmm0
    pand      xmm0, xmm3    
    pand      xmm1, xmm4    
    psrl      xmm0, 4
    psrl      xmm1, 8
    por       xmm0, xmm1
    packuswb  xmm0, xmm0
    lea       eax, [eax + 16]
    movq      qword ptr [edx], xmm0  
    lea       edx, [edx + 8]
    sub       ecx, 4
    jg        convertloop
    ret
  }
}


__declspec(naked) __declspec(align(16))
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      16
 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      16
 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      16
 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      16
 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      16
 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      16
 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGBAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kRGBAToY

    align      16
 convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGBAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
__asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kRGBAToY

    align      16
 convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm2, xmm4
    pmaddubsw  xmm3, xmm4
    lea        eax, [eax + 64]
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ARGBToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kBGRAToU
    movdqa     xmm6, kBGRAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void BGRAToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kBGRAToU
    movdqa     xmm6, kBGRAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kABGRToU
    movdqa     xmm6, kABGRToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void ABGRToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kABGRToU
    movdqa     xmm6, kABGRToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGBAToUVRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                       uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kRGBAToU
    movdqa     xmm6, kRGBAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pavgb      xmm0, [eax + esi]
    pavgb      xmm1, [eax + esi + 16]
    pavgb      xmm2, [eax + esi + 32]
    pavgb      xmm3, [eax + esi + 48]
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void RGBAToUVRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
__asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kRGBAToU
    movdqa     xmm6, kRGBAToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      16
 convertloop:
    
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    movdqu     xmm4, [eax + esi]
    pavgb      xmm0, xmm4
    movdqu     xmm4, [eax + esi + 16]
    pavgb      xmm1, xmm4
    movdqu     xmm4, [eax + esi + 32]
    pavgb      xmm2, xmm4
    movdqu     xmm4, [eax + esi + 48]
    pavgb      xmm3, xmm4
    lea        eax,  [eax + 64]
    movdqa     xmm4, xmm0
    shufps     xmm0, xmm1, 0x88
    shufps     xmm4, xmm1, 0xdd
    pavgb      xmm0, xmm4
    movdqa     xmm4, xmm2
    shufps     xmm2, xmm3, 0x88
    shufps     xmm4, xmm3, 0xdd
    pavgb      xmm2, xmm4

    
    
    
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    pmaddubsw  xmm0, xmm7  
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm1, xmm6  
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm2
    phaddw     xmm1, xmm3
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1
    paddb      xmm0, xmm5            

    
    sub        ecx, 16
    movlps     qword ptr [edx], xmm0 
    movhps     qword ptr [edx + edi], xmm0 
    lea        edx, [edx + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_I422TOARGBROW_SSSE3

#define YG 74 /* static_cast<int8>(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,static_cast<int8>(2.018 * 64)) */
#define UG -25 /* static_cast<int8>(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* static_cast<int8>(-0.813 * 64 - 0.5) */
#define VR 102 /* static_cast<int8>(1.596 * 64 + 0.5) */


#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

static const vec8 kUVToB = {
  UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB
};

static const vec8 kUVToR = {
  UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR
};

static const vec8 kUVToG = {
  UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG
};

static const vec8 kVUToB = {
  VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB,
};

static const vec8 kVUToR = {
  VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR,
};

static const vec8 kVUToG = {
  VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG,
};

static const vec16 kYToRgb = { YG, YG, YG, YG, YG, YG, YG, YG };
static const vec16 kYSub16 = { 16, 16, 16, 16, 16, 16, 16, 16 };
static const vec16 kUVBiasB = { BB, BB, BB, BB, BB, BB, BB, BB };
static const vec16 kUVBiasG = { BG, BG, BG, BG, BG, BG, BG, BG };
static const vec16 kUVBiasR = { BR, BR, BR, BR, BR, BR, BR, BR };





#define READYUV444 __asm {                                                     \
    __asm movq       xmm0, qword ptr [esi] /* U */                /* NOLINT */ \
    __asm movq       xmm1, qword ptr [esi + edi] /* V */          /* NOLINT */ \
    __asm lea        esi,  [esi + 8]                                           \
    __asm punpcklbw  xmm0, xmm1           /* UV */                             \
  }


#define READYUV422 __asm {                                                     \
    __asm movd       xmm0, [esi]          /* U */                              \
    __asm movd       xmm1, [esi + edi]    /* V */                              \
    __asm lea        esi,  [esi + 4]                                           \
    __asm punpcklbw  xmm0, xmm1           /* UV */                             \
    __asm punpcklwd  xmm0, xmm0           /* UVUV (upsample) */                \
  }


#define READYUV411 __asm {                                                     \
    __asm movd       xmm0, [esi]          /* U */                              \
    __asm movd       xmm1, [esi + edi]    /* V */                              \
    __asm lea        esi,  [esi + 2]                                           \
    __asm punpcklbw  xmm0, xmm1           /* UV */                             \
    __asm punpcklwd  xmm0, xmm0           /* UVUV (upsample) */                \
    __asm punpckldq  xmm0, xmm0           /* UVUV (upsample) */                \
  }


#define READNV12 __asm {                                                       \
    __asm movq       xmm0, qword ptr [esi] /* UV */               /* NOLINT */ \
    __asm lea        esi,  [esi + 8]                                           \
    __asm punpcklwd  xmm0, xmm0           /* UVUV (upsample) */                \
  }


#define YUVTORGB __asm {                                                       \
    /* Step 1: Find 4 UV contributions to 8 R,G,B values */                    \
    __asm movdqa     xmm1, xmm0                                                \
    __asm movdqa     xmm2, xmm0                                                \
    __asm pmaddubsw  xmm0, kUVToB        /* scale B UV */                      \
    __asm pmaddubsw  xmm1, kUVToG        /* scale G UV */                      \
    __asm pmaddubsw  xmm2, kUVToR        /* scale R UV */                      \
    __asm psubw      xmm0, kUVBiasB      /* unbias back to signed */           \
    __asm psubw      xmm1, kUVBiasG                                            \
    __asm psubw      xmm2, kUVBiasR                                            \
    /* Step 2: Find Y contribution to 8 R,G,B values */                        \
    __asm movq       xmm3, qword ptr [eax]                        /* NOLINT */ \
    __asm lea        eax, [eax + 8]                                            \
    __asm punpcklbw  xmm3, xmm4                                                \
    __asm psubsw     xmm3, kYSub16                                             \
    __asm pmullw     xmm3, kYToRgb                                             \
    __asm paddsw     xmm0, xmm3           /* B += Y */                         \
    __asm paddsw     xmm1, xmm3           /* G += Y */                         \
    __asm paddsw     xmm2, xmm3           /* R += Y */                         \
    __asm psraw      xmm0, 6                                                   \
    __asm psraw      xmm1, 6                                                   \
    __asm psraw      xmm2, 6                                                   \
    __asm packuswb   xmm0, xmm0           /* B */                              \
    __asm packuswb   xmm1, xmm1           /* G */                              \
    __asm packuswb   xmm2, xmm2           /* R */                              \
  }


#define YVUTORGB __asm {                                                       \
    /* Step 1: Find 4 UV contributions to 8 R,G,B values */                    \
    __asm movdqa     xmm1, xmm0                                                \
    __asm movdqa     xmm2, xmm0                                                \
    __asm pmaddubsw  xmm0, kVUToB        /* scale B UV */                      \
    __asm pmaddubsw  xmm1, kVUToG        /* scale G UV */                      \
    __asm pmaddubsw  xmm2, kVUToR        /* scale R UV */                      \
    __asm psubw      xmm0, kUVBiasB      /* unbias back to signed */           \
    __asm psubw      xmm1, kUVBiasG                                            \
    __asm psubw      xmm2, kUVBiasR                                            \
    /* Step 2: Find Y contribution to 8 R,G,B values */                        \
    __asm movq       xmm3, qword ptr [eax]                        /* NOLINT */ \
    __asm lea        eax, [eax + 8]                                            \
    __asm punpcklbw  xmm3, xmm4                                                \
    __asm psubsw     xmm3, kYSub16                                             \
    __asm pmullw     xmm3, kYToRgb                                             \
    __asm paddsw     xmm0, xmm3           /* B += Y */                         \
    __asm paddsw     xmm1, xmm3           /* G += Y */                         \
    __asm paddsw     xmm2, xmm3           /* R += Y */                         \
    __asm psraw      xmm0, 6                                                   \
    __asm psraw      xmm1, 6                                                   \
    __asm psraw      xmm2, 6                                                   \
    __asm packuswb   xmm0, xmm0           /* B */                              \
    __asm packuswb   xmm1, xmm1           /* G */                              \
    __asm packuswb   xmm2, xmm2           /* R */                              \
  }



__declspec(naked) __declspec(align(16))
void I444ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* argb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV444
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void I422ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* argb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}




__declspec(naked) __declspec(align(16))
void I411ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* argb_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV411
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void NV12ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* uv_buf,
                         uint8* argb_buf,
                         int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READNV12
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void NV21ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* uv_buf,
                         uint8* argb_buf,
                         int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READNV12
    YVUTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void I444ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* argb_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV444
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void I422ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* argb_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}




__declspec(naked) __declspec(align(16))
void I411ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* argb_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV411
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}




__declspec(naked) __declspec(align(16))
void NV12ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* uv_buf,
                                   uint8* argb_buf,
                                   int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READNV12
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    ret
  }
}



__declspec(naked) __declspec(align(16))
void NV21ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* uv_buf,
                                   uint8* argb_buf,
                                   int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READNV12
    YVUTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm5           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToBGRARow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* bgra_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    pcmpeqb    xmm5, xmm5           
    punpcklbw  xmm1, xmm0           
    punpcklbw  xmm5, xmm2           
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           
    punpckhwd  xmm0, xmm1           
    movdqa     [edx], xmm5
    movdqa     [edx + 16], xmm0
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* bgra_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    pcmpeqb    xmm5, xmm5           
    punpcklbw  xmm1, xmm0           
    punpcklbw  xmm5, xmm2           
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           
    punpckhwd  xmm0, xmm1           
    movdqu     [edx], xmm5
    movdqu     [edx + 16], xmm0
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToABGRRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* abgr_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm2, xmm1           
    punpcklbw  xmm0, xmm5           
    movdqa     xmm1, xmm2
    punpcklwd  xmm2, xmm0           
    punpckhwd  xmm1, xmm0           
    movdqa     [edx], xmm2
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* abgr_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm2, xmm1           
    punpcklbw  xmm0, xmm5           
    movdqa     xmm1, xmm2
    punpcklwd  xmm2, xmm0           
    punpckhwd  xmm1, xmm0           
    movdqu     [edx], xmm2
    movdqu     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToRGBARow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgba_buf,
                         int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    pcmpeqb    xmm5, xmm5           
    punpcklbw  xmm1, xmm2           
    punpcklbw  xmm5, xmm0           
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           
    punpckhwd  xmm0, xmm1           
    movdqa     [edx], xmm5
    movdqa     [edx + 16], xmm0
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void I422ToRGBARow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* rgba_buf,
                                   int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        edi, esi
    pxor       xmm4, xmm4

    align      16
 convertloop:
    READYUV422
    YUVTORGB

    
    pcmpeqb    xmm5, xmm5           
    punpcklbw  xmm1, xmm2           
    punpcklbw  xmm5, xmm0           
    movdqa     xmm0, xmm5
    punpcklwd  xmm5, xmm1           
    punpckhwd  xmm0, xmm1           
    movdqu     [edx], xmm5
    movdqu     [edx + 16], xmm0
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

#endif  

#ifdef HAS_YTOARGBROW_SSE2
__declspec(naked) __declspec(align(16))
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) {
  __asm {
    pcmpeqb    xmm4, xmm4           
    pslld      xmm4, 24
    mov        eax,0x10001000
    movd       xmm3,eax
    pshufd     xmm3,xmm3,0
    mov        eax,0x012a012a
    movd       xmm2,eax
    pshufd     xmm2,xmm2,0
    mov        eax, [esp + 4]       
    mov        edx, [esp + 8]       
    mov        ecx, [esp + 12]      

    align      16
 convertloop:
    
    movq       xmm0, qword ptr [eax]
    lea        eax, [eax + 8]
    punpcklbw  xmm0, xmm0           
    psubusw    xmm0, xmm3
    pmulhuw    xmm0, xmm2
    packuswb   xmm0, xmm0           

    
    punpcklbw  xmm0, xmm0           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0           
    punpckhwd  xmm1, xmm1           
    por        xmm0, xmm4
    por        xmm1, xmm4
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx,  [edx + 32]
    sub        ecx, 8
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_MIRRORROW_SSSE3


static const uvec8 kShuffleMirror = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

__declspec(naked) __declspec(align(16))
void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMirror
    lea       eax, [eax - 16]

    align      16
 convertloop:
    movdqa    xmm0, [eax + ecx]
    pshufb    xmm0, xmm5
    sub       ecx, 16
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif  

#ifdef HAS_MIRRORROW_SSE2


__declspec(naked) __declspec(align(16))
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    lea       eax, [eax - 16]

    align      16
 convertloop:
    movdqu    xmm0, [eax + ecx]
    movdqa    xmm1, xmm0        
    psllw     xmm0, 8
    psrlw     xmm1, 8
    por       xmm0, xmm1
    pshuflw   xmm0, xmm0, 0x1b  
    pshufhw   xmm0, xmm0, 0x1b
    pshufd    xmm0, xmm0, 0x4e  
    sub       ecx, 16
    movdqu    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif  

#ifdef HAS_MIRRORROW_UV_SSSE3

static const uvec8 kShuffleMirrorUV = {
  14u, 12u, 10u, 8u, 6u, 4u, 2u, 0u, 15u, 13u, 11u, 9u, 7u, 5u, 3u, 1u
};

__declspec(naked) __declspec(align(16))
void MirrorRowUV_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
                       int width) {
  __asm {
    push      edi
    mov       eax, [esp + 4 + 4]   
    mov       edx, [esp + 4 + 8]   
    mov       edi, [esp + 4 + 12]  
    mov       ecx, [esp + 4 + 16]  
    movdqa    xmm1, kShuffleMirrorUV
    lea       eax, [eax + ecx * 2 - 16]
    sub       edi, edx

    align      16
 convertloop:
    movdqa    xmm0, [eax]
    lea       eax, [eax - 16]
    pshufb    xmm0, xmm1
    sub       ecx, 8
    movlpd    qword ptr [edx], xmm0
    movhpd    qword ptr [edx + edi], xmm0
    lea       edx, [edx + 8]
    jg        convertloop

    pop       edi
    ret
  }
}
#endif  

#ifdef HAS_ARGBMIRRORROW_SSSE3


static const uvec8 kARGBShuffleMirror = {
  12u, 13u, 14u, 15u, 8u, 9u, 10u, 11u, 4u, 5u, 6u, 7u, 0u, 1u, 2u, 3u
};

__declspec(naked) __declspec(align(16))
void ARGBMirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
__asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kARGBShuffleMirror
    lea       eax, [eax - 16]

    align      16
 convertloop:
    movdqa    xmm0, [eax + ecx * 4]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif  

#ifdef HAS_SPLITUV_SSE2
__declspec(naked) __declspec(align(16))
void SplitUV_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    psrlw      xmm2, 8      
    psrlw      xmm3, 8
    packuswb   xmm2, xmm3
    movdqa     [edx], xmm0
    movdqa     [edx + edi], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}
#endif  

#ifdef HAS_COPYROW_SSE2

__declspec(naked) __declspec(align(16))
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     [eax + edx], xmm0
    movdqa     [eax + edx + 16], xmm1
    lea        eax, [eax + 32]
    sub        ecx, 32
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_COPYROW_X86
__declspec(naked) __declspec(align(16))
void CopyRow_X86(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, esi
    mov        edx, edi
    mov        esi, [esp + 4]   
    mov        edi, [esp + 8]   
    mov        ecx, [esp + 12]  
    shr        ecx, 2
    rep movsd
    mov        edi, edx
    mov        esi, eax
    ret
  }
}
#endif  

#ifdef HAS_YUY2TOYROW_SSE2
__declspec(naked) __declspec(align(16))
void YUY2ToYRow_SSE2(const uint8* src_yuy2,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    pcmpeqb    xmm5, xmm5        
    psrlw      xmm5, 8

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToUVRow_SSE2(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    psrlw      xmm0, 8      
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToUV422Row_SSE2(const uint8* src_yuy2,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8      
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    pcmpeqb    xmm5, xmm5        
    psrlw      xmm5, 8

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToUVRow_Unaligned_SSE2(const uint8* src_yuy2, int stride_yuy2,
                                uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    psrlw      xmm0, 8      
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void YUY2ToUV422Row_Unaligned_SSE2(const uint8* src_yuy2,
                                   uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8      
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToYRow_SSE2(const uint8* src_uyvy,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToUVRow_SSE2(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToUV422Row_SSE2(const uint8* src_uyvy,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8    
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToUVRow_Unaligned_SSE2(const uint8* src_uyvy, int stride_uyvy,
                                uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked) __declspec(align(16))
void UYVYToUV422Row_Unaligned_SSE2(const uint8* src_uyvy,
                                   uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      16
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    movdqa     xmm1, xmm0
    pand       xmm0, xmm5  
    packuswb   xmm0, xmm0
    psrlw      xmm1, 8     
    packuswb   xmm1, xmm1
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + edi], xmm1
    lea        edx, [edx + 8]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}
#endif  

#ifdef HAS_ARGBBLENDROW_SSE2

__declspec(naked) __declspec(align(16))
void ARGBBlendRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                       uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm7, xmm7       
    psrlw      xmm7, 15
    pcmpeqb    xmm6, xmm6       
    psrlw      xmm6, 8
    pcmpeqb    xmm5, xmm5       
    psllw      xmm5, 8
    pcmpeqb    xmm4, xmm4       
    pslld      xmm4, 24

    sub        ecx, 1
    je         convertloop1     
    jl         convertloop1b

    
  alignloop1:
    test       edx, 15          
    je         alignloop1b
    movd       xmm3, [eax]
    lea        eax, [eax + 4]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movd       xmm2, [esi]      
    psrlw      xmm3, 8          
    pshufhw    xmm3, xmm3,0F5h  
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movd       xmm1, [esi]      
    lea        esi, [esi + 4]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        alignloop1

  alignloop1b:
    add        ecx, 1 - 4
    jl         convertloop4b

    
  convertloop4:
    movdqu     xmm3, [eax]      
    lea        eax, [eax + 16]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movdqu     xmm2, [esi]      
    psrlw      xmm3, 8          
    pshufhw    xmm3, xmm3,0F5h  
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movdqu     xmm1, [esi]      
    lea        esi, [esi + 16]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jge        convertloop4

  convertloop4b:
    add        ecx, 4 - 1
    jl         convertloop1b

    
  convertloop1:
    movd       xmm3, [eax]      
    lea        eax, [eax + 4]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movd       xmm2, [esi]      
    psrlw      xmm3, 8          
    pshufhw    xmm3, xmm3,0F5h  
    pshuflw    xmm3, xmm3,0F5h
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movd       xmm1, [esi]      
    lea        esi, [esi + 4]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        convertloop1

  convertloop1b:
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBBLENDROW_SSSE3

static const uvec8 kShuffleAlpha = {
  3u, 0x80, 3u, 0x80, 7u, 0x80, 7u, 0x80,
  11u, 0x80, 11u, 0x80, 15u, 0x80, 15u, 0x80
};








__declspec(naked) __declspec(align(16))
void ARGBBlendRow_SSSE3(const uint8* src_argb0, const uint8* src_argb1,
                        uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm7, xmm7       
    psrlw      xmm7, 15
    pcmpeqb    xmm6, xmm6       
    psrlw      xmm6, 8
    pcmpeqb    xmm5, xmm5       
    psllw      xmm5, 8
    pcmpeqb    xmm4, xmm4       
    pslld      xmm4, 24

    sub        ecx, 1
    je         convertloop1     
    jl         convertloop1b

    
  alignloop1:
    test       edx, 15          
    je         alignloop1b
    movd       xmm3, [eax]
    lea        eax, [eax + 4]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movd       xmm2, [esi]      
    pshufb     xmm3, kShuffleAlpha 
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movd       xmm1, [esi]      
    lea        esi, [esi + 4]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        alignloop1

  alignloop1b:
    add        ecx, 1 - 4
    jl         convertloop4b

    test       eax, 15          
    jne        convertuloop4
    test       esi, 15          
    jne        convertuloop4

    
  convertloop4:
    movdqa     xmm3, [eax]      
    lea        eax, [eax + 16]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movdqa     xmm2, [esi]      
    pshufb     xmm3, kShuffleAlpha 
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movdqa     xmm1, [esi]      
    lea        esi, [esi + 16]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jge        convertloop4
    jmp        convertloop4b

    
  convertuloop4:
    movdqu     xmm3, [eax]      
    lea        eax, [eax + 16]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movdqu     xmm2, [esi]      
    pshufb     xmm3, kShuffleAlpha 
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movdqu     xmm1, [esi]      
    lea        esi, [esi + 16]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jge        convertuloop4

  convertloop4b:
    add        ecx, 4 - 1
    jl         convertloop1b

    
  convertloop1:
    movd       xmm3, [eax]      
    lea        eax, [eax + 4]
    movdqa     xmm0, xmm3       
    pxor       xmm3, xmm4       
    movd       xmm2, [esi]      
    pshufb     xmm3, kShuffleAlpha 
    pand       xmm2, xmm6       
    paddw      xmm3, xmm7       
    pmullw     xmm2, xmm3       
    movd       xmm1, [esi]      
    lea        esi, [esi + 4]
    psrlw      xmm1, 8          
    por        xmm0, xmm4       
    pmullw     xmm1, xmm3       
    psrlw      xmm2, 8          
    paddusb    xmm0, xmm2       
    pand       xmm1, xmm5       
    paddusb    xmm0, xmm1       
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        convertloop1

  convertloop1b:
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBATTENUATE_SSE2


__declspec(naked) __declspec(align(16))
void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax
    pcmpeqb    xmm4, xmm4       
    pslld      xmm4, 24
    pcmpeqb    xmm5, xmm5       
    psrld      xmm5, 8

    align      16
 convertloop:
    movdqa     xmm0, [eax]      
    punpcklbw  xmm0, xmm0       
    pshufhw    xmm2, xmm0,0FFh  
    pshuflw    xmm2, xmm2,0FFh
    pmulhuw    xmm0, xmm2       
    movdqa     xmm1, [eax]      
    punpckhbw  xmm1, xmm1       
    pshufhw    xmm2, xmm1,0FFh  
    pshuflw    xmm2, xmm2,0FFh
    pmulhuw    xmm1, xmm2       
    movdqa     xmm2, [eax]      
    psrlw      xmm0, 8
    pand       xmm2, xmm4
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    pand       xmm0, xmm5       
    por        xmm0, xmm2
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBATTENUATEROW_SSSE3

static const uvec8 kShuffleAlpha0 = {
  3u, 3u, 3u, 3u, 3u, 3u, 128u, 128u, 7u, 7u, 7u, 7u, 7u, 7u, 128u, 128u,
};
static const uvec8 kShuffleAlpha1 = {
  11u, 11u, 11u, 11u, 11u, 11u, 128u, 128u,
  15u, 15u, 15u, 15u, 15u, 15u, 128u, 128u,
};
__declspec(naked) __declspec(align(16))
void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax
    pcmpeqb    xmm3, xmm3       
    pslld      xmm3, 24
    movdqa     xmm4, kShuffleAlpha0
    movdqa     xmm5, kShuffleAlpha1

    align      16
 convertloop:
    movdqa     xmm0, [eax]      
    pshufb     xmm0, xmm4       
    movdqa     xmm1, [eax]      
    punpcklbw  xmm1, xmm1       
    pmulhuw    xmm0, xmm1       
    movdqa     xmm1, [eax]      
    pshufb     xmm1, xmm5       
    movdqa     xmm2, [eax]      
    punpckhbw  xmm2, xmm2       
    pmulhuw    xmm1, xmm2       
    movdqa     xmm2, [eax]      
    pand       xmm2, xmm3
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    por        xmm0, xmm2       
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBUNATTENUATEROW_SSE2


__declspec(naked) __declspec(align(16))
void ARGBUnattenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        edx, [esp + 8 + 8]   
    mov        ecx, [esp + 8 + 12]  
    sub        edx, eax
    pcmpeqb    xmm4, xmm4       
    pslld      xmm4, 24

    align      16
 convertloop:
    movdqa     xmm0, [eax]      
    movzx      esi, byte ptr [eax + 3]  
    movzx      edi, byte ptr [eax + 7]  
    punpcklbw  xmm0, xmm0       
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2,0C0h  
    pshuflw    xmm3, xmm3,0C0h  
    movlhps    xmm2, xmm3
    pmulhuw    xmm0, xmm2       

    movdqa     xmm1, [eax]      
    movzx      esi, byte ptr [eax + 11]  
    movzx      edi, byte ptr [eax + 15]  
    punpckhbw  xmm1, xmm1       
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2,0C0h  
    pshuflw    xmm3, xmm3,0C0h  
    movlhps    xmm2, xmm3
    pmulhuw    xmm1, xmm2       

    movdqa     xmm2, [eax]      
    pand       xmm2, xmm4
    packuswb   xmm0, xmm1
    por        xmm0, xmm2
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop
    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBGRAYROW_SSSE3

static const vec8 kARGBToGray = {
  14, 76, 38, 0, 14, 76, 38, 0, 14, 76, 38, 0, 14, 76, 38, 0
};


__declspec(naked) __declspec(align(16))
void ARGBGrayRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm4, kARGBToGray
    sub        edx, eax

    align      16
 convertloop:
    movdqa     xmm0, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    phaddw     xmm0, xmm1
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0   
    movdqa     xmm2, [eax]  
    movdqa     xmm3, [eax + 16]
    psrld      xmm2, 24
    psrld      xmm3, 24
    packuswb   xmm2, xmm3
    packuswb   xmm2, xmm2   
    movdqa     xmm3, xmm0   
    punpcklbw  xmm0, xmm0   
    punpcklbw  xmm3, xmm2   
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm3   
    punpckhwd  xmm1, xmm3   
    sub        ecx, 8
    movdqa     [eax + edx], xmm0
    movdqa     [eax + edx + 16], xmm1
    lea        eax, [eax + 32]
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_ARGBSEPIAROW_SSSE3




static const vec8 kARGBToSepiaB = {
  17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0, 17, 68, 35, 0
};

static const vec8 kARGBToSepiaG = {
  22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0, 22, 88, 45, 0
};

static const vec8 kARGBToSepiaR = {
  24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0, 24, 98, 50, 0
};


__declspec(naked) __declspec(align(16))
void ARGBSepiaRow_SSSE3(uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        ecx, [esp + 8]   
    movdqa     xmm2, kARGBToSepiaB
    movdqa     xmm3, kARGBToSepiaG
    movdqa     xmm4, kARGBToSepiaR

    align      16
 convertloop:
    movdqa     xmm0, [eax]  
    movdqa     xmm6, [eax + 16]
    pmaddubsw  xmm0, xmm2
    pmaddubsw  xmm6, xmm2
    phaddw     xmm0, xmm6
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0   
    movdqa     xmm5, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm5, xmm3
    pmaddubsw  xmm1, xmm3
    phaddw     xmm5, xmm1
    psrlw      xmm5, 7
    packuswb   xmm5, xmm5   
    punpcklbw  xmm0, xmm5   
    movdqa     xmm5, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm5, xmm4
    pmaddubsw  xmm1, xmm4
    phaddw     xmm5, xmm1
    psrlw      xmm5, 7
    packuswb   xmm5, xmm5   
    movdqa     xmm6, [eax]  
    movdqa     xmm1, [eax + 16]
    psrld      xmm6, 24
    psrld      xmm1, 24
    packuswb   xmm6, xmm1
    packuswb   xmm6, xmm6   
    punpcklbw  xmm5, xmm6   
    movdqa     xmm1, xmm0   
    punpcklwd  xmm0, xmm5   
    punpckhwd  xmm1, xmm5   
    sub        ecx, 8
    movdqa     [eax], xmm0
    movdqa     [eax + 16], xmm1
    lea        eax, [eax + 32]
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_ARGBCOLORMATRIXROW_SSSE3




__declspec(naked) __declspec(align(16))
void ARGBColorMatrixRow_SSSE3(uint8* dst_argb, const int8* matrix_argb,
                              int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movd       xmm2, [edx]
    movd       xmm3, [edx + 4]
    movd       xmm4, [edx + 8]
    pshufd     xmm2, xmm2, 0
    pshufd     xmm3, xmm3, 0
    pshufd     xmm4, xmm4, 0

    align      16
 convertloop:
    movdqa     xmm0, [eax]  
    movdqa     xmm6, [eax + 16]
    pmaddubsw  xmm0, xmm2
    pmaddubsw  xmm6, xmm2
    movdqa     xmm5, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm5, xmm3
    pmaddubsw  xmm1, xmm3
    phaddsw    xmm0, xmm6   
    phaddsw    xmm5, xmm1   
    psraw      xmm0, 7      
    psraw      xmm5, 7      
    packuswb   xmm0, xmm0   
    packuswb   xmm5, xmm5   
    punpcklbw  xmm0, xmm5   
    movdqa     xmm5, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm5, xmm4
    pmaddubsw  xmm1, xmm4
    phaddsw    xmm5, xmm1
    psraw      xmm5, 7
    packuswb   xmm5, xmm5   
    movdqa     xmm6, [eax]  
    movdqa     xmm1, [eax + 16]
    psrld      xmm6, 24
    psrld      xmm1, 24
    packuswb   xmm6, xmm1
    packuswb   xmm6, xmm6   
    movdqa     xmm1, xmm0   
    punpcklbw  xmm5, xmm6   
    punpcklwd  xmm0, xmm5   
    punpckhwd  xmm1, xmm5   
    sub        ecx, 8
    movdqa     [eax], xmm0
    movdqa     [eax + 16], xmm1
    lea        eax, [eax + 32]
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_ARGBCOLORTABLEROW_X86

__declspec(naked) __declspec(align(16))
void ARGBColorTableRow_X86(uint8* dst_argb, const uint8* table_argb,
                           int width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    push       ebp
    mov        eax, [esp + 16 + 4]   
    mov        edi, [esp + 16 + 8]   
    mov        ecx, [esp + 16 + 12]  
    xor        ebx, ebx
    xor        edx, edx

    align      16
 convertloop:
    mov        ebp, dword ptr [eax]  
    mov        esi, ebp
    and        ebp, 255
    shr        esi, 8
    and        esi, 255
    mov        bl, [edi + ebp * 4 + 0]  
    mov        dl, [edi + esi * 4 + 1]  
    mov        ebp, dword ptr [eax]  
    mov        esi, ebp
    shr        ebp, 16
    shr        esi, 24
    and        ebp, 255
    mov        [eax], bl
    mov        [eax + 1], dl
    mov        bl, [edi + ebp * 4 + 2]  
    mov        dl, [edi + esi * 4 + 3]  
    mov        [eax + 2], bl
    mov        [eax + 3], dl
    lea        eax, [eax + 4]
    sub        ecx, 1
    jg         convertloop
    pop        ebp
    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}
#endif  

#ifdef HAS_ARGBQUANTIZEROW_SSE2


__declspec(naked) __declspec(align(16))
void ARGBQuantizeRow_SSE2(uint8* dst_argb, int scale, int interval_size,
                          int interval_offset, int width) {
  __asm {
    mov        eax, [esp + 4]    
    movd       xmm2, [esp + 8]   
    movd       xmm3, [esp + 12]  
    movd       xmm4, [esp + 16]  
    mov        ecx, [esp + 20]   
    pshuflw    xmm2, xmm2, 040h
    pshufd     xmm2, xmm2, 044h
    pshuflw    xmm3, xmm3, 040h
    pshufd     xmm3, xmm3, 044h
    pshuflw    xmm4, xmm4, 040h
    pshufd     xmm4, xmm4, 044h
    pxor       xmm5, xmm5  
    pcmpeqb    xmm6, xmm6  
    pslld      xmm6, 24

    align      16
 convertloop:
    movdqa     xmm0, [eax]  
    punpcklbw  xmm0, xmm5   
    pmulhuw    xmm0, xmm2   
    movdqa     xmm1, [eax]  
    punpckhbw  xmm1, xmm5   
    pmulhuw    xmm1, xmm2
    pmullw     xmm0, xmm3   
    movdqa     xmm7, [eax]  
    pmullw     xmm1, xmm3
    pand       xmm7, xmm6   
    paddw      xmm0, xmm4   
    paddw      xmm1, xmm4
    packuswb   xmm0, xmm1
    por        xmm0, xmm7
    sub        ecx, 4
    movdqa     [eax], xmm0
    lea        eax, [eax + 16]
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_CUMULATIVESUMTOAVERAGE_SSE2













void CumulativeSumToAverage_SSE2(const int32* topleft, const int32* botleft,
                                 int width, int area, uint8* dst, int count) {
  __asm {
    mov        eax, topleft  
    mov        esi, botleft  
    mov        edx, width
    movd       xmm4, area
    mov        edi, dst
    mov        ecx, count
    cvtdq2ps   xmm4, xmm4
    rcpss      xmm4, xmm4  
    pshufd     xmm4, xmm4, 0
    sub        ecx, 4
    jl         l4b

    
    align      4
  l4:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]

    
    psubd      xmm0, [eax + edx * 4]
    psubd      xmm1, [eax + edx * 4 + 16]
    psubd      xmm2, [eax + edx * 4 + 32]
    psubd      xmm3, [eax + edx * 4 + 48]
    lea        eax, [eax + 64]

    
    psubd      xmm0, [esi]
    psubd      xmm1, [esi + 16]
    psubd      xmm2, [esi + 32]
    psubd      xmm3, [esi + 48]

    
    paddd      xmm0, [esi + edx * 4]
    paddd      xmm1, [esi + edx * 4 + 16]
    paddd      xmm2, [esi + edx * 4 + 32]
    paddd      xmm3, [esi + edx * 4 + 48]
    lea        esi, [esi + 64]

    cvtdq2ps   xmm0, xmm0   
    cvtdq2ps   xmm1, xmm1
    mulps      xmm0, xmm4
    mulps      xmm1, xmm4
    cvtdq2ps   xmm2, xmm2
    cvtdq2ps   xmm3, xmm3
    mulps      xmm2, xmm4
    mulps      xmm3, xmm4
    cvtps2dq   xmm0, xmm0
    cvtps2dq   xmm1, xmm1
    cvtps2dq   xmm2, xmm2
    cvtps2dq   xmm3, xmm3
    packssdw   xmm0, xmm1
    packssdw   xmm2, xmm3
    packuswb   xmm0, xmm2
    movdqu     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 4
    jge        l4

  l4b:
    add        ecx, 4 - 1
    jl         l1b

    
    align      4
  l1:
    movdqa     xmm0, [eax]
    psubd      xmm0, [eax + edx * 4]
    lea        eax, [eax + 16]
    psubd      xmm0, [esi]
    paddd      xmm0, [esi + edx * 4]
    lea        esi, [esi + 16]
    cvtdq2ps   xmm0, xmm0
    mulps      xmm0, xmm4
    cvtps2dq   xmm0, xmm0
    packssdw   xmm0, xmm0
    packuswb   xmm0, xmm0
    movd       dword ptr [edi], xmm0
    lea        edi, [edi + 4]
    sub        ecx, 1
    jge        l1
  l1b:
  }
}
#endif  

#ifdef HAS_COMPUTECUMULATIVESUMROW_SSE2


void ComputeCumulativeSumRow_SSE2(const uint8* row, int32* cumsum,
                                  const int32* previous_cumsum, int width) {
  __asm {
    mov        eax, row
    mov        edx, cumsum
    mov        esi, previous_cumsum
    mov        ecx, width
    sub        esi, edx
    pxor       xmm0, xmm0
    pxor       xmm1, xmm1

    sub        ecx, 4
    jl         l4b
    test       edx, 15
    jne        l4b

    
    align      4
  l4:
    movdqu     xmm2, [eax]  
    lea        eax, [eax + 16]
    movdqa     xmm4, xmm2

    punpcklbw  xmm2, xmm1
    movdqa     xmm3, xmm2
    punpcklwd  xmm2, xmm1
    punpckhwd  xmm3, xmm1

    punpckhbw  xmm4, xmm1
    movdqa     xmm5, xmm4
    punpcklwd  xmm4, xmm1
    punpckhwd  xmm5, xmm1

    paddd      xmm0, xmm2
    movdqa     xmm2, [edx + esi]  
    paddd      xmm2, xmm0

    paddd      xmm0, xmm3
    movdqa     xmm3, [edx + esi + 16]
    paddd      xmm3, xmm0

    paddd      xmm0, xmm4
    movdqa     xmm4, [edx + esi + 32]
    paddd      xmm4, xmm0

    paddd      xmm0, xmm5
    movdqa     xmm5, [edx + esi + 48]
    paddd      xmm5, xmm0

    movdqa     [edx], xmm2
    movdqa     [edx + 16], xmm3
    movdqa     [edx + 32], xmm4
    movdqa     [edx + 48], xmm5

    lea        edx, [edx + 64]
    sub        ecx, 4
    jge        l4

  l4b:
    add        ecx, 4 - 1
    jl         l1b

    
    align      4
  l1:
    movd       xmm2, dword ptr [eax]  
    lea        eax, [eax + 4]
    punpcklbw  xmm2, xmm1
    punpcklwd  xmm2, xmm1
    paddd      xmm0, xmm2
    movdqu     xmm2, [edx + esi]
    paddd      xmm2, xmm0
    movdqu     [edx], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 1
    jge        l1

 l1b:
  }
}
#endif  

#ifdef HAS_ARGBSHADE_SSE2


__declspec(naked) __declspec(align(16))
void ARGBShadeRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width,
                       uint32 value) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movd       xmm2, [esp + 16]  
    sub        edx, eax
    punpcklbw  xmm2, xmm2
    punpcklqdq xmm2, xmm2

    align      16
 convertloop:
    movdqa     xmm0, [eax]      
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm0       
    punpckhbw  xmm1, xmm1       
    pmulhuw    xmm0, xmm2       
    pmulhuw    xmm1, xmm2       
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqa     [eax + edx], xmm0
    lea        eax, [eax + 16]
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBAFFINEROW_SSE2

__declspec(naked) __declspec(align(16))
LIBYUV_API
void ARGBAffineRow_SSE2(const uint8* src_argb, int src_argb_stride,
                        uint8* dst_argb, const float* uv_dudv, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 12]   
    mov        esi, [esp + 16]  
    mov        edx, [esp + 20]  
    mov        ecx, [esp + 24]  
    movq       xmm2, qword ptr [ecx]  
    movq       xmm7, qword ptr [ecx + 8]  
    mov        ecx, [esp + 28]  
    shl        esi, 16          
    add        esi, 4
    movd       xmm5, esi
    sub        ecx, 4
    jl         l4b

    
    pshufd     xmm7, xmm7, 0x44  
    pshufd     xmm5, xmm5, 0  
    movdqa     xmm0, xmm2    
    addps      xmm0, xmm7
    movlhps    xmm2, xmm0
    movdqa     xmm4, xmm7
    addps      xmm4, xmm4    
    movdqa     xmm3, xmm2    
    addps      xmm3, xmm4
    addps      xmm4, xmm4    

    
    align      4
  l4:
    cvttps2dq  xmm0, xmm2    
    cvttps2dq  xmm1, xmm3    
    packssdw   xmm0, xmm1    
    pmaddwd    xmm0, xmm5    
    movd       esi, xmm0
    pshufd     xmm0, xmm0, 0x39  
    movd       edi, xmm0
    pshufd     xmm0, xmm0, 0x39  
    movd       xmm1, [eax + esi]  
    movd       xmm6, [eax + edi]  
    punpckldq  xmm1, xmm6     
    addps      xmm2, xmm4    
    movq       qword ptr [edx], xmm1
    movd       esi, xmm0
    pshufd     xmm0, xmm0, 0x39  
    movd       edi, xmm0
    movd       xmm6, [eax + esi]  
    movd       xmm0, [eax + edi]  
    punpckldq  xmm6, xmm0     
    addps      xmm3, xmm4    
    sub        ecx, 4
    movq       qword ptr 8[edx], xmm6
    lea        edx, [edx + 16]
    jge        l4

  l4b:
    add        ecx, 4 - 1
    jl         l1b

    
    align      4
  l1:
    cvttps2dq  xmm0, xmm2    
    packssdw   xmm0, xmm0    
    pmaddwd    xmm0, xmm5    
    addps      xmm2, xmm7    
    movd       esi, xmm0
    movd       xmm0, [eax + esi]  
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        l1
  l1b:
    pop        edi
    pop        esi
    ret
  }
}
#endif  


__declspec(naked) __declspec(align(16))
void ARGBInterpolateRow_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                              ptrdiff_t src_stride, int dst_width,
                              int source_y_fraction) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        ecx, [esp + 8 + 16]  
    mov        eax, [esp + 8 + 20]  
    sub        edi, esi
    shr        eax, 1
    cmp        eax, 0
    je         xloop1
    cmp        eax, 64
    je         xloop2
    movd       xmm0, eax  
    neg        eax
    add        eax, 128
    movd       xmm5, eax  
    punpcklbw  xmm5, xmm0
    punpcklwd  xmm5, xmm5
    pshufd     xmm5, xmm5, 0

    align      16
  xloop:
    movdqa     xmm0, [esi]
    movdqa     xmm2, [esi + edx]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm2
    punpckhbw  xmm1, xmm2
    pmaddubsw  xmm0, xmm5
    pmaddubsw  xmm1, xmm5
    psrlw      xmm0, 7
    psrlw      xmm1, 7
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop

    pop        edi
    pop        esi
    ret

    align      16
  xloop1:
    movdqa     xmm0, [esi]
    sub        ecx, 4
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop1

    pop        edi
    pop        esi
    ret

    align      16
  xloop2:
    movdqa     xmm0, [esi]
    pavgb      xmm0, [esi + edx]
    sub        ecx, 4
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop2

    pop        edi
    pop        esi
    ret
  }
}

#endif  

#ifdef __cplusplus
}  
}  
#endif
