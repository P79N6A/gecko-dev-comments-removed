



#include <xmmintrin.h>
#include <emmintrin.h>





MOZ_ALWAYS_INLINE __m128i loadUnaligned128(const __m128i *aSource)
{
  
  __m128 res = _mm_loadl_pi(_mm_set1_ps(0), (const __m64*)aSource);
  return _mm_castps_si128(_mm_loadh_pi(res, ((const __m64*)(aSource)) + 1));
}
