



#include "Blur.h"

#include "SSEHelpers.h"

#include <string.h>

namespace mozilla {
namespace gfx {

MOZ_ALWAYS_INLINE
__m128i Divide(__m128i aValues, __m128i aDivisor)
{
  const __m128i mask = _mm_setr_epi32(0x0, 0xffffffff, 0x0, 0xffffffff);
  static const union {
    int64_t i64[2];
    __m128i m;
  } roundingAddition = { { int64_t(1) << 31, int64_t(1) << 31 } };

  __m128i multiplied31 = _mm_mul_epu32(aValues, aDivisor);
  __m128i multiplied42 = _mm_mul_epu32(_mm_srli_epi64(aValues, 32), aDivisor);

  
  
  __m128i p_3_1 = _mm_srli_epi64(_mm_add_epi64(multiplied31, roundingAddition.m), 32);
  __m128i p4_2_ = _mm_and_si128(_mm_add_epi64(multiplied42, roundingAddition.m), mask);
  __m128i p4321 = _mm_or_si128(p_3_1, p4_2_);
  return p4321;
}

MOZ_ALWAYS_INLINE
__m128i BlurFourPixels(const __m128i& aTopLeft, const __m128i& aTopRight,
                       const __m128i& aBottomRight, const __m128i& aBottomLeft,
                       const __m128i& aDivisor)
{
  __m128i values = _mm_add_epi32(_mm_sub_epi32(_mm_sub_epi32(aBottomRight, aTopRight), aBottomLeft), aTopLeft);
  return Divide(values, aDivisor);
}

MOZ_ALWAYS_INLINE
void LoadIntegralRowFromRow(uint32_t *aDest, const uint8_t *aSource,
                            int32_t aSourceWidth, int32_t aLeftInflation,
                            int32_t aRightInflation)
{
  int32_t currentRowSum = 0;

  for (int x = 0; x < aLeftInflation; x++) {
    currentRowSum += aSource[0];
    aDest[x] = currentRowSum;
  }
  for (int x = aLeftInflation; x < (aSourceWidth + aLeftInflation); x++) {
    currentRowSum += aSource[(x - aLeftInflation)];
    aDest[x] = currentRowSum;
  }
  for (int x = (aSourceWidth + aLeftInflation); x < (aSourceWidth + aLeftInflation + aRightInflation); x++) {
    currentRowSum += aSource[aSourceWidth - 1];
    aDest[x] = currentRowSum;
  }
}





MOZ_ALWAYS_INLINE
__m128i AccumulatePixelSums(__m128i aPixels)
{
  __m128i sumPixels = aPixels;
  __m128i currentPixels = _mm_slli_si128(aPixels, 4);
  sumPixels = _mm_add_epi32(sumPixels, currentPixels);
  currentPixels = _mm_unpacklo_epi64(_mm_setzero_si128(), sumPixels);

  return _mm_add_epi32(sumPixels, currentPixels);
}

MOZ_ALWAYS_INLINE void
GenerateIntegralImage_SSE2(int32_t aLeftInflation, int32_t aRightInflation,
                           int32_t aTopInflation, int32_t aBottomInflation,
                           uint32_t *aIntegralImage, size_t aIntegralImageStride,
                           uint8_t *aSource, int32_t aSourceStride, const IntSize &aSize)
{
  MOZ_ASSERT(!(aLeftInflation & 3));

  uint32_t stride32bit = aIntegralImageStride / 4;

  IntSize integralImageSize(aSize.width + aLeftInflation + aRightInflation,
                            aSize.height + aTopInflation + aBottomInflation);

  LoadIntegralRowFromRow(aIntegralImage, aSource, aSize.width, aLeftInflation, aRightInflation);

  for (int y = 1; y < aTopInflation + 1; y++) {
    uint32_t *intRow = aIntegralImage + (y * stride32bit);
    uint32_t *intPrevRow = aIntegralImage + (y - 1) * stride32bit;
    uint32_t *intFirstRow = aIntegralImage;

    for (int x = 0; x < integralImageSize.width; x += 4) {
      __m128i firstRow = _mm_load_si128((__m128i*)(intFirstRow + x));
      __m128i previousRow = _mm_load_si128((__m128i*)(intPrevRow + x));
      _mm_store_si128((__m128i*)(intRow + x), _mm_add_epi32(firstRow, previousRow));
    }
  }

  for (int y = aTopInflation + 1; y < (aSize.height + aTopInflation); y++) {
    __m128i currentRowSum = _mm_setzero_si128();
    uint32_t *intRow = aIntegralImage + (y * stride32bit);
    uint32_t *intPrevRow = aIntegralImage + (y - 1) * stride32bit;
    uint8_t *sourceRow = aSource + aSourceStride * (y - aTopInflation);

    uint32_t pixel = sourceRow[0];
    for (int x = 0; x < aLeftInflation; x += 4) {
      __m128i sumPixels = AccumulatePixelSums(_mm_shuffle_epi32(_mm_set1_epi32(pixel), _MM_SHUFFLE(0, 0, 0, 0)));

      sumPixels = _mm_add_epi32(sumPixels, currentRowSum);

      currentRowSum = _mm_shuffle_epi32(sumPixels, _MM_SHUFFLE(3, 3, 3, 3));

      _mm_store_si128((__m128i*)(intRow + x), _mm_add_epi32(sumPixels, _mm_load_si128((__m128i*)(intPrevRow + x))));
    }
    for (int x = aLeftInflation; x < (aSize.width + aLeftInflation); x += 4) {
      uint32_t pixels = *(uint32_t*)(sourceRow + (x - aLeftInflation));

      
      
      
      
      currentRowSum = _mm_shuffle_epi32(currentRowSum, _MM_SHUFFLE(3, 3, 3, 3));

      __m128i sumPixels = AccumulatePixelSums(_mm_unpacklo_epi16(_mm_unpacklo_epi8( _mm_set1_epi32(pixels), _mm_setzero_si128()), _mm_setzero_si128()));
      sumPixels = _mm_add_epi32(sumPixels, currentRowSum);

      currentRowSum = sumPixels;

      _mm_store_si128((__m128i*)(intRow + x), _mm_add_epi32(sumPixels, _mm_load_si128((__m128i*)(intPrevRow + x))));
    }

    pixel = sourceRow[aSize.width - 1];
    int x = (aSize.width + aLeftInflation);
    if ((aSize.width & 3)) {
      
      
      uint32_t intCurrentRowSum = ((uint32_t*)&currentRowSum)[(aSize.width % 4) - 1];
      for (; x < integralImageSize.width; x++) {
        
        if (!(x & 3)) {
          
          currentRowSum = _mm_set1_epi32(intCurrentRowSum);
          break;
        }
        intCurrentRowSum += pixel;
        intRow[x] = intPrevRow[x] + intCurrentRowSum;
      }
    } else {
      currentRowSum = _mm_shuffle_epi32(currentRowSum, _MM_SHUFFLE(3, 3, 3, 3));
    }
    for (; x < integralImageSize.width; x += 4) {
      __m128i sumPixels = AccumulatePixelSums(_mm_set1_epi32(pixel));

      sumPixels = _mm_add_epi32(sumPixels, currentRowSum);

      currentRowSum = _mm_shuffle_epi32(sumPixels, _MM_SHUFFLE(3, 3, 3, 3));

      _mm_store_si128((__m128i*)(intRow + x), _mm_add_epi32(sumPixels, _mm_load_si128((__m128i*)(intPrevRow + x))));
    }
  }

  if (aBottomInflation) {
    
    
    
    LoadIntegralRowFromRow(aIntegralImage + (integralImageSize.height - 1) * stride32bit,
                           aSource + (aSize.height - 1) * aSourceStride, aSize.width, aLeftInflation, aRightInflation);


    for (int y = aSize.height + aTopInflation; y < integralImageSize.height; y++) {
      __m128i *intRow = (__m128i*)(aIntegralImage + (y * stride32bit));
      __m128i *intPrevRow = (__m128i*)(aIntegralImage + (y - 1) * stride32bit);
      __m128i *intLastRow = (__m128i*)(aIntegralImage + (integralImageSize.height - 1) * stride32bit);

      for (int x = 0; x < integralImageSize.width; x += 4) {
        _mm_store_si128(intRow + (x / 4),
                        _mm_add_epi32(_mm_load_si128(intLastRow + (x / 4)),
                                      _mm_load_si128(intPrevRow + (x / 4))));
      }
    }
  }
}




void
AlphaBoxBlur::BoxBlur_SSE2(uint8_t* aData,
                           int32_t aLeftLobe,
                           int32_t aRightLobe,
                           int32_t aTopLobe,
                           int32_t aBottomLobe,
                           uint32_t *aIntegralImage,
                           size_t aIntegralImageStride)
{
  IntSize size = GetSize();

  MOZ_ASSERT(size.height > 0);

  
  
  
  aLeftLobe++;
  aTopLobe++;
  int32_t boxSize = (aLeftLobe + aRightLobe) * (aTopLobe + aBottomLobe);

  MOZ_ASSERT(boxSize > 0);

  if (boxSize == 1) {
      return;
  }

  uint32_t reciprocal = uint32_t((uint64_t(1) << 32) / boxSize);

  uint32_t stride32bit = aIntegralImageStride / 4;
  int32_t leftInflation = RoundUpToMultipleOf4(aLeftLobe).value();

  GenerateIntegralImage_SSE2(leftInflation, aRightLobe, aTopLobe, aBottomLobe,
                             aIntegralImage, aIntegralImageStride, aData,
                             mStride, size);

  __m128i divisor = _mm_set1_epi32(reciprocal);

  
  
  uint32_t *innerIntegral = aIntegralImage + (aTopLobe * stride32bit) + leftInflation;

  IntRect skipRect = mSkipRect;
  int32_t stride = mStride;
  uint8_t *data = aData;
  for (int32_t y = 0; y < size.height; y++) {
    bool inSkipRectY = y > skipRect.y && y < skipRect.YMost();

    uint32_t *topLeftBase = innerIntegral + ((y - aTopLobe) * ptrdiff_t(stride32bit) - aLeftLobe);
    uint32_t *topRightBase = innerIntegral + ((y - aTopLobe) * ptrdiff_t(stride32bit) + aRightLobe);
    uint32_t *bottomRightBase = innerIntegral + ((y + aBottomLobe) * ptrdiff_t(stride32bit) + aRightLobe);
    uint32_t *bottomLeftBase = innerIntegral + ((y + aBottomLobe) * ptrdiff_t(stride32bit) - aLeftLobe);

    int32_t x = 0;
    
    for (; x <= size.width - 16; x += 16) {
      if (inSkipRectY && x > skipRect.x && x < skipRect.XMost()) {
        x = skipRect.XMost() - 16;
        
        
        inSkipRectY = false;
        continue;
      }

      __m128i topLeft;
      __m128i topRight;
      __m128i bottomRight;
      __m128i bottomLeft;

      topLeft = loadUnaligned128((__m128i*)(topLeftBase + x));
      topRight = loadUnaligned128((__m128i*)(topRightBase + x));
      bottomRight = loadUnaligned128((__m128i*)(bottomRightBase + x));
      bottomLeft = loadUnaligned128((__m128i*)(bottomLeftBase + x));
      __m128i result1 = BlurFourPixels(topLeft, topRight, bottomRight, bottomLeft, divisor);

      topLeft = loadUnaligned128((__m128i*)(topLeftBase + x + 4));
      topRight = loadUnaligned128((__m128i*)(topRightBase + x + 4));
      bottomRight = loadUnaligned128((__m128i*)(bottomRightBase + x + 4));
      bottomLeft = loadUnaligned128((__m128i*)(bottomLeftBase + x + 4));
      __m128i result2 = BlurFourPixels(topLeft, topRight, bottomRight, bottomLeft, divisor);

      topLeft = loadUnaligned128((__m128i*)(topLeftBase + x + 8));
      topRight = loadUnaligned128((__m128i*)(topRightBase + x + 8));
      bottomRight = loadUnaligned128((__m128i*)(bottomRightBase + x + 8));
      bottomLeft = loadUnaligned128((__m128i*)(bottomLeftBase + x + 8));
      __m128i result3 = BlurFourPixels(topLeft, topRight, bottomRight, bottomLeft, divisor);

      topLeft = loadUnaligned128((__m128i*)(topLeftBase + x + 12));
      topRight = loadUnaligned128((__m128i*)(topRightBase + x + 12));
      bottomRight = loadUnaligned128((__m128i*)(bottomRightBase + x + 12));
      bottomLeft = loadUnaligned128((__m128i*)(bottomLeftBase + x + 12));
      __m128i result4 = BlurFourPixels(topLeft, topRight, bottomRight, bottomLeft, divisor);

      __m128i final = _mm_packus_epi16(_mm_packs_epi32(result1, result2), _mm_packs_epi32(result3, result4));

      _mm_storeu_si128((__m128i*)(data + stride * y + x), final);
    }

    
    for (; x < size.width; x += 4) {
      if (inSkipRectY && x > skipRect.x && x < skipRect.XMost()) {
        x = skipRect.XMost() - 4;
        
        
        inSkipRectY = false;
        continue;
      }
      __m128i topLeft = loadUnaligned128((__m128i*)(topLeftBase + x));
      __m128i topRight = loadUnaligned128((__m128i*)(topRightBase + x));
      __m128i bottomRight = loadUnaligned128((__m128i*)(bottomRightBase + x));
      __m128i bottomLeft = loadUnaligned128((__m128i*)(bottomLeftBase + x));

      __m128i result = BlurFourPixels(topLeft, topRight, bottomRight, bottomLeft, divisor);
      __m128i final = _mm_packus_epi16(_mm_packs_epi32(result, _mm_setzero_si128()), _mm_setzero_si128());

      *(uint32_t*)(data + stride * y + x) = _mm_cvtsi128_si32(final);
    }
  }

}

} 
} 
