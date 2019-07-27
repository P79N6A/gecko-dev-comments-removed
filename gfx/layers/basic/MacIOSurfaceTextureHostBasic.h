




#ifndef MOZILLA_GFX_MACIOSURFACETEXTUREHOST_BASIC_H
#define MOZILLA_GFX_MACIOSURFACETEXTUREHOST_BASIC_H

#include "mozilla/layers/BasicCompositor.h"
#include "mozilla/layers/TextureHostBasic.h"

class MacIOSurface;

namespace mozilla {
namespace layers {

class BasicCompositor;







class MacIOSurfaceTextureSourceBasic
  : public TextureSourceBasic,
    public TextureSource
{
public:
  MacIOSurfaceTextureSourceBasic(BasicCompositor* aCompositor,
                                 MacIOSurface* aSurface);
  virtual ~MacIOSurfaceTextureSourceBasic();

  virtual TextureSourceBasic* AsSourceBasic() MOZ_OVERRIDE { return this; }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;
  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;
  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) MOZ_OVERRIDE;

  virtual void DeallocateDeviceData() MOZ_OVERRIDE { }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

protected:
  RefPtr<BasicCompositor> mCompositor;
  RefPtr<MacIOSurface> mSurface;
  RefPtr<gfx::SourceSurface> mSourceSurface;
};






class MacIOSurfaceTextureHostBasic : public TextureHost
{
public:
  MacIOSurfaceTextureHostBasic(TextureFlags aFlags,
                               const SurfaceDescriptorMacIOSurface& aDescriptor);

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual bool Lock() MOZ_OVERRIDE;

  virtual gfx::SurfaceFormat GetFormat() const MOZ_OVERRIDE;

  virtual TextureSource* GetTextureSources() MOZ_OVERRIDE
  {
    return mTextureSource;
  }

  virtual TemporaryRef<gfx::DataSourceSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return nullptr; 
  }

  virtual gfx::IntSize GetSize() const MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() MOZ_OVERRIDE { return "MacIOSurfaceTextureHostBasic"; }
#endif

protected:
  RefPtr<BasicCompositor> mCompositor;
  RefPtr<MacIOSurfaceTextureSourceBasic> mTextureSource;
  RefPtr<MacIOSurface> mSurface;
};

}
}

#endif 
