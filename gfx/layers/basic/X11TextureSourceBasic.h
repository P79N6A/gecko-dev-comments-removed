




#ifndef MOZILLA_GFX_X11TEXTURESOURCEBASIC__H
#define MOZILLA_GFX_X11TEXTURESOURCEBASIC__H

#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/TextureHostBasic.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {

class BasicCompositor;


class X11TextureSourceBasic
  : public TextureSourceBasic
  , public TextureSource
{
public:
  X11TextureSourceBasic(BasicCompositor* aCompositor, gfxXlibSurface* aSurface);

  virtual X11TextureSourceBasic* AsSourceBasic() override { return this; }

  virtual gfx::IntSize GetSize() const override;

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) override;

  virtual void DeallocateDeviceData() override { }

  virtual void SetCompositor(Compositor* aCompositor) override;

  static gfx::SurfaceFormat ContentTypeToSurfaceFormat(gfxContentType aType);

protected:
  RefPtr<BasicCompositor> mCompositor;
  RefPtr<gfxXlibSurface> mSurface;
  RefPtr<gfx::SourceSurface> mSourceSurface;
};

} 
} 

#endif 
