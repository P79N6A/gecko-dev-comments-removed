









#include "RestyleManager.h"
#include "nsLayoutUtils.h"
#include "GeckoProfiler.h"
#include "nsStyleChangeList.h"
#include "nsRuleProcessorData.h"
#include "nsStyleUtil.h"
#include "nsCSSFrameConstructor.h"
#include "nsSVGEffects.h"
#include "nsCSSRendering.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"
#include "nsViewManager.h"
#include "nsRenderingContext.h"
#include "nsSVGIntegrationUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsContainerFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsBlockFrame.h"
#include "nsViewportFrame.h"
#include "nsSVGTextFrame2.h"
#include "nsSVGTextPathFrame.h"
#include "StickyScrollContainer.h"
#include "nsIRootBox.h"
#include "nsIDOMMutationEvent.h"
#include "nsContentUtils.h"

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

namespace mozilla {

RestyleManager::RestyleManager(nsPresContext* aPresContext)
  : mPresContext(aPresContext)
  , mRebuildAllStyleData(false)
  , mObservingRefreshDriver(false)
  , mInStyleRefresh(false)
  , mHoverGeneration(0)
  , mRebuildAllExtraHint(nsChangeHint(0))
  , mAnimationGeneration(0)
  , mPendingRestyles(ELEMENT_HAS_PENDING_RESTYLE |
                     ELEMENT_IS_POTENTIAL_RESTYLE_ROOT)
  , mPendingAnimationRestyles(ELEMENT_HAS_PENDING_ANIMATION_RESTYLE |
                              ELEMENT_IS_POTENTIAL_ANIMATION_RESTYLE_ROOT)
{
  mPendingRestyles.Init(this);
  mPendingAnimationRestyles.Init(this);
}

void
RestyleManager::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  mOverflowChangedTracker.RemoveFrame(aFrame);
}

#ifdef DEBUG
  
  
static bool gInApplyRenderingChangeToTree = false;
#endif

static void
DoApplyRenderingChangeToTree(nsIFrame* aFrame,
                             nsChangeHint aChange);









static void
SyncViewsAndInvalidateDescendants(nsIFrame* aFrame,
                                  nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");
  NS_ASSERTION(aChange == (aChange & (nsChangeHint_RepaintFrame |
                                      nsChangeHint_SyncFrameView |
                                      nsChangeHint_UpdateOpacityLayer)),
               "Invalid change flag");

  nsView* view = aFrame->GetView();
  if (view) {
    if (aChange & nsChangeHint_SyncFrameView) {
      nsContainerFrame::SyncFrameViewProperties(aFrame->PresContext(),
                                                aFrame, nullptr, view);
    }
  }

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) {
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
          DoApplyRenderingChangeToTree(outOfFlowFrame, aChange);
        } else if (lists.CurrentID() == nsIFrame::kPopupList) {
          DoApplyRenderingChangeToTree(child, aChange);
        } else {  
          SyncViewsAndInvalidateDescendants(child, aChange);
        }
      }
    }
  }
}



















static nsIFrame*
GetFrameForChildrenOnlyTransformHint(nsIFrame *aFrame)
{
  if (aFrame->GetType() == nsGkAtoms::viewportFrame) {
    
    
    
    aFrame = aFrame->GetFirstPrincipalChild();
  }
  
  
  aFrame = aFrame->GetContent()->GetPrimaryFrame();
  if (aFrame->GetType() == nsGkAtoms::svgOuterSVGFrame) {
    aFrame = aFrame->GetFirstPrincipalChild();
    NS_ABORT_IF_FALSE(aFrame->GetType() == nsGkAtoms::svgOuterSVGAnonChildFrame,
                      "Where is the nsSVGOuterSVGFrame's anon child??");
  }
  NS_ABORT_IF_FALSE(aFrame->IsFrameOfType(nsIFrame::eSVG |
                                          nsIFrame::eSVGContainer),
                    "Children-only transforms only expected on SVG frames");
  return aFrame;
}

static void
DoApplyRenderingChangeToTree(nsIFrame* aFrame,
                             nsChangeHint aChange)
{
  NS_PRECONDITION(gInApplyRenderingChangeToTree,
                  "should only be called within ApplyRenderingChangeToTree");

  for ( ; aFrame; aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame)) {
    
    
    
    
    
    SyncViewsAndInvalidateDescendants(aFrame,
      nsChangeHint(aChange & (nsChangeHint_RepaintFrame |
                              nsChangeHint_SyncFrameView |
                              nsChangeHint_UpdateOpacityLayer)));
    
    
    
    bool needInvalidatingPaint = false;

    
    if (aChange & nsChangeHint_RepaintFrame) {
      
      
      
      
      
      needInvalidatingPaint = true;
      aFrame->InvalidateFrameSubtree();
      if (aChange & nsChangeHint_UpdateEffects &&
          aFrame->IsFrameOfType(nsIFrame::eSVG) &&
          !(aFrame->GetStateBits() & NS_STATE_IS_OUTER_SVG)) {
        
        nsSVGUtils::ScheduleReflowSVG(aFrame);
      }
    }
    if (aChange & nsChangeHint_UpdateTextPath) {
      if (aFrame->GetType() == nsGkAtoms::svgTextPathFrame) {
        
        static_cast<nsSVGTextPathFrame*>(aFrame)->NotifyGlyphMetricsChange();
      } else if (aFrame->IsSVGText()) {
        
        NS_ASSERTION(aFrame->GetContent()->IsSVG(nsGkAtoms::textPath),
                     "expected frame for a <textPath> element");
        nsIFrame* text = nsLayoutUtils::GetClosestFrameOfType(
                                                      aFrame,
                                                      nsGkAtoms::svgTextFrame2);
        NS_ASSERTION(text, "expected to find an ancestor nsSVGTextFrame2");
        static_cast<nsSVGTextFrame2*>(text)->NotifyGlyphMetricsChange();
      } else {
        NS_ABORT_IF_FALSE(false, "unexpected frame got "
                                 "nsChangeHint_UpdateTextPath");
      }
    }
    if (aChange & nsChangeHint_UpdateOpacityLayer) {
      
      
      needInvalidatingPaint = true;
      aFrame->MarkLayersActive(nsChangeHint_UpdateOpacityLayer);
      if (nsSVGIntegrationUtils::UsingEffectsForFrame(aFrame)) {
        
        
        aFrame->InvalidateFrameSubtree();
      }
    }
    if ((aChange & nsChangeHint_UpdateTransformLayer) &&
        aFrame->IsTransformed()) {
      aFrame->MarkLayersActive(nsChangeHint_UpdateTransformLayer);
      
      
      
      
      if (!needInvalidatingPaint) {
        needInvalidatingPaint |= !aFrame->TryUpdateTransformOnly();
      }
    }
    if (aChange & nsChangeHint_ChildrenOnlyTransform) {
      needInvalidatingPaint = true;
      nsIFrame* childFrame =
        GetFrameForChildrenOnlyTransformHint(aFrame)->GetFirstPrincipalChild();
      for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
        childFrame->MarkLayersActive(nsChangeHint_UpdateTransformLayer);
      }
    }
    aFrame->SchedulePaint(needInvalidatingPaint ?
                          nsIFrame::PAINT_DEFAULT :
                          nsIFrame::PAINT_COMPOSITE_ONLY);
  }
}

static void
ApplyRenderingChangeToTree(nsPresContext* aPresContext,
                           nsIFrame* aFrame,
                           nsChangeHint aChange)
{
  
  
  
  NS_ASSERTION(!(aChange & nsChangeHint_UpdateTransformLayer) ||
               aFrame->IsTransformed() ||
               aFrame->StyleDisplay()->HasTransformStyle(),
               "Unexpected UpdateTransformLayer hint");

  nsIPresShell *shell = aPresContext->PresShell();
  if (shell->IsPaintingSuppressed()) {
    
    aChange = NS_SubtractHint(aChange, nsChangeHint_RepaintFrame);
    if (!aChange) {
      return;
    }
  }

  
  
  nsStyleContext *bgSC;
  while (!nsCSSRendering::FindBackground(aFrame, &bgSC)) {
    aFrame = aFrame->GetParent();
    NS_ASSERTION(aFrame, "root frame must paint");
  }

  
  

  

#ifdef DEBUG
  gInApplyRenderingChangeToTree = true;
#endif
  DoApplyRenderingChangeToTree(aFrame, aChange);
#ifdef DEBUG
  gInApplyRenderingChangeToTree = false;
#endif
}

