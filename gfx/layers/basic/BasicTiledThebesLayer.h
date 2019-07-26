



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





struct BasicTiledLayerPaintData {
  gfx::Point mScrollOffset;
  gfx3DMatrix mTransformScreenToLayer;
  nsIntRect mLayerCriticalDisplayPort;
  gfx::ZoomScale mResolution;
  nsIntRect mCompositionBounds;
  uint16_t mLowPrecisionPaintCount;
  bool mPaintFinished : 1;
};

class BasicTiledThebesLayer;








class BasicTiledLayerBuffer : public TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>
{
  friend class TiledLayerBuffer<BasicTiledLayerBuffer, BasicTiledLayerTile>;

public:
  BasicTiledLayerBuffer()
    : mLastPaintOpaque(false)
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

  const gfx::ZoomScale& GetFrameResolution() { return mFrameResolution; }
  void SetFrameResolution(const gfx::ZoomScale& aResolution) { mFrameResolution = aResolution; }

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
  gfx::ZoomScale mFrameResolution;
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
  BasicTiledThebesLayer(BasicShadowLayerManager* const aManager);
  ~BasicTiledThebesLayer();

  
  virtual Layer* AsLayer() { return this; }
  virtual void InvalidateRegion(const nsIntRegion& aRegion) {
    mInvalidRegion.Or(mInvalidRegion, aRegion);
    mValidRegion.Sub(mValidRegion, aRegion);
    mLowPrecisionValidRegion.Sub(mLowPrecisionValidRegion, aRegion);
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

  






















  bool ComputeProgressiveUpdateRegion(BasicTiledLayerBuffer& aTiledBuffer,
                                      const nsIntRegion& aInvalidRegion,
                                      const nsIntRegion& aOldValidRegion,
                                      nsIntRegion& aRegionToPaint,
                                      const gfx3DMatrix& aTransform,
                                      const nsIntRect& aCompositionBounds,
                                      const gfx::Point& aScrollOffset,
                                      const gfx::ZoomScale& aResolution,
                                      bool aIsRepeated);

  



  bool ProgressiveUpdate(BasicTiledLayerBuffer& aTiledBuffer,
                         nsIntRegion& aValidRegion,
                         nsIntRegion& aInvalidRegion,
                         const nsIntRegion& aOldValidRegion,
                         const gfx3DMatrix& aTransform,
                         const nsIntRect& aCompositionBounds,
                         const gfx::Point& aScrollOffset,
                         const gfx::ZoomScale& aResolution,
                         LayerManager::DrawThebesLayerCallback aCallback,
                         void* aCallbackData);

  



  void BeginPaint();

  




  void EndPaint(bool aFinish);

  
  BasicTiledLayerBuffer mTiledBuffer;
  BasicTiledLayerBuffer mLowPrecisionTiledBuffer;
  nsIntRegion mLowPrecisionValidRegion;
  gfx::Point mLastScrollOffset;
  bool mFirstPaint;
  BasicTiledLayerPaintData mPaintData;
};

} 
} 

#endif
