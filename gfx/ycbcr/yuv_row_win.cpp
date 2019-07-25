



#include "yuv_row.h"
#define MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE2
#define MOZILLA_SSE_INCLUDE_HEADER_FOR_MMX
#include "mozilla/SSE.h"


#define kCoefficientsRgbU kCoefficientsRgbY + 2048
#define kCoefficientsRgbV kCoefficientsRgbY + 4096

extern "C" {
#if defined(MOZILLA_COMPILE_WITH_SSE2)
__declspec(naked)
void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   
    mov       edi, [esp + 32 + 8]   
    mov       esi, [esp + 32 + 12]  
    mov       ebp, [esp + 32 + 16]  
    mov       ecx, [esp + 32 + 20]  
    jmp       convertend

 convertloop :
    movzx     eax, byte ptr [edi]
    add       edi, 1
    movzx     ebx, byte ptr [esi]
    add       esi, 1
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [kCoefficientsRgbV + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    add       edx, 2
    movq      mm2, [kCoefficientsRgbY + 8 * ebx]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    add       ebp, 8
 convertend :
    sub       ecx, 2
    jns       convertloop

    and       ecx, 1  
    jz        convertdone

    movzx     eax, byte ptr [edi]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    movd      [ebp], mm1
 convertdone :

    popad
    ret
  }
}

__declspec(naked)
void ConvertYUVToRGB32Row(const uint8* y_buf,
                          const uint8* u_buf,
                          const uint8* v_buf,
                          uint8* rgb_buf,
                          int width,
                          int step) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   
    mov       edi, [esp + 32 + 8]   
    mov       esi, [esp + 32 + 12]  
    mov       ebp, [esp + 32 + 16]  
    mov       ecx, [esp + 32 + 20]  
    mov       ebx, [esp + 32 + 24]  
    jmp       wend

 wloop :
    movzx     eax, byte ptr [edi]
    add       edi, ebx
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    add       esi, ebx
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    add       edx, ebx
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    movzx     eax, byte ptr [edx]
    add       edx, ebx
    movq      mm2, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    add       ebp, 8
 wend :
    sub       ecx, 2
    jns       wloop

    and       ecx, 1  
    jz        wdone

    movzx     eax, byte ptr [edi]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    movd      [ebp], mm1
 wdone :

    popad
    ret
  }
}

__declspec(naked)
void RotateConvertYUVToRGB32Row(const uint8* y_buf,
                                const uint8* u_buf,
                                const uint8* v_buf,
                                uint8* rgb_buf,
                                int width,
                                int ystep,
                                int uvstep) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   
    mov       edi, [esp + 32 + 8]   
    mov       esi, [esp + 32 + 12]  
    mov       ebp, [esp + 32 + 16]  
    mov       ecx, [esp + 32 + 20]  
    jmp       wend

 wloop :
    movzx     eax, byte ptr [edi]
    mov       ebx, [esp + 32 + 28]  
    add       edi, ebx
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    add       esi, ebx
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    mov       ebx, [esp + 32 + 24]  
    add       edx, ebx
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    movzx     eax, byte ptr [edx]
    add       edx, ebx
    movq      mm2, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    add       ebp, 8
 wend :
    sub       ecx, 2
    jns       wloop

    and       ecx, 1  
    jz        wdone

    movzx     eax, byte ptr [edi]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    movd      [ebp], mm1
 wdone :

    popad
    ret
  }
}

__declspec(naked)
void DoubleYUVToRGB32Row(const uint8* y_buf,
                         const uint8* u_buf,
                         const uint8* v_buf,
                         uint8* rgb_buf,
                         int width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   
    mov       edi, [esp + 32 + 8]   
    mov       esi, [esp + 32 + 12]  
    mov       ebp, [esp + 32 + 16]  
    mov       ecx, [esp + 32 + 20]  
    jmp       wend

 wloop :
    movzx     eax, byte ptr [edi]
    add       edi, 1
    movzx     ebx, byte ptr [esi]
    add       esi, 1
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [kCoefficientsRgbV + 8 * ebx]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    punpckldq mm1, mm1
    movntq    [ebp], mm1

    movzx     ebx, byte ptr [edx + 1]
    add       edx, 2
    paddsw    mm0, [kCoefficientsRgbY + 8 * ebx]
    psraw     mm0, 6
    packuswb  mm0, mm0
    punpckldq mm0, mm0
    movntq    [ebp+8], mm0
    add       ebp, 16
 wend :
    sub       ecx, 4
    jns       wloop

    add       ecx, 4
    jz        wdone

    movzx     eax, byte ptr [edi]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    movzx     eax, byte ptr [esi]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    movzx     eax, byte ptr [edx]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    jmp       wend1

 wloop1 :
    movd      [ebp], mm1
    add       ebp, 4
 wend1 :
    sub       ecx, 1
    jns       wloop1
 wdone :
    popad
    ret
  }
}





