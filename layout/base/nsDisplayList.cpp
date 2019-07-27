











#include "nsDisplayList.h"

#include <stdint.h>
#include <algorithm>

#include "mozilla/dom/TabChild.h"
#include "mozilla/layers/PLayerTransaction.h"
#include "nsCSSRendering.h"
#include "nsRenderingContext.h"
#include "nsISelectionController.h"
#include "nsIPresShell.h"
#include "nsRegion.h"
#include "nsStyleStructInlines.h"
#include "nsStyleTransformMatrix.h"
#include "gfxMatrix.h"
#include "gfxPrefs.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"
#include "nsLayoutUtils.h"
#include "nsIScrollableFrame.h"
#include "nsIFrameInlines.h"
#include "nsThemeConstants.h"
#include "LayerTreeInvalidation.h"

#include "imgIContainer.h"
#include "BasicLayers.h"
#include "nsBoxFrame.h"
#include "nsViewportFrame.h"
#include "nsSubDocumentFrame.h"
#include "nsSVGEffects.h"
#include "nsSVGElement.h"
#include "nsSVGClipPathFrame.h"
#include "GeckoProfiler.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"
#include "nsViewManager.h"
#include "ImageLayers.h"
#include "ImageContainer.h"
#include "nsCanvasFrame.h"
#include "StickyScrollContainer.h"
#include "mozilla/EventStates.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/Preferences.h"
#include "mozilla/UniquePtr.h"
#include "ActiveLayerTracker.h"
#include "nsContentUtils.h"
#include "nsPrintfCString.h"
#include "UnitTransforms.h"
#include "LayersLogging.h"
#include "FrameLayerBuilder.h"
#include "nsCaret.h"
#include "nsISelection.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::layout;
using namespace mozilla::gfx;

typedef FrameMetrics::ViewID ViewID;

#ifdef DEBUG
static bool
SpammyLayoutWarningsEnabled()
{
  static bool sValue = false;
  static bool sValueInitialized = false;

  if (!sValueInitialized) {
    Preferences::GetBool("layout.spammy_warnings.enabled", &sValue);
    sValueInitialized = true;
  }

  return sValue;
}
#endif

static inline nsIFrame*
GetTransformRootFrame(nsIFrame* aFrame)
{
  return nsLayoutUtils::GetTransformRootFrame(aFrame);
}

static void AddTransformFunctions(nsCSSValueList* aList,
                                  nsStyleContext* aContext,
                                  nsPresContext* aPresContext,
                                  nsRect& aBounds,
                                  InfallibleTArray<TransformFunction>& aFunctions)
{
  if (aList->mValue.GetUnit() == eCSSUnit_None) {
    return;
  }

  for (const nsCSSValueList* curr = aList; curr; curr = curr->mNext) {
    const nsCSSValue& currElem = curr->mValue;
    NS_ASSERTION(currElem.GetUnit() == eCSSUnit_Function,
                 "Stream should consist solely of functions!");
    nsCSSValue::Array* array = currElem.GetArrayValue();
    bool canStoreInRuleTree = true;
    switch (nsStyleTransformMatrix::TransformFunctionOf(array)) {
      case eCSSKeyword_rotatex:
      {
        double theta = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(RotationX(theta));
        break;
      }
      case eCSSKeyword_rotatey:
      {
        double theta = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(RotationY(theta));
        break;
      }
      case eCSSKeyword_rotatez:
      {
        double theta = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(RotationZ(theta));
        break;
      }
      case eCSSKeyword_rotate:
      {
        double theta = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(Rotation(theta));
        break;
      }
      case eCSSKeyword_rotate3d:
      {
        double x = array->Item(1).GetFloatValue();
        double y = array->Item(2).GetFloatValue();
        double z = array->Item(3).GetFloatValue();
        double theta = array->Item(4).GetAngleValueInRadians();
        aFunctions.AppendElement(Rotation3D(x, y, z, theta));
        break;
      }
      case eCSSKeyword_scalex:
      {
        double x = array->Item(1).GetFloatValue();
        aFunctions.AppendElement(Scale(x, 1, 1));
        break;
      }
      case eCSSKeyword_scaley:
      {
        double y = array->Item(1).GetFloatValue();
        aFunctions.AppendElement(Scale(1, y, 1));
        break;
      }
      case eCSSKeyword_scalez:
      {
        double z = array->Item(1).GetFloatValue();
        aFunctions.AppendElement(Scale(1, 1, z));
        break;
      }
      case eCSSKeyword_scale:
      {
        double x = array->Item(1).GetFloatValue();
        
        double y = array->Count() == 2 ? x : array->Item(2).GetFloatValue();
        aFunctions.AppendElement(Scale(x, y, 1));
        break;
      }
      case eCSSKeyword_scale3d:
      {
        double x = array->Item(1).GetFloatValue();
        double y = array->Item(2).GetFloatValue();
        double z = array->Item(3).GetFloatValue();
        aFunctions.AppendElement(Scale(x, y, z));
        break;
      }
      case eCSSKeyword_translatex:
      {
        double x = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(1), aContext, aPresContext, canStoreInRuleTree,
          aBounds.Width());
        aFunctions.AppendElement(Translation(x, 0, 0));
        break;
      }
      case eCSSKeyword_translatey:
      {
        double y = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(1), aContext, aPresContext, canStoreInRuleTree,
          aBounds.Height());
        aFunctions.AppendElement(Translation(0, y, 0));
        break;
      }
      case eCSSKeyword_translatez:
      {
        double z = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(1), aContext, aPresContext, canStoreInRuleTree,
          0);
        aFunctions.AppendElement(Translation(0, 0, z));
        break;
      }
      case eCSSKeyword_translate:
      {
        double x = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(1), aContext, aPresContext, canStoreInRuleTree,
          aBounds.Width());
        
        double y = 0;
        if (array->Count() == 3) {
           y = nsStyleTransformMatrix::ProcessTranslatePart(
            array->Item(2), aContext, aPresContext, canStoreInRuleTree,
            aBounds.Height());
        }
        aFunctions.AppendElement(Translation(x, y, 0));
        break;
      }
      case eCSSKeyword_translate3d:
      {
        double x = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(1), aContext, aPresContext, canStoreInRuleTree,
          aBounds.Width());
        double y = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(2), aContext, aPresContext, canStoreInRuleTree,
          aBounds.Height());
        double z = nsStyleTransformMatrix::ProcessTranslatePart(
          array->Item(3), aContext, aPresContext, canStoreInRuleTree,
          0);

        aFunctions.AppendElement(Translation(x, y, z));
        break;
      }
      case eCSSKeyword_skewx:
      {
        double x = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(SkewX(x));
        break;
      }
      case eCSSKeyword_skewy:
      {
        double y = array->Item(1).GetAngleValueInRadians();
        aFunctions.AppendElement(SkewY(y));
        break;
      }
      case eCSSKeyword_skew:
      {
        double x = array->Item(1).GetAngleValueInRadians();
        
        double y = 0;
        if (array->Count() == 3) {
          y = array->Item(2).GetAngleValueInRadians();
        }
        aFunctions.AppendElement(Skew(x, y));
        break;
      }
      case eCSSKeyword_matrix:
      {
        gfx::Matrix4x4 matrix;
        matrix._11 = array->Item(1).GetFloatValue();
        matrix._12 = array->Item(2).GetFloatValue();
        matrix._13 = 0;
        matrix._14 = 0;
        matrix._21 = array->Item(3).GetFloatValue();
        matrix._22 = array->Item(4).GetFloatValue();
        matrix._23 = 0;
        matrix._24 = 0;
        matrix._31 = 0;
        matrix._32 = 0;
        matrix._33 = 1;
        matrix._34 = 0;
        matrix._41 = array->Item(5).GetFloatValue();
        matrix._42 = array->Item(6).GetFloatValue();
        matrix._43 = 0;
        matrix._44 = 1;
        aFunctions.AppendElement(TransformMatrix(matrix));
        break;
      }
      case eCSSKeyword_matrix3d:
      {
        gfx::Matrix4x4 matrix;
        matrix._11 = array->Item(1).GetFloatValue();
        matrix._12 = array->Item(2).GetFloatValue();
        matrix._13 = array->Item(3).GetFloatValue();
        matrix._14 = array->Item(4).GetFloatValue();
        matrix._21 = array->Item(5).GetFloatValue();
        matrix._22 = array->Item(6).GetFloatValue();
        matrix._23 = array->Item(7).GetFloatValue();
        matrix._24 = array->Item(8).GetFloatValue();
        matrix._31 = array->Item(9).GetFloatValue();
        matrix._32 = array->Item(10).GetFloatValue();
        matrix._33 = array->Item(11).GetFloatValue();
        matrix._34 = array->Item(12).GetFloatValue();
        matrix._41 = array->Item(13).GetFloatValue();
        matrix._42 = array->Item(14).GetFloatValue();
        matrix._43 = array->Item(15).GetFloatValue();
        matrix._44 = array->Item(16).GetFloatValue();
        aFunctions.AppendElement(TransformMatrix(matrix));
        break;
      }
      case eCSSKeyword_interpolatematrix:
      {
        gfx3DMatrix matrix;
        nsStyleTransformMatrix::ProcessInterpolateMatrix(matrix, array,
                                                         aContext,
                                                         aPresContext,
                                                         canStoreInRuleTree,
                                                         aBounds);
        aFunctions.AppendElement(TransformMatrix(gfx::ToMatrix4x4(matrix)));
        break;
      }
      case eCSSKeyword_perspective:
      {
        aFunctions.AppendElement(Perspective(array->Item(1).GetFloatValue()));
        break;
      }
      default:
        NS_ERROR("Function not handled yet!");
    }
  }
}

static TimingFunction
ToTimingFunction(ComputedTimingFunction& aCTF)
{
  if (aCTF.GetType() == nsTimingFunction::Function) {
    const nsSMILKeySpline* spline = aCTF.GetFunction();
    return TimingFunction(CubicBezierFunction(spline->X1(), spline->Y1(),
                                              spline->X2(), spline->Y2()));
  }

  uint32_t type = aCTF.GetType() == nsTimingFunction::StepStart ? 1 : 2;
  return TimingFunction(StepFunction(aCTF.GetSteps(), type));
}

static void
AddAnimationForProperty(nsIFrame* aFrame, nsCSSProperty aProperty,
                        AnimationPlayer* aPlayer, Layer* aLayer,
                        AnimationData& aData, bool aPending)
{
  MOZ_ASSERT(aLayer->AsContainerLayer(), "Should only animate ContainerLayer");
  MOZ_ASSERT(aPlayer->GetSource(),
             "Should not be adding an animation for a player without"
             " an animation");
  nsStyleContext* styleContext = aFrame->StyleContext();
  nsPresContext* presContext = aFrame->PresContext();
  nsRect bounds = nsDisplayTransform::GetFrameBoundsForTransform(aFrame);

  layers::Animation* animation =
    aPending ?
    aLayer->AddAnimationForNextTransaction() :
    aLayer->AddAnimation();

  const AnimationTiming& timing = aPlayer->GetSource()->Timing();
  animation->startTime() = aPlayer->Timeline()->ToTimeStamp(aPlayer->mStartTime.Value() + timing.mDelay);
  animation->duration() = timing.mIterationDuration;
  animation->iterationCount() = timing.mIterationCount;
  animation->direction() = timing.mDirection;
  animation->property() = aProperty;
  animation->data() = aData;

  dom::Animation* anim = aPlayer->GetSource();
  for (size_t propIdx = 0;
       propIdx < anim->Properties().Length();
       propIdx++) {
    AnimationProperty& property = anim->Properties()[propIdx];

    if (aProperty != property.mProperty) {
      continue;
    }

    for (uint32_t segIdx = 0; segIdx < property.mSegments.Length(); segIdx++) {
      AnimationPropertySegment& segment = property.mSegments[segIdx];

      AnimationSegment* animSegment = animation->segments().AppendElement();
      if (aProperty == eCSSProperty_transform) {
        animSegment->startState() = InfallibleTArray<TransformFunction>();
        animSegment->endState() = InfallibleTArray<TransformFunction>();

        nsCSSValueSharedList* list =
          segment.mFromValue.GetCSSValueSharedListValue();
        AddTransformFunctions(list->mHead, styleContext, presContext, bounds,
                              animSegment->startState().get_ArrayOfTransformFunction());

        list = segment.mToValue.GetCSSValueSharedListValue();
        AddTransformFunctions(list->mHead, styleContext, presContext, bounds,
                              animSegment->endState().get_ArrayOfTransformFunction());
      } else if (aProperty == eCSSProperty_opacity) {
        animSegment->startState() = segment.mFromValue.GetFloatValue();
        animSegment->endState() = segment.mToValue.GetFloatValue();
      }

      animSegment->startPortion() = segment.mFromKey;
      animSegment->endPortion() = segment.mToKey;
      animSegment->sampleFn() = ToTimingFunction(segment.mTimingFunction);
    }
  }
}

static void
AddAnimationsForProperty(nsIFrame* aFrame, nsCSSProperty aProperty,
                         AnimationPlayerPtrArray& aPlayers,
                         Layer* aLayer, AnimationData& aData,
                         bool aPending) {
  for (size_t playerIdx = 0; playerIdx < aPlayers.Length(); playerIdx++) {
    AnimationPlayer* player = aPlayers[playerIdx];
    dom::Animation* anim = player->GetSource();
    if (!(anim && anim->HasAnimationOfProperty(aProperty) &&
          player->IsRunning())) {
      continue;
    }
    AddAnimationForProperty(aFrame, aProperty, player, aLayer, aData, aPending);
    player->mIsRunningOnCompositor = true;
  }
}

 void
nsDisplayListBuilder::AddAnimationsAndTransitionsToLayer(Layer* aLayer,
                                                         nsDisplayListBuilder* aBuilder,
                                                         nsDisplayItem* aItem,
                                                         nsIFrame* aFrame,
                                                         nsCSSProperty aProperty)
{
  
  
  
  
  MOZ_ASSERT(!aBuilder == !aItem,
             "should only be called in two configurations, with both "
             "aBuilder and aItem, or with neither");
  MOZ_ASSERT(!aItem || aFrame == aItem->Frame(), "frame mismatch");

  bool pending = !aBuilder;

  if (pending) {
    aLayer->ClearAnimationsForNextTransaction();
  } else {
    aLayer->ClearAnimations();
  }

  nsIContent* content = aFrame->GetContent();
  if (!content) {
    return;
  }
  AnimationPlayerCollection* transitions =
    nsTransitionManager::GetAnimationsForCompositor(content, aProperty);
  AnimationPlayerCollection* animations =
    nsAnimationManager::GetAnimationsForCompositor(content, aProperty);

  if (!animations && !transitions) {
    return;
  }

  
  
  
  if (aItem && !aItem->CanUseAsyncAnimations(aBuilder)) {
    
    
    
    aFrame->Properties().Set(nsIFrame::RefusedAsyncAnimation(),
                            reinterpret_cast<void*>(intptr_t(true)));

    
    
    aFrame->SchedulePaint();
    return;
  }

  AnimationData data;
  if (aProperty == eCSSProperty_transform) {
    nsRect bounds = nsDisplayTransform::GetFrameBoundsForTransform(aFrame);
    
    float scale = nsDeviceContext::AppUnitsPerCSSPixel();
    Point3D offsetToTransformOrigin =
      nsDisplayTransform::GetDeltaToTransformOrigin(aFrame, scale, &bounds);
    Point3D offsetToPerspectiveOrigin =
      nsDisplayTransform::GetDeltaToPerspectiveOrigin(aFrame, scale);
    nscoord perspective = 0.0;
    nsStyleContext* parentStyleContext = aFrame->StyleContext()->GetParent();
    if (parentStyleContext) {
      const nsStyleDisplay* disp = parentStyleContext->StyleDisplay();
      if (disp && disp->mChildPerspective.GetUnit() == eStyleUnit_Coord) {
        perspective = disp->mChildPerspective.GetCoordValue();
      }
    }
    nsPoint origin;
    if (aItem) {
      origin = aItem->ToReferenceFrame();
    } else {
      
      
      nsIFrame* referenceFrame =
        nsLayoutUtils::GetReferenceFrame(GetTransformRootFrame(aFrame));
      origin = aFrame->GetOffsetToCrossDoc(referenceFrame);
    }

    data = TransformData(origin, offsetToTransformOrigin,
                         offsetToPerspectiveOrigin, bounds, perspective,
                         aFrame->PresContext()->AppUnitsPerDevPixel());
  } else if (aProperty == eCSSProperty_opacity) {
    data = null_t();
  }

  if (transitions) {
    AddAnimationsForProperty(aFrame, aProperty, transitions->mPlayers,
                             aLayer, data, pending);
    aLayer->SetAnimationGeneration(transitions->mAnimationGeneration);
  }

  if (animations) {
    AddAnimationsForProperty(aFrame, aProperty, animations->mPlayers,
                             aLayer, data, pending);
    aLayer->SetAnimationGeneration(animations->mAnimationGeneration);
  }
}

nsDisplayListBuilder::nsDisplayListBuilder(nsIFrame* aReferenceFrame,
    Mode aMode, bool aBuildCaret)
    : mReferenceFrame(aReferenceFrame),
      mIgnoreScrollFrame(nullptr),
      mLayerEventRegions(nullptr),
      mCurrentTableItem(nullptr),
      mCurrentFrame(aReferenceFrame),
      mCurrentReferenceFrame(aReferenceFrame),
      mDirtyRect(-1,-1,-1,-1),
      mGlassDisplayItem(nullptr),
      mMode(aMode),
      mCurrentScrollParentId(FrameMetrics::NULL_SCROLL_ID),
      mCurrentScrollbarTarget(FrameMetrics::NULL_SCROLL_ID),
      mCurrentScrollbarFlags(0),
      mBuildCaret(aBuildCaret),
      mIgnoreSuppression(false),
      mHadToIgnoreSuppression(false),
      mIsAtRootOfPseudoStackingContext(false),
      mIncludeAllOutOfFlows(false),
      mDescendIntoSubdocuments(true),
      mSelectedFramesOnly(false),
      mAccurateVisibleRegions(false),
      mAllowMergingAndFlattening(true),
      mWillComputePluginGeometry(false),
      mInTransform(false),
      mSyncDecodeImages(false),
      mIsPaintingToWindow(false),
      mIsCompositingCheap(false),
      mContainsPluginItem(false),
      mAncestorHasTouchEventHandler(false),
      mHaveScrollableDisplayPort(false)
{
  MOZ_COUNT_CTOR(nsDisplayListBuilder);
  PL_InitArenaPool(&mPool, "displayListArena", 1024,
                   std::max(NS_ALIGNMENT_OF(void*),NS_ALIGNMENT_OF(double))-1);

  nsPresContext* pc = aReferenceFrame->PresContext();
  nsIPresShell *shell = pc->PresShell();
  if (pc->IsRenderingOnlySelection()) {
    nsCOMPtr<nsISelectionController> selcon(do_QueryInterface(shell));
    if (selcon) {
      selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(mBoundingSelection));
    }
  }

  nsCSSRendering::BeginFrameTreesLocked();
  PR_STATIC_ASSERT(nsDisplayItem::TYPE_MAX < (1 << nsDisplayItem::TYPE_BITS));
}

static void MarkFrameForDisplay(nsIFrame* aFrame, nsIFrame* aStopAtFrame) {
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderFor(f)) {
    if (f->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)
      return;
    f->AddStateBits(NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO);
    if (f == aStopAtFrame) {
      
      break;
    }
  }
}

void nsDisplayListBuilder::SetContainsBlendMode(uint8_t aBlendMode)
{
  MOZ_ASSERT(aBlendMode != NS_STYLE_BLEND_NORMAL);
  gfxContext::GraphicsOperator op = nsCSSRendering::GetGFXBlendMode(aBlendMode);
  mContainedBlendModes += gfx::CompositionOpForOp(op);
}

bool nsDisplayListBuilder::NeedToForceTransparentSurfaceForItem(nsDisplayItem* aItem)
{
  return aItem == mGlassDisplayItem || aItem->ClearsBackground();
}

void nsDisplayListBuilder::MarkOutOfFlowFrameForDisplay(nsIFrame* aDirtyFrame,
                                                        nsIFrame* aFrame,
                                                        const nsRect& aDirtyRect)
{
  nsRect dirtyRectRelativeToDirtyFrame = aDirtyRect;
  if (nsLayoutUtils::IsFixedPosFrameInDisplayPort(aFrame) &&
      IsPaintingToWindow()) {
    NS_ASSERTION(aDirtyFrame == aFrame->GetParent(), "Dirty frame should be viewport frame");
    
    
    
    nsIPresShell* ps = aFrame->PresContext()->PresShell();
    dirtyRectRelativeToDirtyFrame.MoveTo(0, 0);
    if (ps->IsScrollPositionClampingScrollPortSizeSet()) {
      dirtyRectRelativeToDirtyFrame.SizeTo(ps->GetScrollPositionClampingScrollPortSize());
    } else {
      dirtyRectRelativeToDirtyFrame.SizeTo(aDirtyFrame->GetSize());
    }
  }

  nsRect dirty = dirtyRectRelativeToDirtyFrame - aFrame->GetOffsetTo(aDirtyFrame);
  nsRect overflowRect = aFrame->GetVisualOverflowRect();

  if (aFrame->IsTransformed() &&
      nsLayoutUtils::HasAnimationsForCompositor(aFrame->GetContent(),
                                                eCSSProperty_transform)) {
   




    overflowRect.Inflate(nsPresContext::CSSPixelsToAppUnits(32));
  }

  if (!dirty.IntersectRect(dirty, overflowRect))
    return;
  const DisplayItemClip* clip = mClipState.GetClipForContainingBlockDescendants();
  OutOfFlowDisplayData* data = clip ? new OutOfFlowDisplayData(*clip, dirty)
    : new OutOfFlowDisplayData(dirty);
  aFrame->Properties().Set(nsDisplayListBuilder::OutOfFlowDisplayDataProperty(), data);

  MarkFrameForDisplay(aFrame, aDirtyFrame);
}

static void UnmarkFrameForDisplay(nsIFrame* aFrame) {
  nsPresContext* presContext = aFrame->PresContext();
  presContext->PropertyTable()->
    Delete(aFrame, nsDisplayListBuilder::OutOfFlowDisplayDataProperty());

  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderFor(f)) {
    if (!(f->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO))
      return;
    f->RemoveStateBits(NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO);
  }
}

 FrameMetrics
