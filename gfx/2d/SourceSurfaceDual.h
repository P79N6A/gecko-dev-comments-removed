



     
#ifndef MOZILLA_GFX_SOURCESURFACEDUAL_H_
#define MOZILLA_GFX_SOURCESURFACEDUAL_H_
     
#include "2D.h"
     
namespace mozilla {
namespace gfx {

class DualSurface;
class DualPattern;

class SourceSurfaceDual : public SourceSurface
{
public:
  SourceSurfaceDual(DrawTarget *aDTA, DrawTarget *aDTB)
    : mA(aDTA->Snapshot())
    , mB(aDTB->Snapshot())
  { }

  virtual SurfaceType GetType() const { return SURFACE_DUAL_DT; }
  virtual IntSize GetSize() const { return mA->GetSize(); }
  virtual SurfaceFormat GetFormat() const { return mA->GetFormat(); }

  
  virtual TemporaryRef<DataSourceSurface> GetDataSurface() { return NULL; }
private:
  friend class DualSurface;
  friend class DualPattern;

  RefPtr<SourceSurface> mA;
  RefPtr<SourceSurface> mB;
};

}
}

#endif 
