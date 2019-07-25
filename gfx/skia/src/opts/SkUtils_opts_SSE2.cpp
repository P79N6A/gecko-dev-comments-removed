








#include <emmintrin.h>
#include "SkUtils_opts_SSE2.h"
 
void sk_memset16_SSE2(uint16_t *dst, uint16_t value, int count)
{
    SkASSERT(dst != NULL && count >= 0);

    
    SkASSERT((((size_t) dst) & 0x01) == 0);

    if (count >= 32) {
        while (((size_t)dst) & 0x0F) {
            *dst++ = value;
            --count;
        }
        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i value_wide = _mm_set1_epi16(value);
        while (count >= 32) {
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            count -= 32;
        }
        dst = reinterpret_cast<uint16_t*>(d);
    }
    while (count > 0) {
        *dst++ = value;
        --count;
    }
}
 
void sk_memset32_SSE2(uint32_t *dst, uint32_t value, int count)
{
    SkASSERT(dst != NULL && count >= 0);

    
    SkASSERT((((size_t) dst) & 0x03) == 0);

    if (count >= 16) {
        while (((size_t)dst) & 0x0F) {
            *dst++ = value;
            --count;
        }
        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i value_wide = _mm_set1_epi32(value);
        while (count >= 16) {
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            _mm_store_si128(d++, value_wide);
            count -= 16;
        }
        dst = reinterpret_cast<uint32_t*>(d);
    }
    while (count > 0) {
        *dst++ = value;
        --count;
    }
}