bool
RestyleManager::RecomputePosition(nsIFrame* aFrame)
{
  
  
  
  if (aFrame->GetType() == nsGkAtoms::tableFrame) {
    return true;
  }

  
  
  
  if (aFrame->HasView() ||
      (aFrame->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    StyleChangeReflow(aFrame, nsChangeHint_NeedReflow);
    return false;
  }

  const nsStyleDisplay* display = aFrame->StyleDisplay();
  
  if (display->mPosition == NS_STYLE_POSITION_STATIC) {
    return true;
  }

  aFrame->SchedulePaint();

  
  if (display->IsRelativelyPositionedStyle()) {
    switch (display->mDisplay) {
      case NS_STYLE_DISPLAY_TABLE_CAPTION:
      case NS_STYLE_DISPLAY_TABLE_CELL:
      case NS_STYLE_DISPLAY_TABLE_ROW:
      case NS_STYLE_DISPLAY_TABLE_ROW_GROUP:
      case NS_STYLE_DISPLAY_TABLE_HEADER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP:
      case NS_STYLE_DISPLAY_TABLE_COLUMN:
      case NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP:
        
        
        
        return true;
      default:
        break;
    }

    nsIFrame* cb = aFrame->GetContainingBlock();
    nsMargin newOffsets;

    
    if (display->mPosition == NS_STYLE_POSITION_STICKY) {
      StickyScrollContainer::ComputeStickyOffsets(aFrame);
    } else {
      MOZ_ASSERT(NS_STYLE_POSITION_RELATIVE == display->mPosition,
                 "Unexpected type of positioning");
      const nsSize size = cb->GetContentRectRelativeToSelf().Size();

      nsHTMLReflowState::ComputeRelativeOffsets(
          cb->StyleVisibility()->mDirection,
          aFrame, size.width, size.height, newOffsets);
      NS_ASSERTION(newOffsets.left == -newOffsets.right &&
                   newOffsets.top == -newOffsets.bottom,
                   "ComputeRelativeOffsets should return valid results");
    }

    nsPoint position = aFrame->GetNormalPosition();

    
    nsHTMLReflowState::ApplyRelativePositioning(aFrame, newOffsets, &position);
    aFrame->SetPosition(position);

    return true;
  }

  
  
  
  
  
  
  
  
  const nsStylePosition* position = aFrame->StylePosition();
  if (position->mWidth.GetUnit() != eStyleUnit_Auto &&
      position->mHeight.GetUnit() != eStyleUnit_Auto) {
    
    
    nsRefPtr<nsRenderingContext> rc = aFrame->PresContext()->GetPresShell()->
      GetReferenceRenderingContext();

    
    
    nsIFrame* parentFrame = aFrame->GetParent();
    nsSize parentSize = parentFrame->GetSize();

    nsFrameState savedState = parentFrame->GetStateBits();
    nsHTMLReflowState parentReflowState(aFrame->PresContext(), parentFrame,
                                        rc, parentSize);
    parentFrame->RemoveStateBits(~nsFrameState(0));
    parentFrame->AddStateBits(savedState);

    NS_WARN_IF_FALSE(parentSize.width != NS_INTRINSICSIZE &&
                     parentSize.height != NS_INTRINSICSIZE,
                     "parentSize should be valid");
    parentReflowState.SetComputedWidth(std::max(parentSize.width, 0));
    parentReflowState.SetComputedHeight(std::max(parentSize.height, 0));
    parentReflowState.mComputedMargin.SizeTo(0, 0, 0, 0);
    parentSize.height = NS_AUTOHEIGHT;

    parentReflowState.mComputedPadding = parentFrame->GetUsedPadding();
    parentReflowState.mComputedBorderPadding =
      parentFrame->GetUsedBorderAndPadding();

    nsSize availSize(parentSize.width, NS_INTRINSICSIZE);

    nsSize size = aFrame->GetSize();
    ViewportFrame* viewport = do_QueryFrame(parentFrame);
    nsSize cbSize = viewport ?
      viewport->AdjustReflowStateAsContainingBlock(&parentReflowState).Size()
      : aFrame->GetContainingBlock()->GetSize();
    const nsMargin& parentBorder =
      parentReflowState.mStyleBorder->GetComputedBorder();
    cbSize -= nsSize(parentBorder.LeftRight(), parentBorder.TopBottom());
    nsHTMLReflowState reflowState(aFrame->PresContext(), parentReflowState,
                                  aFrame, availSize, cbSize.width,
                                  cbSize.height);

    
    
    if (NS_AUTOOFFSET == reflowState.mComputedOffsets.left) {
      reflowState.mComputedOffsets.left = cbSize.width -
                                          reflowState.mComputedOffsets.right -
                                          reflowState.mComputedMargin.right -
                                          size.width -
                                          reflowState.mComputedMargin.left;
    }

    if (NS_AUTOOFFSET == reflowState.mComputedOffsets.top) {
      reflowState.mComputedOffsets.top = cbSize.height -
                                         reflowState.mComputedOffsets.bottom -
                                         reflowState.mComputedMargin.bottom -
                                         size.height -
                                         reflowState.mComputedMargin.top;
    }

    
    nsPoint pos(parentBorder.left + reflowState.mComputedOffsets.left +
                reflowState.mComputedMargin.left,
                parentBorder.top + reflowState.mComputedOffsets.top +
                reflowState.mComputedMargin.top);
    aFrame->SetPosition(pos);

    return true;
  }

  
  StyleChangeReflow(aFrame, nsChangeHint_NeedReflow);
  return false;
}

nsresult
RestyleManager::StyleChangeReflow(nsIFrame* aFrame, nsChangeHint aHint)
{
  
  
  if (aFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)
    return NS_OK;

  nsIPresShell::IntrinsicDirty dirtyType;
  if (aHint & nsChangeHint_ClearDescendantIntrinsics) {
    NS_ASSERTION(aHint & nsChangeHint_ClearAncestorIntrinsics,
                 "Please read the comments in nsChangeHint.h");
    dirtyType = nsIPresShell::eStyleChange;
  } else if (aHint & nsChangeHint_ClearAncestorIntrinsics) {
    dirtyType = nsIPresShell::eTreeChange;
  } else {
    dirtyType = nsIPresShell::eResize;
  }

  nsFrameState dirtyBits;
  if (aHint & nsChangeHint_NeedDirtyReflow) {
    dirtyBits = NS_FRAME_IS_DIRTY;
  } else {
    dirtyBits = NS_FRAME_HAS_DIRTY_CHILDREN;
  }

  do {
    mPresContext->PresShell()->FrameNeedsReflow(aFrame, dirtyType, dirtyBits);
    aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame);
  } while (aFrame);

  return NS_OK;
}

NS_DECLARE_FRAME_PROPERTY(ChangeListProperty, nullptr)





static bool
FrameHasPositionedPlaceholderDescendants(nsIFrame* aFrame, uint32_t aPositionMask)
{
  const nsIFrame::ChildListIDs skip(nsIFrame::kAbsoluteList |
                                    nsIFrame::kFixedList);
  for (nsIFrame::ChildListIterator lists(aFrame); !lists.IsDone(); lists.Next()) {
    if (!skip.Contains(lists.CurrentID())) {
      for (nsFrameList::Enumerator childFrames(lists.CurrentList());
           !childFrames.AtEnd(); childFrames.Next()) {
        nsIFrame* f = childFrames.get();
        if (f->GetType() == nsGkAtoms::placeholderFrame) {
          nsIFrame* outOfFlow = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
          
          
          NS_ASSERTION(!outOfFlow->IsSVGText(),
                       "SVG text frames can't be out of flow");
          if (aPositionMask & (1 << outOfFlow->StyleDisplay()->mPosition)) {
            return true;
          }
        }
        if (FrameHasPositionedPlaceholderDescendants(f, aPositionMask)) {
          return true;
        }
      }
    }
  }
  return false;
}

static bool
NeedToReframeForAddingOrRemovingTransform(nsIFrame* aFrame)
{
  static_assert(0 <= NS_STYLE_POSITION_ABSOLUTE &&
                NS_STYLE_POSITION_ABSOLUTE < 32, "Style constant out of range");
  static_assert(0 <= NS_STYLE_POSITION_FIXED &&
                NS_STYLE_POSITION_FIXED < 32, "Style constant out of range");

  uint32_t positionMask;
  
  
  if (aFrame->IsAbsolutelyPositioned() ||
      aFrame->IsRelativelyPositioned()) {
    
    
    
    
    positionMask = 1 << NS_STYLE_POSITION_FIXED;
  } else {
    
    
    positionMask = (1 << NS_STYLE_POSITION_FIXED) |
        (1 << NS_STYLE_POSITION_ABSOLUTE);
  }
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetNextContinuationOrSpecialSibling(f)) {
    if (FrameHasPositionedPlaceholderDescendants(f, positionMask)) {
      return true;
    }
  }
  return false;
}

