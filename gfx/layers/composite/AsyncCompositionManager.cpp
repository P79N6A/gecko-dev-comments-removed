





#include "mozilla/layers/AsyncCompositionManager.h"
#include "base/basictypes.h"

#if defined(MOZ_WIDGET_ANDROID)
# include <android/log.h>
# include "AndroidBridge.h"
#endif

#include "CompositorParent.h"
#include "LayerManagerComposite.h" 

#include "nsStyleAnimation.h"
#include "nsDisplayList.h"
#include "AnimationCommon.h"
#include "nsAnimationManager.h"
#include "mozilla/layers/AsyncPanZoomController.h"

using namespace mozilla::dom;

namespace mozilla {
namespace layers {

enum Op { Resolve, Detach };

static bool
IsSameDimension(ScreenOrientation o1, ScreenOrientation o2)
{
  bool isO1portrait = (o1 == eScreenOrientation_PortraitPrimary || o1 == eScreenOrientation_PortraitSecondary);
  bool isO2portrait = (o2 == eScreenOrientation_PortraitPrimary || o2 == eScreenOrientation_PortraitSecondary);
  return !(isO1portrait ^ isO2portrait);
}

static bool
ContentMightReflowOnOrientationChange(const nsIntRect& rect)
{
  return rect.width != rect.height;
}

template<Op OP>
static void
WalkTheTree(Layer* aLayer,
            Layer* aParent,
            bool& aReady,
            const TargetConfig& aTargetConfig)
{
  if (RefLayer* ref = aLayer->AsRefLayer()) {
    if (const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(ref->GetReferentId())) {
      if (Layer* referent = state->mRoot) {
        if (!ref->GetVisibleRegion().IsEmpty()) {
          ScreenOrientation chromeOrientation = aTargetConfig.orientation();
          ScreenOrientation contentOrientation = state->mTargetConfig.orientation();
          if (!IsSameDimension(chromeOrientation, contentOrientation) &&
              ContentMightReflowOnOrientationChange(aTargetConfig.clientBounds())) {
            aReady = false;
          }
        }

        if (OP == Resolve) {
          ref->ConnectReferentLayer(referent);
          if (AsyncPanZoomController* apzc = state->mController) {
            referent->SetAsyncPanZoomController(apzc);
          }
        } else {
          ref->DetachReferentLayer(referent);
          referent->SetAsyncPanZoomController(nullptr);
        }
      }
    }
  }
  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    WalkTheTree<OP>(child, aLayer, aReady, aTargetConfig);
  }
}

void
AsyncCompositionManager::ResolveRefLayers()
{
  WalkTheTree<Resolve>(mLayerManager->GetRoot(),
                       nullptr,
                       mReadyForCompose,
                       mTargetConfig);
}

void
AsyncCompositionManager::DetachRefLayers()
{
  WalkTheTree<Detach>(mLayerManager->GetRoot(),
                      nullptr,
                      mReadyForCompose,
                      mTargetConfig);
}

void
AsyncCompositionManager::ComputeRotation()
{
  if (!mTargetConfig.naturalBounds().IsEmpty()) {
    mLayerManager->SetWorldTransform(
      ComputeTransformForRotation(mTargetConfig.naturalBounds(),
                                  mTargetConfig.rotation()));
  }
}



static void
Translate2D(gfx3DMatrix& aTransform, const gfxPoint& aOffset)
{
  aTransform._41 += aOffset.x;
  aTransform._42 += aOffset.y;
}

void
AsyncCompositionManager::TransformFixedLayers(Layer* aLayer,
                                              const gfxPoint& aTranslation,
                                              const gfxSize& aScaleDiff,
                                              const gfx::Margin& aFixedLayerMargins)
{
  if (aLayer->GetIsFixedPosition() &&
      !aLayer->GetParent()->GetIsFixedPosition()) {
    
    
    
    const gfxPoint& anchor = aLayer->GetFixedPositionAnchor();
    gfxPoint translation(aTranslation - (anchor - anchor / aScaleDiff));

    
    
    
    
    
    
    
    
    
    
    const gfx::Margin& fixedMargins = aLayer->GetFixedPositionMargins();
    if (fixedMargins.left >= 0) {
      if (anchor.x > 0) {
        translation.x -= aFixedLayerMargins.right - fixedMargins.right;
      } else {
        translation.x += aFixedLayerMargins.left - fixedMargins.left;
      }
    }

    if (fixedMargins.top >= 0) {
      if (anchor.y > 0) {
        translation.y -= aFixedLayerMargins.bottom - fixedMargins.bottom;
      } else {
        translation.y += aFixedLayerMargins.top - fixedMargins.top;
      }
    }

    
    
    
    LayerComposite* layerComposite = aLayer->AsLayerComposite();
    gfx3DMatrix layerTransform;
    if (layerComposite->GetShadowTransformSetByAnimation()) {
      
      layerTransform = aLayer->GetLocalTransform();
    } else {
      layerTransform = aLayer->GetTransform();
    }
    Translate2D(layerTransform, translation);
    if (ContainerLayer* c = aLayer->AsContainerLayer()) {
      layerTransform.Scale(1.0f/c->GetPreXScale(),
                           1.0f/c->GetPreYScale(),
                           1);
    }
    layerTransform.ScalePost(1.0f/aLayer->GetPostXScale(),
                             1.0f/aLayer->GetPostYScale(),
                             1);
    layerComposite->SetShadowTransform(layerTransform);
    layerComposite->SetShadowTransformSetByAnimation(false);

    const nsIntRect* clipRect = aLayer->GetClipRect();
    if (clipRect) {
      nsIntRect transformedClipRect(*clipRect);
      transformedClipRect.MoveBy(translation.x, translation.y);
      layerComposite->SetShadowClipRect(&transformedClipRect);
    }

    
    
    return;
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    TransformFixedLayers(child, aTranslation, aScaleDiff, aFixedLayerMargins);
  }
}

static void
SampleValue(float aPortion, Animation& aAnimation, nsStyleAnimation::Value& aStart,
            nsStyleAnimation::Value& aEnd, Animatable* aValue)
{
  nsStyleAnimation::Value interpolatedValue;
  NS_ASSERTION(aStart.GetUnit() == aEnd.GetUnit() ||
               aStart.GetUnit() == nsStyleAnimation::eUnit_None ||
               aEnd.GetUnit() == nsStyleAnimation::eUnit_None, "Must have same unit");
  nsStyleAnimation::Interpolate(aAnimation.property(), aStart, aEnd,
                                aPortion, interpolatedValue);
  if (aAnimation.property() == eCSSProperty_opacity) {
    *aValue = interpolatedValue.GetFloatValue();
    return;
  }

  nsCSSValueList* interpolatedList = interpolatedValue.GetCSSValueListValue();

  TransformData& data = aAnimation.data().get_TransformData();
  nsPoint origin = data.origin();
  
  
  double cssPerDev = double(nsDeviceContext::AppUnitsPerCSSPixel())
                     / double(data.appUnitsPerDevPixel());
  gfxPoint3D mozOrigin = data.mozOrigin();
  mozOrigin.x = mozOrigin.x * cssPerDev;
  mozOrigin.y = mozOrigin.y * cssPerDev;
  gfxPoint3D perspectiveOrigin = data.perspectiveOrigin();
  perspectiveOrigin.x = perspectiveOrigin.x * cssPerDev;
  perspectiveOrigin.y = perspectiveOrigin.y * cssPerDev;
  nsDisplayTransform::FrameTransformProperties props(interpolatedList,
                                                     mozOrigin,
                                                     perspectiveOrigin,
                                                     data.perspective());
  gfx3DMatrix transform =
    nsDisplayTransform::GetResultingTransformMatrix(props, origin,
                                                    data.appUnitsPerDevPixel(),
                                                    &data.bounds());
  gfxPoint3D scaledOrigin =
    gfxPoint3D(NS_round(NSAppUnitsToFloatPixels(origin.x, data.appUnitsPerDevPixel())),
               NS_round(NSAppUnitsToFloatPixels(origin.y, data.appUnitsPerDevPixel())),
               0.0f);

  transform.Translate(scaledOrigin);

  InfallibleTArray<TransformFunction> functions;
  functions.AppendElement(TransformMatrix(transform));
  *aValue = functions;
}

static bool
SampleAnimations(Layer* aLayer, TimeStamp aPoint)
{
  AnimationArray& animations = aLayer->GetAnimations();
  InfallibleTArray<AnimData>& animationData = aLayer->GetAnimationData();

  bool activeAnimations = false;

  for (uint32_t i = animations.Length(); i-- !=0; ) {
    Animation& animation = animations[i];
    AnimData& animData = animationData[i];

    double numIterations = animation.numIterations() != -1 ?
      animation.numIterations() : NS_IEEEPositiveInfinity();
    double positionInIteration =
      ElementAnimations::GetPositionInIteration(aPoint - animation.startTime(),
                                                animation.duration(),
                                                numIterations,
                                                animation.direction());

    NS_ABORT_IF_FALSE(0.0 <= positionInIteration &&
                      positionInIteration <= 1.0,
                      "position should be in [0-1]");

    int segmentIndex = 0;
    AnimationSegment* segment = animation.segments().Elements();
    while (segment->endPortion() < positionInIteration) {
      ++segment;
      ++segmentIndex;
    }

    double positionInSegment = (positionInIteration - segment->startPortion()) /
                                 (segment->endPortion() - segment->startPortion());

    double portion = animData.mFunctions[segmentIndex]->GetValue(positionInSegment);

    activeAnimations = true;

    
    Animatable interpolatedValue;
    SampleValue(portion, animation, animData.mStartValues[segmentIndex],
                animData.mEndValues[segmentIndex], &interpolatedValue);
    LayerComposite* layerComposite = aLayer->AsLayerComposite();
    switch (animation.property()) {
    case eCSSProperty_opacity:
    {
      layerComposite->SetShadowOpacity(interpolatedValue.get_float());
      break;
    }
    case eCSSProperty_transform:
    {
      gfx3DMatrix matrix = interpolatedValue.get_ArrayOfTransformFunction()[0].get_TransformMatrix().value();
      if (ContainerLayer* c = aLayer->AsContainerLayer()) {
        matrix.ScalePost(c->GetInheritedXScale(),
                         c->GetInheritedYScale(),
                         1);
      }
      layerComposite->SetShadowTransform(matrix);
      layerComposite->SetShadowTransformSetByAnimation(true);
      break;
    }
    default:
      NS_WARNING("Unhandled animated property");
    }
  }

  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    activeAnimations |= SampleAnimations(child, aPoint);
  }

