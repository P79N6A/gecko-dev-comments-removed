




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
  ViewTransform(LayerPoint aTranslation = LayerPoint(),
                CSSToScreenScale aScale = CSSToScreenScale())
    : mTranslation(aTranslation)
    , mScale(aScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0) *
      gfx3DMatrix::ScalingMatrix(mScale.scale, mScale.scale, 1);
  }

  LayerPoint mTranslation;
  CSSToScreenScale mScale;
};









class AsyncCompositionManager MOZ_FINAL : public RefCounted<AsyncCompositionManager>
{
  friend class AutoResolveRefLayers;
public:
  AsyncCompositionManager(LayerManagerComposite* aManager)
    : mLayerManager(aManager)
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

private:
  void TransformScrollableLayer(Layer* aLayer, const gfx3DMatrix& aRootTransform);
  
  
  
  bool ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame, Layer* aLayer,
                                        bool* aWantNextFrame);

  void SetFirstPaintViewport(const LayerIntPoint& aOffset,
                             float aZoom,
                             const CSSRect& aCssPageRect);
  void SetPageRect(const CSSRect& aCssPageRect);
  void SyncViewportInfo(const LayerIntRect& aDisplayPort,
                        float aDisplayResolution,
                        bool aLayersUpdated,
                        ScreenPoint& aScrollOffset,
                        float& aScaleX, float& aScaleY,
                        gfx::Margin& aFixedLayerMargins,
                        ScreenPoint& aOffset);
  void SyncFrameMetrics(const ScreenPoint& aScrollOffset,
                        float aZoom,
                        const CSSRect& aCssPageRect,
                        bool aLayersUpdated,
                        const CSSRect& aDisplayPort,
                        float aDisplayResolution,
                        bool aIsFirstPaint,
                        gfx::Margin& aFixedLayerMargins,
                        ScreenPoint& aOffset);

  






  void TransformFixedLayers(Layer* aLayer,
                            const gfxPoint& aTranslation,
                            const gfxSize& aScaleDiff,
                            const gfx::Margin& aFixedLayerMargins);

  





  void ResolveRefLayers();
  




  void DetachRefLayers();

  TargetConfig mTargetConfig;
  CSSRect mContentRect;

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
