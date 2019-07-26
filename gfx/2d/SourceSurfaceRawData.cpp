




#include "SourceSurfaceRawData.h"
#include "Logging.h"

namespace mozilla {
namespace gfx {

bool
SourceSurfaceRawData::InitWrappingData(uint8_t *aData,
                                       const IntSize &aSize,
                                       int32_t aStride,
                                       SurfaceFormat aFormat,
                                       bool aOwnData)
{
  mRawData = aData;
  mSize = aSize;
  mStride = aStride;
  mFormat = aFormat;
  mOwnData = aOwnData;

  return true;
}

bool
SourceSurfaceAlignedRawData::Init(const IntSize &aSize,
                                  SurfaceFormat aFormat)
{
  mStride = GetAlignedStride<16>(aSize.width * BytesPerPixel(aFormat));
  mArray.Realloc(mStride * aSize.height);
  mSize = aSize;
  mFormat = aFormat;

  return mArray != nullptr;
}

}
}