nsDisplayScrollLayer::ComputeFrameMetrics(nsIFrame* aForFrame,
                                          nsIFrame* aScrollFrame,
                                          const nsIFrame* aReferenceFrame,
                                          Layer* aLayer,
                                          ViewID aScrollParentId,
                                          const nsRect& aViewport,
                                          bool aForceNullScrollId,
                                          bool aIsRoot,
                                          const ContainerLayerParameters& aContainerParameters)
{
  nsPresContext* presContext = aForFrame->PresContext();
  int32_t auPerDevPixel = presContext->AppUnitsPerDevPixel();
  LayoutDeviceToLayerScale resolution(aContainerParameters.mXScale, aContainerParameters.mYScale);

  nsIPresShell* presShell = presContext->GetPresShell();
  FrameMetrics metrics;
  metrics.SetViewport(CSSRect::FromAppUnits(aViewport));

  ViewID scrollId = FrameMetrics::NULL_SCROLL_ID;
  nsIContent* content = aScrollFrame ? aScrollFrame->GetContent() : nullptr;
  if (content) {
    if (!aForceNullScrollId) {
      scrollId = nsLayoutUtils::FindOrCreateIDFor(content);
    }
    nsRect dp;
    if (nsLayoutUtils::GetDisplayPort(content, &dp)) {
      metrics.mDisplayPort = CSSRect::FromAppUnits(dp);
      nsLayoutUtils::LogTestDataForPaint(aLayer->Manager(), scrollId, "displayport",
          metrics.mDisplayPort);
    }
    if (nsLayoutUtils::GetCriticalDisplayPort(content, &dp)) {
      metrics.mCriticalDisplayPort = CSSRect::FromAppUnits(dp);
    }
    DisplayPortMarginsPropertyData* marginsData =
        static_cast<DisplayPortMarginsPropertyData*>(content->GetProperty(nsGkAtoms::DisplayPortMargins));
    if (marginsData) {
      metrics.SetDisplayPortMargins(marginsData->mMargins);
    }
  }

  nsIScrollableFrame* scrollableFrame = nullptr;
  if (aScrollFrame)
    scrollableFrame = aScrollFrame->GetScrollTargetFrame();

  metrics.mScrollableRect = CSSRect::FromAppUnits(
    nsLayoutUtils::CalculateScrollableRectForFrame(scrollableFrame, aForFrame));

  if (scrollableFrame) {
    nsPoint scrollPosition = scrollableFrame->GetScrollPosition();
    metrics.SetScrollOffset(CSSPoint::FromAppUnits(scrollPosition));

    nsPoint smoothScrollPosition = scrollableFrame->LastScrollDestination();
    metrics.SetSmoothScrollOffset(CSSPoint::FromAppUnits(smoothScrollPosition));

    
    
    
    nsIAtom* lastScrollOrigin = scrollableFrame->LastScrollOrigin();
    if (lastScrollOrigin && lastScrollOrigin != nsGkAtoms::apz) {
      metrics.SetScrollOffsetUpdated(scrollableFrame->CurrentScrollGeneration());
    }
    nsIAtom* lastSmoothScrollOrigin = scrollableFrame->LastSmoothScrollOrigin();
    if (lastSmoothScrollOrigin) {
      metrics.SetSmoothScrollOffsetUpdated(scrollableFrame->CurrentScrollGeneration());
    }
  }

  metrics.SetScrollId(scrollId);
  metrics.SetIsRoot(aIsRoot);
  metrics.SetScrollParentId(aScrollParentId);

  
  
  if (aScrollFrame == presShell->GetRootScrollFrame()) {
    metrics.mResolution = ParentLayerToLayerScale(presShell->GetXResolution(),
                                                  presShell->GetYResolution());
  } else {
    metrics.mResolution = ParentLayerToLayerScale(1.0f);
  }

  
  
  metrics.mCumulativeResolution = LayoutDeviceToLayerScale(1.0f);
  nsIPresShell* curPresShell = presShell;
  while (curPresShell != nullptr) {
    ParentLayerToLayerScale presShellResolution(curPresShell->GetXResolution(),
                                                curPresShell->GetYResolution());
    metrics.mCumulativeResolution.scale *= presShellResolution.scale;
    nsPresContext* parentContext = curPresShell->GetPresContext()->GetParentPresContext();
    curPresShell = parentContext ? parentContext->GetPresShell() : nullptr;
  }

  metrics.mDevPixelsPerCSSPixel = CSSToLayoutDeviceScale(
    (float)nsPresContext::AppUnitsPerCSSPixel() / auPerDevPixel);

  
  
  const LayerToScreenScale layerToScreenScale(1.0f);
  metrics.SetZoom(metrics.mCumulativeResolution * metrics.mDevPixelsPerCSSPixel
                  * layerToScreenScale);

  if (presShell) {
    nsIDocument* document = nullptr;
    document = presShell->GetDocument();
    if (document) {
      nsCOMPtr<nsPIDOMWindow> innerWin(document->GetInnerWindow());
      if (innerWin) {
        metrics.mMayHaveTouchListeners = innerWin->HasTouchEventListeners();
      }
    }
    metrics.mMayHaveTouchCaret = presShell->MayHaveTouchCaret();
  }

  LayoutDeviceToParentLayerScale layoutToParentLayerScale =
    
    
    metrics.mCumulativeResolution * LayerToScreenScale(1.0) * ScreenToParentLayerScale(1.0);

  
  
  
  
  nsIFrame* frameForCompositionBoundsCalculation = aScrollFrame ? aScrollFrame : aForFrame;
  nsRect compositionBounds(frameForCompositionBoundsCalculation->GetOffsetToCrossDoc(aReferenceFrame),
                           frameForCompositionBoundsCalculation->GetSize());
  ParentLayerRect frameBounds = LayoutDeviceRect::FromAppUnits(compositionBounds, auPerDevPixel)
                                 * layoutToParentLayerScale;
  metrics.mCompositionBounds = frameBounds;

  
  
  
  
  
  
  
  
  bool isRootScrollFrame = aScrollFrame == presShell->GetRootScrollFrame();
  bool isRootContentDocRootScrollFrame = isRootScrollFrame
                                      && presContext->IsRootContentDocument();
  if (isRootContentDocRootScrollFrame) {
    if (nsIFrame* rootFrame = presShell->GetRootFrame()) {
      
      
      
      
      
      
      
      
      
#ifdef MOZ_WIDGET_ANDROID
      nsIWidget* widget = rootFrame->GetNearestWidget();
#else
      nsView* view = rootFrame->GetView();
      nsIWidget* widget = view ? view->GetWidget() : nullptr;
#endif
      if (widget) {
        nsIntRect widgetBounds;
        widget->GetBounds(widgetBounds);
        metrics.mCompositionBounds = ParentLayerRect(ViewAs<ParentLayerPixel>(widgetBounds));
#ifdef MOZ_WIDGET_ANDROID
        if (frameBounds.height < metrics.mCompositionBounds.height) {
          metrics.mCompositionBounds.height = frameBounds.height;
        }
#endif
      } else {
        LayoutDeviceIntSize contentSize;
        if (nsLayoutUtils::GetContentViewerSize(presContext, contentSize)) {
          LayoutDeviceToParentLayerScale scale(1.0f);
          if (presContext->GetParentPresContext()) {
            gfxSize res = presContext->GetParentPresContext()->PresShell()->GetCumulativeResolution();
            scale = LayoutDeviceToParentLayerScale(res.width, res.height);
          }
          metrics.mCompositionBounds.SizeTo(contentSize * scale);
        }
      }
    }
  }

  
  if (scrollableFrame && !LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars)) {
    nsMargin sizes = scrollableFrame->GetActualScrollbarSizes();
    
    ParentLayerMargin boundMargins = CSSMargin::FromAppUnits(sizes) * CSSToParentLayerScale(1.0f);
    metrics.mCompositionBounds.Deflate(boundMargins);
  }

  metrics.SetRootCompositionSize(
    nsLayoutUtils::CalculateRootCompositionSize(aScrollFrame ? aScrollFrame : aForFrame,
                                                isRootContentDocRootScrollFrame, metrics));

  if (gfxPrefs::APZPrintTree()) {
    if (nsIContent* content = frameForCompositionBoundsCalculation->GetContent()) {
      nsAutoString contentDescription;
      content->Describe(contentDescription);
      metrics.SetContentDescription(NS_LossyConvertUTF16toASCII(contentDescription));
    }
  }

  metrics.SetPresShellId(presShell->GetPresShellId());

  
  
  
  if (aScrollFrame && nsContentUtils::HasScrollgrab(aScrollFrame->GetContent())) {
    metrics.SetHasScrollgrab(true);
  }

  
  
  if (aScrollFrame) {
    if (isRootScrollFrame) {
      metrics.SetBackgroundColor(presShell->GetCanvasBackground());
    } else {
      nsStyleContext* backgroundStyle;
      if (nsCSSRendering::FindBackground(aScrollFrame, &backgroundStyle)) {
        metrics.SetBackgroundColor(backgroundStyle->StyleBackground()->mBackgroundColor);
      }
    }
  }

  return metrics;
}

nsDisplayListBuilder::~nsDisplayListBuilder() {
  NS_ASSERTION(mFramesMarkedForDisplay.Length() == 0,
               "All frames should have been unmarked");
  NS_ASSERTION(mPresShellStates.Length() == 0,
               "All presshells should have been exited");
  NS_ASSERTION(!mCurrentTableItem, "No table item should be active");

  nsCSSRendering::EndFrameTreesLocked();

  for (uint32_t i = 0; i < mDisplayItemClipsToDestroy.Length(); ++i) {
    mDisplayItemClipsToDestroy[i]->DisplayItemClip::~DisplayItemClip();
  }

  PL_FinishArenaPool(&mPool);
  MOZ_COUNT_DTOR(nsDisplayListBuilder);
}

uint32_t
nsDisplayListBuilder::GetBackgroundPaintFlags() {
  uint32_t flags = 0;
  if (mSyncDecodeImages) {
    flags |= nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES;
  }
  if (mIsPaintingToWindow) {
    flags |= nsCSSRendering::PAINTBG_TO_WINDOW;
  }
  return flags;
}

void
nsDisplayListBuilder::SubtractFromVisibleRegion(nsRegion* aVisibleRegion,
                                                const nsRegion& aRegion)
{
  if (aRegion.IsEmpty())
    return;

  nsRegion tmp;
  tmp.Sub(*aVisibleRegion, aRegion);
  
  
  
  
  if (GetAccurateVisibleRegions() || tmp.GetNumRects() <= 15 ||
      tmp.Area() <= aVisibleRegion->Area()/2) {
    *aVisibleRegion = tmp;
  }
}

nsCaret *
nsDisplayListBuilder::GetCaret() {
  nsRefPtr<nsCaret> caret = CurrentPresShellState()->mPresShell->GetCaret();
  return caret;
}

void
nsDisplayListBuilder::EnterPresShell(nsIFrame* aReferenceFrame)
{
  PresShellState* state = mPresShellStates.AppendElement();
  state->mPresShell = aReferenceFrame->PresContext()->PresShell();
  state->mCaretFrame = nullptr;
  state->mFirstFrameMarkedForDisplay = mFramesMarkedForDisplay.Length();

  state->mPresShell->UpdateCanvasBackground();

  if (mIsPaintingToWindow) {
    mReferenceFrame->AddPaintedPresShell(state->mPresShell);

    state->mPresShell->IncrementPaintCount();
  }

  bool buildCaret = mBuildCaret;
  if (mIgnoreSuppression || !state->mPresShell->IsPaintingSuppressed()) {
    if (state->mPresShell->IsPaintingSuppressed()) {
      mHadToIgnoreSuppression = true;
    }
    state->mIsBackgroundOnly = false;
  } else {
    state->mIsBackgroundOnly = true;
    buildCaret = false;
  }

  if (!buildCaret)
    return;

  nsRefPtr<nsCaret> caret = state->mPresShell->GetCaret();
  state->mCaretFrame = caret->GetPaintGeometry(&state->mCaretRect);
  if (state->mCaretFrame) {
    mFramesMarkedForDisplay.AppendElement(state->mCaretFrame);
    MarkFrameForDisplay(state->mCaretFrame, nullptr);
  }
}

void
nsDisplayListBuilder::LeavePresShell(nsIFrame* aReferenceFrame)
{
  NS_ASSERTION(CurrentPresShellState()->mPresShell ==
      aReferenceFrame->PresContext()->PresShell(),
      "Presshell mismatch");
  ResetMarkedFramesForDisplayList();
  mPresShellStates.SetLength(mPresShellStates.Length() - 1);
}

void
nsDisplayListBuilder::ResetMarkedFramesForDisplayList()
{
  
  uint32_t firstFrameForShell = CurrentPresShellState()->mFirstFrameMarkedForDisplay;
  for (uint32_t i = firstFrameForShell;
       i < mFramesMarkedForDisplay.Length(); ++i) {
    UnmarkFrameForDisplay(mFramesMarkedForDisplay[i]);
  }
  mFramesMarkedForDisplay.SetLength(firstFrameForShell);
}

void
nsDisplayListBuilder::MarkFramesForDisplayList(nsIFrame* aDirtyFrame,
                                               const nsFrameList& aFrames,
                                               const nsRect& aDirtyRect) {
  mFramesMarkedForDisplay.SetCapacity(mFramesMarkedForDisplay.Length() + aFrames.GetLength());
  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    mFramesMarkedForDisplay.AppendElement(e.get());
    MarkOutOfFlowFrameForDisplay(aDirtyFrame, e.get(), aDirtyRect);
  }
}

void
nsDisplayListBuilder::MarkPreserve3DFramesForDisplayList(nsIFrame* aDirtyFrame, const nsRect& aDirtyRect)
{
  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  aDirtyFrame->GetChildLists(&childListArray);
  nsIFrame::ChildListArrayIterator lists(childListArray);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame *child = childFrames.get();
      if (child->Preserves3D()) {
        mFramesMarkedForDisplay.AppendElement(child);
        nsRect dirty = aDirtyRect - child->GetOffsetTo(aDirtyFrame);

        child->Properties().Set(nsDisplayListBuilder::Preserve3DDirtyRectProperty(),
                           new nsRect(dirty));

        MarkFrameForDisplay(child, aDirtyFrame);
      }
    }
  }
}

void*
nsDisplayListBuilder::Allocate(size_t aSize) {
  void *tmp;
  PL_ARENA_ALLOCATE(tmp, &mPool, aSize);
  if (!tmp) {
    NS_RUNTIMEABORT("out of memory");
  }
  return tmp;
}

const DisplayItemClip*
nsDisplayListBuilder::AllocateDisplayItemClip(const DisplayItemClip& aOriginal)
{
  void* p = Allocate(sizeof(DisplayItemClip));
  if (!aOriginal.GetRoundedRectCount()) {
    memcpy(p, &aOriginal, sizeof(DisplayItemClip));
    return static_cast<DisplayItemClip*>(p);
  }

  DisplayItemClip* c = new (p) DisplayItemClip(aOriginal);
  mDisplayItemClipsToDestroy.AppendElement(c);
  return c;
}

const nsIFrame*
nsDisplayListBuilder::FindReferenceFrameFor(const nsIFrame *aFrame,
                                            nsPoint* aOffset)
{
  if (aFrame == mCurrentFrame) {
    if (aOffset) {
      *aOffset = mCurrentOffsetToReferenceFrame;
    }
    return mCurrentReferenceFrame;
  }
  for (const nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f))
  {
    if (f == mReferenceFrame || f->IsTransformed()) {
      if (aOffset) {
        *aOffset = aFrame->GetOffsetToCrossDoc(f);
      }
      return f;
    }
  }
  if (aOffset) {
    *aOffset = aFrame->GetOffsetToCrossDoc(mReferenceFrame);
  }
  return mReferenceFrame;
}

void
nsDisplayListBuilder::AdjustWindowDraggingRegion(nsIFrame* aFrame)
{
  if (!IsForPainting() || IsInSubdocument() || IsInTransform()) {
    return;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsRect borderBox = aFrame->GetRectRelativeToSelf().Intersect(mDirtyRect);
  borderBox += ToReferenceFrame(aFrame);
  const DisplayItemClip* clip = ClipState().GetCurrentCombinedClip(this);
  if (clip) {
    borderBox = clip->ApplyNonRoundedIntersection(borderBox);
  }
  if (!borderBox.IsEmpty()) {
    const nsStyleUserInterface* styleUI = aFrame->StyleUserInterface();
    if (styleUI->mWindowDragging == NS_STYLE_WINDOW_DRAGGING_DRAG) {
      mWindowDraggingRegion.OrWith(borderBox);
    } else {
      mWindowDraggingRegion.SubOut(borderBox);
    }
  }
}

void nsDisplayListSet::MoveTo(const nsDisplayListSet& aDestination) const
{
  aDestination.BorderBackground()->AppendToTop(BorderBackground());
  aDestination.BlockBorderBackgrounds()->AppendToTop(BlockBorderBackgrounds());
  aDestination.Floats()->AppendToTop(Floats());
  aDestination.Content()->AppendToTop(Content());
  aDestination.PositionedDescendants()->AppendToTop(PositionedDescendants());
  aDestination.Outlines()->AppendToTop(Outlines());
}

static void
MoveListTo(nsDisplayList* aList, nsTArray<nsDisplayItem*>* aElements) {
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom()) != nullptr) {
    aElements->AppendElement(item);
  }
}

nsRect
nsDisplayList::GetBounds(nsDisplayListBuilder* aBuilder) const {
  nsRect bounds;
  for (nsDisplayItem* i = GetBottom(); i != nullptr; i = i->GetAbove()) {
    bounds.UnionRect(bounds, i->GetClippedBounds(aBuilder));
  }
  return bounds;
}

nsRect
nsDisplayList::GetVisibleRect() const {
  nsRect result;
  for (nsDisplayItem* i = GetBottom(); i != nullptr; i = i->GetAbove()) {
    result.UnionRect(result, i->GetVisibleRect());
  }
  return result;
}

bool
nsDisplayList::ComputeVisibilityForRoot(nsDisplayListBuilder* aBuilder,
                                        nsRegion* aVisibleRegion,
                                        nsIFrame* aDisplayPortFrame) {
  PROFILER_LABEL("nsDisplayList", "ComputeVisibilityForRoot",
    js::ProfileEntry::Category::GRAPHICS);

  nsRegion r;
  r.And(*aVisibleRegion, GetBounds(aBuilder));
  return ComputeVisibilityForSublist(aBuilder, aVisibleRegion,
                                     r.GetBounds(), aDisplayPortFrame);
}

static nsRegion
TreatAsOpaque(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder)
{
  bool snap;
  nsRegion opaque = aItem->GetOpaqueRegion(aBuilder, &snap);
  if (aBuilder->IsForPluginGeometry()) {
    
    
    
    
    
    
    
    
    
    nsIFrame* f = aItem->Frame();
    if (f->PresContext()->IsChrome() && !aItem->GetChildren() &&
        f->StyleDisplay()->mOpacity != 0.0) {
      opaque = aItem->GetBounds(aBuilder, &snap);
    }
  }
  if (opaque.IsEmpty()) {
    return opaque;
  }
  nsRegion opaqueClipped;
  nsRegionRectIterator iter(opaque);
  for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
    opaqueClipped.Or(opaqueClipped, aItem->GetClip().ApproximateIntersectInward(*r));
  }
  return opaqueClipped;
}

bool
nsDisplayList::ComputeVisibilityForSublist(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion,
                                           const nsRect& aListVisibleBounds,
                                           nsIFrame* aDisplayPortFrame) {
#ifdef DEBUG
  nsRegion r;
  r.And(*aVisibleRegion, GetBounds(aBuilder));
  NS_ASSERTION(r.GetBounds().IsEqualInterior(aListVisibleBounds),
               "bad aListVisibleBounds");
#endif

  bool anyVisible = false;

  nsAutoTArray<nsDisplayItem*, 512> elements;
  MoveListTo(this, &elements);

  for (int32_t i = elements.Length() - 1; i >= 0; --i) {
    nsDisplayItem* item = elements[i];
    nsRect bounds = item->GetClippedBounds(aBuilder);

    nsRegion itemVisible;
    itemVisible.And(*aVisibleRegion, bounds);
    item->mVisibleRect = itemVisible.GetBounds();

    if (item->ComputeVisibility(aBuilder, aVisibleRegion)) {
      anyVisible = true;

      nsRegion opaque = TreatAsOpaque(item, aBuilder);
      
      aBuilder->SubtractFromVisibleRegion(aVisibleRegion, opaque);
    }
    AppendToBottom(item);
  }

  mIsOpaque = !aVisibleRegion->Intersects(aListVisibleBounds);
  return anyVisible;
}

void nsDisplayList::PaintRoot(nsDisplayListBuilder* aBuilder,
                              nsRenderingContext* aCtx,
                              uint32_t aFlags) {
  PROFILER_LABEL("nsDisplayList", "PaintRoot",
    js::ProfileEntry::Category::GRAPHICS);
  PaintForFrame(aBuilder, aCtx, aBuilder->RootReferenceFrame(), aFlags);
}






