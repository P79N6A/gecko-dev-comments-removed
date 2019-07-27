









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_X86) && defined(_M_IX86) && defined(_MSC_VER)

#ifdef HAS_ARGBTOYROW_SSSE3


static const vec8 kARGBToY = {
  13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0, 13, 65, 33, 0
};


static const vec8 kARGBToYJ = {
  15, 75, 38, 0, 15, 75, 38, 0, 15, 75, 38, 0, 15, 75, 38, 0
};

static const vec8 kARGBToU = {
  112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0, 112, -74, -38, 0
};

static const vec8 kARGBToUJ = {
  127, -84, -43, 0, 127, -84, -43, 0, 127, -84, -43, 0, 127, -84, -43, 0
};

static const vec8 kARGBToV = {
  -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0, -18, -94, 112, 0,
};

static const vec8 kARGBToVJ = {
  -20, -107, 127, 0, -20, -107, 127, 0, -20, -107, 127, 0, -20, -107, 127, 0
};


static const lvec32 kPermdARGBToY_AVX = {
  0, 4, 1, 5, 2, 6, 3, 7
};


static const lvec8 kShufARGBToUV_AVX = {
  0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15,
  0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15,
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

static const vec16 kAddYJ64 = {
  64, 64, 64, 64, 64, 64, 64, 64
};

static const uvec8 kAddUV128 = {
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
  128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u
};

static const uvec16 kAddUVJ128 = {
  0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u, 0x8080u
};


static const uvec8 kShuffleMaskRGB24ToARGB = {
  0u, 1u, 2u, 12u, 3u, 4u, 5u, 13u, 6u, 7u, 8u, 14u, 9u, 10u, 11u, 15u
};


static const uvec8 kShuffleMaskRAWToARGB = {
  2u, 1u, 0u, 12u, 5u, 4u, 3u, 13u, 8u, 7u, 6u, 14u, 11u, 10u, 9u, 15u
};


static const uvec8 kShuffleMaskARGBToRGB24 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 10u, 12u, 13u, 14u, 128u, 128u, 128u, 128u
};


static const uvec8 kShuffleMaskARGBToRAW = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 8u, 14u, 13u, 12u, 128u, 128u, 128u, 128u
};


static const uvec8 kShuffleMaskARGBToRGB24_0 = {
  0u, 1u, 2u, 4u, 5u, 6u, 8u, 9u, 128u, 128u, 128u, 128u, 10u, 12u, 13u, 14u
};


static const uvec8 kShuffleMaskARGBToRAW_0 = {
  2u, 1u, 0u, 6u, 5u, 4u, 10u, 9u, 128u, 128u, 128u, 128u, 8u, 14u, 13u, 12u
};


__declspec(naked)
void I400ToARGBRow_SSE2(const uint8* src_y, uint8* dst_argb, int pix) {
  __asm {
    mov        eax, [esp + 4]        
    mov        edx, [esp + 8]        
    mov        ecx, [esp + 12]       
    pcmpeqb    xmm5, xmm5            
    pslld      xmm5, 24

    align      4
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

__declspec(naked)
void I400ToARGBRow_Unaligned_SSE2(const uint8* src_y, uint8* dst_argb,
                                  int pix) {
  __asm {
    mov        eax, [esp + 4]        
    mov        edx, [esp + 8]        
    mov        ecx, [esp + 12]       
    pcmpeqb    xmm5, xmm5            
    pslld      xmm5, 24

    align      4
  convertloop:
    movq       xmm0, qword ptr [eax]
    lea        eax,  [eax + 8]
    punpcklbw  xmm0, xmm0
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm0
    punpckhwd  xmm1, xmm1
    por        xmm0, xmm5
    por        xmm1, xmm5
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop
    ret
  }
}

__declspec(naked)
void RGB24ToARGBRow_SSSE3(const uint8* src_rgb24, uint8* dst_argb, int pix) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm5, xmm5       
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRGB24ToARGB

    align      4
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

__declspec(naked)
void RAWToARGBRow_SSSE3(const uint8* src_raw, uint8* dst_argb,
                        int pix) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm5, xmm5       
    pslld     xmm5, 24
    movdqa    xmm4, kShuffleMaskRAWToARGB

    align      4
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








__declspec(naked)
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

    align      4
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


__declspec(naked)
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

    align      4
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


__declspec(naked)
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

    align      4
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

__declspec(naked)
void ARGBToRGB24Row_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm6, kShuffleMaskARGBToRGB24

    align      4
 convertloop:
    movdqu    xmm0, [eax]   
    movdqu    xmm1, [eax + 16]
    movdqu    xmm2, [eax + 32]
    movdqu    xmm3, [eax + 48]
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
    movdqu    [edx], xmm0  
    por       xmm1, xmm5   
    psrldq    xmm2, 8      
    pslldq    xmm3, 4      
    por       xmm2, xmm3   
    movdqu    [edx + 16], xmm1   
    movdqu    [edx + 32], xmm2   
    lea       edx, [edx + 48]
    sub       ecx, 16
    jg        convertloop
    ret
  }
}

__declspec(naked)
void ARGBToRAWRow_SSSE3(const uint8* src_argb, uint8* dst_rgb, int pix) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm6, kShuffleMaskARGBToRAW

    align      4
 convertloop:
    movdqu    xmm0, [eax]   
    movdqu    xmm1, [eax + 16]
    movdqu    xmm2, [eax + 32]
    movdqu    xmm3, [eax + 48]
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
    movdqu    [edx], xmm0  
    por       xmm1, xmm5   
    psrldq    xmm2, 8      
    pslldq    xmm3, 4      
    por       xmm2, xmm3   
    movdqu    [edx + 16], xmm1   
    movdqu    [edx + 32], xmm2   
    lea       edx, [edx + 48]
    sub       ecx, 16
    jg        convertloop
    ret
  }
}

__declspec(naked)
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

    align      4
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


__declspec(naked)
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

    align      4
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

__declspec(naked)
void ARGBToARGB4444Row_SSE2(const uint8* src_argb, uint8* dst_rgb, int pix) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    pcmpeqb   xmm4, xmm4       
    psllw     xmm4, 12
    movdqa    xmm3, xmm4       
    psrlw     xmm3, 8

    align      4
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


__declspec(naked)
void ARGBToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      4
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


__declspec(naked)
void ARGBToYJRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm4, kARGBToYJ
    movdqa     xmm5, kAddYJ64

    align      4
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
    paddw      xmm0, xmm5  
    paddw      xmm2, xmm5
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

#ifdef HAS_ARGBTOYROW_AVX2

__declspec(naked)
void ARGBToYRow_AVX2(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    vbroadcastf128 ymm4, kARGBToY
    vbroadcastf128 ymm5, kAddY16
    vmovdqa    ymm6, kPermdARGBToY_AVX

    align      4
 convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    vmovdqu    ymm2, [eax + 64]
    vmovdqu    ymm3, [eax + 96]
    vpmaddubsw ymm0, ymm0, ymm4
    vpmaddubsw ymm1, ymm1, ymm4
    vpmaddubsw ymm2, ymm2, ymm4
    vpmaddubsw ymm3, ymm3, ymm4
    lea        eax, [eax + 128]
    vphaddw    ymm0, ymm0, ymm1  
    vphaddw    ymm2, ymm2, ymm3
    vpsrlw     ymm0, ymm0, 7
    vpsrlw     ymm2, ymm2, 7
    vpackuswb  ymm0, ymm0, ymm2  
    vpermd     ymm0, ymm6, ymm0  
    vpaddb     ymm0, ymm0, ymm5
    sub        ecx, 32
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    jg         convertloop
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBTOYROW_AVX2

__declspec(naked)
void ARGBToYJRow_AVX2(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    vbroadcastf128 ymm4, kARGBToYJ
    vbroadcastf128 ymm5, kAddYJ64
    vmovdqa    ymm6, kPermdARGBToY_AVX

    align      4
 convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    vmovdqu    ymm2, [eax + 64]
    vmovdqu    ymm3, [eax + 96]
    vpmaddubsw ymm0, ymm0, ymm4
    vpmaddubsw ymm1, ymm1, ymm4
    vpmaddubsw ymm2, ymm2, ymm4
    vpmaddubsw ymm3, ymm3, ymm4
    lea        eax, [eax + 128]
    vphaddw    ymm0, ymm0, ymm1  
    vphaddw    ymm2, ymm2, ymm3
    vpaddw     ymm0, ymm0, ymm5  
    vpaddw     ymm2, ymm2, ymm5
    vpsrlw     ymm0, ymm0, 7
    vpsrlw     ymm2, ymm2, 7
    vpackuswb  ymm0, ymm0, ymm2  
    vpermd     ymm0, ymm6, ymm0  
    sub        ecx, 32
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    jg         convertloop

    vzeroupper
    ret
  }
}
#endif  

__declspec(naked)
void ARGBToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kARGBToY

    align      4
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

__declspec(naked)
void ARGBToYJRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm4, kARGBToYJ
    movdqa     xmm5, kAddYJ64

    align      4
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
    paddw      xmm0, xmm5
    paddw      xmm2, xmm5
    psrlw      xmm0, 7
    psrlw      xmm2, 7
    packuswb   xmm0, xmm2
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    ret
  }
}

__declspec(naked)
void BGRAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      4
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

__declspec(naked)
void BGRAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kBGRAToY

    align      4
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

__declspec(naked)
void ABGRToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      4
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

__declspec(naked)
void ABGRToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kABGRToY

    align      4
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

__declspec(naked)
void RGBAToYRow_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kRGBAToY

    align      4
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

