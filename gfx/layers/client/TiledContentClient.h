




#ifndef MOZILLA_GFX_TILEDCONTENTCLIENT_H
#define MOZILLA_GFX_TILEDCONTENTCLIENT_H

#include <stddef.h>                     
#include <stdint.h>                     
#include <algorithm>                    
#include "Layers.h"                     
#include "TiledLayerBuffer.h"           
#include "Units.h"                      
#include "gfx3DMatrix.h"                
#include "gfxTypes.h"
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/TextureClient.h"
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "mozilla/layers/ISurfaceAllocator.h"
#include "gfxReusableSurfaceWrapper.h"

class gfxImageSurface;

namespace mozilla {
namespace layers {

class BasicTileDescriptor;










struct BasicTiledLayerTile {
  RefPtr<DeprecatedTextureClientTile> mDeprecatedTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  TimeStamp        mLastUpdate;
#endif

  
  BasicTiledLayerTile()
    : mDeprecatedTextureClient(nullptr)
  {}

  BasicTiledLayerTile(DeprecatedTextureClientTile* aTextureClient)
    : mDeprecatedTextureClient(aTextureClient)
  {}

  BasicTiledLayerTile(const BasicTiledLayerTile& o) {
    mDeprecatedTextureClient = o.mDeprecatedTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
  }
  BasicTiledLayerTile& operator=(const BasicTiledLayerTile& o) {
    if (this == &o) return *this;
    mDeprecatedTextureClient = o.mDeprecatedTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
    return *this;
  }
  bool operator== (const BasicTiledLayerTile& o) const {
    return mDeprecatedTextureClient == o.mDeprecatedTextureClient;
  }
  bool operator!= (const BasicTiledLayerTile& o) const {
    return mDeprecatedTextureClient != o.mDeprecatedTextureClient;
  }

  bool IsPlaceholderTile() { return mDeprecatedTextureClient == nullptr; }

  void ReadUnlock() {
    GetSurface()->ReadUnlock();
  }
  void ReadLock() {
    GetSurface()->ReadLock();
  }

  TileDescriptor GetTileDescriptor();
  static BasicTiledLayerTile OpenDescriptor(ISurfaceAllocator *aAllocator, const TileDescriptor& aDesc);

  gfxReusableSurfaceWrapper* GetSurface() {
    return mDeprecatedTextureClient->GetReusableSurfaceWrapper();
  }
};





struct BasicTiledLayerPaintData {
  



  ScreenPoint mScrollOffset;

  




  ScreenPoint mLastScrollOffset;

  



  gfx3DMatrix mTransformScreenToLayout;

  







  nsIntRect mLayoutCriticalDisplayPort;

  



  CSSToScreenScale mResolution;

  




  LayoutDeviceRect mCompositionBounds;

  




  uint16_t mLowPrecisionPaintCount;

  


  bool mFirstPaint : 1;

  




  bool mPaintFinished : 1;
};

class ClientTiledThebesLayer;
class ClientLayerManager;

class SharedFrameMetricsHelper
{
public:
  SharedFrameMetricsHelper();
  ~SharedFrameMetricsHelper();

  






  bool UpdateFromCompositorFrameMetrics(ContainerLayer* aLayer,
                                        bool aHasPendingNewThebesContent,
                                        bool aLowPrecision,
                                        ScreenRect& aCompositionBounds,
                                        CSSToScreenScale& aZoom);

  




  void FindFallbackContentFrameMetrics(ContainerLayer* aLayer,
                                       ScreenRect& aCompositionBounds,
                                       CSSToScreenScale& aZoom);
  






  bool AboutToCheckerboard(const FrameMetrics& aContentMetrics,
                           const FrameMetrics& aCompositorMetrics);
private:
  bool mLastProgressiveUpdateWasLowPrecision;
  bool mProgressiveUpdateWasInDanger;
};







class BasicTiledLayerBuffer
  : public TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>
{
  friend class TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>;

public:
  BasicTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                        ClientLayerManager* aManager,
                        SharedFrameMetricsHelper* aHelper);
  BasicTiledLayerBuffer()
    : mThebesLayer(nullptr)
    , mManager(nullptr)
    , mLastPaintOpaque(false)
    , mSharedFrameMetricsHelper(nullptr)
  {}