void nsDisplayList::PaintForFrame(nsDisplayListBuilder* aBuilder,
                                  nsRenderingContext* aCtx,
                                  nsIFrame* aForFrame,
                                  uint32_t aFlags) {
  nsRefPtr<LayerManager> layerManager;
  bool widgetTransaction = false;
  bool allowRetaining = false;
  bool doBeginTransaction = true;
  nsView *view = nullptr;
  if (aFlags & PAINT_USE_WIDGET_LAYERS) {
    nsIFrame* rootReferenceFrame = aBuilder->RootReferenceFrame();
    view = rootReferenceFrame->GetView();
    NS_ASSERTION(rootReferenceFrame == nsLayoutUtils::GetDisplayRootFrame(rootReferenceFrame),
                 "Reference frame must be a display root for us to use the layer manager");
    nsIWidget* window = rootReferenceFrame->GetNearestWidget();
    if (window) {
      layerManager = window->GetLayerManager(&allowRetaining);
      if (layerManager) {
        doBeginTransaction = !(aFlags & PAINT_EXISTING_TRANSACTION);
        widgetTransaction = true;
      }
    }
  }
  if (!layerManager) {
    if (!aCtx) {
      NS_WARNING("Nowhere to paint into");
      return;
    }
    layerManager = new BasicLayerManager(BasicLayerManager::BLM_OFFSCREEN);
  }

  
  FrameLayerBuilder *oldBuilder = layerManager->GetLayerBuilder();

  FrameLayerBuilder *layerBuilder = new FrameLayerBuilder();
  layerBuilder->Init(aBuilder, layerManager);

  if (aFlags & PAINT_COMPRESSED) {
    layerBuilder->SetLayerTreeCompressionMode();
  }

  if (aFlags & PAINT_FLUSH_LAYERS) {
    FrameLayerBuilder::InvalidateAllLayers(layerManager);
  }

  if (doBeginTransaction) {
    if (aCtx) {
      layerManager->BeginTransactionWithTarget(aCtx->ThebesContext());
    } else {
      layerManager->BeginTransaction();
    }
  }
  if (widgetTransaction) {
    layerBuilder->DidBeginRetainedLayerTransaction(layerManager);
  }

  nsPresContext* presContext = aForFrame->PresContext();
  nsIPresShell* presShell = presContext->GetPresShell();

  NotifySubDocInvalidationFunc computeInvalidFunc =
    presContext->MayHavePaintEventListenerInSubDocument() ? nsPresContext::NotifySubDocInvalidation : 0;
  bool computeInvalidRect = (computeInvalidFunc ||
                             (!layerManager->IsCompositingCheap() && layerManager->NeedsWidgetInvalidation())) &&
                            widgetTransaction;

  UniquePtr<LayerProperties> props;
  if (computeInvalidRect) {
    props = Move(LayerProperties::CloneFrom(layerManager->GetRoot()));
  }

  ContainerLayerParameters containerParameters
    (presShell->GetXResolution(), presShell->GetYResolution());
  nsRefPtr<ContainerLayer> root = layerBuilder->
    BuildContainerLayerFor(aBuilder, layerManager, aForFrame, nullptr, this,
                           containerParameters, nullptr);

  nsIDocument* document = nullptr;
  if (presShell) {
    document = presShell->GetDocument();
  }

  if (!root) {
    layerManager->SetUserData(&gLayerManagerLayerBuilder, oldBuilder);
    return;
  }
  
  root->SetPostScale(1.0f/containerParameters.mXScale,
                     1.0f/containerParameters.mYScale);

  bool isRoot = presContext->IsRootContentDocument();

  nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();

  nsRect viewport(aBuilder->ToReferenceFrame(aForFrame), aForFrame->GetSize());

  root->SetFrameMetrics(
    nsDisplayScrollLayer::ComputeFrameMetrics(aForFrame, rootScrollFrame,
                       aBuilder->FindReferenceFrameFor(aForFrame),
                       root, FrameMetrics::NULL_SCROLL_ID, viewport,
                       !isRoot, isRoot, containerParameters));

  
  
#ifdef DEBUG
  bool usingDisplayport = false;
  if (rootScrollFrame) {
    nsIContent* content = rootScrollFrame->GetContent();
    if (content) {
      usingDisplayport = nsLayoutUtils::GetDisplayPort(content, nullptr);
    }
  }
  if (usingDisplayport &&
      !(root->GetContentFlags() & Layer::CONTENT_OPAQUE) &&
      SpammyLayoutWarningsEnabled()) {
    
    NS_WARNING("Transparent content with displayports can be expensive.");
  }
#endif

  layerManager->SetRoot(root);
  layerBuilder->WillEndTransaction();

  if (widgetTransaction ||
      
      
      
      (document && document->IsBeingUsedAsImage())) {
    aForFrame->ClearInvalidationStateBits();
  }

  bool temp = aBuilder->SetIsCompositingCheap(layerManager->IsCompositingCheap());
  LayerManager::EndTransactionFlags flags = LayerManager::END_DEFAULT;
  if (layerManager->NeedsWidgetInvalidation()) {
    if (aFlags & PAINT_NO_COMPOSITE) {
      flags = LayerManager::END_NO_COMPOSITE;
    }
  } else {
    
    
    if (aBuilder->WillComputePluginGeometry()) {
      flags = LayerManager::END_NO_REMOTE_COMPOSITE;
    }
  }

  MaybeSetupTransactionIdAllocator(layerManager, view);

  layerManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer,
                               aBuilder, flags);
  aBuilder->SetIsCompositingCheap(temp);
  layerBuilder->DidEndTransaction();

  nsIntRegion invalid;
  if (props) {
    invalid = props->ComputeDifferences(root, computeInvalidFunc);
  } else if (widgetTransaction) {
    LayerProperties::ClearInvalidations(root);
  }

  bool shouldInvalidate = layerManager->NeedsWidgetInvalidation();
  if (view) {
    if (props) {
      if (!invalid.IsEmpty()) {
        nsIntRect bounds = invalid.GetBounds();
        nsRect rect(presContext->DevPixelsToAppUnits(bounds.x),
                    presContext->DevPixelsToAppUnits(bounds.y),
                    presContext->DevPixelsToAppUnits(bounds.width),
                    presContext->DevPixelsToAppUnits(bounds.height));
        if (shouldInvalidate) {
          view->GetViewManager()->InvalidateViewNoSuppression(view, rect);
        }
        presContext->NotifyInvalidation(bounds, 0);
      }
    } else if (shouldInvalidate) {
      view->GetViewManager()->InvalidateView(view);
    }
  }

  if (aFlags & PAINT_FLUSH_LAYERS) {
    FrameLayerBuilder::InvalidateAllLayers(layerManager);
  }

  layerManager->SetUserData(&gLayerManagerLayerBuilder, oldBuilder);
}

uint32_t nsDisplayList::Count() const {
  uint32_t count = 0;
  for (nsDisplayItem* i = GetBottom(); i; i = i->GetAbove()) {
    ++count;
  }
  return count;
}

nsDisplayItem* nsDisplayList::RemoveBottom() {
  nsDisplayItem* item = mSentinel.mAbove;
  if (!item)
    return nullptr;
  mSentinel.mAbove = item->mAbove;
  if (item == mTop) {
    
    mTop = &mSentinel;
  }
  item->mAbove = nullptr;
  return item;
}

void nsDisplayList::DeleteAll() {
  nsDisplayItem* item;
  while ((item = RemoveBottom()) != nullptr) {
    item->~nsDisplayItem();
  }
}

static bool
GetMouseThrough(const nsIFrame* aFrame)
{
  if (!aFrame->IsBoxFrame())
    return false;

  const nsIFrame* frame = aFrame;
  while (frame) {
    if (frame->GetStateBits() & NS_FRAME_MOUSE_THROUGH_ALWAYS) {
      return true;
    } else if (frame->GetStateBits() & NS_FRAME_MOUSE_THROUGH_NEVER) {
      return false;
    }
    frame = nsBox::GetParentBox(frame);
  }
  return false;
}

static bool
IsFrameReceivingPointerEvents(nsIFrame* aFrame)
{
  nsSubDocumentFrame* frame = do_QueryFrame(aFrame);
  if (frame && frame->PassPointerEventsToChildren()) {
    return true;
  }
  return NS_STYLE_POINTER_EVENTS_NONE !=
    aFrame->StyleVisibility()->GetEffectivePointerEvents(aFrame);
}



struct FramesWithDepth
{
  explicit FramesWithDepth(float aDepth) :
    mDepth(aDepth)
  {}

  bool operator<(const FramesWithDepth& aOther) const {
    if (mDepth != aOther.mDepth) {
      
      return mDepth > aOther.mDepth;
    }
    return this < &aOther;
  }
  bool operator==(const FramesWithDepth& aOther) const {
    return this == &aOther;
  }

  float mDepth;
  nsTArray<nsIFrame*> mFrames;
};


void FlushFramesArray(nsTArray<FramesWithDepth>& aSource, nsTArray<nsIFrame*>* aDest)
{
  if (aSource.IsEmpty()) {
    return;
  }
  aSource.Sort();
  uint32_t length = aSource.Length();
  for (uint32_t i = 0; i < length; i++) {
    aDest->MoveElementsFrom(aSource[i].mFrames);
  }
  aSource.Clear();
}

void nsDisplayList::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                            nsDisplayItem::HitTestState* aState,
                            nsTArray<nsIFrame*> *aOutFrames) const {
  int32_t itemBufferStart = aState->mItemBuffer.Length();
  nsDisplayItem* item;
  for (item = GetBottom(); item; item = item->GetAbove()) {
    aState->mItemBuffer.AppendElement(item);
  }
  nsAutoTArray<FramesWithDepth, 16> temp;
  for (int32_t i = aState->mItemBuffer.Length() - 1; i >= itemBufferStart; --i) {
    
    
    item = aState->mItemBuffer[i];
    aState->mItemBuffer.SetLength(i);

    bool snap;
    nsRect r = item->GetBounds(aBuilder, &snap).Intersect(aRect);
    if (item->GetClip().MayIntersect(r)) {
      nsAutoTArray<nsIFrame*, 16> outFrames;
      item->HitTest(aBuilder, aRect, aState, &outFrames);

      
      
      nsTArray<nsIFrame*> *writeFrames = aOutFrames;
      if (item->GetType() == nsDisplayItem::TYPE_TRANSFORM &&
          item->Frame()->Preserves3D()) {
        if (outFrames.Length()) {
          nsDisplayTransform *transform = static_cast<nsDisplayTransform*>(item);
          nsPoint point = aRect.TopLeft();
          
          if (aRect.width != 1 || aRect.height != 1) {
            point = aRect.Center();
          }
          temp.AppendElement(FramesWithDepth(transform->GetHitDepthAtPoint(aBuilder, point)));
          writeFrames = &temp[temp.Length() - 1].mFrames;
        }
      } else {
        
        
        FlushFramesArray(temp, aOutFrames);
      }

      for (uint32_t j = 0; j < outFrames.Length(); j++) {
        nsIFrame *f = outFrames.ElementAt(j);
        
        if (!GetMouseThrough(f) && IsFrameReceivingPointerEvents(f)) {
          writeFrames->AppendElement(f);
        }
      }
    }
  }
  
  FlushFramesArray(temp, aOutFrames);
  NS_ASSERTION(aState->mItemBuffer.Length() == uint32_t(itemBufferStart),
               "How did we forget to pop some elements?");
}

static void Sort(nsDisplayList* aList, int32_t aCount, nsDisplayList::SortLEQ aCmp,
                 void* aClosure) {
  if (aCount < 2)
    return;

  nsDisplayList list1;
  nsDisplayList list2;
  int i;
  int32_t half = aCount/2;
  bool sorted = true;
  nsDisplayItem* prev = nullptr;
  for (i = 0; i < aCount; ++i) {
    nsDisplayItem* item = aList->RemoveBottom();
    (i < half ? &list1 : &list2)->AppendToTop(item);
    if (sorted && prev && !aCmp(prev, item, aClosure)) {
      sorted = false;
    }
    prev = item;
  }
  if (sorted) {
    aList->AppendToTop(&list1);
    aList->AppendToTop(&list2);
    return;
  }

  Sort(&list1, half, aCmp, aClosure);
  Sort(&list2, aCount - half, aCmp, aClosure);

  for (i = 0; i < aCount; ++i) {
    if (list1.GetBottom() &&
        (!list2.GetBottom() ||
         aCmp(list1.GetBottom(), list2.GetBottom(), aClosure))) {
      aList->AppendToTop(list1.RemoveBottom());
    } else {
      aList->AppendToTop(list2.RemoveBottom());
    }
  }
}

static nsIContent* FindContentInDocument(nsDisplayItem* aItem, nsIDocument* aDoc) {
  nsIFrame* f = aItem->Frame();
  while (f) {
    nsPresContext* pc = f->PresContext();
    if (pc->Document() == aDoc) {
      return f->GetContent();
    }
    f = nsLayoutUtils::GetCrossDocParentFrame(pc->PresShell()->GetRootFrame());
  }
  return nullptr;
}

static bool IsContentLEQ(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                         void* aClosure) {
  nsIContent* commonAncestor = static_cast<nsIContent*>(aClosure);
  
  
  
  
  nsIDocument* commonAncestorDoc = commonAncestor->OwnerDoc();
  nsIContent* content1 = FindContentInDocument(aItem1, commonAncestorDoc);
  nsIContent* content2 = FindContentInDocument(aItem2, commonAncestorDoc);
  if (!content1 || !content2) {
    NS_ERROR("Document trees are mixed up!");
    
    return true;
  }
  return nsLayoutUtils::CompareTreePosition(content1, content2, commonAncestor) <= 0;
}

static bool IsZOrderLEQ(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                        void* aClosure) {
  
  
  return aItem1->ZIndex() <= aItem2->ZIndex();
}

void nsDisplayList::SortByZOrder(nsDisplayListBuilder* aBuilder,
                                 nsIContent* aCommonAncestor) {
  Sort(aBuilder, IsZOrderLEQ, aCommonAncestor);
}

void nsDisplayList::SortByContentOrder(nsDisplayListBuilder* aBuilder,
                                       nsIContent* aCommonAncestor) {
  Sort(aBuilder, IsContentLEQ, aCommonAncestor);
}

void nsDisplayList::Sort(nsDisplayListBuilder* aBuilder,
                         SortLEQ aCmp, void* aClosure) {
  ::Sort(this, Count(), aCmp, aClosure);
}

nsDisplayItem::nsDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
  : mFrame(aFrame)
  , mClip(aBuilder->ClipState().GetCurrentCombinedClip(aBuilder))
#ifdef MOZ_DUMP_PAINTING
  , mPainted(false)
#endif
{
  mReferenceFrame = aBuilder->FindReferenceFrameFor(aFrame, &mToReferenceFrame);
  NS_ASSERTION(aBuilder->GetDirtyRect().width >= 0 ||
               !aBuilder->IsForPainting(), "dirty rect not set");
  
  
  mVisibleRect = aBuilder->GetDirtyRect() +
      aBuilder->GetCurrentFrameOffsetToReferenceFrame();
}

void
nsDisplayItem::AddInvalidRegionForSyncDecodeBackgroundImages(
  nsDisplayListBuilder* aBuilder,
  const nsDisplayItemGeometry* aGeometry,
  nsRegion* aInvalidRegion)
{
  if (aBuilder->ShouldSyncDecodeImages()) {
    if (!nsCSSRendering::AreAllBackgroundImagesDecodedForFrame(mFrame)) {
      bool snap;
      aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
    }
  }
}

 bool
nsDisplayItem::ForceActiveLayers()
{
  static bool sForce = false;
  static bool sForceCached = false;

  if (!sForceCached) {
    Preferences::AddBoolVarCache(&sForce, "layers.force-active", false);
    sForceCached = true;
  }

  return sForce;
}

 int32_t
nsDisplayItem::MaxActiveLayers()
{
  static int32_t sMaxLayers = false;
  static bool sMaxLayersCached = false;

  if (!sMaxLayersCached) {
    Preferences::AddIntVarCache(&sMaxLayers, "layers.max-active", -1);
    sMaxLayersCached = true;
  }

  return sMaxLayers;
}

int32_t
nsDisplayItem::ZIndex() const
{
  if (!mFrame->IsPositioned() && !mFrame->IsFlexOrGridItem())
    return 0;

  const nsStylePosition* position = mFrame->StylePosition();
  if (position->mZIndex.GetUnit() == eStyleUnit_Integer)
    return position->mZIndex.GetIntValue();

  
  return 0;
}

bool
nsDisplayItem::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion)
{
  return !mVisibleRect.IsEmpty() &&
    !IsInvisibleInRect(aVisibleRegion->GetBounds());
}

bool
nsDisplayItem::RecomputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion) {
  nsRect bounds = GetClippedBounds(aBuilder);

  nsRegion itemVisible;
  itemVisible.And(*aVisibleRegion, bounds);
  mVisibleRect = itemVisible.GetBounds();

  
  
  
  if (!ComputeVisibility(aBuilder, aVisibleRegion)) {
    mVisibleRect = nsRect();
    return false;
  }

  nsRegion opaque = TreatAsOpaque(this, aBuilder);
  aBuilder->SubtractFromVisibleRegion(aVisibleRegion, opaque);
  return true;
}

nsRect
nsDisplayItem::GetClippedBounds(nsDisplayListBuilder* aBuilder)
{
  bool snap;
  nsRect r = GetBounds(aBuilder, &snap);
  return GetClip().ApplyNonRoundedIntersection(r);
}

nsRect
nsDisplaySolidColor::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = true;
  return mBounds;
}

void
nsDisplaySolidColor::Paint(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext* aCtx)
{
  aCtx->SetColor(mColor);
  aCtx->FillRect(mVisibleRect);
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplaySolidColor::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (rgba %d,%d,%d,%d)",
                 NS_GET_R(mColor), NS_GET_G(mColor),
                 NS_GET_B(mColor), NS_GET_A(mColor));
}
#endif

static void
RegisterThemeGeometry(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
{
  if (!aBuilder->IsInSubdocument() && !aBuilder->IsInTransform()) {
    nsIFrame* displayRoot = nsLayoutUtils::GetDisplayRootFrame(aFrame);
    nsRect borderBox(aFrame->GetOffsetTo(displayRoot), aFrame->GetSize());
    aBuilder->RegisterThemeGeometry(aFrame->StyleDisplay()->mAppearance,
        borderBox.ToNearestPixels(aFrame->PresContext()->AppUnitsPerDevPixel()));
  }
}

nsDisplayBackgroundImage::nsDisplayBackgroundImage(nsDisplayListBuilder* aBuilder,
                                                   nsIFrame* aFrame,
                                                   uint32_t aLayer,
                                                   const nsStyleBackground* aBackgroundStyle)
  : nsDisplayImageContainer(aBuilder, aFrame)
  , mBackgroundStyle(aBackgroundStyle)
  , mLayer(aLayer)
{
  MOZ_COUNT_CTOR(nsDisplayBackgroundImage);

  mBounds = GetBoundsInternal(aBuilder);
}

nsDisplayBackgroundImage::~nsDisplayBackgroundImage()
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_DTOR(nsDisplayBackgroundImage);
#endif
}

static nsStyleContext* GetBackgroundStyleContext(nsIFrame* aFrame)
{
  nsStyleContext *sc;
  if (!nsCSSRendering::FindBackground(aFrame, &sc)) {
    
    
    
    
    
    if (!aFrame->StyleDisplay()->mAppearance) {
      return nullptr;
    }

    nsIContent* content = aFrame->GetContent();
    if (!content || content->GetParent()) {
      return nullptr;
    }

    sc = aFrame->StyleContext();
  }
  return sc;
}

 void
SetBackgroundClipRegion(DisplayListClipState::AutoSaveRestore& aClipState,
                        nsIFrame* aFrame, const nsPoint& aToReferenceFrame,
                        const nsStyleBackground::Layer& aLayer,
                        bool aWillPaintBorder)
{
  nsRect borderBox = nsRect(aToReferenceFrame, aFrame->GetSize());

  nsCSSRendering::BackgroundClipState clip;
  nsCSSRendering::GetBackgroundClip(aLayer, aFrame, *aFrame->StyleBorder(),
                                    borderBox, borderBox, aWillPaintBorder,
                                    aFrame->PresContext()->AppUnitsPerDevPixel(),
                                    &clip);

  if (clip.mHasAdditionalBGClipArea) {
    aClipState.ClipContentDescendants(clip.mAdditionalBGClipArea, clip.mBGClipArea,
                                      clip.mHasRoundedCorners ? clip.mRadii : nullptr);
  } else {
    aClipState.ClipContentDescendants(clip.mBGClipArea, clip.mHasRoundedCorners ? clip.mRadii : nullptr);
  }
}


 bool
nsDisplayBackgroundImage::AppendBackgroundItemsToTop(nsDisplayListBuilder* aBuilder,
                                                     nsIFrame* aFrame,
                                                     nsDisplayList* aList)
{
  nsStyleContext* bgSC = nullptr;
  const nsStyleBackground* bg = nullptr;
  nsPresContext* presContext = aFrame->PresContext();
  bool isThemed = aFrame->IsThemed();
  if (!isThemed) {
    bgSC = GetBackgroundStyleContext(aFrame);
    if (bgSC) {
      bg = bgSC->StyleBackground();
    }
  }

  bool drawBackgroundColor = false;
  nscolor color;
  if (!nsCSSRendering::IsCanvasFrame(aFrame) && bg) {
    bool drawBackgroundImage;
    color =
      nsCSSRendering::DetermineBackgroundColor(presContext, bgSC, aFrame,
                                               drawBackgroundImage, drawBackgroundColor);
  }

  bool hasInsetShadow = aFrame->StyleBorder()->mBoxShadow &&
                        aFrame->StyleBorder()->mBoxShadow->HasShadowWithInset(true);
  bool willPaintBorder = !isThemed && !hasInsetShadow &&
                         aFrame->StyleBorder()->HasBorder();

  nsPoint toRef = aBuilder->ToReferenceFrame(aFrame);

  
  
  
  nsDisplayList bgItemList;
  
  
  if ((drawBackgroundColor && color != NS_RGBA(0,0,0,0)) ||
      aBuilder->IsForEventDelivery()) {

    DisplayListClipState::AutoSaveRestore clipState(aBuilder);
    if (bg && !aBuilder->IsForEventDelivery()) {
      SetBackgroundClipRegion(clipState, aFrame, toRef,
                              bg->BottomLayer(),
                              willPaintBorder);
    }
    bgItemList.AppendNewToTop(
        new (aBuilder) nsDisplayBackgroundColor(aBuilder, aFrame, bg,
                                                drawBackgroundColor ? color : NS_RGBA(0, 0, 0, 0)));
  }

  if (isThemed) {
    nsITheme* theme = presContext->GetTheme();
    if (theme->NeedToClearBackgroundBehindWidget(aFrame->StyleDisplay()->mAppearance)) {
      bgItemList.AppendNewToTop(
        new (aBuilder) nsDisplayClearBackground(aBuilder, aFrame));
    }
    nsDisplayThemedBackground* bgItem =
      new (aBuilder) nsDisplayThemedBackground(aBuilder, aFrame);
    bgItemList.AppendNewToTop(bgItem);
    aList->AppendToTop(&bgItemList);
    return true;
  }

  if (!bg) {
    aList->AppendToTop(&bgItemList);
    return false;
  }
 
  bool needBlendContainer = false;

  
  
  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
    if (bg->mLayers[i].mImage.IsEmpty()) {
      continue;
    }

    if (bg->mLayers[i].mBlendMode != NS_STYLE_BLEND_NORMAL) {
      needBlendContainer = true;
    }

    DisplayListClipState::AutoSaveRestore clipState(aBuilder);
    if (!aBuilder->IsForEventDelivery()) {
      const nsStyleBackground::Layer& layer = bg->mLayers[i];
      SetBackgroundClipRegion(clipState, aFrame, toRef,
                              layer, willPaintBorder);
    }

    nsDisplayBackgroundImage* bgItem =
      new (aBuilder) nsDisplayBackgroundImage(aBuilder, aFrame, i, bg);
    bgItemList.AppendNewToTop(bgItem);
  }

  if (needBlendContainer) {
    bgItemList.AppendNewToTop(
      new (aBuilder) nsDisplayBlendContainer(aBuilder, aFrame, &bgItemList));
  }

  aList->AppendToTop(&bgItemList);
  return false;
}




static bool
RoundedBorderIntersectsRect(nsIFrame* aFrame,
                            const nsPoint& aFrameToReferenceFrame,
                            const nsRect& aTestRect)
{
  if (!nsRect(aFrameToReferenceFrame, aFrame->GetSize()).Intersects(aTestRect))
    return false;

  nscoord radii[8];
  return !aFrame->GetBorderRadii(radii) ||
         nsLayoutUtils::RoundedRectIntersectsRect(nsRect(aFrameToReferenceFrame,
                                                  aFrame->GetSize()),
                                                  radii, aTestRect);
}







static bool RoundedRectContainsRect(const nsRect& aRoundedRect,
                                    const nscoord aRadii[8],
                                    const nsRect& aContainedRect) {
  nsRegion rgn = nsLayoutUtils::RoundedRectIntersectRect(aRoundedRect, aRadii, aContainedRect);
  return rgn.Contains(aContainedRect);
}