__declspec(naked)
void RGBAToYRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm5, kAddY16
    movdqa     xmm4, kRGBAToY

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
void ARGBToUVJRow_SSSE3(const uint8* src_argb0, int src_stride_argb,
                        uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kARGBToUJ
    movdqa     xmm6, kARGBToVJ
    movdqa     xmm5, kAddUVJ128
    sub        edi, edx             

    align      4
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
    paddw      xmm0, xmm5            
    paddw      xmm1, xmm5
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1

    
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

#ifdef HAS_ARGBTOUVROW_AVX2
__declspec(naked)
void ARGBToUVRow_AVX2(const uint8* src_argb0, int src_stride_argb,
                      uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    vbroadcastf128 ymm5, kAddUV128
    vbroadcastf128 ymm6, kARGBToV
    vbroadcastf128 ymm7, kARGBToU
    sub        edi, edx             

    align      4
 convertloop:
    
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    vmovdqu    ymm2, [eax + 64]
    vmovdqu    ymm3, [eax + 96]
    vpavgb     ymm0, ymm0, [eax + esi]
    vpavgb     ymm1, ymm1, [eax + esi + 32]
    vpavgb     ymm2, ymm2, [eax + esi + 64]
    vpavgb     ymm3, ymm3, [eax + esi + 96]
    lea        eax,  [eax + 128]
    vshufps    ymm4, ymm0, ymm1, 0x88
    vshufps    ymm0, ymm0, ymm1, 0xdd
    vpavgb     ymm0, ymm0, ymm4  
    vshufps    ymm4, ymm2, ymm3, 0x88
    vshufps    ymm2, ymm2, ymm3, 0xdd
    vpavgb     ymm2, ymm2, ymm4  

    
    
    
    vpmaddubsw ymm1, ymm0, ymm7  
    vpmaddubsw ymm3, ymm2, ymm7
    vpmaddubsw ymm0, ymm0, ymm6  
    vpmaddubsw ymm2, ymm2, ymm6
    vphaddw    ymm1, ymm1, ymm3  
    vphaddw    ymm0, ymm0, ymm2
    vpsraw     ymm1, ymm1, 8
    vpsraw     ymm0, ymm0, 8
    vpacksswb  ymm0, ymm1, ymm0  
    vpermq     ymm0, ymm0, 0xd8  
    vpshufb    ymm0, ymm0, kShufARGBToUV_AVX  
    vpaddb     ymm0, ymm0, ymm5  

    
    sub         ecx, 32
    vextractf128 [edx], ymm0, 0 
    vextractf128 [edx + edi], ymm0, 1 
    lea        edx, [edx + 16]
    jg         convertloop

    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}
#endif  

__declspec(naked)
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

    align      4
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

__declspec(naked)
void ARGBToUVJRow_Unaligned_SSSE3(const uint8* src_argb0, int src_stride_argb,
                                 uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edx, [esp + 8 + 12]  
    mov        edi, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    movdqa     xmm7, kARGBToUJ
    movdqa     xmm6, kARGBToVJ
    movdqa     xmm5, kAddUVJ128
    sub        edi, edx             

    align      4
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
    paddw      xmm0, xmm5            
    paddw      xmm1, xmm5
    psraw      xmm0, 8
    psraw      xmm1, 8
    packsswb   xmm0, xmm1

    
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

__declspec(naked)
void ARGBToUV444Row_SSSE3(const uint8* src_argb0,
                          uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]   
    mov        edx, [esp + 4 + 8]   
    mov        edi, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      4
 convertloop:
    
    movdqa     xmm0, [eax]          
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm3, xmm7
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psraw      xmm0, 8
    psraw      xmm2, 8
    packsswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx,  16
    movdqa     [edx], xmm0

    movdqa     xmm0, [eax]          
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm6
    pmaddubsw  xmm1, xmm6
    pmaddubsw  xmm2, xmm6
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psraw      xmm0, 8
    psraw      xmm2, 8
    packsswb   xmm0, xmm2
    paddb      xmm0, xmm5
    lea        eax,  [eax + 64]
    movdqa     [edx + edi], xmm0
    lea        edx,  [edx + 16]
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked)
void ARGBToUV444Row_Unaligned_SSSE3(const uint8* src_argb0,
                                    uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]   
    mov        edx, [esp + 4 + 8]   
    mov        edi, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      4
 convertloop:
    
    movdqu     xmm0, [eax]          
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm7
    pmaddubsw  xmm1, xmm7
    pmaddubsw  xmm2, xmm7
    pmaddubsw  xmm3, xmm7
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psraw      xmm0, 8
    psraw      xmm2, 8
    packsswb   xmm0, xmm2
    paddb      xmm0, xmm5
    sub        ecx,  16
    movdqu     [edx], xmm0

    movdqu     xmm0, [eax]          
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
    pmaddubsw  xmm0, xmm6
    pmaddubsw  xmm1, xmm6
    pmaddubsw  xmm2, xmm6
    pmaddubsw  xmm3, xmm6
    phaddw     xmm0, xmm1
    phaddw     xmm2, xmm3
    psraw      xmm0, 8
    psraw      xmm2, 8
    packsswb   xmm0, xmm2
    paddb      xmm0, xmm5
    lea        eax,  [eax + 64]
    movdqu     [edx + edi], xmm0
    lea        edx,  [edx + 16]
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked)
void ARGBToUV422Row_SSSE3(const uint8* src_argb0,
                          uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]   
    mov        edx, [esp + 4 + 8]   
    mov        edi, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      4
 convertloop:
    
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + 32]
    movdqa     xmm3, [eax + 48]
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
    ret
  }
}

__declspec(naked)
void ARGBToUV422Row_Unaligned_SSSE3(const uint8* src_argb0,
                                    uint8* dst_u, uint8* dst_v, int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]   
    mov        edx, [esp + 4 + 8]   
    mov        edi, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    movdqa     xmm7, kARGBToU
    movdqa     xmm6, kARGBToV
    movdqa     xmm5, kAddUV128
    sub        edi, edx             

    align      4
 convertloop:
    
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + 32]
    movdqu     xmm3, [eax + 48]
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
    ret
  }
}

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

#define YG 74 /* (int8)(1.164 * 64 + 0.5) */

#define UB 127 /* min(63,(int8)(2.018 * 64)) */
#define UG -25 /* (int8)(-0.391 * 64 - 0.5) */
#define UR 0

#define VB 0
#define VG -52 /* (int8)(-0.813 * 64 - 0.5) */
#define VR 102 /* (int8)(1.596 * 64 + 0.5) */


#define BB UB * 128 + VB * 128
#define BG UG * 128 + VG * 128
#define BR UR * 128 + VR * 128

#ifdef HAS_I422TOARGBROW_AVX2

static const lvec8 kUVToB_AVX = {
  UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB,
  UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB, UB, VB
};
static const lvec8 kUVToR_AVX = {
  UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR,
  UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR, UR, VR
};
static const lvec8 kUVToG_AVX = {
  UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG,
  UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG, UG, VG
};
static const lvec16 kYToRgb_AVX = {
  YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG, YG
};
static const lvec16 kYSub16_AVX = {
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};
static const lvec16 kUVBiasB_AVX = {
  BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB, BB
};
static const lvec16 kUVBiasG_AVX = {
  BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG, BG
};
static const lvec16 kUVBiasR_AVX = {
  BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR, BR
};



__declspec(naked)
void I422ToARGBRow_AVX2(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_argb,
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
    vpcmpeqb   ymm5, ymm5, ymm5     
    vpxor      ymm4, ymm4, ymm4

    align      4
 convertloop:
    vmovq      xmm0, qword ptr [esi]          
    vmovq      xmm1, qword ptr [esi + edi]    
    lea        esi,  [esi + 8]
    vpunpcklbw ymm0, ymm0, ymm1               
    vpermq     ymm0, ymm0, 0xd8
    vpunpcklwd ymm0, ymm0, ymm0              
    vpmaddubsw ymm2, ymm0, kUVToB_AVX        
    vpmaddubsw ymm1, ymm0, kUVToG_AVX        
    vpmaddubsw ymm0, ymm0, kUVToR_AVX        
    vpsubw     ymm2, ymm2, kUVBiasB_AVX      
    vpsubw     ymm1, ymm1, kUVBiasG_AVX
    vpsubw     ymm0, ymm0, kUVBiasR_AVX

    
    vmovdqu    xmm3, [eax]                  
    lea        eax, [eax + 16]
    vpermq     ymm3, ymm3, 0xd8
    vpunpcklbw ymm3, ymm3, ymm4
    vpsubsw    ymm3, ymm3, kYSub16_AVX
    vpmullw    ymm3, ymm3, kYToRgb_AVX
    vpaddsw    ymm2, ymm2, ymm3           
    vpaddsw    ymm1, ymm1, ymm3           
    vpaddsw    ymm0, ymm0, ymm3           
    vpsraw     ymm2, ymm2, 6
    vpsraw     ymm1, ymm1, 6
    vpsraw     ymm0, ymm0, 6
    vpackuswb  ymm2, ymm2, ymm2           
    vpackuswb  ymm1, ymm1, ymm1           
    vpackuswb  ymm0, ymm0, ymm0           

    
    vpunpcklbw ymm2, ymm2, ymm1           
    vpermq     ymm2, ymm2, 0xd8
    vpunpcklbw ymm0, ymm0, ymm5           
    vpermq     ymm0, ymm0, 0xd8
    vpunpcklwd ymm1, ymm2, ymm0           
    vpunpckhwd ymm2, ymm2, ymm0           
    vmovdqu    [edx], ymm1
    vmovdqu    [edx + 32], ymm2
    lea        edx,  [edx + 64]
    sub        ecx, 16
    jg         convertloop
    vzeroupper

    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_I422TOARGBROW_SSSE3

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
    __asm movzx      ebx, word ptr [esi]        /* U */           /* NOLINT */ \
    __asm movd       xmm0, ebx                                                 \
    __asm movzx      ebx, word ptr [esi + edi]  /* V */           /* NOLINT */ \
    __asm movd       xmm1, ebx                                                 \
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



__declspec(naked)
void I444ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_argb,
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

    align      4
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



__declspec(naked)
void I422ToRGB24Row_SSSE3(const uint8* y_buf,
                          const uint8* u_buf,
                          const uint8* v_buf,
                          uint8* dst_rgb24,
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
    movdqa     xmm5, kShuffleMaskARGBToRGB24_0
    movdqa     xmm6, kShuffleMaskARGBToRGB24

    align      4
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm2           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    pshufb     xmm0, xmm5           
    pshufb     xmm1, xmm6           
    palignr    xmm1, xmm0, 12       
    movq       qword ptr [edx], xmm0  
    movdqu     [edx + 8], xmm1      
    lea        edx,  [edx + 24]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked)
void I422ToRAWRow_SSSE3(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* dst_raw,
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
    movdqa     xmm5, kShuffleMaskARGBToRAW_0
    movdqa     xmm6, kShuffleMaskARGBToRAW

    align      4
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm2           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           
    pshufb     xmm0, xmm5           
    pshufb     xmm1, xmm6           
    palignr    xmm1, xmm0, 12       
    movq       qword ptr [edx], xmm0  
    movdqu     [edx + 8], xmm1      
    lea        edx,  [edx + 24]
    sub        ecx, 8
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked)
void I422ToRGB565Row_SSSE3(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb565_buf,
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
    pcmpeqb    xmm5, xmm5       
    psrld      xmm5, 27
    pcmpeqb    xmm6, xmm6       
    psrld      xmm6, 26
    pslld      xmm6, 5
    pcmpeqb    xmm7, xmm7       
    pslld      xmm7, 11

    align      4
 convertloop:
    READYUV422
    YUVTORGB

    
    punpcklbw  xmm0, xmm1           
    punpcklbw  xmm2, xmm2           
    movdqa     xmm1, xmm0
    punpcklwd  xmm0, xmm2           
    punpckhwd  xmm1, xmm2           

    
    movdqa     xmm3, xmm0    
    movdqa     xmm2, xmm0    
    pslld      xmm0, 8       
    psrld      xmm3, 3       
    psrld      xmm2, 5       
    psrad      xmm0, 16      
    pand       xmm3, xmm5    
    pand       xmm2, xmm6    
    pand       xmm0, xmm7    
    por        xmm3, xmm2    
    por        xmm0, xmm3    
    movdqa     xmm3, xmm1    
    movdqa     xmm2, xmm1    
    pslld      xmm1, 8       
    psrld      xmm3, 3       
    psrld      xmm2, 5       
    psrad      xmm1, 16      
    pand       xmm3, xmm5    
    pand       xmm2, xmm6    
    pand       xmm1, xmm7    
    por        xmm3, xmm2    
    por        xmm1, xmm3    
    packssdw   xmm0, xmm1
    sub        ecx, 8
    movdqu     [edx], xmm0   
    lea        edx, [edx + 16]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked)
void I422ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_argb,
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

    align      4
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




__declspec(naked)
void I411ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_argb,
                         int width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        eax, [esp + 12 + 4]   
    mov        esi, [esp + 12 + 8]   
    mov        edi, [esp + 12 + 12]  
    mov        edx, [esp + 12 + 16]  
    mov        ecx, [esp + 12 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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
    pop        ebx
    ret
  }
}



__declspec(naked)
void NV12ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* uv_buf,
                         uint8* dst_argb,
                         int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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



