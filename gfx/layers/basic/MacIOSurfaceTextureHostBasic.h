




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

  virtual TextureSourceBasic* AsSourceBasic() override { return this; }

  virtual gfx::IntSize GetSize() const override;
  virtual gfx::SurfaceFormat GetFormat() const override;
  virtual gfx::SourceSurface* GetSurface(gfx::DrawTarget* aTarget) override;

  virtual void DeallocateDeviceData() override { }

  virtual void SetCompositor(Compositor* aCompositor) override;

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

  virtual void SetCompositor(Compositor* aCompositor) override;

  virtual bool Lock() override;

  virtual gfx::SurfaceFormat GetFormat() const override;

  virtual bool BindTextureSource(CompositableTextureSourceRef& aTexture) override
  {
    aTexture = mTextureSource;
    return !!aTexture;
  }

  virtual already_AddRefed<gfx::DataSourceSurface> GetAsSurface() override
  {
    return nullptr; 
  }

  virtual gfx::IntSize GetSize() const override;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() override { return "MacIOSurfaceTextureHostBasic"; }
#endif

protected:
  RefPtr<BasicCompositor> mCompositor;
  RefPtr<MacIOSurfaceTextureSourceBasic> mTextureSource;
  RefPtr<MacIOSurface> mSurface;
};

} 
} 

#endif 
