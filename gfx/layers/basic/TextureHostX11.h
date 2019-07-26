




#ifndef MOZILLA_GFX_TEXTUREHOSTX11__H
#define MOZILLA_GFX_TEXTUREHOSTX11__H

#include "mozilla/layers/TextureHostBasic.h"
#include "mozilla/gfx/2D.h"

namespace mozilla {
namespace layers {

class BasicCompositor;


class TextureSourceX11
  : public TextureSourceBasic,
    public NewTextureSource
{
public:
  TextureSourceX11(BasicCompositor* aCompositor, gfxXlibSurface* aSurface);

  virtual TextureSourceX11* AsSourceBasic() MOZ_OVERRIDE { return this; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;
  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;
  virtual gfx::SourceSurface* GetSurface() MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE { }

  virtual void SetCompositor(Compositor* aCompositor);

protected:
  BasicCompositor* mCompositor;
  RefPtr<gfxXlibSurface> mSurface;
  RefPtr<gfx::SourceSurface> mSourceSurface;
};


class TextureHostX11 : public TextureHost
{
public:
  TextureHostX11(TextureFlags aFlags,
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
  virtual const char* Name() { return "TextureHostX11"; }
#endif

protected:
  BasicCompositor* mCompositor;
  RefPtr<TextureSourceX11> mTextureSource;
  RefPtr<gfxXlibSurface> mSurface;
};

} 
} 

#endif 
