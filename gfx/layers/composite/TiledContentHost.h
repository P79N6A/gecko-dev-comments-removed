




#ifndef GFX_TILEDCONTENTHOST_H
#define GFX_TILEDCONTENTHOST_H

#include <stdint.h>                     
#include <stdio.h>                      
#include <algorithm>                    
#include "ContentHost.h"                
#include "TiledLayerBuffer.h"           
#include "CompositableHost.h"
#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/layers/TiledContentClient.h"
#include "mozilla/mozalloc.h"           
#include "nsRegion.h"                   
#include "nscore.h"                     

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
#include <ui/Fence.h>
#endif

class gfxReusableSurfaceWrapper;
struct nsIntPoint;
struct nsIntRect;
struct nsIntSize;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}

namespace layers {

class Compositor;
class ISurfaceAllocator;
class Layer;
class ThebesBufferData;
struct EffectChain;


class TileHost {
public:
  
  
  
  
  TileHost()
  {}

  
  TileHost(gfxSharedReadLock* aSharedLock,
               TextureHost* aTextureHost,
               TextureHost* aTextureHostOnWhite,
               TextureSource* aSource,
               TextureSource* aSourceOnWhite)
    : mSharedLock(aSharedLock)
    , mTextureHost(aTextureHost)
    , mTextureHostOnWhite(aTextureHostOnWhite)
    , mTextureSource(aSource)
    , mTextureSourceOnWhite(aSourceOnWhite)
  {}

  TileHost(const TileHost& o) {
    mTextureHost = o.mTextureHost;
    mTextureHostOnWhite = o.mTextureHostOnWhite;
    mTextureSource = o.mTextureSource;
    mTextureSourceOnWhite = o.mTextureSourceOnWhite;
    mSharedLock = o.mSharedLock;
  }
  TileHost& operator=(const TileHost& o) {
    if (this == &o) {
      return *this;
    }
    mTextureHost = o.mTextureHost;
    mTextureHostOnWhite = o.mTextureHostOnWhite;
    mTextureSource = o.mTextureSource;
    mTextureSourceOnWhite = o.mTextureSourceOnWhite;
    mSharedLock = o.mSharedLock;
    return *this;
  }

  bool operator== (const TileHost& o) const {
    return mTextureHost == o.mTextureHost;
  }
  bool operator!= (const TileHost& o) const {
    return mTextureHost != o.mTextureHost;
  }

  bool IsPlaceholderTile() const { return mTextureHost == nullptr; }

  void ReadUnlock() {
    if (mSharedLock) {
      mSharedLock->ReadUnlock();
    }
  }

  RefPtr<gfxSharedReadLock> mSharedLock;
  CompositableTextureHostRef mTextureHost;
  CompositableTextureHostRef mTextureHostOnWhite;
  mutable CompositableTextureSourceRef mTextureSource;
  mutable CompositableTextureSourceRef mTextureSourceOnWhite;
};

class TiledLayerBufferComposite
  : public TiledLayerBuffer<TiledLayerBufferComposite, TileHost>
{
  friend class TiledLayerBuffer<TiledLayerBufferComposite, TileHost>;

public:
  typedef TiledLayerBuffer<TiledLayerBufferComposite, TileHost>::Iterator Iterator;

  TiledLayerBufferComposite();
  TiledLayerBufferComposite(ISurfaceAllocator* aAllocator,
                            const SurfaceDescriptorTiles& aDescriptor,
                            const nsIntRegion& aOldPaintedRegion,
                            Compositor* aCompositor);

  TileHost GetPlaceholderTile() const { return TileHost(); }

  
  
  const CSSToParentLayerScale& GetFrameResolution() { return mFrameResolution; }

  void ReadUnlock();

  void ReleaseTextureHosts();

  




  void Upload();

  void SetCompositor(Compositor* aCompositor);

  bool HasDoubleBufferedTiles() { return mHasDoubleBufferedTiles; }

  bool IsValid() const { return mIsValid; }

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  virtual void SetReleaseFence(const android::sp<android::Fence>& aReleaseFence);
#endif

  
  
  static void RecycleCallback(TextureHost* textureHost, void* aClosure);

protected:
  TileHost ValidateTile(TileHost aTile,
                        const nsIntPoint& aTileRect,
                        const nsIntRegion& dirtyRect);

  
  void ReleaseTile(TileHost aTile) {}

  void SwapTiles(TileHost& aTileA, TileHost& aTileB) { std::swap(aTileA, aTileB); }

  void UnlockTile(TileHost aTile) {}
  void PostValidate(const nsIntRegion& aPaintRegion) {}
private:
  CSSToParentLayerScale mFrameResolution;
  bool mHasDoubleBufferedTiles;
  bool mIsValid;
};





















class TiledContentHost : public ContentHost,
                         public TiledLayerComposer
{
public:
  explicit TiledContentHost(const TextureInfo& aTextureInfo);

protected:
  ~TiledContentHost();

public:
  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE
  {
    return LayerRenderState();
  }


  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    NS_ERROR("N/A for tiled layers");
    return false;
  }