bool
nsDisplayBackgroundImage::IsSingleFixedPositionImage(nsDisplayListBuilder* aBuilder,
                                                     const nsRect& aClipRect,
                                                     gfxRect* aDestRect)
{
  if (!mBackgroundStyle)
    return false;

  if (mBackgroundStyle->mLayers.Length() != 1)
    return false;

  nsPresContext* presContext = mFrame->PresContext();
  uint32_t flags = aBuilder->GetBackgroundPaintFlags();
  nsRect borderArea = nsRect(ToReferenceFrame(), mFrame->GetSize());
  const nsStyleBackground::Layer &layer = mBackgroundStyle->mLayers[mLayer];

  if (layer.mAttachment != NS_STYLE_BG_ATTACHMENT_FIXED)
    return false;

  nsBackgroundLayerState state =
    nsCSSRendering::PrepareBackgroundLayer(presContext, mFrame, flags,
                                           borderArea, aClipRect, layer);
  nsImageRenderer* imageRenderer = &state.mImageRenderer;
  
  if (!imageRenderer->IsRasterImage())
    return false;

  int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  *aDestRect = nsLayoutUtils::RectToGfxRect(state.mFillArea, appUnitsPerDevPixel);

  return true;
}

bool
nsDisplayBackgroundImage::ShouldFixToViewport(LayerManager* aManager)
{
  
  
  if (nsLayoutUtils::UsesAsyncScrolling() ||
      (aManager && aManager->ShouldAvoidComponentAlphaLayers())) {
    return false;
  }

  
  
  return mBackgroundStyle->mLayers[mLayer].mAttachment == NS_STYLE_BG_ATTACHMENT_FIXED &&
         !mBackgroundStyle->mLayers[mLayer].mImage.IsEmpty();
}

bool
nsDisplayBackgroundImage::TryOptimizeToImageLayer(LayerManager* aManager,
                                                  nsDisplayListBuilder* aBuilder)
{
  if (!mBackgroundStyle)
    return false;

  nsPresContext* presContext = mFrame->PresContext();
  uint32_t flags = aBuilder->GetBackgroundPaintFlags();
  nsRect borderArea = nsRect(ToReferenceFrame(), mFrame->GetSize());
  const nsStyleBackground::Layer &layer = mBackgroundStyle->mLayers[mLayer];

  if (layer.mClip != NS_STYLE_BG_CLIP_BORDER) {
    return false;
  }
  nscoord radii[8];
  if (mFrame->GetBorderRadii(radii)) {
    return false;
  }

  nsBackgroundLayerState state =
    nsCSSRendering::PrepareBackgroundLayer(presContext, mFrame, flags,
                                           borderArea, borderArea, layer);
  nsImageRenderer* imageRenderer = &state.mImageRenderer;
  
  if (!imageRenderer->IsRasterImage())
    return false;

  nsRefPtr<ImageContainer> imageContainer = imageRenderer->GetContainer(aManager);
  
  if (!imageContainer)
    return false;

  
  if (!state.mDestArea.IsEqualEdges(state.mFillArea)) {
    return false;
  }

  
  

  int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  mDestRect = nsLayoutUtils::RectToGfxRect(state.mDestArea, appUnitsPerDevPixel);
  mImageContainer = imageContainer;

  
  return true;
}

already_AddRefed<ImageContainer>
nsDisplayBackgroundImage::GetContainer(LayerManager* aManager,
                                       nsDisplayListBuilder *aBuilder)
{
  if (!TryOptimizeToImageLayer(aManager, aBuilder)) {
    return nullptr;
  }

  nsRefPtr<ImageContainer> container = mImageContainer;

  return container.forget();
}

LayerState
nsDisplayBackgroundImage::GetLayerState(nsDisplayListBuilder* aBuilder,
                                        LayerManager* aManager,
                                        const ContainerLayerParameters& aParameters)
{
  bool animated = false;
  if (mBackgroundStyle) {
    const nsStyleBackground::Layer &layer = mBackgroundStyle->mLayers[mLayer];
    const nsStyleImage* image = &layer.mImage;
    if (image->GetType() == eStyleImageType_Image) {
      imgIRequest* imgreq = image->GetImageData();
      nsCOMPtr<imgIContainer> image;
      if (NS_SUCCEEDED(imgreq->GetImage(getter_AddRefs(image))) && image) {
        if (NS_FAILED(image->GetAnimated(&animated))) {
          animated = false;
        }
      }
    }
  }

  if (!animated ||
      !nsLayoutUtils::AnimatedImageLayersEnabled()) {
    if (!aManager->IsCompositingCheap() ||
        !nsLayoutUtils::GPUImageScalingEnabled()) {
      return LAYER_NONE;
    }
  }

  if (!TryOptimizeToImageLayer(aManager, aBuilder)) {
    return LAYER_NONE;
  }

  if (!animated) {
    mozilla::gfx::IntSize imageSize = mImageContainer->GetCurrentSize();
    NS_ASSERTION(imageSize.width != 0 && imageSize.height != 0, "Invalid image size!");

    gfxRect destRect = mDestRect;

    destRect.width *= aParameters.mXScale;
    destRect.height *= aParameters.mYScale;

    
    gfxSize scale = gfxSize(destRect.width / imageSize.width, destRect.height / imageSize.height);

    
    if (scale.width == 1.0f && scale.height == 1.0f) {
      return LAYER_NONE;
    }

    
    if (destRect.width * destRect.height < 64 * 64) {
      return LAYER_NONE;
    }
  }

  return LAYER_ACTIVE;
}

already_AddRefed<Layer>
nsDisplayBackgroundImage::BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     const ContainerLayerParameters& aParameters)
{
  nsRefPtr<ImageLayer> layer = static_cast<ImageLayer*>
    (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, this));
  if (!layer) {
    layer = aManager->CreateImageLayer();
    if (!layer)
      return nullptr;
  }
  layer->SetContainer(mImageContainer);
  ConfigureLayer(layer, aParameters.mOffset);
  return layer.forget();
}

void
nsDisplayBackgroundImage::ConfigureLayer(ImageLayer* aLayer, const nsIntPoint& aOffset)
{
  aLayer->SetFilter(nsLayoutUtils::GetGraphicsFilterForFrame(mFrame));

  mozilla::gfx::IntSize imageSize = mImageContainer->GetCurrentSize();
  NS_ASSERTION(imageSize.width != 0 && imageSize.height != 0, "Invalid image size!");

  gfxPoint p = mDestRect.TopLeft() + aOffset;
  Matrix transform = Matrix::Translation(p.x, p.y);
  transform.PreScale(mDestRect.width / imageSize.width,
                     mDestRect.height / imageSize.height);
  aLayer->SetBaseTransform(gfx::Matrix4x4::From2D(transform));
}

void
nsDisplayBackgroundImage::HitTest(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aRect,
                                  HitTestState* aState,
                                  nsTArray<nsIFrame*> *aOutFrames)
{
  if (RoundedBorderIntersectsRect(mFrame, ToReferenceFrame(), aRect)) {
    aOutFrames->AppendElement(mFrame);
  }
}

bool
nsDisplayBackgroundImage::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                            nsRegion* aVisibleRegion)
{
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion)) {
    return false;
  }

  
  
  
  return mBackgroundStyle;
}

 nsRegion
nsDisplayBackgroundImage::GetInsideClipRegion(nsDisplayItem* aItem,
                                              nsPresContext* aPresContext,
                                              uint8_t aClip, const nsRect& aRect,
                                              bool* aSnap)
{
  nsRegion result;
  if (aRect.IsEmpty())
    return result;

  nsIFrame *frame = aItem->Frame();

  nsRect clipRect;
  if (frame->GetType() == nsGkAtoms::canvasFrame) {
    nsCanvasFrame* canvasFrame = static_cast<nsCanvasFrame*>(frame);
    clipRect = canvasFrame->CanvasArea() + aItem->ToReferenceFrame();
  } else {
    switch (aClip) {
    case NS_STYLE_BG_CLIP_BORDER:
      clipRect = nsRect(aItem->ToReferenceFrame(), frame->GetSize());
      break;
    case NS_STYLE_BG_CLIP_PADDING:
      clipRect = frame->GetPaddingRect() - frame->GetPosition() + aItem->ToReferenceFrame();
      break;
    case NS_STYLE_BG_CLIP_CONTENT:
      clipRect = frame->GetContentRectRelativeToSelf() + aItem->ToReferenceFrame();
      break;
    default:
      NS_NOTREACHED("Unknown clip type");
      return result;
    }
  }

  return clipRect.Intersect(aRect);
}

nsRegion
nsDisplayBackgroundImage::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                          bool* aSnap) {
  nsRegion result;
  *aSnap = false;

  if (!mBackgroundStyle)
    return result;


  *aSnap = true;

  
  
  
  
  
  if (mFrame->StyleBorder()->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_CLONE ||
      (!mFrame->GetPrevContinuation() && !mFrame->GetNextContinuation())) {
    const nsStyleBackground::Layer& layer = mBackgroundStyle->mLayers[mLayer];
    if (layer.mImage.IsOpaque() && layer.mBlendMode == NS_STYLE_BLEND_NORMAL) {
      nsPresContext* presContext = mFrame->PresContext();
      result = GetInsideClipRegion(this, presContext, layer.mClip, mBounds, aSnap);
    }
  }

  return result;
}

bool
nsDisplayBackgroundImage::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) {
  if (!mBackgroundStyle) {
    *aColor = NS_RGBA(0,0,0,0);
    return true;
  }
  return false;
}

nsRect
nsDisplayBackgroundImage::GetPositioningArea()
{
  if (!mBackgroundStyle) {
    return nsRect();
  }
  nsIFrame* attachedToFrame;
  return nsCSSRendering::ComputeBackgroundPositioningArea(
      mFrame->PresContext(), mFrame,
      nsRect(ToReferenceFrame(), mFrame->GetSize()),
      mBackgroundStyle->mLayers[mLayer],
      &attachedToFrame) + ToReferenceFrame();
}

bool
nsDisplayBackgroundImage::RenderingMightDependOnPositioningAreaSizeChange()
{
  if (!mBackgroundStyle)
    return false;

  nscoord radii[8];
  if (mFrame->GetBorderRadii(radii)) {
    
    
    return true;
  }

  const nsStyleBackground::Layer &layer = mBackgroundStyle->mLayers[mLayer];
  if (layer.RenderingMightDependOnPositioningAreaSizeChange()) {
    return true;
  }
  return false;
}

static void CheckForBorderItem(nsDisplayItem *aItem, uint32_t& aFlags)
{
  nsDisplayItem* nextItem = aItem->GetAbove();
  while (nextItem && nextItem->GetType() == nsDisplayItem::TYPE_BACKGROUND) {
    nextItem = nextItem->GetAbove();
  }
  if (nextItem && 
      nextItem->Frame() == aItem->Frame() &&
      nextItem->GetType() == nsDisplayItem::TYPE_BORDER) {
    aFlags |= nsCSSRendering::PAINTBG_WILL_PAINT_BORDER;
  }
}

void
nsDisplayBackgroundImage::Paint(nsDisplayListBuilder* aBuilder,
                                nsRenderingContext* aCtx) {
  PaintInternal(aBuilder, aCtx, mVisibleRect, &mBounds);
}

void
nsDisplayBackgroundImage::PaintInternal(nsDisplayListBuilder* aBuilder,
                                        nsRenderingContext* aCtx, const nsRect& aBounds,
                                        nsRect* aClipRect) {
  nsPoint offset = ToReferenceFrame();
  uint32_t flags = aBuilder->GetBackgroundPaintFlags();
  CheckForBorderItem(this, flags);

  nsCSSRendering::PaintBackground(mFrame->PresContext(), *aCtx, mFrame,
                                  aBounds,
                                  nsRect(offset, mFrame->GetSize()),
                                  flags, aClipRect, mLayer);

}

void nsDisplayBackgroundImage::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                         const nsDisplayItemGeometry* aGeometry,
                                                         nsRegion* aInvalidRegion)
{
  if (!mBackgroundStyle) {
    return;
  }

  const nsDisplayBackgroundGeometry* geometry = static_cast<const nsDisplayBackgroundGeometry*>(aGeometry);

  bool snap;
  nsRect bounds = GetBounds(aBuilder, &snap);
  nsRect positioningArea = GetPositioningArea();
  if (positioningArea.TopLeft() != geometry->mPositioningArea.TopLeft() ||
      (positioningArea.Size() != geometry->mPositioningArea.Size() &&
       RenderingMightDependOnPositioningAreaSizeChange())) {
    
    
    aInvalidRegion->Or(bounds, geometry->mBounds);

    if (positioningArea.Size() != geometry->mPositioningArea.Size()) {
      NotifyRenderingChanged();
    }
    return;
  }
  if (aBuilder->ShouldSyncDecodeImages()) {
    if (mBackgroundStyle &&
        !nsCSSRendering::IsBackgroundImageDecodedForStyleContextAndLayer(mBackgroundStyle, mLayer)) {
      aInvalidRegion->Or(*aInvalidRegion, bounds);

      NotifyRenderingChanged();
    }
  }
  if (!bounds.IsEqualInterior(geometry->mBounds)) {
    
    
    aInvalidRegion->Xor(bounds, geometry->mBounds);

    NotifyRenderingChanged();
  }
}

nsRect
nsDisplayBackgroundImage::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = true;
  return mBounds;
}

nsRect
nsDisplayBackgroundImage::GetBoundsInternal(nsDisplayListBuilder* aBuilder) {
  nsPresContext* presContext = mFrame->PresContext();

  if (!mBackgroundStyle) {
    return nsRect();
  }

  nsRect borderBox = nsRect(ToReferenceFrame(), mFrame->GetSize());
  nsRect clipRect = borderBox;
  if (mFrame->GetType() == nsGkAtoms::canvasFrame) {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    clipRect = frame->CanvasArea() + ToReferenceFrame();
  }
  const nsStyleBackground::Layer& layer = mBackgroundStyle->mLayers[mLayer];
  return nsCSSRendering::GetBackgroundLayerRect(presContext, mFrame,
                                                borderBox, clipRect, layer,
                                                aBuilder->GetBackgroundPaintFlags());
}

uint32_t
nsDisplayBackgroundImage::GetPerFrameKey()
{
  return (mLayer << nsDisplayItem::TYPE_BITS) |
    nsDisplayItem::GetPerFrameKey();
}

nsDisplayThemedBackground::nsDisplayThemedBackground(nsDisplayListBuilder* aBuilder,
                                                     nsIFrame* aFrame)
  : nsDisplayItem(aBuilder, aFrame)
{
  MOZ_COUNT_CTOR(nsDisplayThemedBackground);

  const nsStyleDisplay* disp = mFrame->StyleDisplay();
  mAppearance = disp->mAppearance;
  mFrame->IsThemed(disp, &mThemeTransparency);

  
  switch (disp->mAppearance) {
    case NS_THEME_MOZ_MAC_UNIFIED_TOOLBAR:
    case NS_THEME_TOOLBAR:
    case NS_THEME_WINDOW_TITLEBAR:
    case NS_THEME_WINDOW_BUTTON_BOX:
    case NS_THEME_MOZ_MAC_FULLSCREEN_BUTTON:
    case NS_THEME_WINDOW_BUTTON_BOX_MAXIMIZED:
    case NS_THEME_MAC_VIBRANCY_LIGHT:
    case NS_THEME_MAC_VIBRANCY_DARK:
      RegisterThemeGeometry(aBuilder, aFrame);
      break;
    case NS_THEME_WIN_BORDERLESS_GLASS:
    case NS_THEME_WIN_GLASS:
      aBuilder->SetGlassDisplayItem(this);
      break;
  }

  mBounds = GetBoundsInternal();
}

nsDisplayThemedBackground::~nsDisplayThemedBackground()
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_DTOR(nsDisplayThemedBackground);
#endif
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayThemedBackground::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (themed, appearance:%d)", mAppearance);
}
#endif

void
nsDisplayThemedBackground::HitTest(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aRect,
                                  HitTestState* aState,
                                  nsTArray<nsIFrame*> *aOutFrames)
{
  
  if (nsRect(ToReferenceFrame(), mFrame->GetSize()).Intersects(aRect)) {
    aOutFrames->AppendElement(mFrame);
  }
}

nsRegion
nsDisplayThemedBackground::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                           bool* aSnap) {
  nsRegion result;
  *aSnap = false;

  if (mThemeTransparency == nsITheme::eOpaque) {
    result = nsRect(ToReferenceFrame(), mFrame->GetSize());
  }
  return result;
}

bool
nsDisplayThemedBackground::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) {
  if (mAppearance == NS_THEME_WIN_BORDERLESS_GLASS ||
      mAppearance == NS_THEME_WIN_GLASS) {
    *aColor = NS_RGBA(0,0,0,0);
    return true;
  }
  return false;
}

nsRect
nsDisplayThemedBackground::GetPositioningArea()
{
  return nsRect(ToReferenceFrame(), mFrame->GetSize());
}

void
nsDisplayThemedBackground::Paint(nsDisplayListBuilder* aBuilder,
                                 nsRenderingContext* aCtx)
{
  PaintInternal(aBuilder, aCtx, mVisibleRect, nullptr);
}


void
nsDisplayThemedBackground::PaintInternal(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx, const nsRect& aBounds,
                                         nsRect* aClipRect)
{
  
  nsPresContext* presContext = mFrame->PresContext();
  nsITheme *theme = presContext->GetTheme();
  nsRect borderArea(ToReferenceFrame(), mFrame->GetSize());
  nsRect drawing(borderArea);
  theme->GetWidgetOverflow(presContext->DeviceContext(), mFrame, mAppearance,
                           &drawing);
  drawing.IntersectRect(drawing, aBounds);
  theme->DrawWidgetBackground(aCtx, mFrame, mAppearance, borderArea, drawing);
}

bool nsDisplayThemedBackground::IsWindowActive()
{
  EventStates docState = mFrame->GetContent()->OwnerDoc()->GetDocumentState();
  return !docState.HasState(NS_DOCUMENT_STATE_WINDOW_INACTIVE);
}

void nsDisplayThemedBackground::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                          const nsDisplayItemGeometry* aGeometry,
                                                          nsRegion* aInvalidRegion)
{
  const nsDisplayThemedBackgroundGeometry* geometry = static_cast<const nsDisplayThemedBackgroundGeometry*>(aGeometry);

  bool snap;
  nsRect bounds = GetBounds(aBuilder, &snap);
  nsRect positioningArea = GetPositioningArea();
  if (!positioningArea.IsEqualInterior(geometry->mPositioningArea)) {
    
    aInvalidRegion->Or(bounds, geometry->mBounds);
    return;
  }
  if (!bounds.IsEqualInterior(geometry->mBounds)) {
    
    
    aInvalidRegion->Xor(bounds, geometry->mBounds);
  }
  nsITheme* theme = mFrame->PresContext()->GetTheme();
  if (theme->WidgetAppearanceDependsOnWindowFocus(mAppearance) &&
      IsWindowActive() != geometry->mWindowIsActive) {
    aInvalidRegion->Or(*aInvalidRegion, bounds);
  }
}

nsRect
nsDisplayThemedBackground::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = true;
  return mBounds;
}

nsRect
nsDisplayThemedBackground::GetBoundsInternal() {
  nsPresContext* presContext = mFrame->PresContext();

  nsRect r(nsPoint(0,0), mFrame->GetSize());
  presContext->GetTheme()->
      GetWidgetOverflow(presContext->DeviceContext(), mFrame,
                        mFrame->StyleDisplay()->mAppearance, &r);
#ifdef XP_MACOSX
  
  r.Inflate(mFrame->PresContext()->AppUnitsPerDevPixel());
#endif

  return r + ToReferenceFrame();
}

bool
nsDisplayBackgroundColor::ApplyOpacity(nsDisplayListBuilder* aBuilder,
                                       float aOpacity,
                                       const DisplayItemClip* aClip)
{
  mColor.a = mColor.a * aOpacity;
  if (aClip) {
    IntersectClip(aBuilder, *aClip);
  }
  return true;
}

void
nsDisplayBackgroundColor::Paint(nsDisplayListBuilder* aBuilder,
                                nsRenderingContext* aCtx)
{
  if (mColor == NS_RGBA(0, 0, 0, 0)) {
    return;
  }

  gfxContext* ctx = aCtx->ThebesContext();
  nsRect borderBox = nsRect(ToReferenceFrame(), mFrame->GetSize());

  gfxRect bounds =
    nsLayoutUtils::RectToGfxRect(borderBox, mFrame->PresContext()->AppUnitsPerDevPixel());

  ctx->SetColor(mColor);
  ctx->NewPath();
  ctx->Rectangle(bounds, true);
  ctx->Fill();
}

nsRegion
nsDisplayBackgroundColor::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                          bool* aSnap)
{
  if (mColor.a != 1) {
    return nsRegion();
  }

  if (!mBackgroundStyle)
    return nsRegion();

  *aSnap = true;

  const nsStyleBackground::Layer& bottomLayer = mBackgroundStyle->BottomLayer();
  nsRect borderBox = nsRect(ToReferenceFrame(), mFrame->GetSize());
  nsPresContext* presContext = mFrame->PresContext();
  return nsDisplayBackgroundImage::GetInsideClipRegion(this, presContext, bottomLayer.mClip, borderBox, aSnap);
}

bool
nsDisplayBackgroundColor::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor)
{
  *aColor = NS_RGBA_FROM_GFXRGBA(mColor);
  return true;
}

void
nsDisplayBackgroundColor::HitTest(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aRect,
                                  HitTestState* aState,
                                  nsTArray<nsIFrame*> *aOutFrames)
{
  if (!RoundedBorderIntersectsRect(mFrame, ToReferenceFrame(), aRect)) {
    
    return;
  }

  aOutFrames->AppendElement(mFrame);
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayBackgroundColor::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (rgba %f,%f,%f,%f)",
          mColor.r, mColor.g,
          mColor.b, mColor.a);
}
#endif

already_AddRefed<Layer>
nsDisplayClearBackground::BuildLayer(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     const ContainerLayerParameters& aParameters)
{
  nsRefPtr<ColorLayer> layer = static_cast<ColorLayer*>
    (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, this));
  if (!layer) {
    layer = aManager->CreateColorLayer();
    if (!layer)
      return nullptr;
  }
  layer->SetColor(NS_RGBA(0, 0, 0, 0));
  layer->SetMixBlendMode(gfx::CompositionOp::OP_SOURCE);

  bool snap;
  nsRect bounds = GetBounds(aBuilder, &snap);
  int32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  layer->SetBounds(bounds.ToNearestPixels(appUnitsPerDevPixel)); 

  return layer.forget();
}

nsRect
nsDisplayOutline::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
}

void
nsDisplayOutline::Paint(nsDisplayListBuilder* aBuilder,
                        nsRenderingContext* aCtx) {
  
  nsPoint offset = ToReferenceFrame();
  nsCSSRendering::PaintOutline(mFrame->PresContext(), *aCtx, mFrame,
                               mVisibleRect,
                               nsRect(offset, mFrame->GetSize()),
                               mFrame->StyleContext());
}

bool
nsDisplayOutline::IsInvisibleInRect(const nsRect& aRect)
{
  const nsStyleOutline* outline = mFrame->StyleOutline();
  nsRect borderBox(ToReferenceFrame(), mFrame->GetSize());
  if (borderBox.Contains(aRect) &&
      !nsLayoutUtils::HasNonZeroCorner(outline->mOutlineRadius)) {
    if (outline->mOutlineOffset >= 0) {
      
      
      return true;
    }
  }

  return false;
}

