









#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(LIBYUV_DISABLE_X86) && defined(_M_IX86) && defined(_MSC_VER)


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



__declspec(naked)
void ScaleRowDown2_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8               
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleRowDown2Linear_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                              uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm5
    pand       xmm3, xmm5
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleRowDown2Box_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2            
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm5
    pand       xmm3, xmm5
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    sub        ecx, 16
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleRowDown2_Unaligned_SSE2(const uint8* src_ptr,
                                  ptrdiff_t src_stride,
                                  uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       

    align      4
  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    psrlw      xmm0, 8               
    psrlw      xmm1, 8
    packuswb   xmm0, xmm1
    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleRowDown2Linear_Unaligned_SSE2(const uint8* src_ptr,
                                        ptrdiff_t src_stride,
                                        uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8

    align      4
  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    lea        eax,  [eax + 32]

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm5
    pand       xmm3, xmm5
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleRowDown2Box_Unaligned_SSE2(const uint8* src_ptr,
                                     ptrdiff_t src_stride,
                                     uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    pcmpeqb    xmm5, xmm5            
    psrlw      xmm5, 8

    align      4
  wloop:
    movdqu     xmm0, [eax]
    movdqu     xmm1, [eax + 16]
    movdqu     xmm2, [eax + esi]
    movdqu     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2            
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm5
    pand       xmm3, xmm5
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    sub        ecx, 16
    movdqu     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleRowDown4_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                        uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       
    pcmpeqb    xmm5, xmm5            
    psrld      xmm5, 24
    pslld      xmm5, 16

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    pand       xmm0, xmm5
    pand       xmm1, xmm5
    packuswb   xmm0, xmm1
    psrlw      xmm0, 8
    packuswb   xmm0, xmm0
    sub        ecx, 8
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleRowDown4Box_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                           uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    push       edi
    mov        eax, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        edx, [esp + 8 + 12]   
    mov        ecx, [esp + 8 + 16]   
    lea        edi, [esi + esi * 2]  
    pcmpeqb    xmm7, xmm7            
    psrlw      xmm7, 8

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    pavgb      xmm0, xmm2            
    pavgb      xmm1, xmm3
    movdqa     xmm2, [eax + esi * 2]
    movdqa     xmm3, [eax + esi * 2 + 16]
    movdqa     xmm4, [eax + edi]
    movdqa     xmm5, [eax + edi + 16]
    lea        eax, [eax + 32]
    pavgb      xmm2, xmm4
    pavgb      xmm3, xmm5
    pavgb      xmm0, xmm2
    pavgb      xmm1, xmm3

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    movdqa     xmm3, xmm1
    psrlw      xmm1, 8
    pand       xmm2, xmm7
    pand       xmm3, xmm7
    pavgw      xmm0, xmm2
    pavgw      xmm1, xmm3
    packuswb   xmm0, xmm1

    movdqa     xmm2, xmm0            
    psrlw      xmm0, 8
    pand       xmm2, xmm7
    pavgw      xmm0, xmm2
    packuswb   xmm0, xmm0

    sub        ecx, 8
    movq       qword ptr [edx], xmm0
    lea        edx, [edx + 8]
    jg         wloop

    pop        edi
    pop        esi
    ret
  }
}







__declspec(naked)
void ScaleRowDown34_SSSE3(const uint8* src_ptr, ptrdiff_t src_stride,
                          uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       
    movdqa     xmm3, kShuf0
    movdqa     xmm4, kShuf1
    movdqa     xmm5, kShuf2

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm1
    palignr    xmm1, xmm0, 8
    pshufb     xmm0, xmm3
    pshufb     xmm1, xmm4
    pshufb     xmm2, xmm5
    movq       qword ptr [edx], xmm0
    movq       qword ptr [edx + 8], xmm1
    movq       qword ptr [edx + 16], xmm2
    lea        edx, [edx + 24]
    sub        ecx, 24
    jg         wloop

    ret
  }
}

















