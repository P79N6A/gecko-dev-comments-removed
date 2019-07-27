




#include "ImageScaling.h"
#include "mozilla/Attributes.h"

#include "SSEHelpers.h"













































MOZ_ALWAYS_INLINE __m128i _mm_not_si128(__m128i arg)
{
  __m128i minusone = _mm_set1_epi32(0xffffffff);
  return _mm_xor_si128(arg, minusone);
}






MOZ_ALWAYS_INLINE __m128i avg_sse2_8x2(__m128i *a, __m128i *b, __m128i *c, __m128i *d)
{
#define shuf1 _MM_SHUFFLE(2, 0, 2, 0)
#define shuf2 _MM_SHUFFLE(3, 1, 3, 1)



#define shuffle_si128(arga, argb, imm) \
  _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps((arga)), _mm_castsi128_ps((argb)), (imm)));

  __m128i t = shuffle_si128(*a, *b, shuf1);
  *b = shuffle_si128(*a, *b, shuf2);
  *a = t;
  t = shuffle_si128(*c, *d, shuf1);
  *d = shuffle_si128(*c, *d, shuf2);
  *c = t;

#undef shuf1
#undef shuf2
#undef shuffle_si128

  __m128i sum = _mm_xor_si128(*a, _mm_xor_si128(*b, *c));

  __m128i carry = _mm_or_si128(_mm_and_si128(*a, *b), _mm_or_si128(_mm_and_si128(*a, *c), _mm_and_si128(*b, *c)));

  sum = _mm_avg_epu8(_mm_not_si128(sum), _mm_not_si128(*d));

  return _mm_not_si128(_mm_avg_epu8(sum, _mm_not_si128(carry)));
}

MOZ_ALWAYS_INLINE __m128i avg_sse2_4x2_4x1(__m128i a, __m128i b)
{
  return _mm_not_si128(_mm_avg_epu8(_mm_not_si128(a), _mm_not_si128(b)));
}

MOZ_ALWAYS_INLINE __m128i avg_sse2_8x1_4x1(__m128i a, __m128i b)
{
  __m128i t = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(3, 1, 3, 1)));
  b = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _MM_SHUFFLE(2, 0, 2, 0)));
  a = t;

  return _mm_not_si128(_mm_avg_epu8(_mm_not_si128(a), _mm_not_si128(b)));
}

MOZ_ALWAYS_INLINE uint32_t Avg2x2(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  uint32_t sum = a ^ b ^ c;
  uint32_t carry = (a & b) | (a & c) | (b & c);

  uint32_t mask = 0xfefefefe;

  
  
  sum = (((sum ^ d) & mask) >> 1) + (sum & d);

  return (((sum ^ carry) & mask) >> 1) + (sum & carry);
}


MOZ_ALWAYS_INLINE uint32_t Avg2(uint32_t a, uint32_t b)
{
  uint32_t sum = a ^ b;
  uint32_t carry = (a & b);

  uint32_t mask = 0xfefefefe;

  return ((sum & mask) >> 1) + carry;
}