void
nsDisplayEventReceiver::HitTest(nsDisplayListBuilder* aBuilder,
                                const nsRect& aRect,
                                HitTestState* aState,
                                nsTArray<nsIFrame*> *aOutFrames)
{
  if (!RoundedBorderIntersectsRect(mFrame, ToReferenceFrame(), aRect)) {
    
    return;
  }

  aOutFrames->AppendElement(mFrame);
}

void
nsDisplayLayerEventRegions::AddFrame(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame)
{
  NS_ASSERTION(aBuilder->FindReferenceFrameFor(aFrame) == aBuilder->FindReferenceFrameFor(mFrame),
               "Reference frame mismatch");
  uint8_t pointerEvents = aFrame->StyleVisibility()->mPointerEvents;
  if (pointerEvents == NS_STYLE_POINTER_EVENTS_NONE) {
    return;
  }
  
  
  
  nsRect borderBox(aBuilder->ToReferenceFrame(aFrame), aFrame->GetSize());
  const DisplayItemClip* clip = aBuilder->ClipState().GetCurrentCombinedClip(aBuilder);
  bool borderBoxHasRoundedCorners =
    nsLayoutUtils::HasNonZeroCorner(aFrame->StyleBorder()->mBorderRadius);
  if (clip) {
    borderBox = clip->ApplyNonRoundedIntersection(borderBox);
    if (clip->GetRoundedRectCount() > 0) {
      borderBoxHasRoundedCorners = true;
    }
  }
  if (borderBoxHasRoundedCorners ||
      (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    mMaybeHitRegion.Or(mMaybeHitRegion, borderBox);
  } else {
    mHitRegion.Or(mHitRegion, borderBox);
  }
  if (aBuilder->GetAncestorHasTouchEventHandler()) {
    mDispatchToContentHitRegion.Or(mDispatchToContentHitRegion, borderBox);
  }
}

nsDisplayCaret::nsDisplayCaret(nsDisplayListBuilder* aBuilder,
                               nsIFrame* aCaretFrame)
  : nsDisplayItem(aBuilder, aCaretFrame)
  , mCaret(aBuilder->GetCaret())
  , mBounds(aBuilder->GetCaretRect() + ToReferenceFrame())
{
  MOZ_COUNT_CTOR(nsDisplayCaret);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayCaret::~nsDisplayCaret()
{
  MOZ_COUNT_DTOR(nsDisplayCaret);
}
#endif

nsRect
nsDisplayCaret::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = true;
  
  return mBounds;
}

void
nsDisplayCaret::Paint(nsDisplayListBuilder* aBuilder,
                      nsRenderingContext* aCtx) {
  
  
  mCaret->PaintCaret(aBuilder, aCtx, mFrame, ToReferenceFrame());
}

bool
nsDisplayBorder::IsInvisibleInRect(const nsRect& aRect)
{
  nsRect paddingRect = mFrame->GetPaddingRect() - mFrame->GetPosition() +
    ToReferenceFrame();
  const nsStyleBorder *styleBorder;
  if (paddingRect.Contains(aRect) &&
      !(styleBorder = mFrame->StyleBorder())->IsBorderImageLoaded() &&
      !nsLayoutUtils::HasNonZeroCorner(styleBorder->mBorderRadius)) {
    
    
    
    
    
    
    return true;
  }

  return false;
}
  
nsDisplayItemGeometry* 
nsDisplayBorder::AllocateGeometry(nsDisplayListBuilder* aBuilder)
{
  return new nsDisplayBorderGeometry(this, aBuilder);
}

void
nsDisplayBorder::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                           const nsDisplayItemGeometry* aGeometry,
                                           nsRegion* aInvalidRegion)
{
  const nsDisplayBorderGeometry* geometry = static_cast<const nsDisplayBorderGeometry*>(aGeometry);
  bool snap;
  if (!geometry->mBounds.IsEqualInterior(GetBounds(aBuilder, &snap)) ||
      !geometry->mContentRect.IsEqualInterior(GetContentRect())) {
    
    
    
    aInvalidRegion->Or(GetBounds(aBuilder, &snap), geometry->mBounds);
  }
}
  
void
nsDisplayBorder::Paint(nsDisplayListBuilder* aBuilder,
                       nsRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsCSSRendering::PaintBorder(mFrame->PresContext(), *aCtx, mFrame,
                              mVisibleRect,
                              nsRect(offset, mFrame->GetSize()),
                              mFrame->StyleContext(),
                              mFrame->GetSkipSides());
}

nsRect
nsDisplayBorder::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = true;
  return CalculateBounds(*mFrame->StyleBorder());
}

nsRect
nsDisplayBorder::CalculateBounds(const nsStyleBorder& aStyleBorder)
{
  nsRect borderBounds(ToReferenceFrame(), mFrame->GetSize());
  if (aStyleBorder.IsBorderImageLoaded()) {
    borderBounds.Inflate(aStyleBorder.GetImageOutset());
    return borderBounds;
  } else {
    nsMargin border = aStyleBorder.GetComputedBorder();
    nsRect result;
    if (border.top > 0) {
      result = nsRect(borderBounds.X(), borderBounds.Y(), borderBounds.Width(), border.top);
    }
    if (border.right > 0) {
      result.UnionRect(result, nsRect(borderBounds.XMost() - border.right, borderBounds.Y(), border.right, borderBounds.Height()));
    }
    if (border.bottom > 0) {
      result.UnionRect(result, nsRect(borderBounds.X(), borderBounds.YMost() - border.bottom, borderBounds.Width(), border.bottom));
    }
    if (border.left > 0) {
      result.UnionRect(result, nsRect(borderBounds.X(), borderBounds.Y(), border.left, borderBounds.Height()));
    }

    return result;
  }
}






static void
ComputeDisjointRectangles(const nsRegion& aRegion,
                          nsTArray<nsRect>* aRects) {
  nscoord accumulationMargin = nsPresContext::CSSPixelsToAppUnits(25);
  nsRect accumulated;
  nsRegionRectIterator iter(aRegion);
  while (true) {
    const nsRect* r = iter.Next();
    if (r && !accumulated.IsEmpty() &&
        accumulated.YMost() >= r->y - accumulationMargin) {
      accumulated.UnionRect(accumulated, *r);
      continue;
    }

    if (!accumulated.IsEmpty()) {
      aRects->AppendElement(accumulated);
      accumulated.SetEmpty();
    }

    if (!r)
      break;

    accumulated = *r;
  }
}

void
nsDisplayBoxShadowOuter::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsRect borderRect = mFrame->VisualBorderRectRelativeToSelf() + offset;
  nsPresContext* presContext = mFrame->PresContext();
  nsAutoTArray<nsRect,10> rects;
  ComputeDisjointRectangles(mVisibleRegion, &rects);

  PROFILER_LABEL("nsDisplayBoxShadowOuter", "Paint",
    js::ProfileEntry::Category::GRAPHICS);

  for (uint32_t i = 0; i < rects.Length(); ++i) {
    nsCSSRendering::PaintBoxShadowOuter(presContext, *aCtx, mFrame,
                                        borderRect, rects[i], mOpacity);
  }
}

nsRect
nsDisplayBoxShadowOuter::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mBounds;
}

nsRect
nsDisplayBoxShadowOuter::GetBoundsInternal() {
  return nsLayoutUtils::GetBoxShadowRectForFrame(mFrame, mFrame->GetSize()) +
         ToReferenceFrame();
}

bool
nsDisplayBoxShadowOuter::IsInvisibleInRect(const nsRect& aRect)
{
  nsPoint origin = ToReferenceFrame();
  nsRect frameRect(origin, mFrame->GetSize());
  if (!frameRect.Contains(aRect))
    return false;

  
  
  nscoord twipsRadii[8];
  bool hasBorderRadii = mFrame->GetBorderRadii(twipsRadii);
  if (!hasBorderRadii)
    return true;

  return RoundedRectContainsRect(frameRect, twipsRadii, aRect);
}

bool
nsDisplayBoxShadowOuter::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion)) {
    return false;
  }

  
  mVisibleRegion.And(*aVisibleRegion, mVisibleRect);
  return true;
}

void
nsDisplayBoxShadowOuter::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                   const nsDisplayItemGeometry* aGeometry,
                                                   nsRegion* aInvalidRegion)
{
  const nsDisplayBoxShadowOuterGeometry* geometry =
    static_cast<const nsDisplayBoxShadowOuterGeometry*>(aGeometry);
  bool snap;
  if (!geometry->mBounds.IsEqualInterior(GetBounds(aBuilder, &snap)) ||
      !geometry->mBorderRect.IsEqualInterior(GetBorderRect()) ||
      mOpacity != geometry->mOpacity) {
    nsRegion oldShadow, newShadow;
    nscoord dontCare[8];
    bool hasBorderRadius = mFrame->GetBorderRadii(dontCare);
    if (hasBorderRadius) {
      
      
      oldShadow = geometry->mBounds;
      newShadow = GetBounds(aBuilder, &snap);
    } else {
      oldShadow = oldShadow.Sub(geometry->mBounds, geometry->mBorderRect);
      newShadow = newShadow.Sub(GetBounds(aBuilder, &snap), GetBorderRect());
    }
    aInvalidRegion->Or(oldShadow, newShadow);
  }
}


void
nsDisplayBoxShadowInner::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsRect borderRect = nsRect(offset, mFrame->GetSize());
  nsPresContext* presContext = mFrame->PresContext();
  nsAutoTArray<nsRect,10> rects;
  ComputeDisjointRectangles(mVisibleRegion, &rects);

  PROFILER_LABEL("nsDisplayBoxShadowInner", "Paint",
    js::ProfileEntry::Category::GRAPHICS);

  for (uint32_t i = 0; i < rects.Length(); ++i) {
    aCtx->ThebesContext()->Save();
    aCtx->IntersectClip(rects[i]);
    nsCSSRendering::PaintBoxShadowInner(presContext, *aCtx, mFrame,
                                        borderRect, rects[i]);
    aCtx->ThebesContext()->Restore();
  }
}

bool
nsDisplayBoxShadowInner::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion)) {
    return false;
  }

  
  mVisibleRegion.And(*aVisibleRegion, mVisibleRect);
  return true;
}

nsDisplayWrapList::nsDisplayWrapList(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayList* aList)
  : nsDisplayItem(aBuilder, aFrame)
  , mOverrideZIndex(0)
  , mHasZIndexOverride(false)
{
  MOZ_COUNT_CTOR(nsDisplayWrapList);

  mList.AppendToTop(aList);
  UpdateBounds(aBuilder);

  if (!aFrame || !aFrame->IsTransformed()) {
    return;
  }

  
  
  
  
  if (aFrame->Preserves3DChildren()) {
    mReferenceFrame = 
      aBuilder->FindReferenceFrameFor(GetTransformRootFrame(aFrame));
    mToReferenceFrame = aFrame->GetOffsetToCrossDoc(mReferenceFrame);
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    nsDisplayItem *i = mList.GetBottom();
    if (i && (!i->GetAbove() || i->GetType() == TYPE_TRANSFORM) &&
        i->Frame() == mFrame) {
      mReferenceFrame = i->ReferenceFrame();
      mToReferenceFrame = i->ToReferenceFrame();
    }
  }
  mVisibleRect = aBuilder->GetDirtyRect() +
      aBuilder->GetCurrentFrameOffsetToReferenceFrame();
}

nsDisplayWrapList::nsDisplayWrapList(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayItem* aItem)
  : nsDisplayItem(aBuilder, aFrame)
  , mOverrideZIndex(0)
  , mHasZIndexOverride(false)
{
  MOZ_COUNT_CTOR(nsDisplayWrapList);

  mList.AppendToTop(aItem);
  UpdateBounds(aBuilder);
  
  if (!aFrame || !aFrame->IsTransformed()) {
    return;
  }

  if (aFrame->Preserves3DChildren()) {
    mReferenceFrame = 
      aBuilder->FindReferenceFrameFor(GetTransformRootFrame(aFrame));
    mToReferenceFrame = aFrame->GetOffsetToCrossDoc(mReferenceFrame);
  } else {
    
    if (aItem->Frame() == aFrame) {
      mReferenceFrame = aItem->ReferenceFrame();
      mToReferenceFrame = aItem->ToReferenceFrame();
    }
  }
  mVisibleRect = aBuilder->GetDirtyRect() +
      aBuilder->GetCurrentFrameOffsetToReferenceFrame();
}

nsDisplayWrapList::~nsDisplayWrapList() {
  mList.DeleteAll();

  MOZ_COUNT_DTOR(nsDisplayWrapList);
}

void
nsDisplayWrapList::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                           HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
  mList.HitTest(aBuilder, aRect, aState, aOutFrames);
}

nsRect
nsDisplayWrapList::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mBounds;
}

bool
nsDisplayWrapList::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                     nsRegion* aVisibleRegion) {
  
  nsRegion visibleRegion;
  
  visibleRegion.And(*aVisibleRegion, mVisibleRect);
  nsRegion originalVisibleRegion = visibleRegion;

  bool retval =
    mList.ComputeVisibilityForSublist(aBuilder, &visibleRegion,
                                      mVisibleRect);

  nsRegion removed;
  
  removed.Sub(originalVisibleRegion, visibleRegion);
  
  
  aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);

  return retval;
}

nsRegion
nsDisplayWrapList::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) {
  *aSnap = false;
  nsRegion result;
  if (mList.IsOpaque()) {
    
    result = GetBounds(aBuilder, aSnap);
  }
  return result;
}

bool nsDisplayWrapList::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) {
  
  return false;
}

void nsDisplayWrapList::Paint(nsDisplayListBuilder* aBuilder,
                              nsRenderingContext* aCtx) {
  NS_ERROR("nsDisplayWrapList should have been flattened away for painting");
}






static LayerState
RequiredLayerStateForChildren(nsDisplayListBuilder* aBuilder,
                              LayerManager* aManager,
                              const ContainerLayerParameters& aParameters,
                              const nsDisplayList& aList,
                              nsIFrame* aExpectedAnimatedGeometryRootForChildren)
{
  LayerState result = LAYER_INACTIVE;
  for (nsDisplayItem* i = aList.GetBottom(); i; i = i->GetAbove()) {
    if (result == LAYER_INACTIVE &&
        nsLayoutUtils::GetAnimatedGeometryRootFor(i, aBuilder, aManager) !=
          aExpectedAnimatedGeometryRootForChildren) {
      result = LAYER_ACTIVE;
    }

    LayerState state = i->GetLayerState(aBuilder, aManager, aParameters);
    if ((state == LAYER_ACTIVE || state == LAYER_ACTIVE_FORCE) &&
        state > result) {
      result = state;
    }
    if (state == LAYER_ACTIVE_EMPTY && state > result) {
      result = LAYER_ACTIVE_FORCE;
    }
    if (state == LAYER_NONE) {
      nsDisplayList* list = i->GetSameCoordinateSystemChildren();
      if (list) {
        LayerState childState =
          RequiredLayerStateForChildren(aBuilder, aManager, aParameters, *list,
              aExpectedAnimatedGeometryRootForChildren);
        if (childState > result) {
          result = childState;
        }
      }
    }
  }
  return result;
}

nsRect nsDisplayWrapList::GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
{
  nsRect bounds;
  for (nsDisplayItem* i = mList.GetBottom(); i; i = i->GetAbove()) {
    bounds.UnionRect(bounds, i->GetComponentAlphaBounds(aBuilder));
  }
  return bounds;
}

void
nsDisplayWrapList::SetVisibleRect(const nsRect& aRect)
{
  mVisibleRect = aRect;
}

static nsresult
WrapDisplayList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                nsDisplayList* aList, nsDisplayWrapper* aWrapper) {
  if (!aList->GetTop())
    return NS_OK;
  nsDisplayItem* item = aWrapper->WrapList(aBuilder, aFrame, aList);
  if (!item)
    return NS_ERROR_OUT_OF_MEMORY;
  
  aList->AppendToTop(item);
  return NS_OK;
}

static nsresult
WrapEachDisplayItem(nsDisplayListBuilder* aBuilder,
                    nsDisplayList* aList, nsDisplayWrapper* aWrapper) {
  nsDisplayList newList;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom())) {
    item = aWrapper->WrapItem(aBuilder, item);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    newList.AppendToTop(item);
  }
  
  aList->AppendToTop(&newList);
  return NS_OK;
}

nsresult nsDisplayWrapper::WrapLists(nsDisplayListBuilder* aBuilder,
    nsIFrame* aFrame, const nsDisplayListSet& aIn, const nsDisplayListSet& aOut)
{
  nsresult rv = WrapListsInPlace(aBuilder, aFrame, aIn);
  NS_ENSURE_SUCCESS(rv, rv);

  if (&aOut == &aIn)
    return NS_OK;
  aOut.BorderBackground()->AppendToTop(aIn.BorderBackground());
  aOut.BlockBorderBackgrounds()->AppendToTop(aIn.BlockBorderBackgrounds());
  aOut.Floats()->AppendToTop(aIn.Floats());
  aOut.Content()->AppendToTop(aIn.Content());
  aOut.PositionedDescendants()->AppendToTop(aIn.PositionedDescendants());
  aOut.Outlines()->AppendToTop(aIn.Outlines());
  return NS_OK;
}

nsresult nsDisplayWrapper::WrapListsInPlace(nsDisplayListBuilder* aBuilder,
    nsIFrame* aFrame, const nsDisplayListSet& aLists)
{
  nsresult rv;
  if (WrapBorderBackground()) {
    
    rv = WrapDisplayList(aBuilder, aFrame, aLists.BorderBackground(), this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = WrapDisplayList(aBuilder, aFrame, aLists.BlockBorderBackgrounds(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapEachDisplayItem(aBuilder, aLists.Floats(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapDisplayList(aBuilder, aFrame, aLists.Content(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapEachDisplayItem(aBuilder, aLists.PositionedDescendants(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return WrapEachDisplayItem(aBuilder, aLists.Outlines(), this);
}

nsDisplayOpacity::nsDisplayOpacity(nsDisplayListBuilder* aBuilder,
                                   nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList)
    , mOpacity(aFrame->StyleDisplay()->mOpacity) {
  MOZ_COUNT_CTOR(nsDisplayOpacity);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayOpacity::~nsDisplayOpacity() {
  MOZ_COUNT_DTOR(nsDisplayOpacity);
}
#endif

nsRegion nsDisplayOpacity::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                           bool* aSnap) {
  *aSnap = false;
  
  
  
  return nsRegion();
}


already_AddRefed<Layer>
nsDisplayOpacity::BuildLayer(nsDisplayListBuilder* aBuilder,
                             LayerManager* aManager,
                             const ContainerLayerParameters& aContainerParameters) {
  if (mOpacity == 0 && mFrame->GetContent() &&
      !nsLayoutUtils::HasAnimations(mFrame->GetContent(), eCSSProperty_opacity)) {
    return nullptr;
  }
  nsRefPtr<Layer> container = aManager->GetLayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                           aContainerParameters, nullptr);
  if (!container)
    return nullptr;

  container->SetOpacity(mOpacity);
  nsDisplayListBuilder::AddAnimationsAndTransitionsToLayer(container, aBuilder,
                                                           this, mFrame,
                                                           eCSSProperty_opacity);
  return container.forget();
}






static bool
IsItemTooSmallForActiveLayer(nsDisplayItem* aItem)
{
  nsIntRect visibleDevPixels = aItem->GetVisibleRect().ToOutsidePixels(
          aItem->Frame()->PresContext()->AppUnitsPerDevPixel());
  static const int MIN_ACTIVE_LAYER_SIZE_DEV_PIXELS = 16;
  return visibleDevPixels.Size() <
    nsIntSize(MIN_ACTIVE_LAYER_SIZE_DEV_PIXELS, MIN_ACTIVE_LAYER_SIZE_DEV_PIXELS);
}

bool
nsDisplayOpacity::NeedsActiveLayer()
{
  if (ActiveLayerTracker::IsStyleAnimated(mFrame, eCSSProperty_opacity) &&
      !IsItemTooSmallForActiveLayer(this))
    return true;
  if (mFrame->GetContent()) {
    if (nsLayoutUtils::HasAnimationsForCompositor(mFrame->GetContent(),
                                                  eCSSProperty_opacity)) {
      return true;
    }
  }
  return false;
}

bool
nsDisplayOpacity::ApplyOpacity(nsDisplayListBuilder* aBuilder,
                             float aOpacity,
                             const DisplayItemClip* aClip)
{
  mOpacity = mOpacity * aOpacity;
  if (aClip) {
    IntersectClip(aBuilder, *aClip);
  }
  return true;
}

bool
nsDisplayOpacity::ShouldFlattenAway(nsDisplayListBuilder* aBuilder)
{
  if (NeedsActiveLayer())
    return false;

  nsDisplayItem* child = mList.GetBottom();
  
  
  
  if (!child || child->GetAbove()) {
    return false;
  }

  return child->ApplyOpacity(aBuilder, mOpacity, mClip);
}

nsDisplayItem::LayerState
nsDisplayOpacity::GetLayerState(nsDisplayListBuilder* aBuilder,
                                LayerManager* aManager,
                                const ContainerLayerParameters& aParameters) {
  if (NeedsActiveLayer())
    return LAYER_ACTIVE;

  return RequiredLayerStateForChildren(aBuilder, aManager, aParameters, mList,
    nsLayoutUtils::GetAnimatedGeometryRootFor(this, aBuilder, aManager));
}

bool
nsDisplayOpacity::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion) {
  
  
  
  
  
  nsRect bounds = GetClippedBounds(aBuilder);
  nsRegion visibleUnderChildren;
  visibleUnderChildren.And(*aVisibleRegion, bounds);
  return
    nsDisplayWrapList::ComputeVisibility(aBuilder, &visibleUnderChildren);
}

bool nsDisplayOpacity::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_OPACITY)
    return false;
  
  
  
  if (aItem->Frame()->GetContent() != mFrame->GetContent())
    return false;
  if (aItem->GetClip() != GetClip())
    return false;
  MergeFromTrackingMergedFrames(static_cast<nsDisplayOpacity*>(aItem));
  return true;
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayOpacity::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (opacity %f)", mOpacity);
}
#endif

nsDisplayMixBlendMode::nsDisplayMixBlendMode(nsDisplayListBuilder* aBuilder,
                                             nsIFrame* aFrame, nsDisplayList* aList,
                                             uint32_t aFlags)
: nsDisplayWrapList(aBuilder, aFrame, aList) {
  MOZ_COUNT_CTOR(nsDisplayMixBlendMode);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayMixBlendMode::~nsDisplayMixBlendMode() {
  MOZ_COUNT_DTOR(nsDisplayMixBlendMode);
}
#endif

nsRegion nsDisplayMixBlendMode::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                                bool* aSnap) {
  *aSnap = false;
  
  return nsRegion();
}

LayerState
nsDisplayMixBlendMode::GetLayerState(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     const ContainerLayerParameters& aParameters)
{
  gfxContext::GraphicsOperator op = nsCSSRendering::GetGFXBlendMode(mFrame->StyleDisplay()->mMixBlendMode);
  if (aManager->SupportsMixBlendMode(gfx::CompositionOpForOp(op))) {
    return LAYER_ACTIVE;
  }
  return LAYER_INACTIVE;
}


already_AddRefed<Layer>
nsDisplayMixBlendMode::BuildLayer(nsDisplayListBuilder* aBuilder,
                                  LayerManager* aManager,
                                  const ContainerLayerParameters& aContainerParameters) {
  ContainerLayerParameters newContainerParameters = aContainerParameters;
  newContainerParameters.mDisableSubpixelAntialiasingInDescendants = true;

  nsRefPtr<Layer> container = aManager->GetLayerBuilder()->
  BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                         newContainerParameters, nullptr);
  if (!container) {
    return nullptr;
  }

  container->DeprecatedSetMixBlendMode(nsCSSRendering::GetGFXBlendMode(mFrame->StyleDisplay()->mMixBlendMode));

  return container.forget();
}

bool nsDisplayMixBlendMode::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                              nsRegion* aVisibleRegion) {
  
  
  
  
  
  nsRect bounds = GetClippedBounds(aBuilder);
  nsRegion visibleUnderChildren;
  visibleUnderChildren.And(*aVisibleRegion, bounds);
  return nsDisplayWrapList::ComputeVisibility(aBuilder, &visibleUnderChildren);
}

bool nsDisplayMixBlendMode::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_MIX_BLEND_MODE)
    return false;
  
  
  
  if (aItem->Frame()->GetContent() != mFrame->GetContent())
    return false;
  if (aItem->GetClip() != GetClip())
    return false;
  MergeFromTrackingMergedFrames(static_cast<nsDisplayMixBlendMode*>(aItem));
  return true;
}

nsDisplayBlendContainer::nsDisplayBlendContainer(nsDisplayListBuilder* aBuilder,
                                                 nsIFrame* aFrame, nsDisplayList* aList,
                                                 BlendModeSet& aContainedBlendModes)
    : nsDisplayWrapList(aBuilder, aFrame, aList)
    , mContainedBlendModes(aContainedBlendModes)
    , mCanBeActive(true)
{
  MOZ_COUNT_CTOR(nsDisplayBlendContainer);
}

nsDisplayBlendContainer::nsDisplayBlendContainer(nsDisplayListBuilder* aBuilder,
                                                 nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList)
    , mCanBeActive(false)
{
  MOZ_COUNT_CTOR(nsDisplayBlendContainer);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayBlendContainer::~nsDisplayBlendContainer() {
  MOZ_COUNT_DTOR(nsDisplayBlendContainer);
}
#endif


already_AddRefed<Layer>
nsDisplayBlendContainer::BuildLayer(nsDisplayListBuilder* aBuilder,
                                    LayerManager* aManager,
                                    const ContainerLayerParameters& aContainerParameters) {
  
  
  ContainerLayerParameters newContainerParameters = aContainerParameters;
  newContainerParameters.mDisableSubpixelAntialiasingInDescendants = true;

  nsRefPtr<Layer> container = aManager->GetLayerBuilder()->
  BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                         newContainerParameters, nullptr);
  if (!container) {
    return nullptr;
  }
  
  container->SetForceIsolatedGroup(true);
  return container.forget();
}

bool nsDisplayBlendContainer::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_BLEND_CONTAINER)
    return false;
  
  
  
  if (aItem->Frame()->GetContent() != mFrame->GetContent())
    return false;
  if (aItem->GetClip() != GetClip())
    return false;
  MergeFromTrackingMergedFrames(static_cast<nsDisplayBlendContainer*>(aItem));
  return true;
}

