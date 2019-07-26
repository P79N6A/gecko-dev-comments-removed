




#include "2D.h"
#include "DataSourceSurfaceWrapper.h"

namespace mozilla {
namespace gfx {

TemporaryRef<DataSourceSurface>
DataSourceSurface::GetDataSurface()
{
  return (GetType() == SurfaceType::DATA) ? this : new DataSourceSurfaceWrapper(this);
}

}
}
