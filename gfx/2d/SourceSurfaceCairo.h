



































#ifndef _MOZILLA_GFX_OP_SOURCESURFACE_CAIRO_H_
#define _MOZILLA_GFX_OP_SOURCESURFACE_CAIRO_H

#include "2D.h"

namespace mozilla {
namespace gfx {

class SourceSurfaceCairo : public SourceSurface
{
public:
  SourceSurfaceCairo();
  ~SourceSurfaceCairo();

  virtual SurfaceType GetType() const { return SURFACE_CAIRO; }
  virtual IntSize GetSize() const;
  virtual SurfaceFormat GetFormat() const;
  virtual TemporaryRef<DataSourceSurface> GetDataSurface();

  cairo_surface_t* GetSurface();

  bool InitFromSurface(cairo_surface_t* aSurface,
                       const IntSize& aSize,
                       const SurfaceFormat& aFormat);

private:
  IntSize mSize;
  SurfaceFormat mFormat;
  cairo_surface_t* mSurface;
};

}
}

#endif 