nsDisplayOwnLayer::nsDisplayOwnLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayList* aList,
                                     uint32_t aFlags, ViewID aScrollTarget)
    : nsDisplayWrapList(aBuilder, aFrame, aList)
    , mFlags(aFlags)
    , mScrollTarget(aScrollTarget) {
  MOZ_COUNT_CTOR(nsDisplayOwnLayer);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayOwnLayer::~nsDisplayOwnLayer() {
  MOZ_COUNT_DTOR(nsDisplayOwnLayer);
}
#endif


already_AddRefed<Layer>
nsDisplayOwnLayer::BuildLayer(nsDisplayListBuilder* aBuilder,
                              LayerManager* aManager,
                              const ContainerLayerParameters& aContainerParameters) {
  nsRefPtr<ContainerLayer> layer = aManager->GetLayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                           aContainerParameters, nullptr);
  if (mFlags & VERTICAL_SCROLLBAR) {
    layer->SetScrollbarData(mScrollTarget, Layer::ScrollDirection::VERTICAL);
  }
  if (mFlags & HORIZONTAL_SCROLLBAR) {
    layer->SetScrollbarData(mScrollTarget, Layer::ScrollDirection::HORIZONTAL);
  }

  if (mFlags & GENERATE_SUBDOC_INVALIDATIONS) {
    mFrame->PresContext()->SetNotifySubDocInvalidationData(layer);
  }
  return layer.forget();
}

nsDisplaySubDocument::nsDisplaySubDocument(nsDisplayListBuilder* aBuilder,
                                           nsIFrame* aFrame, nsDisplayList* aList,
                                           uint32_t aFlags)
    : nsDisplayOwnLayer(aBuilder, aFrame, aList, aFlags)
    , mScrollParentId(aBuilder->GetCurrentScrollParentId())
{
  MOZ_COUNT_CTOR(nsDisplaySubDocument);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplaySubDocument::~nsDisplaySubDocument() {
  MOZ_COUNT_DTOR(nsDisplaySubDocument);
}
#endif

already_AddRefed<Layer>
nsDisplaySubDocument::BuildLayer(nsDisplayListBuilder* aBuilder,
                                 LayerManager* aManager,
                                 const ContainerLayerParameters& aContainerParameters) {
  nsPresContext* presContext = mFrame->PresContext();
  nsIFrame* rootScrollFrame = presContext->PresShell()->GetRootScrollFrame();
  ContainerLayerParameters params = aContainerParameters;
  if ((mFlags & GENERATE_SCROLLABLE_LAYER) &&
      rootScrollFrame->GetContent() &&
      nsLayoutUtils::GetCriticalDisplayPort(rootScrollFrame->GetContent(), nullptr)) {
    params.mInLowPrecisionDisplayPort = true; 
  }

  return nsDisplayOwnLayer::BuildLayer(aBuilder, aManager, params);
}

UniquePtr<FrameMetrics>
nsDisplaySubDocument::ComputeFrameMetrics(Layer* aLayer,
                                          const ContainerLayerParameters& aContainerParameters)
{
  if (!(mFlags & GENERATE_SCROLLABLE_LAYER)) {
    return UniquePtr<FrameMetrics>(nullptr);
  }

  nsPresContext* presContext = mFrame->PresContext();
  nsIFrame* rootScrollFrame = presContext->PresShell()->GetRootScrollFrame();
  bool isRootContentDocument = presContext->IsRootContentDocument();
  ContainerLayerParameters params = aContainerParameters;
  if ((mFlags & GENERATE_SCROLLABLE_LAYER) &&
      rootScrollFrame->GetContent() &&
      nsLayoutUtils::GetCriticalDisplayPort(rootScrollFrame->GetContent(), nullptr)) {
    params.mInLowPrecisionDisplayPort = true;
  }

  nsRect viewport = mFrame->GetRect() -
                    mFrame->GetPosition() +
                    mFrame->GetOffsetToCrossDoc(ReferenceFrame());

  return MakeUnique<FrameMetrics>(
    nsDisplayScrollLayer::ComputeFrameMetrics(mFrame, rootScrollFrame, ReferenceFrame(),
                       aLayer, mScrollParentId, viewport,
                       false, isRootContentDocument, params));
}

nsRect
nsDisplaySubDocument::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  bool usingDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(mFrame->PresContext());

  if ((mFlags & GENERATE_SCROLLABLE_LAYER) && usingDisplayPort) {
    *aSnap = false;
    return mFrame->GetRect() + aBuilder->ToReferenceFrame(mFrame);
  }

  return nsDisplayOwnLayer::GetBounds(aBuilder, aSnap);
}

bool
nsDisplaySubDocument::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                        nsRegion* aVisibleRegion)
{
  nsRect displayport;
  bool usingDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(mFrame->PresContext(), &displayport);

  if (!(mFlags & GENERATE_SCROLLABLE_LAYER) || !usingDisplayPort) {
    return nsDisplayWrapList::ComputeVisibility(aBuilder, aVisibleRegion);
  }

  nsRegion childVisibleRegion;
  
  
  
  childVisibleRegion = displayport + mFrame->GetOffsetToCrossDoc(ReferenceFrame());

  nsRect boundedRect =
    childVisibleRegion.GetBounds().Intersect(mList.GetBounds(aBuilder));
  bool visible = mList.ComputeVisibilityForSublist(
    aBuilder, &childVisibleRegion, boundedRect,
    usingDisplayPort ? mFrame : nullptr);

  
  
  
  if (!nsLayoutUtils::UsesAsyncScrolling()) {
    bool snap;
    nsRect bounds = GetBounds(aBuilder, &snap);
    nsRegion removed;
    removed.Sub(bounds, childVisibleRegion);

    aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);
  }

  return visible;
}

bool
nsDisplaySubDocument::ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder)
{
  bool usingDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(mFrame->PresContext());

  if ((mFlags & GENERATE_SCROLLABLE_LAYER) && usingDisplayPort) {
    return true;
  }

  return nsDisplayOwnLayer::ShouldBuildLayerEvenIfInvisible(aBuilder);
}

nsRegion
nsDisplaySubDocument::GetOpaqueRegion(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  bool usingDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(mFrame->PresContext());

  if ((mFlags & GENERATE_SCROLLABLE_LAYER) && usingDisplayPort) {
    *aSnap = false;
    return nsRegion();
  }

  return nsDisplayOwnLayer::GetOpaqueRegion(aBuilder, aSnap);
}

nsDisplayResolution::nsDisplayResolution(nsDisplayListBuilder* aBuilder,
                                         nsIFrame* aFrame, nsDisplayList* aList,
                                         uint32_t aFlags)
    : nsDisplaySubDocument(aBuilder, aFrame, aList, aFlags) {
  MOZ_COUNT_CTOR(nsDisplayResolution);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayResolution::~nsDisplayResolution() {
  MOZ_COUNT_DTOR(nsDisplayResolution);
}
#endif

already_AddRefed<Layer>
nsDisplayResolution::BuildLayer(nsDisplayListBuilder* aBuilder,
                                LayerManager* aManager,
                                const ContainerLayerParameters& aContainerParameters) {
  nsIPresShell* presShell = mFrame->PresContext()->PresShell();
  ContainerLayerParameters containerParameters(
    presShell->GetXResolution(), presShell->GetYResolution(), nsIntPoint(),
    aContainerParameters);

  nsRefPtr<Layer> layer = nsDisplaySubDocument::BuildLayer(
    aBuilder, aManager, containerParameters);
  layer->SetPostScale(1.0f / presShell->GetXResolution(),
                      1.0f / presShell->GetYResolution());
  return layer.forget();
}

nsDisplayStickyPosition::nsDisplayStickyPosition(nsDisplayListBuilder* aBuilder,
                                                 nsIFrame* aFrame,
                                                 nsDisplayList* aList)
  : nsDisplayOwnLayer(aBuilder, aFrame, aList)
{
  MOZ_COUNT_CTOR(nsDisplayStickyPosition);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayStickyPosition::~nsDisplayStickyPosition() {
  MOZ_COUNT_DTOR(nsDisplayStickyPosition);
}
#endif

already_AddRefed<Layer>
nsDisplayStickyPosition::BuildLayer(nsDisplayListBuilder* aBuilder,
                                    LayerManager* aManager,
                                    const ContainerLayerParameters& aContainerParameters) {
  nsRefPtr<Layer> layer =
    nsDisplayOwnLayer::BuildLayer(aBuilder, aManager, aContainerParameters);

  StickyScrollContainer* stickyScrollContainer = StickyScrollContainer::
    GetStickyScrollContainerForFrame(mFrame);
  if (!stickyScrollContainer) {
    return layer.forget();
  }

  nsIFrame* scrollFrame = do_QueryFrame(stickyScrollContainer->ScrollFrame());
  nsPresContext* presContext = scrollFrame->PresContext();

  
  
  nsSize scrollFrameSize = scrollFrame->GetSize();
  if (scrollFrame == presContext->PresShell()->GetRootScrollFrame() &&
      presContext->PresShell()->IsScrollPositionClampingScrollPortSizeSet()) {
    scrollFrameSize = presContext->PresShell()->
      GetScrollPositionClampingScrollPortSize();
  }

  nsLayoutUtils::SetFixedPositionLayerData(layer, scrollFrame,
    nsRect(scrollFrame->GetOffsetToCrossDoc(ReferenceFrame()), scrollFrameSize),
    mFrame, presContext, aContainerParameters);

  ViewID scrollId = nsLayoutUtils::FindOrCreateIDFor(
    stickyScrollContainer->ScrollFrame()->GetScrolledFrame()->GetContent());

  float factor = presContext->AppUnitsPerDevPixel();
  nsRect outer;
  nsRect inner;
  stickyScrollContainer->GetScrollRanges(mFrame, &outer, &inner);
  LayerRect stickyOuter(NSAppUnitsToFloatPixels(outer.x, factor) *
                          aContainerParameters.mXScale,
                        NSAppUnitsToFloatPixels(outer.y, factor) *
                          aContainerParameters.mYScale,
                        NSAppUnitsToFloatPixels(outer.width, factor) *
                          aContainerParameters.mXScale,
                        NSAppUnitsToFloatPixels(outer.height, factor) *
                          aContainerParameters.mYScale);
  LayerRect stickyInner(NSAppUnitsToFloatPixels(inner.x, factor) *
                          aContainerParameters.mXScale,
                        NSAppUnitsToFloatPixels(inner.y, factor) *
                          aContainerParameters.mYScale,
                        NSAppUnitsToFloatPixels(inner.width, factor) *
                          aContainerParameters.mXScale,
                        NSAppUnitsToFloatPixels(inner.height, factor) *
                          aContainerParameters.mYScale);
  layer->SetStickyPositionData(scrollId, stickyOuter, stickyInner);

  return layer.forget();
}

bool nsDisplayStickyPosition::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_STICKY_POSITION)
    return false;
  
  nsDisplayStickyPosition* other = static_cast<nsDisplayStickyPosition*>(aItem);
  if (other->mFrame != mFrame)
    return false;
  if (aItem->GetClip() != GetClip())
    return false;
  MergeFromTrackingMergedFrames(other);
  return true;
}

nsDisplayScrollLayer::nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder,
                                           nsDisplayList* aList,
                                           nsIFrame* aForFrame,
                                           nsIFrame* aScrolledFrame,
                                           nsIFrame* aScrollFrame)
  : nsDisplayWrapList(aBuilder, aForFrame, aList)
  , mScrollFrame(aScrollFrame)
  , mScrolledFrame(aScrolledFrame)
  , mScrollParentId(aBuilder->GetCurrentScrollParentId())
  , mDisplayPortContentsOpaque(false)
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_CTOR(nsDisplayScrollLayer);
#endif

  NS_ASSERTION(mScrolledFrame && mScrolledFrame->GetContent(),
               "Need a child frame with content");
}

nsDisplayScrollLayer::nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem,
                                           nsIFrame* aForFrame,
                                           nsIFrame* aScrolledFrame,
                                           nsIFrame* aScrollFrame)
  : nsDisplayWrapList(aBuilder, aForFrame, aItem)
  , mScrollFrame(aScrollFrame)
  , mScrolledFrame(aScrolledFrame)
  , mScrollParentId(aBuilder->GetCurrentScrollParentId())
  , mDisplayPortContentsOpaque(false)
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_CTOR(nsDisplayScrollLayer);
#endif

  NS_ASSERTION(mScrolledFrame && mScrolledFrame->GetContent(),
               "Need a child frame with content");
}

nsDisplayScrollLayer::nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder,
                                           nsIFrame* aForFrame,
                                           nsIFrame* aScrolledFrame,
                                           nsIFrame* aScrollFrame)
  : nsDisplayWrapList(aBuilder, aForFrame)
  , mScrollFrame(aScrollFrame)
  , mScrolledFrame(aScrolledFrame)
  , mScrollParentId(aBuilder->GetCurrentScrollParentId())
  , mDisplayPortContentsOpaque(false)
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_CTOR(nsDisplayScrollLayer);
#endif

  NS_ASSERTION(mScrolledFrame && mScrolledFrame->GetContent(),
               "Need a child frame with content");
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayScrollLayer::~nsDisplayScrollLayer()
{
  MOZ_COUNT_DTOR(nsDisplayScrollLayer);
}
#endif

nsRect
nsDisplayScrollLayer::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  nsIScrollableFrame* sf = do_QueryFrame(mScrollFrame);
  if (sf) {
    *aSnap = false;
    return sf->GetScrollPortRect() + aBuilder->ToReferenceFrame(mScrollFrame);
  }
  return nsDisplayWrapList::GetBounds(aBuilder, aSnap);
}

nsRect
nsDisplayScrollLayer::GetScrolledContentRectToDraw(nsDisplayListBuilder* aBuilder,
                                                   nsRect* aDisplayPort)
{
  if (aDisplayPort) {
    
    
    
    
    
    return *aDisplayPort + mScrollFrame->GetOffsetToCrossDoc(ReferenceFrame());
  }
  bool snap;
  return GetBounds(aBuilder, &snap);
}

already_AddRefed<Layer>
nsDisplayScrollLayer::BuildLayer(nsDisplayListBuilder* aBuilder,
                                 LayerManager* aManager,
                                 const ContainerLayerParameters& aContainerParameters)
{
  ContainerLayerParameters params = aContainerParameters;
  if (mScrolledFrame->GetContent() &&
      nsLayoutUtils::GetCriticalDisplayPort(mScrolledFrame->GetContent(), nullptr)) {
    params.mInLowPrecisionDisplayPort = true;
  }

  if (mList.IsOpaque()) {
    nsRect displayport;
    bool usingDisplayport =
      nsLayoutUtils::GetDisplayPort(mScrolledFrame->GetContent(), &displayport);
    mDisplayPortContentsOpaque = mList.GetBounds(aBuilder).Contains(
        GetScrolledContentRectToDraw(aBuilder, usingDisplayport ? &displayport : nullptr));
  } else {
    mDisplayPortContentsOpaque = false;
  }

  return aManager->GetLayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                           params, nullptr);
}

UniquePtr<FrameMetrics>
nsDisplayScrollLayer::ComputeFrameMetrics(Layer* aLayer,
                                          const ContainerLayerParameters& aContainerParameters)
{
  ContainerLayerParameters params = aContainerParameters;
  if (mScrolledFrame->GetContent() &&
      nsLayoutUtils::GetCriticalDisplayPort(mScrolledFrame->GetContent(), nullptr)) {
    params.mInLowPrecisionDisplayPort = true; 
  }

  nsRect viewport = mScrollFrame->GetRect() -
                    mScrollFrame->GetPosition() +
                    mScrollFrame->GetOffsetToCrossDoc(ReferenceFrame());

  return UniquePtr<FrameMetrics>(new FrameMetrics(
    ComputeFrameMetrics(mScrolledFrame, mScrollFrame, ReferenceFrame(), aLayer,
                        mScrollParentId, viewport, false, false, params)));
}

bool
nsDisplayScrollLayer::ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder)
{
  if (nsLayoutUtils::GetDisplayPort(mScrolledFrame->GetContent(), nullptr)) {
    return true;
  }

  return nsDisplayWrapList::ShouldBuildLayerEvenIfInvisible(aBuilder);
}

bool
nsDisplayScrollLayer::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                        nsRegion* aVisibleRegion)
{
  if (aBuilder->IsForPluginGeometry()) {
    return nsDisplayWrapList::ComputeVisibility(aBuilder, aVisibleRegion);
  }
  nsRect displayport;
  bool usingDisplayPort =
    nsLayoutUtils::GetDisplayPort(mScrolledFrame->GetContent(), &displayport);
  nsRect scrolledContentRect = GetScrolledContentRectToDraw(aBuilder,
      usingDisplayPort ? &displayport : nullptr);

  nsRect boundedRect = scrolledContentRect.Intersect(mList.GetBounds(aBuilder));
  nsRegion childVisibleRegion = scrolledContentRect;
  bool visible = mList.ComputeVisibilityForSublist(
    aBuilder, &childVisibleRegion, boundedRect,
    usingDisplayPort ? mScrollFrame : nullptr);

  
  
  
  if (!nsLayoutUtils::UsesAsyncScrolling()) {
    bool snap;
    nsRect bounds = GetBounds(aBuilder, &snap);
    nsRegion removed;
    removed.Sub(bounds, childVisibleRegion);
    aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);
  }

  return visible;
}

LayerState
nsDisplayScrollLayer::GetLayerState(nsDisplayListBuilder* aBuilder,
                                    LayerManager* aManager,
                                    const ContainerLayerParameters& aParameters)
{
  
  
  return LAYER_ACTIVE_FORCE;
}




bool
WouldCauseIncorrectClippingOnAbsPosItem(nsDisplayListBuilder* aBuilder,
                                        nsDisplayScrollLayer* aItem)
{
  nsIFrame* scrollFrame = aItem->GetScrollFrame();
  nsIPresShell* presShell = scrollFrame->PresContext()->PresShell();
  if (scrollFrame == presShell->GetRootScrollFrame()) {
    return false;
  }
  nsIFrame* scrolledFrame = aItem->GetScrolledFrame();
  nsIFrame* frame = aItem->Frame();
  if (frame == scrolledFrame || !frame->IsAbsolutelyPositioned() ||
      nsLayoutUtils::IsAncestorFrameCrossDoc(scrollFrame, frame, presShell->GetRootFrame())) {
    return false;
  }
  if (!aItem->GetClip().IsRectAffectedByClip(aItem->GetChildren()->GetBounds(aBuilder))) {
    return false;
  }
  return true;
}

bool
nsDisplayScrollLayer::TryMerge(nsDisplayListBuilder* aBuilder,
                               nsDisplayItem* aItem)
{
  if (aItem->GetType() != TYPE_SCROLL_LAYER) {
    return false;
  }
  nsDisplayScrollLayer* other = static_cast<nsDisplayScrollLayer*>(aItem);
  if (other->mScrolledFrame != this->mScrolledFrame) {
    return false;
  }
  if (aItem->GetClip() != GetClip()) {
    return false;
  }

  if (WouldCauseIncorrectClippingOnAbsPosItem(aBuilder, this) ||
      WouldCauseIncorrectClippingOnAbsPosItem(aBuilder, other)) {
    return false;
  }

  NS_ASSERTION(other->mReferenceFrame == mReferenceFrame,
               "Must have the same reference frame!");

  FrameProperties props = mScrolledFrame->Properties();
  props.Set(nsIFrame::ScrollLayerCount(),
    reinterpret_cast<void*>(GetScrollLayerCount() - 1));

  
  
  
  
  
  nsIFrame* tmp = mFrame;
  mFrame = other->mFrame;
  other->mFrame = tmp;
  MergeFromTrackingMergedFrames(other);
  return true;
}