nsresult
RestyleManager::ProcessRestyledFrames(nsStyleChangeList& aChangeList)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
               "Someone forgot a script blocker");
  int32_t count = aChangeList.Count();
  if (!count)
    return NS_OK;

  PROFILER_LABEL("CSS", "ProcessRestyledFrames");

  
  
  FrameConstructor()->BeginUpdate();

  FramePropertyTable* propTable = mPresContext->PropertyTable();

  
  
  
  int32_t index = count;

  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->Set(changeData->mFrame, ChangeListProperty(),
                     NS_INT32_TO_PTR(1));
    }
  }

  index = count;

  while (0 <= --index) {
    nsIFrame* frame;
    nsIContent* content;
    bool didReflowThisFrame = false;
    nsChangeHint hint;
    aChangeList.ChangeAt(index, frame, content, hint);

    NS_ASSERTION(!(hint & nsChangeHint_AllReflowHints) ||
                 (hint & nsChangeHint_NeedReflow),
                 "Reflow hint bits set without actually asking for a reflow");

    
    if (frame && !propTable->Get(frame, ChangeListProperty())) {
      continue;
    }

    if (frame && frame->GetContent() != content) {
      
      
      frame = nullptr;
      if (!(hint & nsChangeHint_ReconstructFrame)) {
        continue;
      }
    }

    if ((hint & nsChangeHint_AddOrRemoveTransform) && frame &&
        !(hint & nsChangeHint_ReconstructFrame)) {
      if (NeedToReframeForAddingOrRemovingTransform(frame)) {
        NS_UpdateHint(hint, nsChangeHint_ReconstructFrame);
      } else {
        
        
        
        
        
        if (frame->IsPositioned()) {
          
          
          
          frame->AddStateBits(NS_FRAME_MAY_BE_TRANSFORMED);
          if (!frame->IsAbsoluteContainer() &&
              (frame->GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN)) {
            frame->MarkAsAbsoluteContainingBlock();
          }
        } else {
          
          
          
          if (frame->IsAbsoluteContainer()) {
            frame->MarkAsNotAbsoluteContainingBlock();
          }
        }
      }
    }
    if (hint & nsChangeHint_ReconstructFrame) {
      
      
      
      
      
      
      FrameConstructor()->RecreateFramesForContent(content, false);
    } else {
      NS_ASSERTION(frame, "This shouldn't happen");

      if ((frame->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
          (frame->GetStateBits() & NS_FRAME_IS_NONDISPLAY)) {
        
        
        hint = NS_SubtractHint(hint,
                 NS_CombineHint(nsChangeHint_UpdateOverflow,
                                nsChangeHint_ChildrenOnlyTransform));
      }

      if (hint & nsChangeHint_UpdateEffects) {
        nsSVGEffects::UpdateEffects(frame);
      }
      if (hint & nsChangeHint_NeedReflow) {
        StyleChangeReflow(frame, hint);
        didReflowThisFrame = true;
      }
      if (hint & (nsChangeHint_RepaintFrame | nsChangeHint_SyncFrameView |
                  nsChangeHint_UpdateOpacityLayer | nsChangeHint_UpdateTransformLayer |
                  nsChangeHint_ChildrenOnlyTransform)) {
        ApplyRenderingChangeToTree(mPresContext, frame, hint);
      }
      if ((hint & nsChangeHint_RecomputePosition) && !didReflowThisFrame) {
        
        if (!RecomputePosition(frame)) {
          didReflowThisFrame = true;
        }
      }
      NS_ASSERTION(!(hint & nsChangeHint_ChildrenOnlyTransform) ||
                   (hint & nsChangeHint_UpdateOverflow),
                   "nsChangeHint_UpdateOverflow should be passed too");
      if ((hint & nsChangeHint_UpdateOverflow) && !didReflowThisFrame) {
        if (hint & nsChangeHint_ChildrenOnlyTransform) {
          
          nsIFrame* hintFrame = GetFrameForChildrenOnlyTransformHint(frame);
          nsIFrame* childFrame = hintFrame->GetFirstPrincipalChild();
          for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
            NS_ABORT_IF_FALSE(childFrame->IsFrameOfType(nsIFrame::eSVG),
                              "Not expecting non-SVG children");
            
            
            if (!(childFrame->GetStateBits() &
                  (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN))) {
              mOverflowChangedTracker.AddFrame(childFrame);
            }
            NS_ASSERTION(!nsLayoutUtils::GetNextContinuationOrSpecialSibling(childFrame),
                         "SVG frames should not have continuations or special siblings");
            NS_ASSERTION(childFrame->GetParent() == hintFrame,
                         "SVG child frame not expected to have different parent");
          }
        }
        
        
        if (!(frame->GetStateBits() &
              (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN))) {
          while (frame) {
            mOverflowChangedTracker.AddFrame(frame);

            frame =
              nsLayoutUtils::GetNextContinuationOrSpecialSibling(frame);
          }
        }
      }
      if (hint & nsChangeHint_UpdateCursor) {
        mPresContext->PresShell()->SynthesizeMouseMove(false);
      }
    }
  }

  FrameConstructor()->EndUpdate();

  
  
  
  index = count;
  while (0 <= --index) {
    const nsStyleChangeData* changeData;
    aChangeList.ChangeAt(index, &changeData);
    if (changeData->mFrame) {
      propTable->Delete(changeData->mFrame, ChangeListProperty());
    }

#ifdef DEBUG
    
    if (changeData->mContent) {
      if (!nsAnimationManager::ContentOrAncestorHasAnimation(changeData->mContent) &&
          !nsTransitionManager::ContentOrAncestorHasTransition(changeData->mContent)) {
        nsIFrame* frame = changeData->mContent->GetPrimaryFrame();
        if (frame) {
          DebugVerifyStyleTree(frame);
        }
      }
    } else if (!changeData->mFrame ||
               changeData->mFrame->GetType() != nsGkAtoms::viewportFrame) {
      NS_WARNING("Unable to test style tree integrity -- no content node "
                 "(and not a viewport frame)");
    }
#endif
  }

  aChangeList.Clear();
  return NS_OK;
}

void
RestyleManager::RestyleElement(Element*        aElement,
                               nsIFrame*       aPrimaryFrame,
                               nsChangeHint    aMinHint,
                               RestyleTracker& aRestyleTracker,
                               bool            aRestyleDescendants)
{
  NS_ASSERTION(aPrimaryFrame == aElement->GetPrimaryFrame(),
               "frame/content mismatch");
  if (aPrimaryFrame && aPrimaryFrame->GetContent() != aElement) {
    
    
    aPrimaryFrame = nullptr;
  }
  NS_ASSERTION(!aPrimaryFrame || aPrimaryFrame->GetContent() == aElement,
               "frame/content mismatch");

  
  
  if (mPresContext->UsesRootEMUnits() && aPrimaryFrame) {
    nsStyleContext *oldContext = aPrimaryFrame->StyleContext();
    if (!oldContext->GetParent()) { 
      nsRefPtr<nsStyleContext> newContext = mPresContext->StyleSet()->
        ResolveStyleFor(aElement, nullptr );
      if (oldContext->StyleFont()->mFont.size !=
          newContext->StyleFont()->mFont.size) {
        
        newContext = nullptr;
        DoRebuildAllStyleData(aRestyleTracker, nsChangeHint(0));
        if (aMinHint == 0) {
          return;
        }
        aPrimaryFrame = aElement->GetPrimaryFrame();
      }
    }
  }

  if (aMinHint & nsChangeHint_ReconstructFrame) {
    FrameConstructor()->RecreateFramesForContent(aElement, false);
  } else if (aPrimaryFrame) {
    nsStyleChangeList changeList;
    ComputeStyleChangeFor(aPrimaryFrame, &changeList, aMinHint,
                          aRestyleTracker, aRestyleDescendants);
    ProcessRestyledFrames(changeList);
  } else {
    
    FrameConstructor()->MaybeRecreateFramesForElement(aElement);
  }
}



nsresult
RestyleManager::ContentStateChanged(nsIContent* aContent,
                                    nsEventStates aStateMask)
{
  
  
  if (!aContent->IsElement()) {
    return NS_OK;
  }

  Element* aElement = aContent->AsElement();

  nsStyleSet* styleSet = mPresContext->StyleSet();
  NS_ASSERTION(styleSet, "couldn't get style set");

  nsChangeHint hint = NS_STYLE_HINT_NONE;
  
  
  
  
  
  
  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();
  if (primaryFrame) {
    
    if (!primaryFrame->IsGeneratedContentFrame() &&
        aStateMask.HasAtLeastOneOfStates(NS_EVENT_STATE_BROKEN |
                                         NS_EVENT_STATE_USERDISABLED |
                                         NS_EVENT_STATE_SUPPRESSED |
                                         NS_EVENT_STATE_LOADING)) {
      hint = nsChangeHint_ReconstructFrame;
    } else {
      uint8_t app = primaryFrame->StyleDisplay()->mAppearance;
      if (app) {
        nsITheme *theme = mPresContext->GetTheme();
        if (theme && theme->ThemeSupportsWidget(mPresContext,
                                                primaryFrame, app)) {
          bool repaint = false;
          theme->WidgetStateChanged(primaryFrame, app, nullptr, &repaint);
          if (repaint) {
            NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
          }
        }
      }
    }

    primaryFrame->ContentStatesChanged(aStateMask);
  }


  nsRestyleHint rshint =
    styleSet->HasStateDependentStyle(mPresContext, aElement, aStateMask);

  if (aStateMask.HasState(NS_EVENT_STATE_HOVER) && rshint != 0) {
    ++mHoverGeneration;
  }

  if (aStateMask.HasState(NS_EVENT_STATE_VISITED)) {
    
    
    
    NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
  }

  PostRestyleEvent(aElement, rshint, hint);
  return NS_OK;
}


void
RestyleManager::AttributeWillChange(Element* aElement,
                                    int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType)
{
  nsRestyleHint rshint =
    mPresContext->StyleSet()->HasAttributeDependentStyle(mPresContext,
                                                         aElement,
                                                         aAttribute,
                                                         aModType,
                                                         false);
  PostRestyleEvent(aElement, rshint, NS_STYLE_HINT_NONE);
}



void
RestyleManager::AttributeChanged(Element* aElement,
                                 int32_t aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 int32_t aModType)
{
  
  
  
  nsCOMPtr<nsIPresShell> shell = mPresContext->GetPresShell();

  
  nsIFrame* primaryFrame = aElement->GetPrimaryFrame();

#if 0
  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
     ("RestyleManager::AttributeChanged: content=%p[%s] frame=%p",
      aContent, ContentTag(aElement, 0), frame));
#endif

  
  nsChangeHint hint = aElement->GetAttributeChangeHint(aAttribute, aModType);

  bool reframe = (hint & nsChangeHint_ReconstructFrame) != 0;

#ifdef MOZ_XUL
  
  
  
  if (!primaryFrame && !reframe) {
    int32_t namespaceID;
    nsIAtom* tag = mPresContext->Document()->BindingManager()->
                     ResolveTag(aElement, &namespaceID);

    if (namespaceID == kNameSpaceID_XUL &&
        (tag == nsGkAtoms::listitem ||
         tag == nsGkAtoms::listcell))
      return;
  }

  if (aAttribute == nsGkAtoms::tooltiptext ||
      aAttribute == nsGkAtoms::tooltip)
  {
    nsIRootBox* rootBox = nsIRootBox::GetRootBox(mPresContext->GetPresShell());
    if (rootBox) {
      if (aModType == nsIDOMMutationEvent::REMOVAL)
        rootBox->RemoveTooltipSupport(aElement);
      if (aModType == nsIDOMMutationEvent::ADDITION)
        rootBox->AddTooltipSupport(aElement);
    }
  }

#endif 

  if (primaryFrame) {
    
    const nsStyleDisplay* disp = primaryFrame->StyleDisplay();
    if (disp->mAppearance) {
      nsITheme *theme = mPresContext->GetTheme();
      if (theme && theme->ThemeSupportsWidget(mPresContext, primaryFrame, disp->mAppearance)) {
        bool repaint = false;
        theme->WidgetStateChanged(primaryFrame, disp->mAppearance, aAttribute, &repaint);
        if (repaint)
          NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
      }
    }

    
    primaryFrame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    
    
    
    
  }

  
  
  nsRestyleHint rshint =
    mPresContext->StyleSet()->HasAttributeDependentStyle(mPresContext,
                                                         aElement,
                                                         aAttribute,
                                                         aModType,
                                                         true);

  PostRestyleEvent(aElement, rshint, hint);
}

void
RestyleManager::RestyleForEmptyChange(Element* aContainer)
{
  
  
  nsRestyleHint hint = eRestyle_Subtree;
  nsIContent* grandparent = aContainer->GetParent();
  if (grandparent &&
      (grandparent->GetFlags() & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS)) {
    hint = nsRestyleHint(hint | eRestyle_LaterSiblings);
  }
  PostRestyleEvent(aContainer, hint, NS_STYLE_HINT_NONE);
}

