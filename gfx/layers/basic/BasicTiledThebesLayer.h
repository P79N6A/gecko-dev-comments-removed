



#ifndef GFX_BASICTILEDTHEBESLAYER_H
#define GFX_BASICTILEDTHEBESLAYER_H

#include "TiledLayerBuffer.h"
#include "gfxReusableSurfaceWrapper.h"
#include "mozilla/layers/ShadowLayers.h"
#include "BasicLayers.h"
#include "BasicImplData.h"
#include <algorithm>

namespace mozilla {
namespace layers {









struct BasicTiledLayerTile {
  nsRefPtr<gfxReusableSurfaceWrapper> mSurface;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
  TimeStamp        mLastUpdate;
#endif

  
  BasicTiledLayerTile()
    : mSurface(NULL)
  {}
  explicit BasicTiledLayerTile(gfxImageSurface* aSurface)
    : mSurface(new gfxReusableSurfaceWrapper(aSurface))
  {
  }
  BasicTiledLayerTile(const BasicTiledLayerTile& o) {
    mSurface = o.mSurface;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
  }
  BasicTiledLayerTile& operator=(const BasicTiledLayerTile& o) {
    if (this == &o) return *this;
    mSurface = o.mSurface;
#ifdef GFX_TILEDLAYER_DEBUG_OVERLAY
    mLastUpdate = o.mLastUpdate;
#endif
    return *this;
  }
  bool operator== (const BasicTiledLayerTile& o) const {
    return mSurface == o.mSurface;
  }
  bool operator!= (const BasicTiledLayerTile& o) const {
    return mSurface != o.mSurface;
  }
  void ReadUnlock() {
    mSurface->ReadUnlock();
  }
  void ReadLock() {
    mSurface->ReadLock();
  }
};

class BasicTiledThebesLayer;








class BasicTiledLayerBuffer : public TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>
{
  friend class TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>;

public:
  BasicTiledLayerBuffer()
  {}

  void PaintThebes(BasicTiledThebesLayer* aLayer,
                   const nsIntRegion& aNewValidRegion,
                   const nsIntRegion& aPaintRegion,
                   LayerManager::DrawThebesLayerCallback aCallback,
                   void* aCallbackData);

  BasicTiledLayerTile GetPlaceholderTile() const {
    return mPlaceholder;
  }

  void ReadUnlock() {
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (mRetainedTiles[i] == GetPlaceholderTile()) continue;
      mRetainedTiles[i].ReadUnlock();
    }
  }

  void ReadLock() {
    for (size_t i = 0; i < mRetainedTiles.Length(); i++) {
      if (mRetainedTiles[i] == GetPlaceholderTile()) continue;
      mRetainedTiles[i].ReadLock();
    }
  }

  const gfxSize& GetResolution() { return mResolution; }
  void SetResolution(const gfxSize& aResolution) { mResolution = aResolution; }

  bool HasFormatChanged(BasicTiledThebesLayer* aThebesLayer) const;
protected:
  BasicTiledLayerTile ValidateTile(BasicTiledLayerTile aTile,
                                   const nsIntPoint& aTileRect,
                                   const nsIntRegion& dirtyRect);

  
  
  
  
  bool UseSinglePaintBuffer() { return true; }

  void ReleaseTile(BasicTiledLayerTile aTile) {  }

  void SwapTiles(BasicTiledLayerTile& aTileA, BasicTiledLayerTile& aTileB) {
    std::swap(aTileA, aTileB);
  }

private:
  gfxASurface::gfxImageFormat GetFormat() const;
  BasicTiledThebesLayer* mThebesLayer;
  LayerManager::DrawThebesLayerCallback mCallback;
  void* mCallbackData;
  gfxSize mResolution;
  bool mLastPaintOpaque;

  
  nsRefPtr<gfxImageSurface>     mSinglePaintBuffer;
  nsIntPoint                    mSinglePaintBufferOffset;

  BasicTiledLayerTile           mPlaceholder;

  BasicTiledLayerTile ValidateTileInternal(BasicTiledLayerTile aTile,
                                           const nsIntPoint& aTileOrigin,
                                           const nsIntRect& aDirtyRect);
};







class BasicTiledThebesLayer : public ThebesLayer,
                              public BasicImplData,
                              public BasicShadowableLayer
{
  typedef ThebesLayer Base;

public:
  BasicTiledThebesLayer(BasicShadowLayerManager* const aManager)
    : ThebesLayer(aManager, static_cast<BasicImplData*>(this))
  {
    MOZ_COUNT_CTOR(BasicTiledThebesLayer);
  }

  ~BasicTiledThebesLayer()
  {
    MOZ_COUNT_DTOR(BasicTiledThebesLayer);
  }


  
  virtual Layer* AsLayer() { return this; }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) {
    mValidRegion.Sub(mValidRegion, aRegion);
  }

  
  virtual void FillSpecificAttributes(SpecificLayerAttributes& aAttrs);
  virtual ShadowableLayer* AsShadowableLayer() { return this; }

  virtual void Disconnect()
  {
    BasicShadowableLayer::Disconnect();
  }

  virtual void PaintThebes(gfxContext* aContext,
                           Layer* aMaskLayer,
                           LayerManager::DrawThebesLayerCallback aCallback,
                           void* aCallbackData,
                           ReadbackProcessor* aReadback);

private:
  BasicShadowLayerManager* BasicManager()
  {
    return static_cast<BasicShadowLayerManager*>(mManager);
  }

  
  virtual void
  PaintBuffer(gfxContext* aContext,
              const nsIntRegion& aRegionToDraw,
              const nsIntRegion& aExtendedRegionToDraw,
              const nsIntRegion& aRegionToInvalidate,
              bool aDidSelfCopy,
              LayerManager::DrawThebesLayerCallback aCallback,
              void* aCallbackData)
  { NS_RUNTIMEABORT("Not reached."); }

  
  BasicTiledLayerBuffer mTiledBuffer;
};

} 
} 

#endif