  return activeAnimations;
}

bool
AsyncCompositionManager::ApplyAsyncContentTransformToTree(TimeStamp aCurrentFrame,
                                                          Layer *aLayer,
                                                          bool* aWantNextFrame)
{
  bool appliedTransform = false;
  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    appliedTransform |=
      ApplyAsyncContentTransformToTree(aCurrentFrame, child, aWantNextFrame);
  }

  ContainerLayer* container = aLayer->AsContainerLayer();
  if (!container) {
    return appliedTransform;
  }

  if (AsyncPanZoomController* controller = aLayer->GetAsyncPanZoomController()) {
    LayerComposite* layerComposite = aLayer->AsLayerComposite();

    ViewTransform treeTransform;
    gfx::Point scrollOffset;
    *aWantNextFrame |=
      controller->SampleContentTransformForFrame(aCurrentFrame,
                                                 container,
                                                 &treeTransform,
                                                 scrollOffset);

    const gfx3DMatrix& rootTransform = mLayerManager->GetRoot()->GetTransform();
    const FrameMetrics& metrics = container->GetFrameMetrics();
    gfx::Rect displayPortLayersPixels(metrics.mCriticalDisplayPort.IsEmpty() ?
                                      metrics.mDisplayPort : metrics.mCriticalDisplayPort);
    gfx::Margin fixedLayerMargins(0, 0, 0, 0);
    ScreenPoint offset(0, 0);
    SyncFrameMetrics(scrollOffset, treeTransform.mScale.width, metrics.mScrollableRect,
                     mLayersUpdated, displayPortLayersPixels, 1 / rootTransform.GetXScale(),
                     mIsFirstPaint, fixedLayerMargins, offset);

    mIsFirstPaint = false;
    mLayersUpdated = false;

    
    mLayerManager->GetCompositor()->SetScreenRenderOffset(offset);

    gfx3DMatrix transform(gfx3DMatrix(treeTransform) * aLayer->GetTransform());
    
    
    
    transform.Scale(1.0f/container->GetPreXScale(),
                    1.0f/container->GetPreYScale(),
                    1);
    transform.ScalePost(1.0f/aLayer->GetPostXScale(),
                        1.0f/aLayer->GetPostYScale(),
                        1);
    layerComposite->SetShadowTransform(transform);
    NS_ASSERTION(!layerComposite->GetShadowTransformSetByAnimation(),
                 "overwriting animated transform!");

    TransformFixedLayers(
      aLayer,
      -treeTransform.mTranslation / treeTransform.mScale,
      treeTransform.mScale,
      fixedLayerMargins);

    appliedTransform = true;
  }

  return appliedTransform;
}