void
RestyleManager::RestyleForAppend(Element* aContainer,
                                 nsIContent* aFirstNewContent)
{
  NS_ASSERTION(aContainer, "must have container for append");
#ifdef DEBUG
  {
    for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
      NS_ASSERTION(!cur->IsRootOfAnonymousSubtree(),
                   "anonymous nodes should not be in child lists");
    }
  }
#endif
  uint32_t selectorFlags =
    aContainer->GetFlags() & (NODE_ALL_SELECTOR_FLAGS &
                              ~NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS);
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    bool wasEmpty = true; 
    for (nsIContent* cur = aContainer->GetFirstChild();
         cur != aFirstNewContent;
         cur = cur->GetNextSibling()) {
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(cur, true, false)) {
        wasEmpty = false;
        break;
      }
    }
    if (wasEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    for (nsIContent* cur = aFirstNewContent->GetPreviousSibling();
         cur;
         cur = cur->GetPreviousSibling()) {
      if (cur->IsElement()) {
        PostRestyleEvent(cur->AsElement(), eRestyle_Subtree, NS_STYLE_HINT_NONE);
        break;
      }
    }
  }
}




static void
RestyleSiblingsStartingWith(RestyleManager* aRestyleManager,
                            nsIContent* aStartingSibling )
{
  for (nsIContent *sibling = aStartingSibling; sibling;
       sibling = sibling->GetNextSibling()) {
    if (sibling->IsElement()) {
      aRestyleManager->
        PostRestyleEvent(sibling->AsElement(),
                         nsRestyleHint(eRestyle_Subtree | eRestyle_LaterSiblings),
                         NS_STYLE_HINT_NONE);
      break;
    }
  }
}







void
RestyleManager::RestyleForInsertOrChange(Element* aContainer,
                                         nsIContent* aChild)
{
  NS_ASSERTION(!aChild->IsRootOfAnonymousSubtree(),
               "anonymous nodes should not be in child lists");
  uint32_t selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    bool wasEmpty = true; 
    for (nsIContent* child = aContainer->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child == aChild)
        continue;
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, true, false)) {
        wasEmpty = false;
        break;
      }
    }
    if (wasEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS) {
    
    RestyleSiblingsStartingWith(this, aChild->GetNextSibling());
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    bool passedChild = false;
    for (nsIContent* content = aContainer->GetFirstChild();
         content;
         content = content->GetNextSibling()) {
      if (content == aChild) {
        passedChild = true;
        continue;
      }
      if (content->IsElement()) {
        if (passedChild) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    passedChild = false;
    for (nsIContent* content = aContainer->GetLastChild();
         content;
         content = content->GetPreviousSibling()) {
      if (content == aChild) {
        passedChild = true;
        continue;
      }
      if (content->IsElement()) {
        if (passedChild) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
  }
}

void
RestyleManager::RestyleForRemove(Element* aContainer,
                                 nsIContent* aOldChild,
                                 nsIContent* aFollowingSibling)
{
  if (aOldChild->IsRootOfAnonymousSubtree()) {
    
    
    
    NS_WARNING("anonymous nodes should not be in child lists (bug 439258)");
  }
  uint32_t selectorFlags =
    aContainer ? (aContainer->GetFlags() & NODE_ALL_SELECTOR_FLAGS) : 0;
  if (selectorFlags == 0)
    return;

  if (selectorFlags & NODE_HAS_EMPTY_SELECTOR) {
    
    bool isEmpty = true; 
    for (nsIContent* child = aContainer->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      
      
      
      
      if (nsStyleUtil::IsSignificantChild(child, true, false)) {
        isEmpty = false;
        break;
      }
    }
    if (isEmpty) {
      RestyleForEmptyChange(aContainer);
      return;
    }
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR) {
    PostRestyleEvent(aContainer, eRestyle_Subtree, NS_STYLE_HINT_NONE);
    
    return;
  }

  if (selectorFlags & NODE_HAS_SLOW_SELECTOR_LATER_SIBLINGS) {
    
    RestyleSiblingsStartingWith(this, aFollowingSibling);
  }

  if (selectorFlags & NODE_HAS_EDGE_CHILD_SELECTOR) {
    
    bool reachedFollowingSibling = false;
    for (nsIContent* content = aContainer->GetFirstChild();
         content;
         content = content->GetNextSibling()) {
      if (content == aFollowingSibling) {
        reachedFollowingSibling = true;
        
      }
      if (content->IsElement()) {
        if (reachedFollowingSibling) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree,
                           NS_STYLE_HINT_NONE);
        }
        break;
      }
    }
    
    reachedFollowingSibling = (aFollowingSibling == nullptr);
    for (nsIContent* content = aContainer->GetLastChild();
         content;
         content = content->GetPreviousSibling()) {
      if (content->IsElement()) {
        if (reachedFollowingSibling) {
          PostRestyleEvent(content->AsElement(), eRestyle_Subtree, NS_STYLE_HINT_NONE);
        }
        break;
      }
      if (content == aFollowingSibling) {
        reachedFollowingSibling = true;
      }
    }
  }
}

void
RestyleManager::RebuildAllStyleData(nsChangeHint aExtraHint)
{
  NS_ASSERTION(!(aExtraHint & nsChangeHint_ReconstructFrame),
               "Should not reconstruct the root of the frame tree.  "
               "Use ReconstructDocElementHierarchy instead.");

  mRebuildAllStyleData = false;
  NS_UpdateHint(aExtraHint, mRebuildAllExtraHint);
  mRebuildAllExtraHint = nsChangeHint(0);

  nsIPresShell* presShell = mPresContext->GetPresShell();
  if (!presShell || !presShell->GetRootFrame())
    return;

  
  nsRefPtr<nsViewManager> vm = presShell->GetViewManager();

  
  
  nsCOMPtr<nsIPresShell> kungFuDeathGrip(presShell);

  
  
  presShell->GetDocument()->FlushPendingNotifications(Flush_ContentAndNotify);

  nsAutoScriptBlocker scriptBlocker;

  mPresContext->SetProcessingRestyles(true);

  DoRebuildAllStyleData(mPendingRestyles, aExtraHint);

  mPresContext->SetProcessingRestyles(false);

  
  
  
  
  ProcessPendingRestyles();
}

void
RestyleManager::DoRebuildAllStyleData(RestyleTracker& aRestyleTracker,
                                      nsChangeHint aExtraHint)
{
  
  
  nsresult rv = mPresContext->StyleSet()->BeginReconstruct();
  if (NS_FAILED(rv)) {
    return;
  }

  
  
  
  
  
  
  nsStyleChangeList changeList;
  
  
  
  ComputeStyleChangeFor(mPresContext->PresShell()->GetRootFrame(),
                        &changeList, aExtraHint,
                        aRestyleTracker, true);
  
  ProcessRestyledFrames(changeList);
  FlushOverflowChangedTracker();

  
  
  
  
  
  mPresContext->StyleSet()->EndReconstruct();
}

void
RestyleManager::ProcessPendingRestyles()
{
  NS_PRECONDITION(mPresContext->Document(), "No document?  Pshaw!");
  NS_PRECONDITION(!nsContentUtils::IsSafeToRunScript(),
                  "Missing a script blocker!");

  
  NS_ABORT_IF_FALSE(!mPresContext->IsProcessingRestyles(),
                    "Nesting calls to ProcessPendingRestyles?");
  mPresContext->SetProcessingRestyles(true);

  
  
  
  
  
  if (nsLayoutUtils::AreAsyncAnimationsEnabled() &&
      mPendingRestyles.Count() > 0) {
    ++mAnimationGeneration;
    mPresContext->TransitionManager()->UpdateAllThrottledStyles();
  }

  mPendingRestyles.ProcessRestyles();

#ifdef DEBUG
  uint32_t oldPendingRestyleCount = mPendingRestyles.Count();
#endif

  
  
  
  
  
  
  
  
  mPresContext->SetProcessingAnimationStyleChange(true);
  mPendingAnimationRestyles.ProcessRestyles();
  mPresContext->SetProcessingAnimationStyleChange(false);

  mPresContext->SetProcessingRestyles(false);
  NS_POSTCONDITION(mPendingRestyles.Count() == oldPendingRestyleCount,
                   "We should not have posted new non-animation restyles while "
                   "processing animation restyles");

  if (mRebuildAllStyleData) {
    
    
    
    RebuildAllStyleData(nsChangeHint(0));
  }
}

void
RestyleManager::BeginProcessingRestyles()
{
  
  
  mPresContext->FrameConstructor()->BeginUpdate();

  mInStyleRefresh = true;
}

void
RestyleManager::EndProcessingRestyles()
{
  FlushOverflowChangedTracker();

  
  
  mInStyleRefresh = false;

  mPresContext->FrameConstructor()->EndUpdate();

#ifdef DEBUG
  mPresContext->PresShell()->VerifyStyleTree();
#endif
}

void
RestyleManager::PostRestyleEventCommon(Element* aElement,
                                       nsRestyleHint aRestyleHint,
                                       nsChangeHint aMinChangeHint,
                                       bool aForAnimation)
{
  if (MOZ_UNLIKELY(mPresContext->PresShell()->IsDestroying())) {
    return;
  }

  if (aRestyleHint == 0 && !aMinChangeHint) {
    
    return;
  }

  RestyleTracker& tracker =
    aForAnimation ? mPendingAnimationRestyles : mPendingRestyles;
  tracker.AddPendingRestyle(aElement, aRestyleHint, aMinChangeHint);

  PostRestyleEventInternal(false);
}

void
RestyleManager::PostRestyleEventInternal(bool aForLazyConstruction)
{
  
  
  
  bool inRefresh = !aForLazyConstruction && mInStyleRefresh;
  nsIPresShell* presShell = mPresContext->PresShell();
  if (!mObservingRefreshDriver && !inRefresh) {
    mObservingRefreshDriver = mPresContext->RefreshDriver()->
      AddStyleFlushObserver(presShell);
  }

  
  
  
  presShell->GetDocument()->SetNeedStyleFlush();
}

void
RestyleManager::PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint)
{
  NS_ASSERTION(!(aExtraHint & nsChangeHint_ReconstructFrame),
               "Should not reconstruct the root of the frame tree.  "
               "Use ReconstructDocElementHierarchy instead.");

  mRebuildAllStyleData = true;
  NS_UpdateHint(mRebuildAllExtraHint, aExtraHint);

  
  PostRestyleEventInternal(false);
}