__declspec(naked)
void NV21ToARGBRow_SSSE3(const uint8* y_buf,
                         const uint8* uv_buf,
                         uint8* dst_argb,
                         int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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



__declspec(naked)
void I444ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_argb,
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

    align      4
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



__declspec(naked)
void I422ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_argb,
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

    align      4
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




__declspec(naked)
void I411ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_argb,
                                   int width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        eax, [esp + 12 + 4]   
    mov        esi, [esp + 12 + 8]   
    mov        edi, [esp + 12 + 12]  
    mov        edx, [esp + 12 + 16]  
    mov        ecx, [esp + 12 + 20]  
    sub        edi, esi
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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
    pop        ebx
    ret
  }
}



__declspec(naked)
void NV12ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* uv_buf,
                                   uint8* dst_argb,
                                   int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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



__declspec(naked)
void NV21ToARGBRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* uv_buf,
                                   uint8* dst_argb,
                                   int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pcmpeqb    xmm5, xmm5           
    pxor       xmm4, xmm4

    align      4
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

__declspec(naked)
void I422ToBGRARow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_bgra,
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

    align      4
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

__declspec(naked)
void I422ToBGRARow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_bgra,
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

    align      4
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

__declspec(naked)
void I422ToABGRRow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_abgr,
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

    align      4
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

__declspec(naked)
void I422ToABGRRow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_abgr,
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

    align      4
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

