




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
                LayoutDeviceToScreenScale aScale = LayoutDeviceToScreenScale())
    : mTranslation(aTranslation)
    , mScale(aScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0) *
      gfx3DMatrix::ScalingMatrix(mScale.scale, mScale.scale, 1);
  }

  bool operator==(const ViewTransform& rhs) const {
    return mTranslation == rhs.mTranslation && mScale == rhs.mScale;
  }

  bool operator!=(const ViewTransform& rhs) const {
    return !(*this == rhs);
  }

  LayerPoint mTranslation;
  LayoutDeviceToScreenScale mScale;
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
  void TransformScrollableLayer(Layer* aLayer, const LayoutDeviceToLayerScale& aResolution);
  
  
  
  bool ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame, Layer* aLayer,
                                        bool* aWantNextFrame);

  void SetFirstPaintViewport(const LayerIntPoint& aOffset,
                             const CSSToLayerScale& aZoom,
                             const CSSRect& aCssPageRect);
  void SetPageRect(const CSSRect& aCssPageRect);
  void SyncViewportInfo(const LayerIntRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aLayersUpdated,
                        ScreenPoint& aScrollOffset,
                        CSSToScreenScale& aScale,
                        gfx::Margin& aFixedLayerMargins,
                        ScreenPoint& aOffset);
  void SyncFrameMetrics(const ScreenPoint& aScrollOffset,
                        float aZoom,
                        const CSSRect& aCssPageRect,
                        bool aLayersUpdated,
                        const CSSRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aIsFirstPaint,
                        gfx::Margin& aFixedLayerMargins,
                        ScreenPoint& aOffset);

  









  void AlignFixedLayersForAnchorPoint(Layer* aLayer,
                                      Layer* aTransformedSubtreeRoot,
                                      const gfx3DMatrix& aPreviousTransformForRoot,
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
