





#include "mozilla/layers/AsyncCompositionManager.h"
#include <stdint.h>                     
#include "CompositorParent.h"           
#include "FrameMetrics.h"               
#include "LayerManagerComposite.h"      
#include "Layers.h"                     
#include "gfxPoint.h"                   
#include "gfxPoint3D.h"                 
#include "mozilla/StyleAnimationValue.h" 
#include "mozilla/WidgetUtils.h"        
#include "mozilla/dom/AnimationPlayer.h" 
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/Compositor.h"  
#include "nsCSSPropList.h"
#include "nsCoord.h"                    
#include "nsDebug.h"                    
#include "nsDeviceContext.h"            
#include "nsDisplayList.h"              
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#if defined(MOZ_WIDGET_ANDROID)
# include <android/log.h>
# include "AndroidBridge.h"
#endif
#include "GeckoProfiler.h"

struct nsCSSValueSharedList;

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;

enum Op { Resolve, Detach };

static bool
IsSameDimension(dom::ScreenOrientation o1, dom::ScreenOrientation o2)
{
  bool isO1portrait = (o1 == dom::eScreenOrientation_PortraitPrimary || o1 == dom::eScreenOrientation_PortraitSecondary);
  bool isO2portrait = (o2 == dom::eScreenOrientation_PortraitPrimary || o2 == dom::eScreenOrientation_PortraitSecondary);
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
            bool& aReady,
            const TargetConfig& aTargetConfig)
{
  if (RefLayer* ref = aLayer->AsRefLayer()) {
    if (const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(ref->GetReferentId())) {
      if (Layer* referent = state->mRoot) {
        if (!ref->GetVisibleRegion().IsEmpty()) {
          dom::ScreenOrientation chromeOrientation = aTargetConfig.orientation();
          dom::ScreenOrientation contentOrientation = state->mTargetConfig.orientation();
          if (!IsSameDimension(chromeOrientation, contentOrientation) &&
              ContentMightReflowOnOrientationChange(aTargetConfig.clientBounds())) {
            aReady = false;
          }
        }

        if (OP == Resolve) {
          ref->ConnectReferentLayer(referent);
        } else {
          ref->DetachReferentLayer(referent);
          WalkTheTree<OP>(referent, aReady, aTargetConfig);
        }
      }
    }
  }
  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    WalkTheTree<OP>(child, aReady, aTargetConfig);
  }
}

void
AsyncCompositionManager::ResolveRefLayers()
{
  if (!mLayerManager->GetRoot()) {
    return;
  }

  mReadyForCompose = true;
  WalkTheTree<Resolve>(mLayerManager->GetRoot(),
                       mReadyForCompose,
                       mTargetConfig);
}

