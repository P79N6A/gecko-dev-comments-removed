




#ifndef __NS_SVGMASKFRAMENEON_H__
#define __NS_SVGMASKFRAMENEON_H__

#include "mozilla/gfx/2D.h"

using namespace mozilla::gfx;

void
ComputesRGBLuminanceMask_NEON(uint8_t *aData,
                              int32_t aStride,
                              const IntSize &aSize,
                              float aOpacity);

#endif 
