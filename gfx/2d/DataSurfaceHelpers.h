




#pragma once

#include "2D.h"

namespace mozilla {
namespace gfx {

void
ConvertBGRXToBGRA(uint8_t* aData, const IntSize &aSize, int32_t aStride);





uint8_t*
SurfaceToPackedBGRA(DataSourceSurface *aSurface);

}
}
