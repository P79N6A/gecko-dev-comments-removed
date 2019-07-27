




#ifndef MOZILLA_GFX_DATASOURCESURFACEWRAPPER_H_
#define MOZILLA_GFX_DATASOURCESURFACEWRAPPER_H_

#include "2D.h"

namespace mozilla {
namespace gfx {



class DataSourceSurfaceWrapper : public DataSourceSurface
{
public:
  MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(DataSourceSurfaceWrapper)
  explicit DataSourceSurfaceWrapper(DataSourceSurface *aSurface)
   : mSurface(aSurface)
  {}

  virtual SurfaceType GetType() const MOZ_OVERRIDE { return SurfaceType::DATA; }

  virtual uint8_t *GetData() MOZ_OVERRIDE { return mSurface->GetData(); }
  virtual int32_t Stride() MOZ_OVERRIDE { return mSurface->Stride(); }
  virtual IntSize GetSize() const MOZ_OVERRIDE { return mSurface->GetSize(); }
  virtual SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mSurface->GetFormat(); }
  virtual bool IsValid() const MOZ_OVERRIDE { return mSurface->IsValid(); }

private:
  RefPtr<DataSourceSurface> mSurface;
};

}
}

#endif 
