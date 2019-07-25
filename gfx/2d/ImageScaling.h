




#ifndef _MOZILLA_GFX_IMAGESCALING_H
#define _MOZILLA_GFX_IMAGESCALING_H

#include "Types.h"

#include <vector>
#include "Point.h"

namespace mozilla {
namespace gfx {

class ImageHalfScaler
{
public:
  ImageHalfScaler(uint8_t *aData, int32_t aStride, const IntSize &aSize)
    : mOrigData(aData), mOrigStride(aStride), mOrigSize(aSize)
    , mDataStorage(NULL)
  {
  }

  ~ImageHalfScaler()
  {
    delete [] mDataStorage;
  }

  void ScaleForSize(const IntSize &aSize);

  uint8_t *GetScaledData() const { return mData; }
  IntSize GetSize() const { return mSize; }
  uint32_t GetStride() const { return mStride; }

private:
  void HalfImage2D(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                   uint8_t *aDest, uint32_t aDestStride);
  void HalfImageVertical(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                         uint8_t *aDest, uint32_t aDestStride);
  void HalfImageHorizontal(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                           uint8_t *aDest, uint32_t aDestStride);

  
  
  void HalfImage2D_SSE2(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                        uint8_t *aDest, uint32_t aDestStride);
  void HalfImageVertical_SSE2(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                              uint8_t *aDest, uint32_t aDestStride);
  void HalfImageHorizontal_SSE2(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                                uint8_t *aDest, uint32_t aDestStride);

  void HalfImage2D_C(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                     uint8_t *aDest, uint32_t aDestStride);
  void HalfImageVertical_C(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                           uint8_t *aDest, uint32_t aDestStride);
  void HalfImageHorizontal_C(uint8_t *aSource, int32_t aSourceStride, const IntSize &aSourceSize,
                             uint8_t *aDest, uint32_t aDestStride);

  uint8_t *mOrigData;
  int32_t mOrigStride;
  IntSize mOrigSize;

  uint8_t *mDataStorage;
  
  uint8_t *mData;
  IntSize mSize;
  
  uint32_t mStride;
};

}
}

#endif