#ifdef DEBUG
static void
DumpContext(nsIFrame* aFrame, nsStyleContext* aContext)
{
  if (aFrame) {
    fputs("frame: ", stdout);
    nsAutoString  name;
    aFrame->GetFrameName(name);
    fputs(NS_LossyConvertUTF16toASCII(name).get(), stdout);
    fprintf(stdout, " (%p)", static_cast<void*>(aFrame));
  }
  if (aContext) {
    fprintf(stdout, " style: %p ", static_cast<void*>(aContext));

    nsIAtom* pseudoTag = aContext->GetPseudo();
    if (pseudoTag) {
      nsAutoString  buffer;
      pseudoTag->ToString(buffer);
      fputs(NS_LossyConvertUTF16toASCII(buffer).get(), stdout);
      fputs(" ", stdout);
    }
    fputs("{}\n", stdout);
  }
}

static void
VerifySameTree(nsStyleContext* aContext1, nsStyleContext* aContext2)
{
  nsStyleContext* top1 = aContext1;
  nsStyleContext* top2 = aContext2;
  nsStyleContext* parent;
  for (;;) {
    parent = top1->GetParent();
    if (!parent)
      break;
    top1 = parent;
  }
  for (;;) {
    parent = top2->GetParent();
    if (!parent)
      break;
    top2 = parent;
  }
  NS_ASSERTION(top1 == top2,
               "Style contexts are not in the same style context tree");
}

static void
VerifyContextParent(nsPresContext* aPresContext, nsIFrame* aFrame,
                    nsStyleContext* aContext, nsStyleContext* aParentContext)
{
  
  if (!aContext) {
    aContext = aFrame->StyleContext();
  }

  if (!aParentContext) {
    
    
    

    
    nsIFrame* providerFrame = aFrame->GetParentStyleContextFrame();
    if (providerFrame)
      aParentContext = providerFrame->StyleContext();
    
  }

  NS_ASSERTION(aContext, "Failure to get required contexts");
  nsStyleContext* actualParentContext = aContext->GetParent();

  if (aParentContext) {
    if (aParentContext != actualParentContext) {
      DumpContext(aFrame, aContext);
      if (aContext == aParentContext) {
        NS_ERROR("Using parent's style context");
      }
      else {
        NS_ERROR("Wrong parent style context");
        fputs("Wrong parent style context: ", stdout);
        DumpContext(nullptr, actualParentContext);
        fputs("should be using: ", stdout);
        DumpContext(nullptr, aParentContext);
        VerifySameTree(actualParentContext, aParentContext);
        fputs("\n", stdout);
      }
    }

  }
  else {
    if (actualParentContext) {
      NS_ERROR("Have parent context and shouldn't");
      DumpContext(aFrame, aContext);
      fputs("Has parent context: ", stdout);
      DumpContext(nullptr, actualParentContext);
      fputs("Should be null\n\n", stdout);
    }
  }

  nsStyleContext* childStyleIfVisited = aContext->GetStyleIfVisited();
  
  
  
  if (childStyleIfVisited &&
      !((childStyleIfVisited->RuleNode() != aContext->RuleNode() &&
         childStyleIfVisited->GetParent() == aContext->GetParent()) ||
        childStyleIfVisited->GetParent() ==
          aContext->GetParent()->GetStyleIfVisited())) {
    NS_ERROR("Visited style has wrong parent");
    DumpContext(aFrame, aContext);
    fputs("\n", stdout);
  }
}

static void
VerifyStyleTree(nsPresContext* aPresContext, nsIFrame* aFrame,
                nsStyleContext* aParentContext)
{
  nsStyleContext*  context = aFrame->StyleContext();
  VerifyContextParent(aPresContext, aFrame, context, nullptr);

  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        if (nsGkAtoms::placeholderFrame == child->GetType()) {
          
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);

          
          do {
            VerifyStyleTree(aPresContext, outOfFlowFrame, nullptr);
          } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));

          
          
          VerifyContextParent(aPresContext, child, nullptr, nullptr);
        }
        else { 
          VerifyStyleTree(aPresContext, child, nullptr);
        }
      }
    }
  }

  
  int32_t contextIndex = 0;
  for (nsStyleContext* extraContext;
       (extraContext = aFrame->GetAdditionalStyleContext(contextIndex));
       ++contextIndex) {
    VerifyContextParent(aPresContext, aFrame, extraContext, context);
  }
}

void
RestyleManager::DebugVerifyStyleTree(nsIFrame* aFrame)
{
  if (aFrame) {
    nsStyleContext* context = aFrame->StyleContext();
    nsStyleContext* parentContext = context->GetParent();
    VerifyStyleTree(mPresContext, aFrame, parentContext);
  }
}

#endif 



static void
TryStartingTransition(nsPresContext *aPresContext, nsIContent *aContent,
                      nsStyleContext *aOldStyleContext,
                      nsRefPtr<nsStyleContext> *aNewStyleContext )
{
  if (!aContent || !aContent->IsElement()) {
    return;
  }

  
  
  
  
  
  
  nsCOMPtr<nsIStyleRule> coverRule =
    aPresContext->TransitionManager()->StyleContextChanged(
      aContent->AsElement(), aOldStyleContext, *aNewStyleContext);
  if (coverRule) {
    nsCOMArray<nsIStyleRule> rules;
    rules.AppendObject(coverRule);
    *aNewStyleContext = aPresContext->StyleSet()->
                          ResolveStyleByAddingRules(*aNewStyleContext, rules);
  }
}

static inline dom::Element*
ElementForStyleContext(nsIContent* aParentContent,
                       nsIFrame* aFrame,
                       nsCSSPseudoElements::Type aPseudoType)
{
  
  NS_PRECONDITION(aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement ||
                  aPseudoType == nsCSSPseudoElements::ePseudo_AnonBox ||
                  aPseudoType < nsCSSPseudoElements::ePseudo_PseudoElementCount,
                  "Unexpected pseudo");
  
  
  if (aPseudoType == nsCSSPseudoElements::ePseudo_NotPseudoElement) {
    return aFrame->GetContent()->AsElement();
  }

  if (aPseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
    return nullptr;
  }

  if (aPseudoType == nsCSSPseudoElements::ePseudo_firstLetter) {
    NS_ASSERTION(aFrame->GetType() == nsGkAtoms::letterFrame,
                 "firstLetter pseudoTag without a nsFirstLetterFrame");
    nsBlockFrame* block = nsBlockFrame::GetNearestAncestorBlock(aFrame);
    return block->GetContent()->AsElement();
  }

  nsIContent* content = aParentContent ? aParentContent : aFrame->GetContent();
  return content->AsElement();
}

