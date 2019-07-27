




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

  void Dump(std::stringstream& aStream) {
    aStream << "TileHost(...)"; 
  }

  void DumpTexture(std::stringstream& aStream) {
    
    CompositableHost::DumpTextureHost(aStream, mTextureHost);
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

  
  
  const CSSToParentLayerScale2D& GetFrameResolution() { return mFrameResolution; }

  void ReadUnlock();

  void ReleaseTextureHosts();

  




  void Upload();

  void SetCompositor(Compositor* aCompositor);

  bool HasDoubleBufferedTiles() { return mHasDoubleBufferedTiles; }

  bool IsValid() const { return mIsValid; }

  
  
  static void RecycleCallback(TextureHost* textureHost, void* aClosure);

protected:
  TileHost ValidateTile(TileHost aTile,
                        const gfx::IntPoint& aTileRect,
                        const nsIntRegion& dirtyRect);

  
  void ReleaseTile(TileHost aTile) {}

  void SwapTiles(TileHost& aTileA, TileHost& aTileB) { std::swap(aTileA, aTileB); }

  void UnlockTile(TileHost aTile) {}
  void PostValidate(const nsIntRegion& aPaintRegion) {}
private:
  CSSToParentLayerScale2D mFrameResolution;
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
  virtual LayerRenderState GetRenderState() override
  {
    return LayerRenderState();
  }


  virtual bool UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack) override
  {
    NS_ERROR("N/A for tiled layers");
    return false;
  }

  const nsIntRegion& GetValidLowPrecisionRegion() const override
  {
    return mLowPrecisionTiledBuffer.GetValidRegion();
  }

  const nsIntRegion& GetValidRegion() const override
  {
    return mTiledBuffer.GetValidRegion();
  }

  virtual void SetCompositor(Compositor* aCompositor) override
  {
    CompositableHost::SetCompositor(aCompositor);
    mTiledBuffer.SetCompositor(aCompositor);
    mLowPrecisionTiledBuffer.SetCompositor(aCompositor);
    mOldTiledBuffer.SetCompositor(aCompositor);
    mOldLowPrecisionTiledBuffer.SetCompositor(aCompositor);
  }

  virtual bool UseTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                                   const SurfaceDescriptorTiles& aTiledDescriptor) override;

  void Composite(EffectChain& aEffectChain,
                 float aOpacity,
                 const gfx::Matrix4x4& aTransform,
                 const gfx::Filter& aFilter,
                 const gfx::Rect& aClipRect,
                 const nsIntRegion* aVisibleRegion = nullptr) override;

  virtual CompositableType GetType() override { return CompositableType::CONTENT_TILED; }

  virtual TiledLayerComposer* AsTiledLayerComposer() override { return this; }

  virtual void Attach(Layer* aLayer,
                      Compositor* aCompositor,
                      AttachFlags aFlags = NO_FLAGS) override;

  virtual void Detach(Layer* aLayer = nullptr,
                      AttachFlags aFlags = NO_FLAGS) override;

  virtual void Dump(std::stringstream& aStream,
                    const char* aPrefix="",
                    bool aDumpHtml=false) override;

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) override;

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
                  const gfx::IntPoint& aTextureOffset,
                  const gfx::IntSize& aTextureBounds);

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
