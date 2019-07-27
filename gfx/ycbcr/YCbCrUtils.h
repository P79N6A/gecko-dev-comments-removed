




#ifndef MOZILLA_GFX_UTILS_H_
#define MOZILLA_GFX_UTILS_H_

#include "mozilla/gfx/Types.h"
#include "ImageContainer.h"

namespace mozilla {
namespace gfx {

void
GetYCbCrToRGBDestFormatAndSize(const layers::PlanarYCbCrData& aData,
                               SurfaceFormat& aSuggestedFormat,
                               IntSize& aSuggestedSize);

void
ConvertYCbCrToRGB(const layers::PlanarYCbCrData& aData,
                  const SurfaceFormat& aDestFormat,
                  const IntSize& aDestSize,
                  unsigned char* aDestBuffer,
                  int32_t aStride);

} 
} 

#endif 
