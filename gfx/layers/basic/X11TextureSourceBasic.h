




#ifndef MOZILLA_GFX_X11TEXTURESOURCEBASIC__H
#define MOZILLA_GFX_X11TEXTURESOURCEBASIC__H

#include "mozilla/layers/TextureHostBasic.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {

class BasicCompositor;


class X11TextureSourceBasic
  : public TextureSourceBasic
  , public NewTextureSource
{
public:
  X11TextureSourceBasic(BasicCompositor* aCompositor, gfxXlibSurface* aSurface);

  virtual X11TextureSourceBasic* AsSourceBasic() MOZ_OVERRIDE { return this; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE { }

  virtual void SetCompositor(Compositor* aCompositor);

  static gfx::SurfaceFormat ContentTypeToSurfaceFormat(gfxContentType aType);

protected:
  BasicCompositor* mCompositor;
  RefPtr<gfxXlibSurface> mSurface;
  RefPtr<gfx::SourceSurface> mSourceSurface;
};

} 
} 

#endif 