static nsIFrame*
GetPrevContinuationWithPossiblySameStyle(nsIFrame* aFrame)
{
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame *prevContinuation = aFrame->GetPrevContinuation();
  if (!prevContinuation && (aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
    
    
    prevContinuation = static_cast<nsIFrame*>(
      aFrame->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
    if (prevContinuation) {
      prevContinuation = static_cast<nsIFrame*>(
        prevContinuation->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
    }
  }
  return prevContinuation;
}

nsresult
RestyleManager::ReparentStyleContext(nsIFrame* aFrame)
{
  if (nsGkAtoms::placeholderFrame == aFrame->GetType()) {
    
    nsIFrame* outOfFlow =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlow, "no out-of-flow frame");
    do {
      ReparentStyleContext(outOfFlow);
    } while ((outOfFlow = outOfFlow->GetNextContinuation()));
  }

  
  
  nsStyleContext* oldContext = aFrame->StyleContext();

  nsRefPtr<nsStyleContext> newContext;
  nsIFrame* providerFrame = aFrame->GetParentStyleContextFrame();
  bool isChild = providerFrame && providerFrame->GetParent() == aFrame;
  nsStyleContext* newParentContext = nullptr;
  nsIFrame* providerChild = nullptr;
  if (isChild) {
    ReparentStyleContext(providerFrame);
    newParentContext = providerFrame->StyleContext();
    providerChild = providerFrame;
  } else if (providerFrame) {
    newParentContext = providerFrame->StyleContext();
  } else {
    NS_NOTREACHED("Reparenting something that has no usable parent? "
                  "Shouldn't happen!");
  }
  
  
  
  
  

#ifdef DEBUG
  {
    
    
    
    
    
    
    
    
    nsIFrame *nextContinuation = aFrame->GetNextContinuation();
    if (nextContinuation) {
      nsStyleContext *nextContinuationContext =
        nextContinuation->StyleContext();
      NS_ASSERTION(oldContext == nextContinuationContext ||
                   oldContext->GetPseudo() !=
                     nextContinuationContext->GetPseudo() ||
                   oldContext->GetParent() !=
                     nextContinuationContext->GetParent(),
                   "continuations should have the same style context");
    }
  }
#endif

  nsIFrame *prevContinuation =
    GetPrevContinuationWithPossiblySameStyle(aFrame);
  nsStyleContext *prevContinuationContext;
  bool copyFromContinuation =
    prevContinuation &&
    (prevContinuationContext = prevContinuation->StyleContext())
      ->GetPseudo() == oldContext->GetPseudo() &&
     prevContinuationContext->GetParent() == newParentContext;
  if (copyFromContinuation) {
    
    
    
    
    newContext = prevContinuationContext;
  } else {
    nsIFrame* parentFrame = aFrame->GetParent();
    Element* element =
      ElementForStyleContext(parentFrame ? parentFrame->GetContent() : nullptr,
                             aFrame,
                             oldContext->GetPseudoType());
    newContext = mPresContext->StyleSet()->
                   ReparentStyleContext(oldContext, newParentContext, element);
  }

  if (newContext) {
    if (newContext != oldContext) {
      
      
      
      
      
#if 0
      if (!copyFromContinuation) {
        TryStartingTransition(mPresContext, aFrame->GetContent(),
                              oldContext, &newContext);
      }
#endif

      
      
      DebugOnly<nsChangeHint> styleChange =
        oldContext->CalcStyleDifference(newContext, nsChangeHint(0));
      
      
      
      
      NS_ASSERTION(!(styleChange & nsChangeHint_ReconstructFrame),
                   "Our frame tree is likely to be bogus!");

      aFrame->SetStyleContext(newContext);

      nsIFrame::ChildListIterator lists(aFrame);
      for (; !lists.IsDone(); lists.Next()) {
        nsFrameList::Enumerator childFrames(lists.CurrentList());
        for (; !childFrames.AtEnd(); childFrames.Next()) {
          nsIFrame* child = childFrames.get();
          
          if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
              child != providerChild) {
#ifdef DEBUG
            if (nsGkAtoms::placeholderFrame == child->GetType()) {
              nsIFrame* outOfFlowFrame =
                nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
              NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

              NS_ASSERTION(outOfFlowFrame != providerChild,
                           "Out of flow provider?");
            }
#endif
            ReparentStyleContext(child);
          }
        }
      }

      
      
      
      
      
      
      if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
          !aFrame->GetPrevContinuation()) {
        nsIFrame* sib = static_cast<nsIFrame*>
          (aFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
        if (sib) {
          ReparentStyleContext(sib);
        }
      }

      
      int32_t contextIndex = 0;
      for (nsStyleContext* oldExtraContext;
           (oldExtraContext = aFrame->GetAdditionalStyleContext(contextIndex));
           ++contextIndex) {
        nsRefPtr<nsStyleContext> newExtraContext;
        newExtraContext = mPresContext->StyleSet()->
                            ReparentStyleContext(oldExtraContext,
                                                 newContext, nullptr);
        if (newExtraContext) {
          if (newExtraContext != oldExtraContext) {
            
            
            
            styleChange =
              oldExtraContext->CalcStyleDifference(newExtraContext,
                                                   nsChangeHint(0));
            
            
            
            
            
            NS_ASSERTION(!(styleChange & nsChangeHint_ReconstructFrame),
                         "Our frame tree is likely to be bogus!");
          }

          aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
        }
      }
#ifdef DEBUG
      VerifyStyleTree(mPresContext, aFrame, newParentContext);
#endif
    }
  }

  return NS_OK;
}

ElementRestyler::ElementRestyler(nsPresContext* aPresContext,
                                 nsIFrame* aFrame,
                                 nsStyleChangeList* aChangeList,
                                 nsChangeHint aHintsHandledByAncestors,
                                 RestyleTracker& aRestyleTracker,
                                 TreeMatchContext& aTreeMatchContext,
                                 nsTArray<nsIContent*>&
                                   aVisibleKidsOfHiddenElement)
  : mPresContext(aPresContext)
  , mFrame(aFrame)
  , mParentContent(nullptr)
    
    
  , mContent(mFrame->GetContent() ? mFrame->GetContent() : mParentContent)
  , mChangeList(aChangeList)
  , mHintsHandled(NS_SubtractHint(aHintsHandledByAncestors,
                  NS_HintsNotHandledForDescendantsIn(aHintsHandledByAncestors)))
  , mParentFrameHintsNotHandledForDescendants(nsChangeHint(0))
  , mHintsNotHandledForDescendants(nsChangeHint(0))
  , mRestyleTracker(aRestyleTracker)
  , mTreeMatchContext(aTreeMatchContext)
  , mResolvedChild(nullptr)
#ifdef ACCESSIBILITY
  , mDesiredA11yNotifications(eSendAllNotifications)
  , mKidsDesiredA11yNotifications(mDesiredA11yNotifications)
  , mOurA11yNotification(eDontNotify)
  , mVisibleKidsOfHiddenElement(aVisibleKidsOfHiddenElement)
#endif
{
}

ElementRestyler::ElementRestyler(const ElementRestyler& aParentRestyler,
                                 nsIFrame* aFrame,
                                 uint32_t aConstructorFlags)
  : mPresContext(aParentRestyler.mPresContext)
  , mFrame(aFrame)
  , mParentContent(aParentRestyler.mContent)
    
    
  , mContent(mFrame->GetContent() ? mFrame->GetContent() : mParentContent)
  , mChangeList(aParentRestyler.mChangeList)
  , mHintsHandled(NS_SubtractHint(aParentRestyler.mHintsHandled,
                  NS_HintsNotHandledForDescendantsIn(aParentRestyler.mHintsHandled)))
  , mParentFrameHintsNotHandledForDescendants(
      aParentRestyler.mHintsNotHandledForDescendants)
  , mHintsNotHandledForDescendants(nsChangeHint(0))
  , mRestyleTracker(aParentRestyler.mRestyleTracker)
  , mTreeMatchContext(aParentRestyler.mTreeMatchContext)
  , mResolvedChild(nullptr)
#ifdef ACCESSIBILITY
  , mDesiredA11yNotifications(aParentRestyler.mKidsDesiredA11yNotifications)
  , mKidsDesiredA11yNotifications(mDesiredA11yNotifications)
  , mOurA11yNotification(eDontNotify)
  , mVisibleKidsOfHiddenElement(aParentRestyler.mVisibleKidsOfHiddenElement)
#endif
{
  if (aConstructorFlags & FOR_OUT_OF_FLOW_CHILD) {
    mHintsHandled = NS_SubtractHint(mHintsHandled, nsChangeHint_AllReflowHints);
  }
}

ElementRestyler::ElementRestyler(ParentContextFromChildFrame,
                                 const ElementRestyler& aParentRestyler,
                                 nsIFrame* aFrame)
  : mPresContext(aParentRestyler.mPresContext)
  , mFrame(aFrame)
  , mParentContent(aParentRestyler.mParentContent)
    
    
  , mContent(mFrame->GetContent() ? mFrame->GetContent() : mParentContent)
  , mChangeList(aParentRestyler.mChangeList)
  , mHintsHandled(NS_SubtractHint(aParentRestyler.mHintsHandled,
                  NS_HintsNotHandledForDescendantsIn(aParentRestyler.mHintsHandled)))
  , mParentFrameHintsNotHandledForDescendants(
      
      nsChangeHint_Hints_NotHandledForDescendants)
  , mHintsNotHandledForDescendants(nsChangeHint(0))
  , mRestyleTracker(aParentRestyler.mRestyleTracker)
  , mTreeMatchContext(aParentRestyler.mTreeMatchContext)
  , mResolvedChild(nullptr)
#ifdef ACCESSIBILITY
  , mDesiredA11yNotifications(aParentRestyler.mDesiredA11yNotifications)
  , mKidsDesiredA11yNotifications(mDesiredA11yNotifications)
  , mOurA11yNotification(eDontNotify)
  , mVisibleKidsOfHiddenElement(aParentRestyler.mVisibleKidsOfHiddenElement)
#endif
{
}

void
ElementRestyler::CaptureChange(nsStyleContext* aOldContext,
                               nsStyleContext* aNewContext,
                               nsChangeHint aChangeToAssume)
{
  
  NS_ASSERTION(aOldContext->GetPseudo() == aNewContext->GetPseudo(),
               "old and new style contexts should have the same pseudo");
  NS_ASSERTION(aOldContext->GetPseudoType() == aNewContext->GetPseudoType(),
               "old and new style contexts should have the same pseudo");

  nsChangeHint ourChange = aOldContext->CalcStyleDifference(aNewContext,
                             mParentFrameHintsNotHandledForDescendants);
  NS_ASSERTION(!(ourChange & nsChangeHint_AllReflowHints) ||
               (ourChange & nsChangeHint_NeedReflow),
               "Reflow hint bits set without actually asking for a reflow");

  
  
  
  if ((ourChange & nsChangeHint_UpdateEffects) &&
      mContent && !mContent->IsElement()) {
    ourChange = NS_SubtractHint(ourChange, nsChangeHint_UpdateEffects);
  }

  NS_UpdateHint(ourChange, aChangeToAssume);
  if (NS_UpdateHint(mHintsHandled, ourChange)) {
    if (!(ourChange & nsChangeHint_ReconstructFrame) || mContent) {
      mChangeList->AppendChange(mFrame, mContent, ourChange);
    }
  }
  NS_UpdateHint(mHintsNotHandledForDescendants,
                NS_HintsNotHandledForDescendantsIn(ourChange));
}









void
ElementRestyler::Restyle(nsRestyleHint aRestyleHint)
{
  
  
  
  
  
  
  
  
  NS_ASSERTION(mFrame->GetContent() || !mParentContent ||
               !mParentContent->GetParent(),
               "frame must have content (unless at the top of the tree)");

  if (mContent && mContent->IsElement()) {
    mContent->OwnerDoc()->FlushPendingLinkUpdates();
    RestyleTracker::RestyleData restyleData;
    if (mRestyleTracker.GetRestyleData(mContent->AsElement(), &restyleData)) {
      if (NS_UpdateHint(mHintsHandled, restyleData.mChangeHint)) {
        mChangeList->AppendChange(mFrame, mContent, restyleData.mChangeHint);
      }
      aRestyleHint = nsRestyleHint(aRestyleHint | restyleData.mRestyleHint);
    }
  }

  nsRestyleHint childRestyleHint = aRestyleHint;

  if (childRestyleHint == eRestyle_Self) {
    childRestyleHint = nsRestyleHint(0);
  }

  RestyleSelf(aRestyleHint);

  RestyleChildren(childRestyleHint);
}

void
ElementRestyler::RestyleSelf(nsRestyleHint aRestyleHint)
{
  
  
  

  
  
  

  nsChangeHint assumeDifferenceHint = NS_STYLE_HINT_NONE;
  nsRefPtr<nsStyleContext> oldContext = mFrame->StyleContext();
  nsStyleSet* styleSet = mPresContext->StyleSet();

#ifdef ACCESSIBILITY
  mWasFrameVisible = nsIPresShell::IsAccessibilityActive() ?
    oldContext->StyleVisibility()->IsVisible() : false;
#endif

  nsIAtom* const pseudoTag = oldContext->GetPseudo();
  const nsCSSPseudoElements::Type pseudoType = oldContext->GetPseudoType();

  nsStyleContext* parentContext;
  
  
  nsIFrame* providerFrame = mFrame->GetParentStyleContextFrame();
  bool isChild = providerFrame && providerFrame->GetParent() == mFrame;
  if (!isChild) {
    if (providerFrame)
      parentContext = providerFrame->StyleContext();
    else
      parentContext = nullptr;
  }
  else {
    MOZ_ASSERT(providerFrame->GetContent() == mFrame->GetContent(),
               "Postcondition for GetParentStyleContextFrame() violated. "
               "That means we need to add the current element to the "
               "ancestor filter.");

    

    
    
    
    
    
    

    ElementRestyler providerRestyler(PARENT_CONTEXT_FROM_CHILD_FRAME,
                                     *this, providerFrame);
    providerRestyler.Restyle(aRestyleHint);
    assumeDifferenceHint = providerRestyler.HintsHandledForFrame();

    
    
    parentContext = providerFrame->StyleContext();
    
    
    mResolvedChild = providerFrame;
  }

  if (providerFrame != mFrame->GetParent()) {
    
    
    mParentFrameHintsNotHandledForDescendants =
      nsChangeHint_Hints_NotHandledForDescendants;
  }

#ifdef DEBUG
  {
    
    
    
    
    
    
    
    
    nsIFrame *nextContinuation = mFrame->GetNextContinuation();
    if (nextContinuation) {
      nsStyleContext *nextContinuationContext =
        nextContinuation->StyleContext();
      NS_ASSERTION(oldContext == nextContinuationContext ||
                   oldContext->GetPseudo() !=
                     nextContinuationContext->GetPseudo() ||
                   oldContext->GetParent() !=
                     nextContinuationContext->GetParent(),
                   "continuations should have the same style context");
    }
    
    
    
    if ((mFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) &&
        !mFrame->GetPrevContinuation()) {
      nsIFrame *nextIBSibling = static_cast<nsIFrame*>(
        mFrame->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
      if (nextIBSibling) {
        nextIBSibling = static_cast<nsIFrame*>(
          nextIBSibling->Properties().Get(nsIFrame::IBSplitSpecialSibling()));
      }
      if (nextIBSibling) {
        nsStyleContext *nextIBSiblingContext =
          nextIBSibling->StyleContext();
        NS_ASSERTION(oldContext == nextIBSiblingContext ||
                     oldContext->GetPseudo() !=
                       nextIBSiblingContext->GetPseudo() ||
                     oldContext->GetParent() !=
                       nextIBSiblingContext->GetParent(),
                     "continuations should have the same style context");
      }
    }
  }
#endif

  
  nsRefPtr<nsStyleContext> newContext;
  nsIFrame *prevContinuation =
    GetPrevContinuationWithPossiblySameStyle(mFrame);
  nsStyleContext *prevContinuationContext;
  bool copyFromContinuation =
    prevContinuation &&
    (prevContinuationContext = prevContinuation->StyleContext())
      ->GetPseudo() == oldContext->GetPseudo() &&
     prevContinuationContext->GetParent() == parentContext;
  if (copyFromContinuation) {
    
    
    
    
    newContext = prevContinuationContext;
  }
  else if (pseudoTag == nsCSSAnonBoxes::mozNonElement) {
    NS_ASSERTION(mFrame->GetContent(),
                 "non pseudo-element frame without content node");
    newContext = styleSet->ResolveStyleForNonElement(parentContext);
  }
  else if (!aRestyleHint && !prevContinuation) {
    
    
    
    
    
    
    
    
    newContext =
      styleSet->ReparentStyleContext(oldContext, parentContext,
                                     ElementForStyleContext(mParentContent,
                                                            mFrame,
                                                            pseudoType));
  } else if (pseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
    newContext = styleSet->ResolveAnonymousBoxStyle(pseudoTag,
                                                    parentContext);
  }
  else {
    Element* element = ElementForStyleContext(mParentContent,
                                              mFrame,
                                              pseudoType);
    if (pseudoTag) {
      if (pseudoTag == nsCSSPseudoElements::before ||
          pseudoTag == nsCSSPseudoElements::after) {
        
        newContext = styleSet->ProbePseudoElementStyle(element,
                                                       pseudoType,
                                                       parentContext,
                                                       mTreeMatchContext);
        if (!newContext) {
          
          NS_UpdateHint(mHintsHandled, nsChangeHint_ReconstructFrame);
          mChangeList->AppendChange(mFrame, element,
                                    nsChangeHint_ReconstructFrame);
          
          newContext = oldContext;
        }
      } else {
        
        
        NS_ASSERTION(pseudoType <
                       nsCSSPseudoElements::ePseudo_PseudoElementCount,
                     "Unexpected pseudo type");
        newContext = styleSet->ResolvePseudoElementStyle(element,
                                                         pseudoType,
                                                         parentContext);
      }
    }
    else {
      NS_ASSERTION(mFrame->GetContent(),
                   "non pseudo-element frame without content node");
      
      TreeMatchContext::AutoFlexItemStyleFixupSkipper
        flexFixupSkipper(mTreeMatchContext,
                         element->IsRootOfNativeAnonymousSubtree());
      newContext = styleSet->ResolveStyleFor(element, parentContext,
                                             mTreeMatchContext);
    }
  }

  MOZ_ASSERT(newContext);

  if (!parentContext) {
    if (oldContext->RuleNode() == newContext->RuleNode() &&
        oldContext->IsLinkContext() == newContext->IsLinkContext() &&
        oldContext->RelevantLinkVisited() ==
          newContext->RelevantLinkVisited()) {
      
      
      
      
      
      newContext = oldContext;
    }
  }

  if (newContext != oldContext) {
    if (!copyFromContinuation) {
      TryStartingTransition(mPresContext, mFrame->GetContent(),
                            oldContext, &newContext);
    }

    CaptureChange(oldContext, newContext, assumeDifferenceHint);
    if (!(mHintsHandled & nsChangeHint_ReconstructFrame)) {
      
      mFrame->SetStyleContext(newContext);
    }
  }
  oldContext = nullptr;

  
  
  
  int32_t contextIndex = 0;
  for (nsStyleContext* oldExtraContext;
       (oldExtraContext = mFrame->GetAdditionalStyleContext(contextIndex));
       ++contextIndex) {
    nsRefPtr<nsStyleContext> newExtraContext;
    nsIAtom* const extraPseudoTag = oldExtraContext->GetPseudo();
    const nsCSSPseudoElements::Type extraPseudoType =
      oldExtraContext->GetPseudoType();
    NS_ASSERTION(extraPseudoTag &&
                 extraPseudoTag != nsCSSAnonBoxes::mozNonElement,
                 "extra style context is not pseudo element");
    if (extraPseudoType == nsCSSPseudoElements::ePseudo_AnonBox) {
      newExtraContext = styleSet->ResolveAnonymousBoxStyle(extraPseudoTag,
                                                           newContext);
    }
    else {
      
      
      NS_ASSERTION(extraPseudoType <
                     nsCSSPseudoElements::ePseudo_PseudoElementCount,
                   "Unexpected type");
      newExtraContext = styleSet->ResolvePseudoElementStyle(mContent->AsElement(),
                                                            extraPseudoType,
                                                            newContext);
    }

    MOZ_ASSERT(newExtraContext);

    if (oldExtraContext != newExtraContext) {
      CaptureChange(oldExtraContext, newExtraContext, assumeDifferenceHint);
      if (!(mHintsHandled & nsChangeHint_ReconstructFrame)) {
        mFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
      }
    }
  }
}

void
ElementRestyler::RestyleChildren(nsRestyleHint aChildRestyleHint)
{
  RestyleUndisplayedChildren(aChildRestyleHint);

  
  
  
  
  
  
  
  if (!(mHintsHandled & nsChangeHint_ReconstructFrame) &&
      aChildRestyleHint) {
    RestyleBeforePseudo();
  }

  
  
  if (!(mHintsHandled & nsChangeHint_ReconstructFrame) &&
      aChildRestyleHint) {
    RestyleAfterPseudo();
  }

  
  
  
  
  
  
  
  
  
  if (!(mHintsHandled & nsChangeHint_ReconstructFrame)) {
    InitializeAccessibilityNotifications();

    RestyleContentChildren(aChildRestyleHint);

    SendAccessibilityNotifications();
  }
}

void
ElementRestyler::RestyleUndisplayedChildren(nsRestyleHint aChildRestyleHint)
{
  
  
  
  bool checkUndisplayed;
  nsIContent* undisplayedParent;
  nsCSSFrameConstructor* frameConstructor = mPresContext->FrameConstructor();
  if (mFrame->StyleContext()->GetPseudo()) {
    checkUndisplayed = mFrame == frameConstructor->
                                   GetDocElementContainingBlock();
    undisplayedParent = nullptr;
  } else {
    checkUndisplayed = !!mFrame->GetContent();
    undisplayedParent = mFrame->GetContent();
  }
  if (checkUndisplayed &&
      
      
      
      
      
      !(mHintsHandled & nsChangeHint_ReconstructFrame)) {
    UndisplayedNode* undisplayed =
      frameConstructor->GetAllUndisplayedContentIn(undisplayedParent);
    for (TreeMatchContext::AutoAncestorPusher
           pushAncestor(undisplayed, mTreeMatchContext,
                        undisplayedParent ? undisplayedParent->AsElement()
                                          : nullptr);
         undisplayed; undisplayed = undisplayed->mNext) {
      NS_ASSERTION(undisplayedParent ||
                   undisplayed->mContent ==
                     mPresContext->Document()->GetRootElement(),
                   "undisplayed node child of null must be root");
      NS_ASSERTION(!undisplayed->mStyle->GetPseudo(),
                   "Shouldn't have random pseudo style contexts in the "
                   "undisplayed map");

      
      
      
      nsIContent* parent = undisplayed->mContent->GetParent();
      bool pushInsertionPoint = parent && parent->IsActiveChildrenElement();
      TreeMatchContext::AutoAncestorPusher
        insertionPointPusher(pushInsertionPoint,
                             mTreeMatchContext,
                             parent && parent->IsElement() ? parent->AsElement() : nullptr);

      nsRestyleHint thisChildHint = aChildRestyleHint;
      RestyleTracker::RestyleData undisplayedRestyleData;
      if (mRestyleTracker.GetRestyleData(undisplayed->mContent->AsElement(),
                                         &undisplayedRestyleData)) {
        thisChildHint =
          nsRestyleHint(thisChildHint | undisplayedRestyleData.mRestyleHint);
      }
      nsRefPtr<nsStyleContext> undisplayedContext;
      nsStyleSet* styleSet = mPresContext->StyleSet();
      if (thisChildHint) {
        undisplayedContext =
          styleSet->ResolveStyleFor(undisplayed->mContent->AsElement(),
                                    mFrame->StyleContext(),
                                    mTreeMatchContext);
      } else {
        undisplayedContext =
          styleSet->ReparentStyleContext(undisplayed->mStyle,
                                         mFrame->StyleContext(),
                                         undisplayed->mContent->AsElement());
      }
      const nsStyleDisplay* display = undisplayedContext->StyleDisplay();
      if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
        NS_ASSERTION(undisplayed->mContent,
                     "Must have undisplayed content");
        mChangeList->AppendChange(nullptr, undisplayed->mContent,
                                  NS_STYLE_HINT_FRAMECHANGE);
        
        
      } else {
        
        undisplayed->mStyle = undisplayedContext;
      }
    }
  }
}

void
ElementRestyler::RestyleBeforePseudo()
{
  
  
  if (!mFrame->StyleContext()->GetPseudo() &&
      ((mFrame->GetStateBits() & NS_FRAME_MAY_HAVE_GENERATED_CONTENT) ||
       
       (mFrame->GetContentInsertionFrame()->GetStateBits() &
        NS_FRAME_MAY_HAVE_GENERATED_CONTENT))) {
    
    
    nsIFrame* prevContinuation = mFrame->GetPrevContinuation();
    if (!prevContinuation) {
      
      
      if (!nsLayoutUtils::GetBeforeFrame(mFrame) &&
          nsLayoutUtils::HasPseudoStyle(mFrame->GetContent(),
                                        mFrame->StyleContext(),
                                        nsCSSPseudoElements::ePseudo_before,
                                        mPresContext)) {
        
        NS_UpdateHint(mHintsHandled, nsChangeHint_ReconstructFrame);
        mChangeList->AppendChange(mFrame, mContent,
                                  nsChangeHint_ReconstructFrame);
      }
    }
  }
}

void
ElementRestyler::RestyleAfterPseudo()
{
  
  
  if (!mFrame->StyleContext()->GetPseudo() &&
      ((mFrame->GetStateBits() & NS_FRAME_MAY_HAVE_GENERATED_CONTENT) ||
       
       (mFrame->GetContentInsertionFrame()->GetStateBits() &
        NS_FRAME_MAY_HAVE_GENERATED_CONTENT))) {
    
    
    nsIFrame* nextContinuation = mFrame->GetNextContinuation();

    if (!nextContinuation) {
      
      
      if (nsLayoutUtils::HasPseudoStyle(mFrame->GetContent(),
                                        mFrame->StyleContext(),
                                        nsCSSPseudoElements::ePseudo_after,
                                        mPresContext) &&
          !nsLayoutUtils::GetAfterFrame(mFrame)) {
        
        NS_UpdateHint(mHintsHandled, nsChangeHint_ReconstructFrame);
        mChangeList->AppendChange(mFrame, mContent,
                                  nsChangeHint_ReconstructFrame);
      }
    }
  }
}

void
ElementRestyler::InitializeAccessibilityNotifications()
{
#ifdef ACCESSIBILITY
  
  
  
  if (nsIPresShell::IsAccessibilityActive() &&
      !mFrame->GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(mFrame)) {
    if (mDesiredA11yNotifications == eSendAllNotifications) {
      bool isFrameVisible = mFrame->StyleVisibility()->IsVisible();
      if (isFrameVisible != mWasFrameVisible) {
        if (isFrameVisible) {
          
          
          
          mKidsDesiredA11yNotifications = eSkipNotifications;
          mOurA11yNotification = eNotifyShown;
        } else {
          
          
          
          
          
          mKidsDesiredA11yNotifications = eNotifyIfShown;
          mOurA11yNotification = eNotifyHidden;
        }
      }
    } else if (mDesiredA11yNotifications == eNotifyIfShown &&
               mFrame->StyleVisibility()->IsVisible()) {
      
      
      mVisibleKidsOfHiddenElement.AppendElement(mFrame->GetContent());
      mKidsDesiredA11yNotifications = eSkipNotifications;
    }
  }
#endif
}

void
ElementRestyler::RestyleContentChildren(nsRestyleHint aChildRestyleHint)
{
  nsIFrame::ChildListIterator lists(mFrame);
  for (TreeMatchContext::AutoAncestorPusher
         pushAncestor(!lists.IsDone(),
                      mTreeMatchContext,
                      mContent && mContent->IsElement()
                        ? mContent->AsElement() : nullptr);
       !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (!(child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
        
        
        
        

        
        
        nsIContent* parent = child->GetContent() ? child->GetContent()->GetParent() : nullptr;
        bool pushInsertionPoint = parent && parent->IsActiveChildrenElement();
        TreeMatchContext::AutoAncestorPusher
          insertionPointPusher(pushInsertionPoint, mTreeMatchContext,
                               parent && parent->IsElement() ? parent->AsElement() : nullptr);

        
        if (nsGkAtoms::placeholderFrame == child->GetType()) { 
          
          nsIFrame* outOfFlowFrame =
            nsPlaceholderFrame::GetRealFrameForPlaceholder(child);
          NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");
          NS_ASSERTION(outOfFlowFrame != mResolvedChild,
                       "out-of-flow frame not a true descendant");

          
          
          
          
          
          
          
          
          
          

          
          
          do {
            ElementRestyler oofRestyler(*this, outOfFlowFrame,
                                        FOR_OUT_OF_FLOW_CHILD);
            oofRestyler.Restyle(aChildRestyleHint);
          } while ((outOfFlowFrame = outOfFlowFrame->GetNextContinuation()));

          
          
          ElementRestyler phRestyler(*this, child, 0);
          phRestyler.Restyle(aChildRestyleHint);
        }
        else {  
          if (child != mResolvedChild) {
            ElementRestyler childRestyler(*this, child, 0);
            childRestyler.Restyle(aChildRestyleHint);
          }
        }
      }
    }
  }
  
}

void
ElementRestyler::SendAccessibilityNotifications()
{
#ifdef ACCESSIBILITY
  
  if (mOurA11yNotification == eNotifyShown) {
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      nsIPresShell* presShell = mFrame->PresContext()->GetPresShell();
      nsIContent* content = mFrame->GetContent();

      accService->ContentRangeInserted(presShell, content->GetParent(),
                                       content,
                                       content->GetNextSibling());
    }
  } else if (mOurA11yNotification == eNotifyHidden) {
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      nsIPresShell* presShell = mFrame->PresContext()->GetPresShell();
      nsIContent* content = mFrame->GetContent();
      accService->ContentRemoved(presShell, content->GetParent(), content);

      
      uint32_t visibleContentCount = mVisibleKidsOfHiddenElement.Length();
      for (uint32_t idx = 0; idx < visibleContentCount; idx++) {
        nsIContent* childContent = mVisibleKidsOfHiddenElement[idx];
        accService->ContentRangeInserted(presShell, childContent->GetParent(),
                                         childContent,
                                         childContent->GetNextSibling());
      }
      mVisibleKidsOfHiddenElement.Clear();
    }
  }
