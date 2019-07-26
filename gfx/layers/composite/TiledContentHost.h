




#ifndef GFX_TILEDCONTENTHOST_H
#define GFX_TILEDCONTENTHOST_H

#include <stdint.h>                     
#include <stdio.h>                      
#include <algorithm>                    
#include "ContentHost.h"                
#include "TiledLayerBuffer.h"           
#include "CompositableHost.h"
#include "gfxPoint.h"                   
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
class TiledThebesLayerComposite;
struct EffectChain;
 

class TiledTexture {
public:
  
  
  
  
  TiledTexture()
    : mDeprecatedTextureHost(nullptr)
  {}

  
  TiledTexture(DeprecatedTextureHost* aDeprecatedTextureHost)
    : mDeprecatedTextureHost(aDeprecatedTextureHost)
  {}

  TiledTexture(const TiledTexture& o) {
    mDeprecatedTextureHost = o.mDeprecatedTextureHost;
  }
  TiledTexture& operator=(const TiledTexture& o) {
    if (this == &o) {
      return *this;
    }
    mDeprecatedTextureHost = o.mDeprecatedTextureHost;
    return *this;
  }

  void Validate(gfxReusableSurfaceWrapper* aReusableSurface, Compositor* aCompositor, uint16_t aSize);

  bool operator== (const TiledTexture& o) const {
    if (!mDeprecatedTextureHost || !o.mDeprecatedTextureHost) {
      return mDeprecatedTextureHost == o.mDeprecatedTextureHost;
    }
    return *mDeprecatedTextureHost == *o.mDeprecatedTextureHost;
  }
  bool operator!= (const TiledTexture& o) const {
    if (!mDeprecatedTextureHost || !o.mDeprecatedTextureHost) {
      return mDeprecatedTextureHost != o.mDeprecatedTextureHost;
    }
    return *mDeprecatedTextureHost != *o.mDeprecatedTextureHost;
  }

  RefPtr<DeprecatedTextureHost> mDeprecatedTextureHost;
};

class TiledLayerBufferComposite
  : public TiledLayerBuffer<TiledLayerBufferComposite, TiledTexture>
{
  friend class TiledLayerBuffer<TiledLayerBufferComposite, TiledTexture>;

public:
  typedef TiledLayerBuffer<TiledLayerBufferComposite, TiledTexture>::Iterator Iterator;
  TiledLayerBufferComposite()
    : mCompositor(nullptr)
  {}

  void Upload(const BasicTiledLayerBuffer* aMainMemoryTiledBuffer,
              const nsIntRegion& aNewValidRegion,
              const nsIntRegion& aInvalidateRegion,
              const gfxSize& aResolution);

  TiledTexture GetPlaceholderTile() const { return TiledTexture(); }

  
  
  const gfxSize& GetFrameResolution() { return mFrameResolution; }

  void SetCompositor(Compositor* aCompositor)
  {
    mCompositor = aCompositor;
  }

protected:
  TiledTexture ValidateTile(TiledTexture aTile,
                            const nsIntPoint& aTileRect,
                            const nsIntRegion& dirtyRect);

  
  void ReleaseTile(TiledTexture aTile) {}

  void SwapTiles(TiledTexture& aTileA, TiledTexture& aTileB) {
    std::swap(aTileA, aTileB);
  }

private:
  Compositor* mCompositor;
  const BasicTiledLayerBuffer* mMainMemoryTiledBuffer;
  gfxSize mFrameResolution;
};






