__declspec(naked)
void ScaleRowDown34_1_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    movdqa     xmm2, kShuf01
    movdqa     xmm3, kShuf11
    movdqa     xmm4, kShuf21
    movdqa     xmm5, kMadd01
    movdqa     xmm6, kMadd11
    movdqa     xmm7, kRound34

    align      4
  wloop:
    movdqa     xmm0, [eax]           
    movdqa     xmm1, [eax + esi]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    movdqu     xmm0, [eax + 8]       
    movdqu     xmm1, [eax + esi + 8]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 8], xmm0
    movdqa     xmm0, [eax + 16]      
    movdqa     xmm1, [eax + esi + 16]
    lea        eax, [eax + 32]
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, kMadd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    sub        ecx, 24
    movq       qword ptr [edx + 16], xmm0
    lea        edx, [edx + 24]
    jg         wloop

    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleRowDown34_0_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    movdqa     xmm2, kShuf01
    movdqa     xmm3, kShuf11
    movdqa     xmm4, kShuf21
    movdqa     xmm5, kMadd01
    movdqa     xmm6, kMadd11
    movdqa     xmm7, kRound34

    align      4
  wloop:
    movdqa     xmm0, [eax]           
    movdqa     xmm1, [eax + esi]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm2
    pmaddubsw  xmm0, xmm5
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx], xmm0
    movdqu     xmm0, [eax + 8]       
    movdqu     xmm1, [eax + esi + 8]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm3
    pmaddubsw  xmm0, xmm6
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    movq       qword ptr [edx + 8], xmm0
    movdqa     xmm0, [eax + 16]      
    movdqa     xmm1, [eax + esi + 16]
    lea        eax, [eax + 32]
    pavgb      xmm1, xmm0
    pavgb      xmm0, xmm1
    pshufb     xmm0, xmm4
    movdqa     xmm1, kMadd21
    pmaddubsw  xmm0, xmm1
    paddsw     xmm0, xmm7
    psrlw      xmm0, 2
    packuswb   xmm0, xmm0
    sub        ecx, 24
    movq       qword ptr [edx + 16], xmm0
    lea        edx, [edx+24]
    jg         wloop

    pop        esi
    ret
  }
}




__declspec(naked)
void ScaleRowDown38_SSSE3(const uint8* src_ptr, ptrdiff_t src_stride,
                          uint8* dst_ptr, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       
    movdqa     xmm4, kShuf38a
    movdqa     xmm5, kShuf38b

    align      4
  xloop:
    movdqa     xmm0, [eax]           
    movdqa     xmm1, [eax + 16]      
    lea        eax, [eax + 32]
    pshufb     xmm0, xmm4
    pshufb     xmm1, xmm5
    paddusb    xmm0, xmm1

    sub        ecx, 12
    movq       qword ptr [edx], xmm0  
    movhlps    xmm1, xmm0
    movd       [edx + 8], xmm1
    lea        edx, [edx + 12]
    jg         xloop

    ret
  }
}


__declspec(naked)
void ScaleRowDown38_3_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    movdqa     xmm2, kShufAc
    movdqa     xmm3, kShufAc3
    movdqa     xmm4, kScaleAc33
    pxor       xmm5, xmm5

    align      4
  xloop:
    movdqa     xmm0, [eax]           
    movdqa     xmm6, [eax + esi]
    movhlps    xmm1, xmm0
    movhlps    xmm7, xmm6
    punpcklbw  xmm0, xmm5
    punpcklbw  xmm1, xmm5
    punpcklbw  xmm6, xmm5
    punpcklbw  xmm7, xmm5
    paddusw    xmm0, xmm6
    paddusw    xmm1, xmm7
    movdqa     xmm6, [eax + esi * 2]
    lea        eax, [eax + 16]
    movhlps    xmm7, xmm6
    punpcklbw  xmm6, xmm5
    punpcklbw  xmm7, xmm5
    paddusw    xmm0, xmm6
    paddusw    xmm1, xmm7

    movdqa     xmm6, xmm0            
    psrldq     xmm0, 2
    paddusw    xmm6, xmm0
    psrldq     xmm0, 2
    paddusw    xmm6, xmm0
    pshufb     xmm6, xmm2

    movdqa     xmm7, xmm1            
    psrldq     xmm1, 2
    paddusw    xmm7, xmm1
    psrldq     xmm1, 2
    paddusw    xmm7, xmm1
    pshufb     xmm7, xmm3
    paddusw    xmm6, xmm7

    pmulhuw    xmm6, xmm4            
    packuswb   xmm6, xmm6

    sub        ecx, 6
    movd       [edx], xmm6           
    psrlq      xmm6, 16
    movd       [edx + 2], xmm6
    lea        edx, [edx + 6]
    jg         xloop

    pop        esi
    ret
  }
}


