









#include "libyuv/basic_types.h"
#include "libyuv/row.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif

#if !defined(LIBYUV_DISABLE_X86) && defined(_M_IX86) && defined(_MSC_VER)

__declspec(naked)
uint32 SumSquareError_SSE2(const uint8* src_a, const uint8* src_b, int count) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    pxor       xmm0, xmm0
    pxor       xmm5, xmm5

    align      4
  wloop:
    movdqa     xmm1, [eax]
    lea        eax,  [eax + 16]
    movdqa     xmm2, [edx]
    lea        edx,  [edx + 16]
    sub        ecx, 16
    movdqa     xmm3, xmm1  
    psubusb    xmm1, xmm2
    psubusb    xmm2, xmm3
    por        xmm1, xmm2
    movdqa     xmm2, xmm1
    punpcklbw  xmm1, xmm5
    punpckhbw  xmm2, xmm5
    pmaddwd    xmm1, xmm1
    pmaddwd    xmm2, xmm2
    paddd      xmm0, xmm1
    paddd      xmm0, xmm2
    jg         wloop

    pshufd     xmm1, xmm0, 0xee
    paddd      xmm0, xmm1
    pshufd     xmm1, xmm0, 0x01
    paddd      xmm0, xmm1
    movd       eax, xmm0
    ret
  }
}


#if _MSC_VER >= 1700

#pragma warning(disable: 4752)
__declspec(naked)
uint32 SumSquareError_AVX2(const uint8* src_a, const uint8* src_b, int count) {
  __asm {
    mov        eax, [esp + 4]    
    mov        edx, [esp + 8]    
    mov        ecx, [esp + 12]   
    vpxor      ymm0, ymm0, ymm0  
    vpxor      ymm5, ymm5, ymm5  
    sub        edx, eax

    align      4
  wloop:
    vmovdqu    ymm1, [eax]
    vmovdqu    ymm2, [eax + edx]
    lea        eax,  [eax + 32]
    sub        ecx, 32
    vpsubusb   ymm3, ymm1, ymm2  
    vpsubusb   ymm2, ymm2, ymm1
    vpor       ymm1, ymm2, ymm3
    vpunpcklbw ymm2, ymm1, ymm5  
    vpunpckhbw ymm1, ymm1, ymm5
    vpmaddwd   ymm2, ymm2, ymm2  
    vpmaddwd   ymm1, ymm1, ymm1
    vpaddd     ymm0, ymm0, ymm1
    vpaddd     ymm0, ymm0, ymm2
    jg         wloop

    vpshufd    ymm1, ymm0, 0xee  
    vpaddd     ymm0, ymm0, ymm1
    vpshufd    ymm1, ymm0, 0x01  
    vpaddd     ymm0, ymm0, ymm1
    vpermq     ymm1, ymm0, 0x02  
    vpaddd     ymm0, ymm0, ymm1
    vmovd      eax, xmm0
    vzeroupper
    ret
  }
}
#endif  

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






#define pmulld(reg) _asm _emit 0x66 _asm _emit 0x0F _asm _emit 0x38 \
    _asm _emit 0x40 _asm _emit reg

__declspec(naked)
uint32 HashDjb2_SSE41(const uint8* src, int count, uint32 seed) {
  __asm {
    mov        eax, [esp + 4]    
    mov        ecx, [esp + 8]    
    movd       xmm0, [esp + 12]  

    pxor       xmm7, xmm7        
    movdqa     xmm6, kHash16x33

    align      4
  wloop:
    movdqu     xmm1, [eax]       
    lea        eax, [eax + 16]
    pmulld(0xc6)                 
    movdqa     xmm5, kHashMul0
    movdqa     xmm2, xmm1
    punpcklbw  xmm2, xmm7        
    movdqa     xmm3, xmm2
    punpcklwd  xmm3, xmm7        
    pmulld(0xdd)                 
    movdqa     xmm5, kHashMul1
    movdqa     xmm4, xmm2
    punpckhwd  xmm4, xmm7        
    pmulld(0xe5)                 
    movdqa     xmm5, kHashMul2
    punpckhbw  xmm1, xmm7        
    movdqa     xmm2, xmm1
    punpcklwd  xmm2, xmm7        
    pmulld(0xd5)                 
    movdqa     xmm5, kHashMul3
    punpckhwd  xmm1, xmm7        
    pmulld(0xcd)                 
    paddd      xmm3, xmm4        
    paddd      xmm1, xmm2
    sub        ecx, 16
    paddd      xmm1, xmm3

    pshufd     xmm2, xmm1, 0x0e  
    paddd      xmm1, xmm2
    pshufd     xmm2, xmm1, 0x01
    paddd      xmm1, xmm2
    paddd      xmm0, xmm1
    jg         wloop

    movd       eax, xmm0         
    ret
  }
}


#if _MSC_VER >= 1700
__declspec(naked)
uint32 HashDjb2_AVX2(const uint8* src, int count, uint32 seed) {
  __asm {
    mov        eax, [esp + 4]    
    mov        ecx, [esp + 8]    
    movd       xmm0, [esp + 12]  
    movdqa     xmm6, kHash16x33

    align      4
  wloop:
    vpmovzxbd  xmm3, dword ptr [eax]  
    pmulld     xmm0, xmm6  
    vpmovzxbd  xmm4, dword ptr [eax + 4]  
    pmulld     xmm3, kHashMul0
    vpmovzxbd  xmm2, dword ptr [eax + 8]  
    pmulld     xmm4, kHashMul1
    vpmovzxbd  xmm1, dword ptr [eax + 12]  
    pmulld     xmm2, kHashMul2
    lea        eax, [eax + 16]
    pmulld     xmm1, kHashMul3
    paddd      xmm3, xmm4        
    paddd      xmm1, xmm2
    sub        ecx, 16
    paddd      xmm1, xmm3
    pshufd     xmm2, xmm1, 0x0e  
    paddd      xmm1, xmm2
    pshufd     xmm2, xmm1, 0x01
    paddd      xmm1, xmm2
    paddd      xmm0, xmm1
    jg         wloop

    movd       eax, xmm0         
    ret
  }
}
#endif  

#endif  

#ifdef __cplusplus
}  
}  
#endif