__declspec(naked)
void I422ToRGBARow_SSSE3(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* dst_rgba,
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

    align      4
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

__declspec(naked)
void I422ToRGBARow_Unaligned_SSSE3(const uint8* y_buf,
                                   const uint8* u_buf,
                                   const uint8* v_buf,
                                   uint8* dst_rgba,
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

    align      4
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
__declspec(naked)
void YToARGBRow_SSE2(const uint8* y_buf,
                     uint8* rgb_buf,
                     int width) {
  __asm {
    pxor       xmm5, xmm5
    pcmpeqb    xmm4, xmm4           
    pslld      xmm4, 24
    mov        eax, 0x00100010
    movd       xmm3, eax
    pshufd     xmm3, xmm3, 0
    mov        eax, 0x004a004a       
    movd       xmm2, eax
    pshufd     xmm2, xmm2,0
    mov        eax, [esp + 4]       
    mov        edx, [esp + 8]       
    mov        ecx, [esp + 12]      

    align      4
 convertloop:
    
    movq       xmm0, qword ptr [eax]
    lea        eax, [eax + 8]
    punpcklbw  xmm0, xmm5           
    psubusw    xmm0, xmm3
    pmullw     xmm0, xmm2
    psrlw      xmm0, 6
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

__declspec(naked)
void MirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    movdqa    xmm5, kShuffleMirror
    lea       eax, [eax - 16]

    align      4
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

#ifdef HAS_MIRRORROW_AVX2

static const ulvec8 kShuffleMirror_AVX2 = {
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u,
  15u, 14u, 13u, 12u, 11u, 10u, 9u, 8u, 7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

__declspec(naked)
void MirrorRow_AVX2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    vmovdqa   ymm5, kShuffleMirror_AVX2
    lea       eax, [eax - 32]

    align      4
 convertloop:
    vmovdqu   ymm0, [eax + ecx]
    vpshufb   ymm0, ymm0, ymm5
    vpermq    ymm0, ymm0, 0x4e  
    sub       ecx, 32
    vmovdqu   [edx], ymm0
    lea       edx, [edx + 32]
    jg        convertloop
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_MIRRORROW_SSE2


__declspec(naked)
void MirrorRow_SSE2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    lea       eax, [eax - 16]

    align      4
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

__declspec(naked)
void MirrorUVRow_SSSE3(const uint8* src, uint8* dst_u, uint8* dst_v,
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

    align      4
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

__declspec(naked)
void ARGBMirrorRow_SSSE3(const uint8* src, uint8* dst, int width) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    lea       eax, [eax - 16 + ecx * 4]  
    movdqa    xmm5, kARGBShuffleMirror

    align      4
 convertloop:
    movdqa    xmm0, [eax]
    lea       eax, [eax - 16]
    pshufb    xmm0, xmm5
    sub       ecx, 4
    movdqa    [edx], xmm0
    lea       edx, [edx + 16]
    jg        convertloop
    ret
  }
}
#endif  

#ifdef HAS_ARGBMIRRORROW_AVX2

static const ulvec32 kARGBShuffleMirror_AVX2 = {
  7u, 6u, 5u, 4u, 3u, 2u, 1u, 0u
};

__declspec(naked)
void ARGBMirrorRow_AVX2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov       eax, [esp + 4]   
    mov       edx, [esp + 8]   
    mov       ecx, [esp + 12]  
    lea       eax, [eax - 32]
    vmovdqa   ymm5, kARGBShuffleMirror_AVX2

    align      4
 convertloop:
    vpermd    ymm0, ymm5, [eax + ecx * 4]  
    sub       ecx, 8
    vmovdqu   [edx], ymm0
    lea       edx, [edx + 32]
    jg        convertloop
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_SPLITUVROW_SSE2
__declspec(naked)
void SplitUVRow_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      4
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

__declspec(naked)
void SplitUVRow_Unaligned_SSE2(const uint8* src_uv, uint8* dst_u, uint8* dst_v,
                               int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8
    sub        edi, edx

    align      4
  convertloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    movdqa     xmm3, xmm1
    pand       xmm0, xmm5   
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    psrlw      xmm2, 8      
    psrlw      xmm3, 8
    packuswb   xmm2, xmm3
    movdqu     [edx], xmm0
    movdqu     [edx + edi], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}
#endif  

#ifdef HAS_SPLITUVROW_AVX2
__declspec(naked)
void SplitUVRow_AVX2(const uint8* src_uv, uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    vpcmpeqb   ymm5, ymm5, ymm5      
    vpsrlw     ymm5, ymm5, 8
    sub        edi, edx

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax,  [eax + 64]
    vpsrlw     ymm2, ymm0, 8      
    vpsrlw     ymm3, ymm1, 8
    vpand      ymm0, ymm0, ymm5   
    vpand      ymm1, ymm1, ymm5
    vpackuswb  ymm0, ymm0, ymm1
    vpackuswb  ymm2, ymm2, ymm3
    vpermq     ymm0, ymm0, 0xd8
    vpermq     ymm2, ymm2, 0xd8
    vmovdqu    [edx], ymm0
    vmovdqu    [edx + edi], ymm2
    lea        edx, [edx + 32]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_MERGEUVROW_SSE2
__declspec(naked)
void MergeUVRow_SSE2(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                     int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    sub        edx, eax

    align      4
  convertloop:
    movdqa     xmm0, [eax]      
    movdqa     xmm1, [eax + edx]  
    lea        eax,  [eax + 16]
    movdqa     xmm2, xmm0
    punpcklbw  xmm0, xmm1       
    punpckhbw  xmm2, xmm1       
    movdqa     [edi], xmm0
    movdqa     [edi + 16], xmm2
    lea        edi, [edi + 32]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}

__declspec(naked)
void MergeUVRow_Unaligned_SSE2(const uint8* src_u, const uint8* src_v,
                               uint8* dst_uv, int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    sub        edx, eax

    align      4
  convertloop:
    movdqu     xmm0, [eax]      
    movdqu     xmm1, [eax + edx]  
    lea        eax,  [eax + 16]
    movdqa     xmm2, xmm0
    punpcklbw  xmm0, xmm1       
    punpckhbw  xmm2, xmm1       
    movdqu     [edi], xmm0
    movdqu     [edi + 16], xmm2
    lea        edi, [edi + 32]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    ret
  }
}
#endif  

#ifdef HAS_MERGEUVROW_AVX2
__declspec(naked)
void MergeUVRow_AVX2(const uint8* src_u, const uint8* src_v, uint8* dst_uv,
                     int width) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    sub        edx, eax

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]           
    vmovdqu    ymm1, [eax + edx]     
    lea        eax,  [eax + 32]
    vpunpcklbw ymm2, ymm0, ymm1      
    vpunpckhbw ymm0, ymm0, ymm1      
    vperm2i128 ymm1, ymm2, ymm0, 0x20  
    vperm2i128 ymm2, ymm2, ymm0, 0x31  
    vmovdqu    [edi], ymm1
    vmovdqu    [edi + 32], ymm2
    lea        edi, [edi + 64]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_COPYROW_SSE2

__declspec(naked)
void CopyRow_SSE2(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  

    align      4
  convertloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    sub        ecx, 32
    jg         convertloop
    ret
  }
}
#endif  


__declspec(naked)
void CopyRow_ERMS(const uint8* src, uint8* dst, int count) {
  __asm {
    mov        eax, esi
    mov        edx, edi
    mov        esi, [esp + 4]   
    mov        edi, [esp + 8]   
    mov        ecx, [esp + 12]  
    rep movsb
    mov        edi, edx
    mov        esi, eax
    ret
  }
}

#ifdef HAS_COPYROW_X86
__declspec(naked)
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

#ifdef HAS_ARGBCOPYALPHAROW_SSE2

__declspec(naked)
void ARGBCopyAlphaRow_SSE2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    pcmpeqb    xmm0, xmm0       
    pslld      xmm0, 24
    pcmpeqb    xmm1, xmm1       
    psrld      xmm1, 8

    align      4
  convertloop:
    movdqa     xmm2, [eax]
    movdqa     xmm3, [eax + 16]
    lea        eax, [eax + 32]
    movdqa     xmm4, [edx]
    movdqa     xmm5, [edx + 16]
    pand       xmm2, xmm0
    pand       xmm3, xmm0
    pand       xmm4, xmm1
    pand       xmm5, xmm1
    por        xmm2, xmm4
    por        xmm3, xmm5
    movdqa     [edx], xmm2
    movdqa     [edx + 16], xmm3
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBCOPYALPHAROW_AVX2

__declspec(naked)
void ARGBCopyAlphaRow_AVX2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    vpcmpeqb   ymm0, ymm0, ymm0
    vpsrld     ymm0, ymm0, 8    

    align      4
  convertloop:
    vmovdqu    ymm1, [eax]
    vmovdqu    ymm2, [eax + 32]
    lea        eax, [eax + 64]
    vpblendvb  ymm1, ymm1, [edx], ymm0
    vpblendvb  ymm2, ymm2, [edx + 32], ymm0
    vmovdqu    [edx], ymm1
    vmovdqu    [edx + 32], ymm2
    lea        edx, [edx + 64]
    sub        ecx, 16
    jg         convertloop

    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBCOPYYTOALPHAROW_SSE2

__declspec(naked)
void ARGBCopyYToAlphaRow_SSE2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    pcmpeqb    xmm0, xmm0       
    pslld      xmm0, 24
    pcmpeqb    xmm1, xmm1       
    psrld      xmm1, 8

    align      4
  convertloop:
    movq       xmm2, qword ptr [eax]  
    lea        eax, [eax + 8]
    punpcklbw  xmm2, xmm2
    punpckhwd  xmm3, xmm2
    punpcklwd  xmm2, xmm2
    movdqa     xmm4, [edx]
    movdqa     xmm5, [edx + 16]
    pand       xmm2, xmm0
    pand       xmm3, xmm0
    pand       xmm4, xmm1
    pand       xmm5, xmm1
    por        xmm2, xmm4
    por        xmm3, xmm5
    movdqa     [edx], xmm2
    movdqa     [edx + 16], xmm3
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBCOPYYTOALPHAROW_AVX2

__declspec(naked)
void ARGBCopyYToAlphaRow_AVX2(const uint8* src, uint8* dst, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    vpcmpeqb   ymm0, ymm0, ymm0
    vpsrld     ymm0, ymm0, 8    

    align      4
  convertloop:
    vpmovzxbd  ymm1, qword ptr [eax]
    vpmovzxbd  ymm2, qword ptr [eax + 8]
    lea        eax, [eax + 16]
    vpslld     ymm1, ymm1, 24
    vpslld     ymm2, ymm2, 24
    vpblendvb  ymm1, ymm1, [edx], ymm0
    vpblendvb  ymm2, ymm2, [edx + 32], ymm0
    vmovdqu    [edx], ymm1
    vmovdqu    [edx + 32], ymm2
    lea        edx, [edx + 64]
    sub        ecx, 16
    jg         convertloop

    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_SETROW_X86

__declspec(naked)
void SetRow_X86(uint8* dst, uint32 v32, int count) {
  __asm {
    mov        edx, edi
    mov        edi, [esp + 4]   
    mov        eax, [esp + 8]   
    mov        ecx, [esp + 12]  
    shr        ecx, 2
    rep stosd
    mov        edi, edx
    ret
  }
}


__declspec(naked)
void ARGBSetRows_X86(uint8* dst, uint32 v32, int width,
                   int dst_stride, int height) {
  __asm {
    push       esi
    push       edi
    push       ebp
    mov        edi, [esp + 12 + 4]   
    mov        eax, [esp + 12 + 8]   
    mov        ebp, [esp + 12 + 12]  
    mov        edx, [esp + 12 + 16]  
    mov        esi, [esp + 12 + 20]  
    lea        ecx, [ebp * 4]
    sub        edx, ecx             

    align      4
  convertloop:
    mov        ecx, ebp
    rep stosd
    add        edi, edx
    sub        esi, 1
    jg         convertloop

    pop        ebp
    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_YUY2TOYROW_AVX2
__declspec(naked)
void YUY2ToYRow_AVX2(const uint8* src_yuy2,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    vpcmpeqb   ymm5, ymm5, ymm5  
    vpsrlw     ymm5, ymm5, 8

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax,  [eax + 64]
    vpand      ymm0, ymm0, ymm5   
    vpand      ymm1, ymm1, ymm5
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    sub        ecx, 32
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    jg         convertloop
    vzeroupper
    ret
  }
}

__declspec(naked)
void YUY2ToUVRow_AVX2(const uint8* src_yuy2, int stride_yuy2,
                      uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    vpcmpeqb   ymm5, ymm5, ymm5      
    vpsrlw     ymm5, ymm5, 8
    sub        edi, edx

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    vpavgb     ymm0, ymm0, [eax + esi]
    vpavgb     ymm1, ymm1, [eax + esi + 32]
    lea        eax,  [eax + 64]
    vpsrlw     ymm0, ymm0, 8      
    vpsrlw     ymm1, ymm1, 8
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    vpand      ymm1, ymm0, ymm5  
    vpsrlw     ymm0, ymm0, 8     
    vpackuswb  ymm1, ymm1, ymm1  
    vpackuswb  ymm0, ymm0, ymm0  
    vpermq     ymm1, ymm1, 0xd8
    vpermq     ymm0, ymm0, 0xd8
    vextractf128 [edx], ymm1, 0  
    vextractf128 [edx + edi], ymm0, 0 
    lea        edx, [edx + 16]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}

__declspec(naked)
void YUY2ToUV422Row_AVX2(const uint8* src_yuy2,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    vpcmpeqb   ymm5, ymm5, ymm5      
    vpsrlw     ymm5, ymm5, 8
    sub        edi, edx

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax,  [eax + 64]
    vpsrlw     ymm0, ymm0, 8      
    vpsrlw     ymm1, ymm1, 8
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    vpand      ymm1, ymm0, ymm5  
    vpsrlw     ymm0, ymm0, 8     
    vpackuswb  ymm1, ymm1, ymm1  
    vpackuswb  ymm0, ymm0, ymm0  
    vpermq     ymm1, ymm1, 0xd8
    vpermq     ymm0, ymm0, 0xd8
    vextractf128 [edx], ymm1, 0  
    vextractf128 [edx + edi], ymm0, 0 
    lea        edx, [edx + 16]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    vzeroupper
    ret
  }
}

__declspec(naked)
void UYVYToYRow_AVX2(const uint8* src_uyvy,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax,  [eax + 64]
    vpsrlw     ymm0, ymm0, 8      
    vpsrlw     ymm1, ymm1, 8
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    sub        ecx, 32
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    jg         convertloop
    ret
    vzeroupper
  }
}

__declspec(naked)
void UYVYToUVRow_AVX2(const uint8* src_uyvy, int stride_uyvy,
                      uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    vpcmpeqb   ymm5, ymm5, ymm5      
    vpsrlw     ymm5, ymm5, 8
    sub        edi, edx

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    vpavgb     ymm0, ymm0, [eax + esi]
    vpavgb     ymm1, ymm1, [eax + esi + 32]
    lea        eax,  [eax + 64]
    vpand      ymm0, ymm0, ymm5   
    vpand      ymm1, ymm1, ymm5
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    vpand      ymm1, ymm0, ymm5  
    vpsrlw     ymm0, ymm0, 8     
    vpackuswb  ymm1, ymm1, ymm1  
    vpackuswb  ymm0, ymm0, ymm0  
    vpermq     ymm1, ymm1, 0xd8
    vpermq     ymm0, ymm0, 0xd8
    vextractf128 [edx], ymm1, 0  
    vextractf128 [edx + edi], ymm0, 0 
    lea        edx, [edx + 16]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}

__declspec(naked)
void UYVYToUV422Row_AVX2(const uint8* src_uyvy,
                         uint8* dst_u, uint8* dst_v, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    vpcmpeqb   ymm5, ymm5, ymm5      
    vpsrlw     ymm5, ymm5, 8
    sub        edi, edx

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax,  [eax + 64]
    vpand      ymm0, ymm0, ymm5   
    vpand      ymm1, ymm1, ymm5
    vpackuswb  ymm0, ymm0, ymm1   
    vpermq     ymm0, ymm0, 0xd8
    vpand      ymm1, ymm0, ymm5  
    vpsrlw     ymm0, ymm0, 8     
    vpackuswb  ymm1, ymm1, ymm1  
    vpackuswb  ymm0, ymm0, ymm0  
    vpermq     ymm1, ymm1, 0xd8
    vpermq     ymm0, ymm0, 0xd8
    vextractf128 [edx], ymm1, 0  
    vextractf128 [edx + edi], ymm0, 0 
    lea        edx, [edx + 16]
    sub        ecx, 32
    jg         convertloop

    pop        edi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_YUY2TOYROW_SSE2
__declspec(naked)
void YUY2ToYRow_SSE2(const uint8* src_yuy2,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    pcmpeqb    xmm5, xmm5        
    psrlw      xmm5, 8

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
void YUY2ToYRow_Unaligned_SSE2(const uint8* src_yuy2,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    pcmpeqb    xmm5, xmm5        
    psrlw      xmm5, 8

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
void UYVYToYRow_SSE2(const uint8* src_uyvy,
                     uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
void UYVYToYRow_Unaligned_SSE2(const uint8* src_uyvy,
                               uint8* dst_y, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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

    align      4
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

__declspec(naked)
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
    pshufhw    xmm3, xmm3, 0F5h 
    pshuflw    xmm3, xmm3, 0F5h
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
    pshufhw    xmm3, xmm3, 0F5h 
    pshuflw    xmm3, xmm3, 0F5h
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
    pshufhw    xmm3, xmm3, 0F5h 
    pshuflw    xmm3, xmm3, 0F5h
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








__declspec(naked)
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

#ifdef HAS_ARGBATTENUATEROW_SSE2


__declspec(naked)
void ARGBAttenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    pcmpeqb    xmm4, xmm4       
    pslld      xmm4, 24
    pcmpeqb    xmm5, xmm5       
    psrld      xmm5, 8

    align      4
 convertloop:
    movdqa     xmm0, [eax]      
    punpcklbw  xmm0, xmm0       
    pshufhw    xmm2, xmm0, 0FFh 
    pshuflw    xmm2, xmm2, 0FFh
    pmulhuw    xmm0, xmm2       
    movdqa     xmm1, [eax]      
    punpckhbw  xmm1, xmm1       
    pshufhw    xmm2, xmm1, 0FFh 
    pshuflw    xmm2, xmm2, 0FFh
    pmulhuw    xmm1, xmm2       
    movdqa     xmm2, [eax]      
    lea        eax, [eax + 16]
    psrlw      xmm0, 8
    pand       xmm2, xmm4
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    pand       xmm0, xmm5       
    por        xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
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
__declspec(naked)
void ARGBAttenuateRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    pcmpeqb    xmm3, xmm3       
    pslld      xmm3, 24
    movdqa     xmm4, kShuffleAlpha0
    movdqa     xmm5, kShuffleAlpha1

    align      4
 convertloop:
    movdqu     xmm0, [eax]      
    pshufb     xmm0, xmm4       
    movdqu     xmm1, [eax]      
    punpcklbw  xmm1, xmm1       
    pmulhuw    xmm0, xmm1       
    movdqu     xmm1, [eax]      
    pshufb     xmm1, xmm5       
    movdqu     xmm2, [eax]      
    punpckhbw  xmm2, xmm2       
    pmulhuw    xmm1, xmm2       
    movdqu     xmm2, [eax]      
    lea        eax, [eax + 16]
    pand       xmm2, xmm3
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    por        xmm0, xmm2       
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBATTENUATEROW_AVX2

static const ulvec8 kShuffleAlpha_AVX2 = {
  6u, 7u, 6u, 7u, 6u, 7u, 128u, 128u,
  14u, 15u, 14u, 15u, 14u, 15u, 128u, 128u,
  6u, 7u, 6u, 7u, 6u, 7u, 128u, 128u,
  14u, 15u, 14u, 15u, 14u, 15u, 128u, 128u,
};
__declspec(naked)
void ARGBAttenuateRow_AVX2(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax
    vmovdqa    ymm4, kShuffleAlpha_AVX2
    vpcmpeqb   ymm5, ymm5, ymm5 
    vpslld     ymm5, ymm5, 24

    align      4
 convertloop:
    vmovdqu    ymm6, [eax]       
    vpunpcklbw ymm0, ymm6, ymm6  
    vpunpckhbw ymm1, ymm6, ymm6  
    vpshufb    ymm2, ymm0, ymm4  
    vpshufb    ymm3, ymm1, ymm4  
    vpmulhuw   ymm0, ymm0, ymm2  
    vpmulhuw   ymm1, ymm1, ymm3  
    vpand      ymm6, ymm6, ymm5  
    vpsrlw     ymm0, ymm0, 8
    vpsrlw     ymm1, ymm1, 8
    vpackuswb  ymm0, ymm0, ymm1  
    vpor       ymm0, ymm0, ymm6  
    sub        ecx, 8
    vmovdqu    [eax + edx], ymm0
    lea        eax, [eax + 32]
    jg         convertloop

    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBUNATTENUATEROW_SSE2


__declspec(naked)
void ARGBUnattenuateRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        edx, [esp + 8 + 8]   
    mov        ecx, [esp + 8 + 12]  

    align      4
 convertloop:
    movdqu     xmm0, [eax]      
    movzx      esi, byte ptr [eax + 3]  
    movzx      edi, byte ptr [eax + 7]  
    punpcklbw  xmm0, xmm0       
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2, 040h 
    pshuflw    xmm3, xmm3, 040h 
    movlhps    xmm2, xmm3
    pmulhuw    xmm0, xmm2       

    movdqu     xmm1, [eax]      
    movzx      esi, byte ptr [eax + 11]  
    movzx      edi, byte ptr [eax + 15]  
    punpckhbw  xmm1, xmm1       
    movd       xmm2, dword ptr fixed_invtbl8[esi * 4]
    movd       xmm3, dword ptr fixed_invtbl8[edi * 4]
    pshuflw    xmm2, xmm2, 040h 
    pshuflw    xmm3, xmm3, 040h 
    movlhps    xmm2, xmm3
    pmulhuw    xmm1, xmm2       
    lea        eax, [eax + 16]

    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop
    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBUNATTENUATEROW_AVX2

static const ulvec8 kUnattenShuffleAlpha_AVX2 = {
  0u, 1u, 0u, 1u, 0u, 1u, 6u, 7u, 8u, 9u, 8u, 9u, 8u, 9u, 14u, 15,
  0u, 1u, 0u, 1u, 0u, 1u, 6u, 7u, 8u, 9u, 8u, 9u, 8u, 9u, 14u, 15,
};


#ifdef USE_GATHER
__declspec(naked)
void ARGBUnattenuateRow_AVX2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax
    vmovdqa    ymm4, kUnattenShuffleAlpha_AVX2

    align      4
 convertloop:
    vmovdqu    ymm6, [eax]       
    vpcmpeqb   ymm5, ymm5, ymm5  
    vpsrld     ymm2, ymm6, 24    
    vpunpcklbw ymm0, ymm6, ymm6  
    vpunpckhbw ymm1, ymm6, ymm6  
    vpgatherdd ymm3, [ymm2 * 4 + fixed_invtbl8], ymm5  
    vpunpcklwd ymm2, ymm3, ymm3  
    vpunpckhwd ymm3, ymm3, ymm3  
    vpshufb    ymm2, ymm2, ymm4  
    vpshufb    ymm3, ymm3, ymm4  
    vpmulhuw   ymm0, ymm0, ymm2  
    vpmulhuw   ymm1, ymm1, ymm3  
    vpackuswb  ymm0, ymm0, ymm1  
    sub        ecx, 8
    vmovdqu    [eax + edx], ymm0
    lea        eax, [eax + 32]
    jg         convertloop

    vzeroupper
    ret
  }
}
#else  
__declspec(naked)
void ARGBUnattenuateRow_AVX2(const uint8* src_argb, uint8* dst_argb,
                             int width) {
  __asm {

    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    sub        edx, eax
    vmovdqa    ymm5, kUnattenShuffleAlpha_AVX2

    push       esi
    push       edi

    align      4
 convertloop:
    
    movzx      esi, byte ptr [eax + 3]                 
    movzx      edi, byte ptr [eax + 7]                 
    vmovd      xmm0, dword ptr fixed_invtbl8[esi * 4]  
    vmovd      xmm1, dword ptr fixed_invtbl8[edi * 4]  
    movzx      esi, byte ptr [eax + 11]                
    movzx      edi, byte ptr [eax + 15]                
    vpunpckldq xmm6, xmm0, xmm1                        
    vmovd      xmm2, dword ptr fixed_invtbl8[esi * 4]  
    vmovd      xmm3, dword ptr fixed_invtbl8[edi * 4]  
    movzx      esi, byte ptr [eax + 19]                
    movzx      edi, byte ptr [eax + 23]                
    vpunpckldq xmm7, xmm2, xmm3                        
    vmovd      xmm0, dword ptr fixed_invtbl8[esi * 4]  
    vmovd      xmm1, dword ptr fixed_invtbl8[edi * 4]  
    movzx      esi, byte ptr [eax + 27]                
    movzx      edi, byte ptr [eax + 31]                
    vpunpckldq xmm0, xmm0, xmm1                        
    vmovd      xmm2, dword ptr fixed_invtbl8[esi * 4]  
    vmovd      xmm3, dword ptr fixed_invtbl8[edi * 4]  
    vpunpckldq xmm2, xmm2, xmm3                        
    vpunpcklqdq xmm3, xmm6, xmm7                       
    vpunpcklqdq xmm0, xmm0, xmm2                       
    vinserti128 ymm3, ymm3, xmm0, 1 
    

    vmovdqu    ymm6, [eax]       
    vpunpcklbw ymm0, ymm6, ymm6  
    vpunpckhbw ymm1, ymm6, ymm6  
    vpunpcklwd ymm2, ymm3, ymm3  
    vpunpckhwd ymm3, ymm3, ymm3  
    vpshufb    ymm2, ymm2, ymm5  
    vpshufb    ymm3, ymm3, ymm5  
    vpmulhuw   ymm0, ymm0, ymm2  
    vpmulhuw   ymm1, ymm1, ymm3  
    vpackuswb  ymm0, ymm0, ymm1  
    sub        ecx, 8
    vmovdqu    [eax + edx], ymm0
    lea        eax, [eax + 32]
    jg         convertloop

    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}
#endif  
#endif  

#ifdef HAS_ARGBGRAYROW_SSSE3

__declspec(naked)
void ARGBGrayRow_SSSE3(const uint8* src_argb, uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqa     xmm4, kARGBToYJ
    movdqa     xmm5, kAddYJ64

    align      4
 convertloop:
    movdqa     xmm0, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm0, xmm4
    pmaddubsw  xmm1, xmm4
    phaddw     xmm0, xmm1
    paddw      xmm0, xmm5  
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0   
    movdqa     xmm2, [eax]  
    movdqa     xmm3, [eax + 16]
    lea        eax, [eax + 32]
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
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
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


__declspec(naked)
void ARGBSepiaRow_SSSE3(uint8* dst_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        ecx, [esp + 8]   
    movdqa     xmm2, kARGBToSepiaB
    movdqa     xmm3, kARGBToSepiaG
    movdqa     xmm4, kARGBToSepiaR

    align      4
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




__declspec(naked)
void ARGBColorMatrixRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                              const int8* matrix_argb, int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movdqu     xmm5, [ecx]
    pshufd     xmm2, xmm5, 0x00
    pshufd     xmm3, xmm5, 0x55
    pshufd     xmm4, xmm5, 0xaa
    pshufd     xmm5, xmm5, 0xff
    mov        ecx, [esp + 16]  

    align      4
 convertloop:
    movdqa     xmm0, [eax]  
    movdqa     xmm7, [eax + 16]
    pmaddubsw  xmm0, xmm2
    pmaddubsw  xmm7, xmm2
    movdqa     xmm6, [eax]  
    movdqa     xmm1, [eax + 16]
    pmaddubsw  xmm6, xmm3
    pmaddubsw  xmm1, xmm3
    phaddsw    xmm0, xmm7   
    phaddsw    xmm6, xmm1   
    psraw      xmm0, 6      
    psraw      xmm6, 6      
    packuswb   xmm0, xmm0   
    packuswb   xmm6, xmm6   
    punpcklbw  xmm0, xmm6   
    movdqa     xmm1, [eax]  
    movdqa     xmm7, [eax + 16]
    pmaddubsw  xmm1, xmm4
    pmaddubsw  xmm7, xmm4
    phaddsw    xmm1, xmm7   
    movdqa     xmm6, [eax]  
    movdqa     xmm7, [eax + 16]
    pmaddubsw  xmm6, xmm5
    pmaddubsw  xmm7, xmm5
    phaddsw    xmm6, xmm7   
    psraw      xmm1, 6      
    psraw      xmm6, 6      
    packuswb   xmm1, xmm1   
    packuswb   xmm6, xmm6   
    punpcklbw  xmm1, xmm6   
    movdqa     xmm6, xmm0   
    punpcklwd  xmm0, xmm1   
    punpckhwd  xmm6, xmm1   
    sub        ecx, 8
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm6
    lea        eax, [eax + 32]
    lea        edx, [edx + 32]
    jg         convertloop
    ret
  }
}
#endif  

#ifdef HAS_ARGBQUANTIZEROW_SSE2


__declspec(naked)
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

    align      4
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

#ifdef HAS_ARGBSHADEROW_SSE2


__declspec(naked)
void ARGBShadeRow_SSE2(const uint8* src_argb, uint8* dst_argb, int width,
                       uint32 value) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]  
    movd       xmm2, [esp + 16]  
    punpcklbw  xmm2, xmm2
    punpcklqdq xmm2, xmm2

    align      4
 convertloop:
    movdqa     xmm0, [eax]      
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm0       
    punpckhbw  xmm1, xmm1       
    pmulhuw    xmm0, xmm2       
    pmulhuw    xmm1, xmm2       
    psrlw      xmm0, 8
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop

    ret
  }
}
#endif  

#ifdef HAS_ARGBMULTIPLYROW_SSE2

__declspec(naked)
void ARGBMultiplyRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pxor       xmm5, xmm5  

    align      4
 convertloop:
    movdqu     xmm0, [eax]        
    movdqu     xmm2, [esi]        
    movdqu     xmm1, xmm0
    movdqu     xmm3, xmm2
    punpcklbw  xmm0, xmm0         
    punpckhbw  xmm1, xmm1         
    punpcklbw  xmm2, xmm5         
    punpckhbw  xmm3, xmm5         
    pmulhuw    xmm0, xmm2         
    pmulhuw    xmm1, xmm3         
    lea        eax, [eax + 16]
    lea        esi, [esi + 16]
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBADDROW_SSE2


__declspec(naked)
void ARGBAddRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                     uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  

    sub        ecx, 4
    jl         convertloop49

    align      4
 convertloop4:
    movdqu     xmm0, [eax]        
    lea        eax, [eax + 16]
    movdqu     xmm1, [esi]        
    lea        esi, [esi + 16]
    paddusb    xmm0, xmm1         
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jge        convertloop4

 convertloop49:
    add        ecx, 4 - 1
    jl         convertloop19

 convertloop1:
    movd       xmm0, [eax]        
    lea        eax, [eax + 4]
    movd       xmm1, [esi]        
    lea        esi, [esi + 4]
    paddusb    xmm0, xmm1         
    sub        ecx, 1
    movd       [edx], xmm0
    lea        edx, [edx + 4]
    jge        convertloop1

 convertloop19:
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBSUBTRACTROW_SSE2

__declspec(naked)
void ARGBSubtractRow_SSE2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  

    align      4
 convertloop:
    movdqu     xmm0, [eax]        
    lea        eax, [eax + 16]
    movdqu     xmm1, [esi]        
    lea        esi, [esi + 16]
    psubusb    xmm0, xmm1         
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBMULTIPLYROW_AVX2

__declspec(naked)
void ARGBMultiplyRow_AVX2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    vpxor      ymm5, ymm5, ymm5     

    align      4
 convertloop:
    vmovdqu    ymm1, [eax]        
    lea        eax, [eax + 32]
    vmovdqu    ymm3, [esi]        
    lea        esi, [esi + 32]
    vpunpcklbw ymm0, ymm1, ymm1   
    vpunpckhbw ymm1, ymm1, ymm1   
    vpunpcklbw ymm2, ymm3, ymm5   
    vpunpckhbw ymm3, ymm3, ymm5   
    vpmulhuw   ymm0, ymm0, ymm2   
    vpmulhuw   ymm1, ymm1, ymm3   
    vpackuswb  ymm0, ymm0, ymm1
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBADDROW_AVX2

__declspec(naked)
void ARGBAddRow_AVX2(const uint8* src_argb0, const uint8* src_argb1,
                     uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  

    align      4
 convertloop:
    vmovdqu    ymm0, [eax]              
    lea        eax, [eax + 32]
    vpaddusb   ymm0, ymm0, [esi]        
    lea        esi, [esi + 32]
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBSUBTRACTROW_AVX2

__declspec(naked)
void ARGBSubtractRow_AVX2(const uint8* src_argb0, const uint8* src_argb1,
                          uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  

    align      4
 convertloop:
    vmovdqu    ymm0, [eax]              
    lea        eax, [eax + 32]
    vpsubusb   ymm0, ymm0, [esi]        
    lea        esi, [esi + 32]
    vmovdqu    [edx], ymm0
    lea        edx, [edx + 32]
    sub        ecx, 8
    jg         convertloop

    pop        esi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_SOBELXROW_SSE2




__declspec(naked)
void SobelXRow_SSE2(const uint8* src_y0, const uint8* src_y1,
                    const uint8* src_y2, uint8* dst_sobelx, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        esi, [esp + 8 + 8]   
    mov        edi, [esp + 8 + 12]  
    mov        edx, [esp + 8 + 16]  
    mov        ecx, [esp + 8 + 20]  
    sub        esi, eax
    sub        edi, eax
    sub        edx, eax
    pxor       xmm5, xmm5  

    align      4
 convertloop:
    movq       xmm0, qword ptr [eax]            
    movq       xmm1, qword ptr [eax + 2]        
    punpcklbw  xmm0, xmm5
    punpcklbw  xmm1, xmm5
    psubw      xmm0, xmm1
    movq       xmm1, qword ptr [eax + esi]      
    movq       xmm2, qword ptr [eax + esi + 2]  
    punpcklbw  xmm1, xmm5
    punpcklbw  xmm2, xmm5
    psubw      xmm1, xmm2
    movq       xmm2, qword ptr [eax + edi]      
    movq       xmm3, qword ptr [eax + edi + 2]  
    punpcklbw  xmm2, xmm5
    punpcklbw  xmm3, xmm5
    psubw      xmm2, xmm3
    paddw      xmm0, xmm2
    paddw      xmm0, xmm1
    paddw      xmm0, xmm1
    pxor       xmm1, xmm1   
    psubw      xmm1, xmm0
    pmaxsw     xmm0, xmm1
    packuswb   xmm0, xmm0
    sub        ecx, 8
    movq       qword ptr [eax + edx], xmm0
    lea        eax, [eax + 8]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_SOBELYROW_SSE2




__declspec(naked)
void SobelYRow_SSE2(const uint8* src_y0, const uint8* src_y1,
                    uint8* dst_sobely, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    sub        esi, eax
    sub        edx, eax
    pxor       xmm5, xmm5  

    align      4
 convertloop:
    movq       xmm0, qword ptr [eax]            
    movq       xmm1, qword ptr [eax + esi]      
    punpcklbw  xmm0, xmm5
    punpcklbw  xmm1, xmm5
    psubw      xmm0, xmm1
    movq       xmm1, qword ptr [eax + 1]        
    movq       xmm2, qword ptr [eax + esi + 1]  
    punpcklbw  xmm1, xmm5
    punpcklbw  xmm2, xmm5
    psubw      xmm1, xmm2
    movq       xmm2, qword ptr [eax + 2]        
    movq       xmm3, qword ptr [eax + esi + 2]  
    punpcklbw  xmm2, xmm5
    punpcklbw  xmm3, xmm5
    psubw      xmm2, xmm3
    paddw      xmm0, xmm2
    paddw      xmm0, xmm1
    paddw      xmm0, xmm1
    pxor       xmm1, xmm1   
    psubw      xmm1, xmm0
    pmaxsw     xmm0, xmm1
    packuswb   xmm0, xmm0
    sub        ecx, 8
    movq       qword ptr [eax + edx], xmm0
    lea        eax, [eax + 8]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_SOBELROW_SSE2





__declspec(naked)
void SobelRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                   uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    sub        esi, eax
    pcmpeqb    xmm5, xmm5           
    pslld      xmm5, 24             

    align      4
 convertloop:
    movdqa     xmm0, [eax]            
    movdqa     xmm1, [eax + esi]      
    lea        eax, [eax + 16]
    paddusb    xmm0, xmm1             
    movdqa     xmm2, xmm0             
    punpcklbw  xmm2, xmm0             
    punpckhbw  xmm0, xmm0             
    movdqa     xmm1, xmm2             
    punpcklwd  xmm1, xmm2             
    punpckhwd  xmm2, xmm2             
    por        xmm1, xmm5             
    por        xmm2, xmm5
    movdqa     xmm3, xmm0             
    punpcklwd  xmm3, xmm0             
    punpckhwd  xmm0, xmm0             
    por        xmm3, xmm5             
    por        xmm0, xmm5
    sub        ecx, 16
    movdqa     [edx], xmm1
    movdqa     [edx + 16], xmm2
    movdqa     [edx + 32], xmm3
    movdqa     [edx + 48], xmm0
    lea        edx, [edx + 64]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_SOBELTOPLANEROW_SSE2

__declspec(naked)
void SobelToPlaneRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                          uint8* dst_y, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    sub        esi, eax

    align      4
 convertloop:
    movdqa     xmm0, [eax]            
    movdqa     xmm1, [eax + esi]      
    lea        eax, [eax + 16]
    paddusb    xmm0, xmm1             
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_SOBELXYROW_SSE2





__declspec(naked)
void SobelXYRow_SSE2(const uint8* src_sobelx, const uint8* src_sobely,
                     uint8* dst_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        edx, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    sub        esi, eax
    pcmpeqb    xmm5, xmm5           

    align      4
 convertloop:
    movdqa     xmm0, [eax]            
    movdqa     xmm1, [eax + esi]      
    lea        eax, [eax + 16]
    movdqa     xmm2, xmm0
    paddusb    xmm2, xmm1             
    movdqa     xmm3, xmm0             
    punpcklbw  xmm3, xmm5
    punpckhbw  xmm0, xmm5
    movdqa     xmm4, xmm1             
    punpcklbw  xmm4, xmm2
    punpckhbw  xmm1, xmm2
    movdqa     xmm6, xmm4             
    punpcklwd  xmm6, xmm3             
    punpckhwd  xmm4, xmm3             
    movdqa     xmm7, xmm1             
    punpcklwd  xmm7, xmm0             
    punpckhwd  xmm1, xmm0             
    sub        ecx, 16
    movdqa     [edx], xmm6
    movdqa     [edx + 16], xmm4
    movdqa     [edx + 32], xmm7
    movdqa     [edx + 48], xmm1
    lea        edx, [edx + 64]
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_CUMULATIVESUMTOAVERAGEROW_SSE2













void CumulativeSumToAverageRow_SSE2(const int32* topleft, const int32* botleft,
                                    int width, int area, uint8* dst,
                                    int count) {
  __asm {
    mov        eax, topleft  
    mov        esi, botleft  
    mov        edx, width
    movd       xmm5, area
    mov        edi, dst
    mov        ecx, count
    cvtdq2ps   xmm5, xmm5
    rcpss      xmm4, xmm5  
    pshufd     xmm4, xmm4, 0
    sub        ecx, 4
    jl         l4b

    cmp        area, 128  
    ja         l4

    pshufd     xmm5, xmm5, 0        
    pcmpeqb    xmm6, xmm6           
    psrld      xmm6, 16
    cvtdq2ps   xmm6, xmm6
    addps      xmm5, xmm6           
    mulps      xmm5, xmm4           
    cvtps2dq   xmm5, xmm5           
    packssdw   xmm5, xmm5           

    
    align      4
  s4:
    
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

    packssdw   xmm0, xmm1  
    packssdw   xmm2, xmm3

    pmulhuw    xmm0, xmm5
    pmulhuw    xmm2, xmm5

    packuswb   xmm0, xmm2
    movdqu     [edi], xmm0
    lea        edi, [edi + 16]
    sub        ecx, 4
    jge        s4

    jmp        l4b

    
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
    movdqa     xmm2, [esi]  
    paddd      xmm2, xmm0

    paddd      xmm0, xmm3
    movdqa     xmm3, [esi + 16]
    paddd      xmm3, xmm0

    paddd      xmm0, xmm4
    movdqa     xmm4, [esi + 32]
    paddd      xmm4, xmm0

    paddd      xmm0, xmm5
    movdqa     xmm5, [esi + 48]
    lea        esi, [esi + 64]
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
    movdqu     xmm2, [esi]
    lea        esi, [esi + 16]
    paddd      xmm2, xmm0
    movdqu     [edx], xmm2
    lea        edx, [edx + 16]
    sub        ecx, 1
    jge        l1

 l1b:
  }
}
#endif  

#ifdef HAS_ARGBAFFINEROW_SSE2

__declspec(naked)
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

#ifdef HAS_INTERPOLATEROW_AVX2

__declspec(naked)
void InterpolateRow_AVX2(uint8* dst_ptr, const uint8* src_ptr,
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
    shr        eax, 1
    
    cmp        eax, 0
    je         xloop100  
    sub        edi, esi
    cmp        eax, 32
    je         xloop75   
    cmp        eax, 64
    je         xloop50   
    cmp        eax, 96
    je         xloop25   

    vmovd      xmm0, eax  
    neg        eax
    add        eax, 128
    vmovd      xmm5, eax  
    vpunpcklbw xmm5, xmm5, xmm0
    vpunpcklwd xmm5, xmm5, xmm5
    vpxor      ymm0, ymm0, ymm0
    vpermd     ymm5, ymm0, ymm5

    align      4
  xloop:
    vmovdqu    ymm0, [esi]
    vmovdqu    ymm2, [esi + edx]
    vpunpckhbw ymm1, ymm0, ymm2  
    vpunpcklbw ymm0, ymm0, ymm2  
    vpmaddubsw ymm0, ymm0, ymm5
    vpmaddubsw ymm1, ymm1, ymm5
    vpsrlw     ymm0, ymm0, 7
    vpsrlw     ymm1, ymm1, 7
    vpackuswb  ymm0, ymm0, ymm1  
    sub        ecx, 32
    vmovdqu    [esi + edi], ymm0
    lea        esi, [esi + 32]
    jg         xloop
    jmp        xloop99

    
    align      4
  xloop25:
    vmovdqu    ymm0, [esi]
    vpavgb     ymm0, ymm0, [esi + edx]
    vpavgb     ymm0, ymm0, [esi + edx]
    sub        ecx, 32
    vmovdqu    [esi + edi], ymm0
    lea        esi, [esi + 32]
    jg         xloop25
    jmp        xloop99

    
    align      4
  xloop50:
    vmovdqu    ymm0, [esi]
    vpavgb     ymm0, ymm0, [esi + edx]
    sub        ecx, 32
    vmovdqu    [esi + edi], ymm0
    lea        esi, [esi + 32]
    jg         xloop50
    jmp        xloop99

    
    align      4
  xloop75:
    vmovdqu    ymm0, [esi + edx]
    vpavgb     ymm0, ymm0, [esi]
    vpavgb     ymm0, ymm0, [esi]
    sub        ecx, 32
    vmovdqu     [esi + edi], ymm0
    lea        esi, [esi + 32]
    jg         xloop75
    jmp        xloop99

    
    align      4
  xloop100:
    rep movsb

  xloop99:
    pop        edi
    pop        esi
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_INTERPOLATEROW_SSSE3

__declspec(naked)
void InterpolateRow_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
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
    je         xloop100  
    cmp        eax, 32
    je         xloop75   
    cmp        eax, 64
    je         xloop50   
    cmp        eax, 96
    je         xloop25   

    movd       xmm0, eax  
    neg        eax
    add        eax, 128
    movd       xmm5, eax  
    punpcklbw  xmm5, xmm0
    punpcklwd  xmm5, xmm5
    pshufd     xmm5, xmm5, 0

    align      4
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
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop
    jmp        xloop99

    
    align      4
  xloop25:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop25
    jmp        xloop99

    
    align      4
  xloop50:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop50
    jmp        xloop99

    
    align      4
  xloop75:
    movdqa     xmm1, [esi]
    movdqa     xmm0, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop75
    jmp        xloop99

    
    align      4
  xloop100:
    movdqa     xmm0, [esi]
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop100

  xloop99:
    pop        edi
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_INTERPOLATEROW_SSE2

__declspec(naked)
void InterpolateRow_SSE2(uint8* dst_ptr, const uint8* src_ptr,
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
    
    cmp        eax, 0
    je         xloop100  
    cmp        eax, 64
    je         xloop75   
    cmp        eax, 128
    je         xloop50   
    cmp        eax, 192
    je         xloop25   

    movd       xmm5, eax            
    punpcklbw  xmm5, xmm5
    psrlw      xmm5, 1
    punpcklwd  xmm5, xmm5
    punpckldq  xmm5, xmm5
    punpcklqdq xmm5, xmm5
    pxor       xmm4, xmm4

    align      4
  xloop:
    movdqa     xmm0, [esi]  
    movdqa     xmm2, [esi + edx]  
    movdqa     xmm1, xmm0
    movdqa     xmm3, xmm2
    punpcklbw  xmm2, xmm4
    punpckhbw  xmm3, xmm4
    punpcklbw  xmm0, xmm4
    punpckhbw  xmm1, xmm4
    psubw      xmm2, xmm0  
    psubw      xmm3, xmm1
    paddw      xmm2, xmm2  
    paddw      xmm3, xmm3
    pmulhw     xmm2, xmm5  
    pmulhw     xmm3, xmm5
    paddw      xmm0, xmm2  
    paddw      xmm1, xmm3
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop
    jmp        xloop99

    
    align      4
  xloop25:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop25
    jmp        xloop99

    
    align      4
  xloop50:
    movdqa     xmm0, [esi]
    movdqa     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop50
    jmp        xloop99

    
    align      4
  xloop75:
    movdqa     xmm1, [esi]
    movdqa     xmm0, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop75
    jmp        xloop99

    
    align      4
  xloop100:
    movdqa     xmm0, [esi]
    sub        ecx, 16
    movdqa     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop100

  xloop99:
    pop        edi
    pop        esi
    ret
  }
}
#endif  


__declspec(naked)
void InterpolateRow_Unaligned_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
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
    je         xloop100  
    cmp        eax, 32
    je         xloop75   
    cmp        eax, 64
    je         xloop50   
    cmp        eax, 96
    je         xloop25   

    movd       xmm0, eax  
    neg        eax
    add        eax, 128
    movd       xmm5, eax  
    punpcklbw  xmm5, xmm0
    punpcklwd  xmm5, xmm5
    pshufd     xmm5, xmm5, 0

    align      4
  xloop:
    movdqu     xmm0, [esi]
    movdqu     xmm2, [esi + edx]
    movdqu     xmm1, xmm0
    punpcklbw  xmm0, xmm2
    punpckhbw  xmm1, xmm2
    pmaddubsw  xmm0, xmm5
    pmaddubsw  xmm1, xmm5
    psrlw      xmm0, 7
    psrlw      xmm1, 7
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop
    jmp        xloop99

    
    align      4
  xloop25:
    movdqu     xmm0, [esi]
    movdqu     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop25
    jmp        xloop99

    
    align      4
  xloop50:
    movdqu     xmm0, [esi]
    movdqu     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop50
    jmp        xloop99

    
    align      4
  xloop75:
    movdqu     xmm1, [esi]
    movdqu     xmm0, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop75
    jmp        xloop99

    
    align      4
  xloop100:
    movdqu     xmm0, [esi]
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop100

  xloop99:
    pop        edi
    pop        esi
    ret
  }
}

#ifdef HAS_INTERPOLATEROW_SSE2

__declspec(naked)
void InterpolateRow_Unaligned_SSE2(uint8* dst_ptr, const uint8* src_ptr,
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
    
    cmp        eax, 0
    je         xloop100  
    cmp        eax, 64
    je         xloop75   
    cmp        eax, 128
    je         xloop50   
    cmp        eax, 192
    je         xloop25   

    movd       xmm5, eax            
    punpcklbw  xmm5, xmm5
    psrlw      xmm5, 1
    punpcklwd  xmm5, xmm5
    punpckldq  xmm5, xmm5
    punpcklqdq xmm5, xmm5
    pxor       xmm4, xmm4

    align      4
  xloop:
    movdqu     xmm0, [esi]  
    movdqu     xmm2, [esi + edx]  
    movdqu     xmm1, xmm0
    movdqu     xmm3, xmm2
    punpcklbw  xmm2, xmm4
    punpckhbw  xmm3, xmm4
    punpcklbw  xmm0, xmm4
    punpckhbw  xmm1, xmm4
    psubw      xmm2, xmm0  
    psubw      xmm3, xmm1
    paddw      xmm2, xmm2  
    paddw      xmm3, xmm3
    pmulhw     xmm2, xmm5  
    pmulhw     xmm3, xmm5
    paddw      xmm0, xmm2  
    paddw      xmm1, xmm3
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop
    jmp        xloop99

    
    align      4
  xloop25:
    movdqu     xmm0, [esi]
    movdqu     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop25
    jmp        xloop99

    
    align      4
  xloop50:
    movdqu     xmm0, [esi]
    movdqu     xmm1, [esi + edx]
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop50
    jmp        xloop99

    
    align      4
  xloop75:
    movdqu     xmm1, [esi]
    movdqu     xmm0, [esi + edx]
    pavgb      xmm0, xmm1
    pavgb      xmm0, xmm1
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop75
    jmp        xloop99

    
    align      4
  xloop100:
    movdqu     xmm0, [esi]
    sub        ecx, 16
    movdqu     [esi + edi], xmm0
    lea        esi, [esi + 16]
    jg         xloop100

  xloop99:
    pop        edi
    pop        esi
    ret
  }
}
#endif  

__declspec(naked)
void HalfRow_SSE2(const uint8* src_uv, int src_uv_stride,
                  uint8* dst_uv, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    sub        edi, eax

    align      4
  convertloop:
    movdqa     xmm0, [eax]
    pavgb      xmm0, [eax + edx]
    sub        ecx, 16
    movdqa     [eax + edi], xmm0
    lea        eax,  [eax + 16]
    jg         convertloop
    pop        edi
    ret
  }
}

#ifdef HAS_HALFROW_AVX2
__declspec(naked)
void HalfRow_AVX2(const uint8* src_uv, int src_uv_stride,
                  uint8* dst_uv, int pix) {
  __asm {
    push       edi
    mov        eax, [esp + 4 + 4]    
    mov        edx, [esp + 4 + 8]    
    mov        edi, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    sub        edi, eax

    align      4
  convertloop:
    vmovdqu    ymm0, [eax]
    vpavgb     ymm0, ymm0, [eax + edx]
    sub        ecx, 32
    vmovdqu    [eax + edi], ymm0
    lea        eax,  [eax + 32]
    jg         convertloop

    pop        edi
    vzeroupper
    ret
  }
}
#endif  

__declspec(naked)
void ARGBToBayerRow_SSSE3(const uint8* src_argb, uint8* dst_bayer,
                          uint32 selector, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    movd       xmm5, [esp + 12]  
    mov        ecx, [esp + 16]   
    pshufd     xmm5, xmm5, 0

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    pshufb     xmm0, xmm5
    pshufb     xmm1, xmm5
    punpckldq  xmm0, xmm1
    sub        ecx, 8
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    jg         wloop
    ret
  }
}


__declspec(naked)
void ARGBToBayerGGRow_SSE2(const uint8* src_argb, uint8* dst_bayer,
                           uint32 selector, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
                                 
    mov        ecx, [esp + 16]   
    pcmpeqb    xmm5, xmm5        
    psrld      xmm5, 24

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    psrld      xmm0, 8  
    psrld      xmm1, 8
    pand       xmm0, xmm5
    pand       xmm1, xmm5
    packssdw   xmm0, xmm1
    packuswb   xmm0, xmm1
    sub        ecx, 8
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    jg         wloop
    ret
  }
}


__declspec(naked)
void ARGBShuffleRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                          const uint8* shuffler, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    movdqa     xmm5, [ecx]
    mov        ecx, [esp + 16]   

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    pshufb     xmm0, xmm5
    pshufb     xmm1, xmm5
    sub        ecx, 8
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    jg         wloop
    ret
  }
}

__declspec(naked)
void ARGBShuffleRow_Unaligned_SSSE3(const uint8* src_argb, uint8* dst_argb,
                                    const uint8* shuffler, int pix) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    movdqa     xmm5, [ecx]
    mov        ecx, [esp + 16]   

    align      4
  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax, [eax + 32]
    pshufb     xmm0, xmm5
    pshufb     xmm1, xmm5
    sub        ecx, 8
    movdqu     [edx], xmm0
    movdqu     [edx + 16], xmm1
    lea        edx, [edx + 32]
    jg         wloop
    ret
  }
}

