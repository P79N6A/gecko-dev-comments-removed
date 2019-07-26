




#ifndef MOZILLA_GFX_TILEDCONTENTCLIENT_H
#define MOZILLA_GFX_TILEDCONTENTCLIENT_H

#include "mozilla/layers/ContentClient.h"
#include "TiledLayerBuffer.h"
#include "gfxPlatform.h"

namespace mozilla {
namespace layers {










struct BasicTiledLayerTile {
  RefPtr<TextureClientTile> mTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  TimeStamp        mLastUpdate;
#endif

  
  BasicTiledLayerTile()
    : mTextureClient(nullptr)
  {}

  BasicTiledLayerTile(const BasicTiledLayerTile& o) {
    mTextureClient = o.mTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
  }
  BasicTiledLayerTile& operator=(const BasicTiledLayerTile& o) {
    if (this == &o) return *this;
    mTextureClient = o.mTextureClient;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
    return *this;
  }
  bool operator== (const BasicTiledLayerTile& o) const {
    return mTextureClient == o.mTextureClient;
  }
  bool operator!= (const BasicTiledLayerTile& o) const {
    return mTextureClient != o.mTextureClient;
  }

  bool IsPlaceholderTile() { return mTextureClient == nullptr; }

  void ReadUnlock() {
    GetSurface()->ReadUnlock();
  }
  void ReadLock() {
    GetSurface()->ReadLock();
  }

  gfxReusableSurfaceWrapper* GetSurface() {
    return mTextureClient->GetReusableSurfaceWrapper();
  }
};





struct BasicTiledLayerPaintData {
  gfx::Point mScrollOffset;
  gfx::Point mLastScrollOffset;
  gfx3DMatrix mTransformScreenToLayer;
  nsIntRect mLayerCriticalDisplayPort;
  gfxSize mResolution;
  nsIntRect mCompositionBounds;
  uint16_t mLowPrecisionPaintCount;
  bool mFirstPaint : 1;
  bool mPaintFinished : 1;
};

class ClientTiledThebesLayer;
class ClientLayerManager;







class BasicTiledLayerBuffer
  : public TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>
{
  friend class TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>;

public:
  BasicTiledLayerBuffer(ClientTiledThebesLayer* aThebesLayer,
                        ClientLayerManager* aManager);
  BasicTiledLayerBuffer()
    : mThebesLayer(nullptr)
    , mManager(nullptr)
    , mLastPaintOpaque(false)
  {}

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

  const gfxSize& GetFrameResolution() { return mFrameResolution; }
  void SetFrameResolution(const gfxSize& aResolution) { mFrameResolution = aResolution; }

  bool HasFormatChanged() const;

  



  bool ProgressiveUpdate(nsIntRegion& aValidRegion,
                         nsIntRegion& aInvalidRegion,
                         const nsIntRegion& aOldValidRegion,
                         BasicTiledLayerPaintData* aPaintData,
                         LayerManager::DrawThebesLayerCallback aCallback,
                         void* aCallbackData);

  






  BasicTiledLayerBuffer DeepCopy() const;

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
  gfxASurface::gfxContentType GetContentType() const;
  ClientTiledThebesLayer* mThebesLayer;
  ClientLayerManager* mManager;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  gfxSize mFrameResolution;
  bool mLastPaintOpaque;

  
  nsRefPtr<gfxImageSurface>     mSinglePaintBuffer;
  nsIntPoint                    mSinglePaintBufferOffset;

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
  BasicTiledLayerBuffer mTiledBuffer;
  BasicTiledLayerBuffer mLowPrecisionTiledBuffer;
};

}
}

#endif
