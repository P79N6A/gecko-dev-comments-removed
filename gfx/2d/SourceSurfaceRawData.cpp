




#include "SourceSurfaceRawData.h"
#include "Logging.h"
#include "Tools.h"

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

}
}
