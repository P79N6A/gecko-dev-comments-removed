




#include "ImageScaling.h"
#include "2D.h"
#include "DataSurfaceHelpers.h"

#include <math.h>
#include <algorithm>

using namespace std;

namespace mozilla {
namespace gfx {

inline uint32_t Avg2x2(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  
  uint32_t sum = a ^ b ^ c;
  uint32_t carry = (a & b) | (a & c) | (b & c);

  
  uint32_t mask = 0xfefefefe;

  
  sum = (((sum ^ d) & mask) >> 1) + (sum & d);

  
  return (((sum ^ carry) & mask) >> 1) + (sum & carry);
}

inline uint32_t Avg2(uint32_t a, uint32_t b)
{
  
  uint32_t sum = a ^ b;
  uint32_t carry = (a & b);

  
  uint32_t mask = 0xfefefefe;

  
  return ((sum & mask) >> 1) + carry;
}

void
ImageHalfScaler::ScaleForSize(const IntSize &aSize)
{
  uint32_t horizontalDownscales = 0;
  uint32_t verticalDownscales = 0;

  IntSize scaleSize = mOrigSize;
  while ((scaleSize.height / 2) > aSize.height) {
    verticalDownscales++;
    scaleSize.height /= 2;
  }

  while ((scaleSize.width / 2) > aSize.width) {
    horizontalDownscales++;
    scaleSize.width /= 2;
  }

  if (scaleSize == mOrigSize) {
    return;
  }

  IntSize internalSurfSize;

  internalSurfSize.width = max(scaleSize.width, mOrigSize.width / 2);
  internalSurfSize.height = max(scaleSize.height, mOrigSize.height / 2);

  mStride = internalSurfSize.width * 4;
  if (mStride % 16) {
    mStride += 16 - (mStride % 16);
  }

  delete [] mDataStorage;
  
  
  size_t bufLen = BufferSizeFromStrideAndHeight(mStride, internalSurfSize.height, 15);
  if (bufLen == 0) {
    mSize.SizeTo(0, 0);
    mDataStorage = nullptr;
    return;
  }
  mDataStorage = new uint8_t[bufLen];

  if (uintptr_t(mDataStorage) % 16) {
    
    mData = (uint8_t*)(uintptr_t(mDataStorage) +
      (16 - (uintptr_t(mDataStorage) % 16)));
  } else {
    mData = mDataStorage;
  }

  mSize = scaleSize;

  




  IntSize currentSampledSize = mOrigSize;
  uint32_t currentSampledStride = mOrigStride;
  uint8_t *currentSampledData = mOrigData;
  
  while (verticalDownscales && horizontalDownscales) {
    if (currentSampledSize.width % 2) {
      currentSampledSize.width -= 1;
    }
    if (currentSampledSize.height % 2) {
      currentSampledSize.height -= 1;
    }

    HalfImage2D(currentSampledData, currentSampledStride, currentSampledSize,
                mData, mStride);

    verticalDownscales--;
    horizontalDownscales--;
    currentSampledSize.width /= 2;
    currentSampledSize.height /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }

  while (verticalDownscales) {
    if (currentSampledSize.height % 2) {
      currentSampledSize.height -= 1;
    }

    HalfImageVertical(currentSampledData, currentSampledStride, currentSampledSize,
                      mData, mStride);

    verticalDownscales--;
    currentSampledSize.height /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }


  while (horizontalDownscales) {
    if (currentSampledSize.width % 2) {
      currentSampledSize.width -= 1;
    }

    HalfImageHorizontal(currentSampledData, currentSampledStride, currentSampledSize,
                        mData, mStride);

    horizontalDownscales--;
    currentSampledSize.width /= 2;
    currentSampledData = mData;
    currentSampledStride = mStride;
  }
}

void
ImageHalfScaler::HalfImage2D(uint8_t *aSource, int32_t aSourceStride,
                             const IntSize &aSourceSize, uint8_t *aDest,
                             uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImage2D_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImage2D_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImageVertical(uint8_t *aSource, int32_t aSourceStride,
                                   const IntSize &aSourceSize, uint8_t *aDest,
                                   uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImageVertical_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImageVertical_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImageHorizontal(uint8_t *aSource, int32_t aSourceStride,
                                     const IntSize &aSourceSize, uint8_t *aDest,
                                     uint32_t aDestStride)
{
#ifdef USE_SSE2
  if (Factory::HasSSE2()) {
    HalfImageHorizontal_SSE2(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  } else
#endif
  {
    HalfImageHorizontal_C(aSource, aSourceStride, aSourceSize, aDest, aDestStride);
  }
}

void
ImageHalfScaler::HalfImage2D_C(uint8_t *aSource, int32_t aSourceStride,
                               const IntSize &aSourceSize, uint8_t *aDest,
                               uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y += 2) {
    uint32_t *storage = (uint32_t*)(aDest + (y / 2) * aDestStride);
    for (int x = 0; x < aSourceSize.width; x += 2) {
      uint8_t *upperRow = aSource + (y * aSourceStride + x * 4);
      uint8_t *lowerRow = aSource + ((y + 1) * aSourceStride + x * 4);

      *storage++ = Avg2x2(*(uint32_t*)upperRow, *((uint32_t*)upperRow + 1),
                          *(uint32_t*)lowerRow, *((uint32_t*)lowerRow + 1));
    }
  }
}

void
ImageHalfScaler::HalfImageVertical_C(uint8_t *aSource, int32_t aSourceStride,
                                     const IntSize &aSourceSize, uint8_t *aDest,
                                     uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y += 2) {
    uint32_t *storage = (uint32_t*)(aDest + (y / 2) * aDestStride);
    for (int x = 0; x < aSourceSize.width; x++) {
      uint32_t *upperRow = (uint32_t*)(aSource + (y * aSourceStride + x * 4));
      uint32_t *lowerRow = (uint32_t*)(aSource + ((y + 1) * aSourceStride + x * 4));

      *storage++ = Avg2(*upperRow, *lowerRow);
    }
  }
}

void
ImageHalfScaler::HalfImageHorizontal_C(uint8_t *aSource, int32_t aSourceStride,
                                       const IntSize &aSourceSize, uint8_t *aDest,
                                       uint32_t aDestStride)
{
  for (int y = 0; y < aSourceSize.height; y++) {
    uint32_t *storage = (uint32_t*)(aDest + y * aDestStride);
    for (int x = 0; x < aSourceSize.width;  x+= 2) {
      uint32_t *pixels = (uint32_t*)(aSource + (y * aSourceStride + x * 4));

      *storage++ = Avg2(*pixels, *(pixels + 1));
    }
  }
}

} 
} 
