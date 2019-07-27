




#ifndef GFX_ASYNCCOMPOSITIONMANAGER_H
#define GFX_ASYNCCOMPOSITIONMANAGER_H

#include "Units.h"                      
#include "mozilla/layers/LayerManagerComposite.h"  
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/TimeStamp.h"          
#include "mozilla/dom/ScreenOrientation.h"  
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/layers/LayersMessages.h"  
#include "nsRefPtr.h"                   
#include "nsISupportsImpl.h"            

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class Layer;
class LayerManagerComposite;
class AutoResolveRefLayers;


struct ViewTransform {
  explicit ViewTransform(LayerToParentLayerScale aScale = LayerToParentLayerScale(),
                         ParentLayerPoint aTranslation = ParentLayerPoint())
    : mScale(aScale)
    , mTranslation(aTranslation)
  {}

  operator gfx::Matrix4x4() const
  {
    return
      gfx::Matrix4x4::Scaling(mScale.scale, mScale.scale, 1)
                      .PostTranslate(mTranslation.x, mTranslation.y, 0);
  }

  
  
  friend gfx::Matrix4x4 operator*(const ViewTransform& a, const ViewTransform& b) {
    return gfx::Matrix4x4(a) * gfx::Matrix4x4(b);
  }

  bool operator==(const ViewTransform& rhs) const {
    return mTranslation == rhs.mTranslation && mScale == rhs.mScale;
  }

  bool operator!=(const ViewTransform& rhs) const {
    return !(*this == rhs);
  }

  LayerToParentLayerScale mScale;
  ParentLayerPoint mTranslation;
};









class AsyncCompositionManager final
{
  friend class AutoResolveRefLayers;
  ~AsyncCompositionManager()
  {
  }
public:
  NS_INLINE_DECL_REFCOUNTING(AsyncCompositionManager)

  explicit AsyncCompositionManager(LayerManagerComposite* aManager)
    : mLayerManager(aManager)
    , mIsFirstPaint(true)
    , mLayersUpdated(false)
    , mReadyForCompose(true)
  {
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
  void TransformScrollableLayer(Layer* aLayer);
  
  
  bool ApplyAsyncContentTransformToTree(Layer* aLayer);
  



  void ApplyAsyncTransformToScrollbar(Layer* aLayer);

  void SetFirstPaintViewport(const LayerIntPoint& aOffset,
                             const CSSToLayerScale& aZoom,
                             const CSSRect& aCssPageRect);
  void SetPageRect(const CSSRect& aCssPageRect);
  void SyncViewportInfo(const LayerIntRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aLayersUpdated,
                        ParentLayerPoint& aScrollOffset,
                        CSSToParentLayerScale& aScale,
                        LayerMargin& aFixedLayerMargins,
                        ScreenPoint& aOffset);
  void SyncFrameMetrics(const ParentLayerPoint& aScrollOffset,
                        float aZoom,
                        const CSSRect& aCssPageRect,
                        bool aLayersUpdated,
                        const CSSRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aIsFirstPaint,
                        LayerMargin& aFixedLayerMargins,
                        ScreenPoint& aOffset);

  














  void AlignFixedAndStickyLayers(Layer* aLayer, Layer* aTransformedSubtreeRoot,
                                 FrameMetrics::ViewID aTransformScrollId,
                                 const gfx::Matrix4x4& aPreviousTransformForRoot,
                                 const gfx::Matrix4x4& aCurrentTransformForRoot,
                                 const LayerMargin& aFixedLayerMargins);

  





  void ResolveRefLayers();
  




  void DetachRefLayers();

  TargetConfig mTargetConfig;
  CSSRect mContentRect;

  nsRefPtr<LayerManagerComposite> mLayerManager;
  
  
  
  
  
  bool mIsFirstPaint;

  
  
  bool mLayersUpdated;

  bool mReadyForCompose;

  gfx::Matrix mWorldTransform;
};

class MOZ_STACK_CLASS AutoResolveRefLayers {
public:
  explicit AutoResolveRefLayers(AsyncCompositionManager* aManager) : mManager(aManager)
  {
    if (mManager) {
      mManager->ResolveRefLayers();
    }
  }

  ~AutoResolveRefLayers()
  {
    if (mManager) {
      mManager->DetachRefLayers();
    }
  }

private:
  AsyncCompositionManager* mManager;

  AutoResolveRefLayers(const AutoResolveRefLayers&) = delete;
  AutoResolveRefLayers& operator=(const AutoResolveRefLayers&) = delete;
};

} 
} 

#endif
