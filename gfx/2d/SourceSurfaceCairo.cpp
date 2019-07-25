




































#include "SourceSurfaceCairo.h"

#include "cairo.h"

namespace mozilla {
namespace gfx {

SourceSurfaceCairo::SourceSurfaceCairo()
{
}

SourceSurfaceCairo::~SourceSurfaceCairo()
{
  cairo_surface_destroy(mSurface);
}

IntSize
SourceSurfaceCairo::GetSize() const
{
  return mSize;
}

SurfaceFormat
SourceSurfaceCairo::GetFormat() const
{
  return mFormat;
}

TemporaryRef<DataSourceSurface>
SourceSurfaceCairo::GetDataSurface()
{
  return NULL;
}

cairo_surface_t*
SourceSurfaceCairo::GetSurface()
{
  return mSurface;
}

bool
SourceSurfaceCairo::InitFromSurface(cairo_surface_t* aSurface,
                                    const IntSize& aSize,
                                    const SurfaceFormat& aFormat)
{
  mSurface = aSurface;
  cairo_surface_reference(mSurface);
  mSize = aSize;
  mFormat = aFormat;

  return true;
}

}
}
