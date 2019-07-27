




#include "2D.h"
#include "DataSourceSurfaceWrapper.h"

namespace mozilla {
namespace gfx {

TemporaryRef<DataSourceSurface>
DataSourceSurface::GetDataSurface()
{
  RefPtr<DataSourceSurface> surface =
    (GetType() == SurfaceType::DATA) ? this : new DataSourceSurfaceWrapper(this);
  return surface.forget();
}

}
}