void
PropagateClip(nsDisplayListBuilder* aBuilder, const DisplayItemClip& aClip,
              nsDisplayList* aList)
{
  for (nsDisplayItem* i = aList->GetBottom(); i != nullptr; i = i->GetAbove()) {
    DisplayItemClip clip(i->GetClip());
    clip.IntersectWith(aClip);
    i->SetClip(aBuilder, clip);
    nsDisplayList* list = i->GetSameCoordinateSystemChildren();
    if (list) {
      PropagateClip(aBuilder, aClip, list);
    }
  }
}

bool
nsDisplayScrollLayer::ShouldFlattenAway(nsDisplayListBuilder* aBuilder)
{
  bool badAbsPosClip = WouldCauseIncorrectClippingOnAbsPosItem(aBuilder, this);
  if (GetScrollLayerCount() > 1 || badAbsPosClip) {
    
    
    
    
    
    if (!badAbsPosClip) {
      PropagateClip(aBuilder, GetClip(), &mList);
    }

    
    nsresult status;
    mScrolledFrame->GetContent()->GetProperty(nsGkAtoms::AsyncScrollLayerCreationFailed, &status);
    if (status == NS_PROPTABLE_PROP_NOT_THERE) {
      mScrolledFrame->GetContent()->SetProperty(nsGkAtoms::AsyncScrollLayerCreationFailed, nullptr);
      if (badAbsPosClip) {
        printf_stderr("Async scrollable layer creation failed: scroll layer would induce incorrent clipping to an abs pos item.\n");
      } else {
        printf_stderr("Async scrollable layer creation failed: scroll layer can't have scrollable and non-scrollable items interleaved.\n");
      }
#ifdef MOZ_DUMP_PAINTING
      std::stringstream ss;
      nsFrame::PrintDisplayItem(aBuilder, this, ss, true, false);
      printf_stderr("%s\n", ss.str().c_str());
#endif
    }

    return true;
  }
  if (mFrame != mScrolledFrame) {
    mMergedFrames.AppendElement(mFrame);
    mFrame = mScrolledFrame;
  }
  return false;
}

intptr_t
nsDisplayScrollLayer::GetScrollLayerCount()
{
  FrameProperties props = mScrolledFrame->Properties();
#ifdef DEBUG
  bool hasCount = false;
  intptr_t result = reinterpret_cast<intptr_t>(
    props.Get(nsIFrame::ScrollLayerCount(), &hasCount));
  
  
  
  
  NS_ABORT_IF_FALSE(hasCount, "nsDisplayScrollLayer should always be defined");
  return result;
#else
  return reinterpret_cast<intptr_t>(props.Get(nsIFrame::ScrollLayerCount()));
#endif
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayScrollLayer::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (scrollframe %p scrolledframe %p)",
                         mScrollFrame, mScrolledFrame);
}
#endif

nsDisplayScrollInfoLayer::nsDisplayScrollInfoLayer(
  nsDisplayListBuilder* aBuilder,
  nsIFrame* aScrolledFrame,
  nsIFrame* aScrollFrame)
  : nsDisplayScrollLayer(aBuilder, aScrollFrame, aScrolledFrame, aScrollFrame)
{
#ifdef NS_BUILD_REFCNT_LOGGING
  MOZ_COUNT_CTOR(nsDisplayScrollInfoLayer);
#endif
}

nsDisplayScrollInfoLayer::~nsDisplayScrollInfoLayer()
{
  MOZ_COUNT_DTOR(nsDisplayScrollInfoLayer);
}

nsRect
nsDisplayScrollInfoLayer::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  return nsDisplayWrapList::GetBounds(aBuilder, aSnap);
}

LayerState
nsDisplayScrollInfoLayer::GetLayerState(nsDisplayListBuilder* aBuilder,
                                        LayerManager* aManager,
                                        const ContainerLayerParameters& aParameters)
{
  return LAYER_ACTIVE_EMPTY;
}

bool
nsDisplayScrollInfoLayer::TryMerge(nsDisplayListBuilder* aBuilder,
                                   nsDisplayItem* aItem)
{
  return false;
}

bool
nsDisplayScrollInfoLayer::ShouldFlattenAway(nsDisplayListBuilder* aBuilder)
{
  
  
  
  
  return GetScrollLayerCount() == 1;
}

nsDisplayZoom::nsDisplayZoom(nsDisplayListBuilder* aBuilder,
                             nsIFrame* aFrame, nsDisplayList* aList,
                             int32_t aAPD, int32_t aParentAPD,
                             uint32_t aFlags)
    : nsDisplaySubDocument(aBuilder, aFrame, aList, aFlags)
    , mAPD(aAPD), mParentAPD(aParentAPD) {
  MOZ_COUNT_CTOR(nsDisplayZoom);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayZoom::~nsDisplayZoom() {
  MOZ_COUNT_DTOR(nsDisplayZoom);
}
#endif

nsRect nsDisplayZoom::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  nsRect bounds = nsDisplaySubDocument::GetBounds(aBuilder, aSnap);
  *aSnap = false;
  return bounds.ConvertAppUnitsRoundOut(mAPD, mParentAPD);
}

void nsDisplayZoom::HitTest(nsDisplayListBuilder *aBuilder,
                            const nsRect& aRect,
                            HitTestState *aState,
                            nsTArray<nsIFrame*> *aOutFrames)
{
  nsRect rect;
  
  
  if (aRect.width == 1 && aRect.height == 1) {
    rect.MoveTo(aRect.TopLeft().ConvertAppUnits(mParentAPD, mAPD));
    rect.width = rect.height = 1;
  } else {
    rect = aRect.ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  }
  mList.HitTest(aBuilder, rect, aState, aOutFrames);
}

void nsDisplayZoom::Paint(nsDisplayListBuilder* aBuilder,
                          nsRenderingContext* aCtx)
{
  mList.PaintForFrame(aBuilder, aCtx, mFrame, nsDisplayList::PAINT_DEFAULT);
}

bool nsDisplayZoom::ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                      nsRegion *aVisibleRegion)
{
  
  nsRegion visibleRegion;
  
  visibleRegion.And(*aVisibleRegion, mVisibleRect);
  visibleRegion = visibleRegion.ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  nsRegion originalVisibleRegion = visibleRegion;

  nsRect transformedVisibleRect =
    mVisibleRect.ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  bool retval;
  
  
  
  bool usingDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(mFrame->PresContext());
  if (!(mFlags & GENERATE_SCROLLABLE_LAYER) || !usingDisplayPort) {
    retval =
      mList.ComputeVisibilityForSublist(aBuilder, &visibleRegion,
                                        transformedVisibleRect);
  } else {
    retval =
      nsDisplaySubDocument::ComputeVisibility(aBuilder, &visibleRegion);
  }

  nsRegion removed;
  
  removed.Sub(originalVisibleRegion, visibleRegion);
  
  removed = removed.ConvertAppUnitsRoundIn(mAPD, mParentAPD);
  
  
  aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);

  return retval;
}













#undef  UNIFIED_CONTINUATIONS
#undef  DEBUG_HIT










#ifndef UNIFIED_CONTINUATIONS

nsRect
nsDisplayTransform::GetFrameBoundsForTransform(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Can't get the bounds of a nonexistent frame!");

  if (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    return nsRect();
  }

  return nsRect(nsPoint(0, 0), aFrame->GetSize());
}

#else

nsRect
nsDisplayTransform::GetFrameBoundsForTransform(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Can't get the bounds of a nonexistent frame!");

  nsRect result;

  if (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    return result;
  }

  


  for (const nsIFrame *currFrame = aFrame->FirstContinuation();
       currFrame != nullptr;
       currFrame = currFrame->GetNextContinuation())
    {
      


      result.UnionRect(result, nsRect(currFrame->GetOffsetTo(aFrame),
                                      currFrame->GetSize()));
    }

  return result;
}

#endif

nsDisplayTransform::nsDisplayTransform(nsDisplayListBuilder* aBuilder,
                                       nsIFrame *aFrame, nsDisplayList *aList,
                                       const nsRect& aChildrenVisibleRect,
                                       ComputeTransformFunction aTransformGetter,
                                       uint32_t aIndex) 
  : nsDisplayItem(aBuilder, aFrame)
  , mStoredList(aBuilder, aFrame, aList)
  , mTransformGetter(aTransformGetter)
  , mChildrenVisibleRect(aChildrenVisibleRect)
  , mIndex(aIndex)
{
  MOZ_COUNT_CTOR(nsDisplayTransform);
  NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  NS_ABORT_IF_FALSE(!aFrame->IsTransformed(), "Can't specify a transform getter for a transformed frame!");
  Init(aBuilder);
}

void
nsDisplayTransform::SetReferenceFrameToAncestor(nsDisplayListBuilder* aBuilder)
{
  mReferenceFrame =
    aBuilder->FindReferenceFrameFor(GetTransformRootFrame(mFrame));
  mToReferenceFrame = mFrame->GetOffsetToCrossDoc(mReferenceFrame);
  mVisibleRect = aBuilder->GetDirtyRect() + mToReferenceFrame;
}

void
nsDisplayTransform::Init(nsDisplayListBuilder* aBuilder)
{
  mStoredList.SetClip(aBuilder, DisplayItemClip::NoClip());
  mStoredList.SetVisibleRect(mChildrenVisibleRect);
  mPrerender = ShouldPrerenderTransformedContent(aBuilder, mFrame);
  if (mPrerender) {
    bool snap;
    mVisibleRect = GetBounds(aBuilder, &snap);
  }
}

nsDisplayTransform::nsDisplayTransform(nsDisplayListBuilder* aBuilder,
                                       nsIFrame *aFrame, nsDisplayList *aList,
                                       const nsRect& aChildrenVisibleRect,
                                       uint32_t aIndex)
  : nsDisplayItem(aBuilder, aFrame)
  , mStoredList(aBuilder, aFrame, aList)
  , mTransformGetter(nullptr)
  , mChildrenVisibleRect(aChildrenVisibleRect)
  , mIndex(aIndex)
{
  MOZ_COUNT_CTOR(nsDisplayTransform);
  NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  SetReferenceFrameToAncestor(aBuilder);
  Init(aBuilder);
}

nsDisplayTransform::nsDisplayTransform(nsDisplayListBuilder* aBuilder,
                                       nsIFrame *aFrame, nsDisplayItem *aItem,
                                       const nsRect& aChildrenVisibleRect,
                                       uint32_t aIndex)
  : nsDisplayItem(aBuilder, aFrame)
  , mStoredList(aBuilder, aFrame, aItem)
  , mTransformGetter(nullptr)
  , mChildrenVisibleRect(aChildrenVisibleRect)
  , mIndex(aIndex)
{
  MOZ_COUNT_CTOR(nsDisplayTransform);
  NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  SetReferenceFrameToAncestor(aBuilder);
  Init(aBuilder);
}






 Point3D
nsDisplayTransform::GetDeltaToTransformOrigin(const nsIFrame* aFrame,
                                              float aAppUnitsPerPixel,
                                              const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Can't get delta for a null frame!");
  NS_PRECONDITION(aFrame->IsTransformed() || aFrame->StyleDisplay()->BackfaceIsHidden(),
                  "Shouldn't get a delta for an untransformed frame!");

  if (!aFrame->IsTransformed()) {
    return Point3D();
  }

  



  const nsStyleDisplay* display = aFrame->StyleDisplay();
  nsRect boundingRect = (aBoundsOverride ? *aBoundsOverride :
                         nsDisplayTransform::GetFrameBoundsForTransform(aFrame));

  
  float coords[3];
  const nscoord* dimensions[2] =
    {&boundingRect.width, &boundingRect.height};

  for (uint8_t index = 0; index < 2; ++index) {
    


    const nsStyleCoord &coord = display->mTransformOrigin[index];
    if (coord.GetUnit() == eStyleUnit_Calc) {
      const nsStyleCoord::Calc *calc = coord.GetCalcValue();
      coords[index] =
        NSAppUnitsToFloatPixels(*dimensions[index], aAppUnitsPerPixel) *
          calc->mPercent +
        NSAppUnitsToFloatPixels(calc->mLength, aAppUnitsPerPixel);
    } else if (coord.GetUnit() == eStyleUnit_Percent) {
      coords[index] =
        NSAppUnitsToFloatPixels(*dimensions[index], aAppUnitsPerPixel) *
        coord.GetPercentValue();
    } else {
      NS_ABORT_IF_FALSE(coord.GetUnit() == eStyleUnit_Coord, "unexpected unit");
      coords[index] =
        NSAppUnitsToFloatPixels(coord.GetCoordValue(), aAppUnitsPerPixel);
    }
    if ((aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
        coord.GetUnit() != eStyleUnit_Percent) {
      
      
      nscoord offset =
        (index == 0) ? aFrame->GetPosition().x : aFrame->GetPosition().y;
      coords[index] -= NSAppUnitsToFloatPixels(offset, aAppUnitsPerPixel);
    }
  }

  coords[2] = NSAppUnitsToFloatPixels(display->mTransformOrigin[2].GetCoordValue(),
                                      aAppUnitsPerPixel);
  
  coords[0] += NSAppUnitsToFloatPixels(boundingRect.x, aAppUnitsPerPixel);
  coords[1] += NSAppUnitsToFloatPixels(boundingRect.y, aAppUnitsPerPixel);

  return Point3D(coords[0], coords[1], coords[2]);
}






 Point3D
nsDisplayTransform::GetDeltaToPerspectiveOrigin(const nsIFrame* aFrame,
                                                float aAppUnitsPerPixel)
{
  NS_PRECONDITION(aFrame, "Can't get delta for a null frame!");
  NS_PRECONDITION(aFrame->IsTransformed() || aFrame->StyleDisplay()->BackfaceIsHidden(),
                  "Shouldn't get a delta for an untransformed frame!");

  if (!aFrame->IsTransformed()) {
    return Point3D();
  }

  




  
  
  nsIFrame* parent = aFrame->GetParentStyleContextFrame();
  if (!parent) {
    return Point3D();
  }
  const nsStyleDisplay* display = parent->StyleDisplay();
  nsRect boundingRect = nsDisplayTransform::GetFrameBoundsForTransform(parent);

  
  Point3D result;
  result.z = 0.0f;
  gfx::Float* coords[2] = {&result.x, &result.y};
  const nscoord* dimensions[2] =
    {&boundingRect.width, &boundingRect.height};

  for (uint8_t index = 0; index < 2; ++index) {
    


    const nsStyleCoord &coord = display->mPerspectiveOrigin[index];
    if (coord.GetUnit() == eStyleUnit_Calc) {
      const nsStyleCoord::Calc *calc = coord.GetCalcValue();
      *coords[index] =
        NSAppUnitsToFloatPixels(*dimensions[index], aAppUnitsPerPixel) *
          calc->mPercent +
        NSAppUnitsToFloatPixels(calc->mLength, aAppUnitsPerPixel);
    } else if (coord.GetUnit() == eStyleUnit_Percent) {
      *coords[index] =
        NSAppUnitsToFloatPixels(*dimensions[index], aAppUnitsPerPixel) *
        coord.GetPercentValue();
    } else {
      NS_ABORT_IF_FALSE(coord.GetUnit() == eStyleUnit_Coord, "unexpected unit");
      *coords[index] =
        NSAppUnitsToFloatPixels(coord.GetCoordValue(), aAppUnitsPerPixel);
    }
  }

  nsPoint parentOffset = aFrame->GetOffsetTo(parent);
  Point3D gfxOffset(
            NSAppUnitsToFloatPixels(parentOffset.x, aAppUnitsPerPixel),
            NSAppUnitsToFloatPixels(parentOffset.y, aAppUnitsPerPixel),
            0.0f);

  return result - gfxOffset;
}

nsDisplayTransform::FrameTransformProperties::FrameTransformProperties(const nsIFrame* aFrame,
                                                                       float aAppUnitsPerPixel,
                                                                       const nsRect* aBoundsOverride)
  : mFrame(aFrame)
  , mTransformList(aFrame->StyleDisplay()->mSpecifiedTransform)
  , mToTransformOrigin(GetDeltaToTransformOrigin(aFrame, aAppUnitsPerPixel, aBoundsOverride))
  , mToPerspectiveOrigin(GetDeltaToPerspectiveOrigin(aFrame, aAppUnitsPerPixel))
  , mChildPerspective(0)
{
  const nsStyleDisplay* parentDisp = nullptr;
  nsStyleContext* parentStyleContext = aFrame->StyleContext()->GetParent();
  if (parentStyleContext) {
    parentDisp = parentStyleContext->StyleDisplay();
  }
  if (parentDisp && parentDisp->mChildPerspective.GetUnit() == eStyleUnit_Coord) {
    mChildPerspective = parentDisp->mChildPerspective.GetCoordValue();
  }
}





gfx3DMatrix
nsDisplayTransform::GetResultingTransformMatrix(const FrameTransformProperties& aProperties,
                                                const nsPoint& aOrigin,
                                                float aAppUnitsPerPixel,
                                                const nsRect* aBoundsOverride,
                                                nsIFrame** aOutAncestor)
{
  return GetResultingTransformMatrixInternal(aProperties, aOrigin, aAppUnitsPerPixel,
                                             aBoundsOverride, aOutAncestor, false);
}
 
gfx3DMatrix
nsDisplayTransform::GetResultingTransformMatrix(const nsIFrame* aFrame,
                                                const nsPoint& aOrigin,
                                                float aAppUnitsPerPixel,
                                                const nsRect* aBoundsOverride,
                                                nsIFrame** aOutAncestor,
                                                bool aOffsetByOrigin)
{
  FrameTransformProperties props(aFrame,
                                 aAppUnitsPerPixel,
                                 aBoundsOverride);

  return GetResultingTransformMatrixInternal(props, aOrigin, aAppUnitsPerPixel,
                                             aBoundsOverride, aOutAncestor,
                                             aOffsetByOrigin);
}

gfx3DMatrix
nsDisplayTransform::GetResultingTransformMatrixInternal(const FrameTransformProperties& aProperties,
                                                        const nsPoint& aOrigin,
                                                        float aAppUnitsPerPixel,
                                                        const nsRect* aBoundsOverride,
                                                        nsIFrame** aOutAncestor,
                                                        bool aOffsetByOrigin)
{
  const nsIFrame *frame = aProperties.mFrame;

  if (aOutAncestor) {
    *aOutAncestor = nsLayoutUtils::GetCrossDocParentFrame(frame);
  }

  


  nsRect bounds = (aBoundsOverride ? *aBoundsOverride :
                   nsDisplayTransform::GetFrameBoundsForTransform(frame));

  
  bool dummy;
  gfx3DMatrix result;
  
  
  Matrix svgTransform, transformFromSVGParent;
  bool hasSVGTransforms =
    frame && frame->IsSVGTransformed(&svgTransform, &transformFromSVGParent);
  
  if (aProperties.mTransformList) {
    result = nsStyleTransformMatrix::ReadTransforms(aProperties.mTransformList->mHead,
                                                    frame ? frame->StyleContext() : nullptr,
                                                    frame ? frame->PresContext() : nullptr,
                                                    dummy, bounds, aAppUnitsPerPixel);
  } else if (hasSVGTransforms) {
    
    float pixelsPerCSSPx = frame->PresContext()->AppUnitsPerCSSPixel() /
                             aAppUnitsPerPixel;
    svgTransform._31 *= pixelsPerCSSPx;
    svgTransform._32 *= pixelsPerCSSPx;
    result = gfx3DMatrix::From2D(ThebesMatrix(svgTransform));
  }

  if (hasSVGTransforms && !transformFromSVGParent.IsIdentity()) {
    
    float pixelsPerCSSPx = frame->PresContext()->AppUnitsPerCSSPixel() /
                             aAppUnitsPerPixel;
    transformFromSVGParent._31 *= pixelsPerCSSPx;
    transformFromSVGParent._32 *= pixelsPerCSSPx;
    result = result * gfx3DMatrix::From2D(ThebesMatrix(transformFromSVGParent));
  }

  if (aProperties.mChildPerspective > 0.0) {
    gfx3DMatrix perspective;
    perspective._34 =
      -1.0 / NSAppUnitsToFloatPixels(aProperties.mChildPerspective, aAppUnitsPerPixel);
    


    perspective.ChangeBasis(aProperties.mToPerspectiveOrigin - aProperties.mToTransformOrigin);
    result = result * perspective;
  }

  


  Point3D newOrigin =
    Point3D(NSAppUnitsToFloatPixels(aOrigin.x, aAppUnitsPerPixel),
            NSAppUnitsToFloatPixels(aOrigin.y, aAppUnitsPerPixel),
            0.0f);
  Point3D roundedOrigin(hasSVGTransforms ? newOrigin.x : NS_round(newOrigin.x),
                        hasSVGTransforms ? newOrigin.y : NS_round(newOrigin.y),
                        0);
  Point3D offsetBetweenOrigins = roundedOrigin + aProperties.mToTransformOrigin;

  if (frame && frame->Preserves3D()) {
    
    NS_ASSERTION(frame->GetParent() &&
                 frame->GetParent()->IsTransformed() &&
                 frame->GetParent()->Preserves3DChildren(),
                 "Preserve3D mismatch!");
    FrameTransformProperties props(frame->GetParent(),
                                   aAppUnitsPerPixel,
                                   nullptr);

    
    
    
    
    gfx3DMatrix parent =
      GetResultingTransformMatrixInternal(props,
                                          aOrigin - frame->GetPosition(),
                                          aAppUnitsPerPixel, nullptr,
                                          aOutAncestor, !frame->IsTransformed());

    result.ChangeBasis(offsetBetweenOrigins);
    result = result * parent;
    if (aOffsetByOrigin) {
      result.Translate(roundedOrigin);
    }
    return result;
  }

  if (aOffsetByOrigin) {
    
    
    
    
    result.Translate(-aProperties.mToTransformOrigin);
    result.TranslatePost(offsetBetweenOrigins);
  } else {
    result.ChangeBasis(offsetBetweenOrigins);
  }
  return result;
}

bool
nsDisplayOpacity::CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder)
{
  if (ActiveLayerTracker::IsStyleAnimated(mFrame, eCSSProperty_opacity)) {
    return true;
  }

  if (nsLayoutUtils::IsAnimationLoggingEnabled()) {
    nsCString message;
    message.AppendLiteral("Performance warning: Async animation disabled because frame was not marked active for opacity animation");
    AnimationPlayerCollection::LogAsyncAnimationFailure(message,
                                                        Frame()->GetContent());
  }
  return false;
}

bool
nsDisplayTransform::CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder)
{
  if (mPrerender) {
    return true;
  }
  DebugOnly<bool> prerender = ShouldPrerenderTransformedContent(aBuilder, mFrame, true);
  NS_ASSERTION(!prerender, "Something changed under us!");
  return false;
}

 bool
