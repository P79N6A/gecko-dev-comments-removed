




#ifndef MOZILLA_GFX_X11TEXTUREHOST__H
#define MOZILLA_GFX_X11TEXTUREHOST__H

#include "mozilla/layers/TextureHost.h"
#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/gfx/Types.h"

#include "gfxXlibSurface.h"

namespace mozilla {
namespace layers {


class X11TextureHost : public TextureHost
{
public:
  X11TextureHost(TextureFlags aFlags,
                 const SurfaceDescriptorX11& aDescriptor);

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

  virtual NewTextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "X11TextureHost"; }
#endif

protected:
  Compositor* mCompositor;
  RefPtr<NewTextureSource> mTextureSource;
  RefPtr<gfxXlibSurface> mSurface;
};

} 
} 

#endif 