void
AsyncCompositionManager::TransformScrollableLayer(Layer* aLayer, const gfx3DMatrix& aRootTransform)
{
  LayerComposite* layerComposite = aLayer->AsLayerComposite();
  ContainerLayer* container = aLayer->AsContainerLayer();

  const FrameMetrics& metrics = container->GetFrameMetrics();
  
  
  const gfx3DMatrix& currentTransform = aLayer->GetTransform();

  gfx3DMatrix treeTransform;

  float layerPixelRatioX = 1 / aRootTransform.GetXScale(),
        layerPixelRatioY = 1 / aRootTransform.GetYScale();

  LayerIntPoint scrollOffsetLayerPixels = LayerIntPoint::FromCSSPointRounded(
    metrics.mScrollOffset, layerPixelRatioX, layerPixelRatioY);

  if (mIsFirstPaint) {
    mContentRect = metrics.mContentRect;
    SetFirstPaintViewport(scrollOffsetLayerPixels,
                          layerPixelRatioX,
                          mContentRect,
                          metrics.mScrollableRect);
    mIsFirstPaint = false;
  } else if (!metrics.mContentRect.IsEqualEdges(mContentRect)) {
    mContentRect = metrics.mContentRect;
    SetPageRect(metrics.mScrollableRect);
  }

  
  
  
  LayerIntRect displayPort = LayerIntRect::FromCSSRectRounded(
    CSSRect::FromUnknownRect(metrics.mCriticalDisplayPort.IsEmpty()
                             ? metrics.mDisplayPort
                             : metrics.mCriticalDisplayPort),
    layerPixelRatioX, layerPixelRatioY);

  displayPort.x += scrollOffsetLayerPixels.x;
  displayPort.y += scrollOffsetLayerPixels.y;

  gfx::Margin fixedLayerMargins(0, 0, 0, 0);
  ScreenPoint offset(0, 0);
  ScreenPoint scrollOffset(0, 0);
  float scaleX = 1.0,
        scaleY = 1.0;
  SyncViewportInfo(displayPort, layerPixelRatioX, mLayersUpdated,
                   scrollOffset, scaleX, scaleY, fixedLayerMargins,
                   offset);
  mLayersUpdated = false;

  
  mLayerManager->GetCompositor()->SetScreenRenderOffset(offset);

  
  
  
  
  
  
  float tempScaleDiffX = aRootTransform.GetXScale() * scaleX;
  float tempScaleDiffY = aRootTransform.GetYScale() * scaleY;

  LayerIntPoint metricsScrollOffset(0, 0);
  if (metrics.IsScrollable()) {
    metricsScrollOffset = scrollOffsetLayerPixels;
  }

  nsIntPoint scrollCompensation(
    (scrollOffset.x / tempScaleDiffX - metricsScrollOffset.x) * scaleX,
    (scrollOffset.y / tempScaleDiffY - metricsScrollOffset.y) * scaleY);
  treeTransform = gfx3DMatrix(ViewTransform(-scrollCompensation,
                                            gfxSize(scaleX, scaleY)));

  
  
  gfxPoint fixedOffset;
  gfxSize scaleDiff;

  
  
  
  if (mContentRect.width * tempScaleDiffX < metrics.mCompositionBounds.width) {
    fixedOffset.x = -metricsScrollOffset.x;
    scaleDiff.width = std::min(1.0f, metrics.mCompositionBounds.width / (float)mContentRect.width);
  } else {
    fixedOffset.x = clamped(scrollOffset.x / tempScaleDiffX, (float)mContentRect.x,
                       mContentRect.XMost() - metrics.mCompositionBounds.width / tempScaleDiffX) -
               metricsScrollOffset.x;
    scaleDiff.width = tempScaleDiffX;
  }

  if (mContentRect.height * tempScaleDiffY < metrics.mCompositionBounds.height) {
    fixedOffset.y = -metricsScrollOffset.y;
    scaleDiff.height = std::min(1.0f, metrics.mCompositionBounds.height / (float)mContentRect.height);
  } else {
    fixedOffset.y = clamped(scrollOffset.y / tempScaleDiffY, (float)mContentRect.y,
                       mContentRect.YMost() - metrics.mCompositionBounds.height / tempScaleDiffY) -
               metricsScrollOffset.y;
    scaleDiff.height = tempScaleDiffY;
  }

  
  
  
  gfx3DMatrix computedTransform = treeTransform * currentTransform;
  computedTransform.Scale(1.0f/container->GetPreXScale(),
                          1.0f/container->GetPreYScale(),
                          1);
  computedTransform.ScalePost(1.0f/container->GetPostXScale(),
                              1.0f/container->GetPostYScale(),
                              1);
  layerComposite->SetShadowTransform(computedTransform);
  NS_ASSERTION(!layerComposite->GetShadowTransformSetByAnimation(),
               "overwriting animated transform!");
  TransformFixedLayers(aLayer, fixedOffset, scaleDiff, fixedLayerMargins);
}