class TiledContentHost : public ContentHost,
                         public TiledLayerComposer
{
public:
  TiledContentHost(const TextureInfo& aTextureInfo)
    : ContentHost(aTextureInfo)
    , mPendingUpload(false)
    , mPendingLowPrecisionUpload(false)
  {
    MOZ_COUNT_CTOR(TiledContentHost);
  }

  ~TiledContentHost()
  {
    MOZ_COUNT_DTOR(TiledContentHost);
  }

  virtual LayerRenderState GetRenderState() MOZ_OVERRIDE
  {
    return LayerRenderState();
  }


  virtual void UpdateThebes(const ThebesBufferData& aData,
                            const nsIntRegion& aUpdated,
                            const nsIntRegion& aOldValidRegionBack,
                            nsIntRegion* aUpdatedRegionBack)
  {
    MOZ_ASSERT(false, "N/A for tiled layers");
  }

  const nsIntRegion& GetValidLowPrecisionRegion() const
  {
    return mLowPrecisionVideoMemoryTiledBuffer.GetValidRegion();
  }

  void PaintedTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                               const SurfaceDescriptorTiles& aTiledDescriptor);

  
  void RenderTile(const TiledTexture& aTile,
                  EffectChain& aEffectChain,
                  float aOpacity,
                  const gfx::Matrix4x4& aTransform,
                  const gfx::Point& aOffset,
                  const gfx::Filter& aFilter,
                  const gfx::Rect& aClipRect,
                  const nsIntRegion& aScreenRegion,
                  const nsIntPoint& aTextureOffset,
                  const nsIntSize& aTextureBounds);

  void Composite(EffectChain& aEffectChain,
                 float aOpacity,
                 const gfx::Matrix4x4& aTransform,
                 const gfx::Point& aOffset,
                 const gfx::Filter& aFilter,
                 const gfx::Rect& aClipRect,
                 const nsIntRegion* aVisibleRegion = nullptr,
                 TiledLayerProperties* aLayerProperties = nullptr);

  virtual CompositableType GetType() { return BUFFER_TILED; }

  virtual TiledLayerComposer* AsTiledLayerComposer() MOZ_OVERRIDE { return this; }

  virtual void EnsureDeprecatedTextureHost(TextureIdentifier aTextureId,
                                 const SurfaceDescriptor& aSurface,
                                 ISurfaceAllocator* aAllocator,
                                 const TextureInfo& aTextureInfo) MOZ_OVERRIDE
  {
    MOZ_CRASH("Does nothing");
  }

  virtual void SetCompositor(Compositor* aCompositor) MOZ_OVERRIDE
  {
    CompositableHost::SetCompositor(aCompositor);
    mVideoMemoryTiledBuffer.SetCompositor(aCompositor);
    mLowPrecisionVideoMemoryTiledBuffer.SetCompositor(aCompositor);
  }

  virtual void Attach(Layer* aLayer,
                      Compositor* aCompositor,
                      AttachFlags aFlags = NO_FLAGS) MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual void Dump(FILE* aFile=nullptr,
                    const char* aPrefix="",
                    bool aDumpHtml=false) MOZ_OVERRIDE;
#endif

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

private:
  void ProcessUploadQueue(nsIntRegion* aNewValidRegion,
                          TiledLayerProperties* aLayerProperties);
  void ProcessLowPrecisionUploadQueue();

  void RenderLayerBuffer(TiledLayerBufferComposite& aLayerBuffer,
                         const nsIntRegion& aValidRegion,
                         EffectChain& aEffectChain,
                         float aOpacity,
                         const gfx::Point& aOffset,
                         const gfx::Filter& aFilter,
                         const gfx::Rect& aClipRect,
                         const nsIntRegion& aMaskRegion,
                         nsIntRect aVisibleRect,
                         gfx::Matrix4x4 aTransform);

  void EnsureTileStore() {}

  nsIntRegion                  mRegionToUpload;
  nsIntRegion                  mLowPrecisionRegionToUpload;
  BasicTiledLayerBuffer        mMainMemoryTiledBuffer;
  BasicTiledLayerBuffer        mLowPrecisionMainMemoryTiledBuffer;
  TiledLayerBufferComposite    mVideoMemoryTiledBuffer;
  TiledLayerBufferComposite    mLowPrecisionVideoMemoryTiledBuffer;
  bool                         mPendingUpload : 1;
  bool                         mPendingLowPrecisionUpload : 1;
};

}
}

#endif