  const nsIntRegion& GetValidLowPrecisionRegion() const
  {
    return mLowPrecisionTiledBuffer.GetValidRegion();
  }

  virtual void SetCompositor(Compositor* aCompositor)
  {
    CompositableHost::SetCompositor(aCompositor);
    mTiledBuffer.SetCompositor(aCompositor);
    mLowPrecisionTiledBuffer.SetCompositor(aCompositor);
    mOldTiledBuffer.SetCompositor(aCompositor);
    mOldLowPrecisionTiledBuffer.SetCompositor(aCompositor);
  }

  virtual bool UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) MOZ_OVERRIDE;

  void Composite(EffectChain& aEffectChain,
                 float aOpacity,
                 const gfx::Matrix4x4& aTransform,
                 const gfx::Filter& aFilter,
                 const gfx::Rect& aClipRect,
                 const nsIntRegion* aVisibleRegion = nullptr);

  virtual CompositableType GetType() { return CompositableType::CONTENT_TILED; }

  virtual TiledLayerComposer* AsTiledLayerComposer() MOZ_OVERRIDE { return this; }

  virtual void Attach(Layer* aLayer,
                      Compositor* aCompositor,
                      AttachFlags aFlags = NO_FLAGS) MOZ_OVERRIDE;

  virtual void Detach(Layer* aLayer = nullptr,
                      AttachFlags aFlags = NO_FLAGS) MOZ_OVERRIDE;

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

#if defined(MOZ_WIDGET_GONK) && ANDROID_VERSION >= 17
  



  virtual void SetReleaseFence(const android::sp<android::Fence>& aReleaseFence)
  {
    mTiledBuffer.SetReleaseFence(aReleaseFence);
    mLowPrecisionTiledBuffer.SetReleaseFence(aReleaseFence);
  }
#endif

private:

  void RenderLayerBuffer(TiledLayerBufferComposite& aLayerBuffer,
                         const gfxRGBA* aBackgroundColor,
                         EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         nsIntRegion aMaskRegion,
                         gfx::Matrix4x4 aTransform);

  
  void RenderTile(const TileHost& aTile,
                  const gfxRGBA* aBackgroundColor,
                  EffectChain& aEffectChain,
                  float aOpacity,
                  const gfx::Matrix4x4& aTransform,
                  const gfx::Filter& aFilter,
                  const gfx::Rect& aClipRect,
                  const nsIntRegion& aScreenRegion,
                  const nsIntPoint& aTextureOffset,
                  const nsIntSize& aTextureBounds);

  void EnsureTileStore() {}

  TiledLayerBufferComposite    mTiledBuffer;
  TiledLayerBufferComposite    mLowPrecisionTiledBuffer;
  TiledLayerBufferComposite    mOldTiledBuffer;
  TiledLayerBufferComposite    mOldLowPrecisionTiledBuffer;
  bool                         mPendingUpload;
  bool                         mPendingLowPrecisionUpload;
};

}
}

#endif
