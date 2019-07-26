





#include "mozilla/layers/AsyncCompositionManager.h"
#include <stdint.h>                     
#include "AnimationCommon.h"            
#include "CompositorParent.h"           
#include "FrameMetrics.h"               
#include "LayerManagerComposite.h"      
#include "Layers.h"                     
#include "gfxMatrix.h"                  
#include "gfxPoint.h"                   
#include "gfxPoint3D.h"                 
#include "mozilla/WidgetUtils.h"        
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/Compositor.h"  
#include "nsAnimationManager.h"         
#include "nsCSSPropList.h"
#include "nsCoord.h"                    
#include "nsDebug.h"                    
#include "nsDeviceContext.h"            
#include "nsDisplayList.h"              
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   
#include "nsStyleAnimation.h"           
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#if defined(MOZ_WIDGET_ANDROID)
# include <android/log.h>
# include "AndroidBridge.h"
#endif
#include "GeckoProfiler.h"

struct nsCSSValueSharedList;

using namespace mozilla::dom;
using namespace mozilla::gfx;

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
        } else {
          ref->DetachReferentLayer(referent);
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
  WalkTheTree<Resolve>(mLayerManager->GetRoot(),
                       mReadyForCompose,
                       mTargetConfig);
}

void
AsyncCompositionManager::DetachRefLayers()
{
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
GetBaseTransform2D(Layer* aLayer, gfxMatrix* aTransform)
{
  
  gfx3DMatrix localTransform;
  To3DMatrix(aLayer->GetLocalTransform(), localTransform);
  return (aLayer->AsLayerComposite()->GetShadowTransformSetByAnimation() ?
          localTransform : aLayer->GetTransform()).Is2D(aTransform);
}

static void
TranslateShadowLayer2D(Layer* aLayer,
                       const gfxPoint& aTranslation)
{
  gfxMatrix layerTransform;
  if (!GetBaseTransform2D(aLayer, &layerTransform)) {
    return;
  }

  
  layerTransform.x0 += aTranslation.x;
  layerTransform.y0 += aTranslation.y;

  
  
  
  gfx3DMatrix layerTransform3D = gfx3DMatrix::From2D(layerTransform);
  if (ContainerLayer* c = aLayer->AsContainerLayer()) {
    layerTransform3D.Scale(1.0f/c->GetPreXScale(),
                           1.0f/c->GetPreYScale(),
                           1);
  }
  layerTransform3D.ScalePost(1.0f/aLayer->GetPostXScale(),
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
                            gfxMatrix& aMatrix)
{
  
  for (Layer* l = aLayer; l && l != aAncestor; l = l->GetParent()) {
    gfxMatrix l2D;
    if (!GetBaseTransform2D(l, &l2D)) {
      return false;
    }
    aMatrix.Multiply(l2D);
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
                                                   const gfx3DMatrix& aPreviousTransformForRoot,
                                                   const LayerMargin& aFixedLayerMargins)
{
  bool isRootFixed = aLayer->GetIsFixedPosition() &&
    !aLayer->GetParent()->GetIsFixedPosition();
  bool isStickyForSubtree = aLayer->GetIsStickyPosition() &&
    aTransformedSubtreeRoot->AsContainerLayer() &&
    aLayer->GetStickyScrollContainerId() ==
      aTransformedSubtreeRoot->AsContainerLayer()->GetFrameMetrics().mScrollId;
  if (aLayer != aTransformedSubtreeRoot && (isRootFixed || isStickyForSubtree)) {
    
    
    

    
    gfxMatrix ancestorTransform;
    if (!AccumulateLayerTransforms2D(aLayer->GetParent(), aTransformedSubtreeRoot,
                                     ancestorTransform)) {
      return;
    }

    gfxMatrix oldRootTransform;
    Matrix newRootTransform;
    if (!aPreviousTransformForRoot.Is2D(&oldRootTransform) ||
        !aTransformedSubtreeRoot->GetLocalTransform().Is2D(&newRootTransform)) {
      return;
    }

    
    
    gfxMatrix oldCumulativeTransform = ancestorTransform * oldRootTransform;
    gfxMatrix newCumulativeTransform = ancestorTransform * ThebesMatrix(newRootTransform);
    if (newCumulativeTransform.IsSingular()) {
      return;
    }
    gfxMatrix newCumulativeTransformInverse = newCumulativeTransform;
    newCumulativeTransformInverse.Invert();

    
    
    gfxMatrix layerTransform;
    if (!GetBaseTransform2D(aLayer, &layerTransform)) {
      return;
    }

    
    
    
    LayerPoint offsetInOldSubtreeLayerSpace = GetLayerFixedMarginsOffset(aLayer, aFixedLayerMargins);

    
    
    const LayerPoint& anchorInOldSubtreeLayerSpace = aLayer->GetFixedPositionAnchor();
    LayerPoint offsetAnchorInOldSubtreeLayerSpace = anchorInOldSubtreeLayerSpace + offsetInOldSubtreeLayerSpace;

    
    
    gfxPoint anchor(anchorInOldSubtreeLayerSpace.x, anchorInOldSubtreeLayerSpace.y);
    gfxPoint offsetAnchor(offsetAnchorInOldSubtreeLayerSpace.x, offsetAnchorInOldSubtreeLayerSpace.y);
    gfxPoint locallyTransformedAnchor = layerTransform.Transform(anchor);
    gfxPoint locallyTransformedOffsetAnchor = layerTransform.Transform(offsetAnchor);

    
    
    
    
    
    
    
    gfxPoint oldAnchorPositionInNewSpace =
      newCumulativeTransformInverse.Transform(
        oldCumulativeTransform.Transform(locallyTransformedOffsetAnchor));
    gfxPoint translation = oldAnchorPositionInNewSpace - locallyTransformedAnchor;

    if (aLayer->GetIsStickyPosition()) {
      
      
      
      
      
      const LayerRect& stickyOuter = aLayer->GetStickyScrollRangeOuter();
      const LayerRect& stickyInner = aLayer->GetStickyScrollRangeInner();

      translation.y = IntervalOverlap(translation.y, stickyOuter.y, stickyOuter.YMost()) -
                      IntervalOverlap(translation.y, stickyInner.y, stickyInner.YMost());
      translation.x = IntervalOverlap(translation.x, stickyOuter.x, stickyOuter.XMost()) -
                      IntervalOverlap(translation.x, stickyInner.x, stickyInner.XMost());
    }

    
    TranslateShadowLayer2D(aLayer, translation);

    
    
    return;
  }

  
  
  
  if (aLayer->AsContainerLayer() &&
      aLayer->AsContainerLayer()->GetFrameMetrics().IsScrollable() &&
      aLayer != aTransformedSubtreeRoot) {
    AlignFixedAndStickyLayers(aLayer, aLayer, aLayer->GetTransform(), LayerMargin(0, 0, 0, 0));
    return;
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    AlignFixedAndStickyLayers(child, aTransformedSubtreeRoot,
                              aPreviousTransformForRoot, aFixedLayerMargins);
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

  if (AsyncPanZoomController* controller = container->GetAsyncPanZoomController()) {
    LayerComposite* layerComposite = aLayer->AsLayerComposite();
    gfx3DMatrix oldTransform = aLayer->GetTransform();

    ViewTransform treeTransform;
    ScreenPoint scrollOffset;
    *aWantNextFrame |=
      controller->SampleContentTransformForFrame(aCurrentFrame,
                                                 &treeTransform,
                                                 scrollOffset);

    const FrameMetrics& metrics = container->GetFrameMetrics();
    CSSToLayerScale paintScale = metrics.LayersPixelsPerCSSPixel();
    CSSRect displayPort(metrics.mCriticalDisplayPort.IsEmpty() ?
                        metrics.mDisplayPort : metrics.mCriticalDisplayPort);
    LayerMargin fixedLayerMargins(0, 0, 0, 0);
    ScreenPoint offset(0, 0);
    SyncFrameMetrics(scrollOffset, treeTransform.mScale.scale, metrics.mScrollableRect,
                     mLayersUpdated, displayPort, paintScale,
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

    
    
    LayoutDeviceToLayerScale resolution = metrics.mCumulativeResolution;
    oldTransform.Scale(resolution.scale, resolution.scale, 1);

    AlignFixedAndStickyLayers(aLayer, aLayer, oldTransform, fixedLayerMargins);

    appliedTransform = true;
  }

  if (container->GetScrollbarDirection() != Layer::NONE) {
    ApplyAsyncTransformToScrollbar(container);
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

void
AsyncCompositionManager::ApplyAsyncTransformToScrollbar(ContainerLayer* aLayer)
{
  
  
  
  
  
  
  
  
  
  
  
  
  for (Layer* scrollTarget = aLayer->GetPrevSibling();
       scrollTarget;
       scrollTarget = scrollTarget->GetPrevSibling()) {
    if (!scrollTarget->AsContainerLayer()) {
      continue;
    }
    AsyncPanZoomController* apzc = scrollTarget->AsContainerLayer()->GetAsyncPanZoomController();
    if (!apzc) {
      continue;
    }
    const FrameMetrics& metrics = scrollTarget->AsContainerLayer()->GetFrameMetrics();
    if (metrics.mScrollId != aLayer->GetScrollbarTargetContainerId()) {
      continue;
    }
    if (!LayerHasNonContainerDescendants(scrollTarget->AsContainerLayer())) {
      return;
    }

    gfx3DMatrix asyncTransform = gfx3DMatrix(apzc->GetCurrentAsyncTransform());
    gfx3DMatrix nontransientTransform = apzc->GetNontransientAsyncTransform();
    gfx3DMatrix transientTransform = asyncTransform * nontransientTransform.Inverse();

    gfx3DMatrix scrollbarTransform;
    if (aLayer->GetScrollbarDirection() == Layer::VERTICAL) {
      float scale = metrics.CalculateCompositedRectInCssPixels().height / metrics.mScrollableRect.height;
      scrollbarTransform.ScalePost(1.f, 1.f / transientTransform.GetYScale(), 1.f);
      scrollbarTransform.TranslatePost(gfxPoint3D(0, -transientTransform._42 * scale, 0));
    }
    if (aLayer->GetScrollbarDirection() == Layer::HORIZONTAL) {
      float scale = metrics.CalculateCompositedRectInCssPixels().width / metrics.mScrollableRect.width;
      scrollbarTransform.ScalePost(1.f / transientTransform.GetXScale(), 1.f, 1.f);
      scrollbarTransform.TranslatePost(gfxPoint3D(-transientTransform._41 * scale, 0, 0));
    }

    gfx3DMatrix transform = scrollbarTransform * aLayer->GetTransform();
    
    
    
    transform.Scale(1.0f/aLayer->GetPreXScale(),
                    1.0f/aLayer->GetPreYScale(),
                    1);
    transform.ScalePost(1.0f/aLayer->GetPostXScale(),
                        1.0f/aLayer->GetPostYScale(),
                        1);
    aLayer->AsLayerComposite()->SetShadowTransform(transform);

    return;
  }
}

void
AsyncCompositionManager::TransformScrollableLayer(Layer* aLayer)
{
  LayerComposite* layerComposite = aLayer->AsLayerComposite();
  ContainerLayer* container = aLayer->AsContainerLayer();

  const FrameMetrics& metrics = container->GetFrameMetrics();
  
  
  const gfx3DMatrix& currentTransform = aLayer->GetTransform();
  gfx3DMatrix oldTransform = currentTransform;

  gfx3DMatrix treeTransform;

  CSSToLayerScale geckoZoom = metrics.LayersPixelsPerCSSPixel();

  LayerIntPoint scrollOffsetLayerPixels = RoundedToInt(metrics.mScrollOffset * geckoZoom);

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
  ScreenPoint userScroll = metrics.mScrollOffset * userZoom;
  SyncViewportInfo(displayPort, geckoZoom, mLayersUpdated,
                   userScroll, userZoom, fixedLayerMargins,
                   offset);
  mLayersUpdated = false;

  
  mLayerManager->GetCompositor()->SetScreenRenderOffset(offset);

  
  
  
  
  
  
  LayerToScreenScale zoomAdjust = userZoom / geckoZoom;

  LayerPoint geckoScroll(0, 0);
  if (metrics.IsScrollable()) {
    geckoScroll = metrics.mScrollOffset * geckoZoom;
  }

  LayerPoint translation = (userScroll / zoomAdjust) - geckoScroll;
  treeTransform = gfx3DMatrix(ViewTransform(-translation,
                                            userZoom
                                          / metrics.mDevPixelsPerCSSPixel
                                          / metrics.GetParentResolution()));

  
  
  
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
  oldTransform.Translate(overscrollTranslation);

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

  
  
  AlignFixedAndStickyLayers(aLayer, aLayer, oldTransform, fixedLayerMargins);
}

bool
AsyncCompositionManager::TransformShadowTree(TimeStamp aCurrentFrame)
{
  PROFILER_LABEL("AsyncCompositionManager", "TransformShadowTree");
  Layer* root = mLayerManager->GetRoot();
  if (!root) {
    return false;
  }

  
  
  bool wantNextFrame = SampleAnimations(root, aCurrentFrame);

  
  
  
  
  
  
  
  
  
  
  
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