  BasicTiledLayerBuffer(ISurfaceAllocator* aAllocator,
                        const nsIntRegion& aValidRegion,
                        const nsIntRegion& aPaintedRegion,
                        const InfallibleTArray<TileDescriptor>& aTiles,
                        int aRetainedWidth,
                        int aRetainedHeight,
                        float aResolution,
                        SharedFrameMetricsHelper* aHelper)
  {
    mSharedFrameMetricsHelper = aHelper;
    mValidRegion = aValidRegion;
    mPaintedRegion = aPaintedRegion;
    mRetainedWidth = aRetainedWidth;
    mRetainedHeight = aRetainedHeight;
    mResolution = aResolution;

    for(size_t i = 0; i < aTiles.Length(); i++) {
      if (aTiles[i].type() == TileDescriptor::TPlaceholderTileDescriptor) {
        mRetainedTiles.AppendElement(GetPlaceholderTile());
      } else {
        mRetainedTiles.AppendElement(BasicTiledLayerTile::OpenDescriptor(aAllocator, aTiles[i]));
      }
    }
  }

  void PaintThebes(const nsIntRegion& aNewValidRegion,
                   const nsIntRegion& aPaintRegion,
                   LayerManager::DrawThebesLayerCallback aCallback,
                   void* aCallbackData);

  void ReadUnlock() {
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (mRetainedTiles[i].IsPlaceholderTile()) continue;
      mRetainedTiles[i].ReadUnlock();
    }
  }

  void ReadLock() {
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (mRetainedTiles[i].IsPlaceholderTile()) continue;
      mRetainedTiles[i].ReadLock();
    }
  }

  const CSSToScreenScale& GetFrameResolution() { return mFrameResolution; }
  void SetFrameResolution(const CSSToScreenScale& aResolution) { mFrameResolution = aResolution; }

  bool HasFormatChanged() const;

  



  bool ProgressiveUpdate(nsIntRegion& aValidRegion,
                         nsIntRegion& aInvalidRegion,
                         const nsIntRegion& aOldValidRegion,
                         BasicTiledLayerPaintData* aPaintData,
                         LayerManager::DrawThebesLayerCallback aCallback,
                         void* aCallbackData);

  SurfaceDescriptorTiles GetSurfaceDescriptorTiles();

  static BasicTiledLayerBuffer OpenDescriptor(ISurfaceAllocator* aAllocator,
                                              const SurfaceDescriptorTiles& aDescriptor,
                                              SharedFrameMetricsHelper* aHelper);

protected:
  BasicTiledLayerTile ValidateTile(BasicTiledLayerTile aTile,
                                   const nsIntPoint& aTileRect,
                                   const nsIntRegion& dirtyRect);

  
  
  
  
  bool UseSinglePaintBuffer() { return true; }

  void ReleaseTile(BasicTiledLayerTile aTile) {  }

  void SwapTiles(BasicTiledLayerTile& aTileA, BasicTiledLayerTile& aTileB) {
    std::swap(aTileA, aTileB);
  }

  BasicTiledLayerTile GetPlaceholderTile() const { return BasicTiledLayerTile(); }

private:
  gfxContentType GetContentType() const;
  ClientTiledThebesLayer* mThebesLayer;
  ClientLayerManager* mManager;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  CSSToScreenScale mFrameResolution;
  bool mLastPaintOpaque;

  
  nsRefPtr<gfxImageSurface>     mSinglePaintBuffer;
  RefPtr<gfx::DrawTarget>       mSinglePaintDrawTarget;
  nsIntPoint                    mSinglePaintBufferOffset;
  SharedFrameMetricsHelper*  mSharedFrameMetricsHelper;

  BasicTiledLayerTile ValidateTileInternal(BasicTiledLayerTile aTile,
                                           const nsIntPoint& aTileOrigin,
                                           const nsIntRect& aDirtyRect);

  
















  bool ComputeProgressiveUpdateRegion(const nsIntRegion& aInvalidRegion,
                                      const nsIntRegion& aOldValidRegion,
                                      nsIntRegion& aRegionToPaint,
                                      BasicTiledLayerPaintData* aPaintData,
                                      bool aIsRepeated);
};

class TiledContentClient : public CompositableClient
{
  
  
  
  
  friend class ClientTiledThebesLayer;

public:
  TiledContentClient(ClientTiledThebesLayer* aThebesLayer,
                     ClientLayerManager* aManager);

  ~TiledContentClient()
  {
    MOZ_COUNT_DTOR(TiledContentClient);
  }

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return TextureInfo(BUFFER_TILED);
  }

  enum TiledBufferType {
    TILED_BUFFER,
    LOW_PRECISION_TILED_BUFFER
  };
  void LockCopyAndWrite(TiledBufferType aType);

private:
  SharedFrameMetricsHelper mSharedFrameMetricsHelper;
  BasicTiledLayerBuffer mTiledBuffer;
  BasicTiledLayerBuffer mLowPrecisionTiledBuffer;
};

}
}

#endif
