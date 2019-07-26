




#include "2D.h"
#include "DataSourceSurfaceWrapper.h"

namespace mozilla {
namespace gfx {

TemporaryRef<DataSourceSurface>
DataSourceSurface::GetDataSurface()
{
  RefPtr<DataSourceSurface> temp;
  if (GetType() == SURFACE_DATA) {
    temp = this;
  } else {
    temp = new DataSourceSurfaceWrapper(this);
  }
  return temp;
}

}
}