bool
AsyncCompositionManager::TransformShadowTree(TimeStamp aCurrentFrame)
{
  Layer* root = mLayerManager->GetRoot();

  
  
  bool wantNextFrame = SampleAnimations(root, aCurrentFrame);

  const gfx3DMatrix& rootTransform = root->GetTransform();

  
  
  
  
  
  
  
  
  
  
  
  if (!ApplyAsyncContentTransformToTree(aCurrentFrame, root, &wantNextFrame)) {
    nsAutoTArray<Layer*,1> scrollableLayers;
#ifdef MOZ_WIDGET_ANDROID
    scrollableLayers.AppendElement(mLayerManager->GetPrimaryScrollableLayer());
#else
    mLayerManager->GetScrollableLayers(scrollableLayers);
#endif

    for (uint32_t i = 0; i < scrollableLayers.Length(); i++) {
      if (scrollableLayers[i]) {
        TransformScrollableLayer(scrollableLayers[i], rootTransform);
      }
    }
  }

  return wantNextFrame;
}

void
AsyncCompositionManager::SetFirstPaintViewport(const LayerIntPoint& aOffset,
                                               float aZoom,
                                               const LayerIntRect& aPageRect,
                                               const CSSRect& aCssPageRect)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SetFirstPaintViewport(aOffset, aZoom, aPageRect, aCssPageRect);
