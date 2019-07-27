




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
                      gfx::IntPoint* aSrcOffset = nullptr) override;

  virtual TextureSourceBasic* AsSourceBasic() override;

  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) override;

  virtual void DeallocateDeviceData() override;

  virtual gfx::IntSize GetSize() const override;

  virtual gfx::SurfaceFormat GetFormat() const override;

private:
  
  RefPtr<mozilla::gfx::DrawTarget> mBufferDrawTarget;
};

class X11BasicCompositor : public BasicCompositor
{
public:

  explicit X11BasicCompositor(nsIWidget *aWidget) : BasicCompositor(aWidget) {}

  virtual already_AddRefed<DataTextureSource>
  CreateDataTextureSource(TextureFlags aFlags = TextureFlags::NO_FLAGS) override;

  virtual void EndFrame() override;
};

} 
} 

#endif 
