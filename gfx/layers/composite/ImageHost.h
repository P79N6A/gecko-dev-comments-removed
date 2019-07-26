




#ifndef MOZILLA_GFX_IMAGEHOST_H
#define MOZILLA_GFX_IMAGEHOST_H

#include "CompositableHost.h"
#include "mozilla/layers/LayerManagerComposite.h"

namespace mozilla {
namespace layers {







class ImageHost : public CompositableHost
{
public:
  DeprecatedTextureHost* GetDeprecatedTextureHost() MOZ_OVERRIDE { return nullptr; }

protected:
  ImageHost(const TextureInfo& aTextureInfo)
  : CompositableHost(aTextureInfo)
  {
    MOZ_COUNT_CTOR(ImageHost);
  }

  ~ImageHost()
  {
    MOZ_COUNT_DTOR(ImageHost);
  }
};


class ImageHostSingle : public ImageHost
{
public:
  ImageHostSingle(const TextureInfo& aTextureInfo)
    : ImageHost(aTextureInfo)
    , mDeprecatedTextureHost(nullptr)
    , mHasPictureRect(false)
  {}

  virtual CompositableType GetType() { return mTextureInfo.mCompositableType; }

  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo) MOZ_OVERRIDE;

  DeprecatedTextureHost* GetDeprecatedTextureHost() MOZ_OVERRIDE { return mDeprecatedTextureHost; }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Point& aOffset,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr);

  virtual bool Update(const SurfaceDescriptor& aImage,
                      SurfaceDescriptor* aResult = nullptr) MOZ_OVERRIDE
  {
    return ImageHost::Update(aImage, aResult);
  }

  virtual void SetPictureRect(const nsIntRect& aPictureRect) MOZ_OVERRIDE
  {
    mPictureRect = aPictureRect;
    mHasPictureRect = true;
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE
  {
    if (mDeprecatedTextureHost) {
      return mDeprecatedTextureHost->GetRenderState();
    }
    return LayerRenderState();
  }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

#ifdef MOZ_DUMP_PAINTING
  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE
  {
    return mDeprecatedTextureHost->GetAsSurface();
  }
#endif

protected:
  virtual void MakeDeprecatedTextureHost(TextureIdentifier aTextureId,
                               const SurfaceDescriptor& aSurface,
                               ISurfaceAllocator* aAllocator,
                               const TextureInfo& aTextureInfo);

  RefPtr<DeprecatedTextureHost> mDeprecatedTextureHost;
  nsIntRect mPictureRect;
  bool mHasPictureRect;
};




class ImageHostBuffered : public ImageHostSingle
{
public:
  ImageHostBuffered(const TextureInfo& aTextureInfo)
    : ImageHostSingle(aTextureInfo)
  {}

  virtual bool Update(const SurfaceDescriptor& aImage,
                      SurfaceDescriptor* aResult = nullptr) MOZ_OVERRIDE;

protected:
  virtual void MakeDeprecatedTextureHost(TextureIdentifier aTextureId,
                               const SurfaceDescriptor& aSurface,
                               ISurfaceAllocator* aAllocator,
                               const TextureInfo& aTextureInfo) MOZ_OVERRIDE;
};

}
}

#endif
