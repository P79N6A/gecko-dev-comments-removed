








#include <emmintrin.h>
#include "SkBitmapProcState_opts_SSE2.h"
#include "SkUtils.h"

void S32_opaque_D32_filter_DX_SSE2(const SkBitmapProcState& s,
                                   const uint32_t* xy,
                                   int count, uint32_t* colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fDoFilter);
    SkASSERT(s.fBitmap->config() == SkBitmap::kARGB_8888_Config);
    SkASSERT(s.fAlphaScale == 256);

    const char* srcAddr = static_cast<const char*>(s.fBitmap->getPixels());
    unsigned rb = s.fBitmap->rowBytes();
    uint32_t XY = *xy++;
    unsigned y0 = XY >> 14;
    const uint32_t* row0 = reinterpret_cast<const uint32_t*>(srcAddr + (y0 >> 4) * rb);
    const uint32_t* row1 = reinterpret_cast<const uint32_t*>(srcAddr + (XY & 0x3FFF) * rb);
    unsigned subY = y0 & 0xF;

    
    __m128i sixteen = _mm_cvtsi32_si128(16);

    
    sixteen = _mm_shufflelo_epi16(sixteen, 0);

    
    __m128i allY = _mm_cvtsi32_si128(subY);

    
    allY = _mm_shufflelo_epi16(allY, 0);

    
    __m128i negY = _mm_sub_epi16(sixteen, allY);

    
    allY = _mm_unpacklo_epi64(allY, negY);

    
    sixteen = _mm_shuffle_epi32(sixteen, 0);

    
    __m128i zero = _mm_setzero_si128();
    do {
        uint32_t XX = *xy++;    
        unsigned x0 = XX >> 18;
        unsigned x1 = XX & 0x3FFF;

        
        __m128i allX = _mm_cvtsi32_si128((XX >> 14) & 0x0F);
        
        
        allX = _mm_shufflelo_epi16(allX, 0);

        
        allX = _mm_shuffle_epi32(allX, 0);

        
        __m128i negX = _mm_sub_epi16(sixteen, allX);

        
        __m128i a00 = _mm_cvtsi32_si128(row0[x0]);
        __m128i a01 = _mm_cvtsi32_si128(row0[x1]);
        __m128i a10 = _mm_cvtsi32_si128(row1[x0]);
        __m128i a11 = _mm_cvtsi32_si128(row1[x1]);

        
        __m128i a00a10 = _mm_unpacklo_epi32(a10, a00);

        
        a00a10 = _mm_unpacklo_epi8(a00a10, zero);

        
        a00a10 = _mm_mullo_epi16(a00a10, allY);

        
        a00a10 = _mm_mullo_epi16(a00a10, negX);

        
        __m128i a01a11 = _mm_unpacklo_epi32(a11, a01);

        
        a01a11 = _mm_unpacklo_epi8(a01a11, zero);

        
        a01a11 = _mm_mullo_epi16(a01a11, allY);

        
        a01a11 = _mm_mullo_epi16(a01a11, allX);

        
        __m128i sum = _mm_add_epi16(a00a10, a01a11);

        
        __m128i shifted = _mm_shuffle_epi32(sum, 0xEE);

        
        sum = _mm_add_epi16(sum, shifted);

        
        sum = _mm_srli_epi16(sum, 8);

        
        sum = _mm_packus_epi16(sum, zero);

        
        *colors++ = _mm_cvtsi128_si32(sum);
    } while (--count > 0);
}

void S32_alpha_D32_filter_DX_SSE2(const SkBitmapProcState& s,
                                  const uint32_t* xy,
                                  int count, uint32_t* colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fDoFilter);
    SkASSERT(s.fBitmap->config() == SkBitmap::kARGB_8888_Config);
    SkASSERT(s.fAlphaScale < 256);

    const char* srcAddr = static_cast<const char*>(s.fBitmap->getPixels());
    unsigned rb = s.fBitmap->rowBytes();
    uint32_t XY = *xy++;
    unsigned y0 = XY >> 14;
    const uint32_t* row0 = reinterpret_cast<const uint32_t*>(srcAddr + (y0 >> 4) * rb);
    const uint32_t* row1 = reinterpret_cast<const uint32_t*>(srcAddr + (XY & 0x3FFF) * rb);
    unsigned subY = y0 & 0xF;

    
    __m128i sixteen = _mm_cvtsi32_si128(16);

    
    sixteen = _mm_shufflelo_epi16(sixteen, 0);

    
    __m128i allY = _mm_cvtsi32_si128(subY);

    
    allY = _mm_shufflelo_epi16(allY, 0);

    
    __m128i negY = _mm_sub_epi16(sixteen, allY);

    
    allY = _mm_unpacklo_epi64(allY, negY);

    
    sixteen = _mm_shuffle_epi32(sixteen, 0);

    
    __m128i zero = _mm_setzero_si128();

    
    __m128i alpha = _mm_set1_epi16(s.fAlphaScale);

    do {
        uint32_t XX = *xy++;    
        unsigned x0 = XX >> 18;
        unsigned x1 = XX & 0x3FFF;

        
        __m128i allX = _mm_cvtsi32_si128((XX >> 14) & 0x0F);
        
        
        allX = _mm_shufflelo_epi16(allX, 0);

        
        allX = _mm_shuffle_epi32(allX, 0);

        
        __m128i negX = _mm_sub_epi16(sixteen, allX);

        
        __m128i a00 = _mm_cvtsi32_si128(row0[x0]);
        __m128i a01 = _mm_cvtsi32_si128(row0[x1]);
        __m128i a10 = _mm_cvtsi32_si128(row1[x0]);
        __m128i a11 = _mm_cvtsi32_si128(row1[x1]);

        
        __m128i a00a10 = _mm_unpacklo_epi32(a10, a00);

        
        a00a10 = _mm_unpacklo_epi8(a00a10, zero);

        
        a00a10 = _mm_mullo_epi16(a00a10, allY);

        
        a00a10 = _mm_mullo_epi16(a00a10, negX);

        
        __m128i a01a11 = _mm_unpacklo_epi32(a11, a01);

        
        a01a11 = _mm_unpacklo_epi8(a01a11, zero);

        
        a01a11 = _mm_mullo_epi16(a01a11, allY);

        
        a01a11 = _mm_mullo_epi16(a01a11, allX);

        
        __m128i sum = _mm_add_epi16(a00a10, a01a11);

        
        __m128i shifted = _mm_shuffle_epi32(sum, 0xEE);

        
        sum = _mm_add_epi16(sum, shifted);

        
        sum = _mm_srli_epi16(sum, 8);

        
        sum = _mm_mullo_epi16(sum, alpha);

        
        sum = _mm_srli_epi16(sum, 8);

        
        sum = _mm_packus_epi16(sum, zero);

        
        *colors++ = _mm_cvtsi128_si32(sum);
    } while (--count > 0);
}
