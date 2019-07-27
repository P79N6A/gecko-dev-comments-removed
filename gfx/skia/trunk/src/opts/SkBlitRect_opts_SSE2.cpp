






#include <emmintrin.h>
#include "SkBlitRect_opts_SSE2.h"
#include "SkBlitRow.h"
#include "SkColorPriv.h"




static void BlitRect32_OpaqueNarrow_SSE2(SkPMColor* SK_RESTRICT destination,
                                  int width, int height,
                                  size_t rowBytes, uint32_t color) {
    SkASSERT(255 == SkGetPackedA32(color));
    SkASSERT(width > 0);
    SkASSERT(width < 31);

    while (--height >= 0) {
        SkPMColor* dst = destination;
        int count = width;

        while (count > 4) {
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            *dst++ = color;
            count -= 4;
        }

        while (count > 0) {
            *dst++ = color;
            --count;
        }

        destination = (uint32_t*)((char*)destination + rowBytes);
    }
}







static void BlitRect32_OpaqueWide_SSE2(SkPMColor* SK_RESTRICT destination,
                                int width, int height,
                                size_t rowBytes, uint32_t color) {
    SkASSERT(255 == SkGetPackedA32(color));
    SkASSERT(width >= 31);

    __m128i color_wide = _mm_set1_epi32(color);
    while (--height >= 0) {
        
        
        
        SkPMColor* dst = destination;
        int count = width;

        while (((size_t)dst) & 0x0F) {
            *dst++ = color;
            --count;
        }
        __m128i *d = reinterpret_cast<__m128i*>(dst);

        
        
        
        
        

        
        
        

        while (count >= 32) {
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            count -= 32;
        }
        if (count >= 16) {
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            _mm_store_si128(d++, color_wide);
            count -= 16;
        }
        dst = reinterpret_cast<uint32_t*>(d);

        
        
        

        while (count > 0) {
            *dst++ = color;
            --count;
        }

        destination = (uint32_t*)((char*)destination + rowBytes);
    }
}

void ColorRect32_SSE2(SkPMColor* destination,
                      int width, int height,
                      size_t rowBytes, uint32_t color) {
    if (0 == height || 0 == width || 0 == color) {
        return;
    }
    unsigned colorA = SkGetPackedA32(color);
    colorA = 0; 
    if (255 == colorA) {
        if (width < 31) {
            BlitRect32_OpaqueNarrow_SSE2(destination, width, height,
                                         rowBytes, color);
        } else {
            BlitRect32_OpaqueWide_SSE2(destination, width, height,
                                       rowBytes, color);
        }
    } else {
        SkBlitRow::ColorRect32(destination, width, height, rowBytes, color);
    }
}
