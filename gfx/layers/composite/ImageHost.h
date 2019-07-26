




#ifndef MOZILLA_GFX_IMAGEHOST_H
#define MOZILLA_GFX_IMAGEHOST_H

#include <stdio.h>                      
#include "CompositableHost.h"           
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nsCOMPtr.h"                   
#include "nsRect.h"                     
#include "nscore.h"                     
 
class gfxImageSurface;
class nsIntRegion;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
namespace layers {

class Compositor;
class ISurfaceAllocator;
struct EffectChain;




class ImageHost : public CompositableHost
{
public:
  ImageHost(const TextureInfo& aTextureInfo);
  ~ImageHost();

  virtual CompositableType GetType() { return mTextureInfo.mCompositableType; }

  virtual void Composite(EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Matrix4x4& aTransform,
                         const gfx::Point& aOffset,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion* aVisibleRegion = nullptr,
                         TiledLayerProperties* aLayerProperties = nullptr) MOZ_OVERRIDE;

  virtual void UseTextureHost(TextureHost* aTexture) MOZ_OVERRIDE;

  virtual void RemoveTextureHost(uint64_t aTextureID) MOZ_OVERRIDE;

  virtual TextureHost* GetTextureHost() MOZ_OVERRIDE;

  virtual void SetPictureRect(const nsIntRect& aPictureRect) MOZ_OVERRIDE
  {
    mPictureRect = aPictureRect;
    mHasPictureRect = true;
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void OnActorDestroy() MOZ_OVERRIDE
  {
    if (mFrontBuffer) {
      mFrontBuffer->OnActorDestroy();
    }
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=NULL,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;
#endif

protected:

  RefPtr<TextureHost> mFrontBuffer;
  nsIntRect mPictureRect;
  bool mHasPictureRect;
};


class DeprecatedImageHostSingle : public CompositableHost
{
public:
  DeprecatedImageHostSingle(const TextureInfo& aTextureInfo)
    : CompositableHost(aTextureInfo)
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
    return CompositableHost::Update(aImage, aResult);
  }

  virtual void SetPictureRect(const nsIntRect& aPictureRect) MOZ_OVERRIDE
  {
    mPictureRect = aPictureRect;
    mHasPictureRect = true;
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE;

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE;

  virtual void OnActorDestroy() MOZ_OVERRIDE
  {
    if (mDeprecatedTextureHost) {
      mDeprecatedTextureHost->OnActorDestroy();
    }
  }

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxImageSurface> GetAsSurface() MOZ_OVERRIDE;
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




class DeprecatedImageHostBuffered : public DeprecatedImageHostSingle
{
public:
  DeprecatedImageHostBuffered(const TextureInfo& aTextureInfo)
    : DeprecatedImageHostSingle(aTextureInfo)
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
