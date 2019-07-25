#include "nscore.h"
#include "nsAlgorithm.h"
#include <emmintrin.h>
#include <nsUTF8Utils.h>

void
LossyConvertEncoding16to8::write_sse2(const PRUnichar* aSource,
                                      PRUint32 aSourceLength)
{
  char* dest = mDestination;

  
  PRUint32 i = 0;
  PRUint32 alignLen =
    NS_MIN<PRUint32>(aSourceLength, PRUint32(-NS_PTR_TO_INT32(aSource) & 0xf) / sizeof(PRUnichar));
  for (; i < alignLen; i++) {
    dest[i] = static_cast<unsigned char>(aSource[i]);
  }

  
  __m128i vectmask = _mm_set1_epi16(0x00ff);
  for (; aSourceLength - i > 31; i += 32) {
    __m128i source1 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i));
    source1 = _mm_and_si128(source1, vectmask);

    __m128i source2 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i + 8));
    source2 = _mm_and_si128(source2, vectmask);

    __m128i source3 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i + 16));
    source3 = _mm_and_si128(source3, vectmask);

    __m128i source4 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i + 24));
    source4 = _mm_and_si128(source4, vectmask);


    
    
    
    
    __m128i packed1 = _mm_packus_epi16(source1, source2);
    __m128i packed2 = _mm_packus_epi16(source3, source4);

    
    
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i),      packed1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i + 16), packed2);
  }

  
  for (; i < aSourceLength; i++) {
    dest[i] = static_cast<unsigned char>(aSource[i]);
  }

  mDestination += i;
}

void
LossyConvertEncoding8to16::write_sse2(const char* aSource,
                                      PRUint32 aSourceLength)
{
  PRUnichar *dest = mDestination;

  
  
  
  
  PRUint32 i = 0;
  PRUint32 alignLen = NS_MIN(aSourceLength, PRUint32(-NS_PTR_TO_INT32(aSource) & 0xf));
  for (; i < alignLen; i++) {
    dest[i] = static_cast<unsigned char>(aSource[i]);
  }

  
  for (; aSourceLength - i > 31; i += 32) {
    __m128i source1 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i));
    __m128i source2 = _mm_load_si128(reinterpret_cast<const __m128i*>(aSource + i + 16));

    
    __m128i lo1 = _mm_unpacklo_epi8(source1, _mm_setzero_si128());
    __m128i hi1 = _mm_unpackhi_epi8(source1, _mm_setzero_si128());
    __m128i lo2 = _mm_unpacklo_epi8(source2, _mm_setzero_si128());
    __m128i hi2 = _mm_unpackhi_epi8(source2, _mm_setzero_si128());

    
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i),      lo1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i + 8),  hi1);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i + 16), lo2);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dest + i + 24), hi2);
  }

  
  for (; i < aSourceLength; i++) {
    dest[i] = static_cast<unsigned char>(aSource[i]);
  }

  mDestination += i;
}
