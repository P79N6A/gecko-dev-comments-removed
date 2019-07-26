




#ifndef GFX_ASYNCCOMPOSITIONMANAGER_H
#define GFX_ASYNCCOMPOSITIONMANAGER_H

#include "gfxPoint.h"
#include "gfx3DMatrix.h"
#include "nsAutoPtr.h"
#include "nsRect.h"
#include "mozilla/dom/ScreenOrientation.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/layers/LayerTransaction.h" 

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class Layer;
class LayerManagerComposite;
class AutoResolveRefLayers;


struct ViewTransform {
  ViewTransform(gfxPoint aTranslation = gfxPoint(),
                gfxSize aScale = gfxSize(1, 1))
    : mTranslation(aTranslation)
    , mScale(aScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::ScalingMatrix(mScale.width, mScale.height, 1) *
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0);
  }

  gfxPoint mTranslation;
  gfxSize mScale;
};









class AsyncCompositionManager MOZ_FINAL : public RefCounted<AsyncCompositionManager>
{
  friend class AutoResolveRefLayers;
public:
  AsyncCompositionManager(LayerManagerComposite* aManager)
    : mXScale(1.0)
    , mYScale(1.0)
    , mLayerManager(aManager)
    , mIsFirstPaint(false)
    , mLayersUpdated(false)
    , mReadyForCompose(true)
  {
    MOZ_COUNT_CTOR(AsyncCompositionManager);
  }
  ~AsyncCompositionManager()
  {
    MOZ_COUNT_DTOR(AsyncCompositionManager);
  }

  






  void ForceIsFirstPaint() { mIsFirstPaint = true; }

  
  
  bool TransformShadowTree(TimeStamp aCurrentFrame);

  
  
  void ComputeRotation();

  
  void Updated(bool isFirstPaint, const TargetConfig& aTargetConfig)
  {
    mIsFirstPaint |= isFirstPaint;
    mLayersUpdated = true;
    mTargetConfig = aTargetConfig;
  }

  bool RequiresReorientation(mozilla::dom::ScreenOrientation aOrientation)
  {
    return mTargetConfig.orientation() != aOrientation;
  }

  
  bool ReadyForCompose() { return mReadyForCompose; }

  
  
  bool IsFirstPaint() { return mIsFirstPaint; }

  void SetTransformation(float aScale, const nsIntPoint& aScrollOffset);

private:
  void TransformScrollableLayer(Layer* aLayer, const gfx3DMatrix& aRootTransform);
  
  
  
  bool ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame, Layer* aLayer,
                                        bool* aWantNextFrame);

  void SetFirstPaintViewport(const nsIntPoint& aOffset,
                             float aZoom,
                             const nsIntRect& aPageRect,
                             const gfx::Rect& aCssPageRect);
  void SetPageRect(const gfx::Rect& aCssPageRect);
  void SyncViewportInfo(const nsIntRect& aDisplayPort,
                        float aDisplayResolution,
                        bool aLayersUpdated,
                        nsIntPoint& aScrollOffset,
                        float& aScaleX, float& aScaleY,
                        gfx::Margin& aFixedLayerMargins,
                        gfx::Point& aOffset);
  void SyncFrameMetrics(const gfx::Point& aScrollOffset,
                        float aZoom,
                        const gfx::Rect& aCssPageRect,
                        bool aLayersUpdated,
                        const gfx::Rect& aDisplayPort,
                        float aDisplayResolution,
                        bool aIsFirstPaint,
                        gfx::Margin& aFixedLayerMargins,
                        gfx::Point& aOffset);

  






  void TransformFixedLayers(Layer* aLayer,
                            const gfxPoint& aTranslation,
                            const gfxSize& aScaleDiff,
                            const gfx::Margin& aFixedLayerMargins);

  





  void ResolveRefLayers();
  




  void DetachRefLayers();

  TargetConfig mTargetConfig;
  float mXScale;
  float mYScale;
  nsIntPoint mScrollOffset;
  nsIntRect mContentRect;

  nsRefPtr<LayerManagerComposite> mLayerManager;
  
  
  
  
  
  bool mIsFirstPaint;

  
  
  bool mLayersUpdated;

  bool mReadyForCompose;
};

class MOZ_STACK_CLASS AutoResolveRefLayers {
public:
  AutoResolveRefLayers(AsyncCompositionManager* aManager) : mManager(aManager)
  { mManager->ResolveRefLayers(); }

  ~AutoResolveRefLayers()
  { mManager->DetachRefLayers(); }

private:
  AsyncCompositionManager* mManager;

  AutoResolveRefLayers(const AutoResolveRefLayers&) MOZ_DELETE;
  AutoResolveRefLayers& operator=(const AutoResolveRefLayers&) MOZ_DELETE;
};

} 
} 

#endif