__declspec(naked)
void ScaleYUVToRGB32Row(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width,
                        int source_dx) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   
    mov       edi, [esp + 32 + 8]   
    mov       esi, [esp + 32 + 12]  
    mov       ebp, [esp + 32 + 16]  
    mov       ecx, [esp + 32 + 20]  
    xor       ebx, ebx              
    jmp       scaleend

 scaleloop :
    mov       eax, ebx
    sar       eax, 17
    movzx     eax, byte ptr [edi + eax]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    mov       eax, ebx
    sar       eax, 17
    movzx     eax, byte ptr [esi + eax]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    mov       eax, ebx
    add       ebx, [esp + 32 + 24]  
    sar       eax, 16
    movzx     eax, byte ptr [edx + eax]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    mov       eax, ebx
    add       ebx, [esp + 32 + 24]  
    sar       eax, 16
    movzx     eax, byte ptr [edx + eax]
    movq      mm2, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    add       ebp, 8
 scaleend :
    sub       ecx, 2
    jns       scaleloop

    and       ecx, 1  
    jz        scaledone

    mov       eax, ebx
    sar       eax, 17
    movzx     eax, byte ptr [edi + eax]
    movq      mm0, [kCoefficientsRgbU + 8 * eax]
    mov       eax, ebx
    sar       eax, 17
    movzx     eax, byte ptr [esi + eax]
    paddsw    mm0, [kCoefficientsRgbV + 8 * eax]
    mov       eax, ebx
    sar       eax, 16
    movzx     eax, byte ptr [edx + eax]
    movq      mm1, [kCoefficientsRgbY + 8 * eax]
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    movd      [ebp], mm1

 scaledone :
    popad
    ret
  }
}

__declspec(naked)
void LinearScaleYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width,
                              int source_dx) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]  
    mov       edi, [esp + 32 + 8]  
                
    mov       ebp, [esp + 32 + 16] 
    mov       ecx, [esp + 32 + 20] 
    imul      ecx, [esp + 32 + 24] 
    mov       [esp + 32 + 20], ecx 
    mov       ecx, [esp + 32 + 24] 
    xor       ebx, ebx             
    cmp       ecx, 0x20000
    jl        lscaleend
    mov       ebx, 0x8000          
    jmp       lscaleend
lscaleloop:
    mov       eax, ebx
    sar       eax, 0x11

    movzx     ecx, byte ptr [edi + eax]
    movzx     esi, byte ptr [edi + eax + 1]
    mov       eax, ebx
    and       eax, 0x1fffe
    imul      esi, eax
    xor       eax, 0x1fffe
    imul      ecx, eax
    add       ecx, esi
    shr       ecx, 17
    movq      mm0, [kCoefficientsRgbU + 8 * ecx]

    mov       esi, [esp + 32 + 12]
    mov       eax, ebx
    sar       eax, 0x11

    movzx     ecx, byte ptr [esi + eax]
    movzx     esi, byte ptr [esi + eax + 1]
    mov       eax, ebx
    and       eax, 0x1fffe
    imul      esi, eax
    xor       eax, 0x1fffe
    imul      ecx, eax
    add       ecx, esi
    shr       ecx, 17
    paddsw    mm0, [kCoefficientsRgbV + 8 * ecx]

    mov       eax, ebx
    sar       eax, 0x10
    movzx     ecx, byte ptr [edx + eax]
    movzx     esi, byte ptr [1 + edx + eax]
    mov       eax, ebx
    add       ebx, [esp + 32 + 24]
    and       eax, 0xffff
    imul      esi, eax
    xor       eax, 0xffff
    imul      ecx, eax
    add       ecx, esi
    shr       ecx, 16
    movq      mm1, [kCoefficientsRgbY + 8 * ecx]

    cmp       ebx, [esp + 32 + 20]
    jge       lscalelastpixel

    mov       eax, ebx
    sar       eax, 0x10
    movzx     ecx, byte ptr [edx + eax]
    movzx     esi, byte ptr [edx + eax + 1]
    mov       eax, ebx
    add       ebx, [esp + 32 + 24]
    and       eax, 0xffff
    imul      esi, eax
    xor       eax, 0xffff
    imul      ecx, eax
    add       ecx, esi
    shr       ecx, 16
    movq      mm2, [kCoefficientsRgbY + 8 * ecx]

    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 0x6
    psraw     mm2, 0x6
    packuswb  mm1, mm2
    movntq    [ebp], mm1
    add       ebp, 0x8

lscaleend:
    cmp       ebx, [esp + 32 + 20]
    jl        lscaleloop
    popad
    ret

lscalelastpixel:
    paddsw    mm1, mm0
    psraw     mm1, 6
    packuswb  mm1, mm1
    movd      [ebp], mm1
    popad
    ret
  };
}
#else 
void FastConvertYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width) {
  FastConvertYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, 1);
}

void ScaleYUVToRGB32Row(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        int width,
                        int source_dx) {
  ScaleYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, source_dx);
}

void LinearScaleYUVToRGB32Row(const uint8* y_buf,
                              const uint8* u_buf,
                              const uint8* v_buf,
                              uint8* rgb_buf,
                              int width,
                              int source_dx) {
  LinearScaleYUVToRGB32Row_C(y_buf, u_buf, v_buf, rgb_buf, width, source_dx);
}
#endif
}  

