






#include <emmintrin.h>
#include "nscore.h"

namespace mozilla {
namespace SSE2 {

void
Convert_ascii_run(const char *&src,
                  PRUnichar  *&dst,
                  int32_t      len)
{
  if (len > 15) {
    __m128i in, out1, out2;
    __m128d *outp1, *outp2;
    __m128i zeroes;
    uint32_t offset;

    
    while ((NS_PTR_TO_UINT32(src) & 15) && len > 0) {
      if (*src & 0x80U)
        return;
      *dst++ = (PRUnichar) *src++;
      len--;
    }

    zeroes = _mm_setzero_si128();

    offset = NS_PTR_TO_UINT32(dst) & 15;

    
    
    
    

    if (offset == 0) {
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src);
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_stream_si128((__m128i *) dst, out1);
        _mm_stream_si128((__m128i *) (dst + 8), out2);
        dst += 16;
        src += 16;
        len -= 16;
      }
    } else if (offset == 8) {
      outp1 = (__m128d *) &out1;
      outp2 = (__m128d *) &out2;
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src);
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_storel_epi64((__m128i *) dst, out1);
        _mm_storel_epi64((__m128i *) (dst + 8), out2);
        _mm_storeh_pd((double *) (dst + 4), *outp1);
        _mm_storeh_pd((double *) (dst + 12), *outp2);
        src += 16;
        dst += 16;
        len -= 16;
      }
    } else {
      while (len > 15) {
        in = _mm_load_si128((__m128i *) src);
        if (_mm_movemask_epi8(in))
          break;
        out1 = _mm_unpacklo_epi8(in, zeroes);
        out2 = _mm_unpackhi_epi8(in, zeroes);
        _mm_storeu_si128((__m128i *) dst, out1);
        _mm_storeu_si128((__m128i *) (dst + 8), out2);
        src += 16;
        dst += 16;
        len -= 16;
      }
    }
  }

  

  while (len-- > 0 && (*src & 0x80U) == 0) {
    *dst++ = (PRUnichar) *src++;
  }
}

} 
} 