__declspec(naked)
void ScaleRowDown38_2_Box_SSSE3(const uint8* src_ptr,
                                ptrdiff_t src_stride,
                                uint8* dst_ptr, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   
    movdqa     xmm2, kShufAb0
    movdqa     xmm3, kShufAb1
    movdqa     xmm4, kShufAb2
    movdqa     xmm5, kScaleAb2

    align      4
  xloop:
    movdqa     xmm0, [eax]           
    pavgb      xmm0, [eax + esi]
    lea        eax, [eax + 16]

    movdqa     xmm1, xmm0            
    pshufb     xmm1, xmm2
    movdqa     xmm6, xmm0
    pshufb     xmm6, xmm3
    paddusw    xmm1, xmm6
    pshufb     xmm0, xmm4
    paddusw    xmm1, xmm0

    pmulhuw    xmm1, xmm5            
    packuswb   xmm1, xmm1

    sub        ecx, 6
    movd       [edx], xmm1           
    psrlq      xmm1, 16
    movd       [edx + 2], xmm1
    lea        edx, [edx + 6]
    jg         xloop

    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleAddRows_SSE2(const uint8* src_ptr, ptrdiff_t src_stride,
                       uint16* dst_ptr, int src_width,
                       int src_height) {
  __asm {
    push       esi
    push       edi
    push       ebx
    push       ebp
    mov        esi, [esp + 16 + 4]   
    mov        edx, [esp + 16 + 8]   
    mov        edi, [esp + 16 + 12]  
    mov        ecx, [esp + 16 + 16]  
    mov        ebx, [esp + 16 + 20]  
    pxor       xmm4, xmm4
    dec        ebx

    align      4
  xloop:
    
    movdqa     xmm0, [esi]
    lea        eax, [esi + edx]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm4
    punpckhbw  xmm1, xmm4
    lea        esi, [esi + 16]
    mov        ebp, ebx
    test       ebp, ebp
    je         ydone

    
    align      4
  yloop:
    movdqa     xmm2, [eax]       
    lea        eax, [eax + edx]  
    movdqa     xmm3, xmm2
    punpcklbw  xmm2, xmm4
    punpckhbw  xmm3, xmm4
    paddusw    xmm0, xmm2        
    paddusw    xmm1, xmm3
    sub        ebp, 1
    jg         yloop

    align      4
  ydone:
    movdqa     [edi], xmm0
    movdqa     [edi + 16], xmm1
    lea        edi, [edi + 32]

    sub        ecx, 16
    jg         xloop

    pop        ebp
    pop        ebx
    pop        edi
    pop        esi
    ret
  }
}











__declspec(naked)
void ScaleFilterCols_SSSE3(uint8* dst_ptr, const uint8* src_ptr,
                           int dst_width, int x, int dx) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        edi, [esp + 12 + 4]    
    mov        esi, [esp + 12 + 8]    
    mov        ecx, [esp + 12 + 12]   
    movd       xmm2, [esp + 12 + 16]  
    movd       xmm3, [esp + 12 + 20]  
    mov        eax, 0x04040000      
    movd       xmm5, eax
    pcmpeqb    xmm6, xmm6           
    psrlw      xmm6, 9
    pextrw     eax, xmm2, 1         
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2           
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0           
    punpckldq  xmm3, xmm3           
    paddd      xmm3, xmm3           
    pextrw     edx, xmm2, 3         

    
    align      4
  xloop2:
    movdqa     xmm1, xmm2           
    paddd      xmm2, xmm3           
    movzx      ebx, word ptr [esi + eax]  
    movd       xmm0, ebx
    psrlw      xmm1, 9              
    movzx      ebx, word ptr [esi + edx]  
    movd       xmm4, ebx
    pshufb     xmm1, xmm5           
    punpcklwd  xmm0, xmm4
    pxor       xmm1, xmm6           
    pmaddubsw  xmm0, xmm1           
    pextrw     eax, xmm2, 1         
    pextrw     edx, xmm2, 3         
    psrlw      xmm0, 7              
    packuswb   xmm0, xmm0           
    movd       ebx, xmm0
    mov        [edi], bx
    lea        edi, [edi + 2]
    sub        ecx, 2               
    jge        xloop2

    align      4
 xloop29:

    add        ecx, 2 - 1
    jl         xloop99

    
    movzx      ebx, word ptr [esi + eax]  
    movd       xmm0, ebx
    psrlw      xmm2, 9              
    pshufb     xmm2, xmm5           
    pxor       xmm2, xmm6           
    pmaddubsw  xmm0, xmm2           
    psrlw      xmm0, 7              
    packuswb   xmm0, xmm0           
    movd       ebx, xmm0
    mov        [edi], bl

    align      4
 xloop99:

    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}