#endif
}

void
AsyncCompositionManager::SetPageRect(const CSSRect& aCssPageRect)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SetPageRect(aCssPageRect);
#endif
}

void
AsyncCompositionManager::SyncViewportInfo(const LayerIntRect& aDisplayPort,
                                          float aDisplayResolution,
                                          bool aLayersUpdated,
                                          ScreenPoint& aScrollOffset,
                                          float& aScaleX, float& aScaleY,
                                          gfx::Margin& aFixedLayerMargins,
                                          ScreenPoint& aOffset)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SyncViewportInfo(aDisplayPort,
                                            aDisplayResolution,
                                            aLayersUpdated,
                                            aScrollOffset,
                                            aScaleX, aScaleY,
                                            aFixedLayerMargins,
                                            aOffset);
#endif
}

void
AsyncCompositionManager::SyncFrameMetrics(const gfx::Point& aScrollOffset,
                                          float aZoom,
                                          const CSSRect& aCssPageRect,
                                          bool aLayersUpdated,
                                          const gfx::Rect& aDisplayPort,
                                          float aDisplayResolution,
                                          bool aIsFirstPaint,
                                          gfx::Margin& aFixedLayerMargins,
                                          ScreenPoint& aOffset)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SyncFrameMetrics(aScrollOffset, aZoom, aCssPageRect,
                                            aLayersUpdated, aDisplayPort,
                                            aDisplayResolution, aIsFirstPaint,
                                            aFixedLayerMargins, aOffset);
#endif
}

} 
} 
