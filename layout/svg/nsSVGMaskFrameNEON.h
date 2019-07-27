




#ifndef __NS_SVGMASKFRAMENEON_H__
#define __NS_SVGMASKFRAMENEON_H__

#include "mozilla/gfx/2D.h"

using namespace mozilla::gfx;

void
ComputesRGBLuminanceMask_NEON(const uint8_t *aSourceData,
                              int32_t aSourceStride,
                              uint8_t *aDestData,
                              int32_t aDestStride,
                              const IntSize &aSize,
                              float aOpacity);

#endif 