#endif
}

void
RestyleManager::ComputeStyleChangeFor(nsIFrame*          aFrame,
                                      nsStyleChangeList* aChangeList,
                                      nsChangeHint       aMinChange,
                                      RestyleTracker&    aRestyleTracker,
                                      bool               aRestyleDescendants)
{
  PROFILER_LABEL("CSS", "ComputeStyleChangeFor");

  nsIContent *content = aFrame->GetContent();
  if (aMinChange) {
    aChangeList->AppendChange(aFrame, content, aMinChange);
  }

  nsIFrame* frame = aFrame;
  nsIFrame* frame2 = aFrame;

  NS_ASSERTION(!frame->GetPrevContinuation(), "must start with the first in flow");

  
  
  

  FramePropertyTable* propTable = mPresContext->PropertyTable();

  TreeMatchContext treeMatchContext(true,
                                    nsRuleWalker::eRelevantLinkUnvisited,
                                    mPresContext->Document());
  nsIContent *parent = content ? content->GetParent() : nullptr;
  Element *parentElement =
    parent && parent->IsElement() ? parent->AsElement() : nullptr;
  treeMatchContext.InitAncestors(parentElement);
  nsTArray<nsIContent*> visibleKidsOfHiddenElement;
  do {
    
    do {
      
      ElementRestyler restyler(mPresContext, frame, aChangeList,
                               aMinChange, aRestyleTracker,
                               treeMatchContext,
                               visibleKidsOfHiddenElement);

      restyler.Restyle(aRestyleDescendants ? eRestyle_Subtree : eRestyle_Self);

      if (restyler.HintsHandledForFrame() & nsChangeHint_ReconstructFrame) {
        
        
        
        NS_ASSERTION(!frame->GetPrevContinuation(),
                     "continuing frame had more severe impact than first-in-flow");
        return;
      }

      frame = frame->GetNextContinuation();
    } while (frame);

    
    if (!(frame2->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
      
      return;
    }

    frame2 = static_cast<nsIFrame*>
      (propTable->Get(frame2, nsIFrame::IBSplitSpecialSibling()));
    frame = frame2;
  } while (frame2);
}

} 