#ifdef HAS_ARGBSHUFFLEROW_AVX2
__declspec(naked)
void ARGBShuffleRow_AVX2(const uint8* src_argb, uint8* dst_argb,
                         const uint8* shuffler, int pix) {
  __asm {
    mov        eax, [esp + 4]     
    mov        edx, [esp + 8]     
    mov        ecx, [esp + 12]    
    vbroadcastf128 ymm5, [ecx]    
    mov        ecx, [esp + 16]    

    align      4
  wloop:
    vmovdqu    ymm0, [eax]
    vmovdqu    ymm1, [eax + 32]
    lea        eax, [eax + 64]
    vpshufb    ymm0, ymm0, ymm5
    vpshufb    ymm1, ymm1, ymm5
    sub        ecx, 16
    vmovdqu    [edx], ymm0
    vmovdqu    [edx + 32], ymm1
    lea        edx, [edx + 64]
    jg         wloop

    vzeroupper
    ret
  }
}
#endif  

__declspec(naked)
void ARGBShuffleRow_SSE2(const uint8* src_argb, uint8* dst_argb,
                         const uint8* shuffler, int pix) {
  __asm {
    push       ebx
    push       esi
    mov        eax, [esp + 8 + 4]    
    mov        edx, [esp + 8 + 8]    
    mov        esi, [esp + 8 + 12]   
    mov        ecx, [esp + 8 + 16]   
    pxor       xmm5, xmm5

    mov        ebx, [esi]   
    cmp        ebx, 0x03000102
    je         shuf_3012
    cmp        ebx, 0x00010203
    je         shuf_0123
    cmp        ebx, 0x00030201
    je         shuf_0321
    cmp        ebx, 0x02010003
    je         shuf_2103

  
  shuf_any1:
    movzx      ebx, byte ptr [esi]
    movzx      ebx, byte ptr [eax + ebx]
    mov        [edx], bl
    movzx      ebx, byte ptr [esi + 1]
    movzx      ebx, byte ptr [eax + ebx]
    mov        [edx + 1], bl
    movzx      ebx, byte ptr [esi + 2]
    movzx      ebx, byte ptr [eax + ebx]
    mov        [edx + 2], bl
    movzx      ebx, byte ptr [esi + 3]
    movzx      ebx, byte ptr [eax + ebx]
    mov        [edx + 3], bl
    lea        eax, [eax + 4]
    lea        edx, [edx + 4]
    sub        ecx, 1
    jg         shuf_any1
    jmp        shuf99

    align      4
  shuf_0123:
    movdqu     xmm0, [eax]
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm5
    punpckhbw  xmm1, xmm5
    pshufhw    xmm0, xmm0, 01Bh   
    pshuflw    xmm0, xmm0, 01Bh
    pshufhw    xmm1, xmm1, 01Bh
    pshuflw    xmm1, xmm1, 01Bh
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         shuf_0123
    jmp        shuf99

    align      4
  shuf_0321:
    movdqu     xmm0, [eax]
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm5
    punpckhbw  xmm1, xmm5
    pshufhw    xmm0, xmm0, 039h   
    pshuflw    xmm0, xmm0, 039h
    pshufhw    xmm1, xmm1, 039h
    pshuflw    xmm1, xmm1, 039h
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         shuf_0321
    jmp        shuf99

    align      4
  shuf_2103:
    movdqu     xmm0, [eax]
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm5
    punpckhbw  xmm1, xmm5
    pshufhw    xmm0, xmm0, 093h   
    pshuflw    xmm0, xmm0, 093h
    pshufhw    xmm1, xmm1, 093h
    pshuflw    xmm1, xmm1, 093h
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         shuf_2103
    jmp        shuf99

    align      4
  shuf_3012:
    movdqu     xmm0, [eax]
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm5
    punpckhbw  xmm1, xmm5
    pshufhw    xmm0, xmm0, 0C6h   
    pshuflw    xmm0, xmm0, 0C6h
    pshufhw    xmm1, xmm1, 0C6h
    pshuflw    xmm1, xmm1, 0C6h
    packuswb   xmm0, xmm1
    sub        ecx, 4
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         shuf_3012

  shuf99:
    pop        esi
    pop        ebx
    ret
  }
}







