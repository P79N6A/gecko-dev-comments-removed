



#ifndef MOZILLA_GFX_SCALE_H_
#define MOZILLA_GFX_SCALE_H_

#include "Types.h"

namespace mozilla {
namespace gfx {

















GFX2D_API bool Scale(uint8_t* srcData, int32_t srcWidth, int32_t srcHeight, int32_t srcStride,
                     uint8_t* dstData, int32_t dstWidth, int32_t dstHeight, int32_t dstStride,
                     SurfaceFormat format);

}
}

#endif 