__declspec(naked)
void ScaleColsUp2_SSE2(uint8* dst_ptr, const uint8* src_ptr,
                       int dst_width, int x, int dx) {
  __asm {
    mov        edx, [esp + 4]    
    mov        eax, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      4
  wloop:
    movdqa     xmm0, [eax]
    lea        eax,  [eax + 16]
    movdqa     xmm1, xmm0
    punpcklbw  xmm0, xmm0
    punpckhbw  xmm1, xmm1
    sub        ecx, 32
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleARGBRowDown2_SSE2(const uint8* src_argb,
                            ptrdiff_t src_stride,
                            uint8* dst_argb, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    shufps     xmm0, xmm1, 0xdd
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleARGBRowDown2Linear_SSE2(const uint8* src_argb,
                                  ptrdiff_t src_stride,
                                  uint8* dst_argb, int dst_width) {
  __asm {
    mov        eax, [esp + 4]        
                                     
    mov        edx, [esp + 12]       
    mov        ecx, [esp + 16]       

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    lea        eax,  [eax + 32]
    movdqa     xmm2, xmm0
    shufps     xmm0, xmm1, 0x88      
    shufps     xmm2, xmm1, 0xdd      
    pavgb      xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    ret
  }
}



__declspec(naked)
void ScaleARGBRowDown2Box_SSE2(const uint8* src_argb,
                               ptrdiff_t src_stride,
                               uint8* dst_argb, int dst_width) {
  __asm {
    push       esi
    mov        eax, [esp + 4 + 4]    
    mov        esi, [esp + 4 + 8]    
    mov        edx, [esp + 4 + 12]   
    mov        ecx, [esp + 4 + 16]   

    align      4
  wloop:
    movdqa     xmm0, [eax]
    movdqa     xmm1, [eax + 16]
    movdqa     xmm2, [eax + esi]
    movdqa     xmm3, [eax + esi + 16]
    lea        eax,  [eax + 32]
    pavgb      xmm0, xmm2            
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0            
    shufps     xmm0, xmm1, 0x88      
    shufps     xmm2, xmm1, 0xdd      
    pavgb      xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleARGBRowDownEven_SSE2(const uint8* src_argb, ptrdiff_t src_stride,
                               int src_stepx,
                               uint8* dst_argb, int dst_width) {
  __asm {
    push       ebx
    push       edi
    mov        eax, [esp + 8 + 4]    
                                     
    mov        ebx, [esp + 8 + 12]   
    mov        edx, [esp + 8 + 16]   
    mov        ecx, [esp + 8 + 20]   
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

    align      4
  wloop:
    movd       xmm0, [eax]
    movd       xmm1, [eax + ebx]
    punpckldq  xmm0, xmm1
    movd       xmm2, [eax + ebx * 2]
    movd       xmm3, [eax + edi]
    lea        eax,  [eax + ebx * 4]
    punpckldq  xmm2, xmm3
    punpcklqdq xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        edi
    pop        ebx
    ret
  }
}



__declspec(naked)
void ScaleARGBRowDownEvenBox_SSE2(const uint8* src_argb,
                                  ptrdiff_t src_stride,
                                  int src_stepx,
                                  uint8* dst_argb, int dst_width) {
  __asm {
    push       ebx
    push       esi
    push       edi
    mov        eax, [esp + 12 + 4]    
    mov        esi, [esp + 12 + 8]    
    mov        ebx, [esp + 12 + 12]   
    mov        edx, [esp + 12 + 16]   
    mov        ecx, [esp + 12 + 20]   
    lea        esi, [eax + esi]       
    lea        ebx, [ebx * 4]
    lea        edi, [ebx + ebx * 2]

    align      4
  wloop:
    movq       xmm0, qword ptr [eax]  
    movhps     xmm0, qword ptr [eax + ebx]
    movq       xmm1, qword ptr [eax + ebx * 2]
    movhps     xmm1, qword ptr [eax + edi]
    lea        eax,  [eax + ebx * 4]
    movq       xmm2, qword ptr [esi]  
    movhps     xmm2, qword ptr [esi + ebx]
    movq       xmm3, qword ptr [esi + ebx * 2]
    movhps     xmm3, qword ptr [esi + edi]
    lea        esi,  [esi + ebx * 4]
    pavgb      xmm0, xmm2            
    pavgb      xmm1, xmm3
    movdqa     xmm2, xmm0            
    shufps     xmm0, xmm1, 0x88      
    shufps     xmm2, xmm1, 0xdd      
    pavgb      xmm0, xmm2
    sub        ecx, 4
    movdqa     [edx], xmm0
    lea        edx, [edx + 16]
    jg         wloop

    pop        edi
    pop        esi
    pop        ebx
    ret
  }
}


__declspec(naked)
void ScaleARGBCols_SSE2(uint8* dst_argb, const uint8* src_argb,
                        int dst_width, int x, int dx) {
  __asm {
    push       edi
    push       esi
    mov        edi, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        ecx, [esp + 8 + 12]   
    movd       xmm2, [esp + 8 + 16]  
    movd       xmm3, [esp + 8 + 20]  

    pshufd     xmm2, xmm2, 0         
    pshufd     xmm0, xmm3, 0x11      
    paddd      xmm2, xmm0
    paddd      xmm3, xmm3            
    pshufd     xmm0, xmm3, 0x05      
    paddd      xmm2, xmm0            
    paddd      xmm3, xmm3            
    pshufd     xmm3, xmm3, 0         

    pextrw     eax, xmm2, 1          
    pextrw     edx, xmm2, 3          

    cmp        ecx, 0
    jle        xloop99
    sub        ecx, 4
    jl         xloop49

    
    align      4
 xloop4:
    movd       xmm0, [esi + eax * 4]  
    movd       xmm1, [esi + edx * 4]  
    pextrw     eax, xmm2, 5           
    pextrw     edx, xmm2, 7           
    paddd      xmm2, xmm3             
    punpckldq  xmm0, xmm1             

    movd       xmm1, [esi + eax * 4]  
    movd       xmm4, [esi + edx * 4]  
    pextrw     eax, xmm2, 1           
    pextrw     edx, xmm2, 3           
    punpckldq  xmm1, xmm4             
    punpcklqdq xmm0, xmm1             
    sub        ecx, 4                 
    movdqu     [edi], xmm0
    lea        edi, [edi + 16]
    jge        xloop4

    align      4
 xloop49:
    test       ecx, 2
    je         xloop29

    
    movd       xmm0, [esi + eax * 4]  
    movd       xmm1, [esi + edx * 4]  
    pextrw     eax, xmm2, 5           
    punpckldq  xmm0, xmm1             

    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]

 xloop29:
    test       ecx, 1
    je         xloop99

    
    movd       xmm0, [esi + eax * 4]  
    movd       dword ptr [edi], xmm0
    align      4
 xloop99:

    pop        esi
    pop        edi
    ret
  }
}





static uvec8 kShuffleColARGB = {
  0u, 4u, 1u, 5u, 2u, 6u, 3u, 7u,  
  8u, 12u, 9u, 13u, 10u, 14u, 11u, 15u  
};


static uvec8 kShuffleFractions = {
  0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 4u, 4u, 4u, 4u, 4u, 4u, 4u, 4u,
};

__declspec(naked)
void ScaleARGBFilterCols_SSSE3(uint8* dst_argb, const uint8* src_argb,
                               int dst_width, int x, int dx) {
  __asm {
    push       esi
    push       edi
    mov        edi, [esp + 8 + 4]    
    mov        esi, [esp + 8 + 8]    
    mov        ecx, [esp + 8 + 12]   
    movd       xmm2, [esp + 8 + 16]  
    movd       xmm3, [esp + 8 + 20]  
    movdqa     xmm4, kShuffleColARGB
    movdqa     xmm5, kShuffleFractions
    pcmpeqb    xmm6, xmm6           
    psrlw      xmm6, 9
    pextrw     eax, xmm2, 1         
    sub        ecx, 2
    jl         xloop29

    movdqa     xmm0, xmm2           
    paddd      xmm0, xmm3
    punpckldq  xmm2, xmm0           
    punpckldq  xmm3, xmm3           
    paddd      xmm3, xmm3           
    pextrw     edx, xmm2, 3         

    
    align      4
  xloop2:
    movdqa     xmm1, xmm2           
    paddd      xmm2, xmm3           
    movq       xmm0, qword ptr [esi + eax * 4]  
    psrlw      xmm1, 9              
    movhps     xmm0, qword ptr [esi + edx * 4]  
    pshufb     xmm1, xmm5           
    pshufb     xmm0, xmm4           
    pxor       xmm1, xmm6           
    pmaddubsw  xmm0, xmm1           
    pextrw     eax, xmm2, 1         
    pextrw     edx, xmm2, 3         
    psrlw      xmm0, 7              
    packuswb   xmm0, xmm0           
    movq       qword ptr [edi], xmm0
    lea        edi, [edi + 8]
    sub        ecx, 2               
    jge        xloop2

    align      4
 xloop29:

    add        ecx, 2 - 1
    jl         xloop99

    
    psrlw      xmm2, 9              
    movq       xmm0, qword ptr [esi + eax * 4]  
    pshufb     xmm2, xmm5           
    pshufb     xmm0, xmm4           
    pxor       xmm2, xmm6           
    pmaddubsw  xmm0, xmm2           
    psrlw      xmm0, 7
    packuswb   xmm0, xmm0           
    movd       [edi], xmm0

    align      4
 xloop99:

    pop        edi
    pop        esi
    ret
  }
}



__declspec(naked)
void ScaleARGBColsUp2_SSE2(uint8* dst_argb, const uint8* src_argb,
                           int dst_width, int x, int dx) {
  __asm {
    mov        edx, [esp + 4]    
    mov        eax, [esp + 8]    
    mov        ecx, [esp + 12]   

    align      4
  wloop:
    movdqa     xmm0, [eax]
    lea        eax,  [eax + 16]
    movdqa     xmm1, xmm0
    punpckldq  xmm0, xmm0
    punpckhdq  xmm1, xmm1
    sub        ecx, 8
    movdqa     [edx], xmm0
    movdqa     [edx + 16], xmm1
    lea        edx, [edx + 32]
    jg         wloop

    ret
  }
}


__declspec(naked)
int FixedDiv_X86(int num, int div) {
  __asm {
    mov        eax, [esp + 4]    
    cdq                          
    shld       edx, eax, 16      
    shl        eax, 16
    idiv       dword ptr [esp + 8]
    ret
  }
}


__declspec(naked)
int FixedDiv1_X86(int num, int div) {
  __asm {
    mov        eax, [esp + 4]    
    mov        ecx, [esp + 8]    
    cdq                          
    shld       edx, eax, 16      
    shl        eax, 16
    sub        eax, 0x00010001
    sbb        edx, 0
    sub        ecx, 1
    idiv       ecx
    ret
  }
}

#endif  

#ifdef __cplusplus
}  
}  
#endif