namespace mozilla {
namespace gfx {

void
ImageHalfScaler::HalfImage2D_SSE2(uint8_t *aSource, int32_t aSourceStride,
                                  const IntSize &aSourceSize, uint8_t *aDest,
                                  uint32_t aDestStride)
{
  const int Bpp = 4;

  for (int y = 0; y < aSourceSize.height; y += 2) {
    __m128i *storage = (__m128i*)(aDest + (y / 2) * aDestStride);
    int x = 0;
    
    if (!(uintptr_t(aSource + (y * aSourceStride)) % 16) &&
        !(uintptr_t(aSource + ((y + 1) * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i *upperRow = (__m128i*)(aSource + (y * aSourceStride + x * Bpp));
        __m128i *lowerRow = (__m128i*)(aSource + ((y + 1) * aSourceStride + x * Bpp));

        __m128i a = _mm_load_si128(upperRow);
        __m128i b = _mm_load_si128(upperRow + 1);
        __m128i c = _mm_load_si128(lowerRow);
        __m128i d = _mm_load_si128(lowerRow + 1);

        *storage++ = avg_sse2_8x2(&a, &b, &c, &d);
      }
    } else if (!(uintptr_t(aSource + (y * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i *upperRow = (__m128i*)(aSource + (y * aSourceStride + x * Bpp));
        __m128i *lowerRow = (__m128i*)(aSource + ((y + 1) * aSourceStride + x * Bpp));

        __m128i a = _mm_load_si128(upperRow);
        __m128i b = _mm_load_si128(upperRow + 1);
        __m128i c = loadUnaligned128(lowerRow);
        __m128i d = loadUnaligned128(lowerRow + 1);

        *storage++ = avg_sse2_8x2(&a, &b, &c, &d);
      }
    } else if (!(uintptr_t(aSource + ((y + 1) * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i *upperRow = (__m128i*)(aSource + (y * aSourceStride + x * Bpp));
        __m128i *lowerRow = (__m128i*)(aSource + ((y + 1) * aSourceStride + x * Bpp));

        __m128i a = loadUnaligned128((__m128i*)upperRow);
        __m128i b = loadUnaligned128((__m128i*)upperRow + 1);
        __m128i c = _mm_load_si128((__m128i*)lowerRow);
        __m128i d = _mm_load_si128((__m128i*)lowerRow + 1);

        *storage++ = avg_sse2_8x2(&a, &b, &c, &d);
      }
    } else {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i *upperRow = (__m128i*)(aSource + (y * aSourceStride + x * Bpp));
        __m128i *lowerRow = (__m128i*)(aSource + ((y + 1) * aSourceStride + x * Bpp));

        __m128i a = loadUnaligned128(upperRow);
        __m128i b = loadUnaligned128(upperRow + 1);
        __m128i c = loadUnaligned128(lowerRow);
        __m128i d = loadUnaligned128(lowerRow + 1);

        *storage++ = avg_sse2_8x2(&a, &b, &c, &d);
      }
    }

    uint32_t *unalignedStorage = (uint32_t*)storage;
    
    
    
    
    
    
    
    
    
    for (; x < aSourceSize.width; x += 2) {
      uint8_t *upperRow = aSource + (y * aSourceStride + x * Bpp);
      uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * Bpp);

      *unalignedStorage++ = Avg2x2(*(uint32_t*)upperRow, *((uint32_t*)upperRow + 1),
                                   *(uint32_t*)lowerRow, *((uint32_t*)lowerRow + 1));
    }
  }
}

void
ImageHalfScaler::HalfImageVertical_SSE2(uint8_t *aSource, int32_t aSourceStride,
                                        const IntSize &aSourceSize, uint8_t *aDest,
                                        uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y += 2) {
    __m128i *storage = (__m128i*)(aDest + (y / 2) * aDestStride);
    int x = 0;
    
    if (!(uintptr_t(aSource + (y * aSourceStride)) % 16) &&
        !(uintptr_t(aSource + ((y + 1) * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 3); x += 4) {
        uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
        uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

        __m128i a = _mm_load_si128((__m128i*)upperRow);
        __m128i b = _mm_load_si128((__m128i*)lowerRow);

        *storage++ = avg_sse2_4x2_4x1(a, b);
      }
    } else if (!(uintptr_t(aSource + (y * aSourceStride)) % 16)) {
      
      for (; x < (aSourceSize.width - 3); x += 4) {
        uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
        uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

        __m128i a = _mm_load_si128((__m128i*)upperRow);
        __m128i b = loadUnaligned128((__m128i*)lowerRow);

        *storage++ = avg_sse2_4x2_4x1(a, b);
      }
    } else if (!(uintptr_t(aSource + ((y + 1) * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 3); x += 4) {
        uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
        uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

        __m128i a = loadUnaligned128((__m128i*)upperRow);
        __m128i b = _mm_load_si128((__m128i*)lowerRow);

        *storage++ = avg_sse2_4x2_4x1(a, b);
      }
    } else {
      for (; x < (aSourceSize.width - 3); x += 4) {
        uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
        uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

        __m128i a = loadUnaligned128((__m128i*)upperRow);
        __m128i b = loadUnaligned128((__m128i*)lowerRow);

        *storage++ = avg_sse2_4x2_4x1(a, b);
      }
    }

    uint32_t *unalignedStorage = (uint32_t*)storage;
    
    
    
    
    for (; x < aSourceSize.width; x++) {
      uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
      uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

      *unalignedStorage++ = Avg2(*(uint32_t*)upperRow, *(uint32_t*)lowerRow);
    }
  }
}

void
ImageHalfScaler::HalfImageHorizontal_SSE2(uint8_t *aSource, int32_t aSourceStride,
                                          const IntSize &aSourceSize, uint8_t *aDest,
                                          uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y++) {
    __m128i *storage = (__m128i*)(aDest + (y * aDestStride));
    int x = 0;
    
    if (!(uintptr_t(aSource + (y * aSourceStride)) % 16)) {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i* pixels = (__m128i*)(aSource + (y * aSourceStride + x * 4));

        __m128i a = _mm_load_si128(pixels);
        __m128i b = _mm_load_si128(pixels + 1);

        *storage++ = avg_sse2_8x1_4x1(a, b);
      }
    } else {
      for (; x < (aSourceSize.width - 7); x += 8) {
        __m128i* pixels = (__m128i*)(aSource + (y * aSourceStride + x * 4));

        __m128i a = loadUnaligned128(pixels);
        __m128i b = loadUnaligned128(pixels + 1);

        *storage++ = avg_sse2_8x1_4x1(a, b);
      }
    }

    uint32_t *unalignedStorage = (uint32_t*)storage;
    
    
    
    
    for (; x < aSourceSize.width; x += 2) {
      uint32_t *pixels = (uint32_t*)(aSource + (y * aSourceStride + x * 4));

      *unalignedStorage++ = Avg2(*pixels, *(pixels + 1));
    }
  }
}

}
}