void
AsyncCompositionManager::DetachRefLayers()
{
  if (!mLayerManager->GetRoot()) {
    return;
  }
  WalkTheTree<Detach>(mLayerManager->GetRoot(),
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

static bool
GetBaseTransform2D(Layer* aLayer, Matrix* aTransform)
{
  
  return (aLayer->AsLayerComposite()->GetShadowTransformSetByAnimation() ?
          aLayer->GetLocalTransform() : aLayer->GetTransform()).Is2D(aTransform);
}

static void
TranslateShadowLayer2D(Layer* aLayer,
                       const gfxPoint& aTranslation)
{
  
  
  
  
  
  Matrix layerTransform;
  if (!aLayer->GetLocalTransform().Is2D(&layerTransform)) {
    return;
  }

  
  layerTransform._31 += aTranslation.x;
  layerTransform._32 += aTranslation.y;

  
  
  
  Matrix4x4 layerTransform3D = Matrix4x4::From2D(layerTransform);
  if (ContainerLayer* c = aLayer->AsContainerLayer()) {
    layerTransform3D.Scale(1.0f/c->GetPreXScale(),
                           1.0f/c->GetPreYScale(),
                           1);
  }
  layerTransform3D = layerTransform3D *
    Matrix4x4().Scale(1.0f/aLayer->GetPostXScale(),
                      1.0f/aLayer->GetPostYScale(),
                      1);

  LayerComposite* layerComposite = aLayer->AsLayerComposite();
  layerComposite->SetShadowTransform(layerTransform3D);
  layerComposite->SetShadowTransformSetByAnimation(false);

  const nsIntRect* clipRect = aLayer->GetClipRect();
  if (clipRect) {
    nsIntRect transformedClipRect(*clipRect);
    transformedClipRect.MoveBy(aTranslation.x, aTranslation.y);
    layerComposite->SetShadowClipRect(&transformedClipRect);
  }
}

static bool
AccumulateLayerTransforms2D(Layer* aLayer,
                            Layer* aAncestor,
                            Matrix& aMatrix)
{
  
  for (Layer* l = aLayer; l && l != aAncestor; l = l->GetParent()) {
    Matrix l2D;
    if (!GetBaseTransform2D(l, &l2D)) {
      return false;
    }
    aMatrix *= l2D;
  }

  return true;
}

static LayerPoint
GetLayerFixedMarginsOffset(Layer* aLayer,
                           const LayerMargin& aFixedLayerMargins)
{
  
  
  
  LayerPoint translation;
  const LayerPoint& anchor = aLayer->GetFixedPositionAnchor();
  const LayerMargin& fixedMargins = aLayer->GetFixedPositionMargins();

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

  return translation;
}

static gfxFloat
IntervalOverlap(gfxFloat aTranslation, gfxFloat aMin, gfxFloat aMax)
{
  
  
  if (aTranslation > 0) {
    return std::max(0.0, std::min(aMax, aTranslation) - std::max(aMin, 0.0));
  } else {
    return std::min(0.0, std::max(aMin, aTranslation) - std::min(aMax, 0.0));
  }
}

void
AsyncCompositionManager::AlignFixedAndStickyLayers(Layer* aLayer,
                                                   Layer* aTransformedSubtreeRoot,
                                                   const Matrix4x4& aPreviousTransformForRoot,
                                                   const Matrix4x4& aCurrentTransformForRoot,
                                                   const LayerMargin& aFixedLayerMargins)
{
  bool isRootFixed = aLayer->GetIsFixedPosition() &&
    !aLayer->GetParent()->GetIsFixedPosition();
  bool isStickyForSubtree = aLayer->GetIsStickyPosition() &&
    aTransformedSubtreeRoot->AsContainerLayer() &&
    aLayer->GetStickyScrollContainerId() ==
      aTransformedSubtreeRoot->AsContainerLayer()->GetFrameMetrics().GetScrollId();
  if (aLayer != aTransformedSubtreeRoot && (isRootFixed || isStickyForSubtree)) {
    
    
    

    
    Matrix ancestorTransform;
    if (!AccumulateLayerTransforms2D(aLayer->GetParent(), aTransformedSubtreeRoot,
                                     ancestorTransform)) {
      return;
    }

    Matrix oldRootTransform;
    Matrix newRootTransform;
    if (!aPreviousTransformForRoot.Is2D(&oldRootTransform) ||
        !aCurrentTransformForRoot.Is2D(&newRootTransform)) {
      return;
    }

    
    
    Matrix oldCumulativeTransform = ancestorTransform * oldRootTransform;
    Matrix newCumulativeTransform = ancestorTransform * newRootTransform;
    if (newCumulativeTransform.IsSingular()) {
      return;
    }
    Matrix newCumulativeTransformInverse = newCumulativeTransform;
    newCumulativeTransformInverse.Invert();

    
    
    Matrix layerTransform;
    if (!GetBaseTransform2D(aLayer, &layerTransform)) {
      return;
    }

    
    
    
    LayerPoint offsetInOldSubtreeLayerSpace = GetLayerFixedMarginsOffset(aLayer, aFixedLayerMargins);

    
    
    const LayerPoint& anchorInOldSubtreeLayerSpace = aLayer->GetFixedPositionAnchor();
    LayerPoint offsetAnchorInOldSubtreeLayerSpace = anchorInOldSubtreeLayerSpace + offsetInOldSubtreeLayerSpace;

    
    
    Point anchor(anchorInOldSubtreeLayerSpace.x, anchorInOldSubtreeLayerSpace.y);
    Point offsetAnchor(offsetAnchorInOldSubtreeLayerSpace.x, offsetAnchorInOldSubtreeLayerSpace.y);
    Point locallyTransformedAnchor = layerTransform * anchor;
    Point locallyTransformedOffsetAnchor = layerTransform * offsetAnchor;

    
    
    
    
    
    
    
    Point oldAnchorPositionInNewSpace =
      newCumulativeTransformInverse * (oldCumulativeTransform * locallyTransformedOffsetAnchor);
    Point translation = oldAnchorPositionInNewSpace - locallyTransformedAnchor;

    if (aLayer->GetIsStickyPosition()) {
      
      
      
      
      
      const LayerRect& stickyOuter = aLayer->GetStickyScrollRangeOuter();
      const LayerRect& stickyInner = aLayer->GetStickyScrollRangeInner();

      translation.y = IntervalOverlap(translation.y, stickyOuter.y, stickyOuter.YMost()) -
                      IntervalOverlap(translation.y, stickyInner.y, stickyInner.YMost());
      translation.x = IntervalOverlap(translation.x, stickyOuter.x, stickyOuter.XMost()) -
                      IntervalOverlap(translation.x, stickyInner.x, stickyInner.XMost());
    }

    
    TranslateShadowLayer2D(aLayer, ThebesPoint(translation));

    
    
    return;
  }

  
  
  
  
  if (aLayer->AsContainerLayer() &&
      aLayer->AsContainerLayer()->GetFrameMetrics().IsScrollable() &&
      aLayer != aTransformedSubtreeRoot) {
    return;
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    AlignFixedAndStickyLayers(child, aTransformedSubtreeRoot,
                              aPreviousTransformForRoot,
                              aCurrentTransformForRoot, aFixedLayerMargins);
  }
}

static void
SampleValue(float aPortion, Animation& aAnimation, StyleAnimationValue& aStart,
            StyleAnimationValue& aEnd, Animatable* aValue)
{
  StyleAnimationValue interpolatedValue;
  NS_ASSERTION(aStart.GetUnit() == aEnd.GetUnit() ||
               aStart.GetUnit() == StyleAnimationValue::eUnit_None ||
               aEnd.GetUnit() == StyleAnimationValue::eUnit_None,
               "Must have same unit");
  StyleAnimationValue::Interpolate(aAnimation.property(), aStart, aEnd,
                                aPortion, interpolatedValue);
  if (aAnimation.property() == eCSSProperty_opacity) {
    *aValue = interpolatedValue.GetFloatValue();
    return;
  }

  nsCSSValueSharedList* interpolatedList =
    interpolatedValue.GetCSSValueSharedListValue();

  TransformData& data = aAnimation.data().get_TransformData();
  nsPoint origin = data.origin();
  
  
  double cssPerDev = double(nsDeviceContext::AppUnitsPerCSSPixel())
                     / double(data.appUnitsPerDevPixel());
  gfxPoint3D transformOrigin = data.transformOrigin();
  transformOrigin.x = transformOrigin.x * cssPerDev;
  transformOrigin.y = transformOrigin.y * cssPerDev;
  gfxPoint3D perspectiveOrigin = data.perspectiveOrigin();
  perspectiveOrigin.x = perspectiveOrigin.x * cssPerDev;
  perspectiveOrigin.y = perspectiveOrigin.y * cssPerDev;
  nsDisplayTransform::FrameTransformProperties props(interpolatedList,
                                                     transformOrigin,
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
  functions.AppendElement(TransformMatrix(ToMatrix4x4(transform)));
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

    activeAnimations = true;

    TimeDuration elapsedDuration = aPoint - animation.startTime();
    
    
    
    
    
    
    
    
    
    if (elapsedDuration.ToSeconds() < 0) {
      continue;
    }

    AnimationTiming timing;
    timing.mIterationDuration = animation.duration();
    
    
    timing.mDelay = TimeDuration(0);
    timing.mIterationCount = animation.iterationCount();
    timing.mDirection = animation.direction();
    
    
    
    
    timing.mFillMode = NS_STYLE_ANIMATION_FILL_MODE_BOTH;

    ComputedTiming computedTiming =
      dom::AnimationPlayer::GetComputedTimingAt(
        Nullable<TimeDuration>(elapsedDuration), timing);

    NS_ABORT_IF_FALSE(0.0 <= computedTiming.mTimeFraction &&
                      computedTiming.mTimeFraction <= 1.0,
                      "time fraction should be in [0-1]");

    int segmentIndex = 0;
    AnimationSegment* segment = animation.segments().Elements();
    while (segment->endPortion() < computedTiming.mTimeFraction) {
      ++segment;
      ++segmentIndex;
    }

    double positionInSegment =
      (computedTiming.mTimeFraction - segment->startPortion()) /
      (segment->endPortion() - segment->startPortion());

    double portion =
      animData.mFunctions[segmentIndex]->GetValue(positionInSegment);

    
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
      Matrix4x4 matrix = interpolatedValue.get_ArrayOfTransformFunction()[0].get_TransformMatrix().value();
      if (ContainerLayer* c = aLayer->AsContainerLayer()) {
        matrix = matrix * Matrix4x4().Scale(c->GetInheritedXScale(),
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

Matrix4x4
AdjustAndCombineWithCSSTransform(const Matrix4x4& asyncTransform, Layer* aLayer)
{
  Matrix4x4 result = asyncTransform;

  
  
  
  
  
  
  if (const nsIntRect* shadowClipRect = aLayer->AsLayerComposite()->GetShadowClipRect()) {
    if (shadowClipRect->TopLeft() != nsIntPoint()) {  
      result.ChangeBasis(shadowClipRect->x, shadowClipRect->y, 0);
    }
  }

  
  result = result * aLayer->GetTransform();
  return result;
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

  if (AsyncPanZoomController* controller = container->GetAsyncPanZoomController()) {
    LayerComposite* layerComposite = aLayer->AsLayerComposite();
    Matrix4x4 oldTransform = aLayer->GetTransform();

    ViewTransform asyncTransformWithoutOverscroll, overscrollTransform;
    ScreenPoint scrollOffset;
    *aWantNextFrame |=
      controller->SampleContentTransformForFrame(aCurrentFrame,
                                                 &asyncTransformWithoutOverscroll,
                                                 scrollOffset,
                                                 &overscrollTransform);

    const FrameMetrics& metrics = container->GetFrameMetrics();
    CSSToLayerScale paintScale = metrics.LayersPixelsPerCSSPixel();
    CSSRect displayPort(metrics.mCriticalDisplayPort.IsEmpty() ?
                        metrics.mDisplayPort : metrics.mCriticalDisplayPort);
    LayerMargin fixedLayerMargins(0, 0, 0, 0);
    ScreenPoint offset(0, 0);
    SyncFrameMetrics(scrollOffset, asyncTransformWithoutOverscroll.mScale.scale,
                     metrics.mScrollableRect, mLayersUpdated, displayPort,
                     paintScale, mIsFirstPaint, fixedLayerMargins, offset);

    mIsFirstPaint = false;
    mLayersUpdated = false;

    
    mLayerManager->GetCompositor()->SetScreenRenderOffset(offset);

    Matrix4x4 transform = AdjustAndCombineWithCSSTransform(
        asyncTransformWithoutOverscroll * overscrollTransform, aLayer);

    
    
    
    transform.Scale(1.0f/container->GetPreXScale(),
                    1.0f/container->GetPreYScale(),
                    1);
    transform = transform * Matrix4x4().Scale(1.0f/aLayer->GetPostXScale(),
                                              1.0f/aLayer->GetPostYScale(),
                                              1);
    layerComposite->SetShadowTransform(transform);
    NS_ASSERTION(!layerComposite->GetShadowTransformSetByAnimation(),
                 "overwriting animated transform!");

    
    
    LayoutDeviceToLayerScale resolution = metrics.mCumulativeResolution;
    oldTransform.Scale(resolution.scale, resolution.scale, 1);

    
    
    
    
    Matrix4x4 transformWithoutOverscroll = AdjustAndCombineWithCSSTransform(
        asyncTransformWithoutOverscroll, aLayer);
    AlignFixedAndStickyLayers(aLayer, aLayer, oldTransform,
                              transformWithoutOverscroll, fixedLayerMargins);

    appliedTransform = true;
  }

  if (container->GetScrollbarDirection() != Layer::NONE) {
    ApplyAsyncTransformToScrollbar(aCurrentFrame, container);
  }
  return appliedTransform;
}

static bool
LayerHasNonContainerDescendants(ContainerLayer* aContainer)
{
  for (Layer* child = aContainer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    ContainerLayer* container = child->AsContainerLayer();
    if (!container || LayerHasNonContainerDescendants(container)) {
      return true;
    }
  }

  return false;
}

static bool
LayerIsContainerForScrollbarTarget(Layer* aTarget, ContainerLayer* aScrollbar)
{
  if (!aTarget->AsContainerLayer()) {
    return false;
  }
  AsyncPanZoomController* apzc = aTarget->AsContainerLayer()->GetAsyncPanZoomController();
  if (!apzc) {
    return false;
  }
  const FrameMetrics& metrics = aTarget->AsContainerLayer()->GetFrameMetrics();
  if (metrics.GetScrollId() != aScrollbar->GetScrollbarTargetContainerId()) {
    return false;
  }
  return true;
}

static void
ApplyAsyncTransformToScrollbarForContent(TimeStamp aCurrentFrame, ContainerLayer* aScrollbar,
                                         Layer* aContent, bool aScrollbarIsChild)
{
  ContainerLayer* content = aContent->AsContainerLayer();

  
  
  
  
  
  if (!LayerHasNonContainerDescendants(content)) {
    return;
  }

  const FrameMetrics& metrics = content->GetFrameMetrics();
  AsyncPanZoomController* apzc = content->GetAsyncPanZoomController();

  if (aScrollbarIsChild) {
    
    
    
    
    
    
    ViewTransform asyncTransform;
    ScreenPoint scrollOffset;
    apzc->SampleContentTransformForFrame(aCurrentFrame, &asyncTransform, scrollOffset);
  }

  Matrix4x4 asyncTransform = apzc->GetCurrentAsyncTransform();
  Matrix4x4 nontransientTransform = apzc->GetNontransientAsyncTransform();
  Matrix4x4 nontransientUntransform = nontransientTransform;
  nontransientUntransform.Invert();
  Matrix4x4 transientTransform = asyncTransform * nontransientUntransform;

  
  
  
  
  
  
  
  
  
  
  
  
  
  Matrix4x4 scrollbarTransform;
  if (aScrollbar->GetScrollbarDirection() == Layer::VERTICAL) {
    float scale = metrics.CalculateCompositedSizeInCssPixels().height / metrics.mScrollableRect.height;
    scrollbarTransform = scrollbarTransform * Matrix4x4().Scale(1.f, 1.f / transientTransform._22, 1.f);
    scrollbarTransform = scrollbarTransform * Matrix4x4().Translate(0, -transientTransform._42 * scale, 0);
  }
  if (aScrollbar->GetScrollbarDirection() == Layer::HORIZONTAL) {
    float scale = metrics.CalculateCompositedSizeInCssPixels().width / metrics.mScrollableRect.width;
    scrollbarTransform = scrollbarTransform * Matrix4x4().Scale(1.f / transientTransform._11, 1.f, 1.f);
    scrollbarTransform = scrollbarTransform * Matrix4x4().Translate(-transientTransform._41 * scale, 0, 0);
  }

  Matrix4x4 transform = scrollbarTransform * aScrollbar->GetTransform();

  if (aScrollbarIsChild) {
    
    
    
    
    
    transientTransform.Invert();
    transform = transform * transientTransform;
  }

  
  
  
  transform.Scale(1.0f/aScrollbar->GetPreXScale(),
                  1.0f/aScrollbar->GetPreYScale(),
                  1);
  transform = transform * Matrix4x4().Scale(1.0f/aScrollbar->GetPostXScale(),
                                            1.0f/aScrollbar->GetPostYScale(),
                                            1);
  aScrollbar->AsLayerComposite()->SetShadowTransform(transform);
}

static Layer*
FindScrolledLayerForScrollbar(ContainerLayer* aLayer, bool* aOutIsAncestor)
{
  
  for (Layer* ancestor = aLayer; ancestor; ancestor = ancestor->GetParent()) {
    for (Layer* scrollTarget = ancestor;
         scrollTarget;
         scrollTarget = scrollTarget->GetPrevSibling()) {
      if (scrollTarget != aLayer &&
          LayerIsContainerForScrollbarTarget(scrollTarget, aLayer)) {
        *aOutIsAncestor = (scrollTarget == ancestor);
        return scrollTarget;
      }
    }
    for (Layer* scrollTarget = ancestor->GetNextSibling();
         scrollTarget;
         scrollTarget = scrollTarget->GetNextSibling()) {
      if (LayerIsContainerForScrollbarTarget(scrollTarget, aLayer)) {
        *aOutIsAncestor = false;
        return scrollTarget;
      }
    }
  }
  return nullptr;
}

void
AsyncCompositionManager::ApplyAsyncTransformToScrollbar(TimeStamp aCurrentFrame, ContainerLayer* aLayer)
{
  
  
  
  
  
  
  
  bool isAncestor = false;
  Layer* scrollTarget = FindScrolledLayerForScrollbar(aLayer, &isAncestor);
  if (scrollTarget) {
    ApplyAsyncTransformToScrollbarForContent(aCurrentFrame, aLayer, scrollTarget,
                                             isAncestor);
  }
}

void
AsyncCompositionManager::TransformScrollableLayer(Layer* aLayer)
{
  LayerComposite* layerComposite = aLayer->AsLayerComposite();
  ContainerLayer* container = aLayer->AsContainerLayer();

  const FrameMetrics& metrics = container->GetFrameMetrics();
  
  
  Matrix4x4 oldTransform = aLayer->GetTransform();

  CSSToLayerScale geckoZoom = metrics.LayersPixelsPerCSSPixel();

  LayerIntPoint scrollOffsetLayerPixels = RoundedToInt(metrics.GetScrollOffset() * geckoZoom);

  if (mIsFirstPaint) {
    mContentRect = metrics.mScrollableRect;
    SetFirstPaintViewport(scrollOffsetLayerPixels,
                          geckoZoom,
                          mContentRect);
    mIsFirstPaint = false;
  } else if (!metrics.mScrollableRect.IsEqualEdges(mContentRect)) {
    mContentRect = metrics.mScrollableRect;
    SetPageRect(mContentRect);
  }

  
  
  
  LayerIntRect displayPort = RoundedToInt(
    (metrics.mCriticalDisplayPort.IsEmpty()
      ? metrics.mDisplayPort
      : metrics.mCriticalDisplayPort
    ) * geckoZoom);
  displayPort += scrollOffsetLayerPixels;

  LayerMargin fixedLayerMargins(0, 0, 0, 0);
  ScreenPoint offset(0, 0);

  
  
  
  
  
  CSSToScreenScale userZoom(metrics.mDevPixelsPerCSSPixel * metrics.mCumulativeResolution * LayerToScreenScale(1));
  ScreenPoint userScroll = metrics.GetScrollOffset() * userZoom;
  SyncViewportInfo(displayPort, geckoZoom, mLayersUpdated,
                   userScroll, userZoom, fixedLayerMargins,
                   offset);
  mLayersUpdated = false;

  
  mLayerManager->GetCompositor()->SetScreenRenderOffset(offset);

  
  
  
  
  
  
  LayerToScreenScale zoomAdjust = userZoom / geckoZoom;

  LayerPoint geckoScroll(0, 0);
  if (metrics.IsScrollable()) {
    geckoScroll = metrics.GetScrollOffset() * geckoZoom;
  }

  LayerPoint translation = (userScroll / zoomAdjust) - geckoScroll;
  Matrix4x4 treeTransform = ViewTransform(-translation,
                                            userZoom
                                          / metrics.mDevPixelsPerCSSPixel
                                          / metrics.GetParentResolution());

  
  
  
  Matrix4x4 computedTransform = treeTransform * oldTransform;
  computedTransform.Scale(1.0f/container->GetPreXScale(),
                          1.0f/container->GetPreYScale(),
                          1);
  computedTransform.ScalePost(1.0f/container->GetPostXScale(),
                              1.0f/container->GetPostYScale(),
                              1);
  layerComposite->SetShadowTransform(computedTransform);
  NS_ASSERTION(!layerComposite->GetShadowTransformSetByAnimation(),
               "overwriting animated transform!");

  
  
  oldTransform.Scale(metrics.mResolution.scale, metrics.mResolution.scale, 1);

  
  
  
  
  
  ScreenRect contentScreenRect = mContentRect * userZoom;
  gfxPoint3D overscrollTranslation;
  if (userScroll.x < contentScreenRect.x) {
    overscrollTranslation.x = contentScreenRect.x - userScroll.x;
  } else if (userScroll.x + metrics.mCompositionBounds.width > contentScreenRect.XMost()) {
    overscrollTranslation.x = contentScreenRect.XMost() -
      (userScroll.x + metrics.mCompositionBounds.width);
  }
  if (userScroll.y < contentScreenRect.y) {
    overscrollTranslation.y = contentScreenRect.y - userScroll.y;
  } else if (userScroll.y + metrics.mCompositionBounds.height > contentScreenRect.YMost()) {
    overscrollTranslation.y = contentScreenRect.YMost() -
      (userScroll.y + metrics.mCompositionBounds.height);
  }
  oldTransform.Translate(overscrollTranslation.x,
                         overscrollTranslation.y,
                         overscrollTranslation.z);

  gfx::Size underZoomScale(1.0f, 1.0f);
  if (mContentRect.width * userZoom.scale < metrics.mCompositionBounds.width) {
    underZoomScale.width = (mContentRect.width * userZoom.scale) /
      metrics.mCompositionBounds.width;
  }
  if (mContentRect.height * userZoom.scale < metrics.mCompositionBounds.height) {
    underZoomScale.height = (mContentRect.height * userZoom.scale) /
      metrics.mCompositionBounds.height;
  }
  oldTransform.Scale(underZoomScale.width, underZoomScale.height, 1);

  
  
  AlignFixedAndStickyLayers(aLayer, aLayer, oldTransform,
                            aLayer->GetLocalTransform(), fixedLayerMargins);
}

void
ClearAsyncTransforms(Layer* aLayer)
{
  if (!aLayer->AsLayerComposite()->GetShadowTransformSetByAnimation()) {
    aLayer->AsLayerComposite()->SetShadowTransform(aLayer->GetBaseTransform());
  }
  for (Layer* child = aLayer->GetFirstChild();
      child; child = child->GetNextSibling()) {
    ClearAsyncTransforms(child);
  }
}

bool
AsyncCompositionManager::TransformShadowTree(TimeStamp aCurrentFrame)
{
  PROFILER_LABEL("AsyncCompositionManager", "TransformShadowTree",
    js::ProfileEntry::Category::GRAPHICS);

  Layer* root = mLayerManager->GetRoot();
  if (!root) {
    return false;
  }

  
  
  bool wantNextFrame = SampleAnimations(root, aCurrentFrame);

  
  
  
  
  ClearAsyncTransforms(root);

  
  
  
  
  
  
  
  
  
  
  
  if (!ApplyAsyncContentTransformToTree(aCurrentFrame, root, &wantNextFrame)) {
    nsAutoTArray<Layer*,1> scrollableLayers;
#ifdef MOZ_WIDGET_ANDROID
    scrollableLayers.AppendElement(mLayerManager->GetPrimaryScrollableLayer());
#else
    mLayerManager->GetScrollableLayers(scrollableLayers);
#endif

    for (uint32_t i = 0; i < scrollableLayers.Length(); i++) {
      if (scrollableLayers[i]) {
        TransformScrollableLayer(scrollableLayers[i]);
      }
    }
  }

  return wantNextFrame;
}

void
AsyncCompositionManager::SetFirstPaintViewport(const LayerIntPoint& aOffset,
                                               const CSSToLayerScale& aZoom,
                                               const CSSRect& aCssPageRect)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SetFirstPaintViewport(aOffset, aZoom, aCssPageRect);
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
                                          const CSSToLayerScale& aDisplayResolution,
                                          bool aLayersUpdated,
                                          ScreenPoint& aScrollOffset,
                                          CSSToScreenScale& aScale,
                                          LayerMargin& aFixedLayerMargins,
                                          ScreenPoint& aOffset)
{
#ifdef MOZ_WIDGET_ANDROID
  AndroidBridge::Bridge()->SyncViewportInfo(aDisplayPort,
                                            aDisplayResolution,
                                            aLayersUpdated,
                                            aScrollOffset,
                                            aScale,
                                            aFixedLayerMargins,
                                            aOffset);
#endif
}

void
AsyncCompositionManager::SyncFrameMetrics(const ScreenPoint& aScrollOffset,
                                          float aZoom,
                                          const CSSRect& aCssPageRect,
                                          bool aLayersUpdated,
                                          const CSSRect& aDisplayPort,
                                          const CSSToLayerScale& aDisplayResolution,
                                          bool aIsFirstPaint,
                                          LayerMargin& aFixedLayerMargins,
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
