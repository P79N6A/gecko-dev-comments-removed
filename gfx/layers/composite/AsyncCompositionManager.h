




#ifndef GFX_ASYNCCOMPOSITIONMANAGER_H
#define GFX_ASYNCCOMPOSITIONMANAGER_H

#include "Units.h"                      
#include "mozilla/layers/LayerManagerComposite.h"  
#include "gfx3DMatrix.h"                
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/TimeStamp.h"          
#include "mozilla/dom/ScreenOrientation.h"  
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/layers/LayersMessages.h"  
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            

namespace mozilla {
namespace layers {

class AsyncPanZoomController;
class Layer;
class LayerManagerComposite;
class AutoResolveRefLayers;


struct ViewTransform {
  ViewTransform(LayerPoint aTranslation = LayerPoint(),
                ParentLayerToScreenScale aScale = ParentLayerToScreenScale())
    : mTranslation(aTranslation)
    , mScale(aScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0) *
      gfx3DMatrix::ScalingMatrix(mScale.scale, mScale.scale, 1);
  }

  
  
  friend gfx3DMatrix operator*(const ViewTransform& a, const ViewTransform& b) {
    return gfx3DMatrix(a) * gfx3DMatrix(b);
  }

  bool operator==(const ViewTransform& rhs) const {
    return mTranslation == rhs.mTranslation && mScale == rhs.mScale;
  }

  bool operator!=(const ViewTransform& rhs) const {
    return !(*this == rhs);
  }

  LayerPoint mTranslation;
  ParentLayerToScreenScale mScale;
};









class AsyncCompositionManager MOZ_FINAL
{
  friend class AutoResolveRefLayers;
public:
  NS_INLINE_DECL_REFCOUNTING(AsyncCompositionManager)

  AsyncCompositionManager(LayerManagerComposite* aManager)
    : mLayerManager(aManager)
    , mIsFirstPaint(false)
    , mLayersUpdated(false)
    , mReadyForCompose(true)
  {
  }
  ~AsyncCompositionManager()
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
  
  
  
  bool ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame, Layer* aLayer,
                                        bool* aWantNextFrame);
  



  void ApplyAsyncTransformToScrollbar(TimeStamp aCurrentFrame, ContainerLayer* aLayer);

  void SetFirstPaintViewport(const LayerIntPoint& aOffset,
                             const CSSToLayerScale& aZoom,
                             const CSSRect& aCssPageRect);
  void SetPageRect(const CSSRect& aCssPageRect);
  void SyncViewportInfo(const LayerIntRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aLayersUpdated,
                        ScreenPoint& aScrollOffset,
                        CSSToScreenScale& aScale,
                        LayerMargin& aFixedLayerMargins,
                        ScreenPoint& aOffset);
  void SyncFrameMetrics(const ScreenPoint& aScrollOffset,
                        float aZoom,
                        const CSSRect& aCssPageRect,
                        bool aLayersUpdated,
                        const CSSRect& aDisplayPort,
                        const CSSToLayerScale& aDisplayResolution,
                        bool aIsFirstPaint,
                        LayerMargin& aFixedLayerMargins,
                        ScreenPoint& aOffset);

  














  void AlignFixedAndStickyLayers(Layer* aLayer, Layer* aTransformedSubtreeRoot,
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
};

class MOZ_STACK_CLASS AutoResolveRefLayers {
public:
  AutoResolveRefLayers(AsyncCompositionManager* aManager) : mManager(aManager)
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

  AutoResolveRefLayers(const AutoResolveRefLayers&) MOZ_DELETE;
  AutoResolveRefLayers& operator=(const AutoResolveRefLayers&) MOZ_DELETE;
};

} 
} 

#endif
