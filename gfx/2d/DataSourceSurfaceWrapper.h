




#ifndef MOZILLA_GFX_DATASOURCESURFACEWRAPPER_H_
#define MOZILLA_GFX_DATASOURCESURFACEWRAPPER_H_

#include "2D.h"

namespace mozilla {
namespace gfx {



class DataSourceSurfaceWrapper : public DataSourceSurface
{
public:
  DataSourceSurfaceWrapper(DataSourceSurface *aSurface)
   : mSurface(aSurface)
  {}

  virtual SurfaceType GetType() const MOZ_OVERRIDE { return SURFACE_DATA; }

  virtual uint8_t *GetData() MOZ_OVERRIDE { return mSurface->GetData(); }
  virtual int32_t Stride() MOZ_OVERRIDE { return mSurface->Stride(); }
  virtual IntSize GetSize() const MOZ_OVERRIDE { return mSurface->GetSize(); }
  virtual SurfaceFormat GetFormat() const MOZ_OVERRIDE { return mSurface->GetFormat(); }
  virtual bool IsValid() const MOZ_OVERRIDE { return mSurface->IsValid(); }
  virtual void MarkDirty() { mSurface->MarkDirty(); }

private:
  RefPtr<DataSourceSurface> mSurface;
};

}
}

#endif 