__declspec(naked)
void I422ToYUY2Row_SSE2(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_frame, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    sub        edx, esi

    align      4
  convertloop:
    movq       xmm2, qword ptr [esi] 
    movq       xmm3, qword ptr [esi + edx] 
    lea        esi, [esi + 8]
    punpcklbw  xmm2, xmm3 
    movdqu     xmm0, [eax] 
    lea        eax, [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm2 
    punpckhbw  xmm1, xmm2
    movdqu     [edi], xmm0
    movdqu     [edi + 16], xmm1
    lea        edi, [edi + 32]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

__declspec(naked)
void I422ToUYVYRow_SSE2(const uint8* src_y,
                        const uint8* src_u,
                        const uint8* src_v,
                        uint8* dst_frame, int width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        edi, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    sub        edx, esi

    align      4
  convertloop:
    movq       xmm2, qword ptr [esi] 
    movq       xmm3, qword ptr [esi + edx] 
    lea        esi, [esi + 8]
    punpcklbw  xmm2, xmm3 
    movdqu     xmm0, [eax] 
    movdqa     xmm1, xmm2
    lea        eax, [eax + 16]
    punpcklbw  xmm1, xmm0 
    punpckhbw  xmm2, xmm0
    movdqu     [edi], xmm1
    movdqu     [edi + 16], xmm2
    lea        edi, [edi + 32]
    sub        ecx, 16
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}

#ifdef HAS_ARGBPOLYNOMIALROW_SSE2
__declspec(naked)
void ARGBPolynomialRow_SSE2(const uint8* src_argb,
                            uint8* dst_argb, const float* poly,
                            int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        edx, [esp + 4 + 8]   
    mov        esi, [esp + 4 + 12]  
    mov        ecx, [esp + 4 + 16]  
    pxor       xmm3, xmm3  

    
    align      4
 convertloop:


    movq       xmm0, qword ptr [eax]  
    lea        eax, [eax + 8]
    punpcklbw  xmm0, xmm3
    movdqa     xmm4, xmm0
    punpcklwd  xmm0, xmm3  
    punpckhwd  xmm4, xmm3  
    cvtdq2ps   xmm0, xmm0  
    cvtdq2ps   xmm4, xmm4
    movdqa     xmm1, xmm0  
    movdqa     xmm5, xmm4
    mulps      xmm0, [esi + 16]  
    mulps      xmm4, [esi + 16]
    addps      xmm0, [esi]  
    addps      xmm4, [esi]
    movdqa     xmm2, xmm1
    movdqa     xmm6, xmm5
    mulps      xmm2, xmm1  
    mulps      xmm6, xmm5
    mulps      xmm1, xmm2  
    mulps      xmm5, xmm6
    mulps      xmm2, [esi + 32]  
    mulps      xmm6, [esi + 32]
    mulps      xmm1, [esi + 48]  
    mulps      xmm5, [esi + 48]
    addps      xmm0, xmm2  
    addps      xmm4, xmm6
    addps      xmm0, xmm1  
    addps      xmm4, xmm5
    cvttps2dq  xmm0, xmm0
    cvttps2dq  xmm4, xmm4
    packuswb   xmm0, xmm4
    packuswb   xmm0, xmm0
    sub        ecx, 2
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    jg         convertloop
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBPOLYNOMIALROW_AVX2
__declspec(naked)
void ARGBPolynomialRow_AVX2(const uint8* src_argb,
                            uint8* dst_argb, const float* poly,
                            int width) {
  __asm {
    mov        eax, [esp + 4]   
    mov        edx, [esp + 8]   
    mov        ecx, [esp + 12]   
    vbroadcastf128 ymm4, [ecx]       
    vbroadcastf128 ymm5, [ecx + 16]  
    vbroadcastf128 ymm6, [ecx + 32]  
    vbroadcastf128 ymm7, [ecx + 48]  
    mov        ecx, [esp + 16]  

    
    align      4
 convertloop:
    vpmovzxbd   ymm0, qword ptr [eax]  
    lea         eax, [eax + 8]
    vcvtdq2ps   ymm0, ymm0        
    vmulps      ymm2, ymm0, ymm0  
    vmulps      ymm3, ymm0, ymm7  
    vfmadd132ps ymm0, ymm4, ymm5  
    vfmadd231ps ymm0, ymm2, ymm6  
    vfmadd231ps ymm0, ymm2, ymm3  
    vcvttps2dq  ymm0, ymm0
    vpackusdw   ymm0, ymm0, ymm0  
    vpermq      ymm0, ymm0, 0xd8  
    vpackuswb   xmm0, xmm0, xmm0  
    sub         ecx, 2
    vmovq       qword ptr [edx], xmm0
    lea         edx, [edx + 8]
    jg          convertloop
    vzeroupper
    ret
  }
}
#endif  

#ifdef HAS_ARGBCOLORTABLEROW_X86

__declspec(naked)
void ARGBColorTableRow_X86(uint8* dst_argb, const uint8* table_argb,
                           int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        ecx, [esp + 4 + 12]  

    
    align      4
  convertloop:
    movzx      edx, byte ptr [eax]
    lea        eax, [eax + 4]
    movzx      edx, byte ptr [esi + edx * 4]
    mov        byte ptr [eax - 4], dl
    movzx      edx, byte ptr [eax - 4 + 1]
    movzx      edx, byte ptr [esi + edx * 4 + 1]
    mov        byte ptr [eax - 4 + 1], dl
    movzx      edx, byte ptr [eax - 4 + 2]
    movzx      edx, byte ptr [esi + edx * 4 + 2]
    mov        byte ptr [eax - 4 + 2], dl
    movzx      edx, byte ptr [eax - 4 + 3]
    movzx      edx, byte ptr [esi + edx * 4 + 3]
    mov        byte ptr [eax - 4 + 3], dl
    dec        ecx
    jg         convertloop
    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_RGBCOLORTABLEROW_X86

__declspec(naked)
void RGBColorTableRow_X86(uint8* dst_argb, const uint8* table_argb, int width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]   
    mov        esi, [esp + 4 + 8]   
    mov        ecx, [esp + 4 + 12]  

    
    align      4
  convertloop:
    movzx      edx, byte ptr [eax]
    lea        eax, [eax + 4]
    movzx      edx, byte ptr [esi + edx * 4]
    mov        byte ptr [eax - 4], dl
    movzx      edx, byte ptr [eax - 4 + 1]
    movzx      edx, byte ptr [esi + edx * 4 + 1]
    mov        byte ptr [eax - 4 + 1], dl
    movzx      edx, byte ptr [eax - 4 + 2]
    movzx      edx, byte ptr [esi + edx * 4 + 2]
    mov        byte ptr [eax - 4 + 2], dl
    dec        ecx
    jg         convertloop

    pop        esi
    ret
  }
}
#endif  

#ifdef HAS_ARGBLUMACOLORTABLEROW_SSSE3

__declspec(naked)
void ARGBLumaColorTableRow_SSSE3(const uint8* src_argb, uint8* dst_argb,
                                 int width,
                                 const uint8* luma, uint32 lumacoeff) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]   
    mov        edi, [esp + 8 + 8]   
    mov        ecx, [esp + 8 + 12]  
    movd       xmm2, dword ptr [esp + 8 + 16]  
    movd       xmm3, dword ptr [esp + 8 + 20]  
    pshufd     xmm2, xmm2, 0
    pshufd     xmm3, xmm3, 0
    pcmpeqb    xmm4, xmm4        
    psllw      xmm4, 8
    pxor       xmm5, xmm5

    
    align      4
  convertloop:
    movdqu     xmm0, qword ptr [eax]      
    pmaddubsw  xmm0, xmm3
    phaddw     xmm0, xmm0
    pand       xmm0, xmm4  
    punpcklwd  xmm0, xmm5
    paddd      xmm0, xmm2  
    movd       esi, xmm0
    pshufd     xmm0, xmm0, 0x39  

    movzx      edx, byte ptr [eax]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi], dl
    movzx      edx, byte ptr [eax + 1]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 1], dl
    movzx      edx, byte ptr [eax + 2]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 2], dl
    movzx      edx, byte ptr [eax + 3]  
    mov        byte ptr [edi + 3], dl

    movd       esi, xmm0
    pshufd     xmm0, xmm0, 0x39  

    movzx      edx, byte ptr [eax + 4]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 4], dl
    movzx      edx, byte ptr [eax + 5]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 5], dl
    movzx      edx, byte ptr [eax + 6]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 6], dl
    movzx      edx, byte ptr [eax + 7]  
    mov        byte ptr [edi + 7], dl

    movd       esi, xmm0
    pshufd     xmm0, xmm0, 0x39  

    movzx      edx, byte ptr [eax + 8]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 8], dl
    movzx      edx, byte ptr [eax + 9]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 9], dl
    movzx      edx, byte ptr [eax + 10]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 10], dl
    movzx      edx, byte ptr [eax + 11]  
    mov        byte ptr [edi + 11], dl

    movd       esi, xmm0

    movzx      edx, byte ptr [eax + 12]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 12], dl
    movzx      edx, byte ptr [eax + 13]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 13], dl
    movzx      edx, byte ptr [eax + 14]
    movzx      edx, byte ptr [esi + edx]
    mov        byte ptr [edi + 14], dl
    movzx      edx, byte ptr [eax + 15]  
    mov        byte ptr [edi + 15], dl

    sub        ecx, 4
    lea        eax, [eax + 16]
    lea        edi, [edi + 16]
    jg         convertloop

    pop        edi
    pop        esi
    ret
  }
}
#endif  

#endif  

#ifdef __cplusplus
}  
}  
#endif
