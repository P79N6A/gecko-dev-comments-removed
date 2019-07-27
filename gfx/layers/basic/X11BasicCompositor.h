




#ifndef MOZILLA_GFX_X11BASICCOMPOSITOR_H
#define MOZILLA_GFX_X11BASICCOMPOSITOR_H

#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/X11TextureSourceBasic.h"
#include "mozilla/layers/TextureHostBasic.h"
#include "gfxXlibSurface.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {


class X11DataTextureSourceBasic : public DataTextureSource
                                , public TextureSourceBasic
{
public:
  X11DataTextureSourceBasic() {};

  virtual bool Update(gfx::DataSourceSurface* aSurface,
                      nsIntRegion* aDestRegion = nullptr,
                      gfx::IntPoint* aSrcOffset = nullptr) MOZ_OVERRIDE;

  virtual TextureSourceBasic* AsSourceBasic() MOZ_OVERRIDE;

  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

private:
  
  RefPtr<mozilla::gfx::DrawTarget> mBufferDrawTarget;
};

class X11BasicCompositor : public BasicCompositor
{
public:

  explicit X11BasicCompositor(nsIWidget *aWidget) : BasicCompositor(aWidget) {}

  virtual TemporaryRef<DataTextureSource>
  CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) MOZ_OVERRIDE;
};

} 
} 

#endif 
