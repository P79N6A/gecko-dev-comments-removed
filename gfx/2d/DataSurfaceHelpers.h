




#ifndef _MOZILLA_GFX_DATASURFACEHELPERS_H
#define _MOZILLA_GFX_DATASURFACEHELPERS_H

#include "2D.h"

namespace mozilla {
namespace gfx {

void
ConvertBGRXToBGRA(uint8_t* aData, const IntSize &aSize, const int32_t aStride);







void
CopySurfaceDataToPackedArray(uint8_t* aSrc, uint8_t* aDst, IntSize aSrcSize,
                             int32_t aSrcStride, int32_t aBytesPerPixel);






uint8_t*
SurfaceToPackedBGRA(DataSourceSurface *aSurface);











uint8_t*
SurfaceToPackedBGR(DataSourceSurface *aSurface);







void
ClearDataSourceSurface(DataSourceSurface *aSurface);












size_t
BufferSizeFromStrideAndHeight(int32_t aStride,
                              int32_t aHeight,
                              int32_t aExtraBytes = 0);




void
CopyRect(DataSourceSurface* aSrc, DataSourceSurface* aDest,
         IntRect aSrcRect, IntPoint aDestPoint);







already_AddRefed<DataSourceSurface>
CreateDataSourceSurfaceByCloning(DataSourceSurface* aSource);




uint8_t*
DataAtOffset(DataSourceSurface* aSurface,
             DataSourceSurface::MappedSurface* aMap,
             IntPoint aPoint);






bool
SurfaceContainsPoint(SourceSurface* aSurface, const IntPoint& aPoint);

}
}

#endif 