nsDisplayTransform::ShouldPrerenderTransformedContent(nsDisplayListBuilder* aBuilder,
                                                      nsIFrame* aFrame,
                                                      bool aLogAnimations)
{
  
  
  
  
  if (!ActiveLayerTracker::IsStyleAnimated(aFrame, eCSSProperty_transform) &&
      (!aFrame->GetContent() ||
       !nsLayoutUtils::HasAnimationsForCompositor(aFrame->GetContent(),
                                                  eCSSProperty_transform))) {
    if (aLogAnimations) {
      nsCString message;
      message.AppendLiteral("Performance warning: Async animation disabled because frame was not marked active for transform animation");
      AnimationPlayerCollection::LogAsyncAnimationFailure(message,
                                                          aFrame->GetContent());
    }
    return false;
  }

  nsSize refSize = aBuilder->RootReferenceFrame()->GetSize();
  
  
  
  refSize += nsSize(refSize.width / 8, refSize.height / 8);
  nsSize frameSize = aFrame->GetVisualOverflowRectRelativeToSelf().Size();
  nscoord maxInAppUnits = nscoord_MAX;
  if (frameSize <= refSize) {
    maxInAppUnits = aFrame->PresContext()->DevPixelsToAppUnits(4096);
    nsRect visual = aFrame->GetVisualOverflowRect();
    if (visual.width <= maxInAppUnits && visual.height <= maxInAppUnits) {
      return true;
    }
  }

  if (aLogAnimations) {
    nsRect visual = aFrame->GetVisualOverflowRect();

    nsCString message;
    message.AppendLiteral("Performance warning: Async animation disabled because frame size (");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(frameSize.width));
    message.AppendLiteral(", ");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(frameSize.height));
    message.AppendLiteral(") is bigger than the viewport (");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(refSize.width));
    message.AppendLiteral(", ");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(refSize.height));
    message.AppendLiteral(") or the visual rectangle (");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(visual.width));
    message.AppendLiteral(", ");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(visual.height));
    message.AppendLiteral(") is larger than the max allowable value (");
    message.AppendInt(nsPresContext::AppUnitsToIntCSSPixels(maxInAppUnits));
    message.Append(')');
    AnimationPlayerCollection::LogAsyncAnimationFailure(message,
                                                        aFrame->GetContent());
  }
  return false;
}


static bool IsFrameVisible(nsIFrame* aFrame, const Matrix4x4& aMatrix)
{
  if (aMatrix.IsSingular()) {
    return false;
  }
  if (aFrame->StyleDisplay()->mBackfaceVisibility == NS_STYLE_BACKFACE_VISIBILITY_HIDDEN &&
      aMatrix.IsBackfaceVisible()) {
    return false;
  }
  return true;
}

const Matrix4x4&
nsDisplayTransform::GetTransform()
{
  if (mTransform.IsIdentity()) {
    float scale = mFrame->PresContext()->AppUnitsPerDevPixel();
    Point3D newOrigin =
      Point3D(NSAppUnitsToFloatPixels(mToReferenceFrame.x, scale),
              NSAppUnitsToFloatPixels(mToReferenceFrame.y, scale),
              0.0f);
    if (mTransformGetter) {
      mTransform = mTransformGetter(mFrame, scale);
      mTransform.ChangeBasis(newOrigin.x, newOrigin.y, newOrigin.z);
    } else {
      







      mTransform = ToMatrix4x4(
        GetResultingTransformMatrix(mFrame, ToReferenceFrame(), scale,
                                    nullptr, nullptr, mFrame->IsTransformed()));
    }
  }
  return mTransform;
}

bool
nsDisplayTransform::ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder)
{
  return ShouldPrerender();
}

already_AddRefed<Layer> nsDisplayTransform::BuildLayer(nsDisplayListBuilder *aBuilder,
                                                       LayerManager *aManager,
                                                       const ContainerLayerParameters& aContainerParameters)
{
  const Matrix4x4& newTransformMatrix = GetTransform();

  if (mFrame->StyleDisplay()->mBackfaceVisibility == NS_STYLE_BACKFACE_VISIBILITY_HIDDEN &&
      newTransformMatrix.IsBackfaceVisible()) {
    return nullptr;
  }

  uint32_t flags = ShouldPrerender() ?
    FrameLayerBuilder::CONTAINER_NOT_CLIPPED_BY_ANCESTORS : 0;
  nsRefPtr<ContainerLayer> container = aManager->GetLayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, mStoredList.GetChildren(),
                           aContainerParameters, &newTransformMatrix, flags);

  if (!container) {
    return nullptr;
  }

  
  
  if (mFrame->Preserves3D() || mFrame->Preserves3DChildren()) {
    container->SetContentFlags(container->GetContentFlags() | Layer::CONTENT_PRESERVE_3D);
  } else {
    container->SetContentFlags(container->GetContentFlags() & ~Layer::CONTENT_PRESERVE_3D);
  }

  nsDisplayListBuilder::AddAnimationsAndTransitionsToLayer(container, aBuilder,
                                                           this, mFrame,
                                                           eCSSProperty_transform);
  if (ShouldPrerender()) {
    container->SetUserData(nsIFrame::LayerIsPrerenderedDataKey(),
                           nullptr);
    container->SetContentFlags(container->GetContentFlags() | Layer::CONTENT_MAY_CHANGE_TRANSFORM);
  } else {
    container->RemoveUserData(nsIFrame::LayerIsPrerenderedDataKey());
    container->SetContentFlags(container->GetContentFlags() & ~Layer::CONTENT_MAY_CHANGE_TRANSFORM);
  }
  return container.forget();
}

nsDisplayItem::LayerState
nsDisplayTransform::GetLayerState(nsDisplayListBuilder* aBuilder,
                                  LayerManager* aManager,
                                  const ContainerLayerParameters& aParameters) {
  
  
  if (!GetTransform().Is2D() || mFrame->Preserves3D()) {
    return LAYER_ACTIVE_FORCE;
  }
  
  
  if (ActiveLayerTracker::IsStyleAnimated(mFrame, eCSSProperty_transform) &&
      !IsItemTooSmallForActiveLayer(this))
    return LAYER_ACTIVE;
  if (mFrame->GetContent()) {
    if (nsLayoutUtils::HasAnimationsForCompositor(mFrame->GetContent(),
                                                  eCSSProperty_transform)) {
      return LAYER_ACTIVE;
    }
  }

  const nsStyleDisplay* disp = mFrame->StyleDisplay();
  if ((disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_TRANSFORM)) {
    return LAYER_ACTIVE;
  }

  
  
  
  
  return RequiredLayerStateForChildren(aBuilder, aManager, aParameters,
    *mStoredList.GetChildren(), Frame());
}

bool nsDisplayTransform::ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                             nsRegion *aVisibleRegion)
{
  



  nsRect untransformedVisibleRect;
  if (ShouldPrerender() ||
      !UntransformVisibleRect(aBuilder, &untransformedVisibleRect))
  {
    untransformedVisibleRect = mFrame->GetVisualOverflowRectRelativeToSelf();
  }
  nsRegion untransformedVisible = untransformedVisibleRect;
  
  
  
  mStoredList.RecomputeVisibility(aBuilder, &untransformedVisible);
  return true;
}

#ifdef DEBUG_HIT
#include <time.h>
#endif


void nsDisplayTransform::HitTest(nsDisplayListBuilder *aBuilder,
                                 const nsRect& aRect,
                                 HitTestState *aState,
                                 nsTArray<nsIFrame*> *aOutFrames)
{
  






  
  float factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  Matrix4x4 matrix = GetTransform();

  if (!IsFrameVisible(mFrame, matrix)) {
    return;
  }

  




  
  matrix.Invert();
  nsRect resultingRect;
  if (aRect.width == 1 && aRect.height == 1) {
    
    Point4D point = matrix.ProjectPoint(Point(NSAppUnitsToFloatPixels(aRect.x, factor),
                                              NSAppUnitsToFloatPixels(aRect.y, factor)));
    if (!point.HasPositiveWCoord()) {
      return;
    }

    Point point2d = point.As2DPoint();

    resultingRect = nsRect(NSFloatPixelsToAppUnits(float(point2d.x), factor),
                           NSFloatPixelsToAppUnits(float(point2d.y), factor),
                           1, 1);

  } else {
    Rect originalRect(NSAppUnitsToFloatPixels(aRect.x, factor),
                      NSAppUnitsToFloatPixels(aRect.y, factor),
                      NSAppUnitsToFloatPixels(aRect.width, factor),
                      NSAppUnitsToFloatPixels(aRect.height, factor));

    Rect rect = matrix.ProjectRectBounds(originalRect);

    bool snap;
    nsRect childBounds = mStoredList.GetBounds(aBuilder, &snap);
    Rect childGfxBounds(NSAppUnitsToFloatPixels(childBounds.x, factor),
                        NSAppUnitsToFloatPixels(childBounds.y, factor),
                        NSAppUnitsToFloatPixels(childBounds.width, factor),
                        NSAppUnitsToFloatPixels(childBounds.height, factor));
    rect = rect.Intersect(childGfxBounds);

    resultingRect = nsRect(NSFloatPixelsToAppUnits(float(rect.X()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Y()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Width()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Height()), factor));
  }

  if (resultingRect.IsEmpty()) {
    return;
  }


#ifdef DEBUG_HIT
  printf("Frame: %p\n", dynamic_cast<void *>(mFrame));
  printf("  Untransformed point: (%f, %f)\n", resultingRect.X(), resultingRect.Y());
  uint32_t originalFrameCount = aOutFrames.Length();
#endif

  mStoredList.HitTest(aBuilder, resultingRect, aState, aOutFrames);

#ifdef DEBUG_HIT
  if (originalFrameCount != aOutFrames.Length())
    printf("  Hit! Time: %f, first frame: %p\n", static_cast<double>(clock()),
           dynamic_cast<void *>(aOutFrames.ElementAt(0)));
  printf("=== end of hit test ===\n");
#endif

}

float
nsDisplayTransform::GetHitDepthAtPoint(nsDisplayListBuilder* aBuilder, const nsPoint& aPoint)
{
  
  float factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  Matrix4x4 matrix = GetTransform();

  NS_ASSERTION(IsFrameVisible(mFrame, matrix), "We can't have hit a frame that isn't visible!");

  Matrix4x4 inverse = matrix;
  inverse.Invert();
  Point4D point = inverse.ProjectPoint(Point(NSAppUnitsToFloatPixels(aPoint.x, factor),
                                             NSAppUnitsToFloatPixels(aPoint.y, factor)));
  NS_ASSERTION(point.HasPositiveWCoord(), "Why are we trying to get the depth for a point we didn't hit?");

  Point point2d = point.As2DPoint();

  Point3D transformed = matrix * Point3D(point2d.x, point2d.y, 0);
  return transformed.z;
}




nsRect nsDisplayTransform::GetBounds(nsDisplayListBuilder *aBuilder, bool* aSnap)
{
  nsRect untransformedBounds = ShouldPrerender() ?
    mFrame->GetVisualOverflowRectRelativeToSelf() :
    mStoredList.GetBounds(aBuilder, aSnap);
  *aSnap = false;
  
  float factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  return nsLayoutUtils::MatrixTransformRect(untransformedBounds,
                                            To3DMatrix(GetTransform()),
                                            factor);
}

















nsRegion nsDisplayTransform::GetOpaqueRegion(nsDisplayListBuilder *aBuilder,
                                             bool* aSnap)
{
  *aSnap = false;
  nsRect untransformedVisible;
  
  
  
  
  
  
  if (ShouldPrerender() ||
      !UntransformVisibleRect(aBuilder, &untransformedVisible)) {
      return nsRegion();
  }

  const Matrix4x4& matrix = GetTransform();

  nsRegion result;
  Matrix matrix2d;
  bool tmpSnap;
  if (matrix.Is2D(&matrix2d) &&
      matrix2d.PreservesAxisAlignedRectangles() &&
      mStoredList.GetOpaqueRegion(aBuilder, &tmpSnap).Contains(untransformedVisible)) {
    result = mVisibleRect.Intersect(GetBounds(aBuilder, &tmpSnap));
  }
  return result;
}





bool nsDisplayTransform::IsUniform(nsDisplayListBuilder *aBuilder, nscolor* aColor)
{
  nsRect untransformedVisible;
  if (!UntransformVisibleRect(aBuilder, &untransformedVisible)) {
    return false;
  }
  const Matrix4x4& matrix = GetTransform();

  Matrix matrix2d;
  return matrix.Is2D(&matrix2d) &&
         matrix2d.PreservesAxisAlignedRectangles() &&
         mStoredList.GetVisibleRect().Contains(untransformedVisible) &&
         mStoredList.IsUniform(aBuilder, aColor);
}





#ifndef UNIFIED_CONTINUATIONS

bool
nsDisplayTransform::TryMerge(nsDisplayListBuilder *aBuilder,
                             nsDisplayItem *aItem)
{
  return false;
}

#else

bool
nsDisplayTransform::TryMerge(nsDisplayListBuilder *aBuilder,
                             nsDisplayItem *aItem)
{
  NS_PRECONDITION(aItem, "Why did you try merging with a null item?");
  NS_PRECONDITION(aBuilder, "Why did you try merging with a null builder?");

  
  if (aItem->GetType() != TYPE_TRANSFORM)
    return false;

  
  if (aItem->Frame()->GetContent() != mFrame->GetContent())
    return false;

  if (aItem->GetClip() != GetClip())
    return false;

  


  mStoredList.MergeFromTrackingMergedFrames(&static_cast<nsDisplayTransform*>(aItem)->mStoredList);
  return true;
}

#endif















nsRect nsDisplayTransform::TransformRect(const nsRect &aUntransformedBounds,
                                         const nsIFrame* aFrame,
                                         const nsPoint &aOrigin,
                                         const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Can't take the transform based on a null frame!");

  float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
  return nsLayoutUtils::MatrixTransformRect
    (aUntransformedBounds,
     GetResultingTransformMatrix(aFrame, aOrigin, factor, aBoundsOverride),
     factor);
}

nsRect nsDisplayTransform::TransformRectOut(const nsRect &aUntransformedBounds,
                                            const nsIFrame* aFrame,
                                            const nsPoint &aOrigin,
                                            const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Can't take the transform based on a null frame!");

  float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
  return nsLayoutUtils::MatrixTransformRectOut
    (aUntransformedBounds,
     GetResultingTransformMatrix(aFrame, aOrigin, factor, aBoundsOverride),
     factor);
}

bool nsDisplayTransform::UntransformRect(const nsRect &aTransformedBounds,
                                         const nsRect &aChildBounds,
                                         const nsIFrame* aFrame,
                                         const nsPoint &aOrigin,
                                         nsRect *aOutRect)
{
  NS_PRECONDITION(aFrame, "Can't take the transform based on a null frame!");

  float factor = aFrame->PresContext()->AppUnitsPerDevPixel();

  gfx3DMatrix transform = GetResultingTransformMatrix(aFrame, aOrigin, factor, nullptr);
  if (transform.IsSingular()) {
    return false;
  }

  Rect result(NSAppUnitsToFloatPixels(aTransformedBounds.x, factor),
              NSAppUnitsToFloatPixels(aTransformedBounds.y, factor),
              NSAppUnitsToFloatPixels(aTransformedBounds.width, factor),
              NSAppUnitsToFloatPixels(aTransformedBounds.height, factor));

  Rect childGfxBounds(NSAppUnitsToFloatPixels(aChildBounds.x, factor),
                      NSAppUnitsToFloatPixels(aChildBounds.y, factor),
                      NSAppUnitsToFloatPixels(aChildBounds.width, factor),
                      NSAppUnitsToFloatPixels(aChildBounds.height, factor));

  result = ToMatrix4x4(transform.Inverse()).ProjectRectBounds(result);
  result = result.Intersect(childGfxBounds);
  *aOutRect = nsLayoutUtils::RoundGfxRectToAppRect(ThebesRect(result), factor);
  return true;
}

bool nsDisplayTransform::UntransformVisibleRect(nsDisplayListBuilder* aBuilder,
                                                nsRect *aOutRect)
{
  const gfx3DMatrix& matrix = To3DMatrix(GetTransform());
  if (matrix.IsSingular())
    return false;

  
  float factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  Rect result(NSAppUnitsToFloatPixels(mVisibleRect.x, factor),
              NSAppUnitsToFloatPixels(mVisibleRect.y, factor),
              NSAppUnitsToFloatPixels(mVisibleRect.width, factor),
              NSAppUnitsToFloatPixels(mVisibleRect.height, factor));

  bool snap;
  nsRect childBounds = mStoredList.GetBounds(aBuilder, &snap);
  Rect childGfxBounds(NSAppUnitsToFloatPixels(childBounds.x, factor),
                      NSAppUnitsToFloatPixels(childBounds.y, factor),
                      NSAppUnitsToFloatPixels(childBounds.width, factor),
                      NSAppUnitsToFloatPixels(childBounds.height, factor));

  
  result = ToMatrix4x4(matrix.Inverse()).ProjectRectBounds(result);
  result = result.Intersect(childGfxBounds);

  *aOutRect = nsLayoutUtils::RoundGfxRectToAppRect(ThebesRect(result), factor);

  return true;
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayTransform::WriteDebugInfo(nsACString& aTo)
{
  std::stringstream ss;
  AppendToString(ss, GetTransform());
  aTo += ss.str().c_str();
}
#endif

nsDisplaySVGEffects::nsDisplaySVGEffects(nsDisplayListBuilder* aBuilder,
                                         nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList),
      mEffectsBounds(aFrame->GetVisualOverflowRectRelativeToSelf())
{
  MOZ_COUNT_CTOR(nsDisplaySVGEffects);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplaySVGEffects::~nsDisplaySVGEffects()
{
  MOZ_COUNT_DTOR(nsDisplaySVGEffects);
}
#endif

nsRegion nsDisplaySVGEffects::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                              bool* aSnap)
{
  *aSnap = false;
  return nsRegion();
}

void
nsDisplaySVGEffects::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                             HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsPoint rectCenter(aRect.x + aRect.width / 2, aRect.y + aRect.height / 2);
  if (nsSVGIntegrationUtils::HitTestFrameForEffects(mFrame,
      rectCenter - ToReferenceFrame())) {
    mList.HitTest(aBuilder, aRect, aState, aOutFrames);
  }
}

void
nsDisplaySVGEffects::PaintAsLayer(nsDisplayListBuilder* aBuilder,
                                  nsRenderingContext* aCtx,
                                  LayerManager* aManager)
{
  nsSVGIntegrationUtils::PaintFramesWithEffects(aCtx, mFrame,
                                                mVisibleRect,
                                                aBuilder, aManager);
}

LayerState
nsDisplaySVGEffects::GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters)
{
  return LAYER_SVG_EFFECTS;
}

already_AddRefed<Layer>
nsDisplaySVGEffects::BuildLayer(nsDisplayListBuilder* aBuilder,
                                LayerManager* aManager,
                                const ContainerLayerParameters& aContainerParameters)
{
  const nsIContent* content = mFrame->GetContent();
  bool hasSVGLayout = (mFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT);
  if (hasSVGLayout) {
    nsISVGChildFrame *svgChildFrame = do_QueryFrame(mFrame);
    if (!svgChildFrame || !mFrame->GetContent()->IsSVG()) {
      NS_ASSERTION(false, "why?");
      return nullptr;
    }
    if (!static_cast<const nsSVGElement*>(content)->HasValidDimensions()) {
      return nullptr; 
    }
  }

  float opacity = mFrame->StyleDisplay()->mOpacity;
  if (opacity == 0.0f)
    return nullptr;

  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(mFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);

  bool isOK = effectProperties.HasNoFilterOrHasValidFilter();
  effectProperties.GetClipPathFrame(&isOK);
  effectProperties.GetMaskFrame(&isOK);

  if (!isOK) {
    return nullptr;
  }

  ContainerLayerParameters newContainerParameters = aContainerParameters;
  if (effectProperties.HasValidFilter()) {
    newContainerParameters.mDisableSubpixelAntialiasingInDescendants = true;
  }

  nsRefPtr<ContainerLayer> container = aManager->GetLayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, &mList,
                           newContainerParameters, nullptr);

  return container.forget();
}

bool nsDisplaySVGEffects::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                              nsRegion* aVisibleRegion) {
  nsPoint offset = ToReferenceFrame();
  nsRect dirtyRect =
    nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(mFrame,
                                                           mVisibleRect - offset) +
    offset;

  
  
  nsRegion childrenVisible(dirtyRect);
  nsRect r = dirtyRect.Intersect(mList.GetBounds(aBuilder));
  mList.ComputeVisibilityForSublist(aBuilder, &childrenVisible, r);
  return true;
}

bool nsDisplaySVGEffects::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem)
{
  if (aItem->GetType() != TYPE_SVG_EFFECTS)
    return false;
  
  
  
  if (aItem->Frame()->GetContent() != mFrame->GetContent())
    return false;
  if (aItem->GetClip() != GetClip())
    return false;
  nsDisplaySVGEffects* other = static_cast<nsDisplaySVGEffects*>(aItem);
  MergeFromTrackingMergedFrames(other);
  mEffectsBounds.UnionRect(mEffectsBounds,
    other->mEffectsBounds + other->mFrame->GetOffsetTo(mFrame));
  return true;
}

gfxRect
nsDisplaySVGEffects::BBoxInUserSpace() const
{
  return nsSVGUtils::GetBBox(mFrame);
}

gfxPoint
nsDisplaySVGEffects::UserSpaceOffset() const
{
  return nsSVGUtils::FrameSpaceInCSSPxToUserSpaceOffset(mFrame);
}

void
nsDisplaySVGEffects::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                               const nsDisplayItemGeometry* aGeometry,
                                               nsRegion* aInvalidRegion)
{
  const nsDisplaySVGEffectsGeometry* geometry =
    static_cast<const nsDisplaySVGEffectsGeometry*>(aGeometry);
  bool snap;
  nsRect bounds = GetBounds(aBuilder, &snap);
  if (geometry->mFrameOffsetToReferenceFrame != ToReferenceFrame() ||
      geometry->mUserSpaceOffset != UserSpaceOffset() ||
      !geometry->mBBox.IsEqualInterior(BBoxInUserSpace())) {
    
    
    
    
    
    
    aInvalidRegion->Or(bounds, geometry->mBounds);
  }
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplaySVGEffects::PrintEffects(nsACString& aTo)
{
  nsIFrame* firstFrame =
    nsLayoutUtils::FirstContinuationOrIBSplitSibling(mFrame);
  nsSVGEffects::EffectProperties effectProperties =
    nsSVGEffects::GetEffectProperties(firstFrame);
  bool isOK = true;
  nsSVGClipPathFrame *clipPathFrame = effectProperties.GetClipPathFrame(&isOK);
  bool first = true;
  aTo += " effects=(";
  if (mFrame->StyleDisplay()->mOpacity != 1.0f) {
    first = false;
    aTo += nsPrintfCString("opacity(%f)", mFrame->StyleDisplay()->mOpacity);
  }
  if (clipPathFrame) {
    if (!first) {
      aTo += ", ";
    }
    aTo += nsPrintfCString("clip(%s)", clipPathFrame->IsTrivial() ? "trivial" : "non-trivial");
    first = false;
  }
  if (effectProperties.HasValidFilter()) {
    if (!first) {
      aTo += ", ";
    }
    aTo += "filter";
    first = false;
  }
  if (effectProperties.GetMaskFrame(&isOK)) {
    if (!first) {
      aTo += ", ";
    }
    aTo += "mask";
  }
  aTo += ")";
}
#endif

