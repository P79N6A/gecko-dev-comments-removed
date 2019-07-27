




#include "2D.h"
#include "DataSourceSurfaceWrapper.h"

namespace mozilla {
namespace gfx {

already_AddRefed<DataSourceSurface>
DataSourceSurface::GetDataSurface()
{
  RefPtr<DataSourceSurface> surface =
    (GetType() == SurfaceType::DATA) ? this : new DataSourceSurfaceWrapper(this);
  return surface.forget();
}

} 
} 
