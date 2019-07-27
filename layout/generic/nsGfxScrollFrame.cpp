






#include "nsGfxScrollFrame.h"

#include "base/compiler_specific.h"
#include "DisplayItemClip.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsView.h"
#include "nsIScrollable.h"
#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsContentList.h"
#include "nsIDocumentInlines.h"
#include "nsFontMetrics.h"
#include "nsBoxLayoutState.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsITextControlFrame.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsAutoPtr.h"
#include "nsPresState.h"
#include "nsIHTMLDocument.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsBidiPresUtils.h"
#include "nsBidiUtils.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/dom/Element.h"
#include <stdint.h>
#include "mozilla/MathAlgorithms.h"
#include "FrameLayerBuilder.h"
#include "nsSMILKeySpline.h"
#include "nsSubDocumentFrame.h"
#include "nsSVGOuterSVGFrame.h"
#include "mozilla/Attributes.h"
#include "ScrollbarActivity.h"
#include "nsRefreshDriver.h"
#include "nsThemeConstants.h"
#include "nsSVGIntegrationUtils.h"
#include "nsIScrollPositionListener.h"
#include "StickyScrollContainer.h"
#include "nsIFrameInlines.h"
#include "gfxPlatform.h"
#include "gfxPrefs.h"
#include "AsyncScrollBase.h"
#include <mozilla/layers/AxisPhysicsModel.h>
#include <mozilla/layers/AxisPhysicsMSDModel.h>
#include <algorithm>
#include <cstdlib> 
#include <cmath> 

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layout;

static uint32_t
GetOverflowChange(const nsRect& aCurScrolledRect, const nsRect& aPrevScrolledRect)
{
  uint32_t result = 0;
  if (aPrevScrolledRect.x != aCurScrolledRect.x ||
      aPrevScrolledRect.width != aCurScrolledRect.width) {
    result |= nsIScrollableFrame::HORIZONTAL;
  }
  if (aPrevScrolledRect.y != aCurScrolledRect.y ||
      aPrevScrolledRect.height != aCurScrolledRect.height) {
    result |= nsIScrollableFrame::VERTICAL;
  }
  return result;
}





nsHTMLScrollFrame*
NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot)
{
  return new (aPresShell) nsHTMLScrollFrame(aContext, aIsRoot);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLScrollFrame)

nsHTMLScrollFrame::nsHTMLScrollFrame(nsStyleContext* aContext, bool aIsRoot)
  : nsContainerFrame(aContext),
    mHelper(ALLOW_THIS_IN_INITIALIZER_LIST(this), aIsRoot)
{
}

void
nsHTMLScrollFrame::ScrollbarActivityStarted() const
{
  if (mHelper.mScrollbarActivity) {
    mHelper.mScrollbarActivity->ActivityStarted();
  }
}

void
nsHTMLScrollFrame::ScrollbarActivityStopped() const
{
  if (mHelper.mScrollbarActivity) {
    mHelper.mScrollbarActivity->ActivityStopped();
  }
}

nsresult
nsHTMLScrollFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  return mHelper.CreateAnonymousContent(aElements);
}

void
nsHTMLScrollFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                            uint32_t aFilter)
{
  mHelper.AppendAnonymousContentTo(aElements, aFilter);
}

void
nsHTMLScrollFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  mHelper.Destroy();
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

void
nsHTMLScrollFrame::SetInitialChildList(ChildListID  aListID,
                                       nsFrameList& aChildList)
{
  nsContainerFrame::SetInitialChildList(aListID, aChildList);
  mHelper.ReloadChildFrames();
}


void
nsHTMLScrollFrame::AppendFrames(ChildListID  aListID,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  mFrames.AppendFrames(nullptr, aFrameList);
  mHelper.ReloadChildFrames();
}

void
nsHTMLScrollFrame::InsertFrames(ChildListID aListID,
                                nsIFrame* aPrevFrame,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");
  mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);
  mHelper.ReloadChildFrames();
}

void
nsHTMLScrollFrame::RemoveFrame(ChildListID aListID,
                               nsIFrame* aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  mFrames.DestroyFrame(aOldFrame);
  mHelper.ReloadChildFrames();
}

nsSplittableType
nsHTMLScrollFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

nsIAtom*
nsHTMLScrollFrame::GetType() const
{
  return nsGkAtoms::scrollFrame;
}







struct MOZ_STACK_CLASS ScrollReflowState {
  const nsHTMLReflowState& mReflowState;
  nsBoxLayoutState mBoxState;
  ScrollbarStyles mStyles;
  nsMargin mComputedBorder;

  
  nsOverflowAreas mContentsOverflowAreas;
  bool mReflowedContentsWithHScrollbar;
  bool mReflowedContentsWithVScrollbar;

  
  
  nsSize mInsideBorderSize;
  
  bool mShowHScrollbar;
  
  bool mShowVScrollbar;

  ScrollReflowState(nsIScrollableFrame* aFrame,
                    const nsHTMLReflowState& aState) :
    mReflowState(aState),
    
    
    mBoxState(aState.frame->PresContext(), aState.rendContext, 0),
    mStyles(aFrame->GetScrollbarStyles()) {
  }
};


static nsSize ComputeInsideBorderSize(ScrollReflowState* aState,
                                      const nsSize& aDesiredInsideBorderSize)
{
  
  
  
  nscoord contentWidth = aState->mReflowState.ComputedWidth();
  if (contentWidth == NS_UNCONSTRAINEDSIZE) {
    contentWidth = aDesiredInsideBorderSize.width -
      aState->mReflowState.ComputedPhysicalPadding().LeftRight();
  }
  nscoord contentHeight = aState->mReflowState.ComputedHeight();
  if (contentHeight == NS_UNCONSTRAINEDSIZE) {
    contentHeight = aDesiredInsideBorderSize.height -
      aState->mReflowState.ComputedPhysicalPadding().TopBottom();
  }

  contentWidth  = aState->mReflowState.ApplyMinMaxWidth(contentWidth);
  contentHeight = aState->mReflowState.ApplyMinMaxHeight(contentHeight);
  return nsSize(contentWidth + aState->mReflowState.ComputedPhysicalPadding().LeftRight(),
                contentHeight + aState->mReflowState.ComputedPhysicalPadding().TopBottom());
}

static void
GetScrollbarMetrics(nsBoxLayoutState& aState, nsIFrame* aBox, nsSize* aMin,
                    nsSize* aPref, bool aVertical)
{
  NS_ASSERTION(aState.GetRenderingContext(),
               "Must have rendering context in layout state for size "
               "computations");

  if (aMin) {
    *aMin = aBox->GetMinSize(aState);
    nsBox::AddMargin(aBox, *aMin);
    if (aMin->width < 0) {
      aMin->width = 0;
    }
    if (aMin->height < 0) {
      aMin->height = 0;
    }
  }

  if (aPref) {
    *aPref = aBox->GetPrefSize(aState);
    nsBox::AddMargin(aBox, *aPref);
    if (aPref->width < 0) {
      aPref->width = 0;
    }
    if (aPref->height < 0) {
      aPref->height = 0;
    }
  }
}






















bool
nsHTMLScrollFrame::TryLayout(ScrollReflowState* aState,
                             nsHTMLReflowMetrics* aKidMetrics,
                             bool aAssumeHScroll, bool aAssumeVScroll,
                             bool aForce)
{
  if ((aState->mStyles.mVertical == NS_STYLE_OVERFLOW_HIDDEN && aAssumeVScroll) ||
      (aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN && aAssumeHScroll)) {
    NS_ASSERTION(!aForce, "Shouldn't be forcing a hidden scrollbar to show!");
    return false;
  }

  if (aAssumeVScroll != aState->mReflowedContentsWithVScrollbar ||
      (aAssumeHScroll != aState->mReflowedContentsWithHScrollbar &&
       ScrolledContentDependsOnHeight(aState))) {
    ReflowScrolledFrame(aState, aAssumeHScroll, aAssumeVScroll, aKidMetrics,
                        false);
  }

  nsSize vScrollbarMinSize(0, 0);
  nsSize vScrollbarPrefSize(0, 0);
  if (mHelper.mVScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mHelper.mVScrollbarBox,
                        &vScrollbarMinSize,
                        aAssumeVScroll ? &vScrollbarPrefSize : nullptr, true);
    nsScrollbarFrame* scrollbar = do_QueryFrame(mHelper.mVScrollbarBox);
    scrollbar->SetScrollbarMediatorContent(mContent);
  }
  nscoord vScrollbarDesiredWidth = aAssumeVScroll ? vScrollbarPrefSize.width : 0;
  nscoord vScrollbarMinHeight = aAssumeVScroll ? vScrollbarMinSize.height : 0;

  nsSize hScrollbarMinSize(0, 0);
  nsSize hScrollbarPrefSize(0, 0);
  if (mHelper.mHScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mHelper.mHScrollbarBox,
                        &hScrollbarMinSize,
                        aAssumeHScroll ? &hScrollbarPrefSize : nullptr, false);
    nsScrollbarFrame* scrollbar = do_QueryFrame(mHelper.mHScrollbarBox);
    scrollbar->SetScrollbarMediatorContent(mContent);
  }
  nscoord hScrollbarDesiredHeight = aAssumeHScroll ? hScrollbarPrefSize.height : 0;
  nscoord hScrollbarMinWidth = aAssumeHScroll ? hScrollbarMinSize.width : 0;

  
  
  nsSize desiredInsideBorderSize;
  desiredInsideBorderSize.width = vScrollbarDesiredWidth +
    std::max(aKidMetrics->Width(), hScrollbarMinWidth);
  desiredInsideBorderSize.height = hScrollbarDesiredHeight +
    std::max(aKidMetrics->Height(), vScrollbarMinHeight);
  aState->mInsideBorderSize =
    ComputeInsideBorderSize(aState, desiredInsideBorderSize);
  nsSize scrollPortSize = nsSize(std::max(0, aState->mInsideBorderSize.width - vScrollbarDesiredWidth),
                                 std::max(0, aState->mInsideBorderSize.height - hScrollbarDesiredHeight));
  nsSize visualScrollPortSize = scrollPortSize;
  nsIPresShell* presShell = PresContext()->PresShell();
  if (mHelper.mIsRoot && presShell->IsScrollPositionClampingScrollPortSizeSet()) {
    visualScrollPortSize = presShell->GetScrollPositionClampingScrollPortSize();
  }

  if (!aForce) {
    nsRect scrolledRect =
      mHelper.GetScrolledRectInternal(aState->mContentsOverflowAreas.ScrollableOverflow(),
                                     scrollPortSize);
    nscoord oneDevPixel = aState->mBoxState.PresContext()->DevPixelsToAppUnits(1);

    
    if (aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
      bool wantHScrollbar =
        aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.XMost() >= visualScrollPortSize.width + oneDevPixel ||
        scrolledRect.x <= -oneDevPixel;
      if (scrollPortSize.width < hScrollbarMinSize.width)
        wantHScrollbar = false;
      if (wantHScrollbar != aAssumeHScroll)
        return false;
    }

    
    if (aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
      bool wantVScrollbar =
        aState->mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.YMost() >= visualScrollPortSize.height + oneDevPixel ||
        scrolledRect.y <= -oneDevPixel;
      if (scrollPortSize.height < vScrollbarMinSize.height)
        wantVScrollbar = false;
      if (wantVScrollbar != aAssumeVScroll)
        return false;
    }
  }

  nscoord vScrollbarActualWidth = aState->mInsideBorderSize.width - scrollPortSize.width;

  aState->mShowHScrollbar = aAssumeHScroll;
  aState->mShowVScrollbar = aAssumeVScroll;
  nsPoint scrollPortOrigin(aState->mComputedBorder.left,
                           aState->mComputedBorder.top);
  if (!mHelper.IsScrollbarOnRight()) {
    scrollPortOrigin.x += vScrollbarActualWidth;
  }
  mHelper.mScrollPort = nsRect(scrollPortOrigin, scrollPortSize);
  return true;
}

bool
nsHTMLScrollFrame::ScrolledContentDependsOnHeight(ScrollReflowState* aState)
{
  
  
  return (mHelper.mScrolledFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT) ||
    aState->mReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
    aState->mReflowState.ComputedMinHeight() > 0 ||
    aState->mReflowState.ComputedMaxHeight() != NS_UNCONSTRAINEDSIZE;
}

void
nsHTMLScrollFrame::ReflowScrolledFrame(ScrollReflowState* aState,
                                       bool aAssumeHScroll,
                                       bool aAssumeVScroll,
                                       nsHTMLReflowMetrics* aMetrics,
                                       bool aFirstPass)
{
  WritingMode wm = mHelper.mScrolledFrame->GetWritingMode();

  
  
  LogicalMargin padding = aState->mReflowState.ComputedLogicalPadding();
  nscoord availISize =
    aState->mReflowState.ComputedISize() + padding.IStartEnd(wm);

  nscoord computedBSize = aState->mReflowState.ComputedBSize();
  nscoord computedMinBSize = aState->mReflowState.ComputedMinBSize();
  nscoord computedMaxBSize = aState->mReflowState.ComputedMaxBSize();
  if (!ShouldPropagateComputedHeightToScrolledContent()) {
    computedBSize = NS_UNCONSTRAINEDSIZE;
    computedMinBSize = 0;
    computedMaxBSize = NS_UNCONSTRAINEDSIZE;
  }

  if (wm.IsVertical()) {
    if (aAssumeVScroll) {
      nsSize vScrollbarPrefSize;
      GetScrollbarMetrics(aState->mBoxState, mHelper.mVScrollbarBox,
                          nullptr, &vScrollbarPrefSize, false);
      if (computedBSize != NS_UNCONSTRAINEDSIZE) {
        computedBSize = std::max(0, computedBSize - vScrollbarPrefSize.width);
      }
      computedMinBSize = std::max(0, computedMinBSize - vScrollbarPrefSize.width);
      if (computedMaxBSize != NS_UNCONSTRAINEDSIZE) {
        computedMaxBSize = std::max(0, computedMaxBSize - vScrollbarPrefSize.width);
      }
    }

    if (aAssumeHScroll) {
      nsSize hScrollbarPrefSize;
      GetScrollbarMetrics(aState->mBoxState, mHelper.mHScrollbarBox,
                          nullptr, &hScrollbarPrefSize, true);
      availISize = std::max(0, availISize - hScrollbarPrefSize.height);
    }
  } else {
    if (aAssumeHScroll) {
      nsSize hScrollbarPrefSize;
      GetScrollbarMetrics(aState->mBoxState, mHelper.mHScrollbarBox,
                          nullptr, &hScrollbarPrefSize, false);
      if (computedBSize != NS_UNCONSTRAINEDSIZE) {
        computedBSize = std::max(0, computedBSize - hScrollbarPrefSize.height);
      }
      computedMinBSize = std::max(0, computedMinBSize - hScrollbarPrefSize.height);
      if (computedMaxBSize != NS_UNCONSTRAINEDSIZE) {
        computedMaxBSize = std::max(0, computedMaxBSize - hScrollbarPrefSize.height);
      }
    }

    if (aAssumeVScroll) {
      nsSize vScrollbarPrefSize;
      GetScrollbarMetrics(aState->mBoxState, mHelper.mVScrollbarBox,
                          nullptr, &vScrollbarPrefSize, true);
      availISize = std::max(0, availISize - vScrollbarPrefSize.width);
    }
  }

  nsPresContext* presContext = PresContext();

  
  nsHTMLReflowState
    kidReflowState(presContext, aState->mReflowState,
                   mHelper.mScrolledFrame,
                   LogicalSize(wm, availISize, NS_UNCONSTRAINEDSIZE),
                   -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  const nsMargin physicalPadding = padding.GetPhysicalMargin(wm);
  kidReflowState.Init(presContext, -1, -1, nullptr,
                      &physicalPadding);
  kidReflowState.mFlags.mAssumingHScrollbar = aAssumeHScroll;
  kidReflowState.mFlags.mAssumingVScrollbar = aAssumeVScroll;
  kidReflowState.SetComputedBSize(computedBSize);
  kidReflowState.ComputedMinBSize() = computedMinBSize;
  kidReflowState.ComputedMaxBSize() = computedMaxBSize;

  
  
  bool didHaveHorizontalScrollbar = mHelper.mHasHorizontalScrollbar;
  bool didHaveVerticalScrollbar = mHelper.mHasVerticalScrollbar;
  mHelper.mHasHorizontalScrollbar = aAssumeHScroll;
  mHelper.mHasVerticalScrollbar = aAssumeVScroll;

  nsReflowStatus status;
  
  
  
  ReflowChild(mHelper.mScrolledFrame, presContext, *aMetrics,
              kidReflowState, wm, LogicalPoint(wm), 0,
              NS_FRAME_NO_MOVE_FRAME, status);

  mHelper.mHasHorizontalScrollbar = didHaveHorizontalScrollbar;
  mHelper.mHasVerticalScrollbar = didHaveVerticalScrollbar;

  
  
  
  
  
  FinishReflowChild(mHelper.mScrolledFrame, presContext,
                    *aMetrics, &kidReflowState, wm, LogicalPoint(wm), 0,
                    NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_SIZE_VIEW);

  
  
  
  
  
  
  
  aMetrics->UnionOverflowAreasWithDesiredBounds();

  if (MOZ_UNLIKELY(StyleDisplay()->mOverflowClipBox ==
                     NS_STYLE_OVERFLOW_CLIP_BOX_CONTENT_BOX)) {
    nsOverflowAreas childOverflow;
    nsLayoutUtils::UnionChildOverflow(mHelper.mScrolledFrame, childOverflow);
    nsRect childScrollableOverflow = childOverflow.ScrollableOverflow();
    childScrollableOverflow.Inflate(padding.GetPhysicalMargin(wm));
    nsRect contentArea =
      wm.IsVertical() ? nsRect(0, 0, computedBSize, availISize)
                      : nsRect(0, 0, availISize, computedBSize);
    if (!contentArea.Contains(childScrollableOverflow)) {
      aMetrics->mOverflowAreas.ScrollableOverflow() = childScrollableOverflow;
    }
  }

  aState->mContentsOverflowAreas = aMetrics->mOverflowAreas;
  aState->mReflowedContentsWithHScrollbar = aAssumeHScroll;
  aState->mReflowedContentsWithVScrollbar = aAssumeVScroll;
}

bool
nsHTMLScrollFrame::GuessHScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mHorizontal != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL;

  return mHelper.mHasHorizontalScrollbar;
}

bool
nsHTMLScrollFrame::GuessVScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mVertical != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL;

  
  
  
  if (mHelper.mHadNonInitialReflow) {
    return mHelper.mHasVerticalScrollbar;
  }

  
  
  if (InInitialReflow())
    return false;

  if (mHelper.mIsRoot) {
    nsIFrame *f = mHelper.mScrolledFrame->GetFirstPrincipalChild();
    if (f && f->GetType() == nsGkAtoms::svgOuterSVGFrame &&
        static_cast<nsSVGOuterSVGFrame*>(f)->VerticalScrollbarNotNeeded()) {
      
      return false;
    }
    
    
    
    
    return true;
  }

  
  
  
  
  return false;
}

bool
nsHTMLScrollFrame::InInitialReflow() const
{
  
  
  
  
  
  
  return !mHelper.mIsRoot && (GetStateBits() & NS_FRAME_FIRST_REFLOW);
}

void
nsHTMLScrollFrame::ReflowContents(ScrollReflowState* aState,
                                  const nsHTMLReflowMetrics& aDesiredSize)
{
  nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.GetWritingMode(), aDesiredSize.mFlags);
  ReflowScrolledFrame(aState, GuessHScrollbarNeeded(*aState),
                      GuessVScrollbarNeeded(*aState), &kidDesiredSize, true);

  
  
  
  
  
  
  
  
  
  
  
  

  
  
  

  
  
  if ((aState->mReflowedContentsWithHScrollbar || aState->mReflowedContentsWithVScrollbar) &&
      aState->mStyles.mVertical != NS_STYLE_OVERFLOW_SCROLL &&
      aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL) {
    nsSize insideBorderSize =
      ComputeInsideBorderSize(aState,
                              nsSize(kidDesiredSize.Width(), kidDesiredSize.Height()));
    nsRect scrolledRect =
      mHelper.GetScrolledRectInternal(kidDesiredSize.ScrollableOverflow(),
                                     insideBorderSize);
    if (nsRect(nsPoint(0, 0), insideBorderSize).Contains(scrolledRect)) {
      
      ReflowScrolledFrame(aState, false, false, &kidDesiredSize, false);
    }
  }

  
  
  

  
  
  if (TryLayout(aState, &kidDesiredSize, aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, false))
    return;
  if (TryLayout(aState, &kidDesiredSize, !aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, false))
    return;

  
  
  
  
  bool newVScrollbarState = !aState->mReflowedContentsWithVScrollbar;
  if (TryLayout(aState, &kidDesiredSize, false, newVScrollbarState, false))
    return;
  if (TryLayout(aState, &kidDesiredSize, true, newVScrollbarState, false))
    return;

  
  
  
  TryLayout(aState, &kidDesiredSize,
            aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN,
            aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN,
            true);
}

void
nsHTMLScrollFrame::PlaceScrollArea(const ScrollReflowState& aState,
                                   const nsPoint& aScrollPosition)
{
  nsIFrame *scrolledFrame = mHelper.mScrolledFrame;
  
  scrolledFrame->SetPosition(mHelper.mScrollPort.TopLeft() - aScrollPosition);

  nsRect scrolledArea;
  
  nsSize portSize = mHelper.mScrollPort.Size();
  nsRect scrolledRect =
    mHelper.GetScrolledRectInternal(aState.mContentsOverflowAreas.ScrollableOverflow(),
                                   portSize);
  scrolledArea.UnionRectEdges(scrolledRect,
                              nsRect(nsPoint(0,0), portSize));

  
  
  
  
  
  
  
  
  
  nsOverflowAreas overflow(scrolledArea, scrolledArea);
  scrolledFrame->FinishAndStoreOverflow(overflow,
                                        scrolledFrame->GetSize());

  
  
  
  
  
  nsContainerFrame::SyncFrameViewAfterReflow(scrolledFrame->PresContext(),
                                             scrolledFrame,
                                             scrolledFrame->GetView(),
                                             scrolledArea,
                                             0);
}

nscoord
nsHTMLScrollFrame::GetIntrinsicVScrollbarWidth(nsRenderingContext *aRenderingContext)
{
  ScrollbarStyles ss = GetScrollbarStyles();
  if (ss.mVertical != NS_STYLE_OVERFLOW_SCROLL || !mHelper.mVScrollbarBox)
    return 0;

  
  
  nsBoxLayoutState bls(PresContext(), aRenderingContext, 0);
  nsSize vScrollbarPrefSize(0, 0);
  GetScrollbarMetrics(bls, mHelper.mVScrollbarBox,
                      nullptr, &vScrollbarPrefSize, true);
  return vScrollbarPrefSize.width;
}

 nscoord
nsHTMLScrollFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = mHelper.mScrolledFrame->GetMinISize(aRenderingContext);
  DISPLAY_MIN_WIDTH(this, result);
  return result + GetIntrinsicVScrollbarWidth(aRenderingContext);
}

 nscoord
nsHTMLScrollFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = mHelper.mScrolledFrame->GetPrefISize(aRenderingContext);
  DISPLAY_PREF_WIDTH(this, result);
  return NSCoordSaturatingAdd(result, GetIntrinsicVScrollbarWidth(aRenderingContext));
}

nsresult
nsHTMLScrollFrame::GetPadding(nsMargin& aMargin)
{
  
  
  
  
  aMargin.SizeTo(0,0,0,0);
  return NS_OK;
}

bool
nsHTMLScrollFrame::IsCollapsed()
{
  
  return false;
}



static nsIContent*
GetBrowserRoot(nsIContent* aContent)
{
  if (aContent) {
    nsIDocument* doc = aContent->GetCurrentDoc();
    nsPIDOMWindow* win = doc->GetWindow();
    if (win) {
      nsCOMPtr<Element> frameElement = win->GetFrameElementInternal();
      if (frameElement &&
          frameElement->NodeInfo()->Equals(nsGkAtoms::browser, kNameSpaceID_XUL))
        return frameElement;
    }
  }

  return nullptr;
}

void
nsHTMLScrollFrame::Reflow(nsPresContext*           aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsHTMLScrollFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  mHelper.HandleScrollbarStyleSwitching();

  ScrollReflowState state(this, aReflowState);
  
  
  if (!mHelper.mVScrollbarBox || mHelper.mNeverHasVerticalScrollbar)
    state.mStyles.mVertical = NS_STYLE_OVERFLOW_HIDDEN;
  if (!mHelper.mHScrollbarBox || mHelper.mNeverHasHorizontalScrollbar)
    state.mStyles.mHorizontal = NS_STYLE_OVERFLOW_HIDDEN;

  
  bool reflowHScrollbar = true;
  bool reflowVScrollbar = true;
  bool reflowScrollCorner = true;
  if (!aReflowState.ShouldReflowAllKids()) {
    #define NEEDS_REFLOW(frame_) \
      ((frame_) && NS_SUBTREE_DIRTY(frame_))

    reflowHScrollbar = NEEDS_REFLOW(mHelper.mHScrollbarBox);
    reflowVScrollbar = NEEDS_REFLOW(mHelper.mVScrollbarBox);
    reflowScrollCorner = NEEDS_REFLOW(mHelper.mScrollCornerBox) ||
                         NEEDS_REFLOW(mHelper.mResizerBox);

    #undef NEEDS_REFLOW
  }

  if (mHelper.mIsRoot) {
    mHelper.mCollapsedResizer = true;

    nsIContent* browserRoot = GetBrowserRoot(mContent);
    if (browserRoot) {
      bool showResizer = browserRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::showresizer);
      reflowScrollCorner = showResizer == mHelper.mCollapsedResizer;
      mHelper.mCollapsedResizer = !showResizer;
    }
  }

  nsRect oldScrollAreaBounds = mHelper.mScrollPort;
  nsRect oldScrolledAreaBounds =
    mHelper.mScrolledFrame->GetScrollableOverflowRectRelativeToParent();
  nsPoint oldScrollPosition = mHelper.GetScrollPosition();

  state.mComputedBorder = aReflowState.ComputedPhysicalBorderPadding() -
    aReflowState.ComputedPhysicalPadding();

  ReflowContents(&state, aDesiredSize);

  
  
  
  PlaceScrollArea(state, oldScrollPosition);
  if (!mHelper.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mHelper);
    mHelper.mPostedReflowCallback = true;
  }

  bool didHaveHScrollbar = mHelper.mHasHorizontalScrollbar;
  bool didHaveVScrollbar = mHelper.mHasVerticalScrollbar;
  mHelper.mHasHorizontalScrollbar = state.mShowHScrollbar;
  mHelper.mHasVerticalScrollbar = state.mShowVScrollbar;
  nsRect newScrollAreaBounds = mHelper.mScrollPort;
  nsRect newScrolledAreaBounds =
    mHelper.mScrolledFrame->GetScrollableOverflowRectRelativeToParent();
  if (mHelper.mSkippedScrollbarLayout ||
      reflowHScrollbar || reflowVScrollbar || reflowScrollCorner ||
      (GetStateBits() & NS_FRAME_IS_DIRTY) ||
      didHaveHScrollbar != state.mShowHScrollbar ||
      didHaveVScrollbar != state.mShowVScrollbar ||
      !oldScrollAreaBounds.IsEqualEdges(newScrollAreaBounds) ||
      !oldScrolledAreaBounds.IsEqualEdges(newScrolledAreaBounds)) {
    if (!mHelper.mSupppressScrollbarUpdate) {
      mHelper.mSkippedScrollbarLayout = false;
      mHelper.SetScrollbarVisibility(mHelper.mHScrollbarBox, state.mShowHScrollbar);
      mHelper.SetScrollbarVisibility(mHelper.mVScrollbarBox, state.mShowVScrollbar);
      
      nsRect insideBorderArea =
        nsRect(nsPoint(state.mComputedBorder.left, state.mComputedBorder.top),
               state.mInsideBorderSize);
      mHelper.LayoutScrollbars(state.mBoxState, insideBorderArea,
                              oldScrollAreaBounds);
    } else {
      mHelper.mSkippedScrollbarLayout = true;
    }
  }

  aDesiredSize.Width() = state.mInsideBorderSize.width +
    state.mComputedBorder.LeftRight();
  aDesiredSize.Height() = state.mInsideBorderSize.height +
    state.mComputedBorder.TopBottom();

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (mHelper.IsIgnoringViewportClipping()) {
    aDesiredSize.mOverflowAreas.UnionWith(
      state.mContentsOverflowAreas + mHelper.mScrolledFrame->GetPosition());
  }

  mHelper.UpdateSticky();
  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  if (!InInitialReflow() && !mHelper.mHadNonInitialReflow) {
    mHelper.mHadNonInitialReflow = true;
  }

  if (mHelper.mIsRoot && !oldScrolledAreaBounds.IsEqualEdges(newScrolledAreaBounds)) {
    mHelper.PostScrolledAreaEvent();
  }

  mHelper.UpdatePrevScrolledRect();

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  mHelper.PostOverflowEvent();
}




#ifdef DEBUG_FRAME_DUMP
nsresult
nsHTMLScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLScroll"), aResult);
}
#endif

#ifdef ACCESSIBILITY
a11y::AccType
nsHTMLScrollFrame::AccessibleType()
{
  if (IsTableCaption()) {
    return GetRect().IsEmpty() ? a11y::eNoType : a11y::eHTMLCaptionType;
  }

  
  
  if (mContent->IsRootOfNativeAnonymousSubtree() ||
      GetScrollbarStyles().IsHiddenInBothDirections()) {
    return a11y::eNoType;
  }

  return a11y::eHyperTextType;
}
#endif

NS_QUERYFRAME_HEAD(nsHTMLScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollbarMediator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)



nsXULScrollFrame*
NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     bool aIsRoot, bool aClipAllDescendants)
{
  return new (aPresShell) nsXULScrollFrame(aContext, aIsRoot,
                                           aClipAllDescendants);
}

NS_IMPL_FRAMEARENA_HELPERS(nsXULScrollFrame)

nsXULScrollFrame::nsXULScrollFrame(nsStyleContext* aContext,
                                   bool aIsRoot, bool aClipAllDescendants)
  : nsBoxFrame(aContext, aIsRoot),
    mHelper(ALLOW_THIS_IN_INITIALIZER_LIST(this), aIsRoot)
{
  SetLayoutManager(nullptr);
  mHelper.mClipAllDescendants = aClipAllDescendants;
}

void
nsXULScrollFrame::ScrollbarActivityStarted() const
{
  if (mHelper.mScrollbarActivity) {
    mHelper.mScrollbarActivity->ActivityStarted();
  }
}

void
nsXULScrollFrame::ScrollbarActivityStopped() const
{
  if (mHelper.mScrollbarActivity) {
    mHelper.mScrollbarActivity->ActivityStopped();
  }
}

nsMargin
ScrollFrameHelper::GetDesiredScrollbarSizes(nsBoxLayoutState* aState)
{
  NS_ASSERTION(aState && aState->GetRenderingContext(),
               "Must have rendering context in layout state for size "
               "computations");

  nsMargin result(0, 0, 0, 0);

  if (mVScrollbarBox) {
    nsSize size = mVScrollbarBox->GetPrefSize(*aState);
    nsBox::AddMargin(mVScrollbarBox, size);
    if (IsScrollbarOnRight())
      result.left = size.width;
    else
      result.right = size.width;
  }

  if (mHScrollbarBox) {
    nsSize size = mHScrollbarBox->GetPrefSize(*aState);
    nsBox::AddMargin(mHScrollbarBox, size);
    
    
    result.bottom = size.height;
  }

  return result;
}

nscoord
ScrollFrameHelper::GetNondisappearingScrollbarWidth(nsBoxLayoutState* aState)
{
  NS_ASSERTION(aState && aState->GetRenderingContext(),
               "Must have rendering context in layout state for size "
               "computations");

  if (LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
    
    
    nsITheme* theme = aState->PresContext()->GetTheme();
    if (theme &&
        theme->ThemeSupportsWidget(aState->PresContext(),
                                   mVScrollbarBox,
                                   NS_THEME_SCROLLBAR_NON_DISAPPEARING)) {
      LayoutDeviceIntSize size;
      bool canOverride = true;
      theme->GetMinimumWidgetSize(aState->PresContext(),
                                  mVScrollbarBox,
                                  NS_THEME_SCROLLBAR_NON_DISAPPEARING,
                                  &size,
                                  &canOverride);
      if (size.width) {
        return aState->PresContext()->DevPixelsToAppUnits(size.width);
      }
    }
  }

  return GetDesiredScrollbarSizes(aState).LeftRight();
}

void
ScrollFrameHelper::HandleScrollbarStyleSwitching()
{
  
  if (mScrollbarActivity &&
      LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) == 0) {
    mScrollbarActivity->Destroy();
    mScrollbarActivity = nullptr;
    mOuter->PresContext()->ThemeChanged();
  }
  else if (!mScrollbarActivity &&
           LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
    mScrollbarActivity = new ScrollbarActivity(do_QueryFrame(mOuter));
    mOuter->PresContext()->ThemeChanged();
  }
}

#if defined(MOZ_B2G) || (defined(MOZ_WIDGET_ANDROID) && !defined(MOZ_ANDROID_APZ))
static bool IsFocused(nsIContent* aContent)
{
  
  
  
  
  while (aContent && aContent->IsInAnonymousSubtree()) {
    aContent = aContent->GetParent();
  }

  return aContent ? nsContentUtils::IsFocusedContent(aContent) : false;
}
#endif

bool
ScrollFrameHelper::WantAsyncScroll() const
{
  ScrollbarStyles styles = GetScrollbarStylesFromFrame();
  uint32_t directions = mOuter->GetScrollTargetFrame()->GetPerceivedScrollingDirections();
  bool isVScrollable = !!(directions & nsIScrollableFrame::VERTICAL) &&
                       (styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN);
  bool isHScrollable = !!(directions & nsIScrollableFrame::HORIZONTAL) &&
                       (styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN);

#if defined(MOZ_B2G) || (defined(MOZ_WIDGET_ANDROID) && !defined(MOZ_ANDROID_APZ))
  
  bool canScrollWithoutScrollbars = IsFocused(mOuter->GetContent());
#else
  bool canScrollWithoutScrollbars = true;
#endif

  
  
  bool isVAsyncScrollable = isVScrollable && (mVScrollbarBox || canScrollWithoutScrollbars);
  bool isHAsyncScrollable = isHScrollable && (mHScrollbarBox || canScrollWithoutScrollbars);
  return isVAsyncScrollable || isHAsyncScrollable;
}

static nsRect
GetOnePixelRangeAroundPoint(nsPoint aPoint, bool aIsHorizontal)
{
  nsRect allowedRange(aPoint, nsSize());
  nscoord halfPixel = nsPresContext::CSSPixelsToAppUnits(0.5f);
  if (aIsHorizontal) {
    allowedRange.x = aPoint.x - halfPixel;
    allowedRange.width = halfPixel*2 - 1;
  } else {
    allowedRange.y = aPoint.y - halfPixel;
    allowedRange.height = halfPixel*2 - 1;
  }
  return allowedRange;
}

void
ScrollFrameHelper::ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                                nsIScrollbarMediator::ScrollSnapMode aSnap)
{
  ScrollByUnit(aScrollbar, nsIScrollableFrame::SMOOTH, aDirection,
               nsIScrollableFrame::PAGES, aSnap);
}

void
ScrollFrameHelper::ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                                 nsIScrollbarMediator::ScrollSnapMode aSnap)
{
  ScrollByUnit(aScrollbar, nsIScrollableFrame::INSTANT, aDirection,
               nsIScrollableFrame::WHOLE, aSnap);
}

void
ScrollFrameHelper::ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection,
                                nsIScrollbarMediator::ScrollSnapMode aSnap)
{
  bool isHorizontal = aScrollbar->IsHorizontal();
  nsIntPoint delta;
  if (isHorizontal) {
    const double kScrollMultiplier =
      Preferences::GetInt("toolkit.scrollbox.horizontalScrollDistance",
                          NS_DEFAULT_HORIZONTAL_SCROLL_DISTANCE);
    delta.x = aDirection * kScrollMultiplier;
    if (GetLineScrollAmount().width * delta.x > GetPageScrollAmount().width) {
      
      
      
      ScrollByPage(aScrollbar, aDirection);
      return;
    }
  } else {
    const double kScrollMultiplier =
      Preferences::GetInt("toolkit.scrollbox.verticalScrollDistance",
                          NS_DEFAULT_VERTICAL_SCROLL_DISTANCE);
    delta.y = aDirection * kScrollMultiplier;
    if (GetLineScrollAmount().height * delta.y > GetPageScrollAmount().height) {
      
      
      
      ScrollByPage(aScrollbar, aDirection);
      return;
    }
  }
  
  nsIntPoint overflow;
  ScrollBy(delta, nsIScrollableFrame::LINES, nsIScrollableFrame::SMOOTH,
           &overflow, nsGkAtoms::other, nsIScrollableFrame::NOT_MOMENTUM,
           aSnap);
}

void
ScrollFrameHelper::RepeatButtonScroll(nsScrollbarFrame* aScrollbar)
{
  aScrollbar->MoveToNewPosition();
}

void
ScrollFrameHelper::ThumbMoved(nsScrollbarFrame* aScrollbar,
                              nscoord aOldPos,
                              nscoord aNewPos)
{
  MOZ_ASSERT(aScrollbar != nullptr);
  bool isHorizontal = aScrollbar->IsHorizontal();
  nsPoint current = GetScrollPosition();
  nsPoint dest = current;
  if (isHorizontal) {
    dest.x = IsLTR() ? aNewPos : aNewPos - GetScrollRange().width;
  } else {
    dest.y = aNewPos;
  }
  nsRect allowedRange = GetOnePixelRangeAroundPoint(dest, isHorizontal);

  
  
  
  if (allowedRange.ClampPoint(current) == current) {
    return;
  }

  ScrollTo(dest, nsIScrollableFrame::INSTANT, &allowedRange);
}

void
ScrollFrameHelper::ScrollbarReleased(nsScrollbarFrame* aScrollbar)
{
  
  
  mVelocityQueue.Reset();

  
  
  ScrollSnap(mDestination, nsIScrollableFrame::SMOOTH);
}

void
ScrollFrameHelper::ScrollByUnit(nsScrollbarFrame* aScrollbar,
                                nsIScrollableFrame::ScrollMode aMode,
                                int32_t aDirection,
                                nsIScrollableFrame::ScrollUnit aUnit,
                                nsIScrollbarMediator::ScrollSnapMode aSnap)
{
  MOZ_ASSERT(aScrollbar != nullptr);
  bool isHorizontal = aScrollbar->IsHorizontal();
  nsIntPoint delta;
  if (isHorizontal) {
    delta.x = aDirection;
  } else {
    delta.y = aDirection;
  }
  nsIntPoint overflow;
  ScrollBy(delta, aUnit, aMode, &overflow, nsGkAtoms::other,
           nsIScrollableFrame::NOT_MOMENTUM, aSnap);
}

nsresult
nsXULScrollFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  return mHelper.CreateAnonymousContent(aElements);
}

void
nsXULScrollFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                           uint32_t aFilter)
{
  mHelper.AppendAnonymousContentTo(aElements, aFilter);
}

void
nsXULScrollFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mHelper.Destroy();
  nsBoxFrame::DestroyFrom(aDestructRoot);
}

void
nsXULScrollFrame::SetInitialChildList(ChildListID     aListID,
                                      nsFrameList&    aChildList)
{
  nsBoxFrame::SetInitialChildList(aListID, aChildList);
  mHelper.ReloadChildFrames();
}


void
nsXULScrollFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  nsBoxFrame::AppendFrames(aListID, aFrameList);
  mHelper.ReloadChildFrames();
}

void
nsXULScrollFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
  mHelper.ReloadChildFrames();
}

void
nsXULScrollFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  nsBoxFrame::RemoveFrame(aListID, aOldFrame);
  mHelper.ReloadChildFrames();
}

nsSplittableType
nsXULScrollFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

nsresult
nsXULScrollFrame::GetPadding(nsMargin& aMargin)
{
  aMargin.SizeTo(0,0,0,0);
  return NS_OK;
}

nsIAtom*
nsXULScrollFrame::GetType() const
{
  return nsGkAtoms::scrollFrame;
}

nscoord
nsXULScrollFrame::GetBoxAscent(nsBoxLayoutState& aState)
{
  if (!mHelper.mScrolledFrame)
    return 0;

  nscoord ascent = mHelper.mScrolledFrame->GetBoxAscent(aState);
  nsMargin m(0,0,0,0);
  GetBorderAndPadding(m);
  ascent += m.top;
  GetMargin(m);
  ascent += m.top;

  return ascent;
}

nsSize
nsXULScrollFrame::GetPrefSize(nsBoxLayoutState& aState)
{
#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  nsSize pref = mHelper.mScrolledFrame->GetPrefSize(aState);

  ScrollbarStyles styles = GetScrollbarStyles();

  

  if (mHelper.mVScrollbarBox &&
      styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
    nsSize vSize = mHelper.mVScrollbarBox->GetPrefSize(aState);
    nsBox::AddMargin(mHelper.mVScrollbarBox, vSize);
    pref.width += vSize.width;
  }

  if (mHelper.mHScrollbarBox &&
      styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL) {
    nsSize hSize = mHelper.mHScrollbarBox->GetPrefSize(aState);
    nsBox::AddMargin(mHelper.mHScrollbarBox, hSize);
    pref.height += hSize.height;
  }

  AddBorderAndPadding(pref);
  bool widthSet, heightSet;
  nsIFrame::AddCSSPrefSize(this, pref, widthSet, heightSet);
  return pref;
}

nsSize
nsXULScrollFrame::GetMinSize(nsBoxLayoutState& aState)
{
#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  nsSize min = mHelper.mScrolledFrame->GetMinSizeForScrollArea(aState);

  ScrollbarStyles styles = GetScrollbarStyles();

  if (mHelper.mVScrollbarBox &&
      styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
     nsSize vSize = mHelper.mVScrollbarBox->GetMinSize(aState);
     AddMargin(mHelper.mVScrollbarBox, vSize);
     min.width += vSize.width;
     if (min.height < vSize.height)
        min.height = vSize.height;
  }

  if (mHelper.mHScrollbarBox &&
      styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL) {
     nsSize hSize = mHelper.mHScrollbarBox->GetMinSize(aState);
     AddMargin(mHelper.mHScrollbarBox, hSize);
     min.height += hSize.height;
     if (min.width < hSize.width)
        min.width = hSize.width;
  }

  AddBorderAndPadding(min);
  bool widthSet, heightSet;
  nsIFrame::AddCSSMinSize(aState, this, min, widthSet, heightSet);
  return min;
}

nsSize
nsXULScrollFrame::GetMaxSize(nsBoxLayoutState& aState)
{
#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  nsSize maxSize(NS_INTRINSICSIZE, NS_INTRINSICSIZE);

  AddBorderAndPadding(maxSize);
  bool widthSet, heightSet;
  nsIFrame::AddCSSMaxSize(this, maxSize, widthSet, heightSet);
  return maxSize;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsXULScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("XULScroll"), aResult);
}
#endif

NS_IMETHODIMP
nsXULScrollFrame::DoLayout(nsBoxLayoutState& aState)
{
  uint32_t flags = aState.LayoutFlags();
  nsresult rv = Layout(aState);
  aState.SetLayoutFlags(flags);

  nsBox::DoLayout(aState);
  return rv;
}

NS_QUERYFRAME_HEAD(nsXULScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollbarMediator)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)



#define SMOOTH_SCROLL_PREF_NAME "general.smoothScroll"


class ScrollFrameHelper::AsyncSmoothMSDScroll final : public nsARefreshObserver {
public:
  AsyncSmoothMSDScroll(const nsPoint &aInitialPosition,
                       const nsPoint &aInitialDestination,
                       const nsSize &aInitialVelocity,
                       const nsRect &aRange,
                       const mozilla::TimeStamp &aStartTime)
    : mXAxisModel(aInitialPosition.x, aInitialDestination.x,
                  aInitialVelocity.width,
                  gfxPrefs::ScrollBehaviorSpringConstant(),
                  gfxPrefs::ScrollBehaviorDampingRatio())
    , mYAxisModel(aInitialPosition.y, aInitialDestination.y,
                  aInitialVelocity.height,
                  gfxPrefs::ScrollBehaviorSpringConstant(),
                  gfxPrefs::ScrollBehaviorDampingRatio())
    , mRange(aRange)
    , mLastRefreshTime(aStartTime)
    , mCallee(nullptr)
  {
  }

  NS_INLINE_DECL_REFCOUNTING(AsyncSmoothMSDScroll, override)

  nsSize GetVelocity() {
    
    return nsSize(mXAxisModel.GetVelocity(), mYAxisModel.GetVelocity());
  }

  nsPoint GetPosition() {
    
    return nsPoint(NSToCoordRound(mXAxisModel.GetPosition()), NSToCoordRound(mYAxisModel.GetPosition()));
  }

  void SetDestination(const nsPoint &aDestination) {
    mXAxisModel.SetDestination(static_cast<int32_t>(aDestination.x));
    mYAxisModel.SetDestination(static_cast<int32_t>(aDestination.y));
  }

  void SetRange(const nsRect &aRange)
  {
    mRange = aRange;
  }

  nsRect GetRange()
  {
    return mRange;
  }

  void Simulate(const TimeDuration& aDeltaTime)
  {
    mXAxisModel.Simulate(aDeltaTime);
    mYAxisModel.Simulate(aDeltaTime);

    nsPoint desired = GetPosition();
    nsPoint clamped = mRange.ClampPoint(desired);
    if(desired.x != clamped.x) {
      
      
      
      mXAxisModel.SetVelocity(0.0);
      mXAxisModel.SetPosition(clamped.x);
    }

    if(desired.y != clamped.y) {
      
      
      
      mYAxisModel.SetVelocity(0.0);
      mYAxisModel.SetPosition(clamped.y);
    }
  }

  bool IsFinished()
  {
    return mXAxisModel.IsFinished() && mYAxisModel.IsFinished();
  }

  virtual void WillRefresh(mozilla::TimeStamp aTime) override {
    mozilla::TimeDuration deltaTime = aTime - mLastRefreshTime;
    mLastRefreshTime = aTime;

    
    
    ScrollFrameHelper::AsyncSmoothMSDScrollCallback(mCallee, deltaTime);
  }

  




  bool SetRefreshObserver(ScrollFrameHelper *aCallee) {
    NS_ASSERTION(aCallee && !mCallee, "AsyncSmoothMSDScroll::SetRefreshObserver - Invalid usage.");

    if (!RefreshDriver(aCallee)->AddRefreshObserver(this, Flush_Style)) {
      return false;
    }

    mCallee = aCallee;
    return true;
  }

private:
  
  ~AsyncSmoothMSDScroll() {
    RemoveObserver();
  }

  nsRefreshDriver* RefreshDriver(ScrollFrameHelper* aCallee) {
    return aCallee->mOuter->PresContext()->RefreshDriver();
  }

  




  void RemoveObserver() {
    if (mCallee) {
      RefreshDriver(mCallee)->RemoveRefreshObserver(this, Flush_Style);
    }
  }

  mozilla::layers::AxisPhysicsMSDModel mXAxisModel, mYAxisModel;
  nsRect mRange;
  mozilla::TimeStamp mLastRefreshTime;
  ScrollFrameHelper *mCallee;
};


class ScrollFrameHelper::AsyncScroll final
  : public nsARefreshObserver,
    public AsyncScrollBase
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  explicit AsyncScroll(nsPoint aStartPos)
    : AsyncScrollBase(aStartPos)
    , mCallee(nullptr)
  {}

private:
  
  ~AsyncScroll() {
    RemoveObserver();
  }

public:
  void InitSmoothScroll(TimeStamp aTime, nsPoint aDestination,
                        nsIAtom *aOrigin, const nsRect& aRange,
                        const nsSize& aCurrentVelocity);
  void Init(const nsRect& aRange) {
    mRange = aRange;
  }

  
  nsCOMPtr<nsIAtom> mOrigin;

  
  nsRect mRange;
  bool mIsSmoothScroll;

private:
  void InitPreferences(TimeStamp aTime, nsIAtom *aOrigin);



public:
  NS_INLINE_DECL_REFCOUNTING(AsyncScroll, override)

  




  bool SetRefreshObserver(ScrollFrameHelper *aCallee) {
    NS_ASSERTION(aCallee && !mCallee, "AsyncScroll::SetRefreshObserver - Invalid usage.");

    if (!RefreshDriver(aCallee)->AddRefreshObserver(this, Flush_Style)) {
      return false;
    }

    mCallee = aCallee;
    return true;
  }

  virtual void WillRefresh(mozilla::TimeStamp aTime) override {
    
    
    ScrollFrameHelper::AsyncScrollCallback(mCallee, aTime);
  }

private:
  ScrollFrameHelper *mCallee;

  nsRefreshDriver* RefreshDriver(ScrollFrameHelper* aCallee) {
    return aCallee->mOuter->PresContext()->RefreshDriver();
  }

  




  void RemoveObserver() {
    if (mCallee) {
      RefreshDriver(mCallee)->RemoveRefreshObserver(this, Flush_Style);
    }
  }
};





void
ScrollFrameHelper::AsyncScroll::InitPreferences(TimeStamp aTime, nsIAtom *aOrigin)
{
  if (!aOrigin){
    aOrigin = nsGkAtoms::other;
  }

  
  if (!mIsFirstIteration && aOrigin == mOrigin) {
    return;
  }

  mOrigin = aOrigin;
  mOriginMinMS = mOriginMaxMS = 0;
  bool isOriginSmoothnessEnabled = false;
  mIntervalRatio = 1;

  
  static const int32_t kDefaultMinMS = 150, kDefaultMaxMS = 150;
  static const bool kDefaultIsSmoothEnabled = true;

  nsAutoCString originName;
  aOrigin->ToUTF8String(originName);
  nsAutoCString prefBase = NS_LITERAL_CSTRING("general.smoothScroll.") + originName;

  isOriginSmoothnessEnabled = Preferences::GetBool(prefBase.get(), kDefaultIsSmoothEnabled);
  if (isOriginSmoothnessEnabled) {
    nsAutoCString prefMin = prefBase + NS_LITERAL_CSTRING(".durationMinMS");
    nsAutoCString prefMax = prefBase + NS_LITERAL_CSTRING(".durationMaxMS");
    mOriginMinMS = Preferences::GetInt(prefMin.get(), kDefaultMinMS);
    mOriginMaxMS = Preferences::GetInt(prefMax.get(), kDefaultMaxMS);

    static const int32_t kSmoothScrollMaxAllowedAnimationDurationMS = 10000;
    mOriginMaxMS = clamped(mOriginMaxMS, 0, kSmoothScrollMaxAllowedAnimationDurationMS);
    mOriginMinMS = clamped(mOriginMinMS, 0, mOriginMaxMS);
  }

  
  
  static const double kDefaultDurationToIntervalRatio = 2; 
  mIntervalRatio = Preferences::GetInt("general.smoothScroll.durationToIntervalRatio",
                                       kDefaultDurationToIntervalRatio * 100) / 100.0;

  
  mIntervalRatio = std::max(1.0, mIntervalRatio);

  if (mIsFirstIteration) {
    InitializeHistory(aTime);
  }
}

void
ScrollFrameHelper::AsyncScroll::InitSmoothScroll(TimeStamp aTime,
                                                 nsPoint aDestination,
                                                 nsIAtom *aOrigin,
                                                 const nsRect& aRange,
                                                 const nsSize& aCurrentVelocity)
{
  InitPreferences(aTime, aOrigin);
  mRange = aRange;

  Update(aTime, aDestination, aCurrentVelocity);
}

bool
ScrollFrameHelper::IsSmoothScrollingEnabled()
{
  return Preferences::GetBool(SMOOTH_SCROLL_PREF_NAME, false);
}

class ScrollFrameActivityTracker final : public nsExpirationTracker<ScrollFrameHelper,4> {
public:
  
  
  enum { TIMEOUT_MS = 1000 };
  ScrollFrameActivityTracker()
    : nsExpirationTracker<ScrollFrameHelper,4>(TIMEOUT_MS) {}
  ~ScrollFrameActivityTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(ScrollFrameHelper *aObject) {
    RemoveObject(aObject);
    aObject->MarkNotRecentlyScrolled();
  }
};

static ScrollFrameActivityTracker *gScrollFrameActivityTracker = nullptr;







static uint32_t sScrollGenerationCounter = 0;

ScrollFrameHelper::ScrollFrameHelper(nsContainerFrame* aOuter,
                                             bool aIsRoot)
  : mHScrollbarBox(nullptr)
  , mVScrollbarBox(nullptr)
  , mScrolledFrame(nullptr)
  , mScrollCornerBox(nullptr)
  , mResizerBox(nullptr)
  , mOuter(aOuter)
  , mAsyncScroll(nullptr)
  , mAsyncSmoothMSDScroll(nullptr)
  , mLastScrollOrigin(nsGkAtoms::other)
  , mLastSmoothScrollOrigin(nullptr)
  , mScrollGeneration(++sScrollGenerationCounter)
  , mDestination(0, 0)
  , mScrollPosAtLastPaint(0, 0)
  , mRestorePos(-1, -1)
  , mLastPos(-1, -1)
  , mResolution(1.0)
  , mScrollPosForLayerPixelAlignment(-1, -1)
  , mLastUpdateImagesPos(-1, -1)
  , mNeverHasVerticalScrollbar(false)
  , mNeverHasHorizontalScrollbar(false)
  , mHasVerticalScrollbar(false)
  , mHasHorizontalScrollbar(false)
  , mFrameIsUpdatingScrollbar(false)
  , mDidHistoryRestore(false)
  , mIsRoot(aIsRoot)
  , mClipAllDescendants(aIsRoot)
  , mSupppressScrollbarUpdate(false)
  , mSkippedScrollbarLayout(false)
  , mHadNonInitialReflow(false)
  , mHorizontalOverflow(false)
  , mVerticalOverflow(false)
  , mPostedReflowCallback(false)
  , mMayHaveDirtyFixedChildren(false)
  , mUpdateScrollbarAttributes(false)
  , mHasBeenScrolledRecently(false)
  , mCollapsedResizer(false)
  , mShouldBuildScrollableLayer(false)
  , mIsScrollableLayerInRootContainer(false)
  , mHasBeenScrolled(false)
  , mIsResolutionSet(false)
  , mIgnoreMomentumScroll(false)
  , mScaleToResolution(false)
  , mTransformingByAPZ(false)
  , mVelocityQueue(aOuter->PresContext())
{
  if (LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
    mScrollbarActivity = new ScrollbarActivity(do_QueryFrame(aOuter));
  }

  EnsureImageVisPrefsCached();

  if (IsAlwaysActive() &&
      gfxPrefs::LayersTilesEnabled() &&
      !nsLayoutUtils::UsesAsyncScrolling() &&
      mOuter->GetContent()) {
    
    
    
    nsLayoutUtils::SetDisplayPortMargins(mOuter->GetContent(),
                                         mOuter->PresContext()->PresShell(),
                                         ScreenMargin(),
                                         0,
                                         nsLayoutUtils::RepaintMode::DoNotRepaint);
  }

}

ScrollFrameHelper::~ScrollFrameHelper()
{
  if (mActivityExpirationState.IsTracked()) {
    gScrollFrameActivityTracker->RemoveObject(this);
  }
  if (gScrollFrameActivityTracker &&
      gScrollFrameActivityTracker->IsEmpty()) {
    delete gScrollFrameActivityTracker;
    gScrollFrameActivityTracker = nullptr;
  }

  if (mScrollActivityTimer) {
    mScrollActivityTimer->Cancel();
    mScrollActivityTimer = nullptr;
  }
}




void
ScrollFrameHelper::AsyncSmoothMSDScrollCallback(ScrollFrameHelper* aInstance,
                                                mozilla::TimeDuration aDeltaTime)
{
  NS_ASSERTION(aInstance != nullptr, "aInstance must not be null");
  NS_ASSERTION(aInstance->mAsyncSmoothMSDScroll,
    "Did not expect AsyncSmoothMSDScrollCallback without an active MSD scroll.");

  nsRect range = aInstance->mAsyncSmoothMSDScroll->GetRange();
  aInstance->mAsyncSmoothMSDScroll->Simulate(aDeltaTime);

  if (!aInstance->mAsyncSmoothMSDScroll->IsFinished()) {
    nsPoint destination = aInstance->mAsyncSmoothMSDScroll->GetPosition();
    
    
    
    
    nsRect intermediateRange =
      nsRect(destination, nsSize()).UnionEdges(range);
    aInstance->ScrollToImpl(destination, intermediateRange);
    
    return;
  }

  aInstance->CompleteAsyncScroll(range);
}




void
ScrollFrameHelper::AsyncScrollCallback(ScrollFrameHelper* aInstance,
                                       mozilla::TimeStamp aTime)
{
  MOZ_ASSERT(aInstance != nullptr, "aInstance must not be null");
  MOZ_ASSERT(aInstance->mAsyncScroll,
    "Did not expect AsyncScrollCallback without an active async scroll.");

  if (!aInstance || !aInstance->mAsyncScroll) {
    return;  
  }

  nsRect range = aInstance->mAsyncScroll->mRange;
  if (aInstance->mAsyncScroll->mIsSmoothScroll) {
    if (!aInstance->mAsyncScroll->IsFinished(aTime)) {
      nsPoint destination = aInstance->mAsyncScroll->PositionAt(aTime);
      
      
      
      nsRect intermediateRange =
        nsRect(aInstance->GetScrollPosition(), nsSize()).UnionEdges(range);
      aInstance->ScrollToImpl(destination, intermediateRange);
      
      return;
    }
  }

  aInstance->CompleteAsyncScroll(range);
}

void
ScrollFrameHelper::CompleteAsyncScroll(const nsRect &aRange, nsIAtom* aOrigin)
{
  
  mAsyncSmoothMSDScroll = nullptr;
  mAsyncScroll = nullptr;
  nsWeakFrame weakFrame(mOuter);
  ScrollToImpl(mDestination, aRange, aOrigin);
  if (!weakFrame.IsAlive()) {
    return;
  }
  
  
  mDestination = GetScrollPosition();
}

void
ScrollFrameHelper::ScrollToCSSPixels(const CSSIntPoint& aScrollPosition,
                                     nsIScrollableFrame::ScrollMode aMode)
{
  nsPoint current = GetScrollPosition();
  CSSIntPoint currentCSSPixels = GetScrollPositionCSSPixels();
  nsPoint pt = CSSPoint::ToAppUnits(aScrollPosition);
  nscoord halfPixel = nsPresContext::CSSPixelsToAppUnits(0.5f);
  nsRect range(pt.x - halfPixel, pt.y - halfPixel, 2*halfPixel - 1, 2*halfPixel - 1);
  
  
  
  if (currentCSSPixels.x == aScrollPosition.x) {
    pt.x = current.x;
    range.x = pt.x;
    range.width = 0;
  }
  if (currentCSSPixels.y == aScrollPosition.y) {
    pt.y = current.y;
    range.y = pt.y;
    range.height = 0;
  }
  ScrollTo(pt, aMode, &range);
  
}

void
ScrollFrameHelper::ScrollToCSSPixelsApproximate(const CSSPoint& aScrollPosition,
                                                nsIAtom *aOrigin)
{
  nsPoint pt = CSSPoint::ToAppUnits(aScrollPosition);
  nscoord halfRange = nsPresContext::CSSPixelsToAppUnits(1000);
  nsRect range(pt.x - halfRange, pt.y - halfRange, 2*halfRange - 1, 2*halfRange - 1);
  ScrollToWithOrigin(pt, nsIScrollableFrame::INSTANT, aOrigin, &range);
  
}

CSSIntPoint
ScrollFrameHelper::GetScrollPositionCSSPixels()
{
  return CSSIntPoint::FromAppUnitsRounded(GetScrollPosition());
}





void
ScrollFrameHelper::ScrollToWithOrigin(nsPoint aScrollPosition,
                                          nsIScrollableFrame::ScrollMode aMode,
                                          nsIAtom *aOrigin,
                                          const nsRect* aRange,
                                          nsIScrollbarMediator::ScrollSnapMode aSnap)
{

  if (aSnap == nsIScrollableFrame::ENABLE_SNAP) {
    GetSnapPointForDestination(nsIScrollableFrame::DEVICE_PIXELS,
                               mDestination,
                               aScrollPosition);
  }

  nsRect scrollRange = GetScrollRangeForClamping();
  mDestination = scrollRange.ClampPoint(aScrollPosition);

  nsRect range = aRange ? *aRange : nsRect(aScrollPosition, nsSize(0, 0));

  if (aMode == nsIScrollableFrame::INSTANT) {
    
    
    CompleteAsyncScroll(range, aOrigin);
    return;
  }

  nsPresContext* presContext = mOuter->PresContext();
  TimeStamp now = presContext->RefreshDriver()->MostRecentRefresh();
  bool isSmoothScroll = (aMode == nsIScrollableFrame::SMOOTH) &&
                          IsSmoothScrollingEnabled();

  nsSize currentVelocity(0, 0);

  if (gfxPrefs::ScrollBehaviorEnabled()) {
    if (aMode == nsIScrollableFrame::SMOOTH_MSD) {
      mIgnoreMomentumScroll = true;
      if (!mAsyncSmoothMSDScroll) {
        nsPoint sv = mVelocityQueue.GetVelocity();
        currentVelocity.width = sv.x;
        currentVelocity.height = sv.y;
        if (mAsyncScroll) {
          if (mAsyncScroll->mIsSmoothScroll) {
            currentVelocity = mAsyncScroll->VelocityAt(now);
          }
          mAsyncScroll = nullptr;
        }

        if (gfxPrefs::AsyncPanZoomEnabled()) {
          
          
          
          mLastSmoothScrollOrigin = aOrigin;
          mScrollGeneration = ++sScrollGenerationCounter;

          if (!nsLayoutUtils::GetDisplayPort(mOuter->GetContent())) {
            
            
            
            
            nsLayoutUtils::CalculateAndSetDisplayPortMargins(
              mOuter->GetScrollTargetFrame(),
              nsLayoutUtils::RepaintMode::DoNotRepaint);
          }

          
          
          mOuter->SchedulePaint();
          return;
        }

        mAsyncSmoothMSDScroll =
          new AsyncSmoothMSDScroll(GetScrollPosition(), mDestination,
                                   currentVelocity, GetScrollRangeForClamping(),
                                   now);

        if (!mAsyncSmoothMSDScroll->SetRefreshObserver(this)) {
          
          CompleteAsyncScroll(range, aOrigin);
          return;
        }
      } else {
        
        
        mAsyncSmoothMSDScroll->SetDestination(mDestination);
      }

      return;
    } else {
      if (mAsyncSmoothMSDScroll) {
        currentVelocity = mAsyncSmoothMSDScroll->GetVelocity();
        mAsyncSmoothMSDScroll = nullptr;
      }
    }
  }

  if (!mAsyncScroll) {
    mAsyncScroll = new AsyncScroll(GetScrollPosition());
    if (!mAsyncScroll->SetRefreshObserver(this)) {
      
      CompleteAsyncScroll(range, aOrigin);
      return;
    }
  }

  mAsyncScroll->mIsSmoothScroll = isSmoothScroll;

  if (isSmoothScroll) {
    mAsyncScroll->InitSmoothScroll(now, mDestination, aOrigin, range, currentVelocity);
  } else {
    mAsyncScroll->Init(range);
  }
}



static void AdjustViews(nsIFrame* aFrame)
{
  nsView* view = aFrame->GetView();
  if (view) {
    nsPoint pt;
    aFrame->GetParent()->GetClosestView(&pt);
    pt += aFrame->GetPosition();
    view->SetPosition(pt.x, pt.y);

    return;
  }

  if (!(aFrame->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    return;
  }

  
  
  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    if (lists.CurrentID() == nsIFrame::kPopupList) {
      continue;
    }
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      AdjustViews(childFrames.get());
    }
  }
}

static bool
NeedToInvalidateOnScroll(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() & NS_SCROLLFRAME_INVALIDATE_CONTENTS_ON_SCROLL) != 0;
}

bool ScrollFrameHelper::IsIgnoringViewportClipping() const
{
  if (!mIsRoot)
    return false;
  nsSubDocumentFrame* subdocFrame = static_cast<nsSubDocumentFrame*>
    (nsLayoutUtils::GetCrossDocParentFrame(mOuter->PresContext()->PresShell()->GetRootFrame()));
  return subdocFrame && !subdocFrame->ShouldClipSubdocument();
}

void ScrollFrameHelper::MarkScrollbarsDirtyForReflow() const
{
  nsIPresShell* presShell = mOuter->PresContext()->PresShell();
  if (mVScrollbarBox) {
    presShell->FrameNeedsReflow(mVScrollbarBox, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
  }
  if (mHScrollbarBox) {
    presShell->FrameNeedsReflow(mHScrollbarBox, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
  }
}

bool ScrollFrameHelper::ShouldClampScrollPosition() const
{
  if (!mIsRoot)
    return true;
  nsSubDocumentFrame* subdocFrame = static_cast<nsSubDocumentFrame*>
    (nsLayoutUtils::GetCrossDocParentFrame(mOuter->PresContext()->PresShell()->GetRootFrame()));
  return !subdocFrame || subdocFrame->ShouldClampScrollPosition();
}

bool ScrollFrameHelper::IsAlwaysActive() const
{
  if (nsDisplayItem::ForceActiveLayers()) {
    return true;
  }

  
  
  
  if (!(mIsRoot && mOuter->PresContext()->IsRootContentDocument())) {
     return false;
  }

  
  if (mHasBeenScrolled) {
    return true;
  }

  
  
  ScrollbarStyles styles = GetScrollbarStylesFromFrame();
  return (styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN &&
          styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN);
}

void ScrollFrameHelper::MarkNotRecentlyScrolled()
{
  if (!mHasBeenScrolledRecently)
    return;

  mHasBeenScrolledRecently = false;
  mOuter->SchedulePaint();
}

void ScrollFrameHelper::MarkRecentlyScrolled()
{
  mHasBeenScrolledRecently = true;
  if (IsAlwaysActive())
    return;

  if (mActivityExpirationState.IsTracked()) {
    gScrollFrameActivityTracker->MarkUsed(this);
  } else {
    if (!gScrollFrameActivityTracker) {
      gScrollFrameActivityTracker = new ScrollFrameActivityTracker();
    }
    gScrollFrameActivityTracker->AddObject(this);
  }
}

void ScrollFrameHelper::ScrollVisual(nsPoint aOldScrolledFramePos)
{
  
  
  
  
  mHasBeenScrolled = true;

  AdjustViews(mScrolledFrame);
  
  
  bool needToInvalidateOnScroll = NeedToInvalidateOnScroll(mOuter);
  mOuter->RemoveStateBits(NS_SCROLLFRAME_INVALIDATE_CONTENTS_ON_SCROLL);
  if (needToInvalidateOnScroll) {
    MarkNotRecentlyScrolled();
  } else {
    MarkRecentlyScrolled();
  }

  mOuter->SchedulePaint();
}










static nscoord
ClampAndAlignWithPixels(nscoord aDesired,
                        nscoord aBoundLower, nscoord aBoundUpper,
                        nscoord aDestLower, nscoord aDestUpper,
                        nscoord aAppUnitsPerPixel, double aRes,
                        nscoord aCurrent)
{
  
  
  nscoord destLower = clamped(aDestLower, aBoundLower, aBoundUpper);
  nscoord destUpper = clamped(aDestUpper, aBoundLower, aBoundUpper);

  nscoord desired = clamped(aDesired, destLower, destUpper);

  double currentLayerVal = (aRes*aCurrent)/aAppUnitsPerPixel;
  double desiredLayerVal = (aRes*desired)/aAppUnitsPerPixel;
  double delta = desiredLayerVal - currentLayerVal;
  double nearestLayerVal = NS_round(delta) + currentLayerVal;

  
  
  nscoord aligned =
    NSToCoordRoundWithClamp(nearestLayerVal*aAppUnitsPerPixel/aRes);

  
  
  if (aBoundUpper == destUpper &&
      static_cast<decltype(Abs(desired))>(aBoundUpper - desired) <
      Abs(desired - aligned))
    return aBoundUpper;

  if (aBoundLower == destLower &&
      static_cast<decltype(Abs(desired))>(desired - aBoundLower) <
      Abs(aligned - desired))
    return aBoundLower;

  
  if (aligned >= destLower && aligned <= destUpper)
    return aligned;

  
  double oppositeLayerVal =
    nearestLayerVal + ((nearestLayerVal < desiredLayerVal) ? 1.0 : -1.0);
  nscoord opposite =
    NSToCoordRoundWithClamp(oppositeLayerVal*aAppUnitsPerPixel/aRes);
  if (opposite >= destLower && opposite <= destUpper) {
    return opposite;
  }

  
  return desired;
}






static nsPoint
ClampAndAlignWithLayerPixels(const nsPoint& aPt,
                             const nsRect& aBounds,
                             const nsRect& aRange,
                             const nsPoint& aCurrent,
                             nscoord aAppUnitsPerPixel,
                             const gfxSize& aScale)
{
  return nsPoint(ClampAndAlignWithPixels(aPt.x, aBounds.x, aBounds.XMost(),
                                         aRange.x, aRange.XMost(),
                                         aAppUnitsPerPixel, aScale.width,
                                         aCurrent.x),
                 ClampAndAlignWithPixels(aPt.y, aBounds.y, aBounds.YMost(),
                                         aRange.y, aRange.YMost(),
                                         aAppUnitsPerPixel, aScale.height,
                                         aCurrent.y));
}

 void
ScrollFrameHelper::ScrollActivityCallback(nsITimer *aTimer, void* anInstance)
{
  ScrollFrameHelper* self = static_cast<ScrollFrameHelper*>(anInstance);

  
  self->mScrollActivityTimer->Cancel();
  self->mScrollActivityTimer = nullptr;
  self->mOuter->PresContext()->PresShell()->SynthesizeMouseMove(true);
}


void
ScrollFrameHelper::ScheduleSyntheticMouseMove()
{
  if (!mScrollActivityTimer) {
    mScrollActivityTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mScrollActivityTimer)
      return;
  }

  mScrollActivityTimer->InitWithFuncCallback(
        ScrollActivityCallback, this, 100, nsITimer::TYPE_ONE_SHOT);
}

void
ScrollFrameHelper::ScrollToImpl(nsPoint aPt, const nsRect& aRange, nsIAtom* aOrigin)
{
  if (aOrigin == nullptr) {
    
    
    
    aOrigin = nsGkAtoms::other;
  }

  nsPresContext* presContext = mOuter->PresContext();
  nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  
  
  gfxSize scale = FrameLayerBuilder::GetPaintedLayerScaleForFrame(mScrolledFrame);
  nsPoint curPos = GetScrollPosition();
  nsPoint alignWithPos = mScrollPosForLayerPixelAlignment == nsPoint(-1,-1)
      ? curPos : mScrollPosForLayerPixelAlignment;
  
  
  
  
  
  
  
  
  
  
  nsPoint pt =
    ClampAndAlignWithLayerPixels(aPt,
                                 GetScrollRangeForClamping(),
                                 aRange,
                                 alignWithPos,
                                 appUnitsPerDevPixel,
                                 scale);
  if (pt == curPos) {
    return;
  }

  bool needImageVisibilityUpdate = (mLastUpdateImagesPos == nsPoint(-1,-1));

  nsPoint dist(std::abs(pt.x - mLastUpdateImagesPos.x),
               std::abs(pt.y - mLastUpdateImagesPos.y));
  nsSize scrollPortSize = GetScrollPositionClampingScrollPortSize();
  nscoord horzAllowance = std::max(scrollPortSize.width / std::max(sHorzScrollFraction, 1),
                                   nsPresContext::AppUnitsPerCSSPixel());
  nscoord vertAllowance = std::max(scrollPortSize.height / std::max(sVertScrollFraction, 1),
                                   nsPresContext::AppUnitsPerCSSPixel());
  if (dist.x >= horzAllowance || dist.y >= vertAllowance) {
    needImageVisibilityUpdate = true;
  }

  if (needImageVisibilityUpdate) {
    presContext->PresShell()->ScheduleImageVisibilityUpdate();
  }

  
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->ScrollPositionWillChange(pt.x, pt.y);
  }

  nsPoint oldScrollFramePos = mScrolledFrame->GetPosition();
  
  mScrolledFrame->SetPosition(mScrollPort.TopLeft() - pt);
  mLastScrollOrigin = aOrigin;
  mLastSmoothScrollOrigin = nullptr;
  mScrollGeneration = ++sScrollGenerationCounter;

  
  ScrollVisual(oldScrollFramePos);

  if (mOuter->ChildrenHavePerspective()) {
    
    
    mOuter->RecomputePerspectiveChildrenOverflow(mOuter->StyleContext(), nullptr);
  }

  ScheduleSyntheticMouseMove();
  nsWeakFrame weakFrame(mOuter);
  UpdateScrollbarPosition();
  if (!weakFrame.IsAlive()) {
    return;
  }
  PostScrollEvent();

  
  for (uint32_t i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->ScrollPositionDidChange(pt.x, pt.y);
  }

  nsCOMPtr<nsIDocShell> docShell = presContext->GetDocShell();
  if (docShell) {
    docShell->NotifyScrollObservers();
  }
}

static int32_t
MaxZIndexInList(nsDisplayList* aList, nsDisplayListBuilder* aBuilder)
{
  int32_t maxZIndex = 0;
  for (nsDisplayItem* item = aList->GetBottom(); item; item = item->GetAbove()) {
    maxZIndex = std::max(maxZIndex, item->ZIndex());
  }
  return maxZIndex;
}





static int32_t
MaxZIndexInListOfItemsContainedInFrame(nsDisplayList* aList, nsIFrame* aFrame)
{
  int32_t maxZIndex = -1;
  for (nsDisplayItem* item = aList->GetBottom(); item; item = item->GetAbove()) {
    if (nsLayoutUtils::IsProperAncestorFrame(aFrame, item->Frame())) {
      maxZIndex = std::max(maxZIndex, item->ZIndex());
    }
  }
  return maxZIndex;
}

static const uint32_t APPEND_OWN_LAYER = 0x1;
static const uint32_t APPEND_POSITIONED = 0x2;
static const uint32_t APPEND_SCROLLBAR_CONTAINER = 0x4;

static void
AppendToTop(nsDisplayListBuilder* aBuilder, const nsDisplayListSet& aLists,
            nsDisplayList* aSource, nsIFrame* aSourceFrame, uint32_t aFlags)
{
  if (aSource->IsEmpty())
    return;

  nsDisplayWrapList* newItem;
  if (aFlags & APPEND_OWN_LAYER) {
    uint32_t flags = (aFlags & APPEND_SCROLLBAR_CONTAINER)
                     ? nsDisplayOwnLayer::SCROLLBAR_CONTAINER
                     : 0;
    newItem = new (aBuilder) nsDisplayOwnLayer(aBuilder, aSourceFrame, aSource, flags);
  } else {
    newItem = new (aBuilder) nsDisplayWrapList(aBuilder, aSourceFrame, aSource);
  }

  if (aFlags & APPEND_POSITIONED) {
    
    
    
    nsDisplayList* positionedDescendants = aLists.PositionedDescendants();
    if (!positionedDescendants->IsEmpty()) {
      newItem->SetOverrideZIndex(MaxZIndexInList(positionedDescendants, aBuilder));
      positionedDescendants->AppendNewToTop(newItem);
    } else {
      aLists.Outlines()->AppendNewToTop(newItem);
    }
  } else {
    aLists.BorderBackground()->AppendNewToTop(newItem);
  }
}

struct HoveredStateComparator
{
  bool Equals(nsIFrame* A, nsIFrame* B) const {
    bool aHovered = A->GetContent()->HasAttr(kNameSpaceID_None,
                                             nsGkAtoms::hover);
    bool bHovered = B->GetContent()->HasAttr(kNameSpaceID_None,
                                             nsGkAtoms::hover);
    return aHovered == bHovered;
  }
  bool LessThan(nsIFrame* A, nsIFrame* B) const {
    bool aHovered = A->GetContent()->HasAttr(kNameSpaceID_None,
                                             nsGkAtoms::hover);
    bool bHovered = B->GetContent()->HasAttr(kNameSpaceID_None,
                                             nsGkAtoms::hover);
    return !aHovered && bHovered;
  }
};

void
ScrollFrameHelper::AppendScrollPartsTo(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists,
                                           bool                    aUsingDisplayPort,
                                           bool                    aCreateLayer,
                                           bool                    aPositioned)
{
  nsITheme* theme = mOuter->PresContext()->GetTheme();
  if (theme &&
      theme->ShouldHideScrollbars()) {
    return;
  }

  bool overlayScrollbars =
    LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0;

  nsAutoTArray<nsIFrame*, 3> scrollParts;
  for (nsIFrame* kid = mOuter->GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
    if (kid == mScrolledFrame ||
        (kid->IsAbsPosContaininingBlock() || overlayScrollbars) != aPositioned)
      continue;

    scrollParts.AppendElement(kid);
  }
  if (scrollParts.IsEmpty()) {
    return;
  }

  
  
  
  mozilla::layers::FrameMetrics::ViewID scrollTargetId = IsMaybeScrollingActive()
    ? nsLayoutUtils::FindOrCreateIDFor(mScrolledFrame->GetContent())
    : mozilla::layers::FrameMetrics::NULL_SCROLL_ID;

  scrollParts.Sort(HoveredStateComparator());

  DisplayListClipState::AutoSaveRestore clipState(aBuilder);
  
  
  
  if (mIsRoot) {
    clipState.ClipContentDescendants(
        mOuter->GetRectRelativeToSelf() + aBuilder->ToReferenceFrame(mOuter));
  }

  for (uint32_t i = 0; i < scrollParts.Length(); ++i) {
    uint32_t flags = 0;
    uint32_t appendToTopFlags = 0;
    if (scrollParts[i] == mVScrollbarBox) {
      flags |= nsDisplayOwnLayer::VERTICAL_SCROLLBAR;
      appendToTopFlags |= APPEND_SCROLLBAR_CONTAINER;
    }
    if (scrollParts[i] == mHScrollbarBox) {
      flags |= nsDisplayOwnLayer::HORIZONTAL_SCROLLBAR;
      appendToTopFlags |= APPEND_SCROLLBAR_CONTAINER;
    }

    
    
    nsRect dirty = aUsingDisplayPort ?
      scrollParts[i]->GetVisualOverflowRectRelativeToParent() : aDirtyRect;
    nsDisplayListBuilder::AutoBuildingDisplayList
      buildingForChild(aBuilder, scrollParts[i],
                       dirty + mOuter->GetOffsetTo(scrollParts[i]), true);
    nsDisplayListBuilder::AutoCurrentScrollbarInfoSetter
      infoSetter(aBuilder, scrollTargetId, flags);
    nsDisplayListCollection partList;
    mOuter->BuildDisplayListForChild(
      aBuilder, scrollParts[i], dirty, partList,
      nsIFrame::DISPLAY_CHILD_FORCE_STACKING_CONTEXT);

    
    
    bool isOverlayScrollbar = (flags != 0) && overlayScrollbars;
    if (aCreateLayer || isOverlayScrollbar) {
      appendToTopFlags |= APPEND_OWN_LAYER;
    }
    if (aPositioned) {
      appendToTopFlags |= APPEND_POSITIONED;
    }

    
    
    ::AppendToTop(aBuilder, aLists,
                  partList.PositionedDescendants(), scrollParts[i],
                  appendToTopFlags);
  }
}

 bool ScrollFrameHelper::sImageVisPrefsCached = false;
 uint32_t ScrollFrameHelper::sHorzExpandScrollPort = 0;
 uint32_t ScrollFrameHelper::sVertExpandScrollPort = 1;
 int32_t ScrollFrameHelper::sHorzScrollFraction = 2;
 int32_t ScrollFrameHelper::sVertScrollFraction = 2;

 void
ScrollFrameHelper::EnsureImageVisPrefsCached()
{
  if (!sImageVisPrefsCached) {
    Preferences::AddUintVarCache(&sHorzExpandScrollPort,
      "layout.imagevisibility.numscrollportwidths", (uint32_t)0);
    Preferences::AddUintVarCache(&sVertExpandScrollPort,
      "layout.imagevisibility.numscrollportheights", 1);

    Preferences::AddIntVarCache(&sHorzScrollFraction,
      "layout.imagevisibility.amountscrollbeforeupdatehorizontal", 2);
    Preferences::AddIntVarCache(&sVertScrollFraction,
      "layout.imagevisibility.amountscrollbeforeupdatevertical", 2);

    sImageVisPrefsCached = true;
  }
}

nsRect
ScrollFrameHelper::ExpandRectToNearlyVisible(const nsRect& aRect) const
{
  
  
  nsRect scrollRange = GetScrollRangeForClamping();
  nsPoint scrollPos = GetScrollPosition();
  nsMargin expand(0, 0, 0, 0);

  nscoord vertShift = sVertExpandScrollPort * aRect.height;
  if (scrollRange.y < scrollPos.y) {
    expand.top = vertShift;
  }
  if (scrollPos.y < scrollRange.YMost()) {
    expand.bottom = vertShift;
  }

  nscoord horzShift = sHorzExpandScrollPort * aRect.width;
  if (scrollRange.x < scrollPos.x) {
    expand.left = horzShift;
  }
  if (scrollPos.x < scrollRange.XMost()) {
    expand.right = horzShift;
  }

  nsRect rect = aRect;
  rect.Inflate(expand);
  return rect;
}

static bool
ShouldBeClippedByFrame(nsIFrame* aClipFrame, nsIFrame* aClippedFrame)
{
  return nsLayoutUtils::IsProperAncestorFrame(aClipFrame, aClippedFrame);
}

static void
ClipItemsExceptCaret(nsDisplayList* aList, nsDisplayListBuilder* aBuilder,
                     nsIFrame* aClipFrame, const DisplayItemClip& aClip)
{
  nsDisplayItem* i = aList->GetBottom();
  for (; i; i = i->GetAbove()) {
    if (!::ShouldBeClippedByFrame(aClipFrame, i->Frame())) {
      continue;
    }

    bool unused;
    nsRect bounds = i->GetBounds(aBuilder, &unused);
    bool isAffectedByClip = aClip.IsRectAffectedByClip(bounds);
    if (isAffectedByClip && nsDisplayItem::TYPE_CARET == i->GetType()) {
      
      
      
      auto half = bounds.height / 2;
      bounds.y += half;
      bounds.height -= half;
      isAffectedByClip = aClip.IsRectAffectedByClip(bounds);
      if (isAffectedByClip) {
        
        nsRect rightSide(bounds.x - 1, bounds.y, 1, bounds.height);
        isAffectedByClip = aClip.IsRectAffectedByClip(rightSide);
        
        if (isAffectedByClip) {
          isAffectedByClip = i->Frame()->GetRect().height != 0;
        }
      }
    }
    if (isAffectedByClip) {
      DisplayItemClip newClip;
      newClip.IntersectWith(i->GetClip());
      newClip.IntersectWith(aClip);
      i->SetClip(aBuilder, newClip);
    }
    nsDisplayList* children = i->GetSameCoordinateSystemChildren();
    if (children) {
      ClipItemsExceptCaret(children, aBuilder, aClipFrame, aClip);
    }
  }
}

static void
ClipListsExceptCaret(nsDisplayListCollection* aLists,
                     nsDisplayListBuilder* aBuilder,
                     nsIFrame* aClipFrame,
                     const DisplayItemClip& aClip)
{
  ::ClipItemsExceptCaret(aLists->BorderBackground(), aBuilder, aClipFrame, aClip);
  ::ClipItemsExceptCaret(aLists->BlockBorderBackgrounds(), aBuilder, aClipFrame, aClip);
  ::ClipItemsExceptCaret(aLists->Floats(), aBuilder, aClipFrame, aClip);
  ::ClipItemsExceptCaret(aLists->PositionedDescendants(), aBuilder, aClipFrame, aClip);
  ::ClipItemsExceptCaret(aLists->Outlines(), aBuilder, aClipFrame, aClip);
  ::ClipItemsExceptCaret(aLists->Content(), aBuilder, aClipFrame, aClip);
}

void
ScrollFrameHelper::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  if (aBuilder->IsForImageVisibility()) {
    mLastUpdateImagesPos = GetScrollPosition();
  }

  mOuter->DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (aBuilder->IsPaintingToWindow()) {
    mScrollPosAtLastPaint = GetScrollPosition();
    if (IsMaybeScrollingActive() && NeedToInvalidateOnScroll(mOuter)) {
      MarkNotRecentlyScrolled();
    }
    if (IsMaybeScrollingActive()) {
      if (mScrollPosForLayerPixelAlignment == nsPoint(-1,-1)) {
        mScrollPosForLayerPixelAlignment = mScrollPosAtLastPaint;
      }
    } else {
      mScrollPosForLayerPixelAlignment = nsPoint(-1,-1);
    }
  }

  
  
  
  
  
  
  
  
  bool createLayersForScrollbars = mIsRoot &&
    mOuter->PresContext()->IsRootContentDocument();

  if (aBuilder->GetIgnoreScrollFrame() == mOuter || IsIgnoringViewportClipping()) {
    
    
    mAddClipRectToLayer = false;

    
    
    
    bool usingDisplayPort = aBuilder->IsPaintingToWindow() &&
        nsLayoutUtils::GetDisplayPort(mOuter->GetContent());

    if (usingDisplayPort) {
      
      
      mShouldBuildScrollableLayer = true;
      mIsScrollableLayerInRootContainer = true;
    }

    bool addScrollBars = mIsRoot && usingDisplayPort && !aBuilder->IsForEventDelivery();

    if (addScrollBars) {
      
      AppendScrollPartsTo(aBuilder, aDirtyRect, aLists, usingDisplayPort,
                          createLayersForScrollbars, false);
    }

    
    
    
    mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame,
                                     aDirtyRect, aLists);

    if (addScrollBars) {
      
      AppendScrollPartsTo(aBuilder, aDirtyRect, aLists, usingDisplayPort,
                          createLayersForScrollbars, true);
    }

    return;
  }

  
  
  mAddClipRectToLayer =
    !(mIsRoot && mOuter->PresContext()->PresShell()->GetIsViewportOverridden());

  
  
  
  
  
  
  
  
  nsRect dirtyRect = aDirtyRect.Intersect(mScrollPort);

  nsRect displayPort;
  bool usingDisplayport = false;
  if (aBuilder->IsPaintingToWindow()) {
    bool wasUsingDisplayPort = nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), nullptr);

    if (mIsRoot) {
      if (gfxPrefs::LayoutUseContainersForRootFrames()) {
        
        
        usingDisplayport = nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &displayPort);
      } else {
        
        
        nsRect displayportBase = dirtyRect;
        if (mIsRoot && mOuter->PresContext()->IsRootContentDocument()) {
          displayportBase =
            nsRect(nsPoint(0, 0), nsLayoutUtils::CalculateCompositionSizeForFrame(mOuter));
        }
        usingDisplayport = nsLayoutUtils::GetOrMaybeCreateDisplayPort(
              *aBuilder, mOuter, displayportBase, &displayPort);
      }
    } else {
      
      
      
      
      nsRect displayportBase = dirtyRect;
      usingDisplayport = nsLayoutUtils::GetOrMaybeCreateDisplayPort(
          *aBuilder, mOuter, displayportBase, &displayPort);
    }

    bool usingLowPrecision = gfxPrefs::UseLowPrecisionBuffer();
    if (usingDisplayport && usingLowPrecision) {
      
      nsRect critDp;
      nsLayoutUtils::GetCriticalDisplayPort(mOuter->GetContent(), &critDp);
    }

    
    if (usingDisplayport) {
      dirtyRect = displayPort;

      
      
      if (!wasUsingDisplayPort) {
        aBuilder->RecomputeCurrentAnimatedGeometryRoot();
      }
    }
  }

  
  
  
  
  
  
  
  AppendScrollPartsTo(aBuilder, aDirtyRect, aLists, usingDisplayport,
                      createLayersForScrollbars, false);

  if (aBuilder->IsForImageVisibility()) {
    
    
    
    
    dirtyRect = ExpandRectToNearlyVisible(dirtyRect);
  }

  const nsStyleDisplay* disp = mOuter->StyleDisplay();
  if (disp && (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_SCROLL)) {
    aBuilder->AddToWillChangeBudget(mOuter, GetScrollPositionClampingScrollPortSize());
  }

  
  
  
  
  
  
  
  
  
  mShouldBuildScrollableLayer = usingDisplayport || nsContentUtils::HasScrollgrab(mOuter->GetContent());
  bool shouldBuildLayer = false;
  if (mShouldBuildScrollableLayer) {
    shouldBuildLayer = true;
  } else {
    shouldBuildLayer =
      gfxPrefs::AsyncPanZoomEnabled() &&
      WantAsyncScroll() &&
      
      
      
      
      (!(gfxPrefs::LayoutUseContainersForRootFrames() && mIsRoot) ||
       (aBuilder->RootReferenceFrame()->PresContext() != mOuter->PresContext()));
  }

  mScrollParentID = aBuilder->GetCurrentScrollParentId();

  nsDisplayListCollection scrolledContent;
  {
    
    
    
    
    
    nsDisplayListBuilder::AutoCurrentScrollParentIdSetter idSetter(
        aBuilder,
        shouldBuildLayer && mScrolledFrame->GetContent()
            ? nsLayoutUtils::FindOrCreateIDFor(mScrolledFrame->GetContent())
            : aBuilder->GetCurrentScrollParentId());
    DisplayListClipState::AutoSaveRestore clipState(aBuilder);

    if (usingDisplayport) {
      
      
      
      
      
      
      
      
      
      
      
      
      clipState.Clear();
    } else {
      nsRect clip = mScrollPort + aBuilder->ToReferenceFrame(mOuter);
      nscoord radii[8];
      bool haveRadii = mOuter->GetPaddingBoxBorderRadii(radii);
      
      
      if (mClipAllDescendants) {
        clipState.ClipContentDescendants(clip, haveRadii ? radii : nullptr);
      } else {
        clipState.ClipContainingBlockDescendants(clip, haveRadii ? radii : nullptr);
      }
    }

    mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame, dirtyRect, scrolledContent);

    if (idSetter.ShouldForceLayerForScrollParent() &&
        !gfxPrefs::LayoutUseContainersForRootFrames())
    {
      
      
      
      
      
      MOZ_ASSERT(shouldBuildLayer && mScrolledFrame->GetContent());
      mShouldBuildScrollableLayer = true;
    }
  }

  if (mShouldBuildScrollableLayer && !gfxPrefs::LayoutUseContainersForRootFrames()) {
    aBuilder->ForceLayerForScrollParent();
  }

  if (MOZ_UNLIKELY(mOuter->StyleDisplay()->mOverflowClipBox ==
                     NS_STYLE_OVERFLOW_CLIP_BOX_CONTENT_BOX)) {
    
    
    nsRect clipRect = mScrollPort + aBuilder->ToReferenceFrame(mOuter);
    nsRect so = mScrolledFrame->GetScrollableOverflowRect();
    if (clipRect.width != so.width || clipRect.height != so.height ||
        so.x < 0 || so.y < 0) {
      
      
      
      
      clipRect.Deflate(mOuter->GetUsedPadding());
      DisplayItemClip clip;
      clip.SetTo(clipRect);
      ::ClipListsExceptCaret(&scrolledContent, aBuilder, mScrolledFrame, clip);
    }
  }

  if (shouldBuildLayer) {
    
    
    nsDisplayLayerEventRegions* inactiveRegionItem = nullptr;
    if (aBuilder->IsPaintingToWindow() &&
        !mShouldBuildScrollableLayer &&
        shouldBuildLayer &&
        aBuilder->IsBuildingLayerEventRegions())
    {
      inactiveRegionItem = new (aBuilder) nsDisplayLayerEventRegions(aBuilder, mScrolledFrame);
      inactiveRegionItem->AddInactiveScrollPort(mScrollPort + aBuilder->ToReferenceFrame(mOuter));
    }

    
    
    
    nsDisplayScrollInfoLayer* layerItem = new (aBuilder) nsDisplayScrollInfoLayer(
      aBuilder, mScrolledFrame, mOuter);

    nsDisplayList* positionedDescendants = scrolledContent.PositionedDescendants();
    nsDisplayList* destinationList = nullptr;
    int32_t zindex =
      MaxZIndexInListOfItemsContainedInFrame(positionedDescendants, mOuter);
    if (zindex >= 0) {
      layerItem->SetOverrideZIndex(zindex);
      destinationList = positionedDescendants;
    } else {
      destinationList = scrolledContent.Outlines();
    }
    if (inactiveRegionItem) {
      destinationList->AppendNewToTop(inactiveRegionItem);
    }
    destinationList->AppendNewToTop(layerItem);
  }
  
  AppendScrollPartsTo(aBuilder, aDirtyRect, scrolledContent, usingDisplayport,
                      createLayersForScrollbars, true);
  scrolledContent.MoveTo(aLists);
}

void
ScrollFrameHelper::ComputeFrameMetrics(Layer* aLayer,
                                       nsIFrame* aContainerReferenceFrame,
                                       const ContainerLayerParameters& aParameters,
                                       nsRect* aClipRect,
                                       nsTArray<FrameMetrics>* aOutput) const
{
  nsPoint toReferenceFrame = mOuter->GetOffsetToCrossDoc(aContainerReferenceFrame);
  bool isRoot = mIsRoot && mOuter->PresContext()->IsRootContentDocument();
  
  
  
  
  bool omitClip = gfxPrefs::AsyncPanZoomEnabled() && aOutput->Length() > 0;
  if (!omitClip && (!gfxPrefs::LayoutUseContainersForRootFrames() || mAddClipRectToLayer)) {
    nsRect clip = nsRect(mScrollPort.TopLeft() + toReferenceFrame,
                         nsLayoutUtils::CalculateCompositionSizeForFrame(mOuter));
    if (isRoot) {
      double res = mOuter->PresContext()->PresShell()->GetResolution();
      clip.width = NSToCoordRound(clip.width / res);
      clip.height = NSToCoordRound(clip.height / res);
    }
    
    
    *aClipRect = clip;
  }

  if (!mShouldBuildScrollableLayer || mIsScrollableLayerInRootContainer) {
    return;
  }

  MOZ_ASSERT(mScrolledFrame->GetContent());

  nsRect scrollport = mScrollPort + toReferenceFrame;
  *aOutput->AppendElement() =
      nsLayoutUtils::ComputeFrameMetrics(
        mScrolledFrame, mOuter, mOuter->GetContent(),
        aContainerReferenceFrame, aLayer, mScrollParentID,
        scrollport, isRoot, aParameters);
}

bool
ScrollFrameHelper::IsRectNearlyVisible(const nsRect& aRect) const
{
  
  nsRect displayPort;
  bool usingDisplayport = nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &displayPort);
  return aRect.Intersects(ExpandRectToNearlyVisible(usingDisplayport ? displayPort : mScrollPort));
}

static void HandleScrollPref(nsIScrollable *aScrollable, int32_t aOrientation,
                             uint8_t& aValue)
{
  int32_t pref;
  aScrollable->GetDefaultScrollbarPreferences(aOrientation, &pref);
  switch (pref) {
    case nsIScrollable::Scrollbar_Auto:
      
      break;
    case nsIScrollable::Scrollbar_Never:
      aValue = NS_STYLE_OVERFLOW_HIDDEN;
      break;
    case nsIScrollable::Scrollbar_Always:
      aValue = NS_STYLE_OVERFLOW_SCROLL;
      break;
  }
}

ScrollbarStyles
ScrollFrameHelper::GetScrollbarStylesFromFrame() const
{
  nsPresContext* presContext = mOuter->PresContext();
  if (!presContext->IsDynamic() &&
      !(mIsRoot && presContext->HasPaginatedScrolling())) {
    return ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, NS_STYLE_OVERFLOW_HIDDEN);
  }

  if (!mIsRoot) {
    const nsStyleDisplay* disp = mOuter->StyleDisplay();
    return ScrollbarStyles(disp);
  }

  ScrollbarStyles result = presContext->GetViewportScrollbarStylesOverride();
  nsCOMPtr<nsISupports> container = presContext->GetContainerWeak();
  nsCOMPtr<nsIScrollable> scrollable = do_QueryInterface(container);
  if (scrollable) {
    HandleScrollPref(scrollable, nsIScrollable::ScrollOrientation_X,
                     result.mHorizontal);
    HandleScrollPref(scrollable, nsIScrollable::ScrollOrientation_Y,
                     result.mVertical);
  }
  return result;
}

nsRect
ScrollFrameHelper::GetScrollRange() const
{
  return GetScrollRange(mScrollPort.width, mScrollPort.height);
}

nsRect
ScrollFrameHelper::GetScrollRange(nscoord aWidth, nscoord aHeight) const
{
  nsRect range = GetScrolledRect();
  range.width = std::max(range.width - aWidth, 0);
  range.height = std::max(range.height - aHeight, 0);
  return range;
}

nsRect
ScrollFrameHelper::GetScrollRangeForClamping() const
{
  if (!ShouldClampScrollPosition()) {
    return nsRect(nscoord_MIN/2, nscoord_MIN/2,
                  nscoord_MAX - nscoord_MIN/2, nscoord_MAX - nscoord_MIN/2);
  }
  nsSize scrollPortSize = GetScrollPositionClampingScrollPortSize();
  return GetScrollRange(scrollPortSize.width, scrollPortSize.height);
}

nsSize
ScrollFrameHelper::GetScrollPositionClampingScrollPortSize() const
{
  nsIPresShell* presShell = mOuter->PresContext()->PresShell();
  if (mIsRoot && presShell->IsScrollPositionClampingScrollPortSizeSet()) {
    return presShell->GetScrollPositionClampingScrollPortSize();
  }
  return mScrollPort.Size();
}

float
ScrollFrameHelper::GetResolution() const
{
  return mResolution;
}

void
ScrollFrameHelper::SetResolution(float aResolution)
{
  mResolution = aResolution;
  mIsResolutionSet = true;
  mScaleToResolution = false;
}

void
ScrollFrameHelper::SetResolutionAndScaleTo(float aResolution)
{
  MOZ_ASSERT(mIsRoot);  
  mResolution = aResolution;
  mIsResolutionSet = true;
  mScaleToResolution = true;
}

static void
AdjustForWholeDelta(int32_t aDelta, nscoord* aCoord)
{
  if (aDelta < 0) {
    *aCoord = nscoord_MIN;
  } else if (aDelta > 0) {
    *aCoord = nscoord_MAX;
  }
}









static void
CalcRangeForScrollBy(int32_t aDelta, nscoord aPos,
                     float aNegTolerance,
                     float aPosTolerance,
                     nscoord aMultiplier,
                     nscoord* aLower, nscoord* aUpper)
{
  if (!aDelta) {
    *aLower = *aUpper = aPos;
    return;
  }
  *aLower = aPos - NSToCoordRound(aMultiplier * (aDelta > 0 ? aNegTolerance : aPosTolerance));
  *aUpper = aPos + NSToCoordRound(aMultiplier * (aDelta > 0 ? aPosTolerance : aNegTolerance));
}

void
ScrollFrameHelper::ScrollBy(nsIntPoint aDelta,
                            nsIScrollableFrame::ScrollUnit aUnit,
                            nsIScrollableFrame::ScrollMode aMode,
                            nsIntPoint* aOverflow,
                            nsIAtom *aOrigin,
                            nsIScrollableFrame::ScrollMomentum aMomentum,
                            nsIScrollbarMediator::ScrollSnapMode aSnap)
{
  
  
  
  
  switch (aMomentum) {
  case nsIScrollableFrame::NOT_MOMENTUM:
    mIgnoreMomentumScroll = false;
    break;
  case nsIScrollableFrame::SYNTHESIZED_MOMENTUM_EVENT:
    if (mIgnoreMomentumScroll) {
      return;
    }
    break;
  }

  if (mAsyncSmoothMSDScroll != nullptr) {
    
    
    
    mDestination = GetScrollPosition();
  }

  nsSize deltaMultiplier;
  float negativeTolerance;
  float positiveTolerance;
  if (!aOrigin){
    aOrigin = nsGkAtoms::other;
  }
  bool isGenericOrigin = (aOrigin == nsGkAtoms::other);
  switch (aUnit) {
  case nsIScrollableFrame::DEVICE_PIXELS: {
    nscoord appUnitsPerDevPixel =
      mOuter->PresContext()->AppUnitsPerDevPixel();
    deltaMultiplier = nsSize(appUnitsPerDevPixel, appUnitsPerDevPixel);
    if (isGenericOrigin){
      aOrigin = nsGkAtoms::pixels;
    }
    negativeTolerance = positiveTolerance = 0.5f;
    break;
  }
  case nsIScrollableFrame::LINES: {
    deltaMultiplier = GetLineScrollAmount();
    if (isGenericOrigin){
      aOrigin = nsGkAtoms::lines;
    }
    negativeTolerance = positiveTolerance = 0.1f;
    break;
  }
  case nsIScrollableFrame::PAGES: {
    deltaMultiplier = GetPageScrollAmount();
    if (isGenericOrigin){
      aOrigin = nsGkAtoms::pages;
    }
    negativeTolerance = 0.05f;
    positiveTolerance = 0;
    break;
  }
  case nsIScrollableFrame::WHOLE: {
    nsPoint pos = GetScrollPosition();
    AdjustForWholeDelta(aDelta.x, &pos.x);
    AdjustForWholeDelta(aDelta.y, &pos.y);
    if (aSnap == nsIScrollableFrame::ENABLE_SNAP) {
      GetSnapPointForDestination(aUnit, mDestination, pos);
    }
    ScrollTo(pos, aMode);
    
    if (aOverflow) {
      *aOverflow = nsIntPoint(0, 0);
    }
    return;
  }
  default:
    NS_ERROR("Invalid scroll mode");
    return;
  }

  nsPoint newPos = mDestination + nsPoint(aDelta.x*deltaMultiplier.width, aDelta.y*deltaMultiplier.height);

  if (aSnap == nsIScrollableFrame::ENABLE_SNAP) {
    ScrollbarStyles styles = GetScrollbarStylesFromFrame();
    if (styles.mScrollSnapTypeY != NS_STYLE_SCROLL_SNAP_TYPE_NONE ||
        styles.mScrollSnapTypeX != NS_STYLE_SCROLL_SNAP_TYPE_NONE) {
      nscoord appUnitsPerDevPixel = mOuter->PresContext()->AppUnitsPerDevPixel();
      deltaMultiplier = nsSize(appUnitsPerDevPixel, appUnitsPerDevPixel);
      negativeTolerance = 0.1f;
      positiveTolerance = 0;
      nsIScrollableFrame::ScrollUnit snapUnit = aUnit;
      if (aOrigin == nsGkAtoms::mouseWheel) {
        
        
        
        snapUnit = nsIScrollableFrame::LINES;
      }
      GetSnapPointForDestination(snapUnit, mDestination, newPos);
    }
  }

  
  nscoord rangeLowerX, rangeUpperX, rangeLowerY, rangeUpperY;
  CalcRangeForScrollBy(aDelta.x, newPos.x, negativeTolerance, positiveTolerance,
                       deltaMultiplier.width, &rangeLowerX, &rangeUpperX);
  CalcRangeForScrollBy(aDelta.y, newPos.y, negativeTolerance, positiveTolerance,
                       deltaMultiplier.height, &rangeLowerY, &rangeUpperY);
  nsRect range(rangeLowerX,
               rangeLowerY,
               rangeUpperX - rangeLowerX,
               rangeUpperY - rangeLowerY);
  nsWeakFrame weakFrame(mOuter);
  ScrollToWithOrigin(newPos, aMode, aOrigin, &range);
  if (!weakFrame.IsAlive()) {
    return;
  }

  if (aOverflow) {
    nsPoint clampAmount = newPos - mDestination;
    float appUnitsPerDevPixel = mOuter->PresContext()->AppUnitsPerDevPixel();
    *aOverflow = nsIntPoint(
        NSAppUnitsToIntPixels(clampAmount.x, appUnitsPerDevPixel),
        NSAppUnitsToIntPixels(clampAmount.y, appUnitsPerDevPixel));
  }

  if (aUnit == nsIScrollableFrame::DEVICE_PIXELS &&
      !gfxPrefs::AsyncPanZoomEnabled()) {
    
    
    mVelocityQueue.Sample(GetScrollPosition());
  }
}

void
ScrollFrameHelper::ScrollSnap(nsIScrollableFrame::ScrollMode aMode)
{
  float flingSensitivity = gfxPrefs::ScrollSnapPredictionSensitivity();
  int maxVelocity = gfxPrefs::ScrollSnapPredictionMaxVelocity();
  maxVelocity = nsPresContext::CSSPixelsToAppUnits(maxVelocity);
  int maxOffset = maxVelocity * flingSensitivity;
  nsPoint velocity = mVelocityQueue.GetVelocity();
  
  nsPoint predictedOffset = nsPoint(velocity.x * flingSensitivity,
                                    velocity.y * flingSensitivity);
  predictedOffset.Clamp(maxOffset);
  nsPoint pos = GetScrollPosition();
  nsPoint destinationPos = pos + predictedOffset;
  ScrollSnap(destinationPos, aMode);
}

void
ScrollFrameHelper::FlingSnap(const mozilla::CSSPoint& aDestination)
{
  ScrollSnap(CSSPoint::ToAppUnits(aDestination));
}

void
ScrollFrameHelper::ScrollSnap(const nsPoint &aDestination,
                              nsIScrollableFrame::ScrollMode aMode)
{
  nsRect scrollRange = GetScrollRangeForClamping();
  nsPoint pos = GetScrollPosition();
  nsPoint snapDestination = scrollRange.ClampPoint(aDestination);
  if (GetSnapPointForDestination(nsIScrollableFrame::DEVICE_PIXELS,
                                                 pos,
                                                 snapDestination)) {
    ScrollTo(snapDestination, aMode);
  }
}

nsSize
ScrollFrameHelper::GetLineScrollAmount() const
{
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(mOuter, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(mOuter));
  NS_ASSERTION(fm, "FontMetrics is null, assuming fontHeight == 1 appunit");
  static nscoord sMinLineScrollAmountInPixels = -1;
  if (sMinLineScrollAmountInPixels < 0) {
    Preferences::AddIntVarCache(&sMinLineScrollAmountInPixels,
                                "mousewheel.min_line_scroll_amount", 1);
  }
  int32_t appUnitsPerDevPixel = mOuter->PresContext()->AppUnitsPerDevPixel();
  nscoord minScrollAmountInAppUnits =
    std::max(1, sMinLineScrollAmountInPixels) * appUnitsPerDevPixel;
  nscoord horizontalAmount = fm ? fm->AveCharWidth() : 0;
  nscoord verticalAmount = fm ? fm->MaxHeight() : 0;
  return nsSize(std::max(horizontalAmount, minScrollAmountInAppUnits),
                std::max(verticalAmount, minScrollAmountInAppUnits));
}













struct TopAndBottom
{
  TopAndBottom(nscoord aTop, nscoord aBottom) : top(aTop), bottom(aBottom) {}

  nscoord top, bottom;
};
struct TopComparator
{
  bool Equals(const TopAndBottom& A, const TopAndBottom& B) const {
    return A.top == B.top;
  }
  bool LessThan(const TopAndBottom& A, const TopAndBottom& B) const {
    return A.top < B.top;
  }
};
struct ReverseBottomComparator
{
  bool Equals(const TopAndBottom& A, const TopAndBottom& B) const {
    return A.bottom == B.bottom;
  }
  bool LessThan(const TopAndBottom& A, const TopAndBottom& B) const {
    return A.bottom > B.bottom;
  }
};
static nsSize
GetScrollPortSizeExcludingHeadersAndFooters(nsIFrame* aViewportFrame,
                                            const nsRect& aScrollPort)
{
  nsTArray<TopAndBottom> list;
  nsFrameList fixedFrames = aViewportFrame->GetChildList(nsIFrame::kFixedList);
  for (nsFrameList::Enumerator iterator(fixedFrames); !iterator.AtEnd();
       iterator.Next()) {
    nsIFrame* f = iterator.get();
    nsRect r = f->GetRect().Intersect(aScrollPort);
    if (r.x == 0 && r.width == aScrollPort.width &&
        r.height <= aScrollPort.height/3) {
      list.AppendElement(TopAndBottom(r.y, r.YMost()));
    }
  }

  list.Sort(TopComparator());
  nscoord headerBottom = 0;
  for (uint32_t i = 0; i < list.Length(); ++i) {
    if (list[i].top <= headerBottom) {
      headerBottom = std::max(headerBottom, list[i].bottom);
    }
  }

  list.Sort(ReverseBottomComparator());
  nscoord footerTop = aScrollPort.height;
  for (uint32_t i = 0; i < list.Length(); ++i) {
    if (list[i].bottom >= footerTop) {
      footerTop = std::min(footerTop, list[i].top);
    }
  }

  headerBottom = std::min(aScrollPort.height/3, headerBottom);
  footerTop = std::max(aScrollPort.height - aScrollPort.height/3, footerTop);

  return nsSize(aScrollPort.width, footerTop - headerBottom);
}

nsSize
ScrollFrameHelper::GetPageScrollAmount() const
{
  nsSize lineScrollAmount = GetLineScrollAmount();
  nsSize effectiveScrollPortSize;
  if (mIsRoot) {
    
    
    nsIFrame* root = mOuter->PresContext()->PresShell()->GetRootFrame();
    effectiveScrollPortSize =
      GetScrollPortSizeExcludingHeadersAndFooters(root, mScrollPort);
  } else {
    effectiveScrollPortSize = mScrollPort.Size();
  }
  
  
  return nsSize(
    effectiveScrollPortSize.width -
      std::min(effectiveScrollPortSize.width/10, 2*lineScrollAmount.width),
    effectiveScrollPortSize.height -
      std::min(effectiveScrollPortSize.height/10, 2*lineScrollAmount.height));
}

  






void
ScrollFrameHelper::ScrollToRestoredPosition()
{
  if (mRestorePos.y == -1 || mLastPos.x == -1 || mLastPos.y == -1) {
    return;
  }
  
  
  
  
  
  
  

  
  if (GetLogicalScrollPosition() == mLastPos) {
    
    
    
    if (mRestorePos != mLastPos ) {
      nsPoint scrollToPos = mRestorePos;
      if (!IsLTR())
        
        scrollToPos.x = mScrollPort.x -
          (mScrollPort.XMost() - scrollToPos.x - mScrolledFrame->GetRect().width);
      nsWeakFrame weakFrame(mOuter);
      ScrollTo(scrollToPos, nsIScrollableFrame::INSTANT);
      if (!weakFrame.IsAlive()) {
        return;
      }
      
      
      mLastPos = GetLogicalScrollPosition();
    } else {
      
      mRestorePos.y = -1;
      mLastPos.x = -1;
      mLastPos.y = -1;
    }
  } else {
    
    mLastPos.x = -1;
    mLastPos.y = -1;
  }
}

nsresult
ScrollFrameHelper::FireScrollPortEvent()
{
  mAsyncScrollPortEvent.Forget();

  
  nsSize scrollportSize = mScrollPort.Size();
  nsSize childSize = GetScrolledRect().Size();

  bool newVerticalOverflow = childSize.height > scrollportSize.height;
  bool vertChanged = mVerticalOverflow != newVerticalOverflow;

  bool newHorizontalOverflow = childSize.width > scrollportSize.width;
  bool horizChanged = mHorizontalOverflow != newHorizontalOverflow;

  if (!vertChanged && !horizChanged) {
    return NS_OK;
  }

  
  
  bool both = vertChanged && horizChanged &&
                newVerticalOverflow == newHorizontalOverflow;
  InternalScrollPortEvent::orientType orient;
  if (both) {
    orient = InternalScrollPortEvent::both;
    mHorizontalOverflow = newHorizontalOverflow;
    mVerticalOverflow = newVerticalOverflow;
  }
  else if (vertChanged) {
    orient = InternalScrollPortEvent::vertical;
    mVerticalOverflow = newVerticalOverflow;
    if (horizChanged) {
      
      
      
      PostOverflowEvent();
    }
  }
  else {
    orient = InternalScrollPortEvent::horizontal;
    mHorizontalOverflow = newHorizontalOverflow;
  }

  InternalScrollPortEvent event(true,
    (orient == InternalScrollPortEvent::horizontal ? mHorizontalOverflow :
                                                     mVerticalOverflow) ?
    NS_SCROLLPORT_OVERFLOW : NS_SCROLLPORT_UNDERFLOW, nullptr);
  event.orient = orient;
  return EventDispatcher::Dispatch(mOuter->GetContent(),
                                   mOuter->PresContext(), &event);
}

void
ScrollFrameHelper::ReloadChildFrames()
{
  mScrolledFrame = nullptr;
  mHScrollbarBox = nullptr;
  mVScrollbarBox = nullptr;
  mScrollCornerBox = nullptr;
  mResizerBox = nullptr;

  nsIFrame* frame = mOuter->GetFirstPrincipalChild();
  while (frame) {
    nsIContent* content = frame->GetContent();
    if (content == mOuter->GetContent()) {
      NS_ASSERTION(!mScrolledFrame, "Already found the scrolled frame");
      mScrolledFrame = frame;
    } else {
      nsAutoString value;
      content->GetAttr(kNameSpaceID_None, nsGkAtoms::orient, value);
      if (!value.IsEmpty()) {
        
        if (value.LowerCaseEqualsLiteral("horizontal")) {
          NS_ASSERTION(!mHScrollbarBox, "Found multiple horizontal scrollbars?");
          mHScrollbarBox = frame;
        } else {
          NS_ASSERTION(!mVScrollbarBox, "Found multiple vertical scrollbars?");
          mVScrollbarBox = frame;
        }
      } else if (content->IsXULElement(nsGkAtoms::resizer)) {
        NS_ASSERTION(!mResizerBox, "Found multiple resizers");
        mResizerBox = frame;
      } else if (content->IsXULElement(nsGkAtoms::scrollcorner)) {
        
        NS_ASSERTION(!mScrollCornerBox, "Found multiple scrollcorners");
        mScrollCornerBox = frame;
      }
    }

    frame = frame->GetNextSibling();
  }
}

nsresult
ScrollFrameHelper::CreateAnonymousContent(
  nsTArray<nsIAnonymousContentCreator::ContentInfo>& aElements)
{
  nsPresContext* presContext = mOuter->PresContext();
  nsIFrame* parent = mOuter->GetParent();

  
  
  
  
  
  if (presContext->Document()->IsBeingUsedAsImage() ||
      (!presContext->IsDynamic() &&
       !(mIsRoot && presContext->HasPaginatedScrolling()))) {
    mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = true;
    return NS_OK;
  }

  
  int8_t resizeStyle = mOuter->StyleDisplay()->mResize;
  bool isResizable = resizeStyle != NS_STYLE_RESIZE_NONE;

  nsIScrollableFrame *scrollable = do_QueryFrame(mOuter);

  
  
  
  
  
  bool canHaveHorizontal;
  bool canHaveVertical;
  if (!mIsRoot) {
    ScrollbarStyles styles = scrollable->GetScrollbarStyles();
    canHaveHorizontal = styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN;
    canHaveVertical = styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN;
    if (!canHaveHorizontal && !canHaveVertical && !isResizable) {
      
      return NS_OK;
    }
  } else {
    canHaveHorizontal = true;
    canHaveVertical = true;
  }

  
  nsITextControlFrame* textFrame = do_QueryFrame(parent);
  if (textFrame) {
    
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textAreaElement(do_QueryInterface(parent->GetContent()));
    if (!textAreaElement) {
      mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = true;
      return NS_OK;
    }
  }

  nsNodeInfoManager *nodeInfoManager =
    presContext->Document()->NodeInfoManager();
  nsRefPtr<NodeInfo> nodeInfo;
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollbar, nullptr,
                                          kNameSpaceID_XUL,
                                          nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  if (canHaveHorizontal) {
    nsRefPtr<NodeInfo> ni = nodeInfo;
    NS_TrustedNewXULElement(getter_AddRefs(mHScrollbarContent), ni.forget());
    mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("horizontal"), false);
    mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                NS_LITERAL_STRING("always"), false);
    if (mIsRoot) {
      mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::root_,
                                  NS_LITERAL_STRING("true"), false);
    }
    if (!aElements.AppendElement(mHScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveVertical) {
    nsRefPtr<NodeInfo> ni = nodeInfo;
    NS_TrustedNewXULElement(getter_AddRefs(mVScrollbarContent), ni.forget());
    mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("vertical"), false);
    mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                NS_LITERAL_STRING("always"), false);
    if (mIsRoot) {
      mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::root_,
                                  NS_LITERAL_STRING("true"), false);
    }
    if (!aElements.AppendElement(mVScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (isResizable) {
    nsRefPtr<NodeInfo> nodeInfo;
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::resizer, nullptr,
                                            kNameSpaceID_XUL,
                                            nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

    NS_TrustedNewXULElement(getter_AddRefs(mResizerContent), nodeInfo.forget());

    nsAutoString dir;
    switch (resizeStyle) {
      case NS_STYLE_RESIZE_HORIZONTAL:
        if (IsScrollbarOnRight()) {
          dir.AssignLiteral("right");
        }
        else {
          dir.AssignLiteral("left");
        }
        break;
      case NS_STYLE_RESIZE_VERTICAL:
        dir.AssignLiteral("bottom");
        break;
      case NS_STYLE_RESIZE_BOTH:
        dir.AssignLiteral("bottomend");
        break;
      default:
        NS_WARNING("only resizable types should have resizers");
    }
    mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, dir, false);

    if (mIsRoot) {
      nsIContent* browserRoot = GetBrowserRoot(mOuter->GetContent());
      mCollapsedResizer = !(browserRoot &&
                            browserRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::showresizer));
    }
    else {
      mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::element,
                                    NS_LITERAL_STRING("_parent"), false);
    }

    mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                  NS_LITERAL_STRING("always"), false);

    if (!aElements.AppendElement(mResizerContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveHorizontal && canHaveVertical) {
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollcorner, nullptr,
                                            kNameSpaceID_XUL,
                                            nsIDOMNode::ELEMENT_NODE);
    NS_TrustedNewXULElement(getter_AddRefs(mScrollCornerContent), nodeInfo.forget());
    if (!aElements.AppendElement(mScrollCornerContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
ScrollFrameHelper::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                                uint32_t aFilter)
{
  if (mHScrollbarContent) {
    aElements.AppendElement(mHScrollbarContent);
  }

  if (mVScrollbarContent) {
    aElements.AppendElement(mVScrollbarContent);
  }

  if (mScrollCornerContent) {
    aElements.AppendElement(mScrollCornerContent);
  }

  if (mResizerContent) {
    aElements.AppendElement(mResizerContent);
  }
}

void
ScrollFrameHelper::Destroy()
{
  if (mScrollbarActivity) {
    mScrollbarActivity->Destroy();
    mScrollbarActivity = nullptr;
  }

  
  nsContentUtils::DestroyAnonymousContent(&mHScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mVScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mScrollCornerContent);
  nsContentUtils::DestroyAnonymousContent(&mResizerContent);

  if (mPostedReflowCallback) {
    mOuter->PresContext()->PresShell()->CancelReflowCallback(this);
    mPostedReflowCallback = false;
  }
}







void
ScrollFrameHelper::UpdateScrollbarPosition()
{
  nsWeakFrame weakFrame(mOuter);
  mFrameIsUpdatingScrollbar = true;

  nsPoint pt = GetScrollPosition();
  if (mVScrollbarBox) {
    SetCoordAttribute(mVScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      pt.y - GetScrolledRect().y);
    if (!weakFrame.IsAlive()) {
      return;
    }
  }
  if (mHScrollbarBox) {
    SetCoordAttribute(mHScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      pt.x - GetScrolledRect().x);
    if (!weakFrame.IsAlive()) {
      return;
    }
  }

  mFrameIsUpdatingScrollbar = false;
}

void ScrollFrameHelper::CurPosAttributeChanged(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "aContent must not be null");
  NS_ASSERTION((mHScrollbarBox && mHScrollbarBox->GetContent() == aContent) ||
               (mVScrollbarBox && mVScrollbarBox->GetContent() == aContent),
               "unexpected child");

  
  
  
  
  
  
  
  
  
  if (mFrameIsUpdatingScrollbar)
    return;

  nsRect scrolledRect = GetScrolledRect();

  nsPoint current = GetScrollPosition() - scrolledRect.TopLeft();
  nsPoint dest;
  nsRect allowedRange;
  dest.x = GetCoordAttribute(mHScrollbarBox, nsGkAtoms::curpos, current.x,
                             &allowedRange.x, &allowedRange.width);
  dest.y = GetCoordAttribute(mVScrollbarBox, nsGkAtoms::curpos, current.y,
                             &allowedRange.y, &allowedRange.height);
  current += scrolledRect.TopLeft();
  dest += scrolledRect.TopLeft();
  allowedRange += scrolledRect.TopLeft();

  
  
  
  if (allowedRange.ClampPoint(current) == current) {
    return;
  }

  if (mScrollbarActivity) {
    nsRefPtr<ScrollbarActivity> scrollbarActivity(mScrollbarActivity);
    scrollbarActivity->ActivityOccurred();
  }

  bool isSmooth = aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::smooth);
  if (isSmooth) {
    
    
    
    
    nsWeakFrame weakFrame(mOuter);
    UpdateScrollbarPosition();
    if (!weakFrame.IsAlive()) {
      return;
    }
  }
  ScrollToWithOrigin(dest,
                     isSmooth ? nsIScrollableFrame::SMOOTH : nsIScrollableFrame::INSTANT,
                     nsGkAtoms::scrollbars, &allowedRange);
  
}



NS_IMETHODIMP
ScrollFrameHelper::ScrollEvent::Run()
{
  if (mHelper)
    mHelper->FireScrollEvent();
  return NS_OK;
}

void
ScrollFrameHelper::FireScrollEvent()
{
  mScrollEvent.Forget();

  WidgetGUIEvent event(true, NS_SCROLL_EVENT, nullptr);
  nsEventStatus status = nsEventStatus_eIgnore;
  nsIContent* content = mOuter->GetContent();
  nsPresContext* prescontext = mOuter->PresContext();
  
  
  if (mIsRoot) {
    nsIDocument* doc = content->GetCurrentDoc();
    if (doc) {
      EventDispatcher::Dispatch(doc, prescontext, &event, nullptr,  &status);
    }
  } else {
    
    
    event.mFlags.mBubbles = false;
    EventDispatcher::Dispatch(content, prescontext, &event, nullptr, &status);
  }
}

void
ScrollFrameHelper::PostScrollEvent()
{
  if (mScrollEvent.IsPending())
    return;

  nsRootPresContext* rpc = mOuter->PresContext()->GetRootPresContext();
  if (!rpc)
    return;
  mScrollEvent = new ScrollEvent(this);
  rpc->AddWillPaintObserver(mScrollEvent.get());
}

NS_IMETHODIMP
ScrollFrameHelper::AsyncScrollPortEvent::Run()
{
  if (mHelper) {
    mHelper->mOuter->PresContext()->GetPresShell()->
      FlushPendingNotifications(Flush_InterruptibleLayout);
  }
  return mHelper ? mHelper->FireScrollPortEvent() : NS_OK;
}

bool
nsXULScrollFrame::AddHorizontalScrollbar(nsBoxLayoutState& aState, bool aOnBottom)
{
  if (!mHelper.mHScrollbarBox)
    return true;

  return AddRemoveScrollbar(aState, aOnBottom, true, true);
}

bool
nsXULScrollFrame::AddVerticalScrollbar(nsBoxLayoutState& aState, bool aOnRight)
{
  if (!mHelper.mVScrollbarBox)
    return true;

  return AddRemoveScrollbar(aState, aOnRight, false, true);
}

void
nsXULScrollFrame::RemoveHorizontalScrollbar(nsBoxLayoutState& aState, bool aOnBottom)
{
  
  DebugOnly<bool> result = AddRemoveScrollbar(aState, aOnBottom, true, false);
  NS_ASSERTION(result, "Removing horizontal scrollbar failed to fit??");
}

void
nsXULScrollFrame::RemoveVerticalScrollbar(nsBoxLayoutState& aState, bool aOnRight)
{
  
  DebugOnly<bool> result = AddRemoveScrollbar(aState, aOnRight, false, false);
  NS_ASSERTION(result, "Removing vertical scrollbar failed to fit??");
}

bool
nsXULScrollFrame::AddRemoveScrollbar(nsBoxLayoutState& aState,
                                     bool aOnRightOrBottom, bool aHorizontal, bool aAdd)
{
  if (aHorizontal) {
     if (mHelper.mNeverHasHorizontalScrollbar || !mHelper.mHScrollbarBox)
       return false;

     nsSize hSize = mHelper.mHScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mHelper.mHScrollbarBox, hSize);

     mHelper.SetScrollbarVisibility(mHelper.mHScrollbarBox, aAdd);

     bool hasHorizontalScrollbar;
     bool fit = AddRemoveScrollbar(hasHorizontalScrollbar,
                                     mHelper.mScrollPort.y,
                                     mHelper.mScrollPort.height,
                                     hSize.height, aOnRightOrBottom, aAdd);
     mHelper.mHasHorizontalScrollbar = hasHorizontalScrollbar;    
     if (!fit)
        mHelper.SetScrollbarVisibility(mHelper.mHScrollbarBox, !aAdd);

     return fit;
  } else {
     if (mHelper.mNeverHasVerticalScrollbar || !mHelper.mVScrollbarBox)
       return false;

     nsSize vSize = mHelper.mVScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mHelper.mVScrollbarBox, vSize);

     mHelper.SetScrollbarVisibility(mHelper.mVScrollbarBox, aAdd);

     bool hasVerticalScrollbar;
     bool fit = AddRemoveScrollbar(hasVerticalScrollbar,
                                     mHelper.mScrollPort.x,
                                     mHelper.mScrollPort.width,
                                     vSize.width, aOnRightOrBottom, aAdd);
     mHelper.mHasVerticalScrollbar = hasVerticalScrollbar;    
     if (!fit)
        mHelper.SetScrollbarVisibility(mHelper.mVScrollbarBox, !aAdd);

     return fit;
  }
}

bool
nsXULScrollFrame::AddRemoveScrollbar(bool& aHasScrollbar, nscoord& aXY,
                                     nscoord& aSize, nscoord aSbSize,
                                     bool aOnRightOrBottom, bool aAdd)
{
   nscoord size = aSize;
   nscoord xy = aXY;

   if (size != NS_INTRINSICSIZE) {
     if (aAdd) {
        size -= aSbSize;
        if (!aOnRightOrBottom && size >= 0)
          xy += aSbSize;
     } else {
        size += aSbSize;
        if (!aOnRightOrBottom)
          xy -= aSbSize;
     }
   }

   
   if (size >= 0) {
       aHasScrollbar = aAdd;
       aSize = size;
       aXY = xy;
       return true;
   }

   aHasScrollbar = false;
   return false;
}

void
nsXULScrollFrame::LayoutScrollArea(nsBoxLayoutState& aState,
                                   const nsPoint& aScrollPosition)
{
  uint32_t oldflags = aState.LayoutFlags();
  nsRect childRect = nsRect(mHelper.mScrollPort.TopLeft() - aScrollPosition,
                            mHelper.mScrollPort.Size());
  int32_t flags = NS_FRAME_NO_MOVE_VIEW;

  nsSize minSize = mHelper.mScrolledFrame->GetMinSize(aState);

  if (minSize.height > childRect.height)
    childRect.height = minSize.height;

  if (minSize.width > childRect.width)
    childRect.width = minSize.width;

  aState.SetLayoutFlags(flags);
  ClampAndSetBounds(aState, childRect, aScrollPosition);
  mHelper.mScrolledFrame->Layout(aState);

  childRect = mHelper.mScrolledFrame->GetRect();

  if (childRect.width < mHelper.mScrollPort.width ||
      childRect.height < mHelper.mScrollPort.height)
  {
    childRect.width = std::max(childRect.width, mHelper.mScrollPort.width);
    childRect.height = std::max(childRect.height, mHelper.mScrollPort.height);

    
    
    
    ClampAndSetBounds(aState, childRect, aScrollPosition, true);
  }

  aState.SetLayoutFlags(oldflags);

}

void ScrollFrameHelper::PostOverflowEvent()
{
  if (mAsyncScrollPortEvent.IsPending())
    return;

  
  nsSize scrollportSize = mScrollPort.Size();
  nsSize childSize = GetScrolledRect().Size();

  bool newVerticalOverflow = childSize.height > scrollportSize.height;
  bool vertChanged = mVerticalOverflow != newVerticalOverflow;

  bool newHorizontalOverflow = childSize.width > scrollportSize.width;
  bool horizChanged = mHorizontalOverflow != newHorizontalOverflow;

  if (!vertChanged && !horizChanged) {
    return;
  }

  nsRootPresContext* rpc = mOuter->PresContext()->GetRootPresContext();
  if (!rpc)
    return;

  mAsyncScrollPortEvent = new AsyncScrollPortEvent(this);
  rpc->AddWillPaintObserver(mAsyncScrollPortEvent.get());
}

bool
ScrollFrameHelper::IsLTR() const
{
  

  nsIFrame *frame = mOuter;
  
  if (mIsRoot) {
    
    nsPresContext *presContext = mOuter->PresContext();
    nsIDocument *document = presContext->Document();
    Element *root = document->GetRootElement();

    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
    if (htmlDoc) {
      Element *bodyElement = document->GetBodyElement();
      if (bodyElement)
        root = bodyElement; 
    }

    if (root) {
      nsIFrame *rootsFrame = root->GetPrimaryFrame();
      if (rootsFrame)
        frame = rootsFrame;
    }
  }

  WritingMode wm = frame->GetWritingMode();
  return wm.IsVertical() ? wm.IsVerticalLR() : wm.IsBidiLTR();
}

bool
ScrollFrameHelper::IsScrollbarOnRight() const
{
  nsPresContext *presContext = mOuter->PresContext();

  
  
  
  if (!mIsRoot)
    return IsLTR();
  switch (presContext->GetCachedIntPref(kPresContext_ScrollbarSide)) {
    default:
    case 0: 
      return presContext->GetCachedIntPref(kPresContext_BidiDirection)
             == IBMBIDI_TEXTDIRECTION_LTR;
    case 1: 
      return IsLTR();
    case 2: 
      return true;
    case 3: 
      return false;
  }
}

bool
ScrollFrameHelper::IsMaybeScrollingActive() const
{
  const nsStyleDisplay* disp = mOuter->StyleDisplay();
  if (disp && (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_SCROLL)) {
    return true;
  }

  return mHasBeenScrolledRecently ||
      IsAlwaysActive() ||
      mShouldBuildScrollableLayer;
}

bool
ScrollFrameHelper::IsScrollingActive(nsDisplayListBuilder* aBuilder) const
{
  const nsStyleDisplay* disp = mOuter->StyleDisplay();
  if (disp && (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_SCROLL) &&
    aBuilder->IsInWillChangeBudget(mOuter)) {
    return true;
  }

  return mHasBeenScrolledRecently ||
        IsAlwaysActive() ||
        mShouldBuildScrollableLayer;
}





nsresult
nsXULScrollFrame::Layout(nsBoxLayoutState& aState)
{
  bool scrollbarRight = mHelper.IsScrollbarOnRight();
  bool scrollbarBottom = true;

  
  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);

  nsRect oldScrollAreaBounds = mHelper.mScrollPort;
  nsPoint oldScrollPosition = mHelper.GetLogicalScrollPosition();

  
  mHelper.mScrollPort = clientRect;

  
























  ScrollbarStyles styles = GetScrollbarStyles();

  
  if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL)
     mHelper.mHasHorizontalScrollbar = true;
  if (styles.mVertical == NS_STYLE_OVERFLOW_SCROLL)
     mHelper.mHasVerticalScrollbar = true;

  if (mHelper.mHasHorizontalScrollbar)
     AddHorizontalScrollbar(aState, scrollbarBottom);

  if (mHelper.mHasVerticalScrollbar)
     AddVerticalScrollbar(aState, scrollbarRight);

  
  LayoutScrollArea(aState, oldScrollPosition);

  
  bool needsLayout = false;

  
  if (styles.mVertical != NS_STYLE_OVERFLOW_SCROLL) {
    
    nsRect scrolledRect = mHelper.GetScrolledRect();

    
      if (scrolledRect.height <= mHelper.mScrollPort.height
          || styles.mVertical != NS_STYLE_OVERFLOW_AUTO) {
        if (mHelper.mHasVerticalScrollbar) {
          
          
          RemoveVerticalScrollbar(aState, scrollbarRight);
          needsLayout = true;
        }
      } else {
        if (!mHelper.mHasVerticalScrollbar) {
          
          
          if (AddVerticalScrollbar(aState, scrollbarRight))
            needsLayout = true;
        }
    }

    
    if (needsLayout) {
       nsBoxLayoutState resizeState(aState);
       LayoutScrollArea(resizeState, oldScrollPosition);
       needsLayout = false;
    }
  }


  
  if (styles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL)
  {
    
    nsRect scrolledRect = mHelper.GetScrolledRect();

    
    
    if ((scrolledRect.width > mHelper.mScrollPort.width)
        && styles.mHorizontal == NS_STYLE_OVERFLOW_AUTO) {

      if (!mHelper.mHasHorizontalScrollbar) {
           
          if (AddHorizontalScrollbar(aState, scrollbarBottom))
             needsLayout = true;

           
           
           
           
           
           

      }
    } else {
        
        
      if (mHelper.mHasHorizontalScrollbar) {
        RemoveHorizontalScrollbar(aState, scrollbarBottom);
        needsLayout = true;
      }
    }
  }

  
  if (needsLayout) {
     nsBoxLayoutState resizeState(aState);
     LayoutScrollArea(resizeState, oldScrollPosition);
     needsLayout = false;
  }

  
  nsSize hMinSize(0, 0);
  if (mHelper.mHScrollbarBox && mHelper.mHasHorizontalScrollbar) {
    GetScrollbarMetrics(aState, mHelper.mHScrollbarBox, &hMinSize, nullptr, false);
  }
  nsSize vMinSize(0, 0);
  if (mHelper.mVScrollbarBox && mHelper.mHasVerticalScrollbar) {
    GetScrollbarMetrics(aState, mHelper.mVScrollbarBox, &vMinSize, nullptr, true);
  }

  
  
  
  
  
  if (mHelper.mHasHorizontalScrollbar &&
      (hMinSize.width > clientRect.width - vMinSize.width
       || hMinSize.height > clientRect.height)) {
    RemoveHorizontalScrollbar(aState, scrollbarBottom);
    needsLayout = true;
  }
  
  if (mHelper.mHasVerticalScrollbar &&
      (vMinSize.height > clientRect.height - hMinSize.height
       || vMinSize.width > clientRect.width)) {
    RemoveVerticalScrollbar(aState, scrollbarRight);
    needsLayout = true;
  }

  
  if (needsLayout) {
    nsBoxLayoutState resizeState(aState);
    LayoutScrollArea(resizeState, oldScrollPosition);
  }

  if (!mHelper.mSupppressScrollbarUpdate) {
    mHelper.LayoutScrollbars(aState, clientRect, oldScrollAreaBounds);
  }
  if (!mHelper.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mHelper);
    mHelper.mPostedReflowCallback = true;
  }
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    mHelper.mHadNonInitialReflow = true;
  }

  mHelper.UpdateSticky();

  
  
  nsIFrame* f = mHelper.mScrolledFrame->GetContentInsertionFrame();
  if (nsLayoutUtils::GetAsBlock(f)) {
    nsRect origRect = f->GetRect();
    nsRect clippedRect = origRect;
    clippedRect.MoveBy(mHelper.mScrollPort.TopLeft());
    clippedRect.IntersectRect(clippedRect, mHelper.mScrollPort);
    nsOverflowAreas overflow = f->GetOverflowAreas();
    f->FinishAndStoreOverflow(overflow, clippedRect.Size());
    clippedRect.MoveTo(origRect.TopLeft());
    f->SetRect(clippedRect);
  }

  mHelper.UpdatePrevScrolledRect();

  mHelper.PostOverflowEvent();
  return NS_OK;
}

void
ScrollFrameHelper::FinishReflowForScrollbar(nsIContent* aContent,
                                                nscoord aMinXY, nscoord aMaxXY,
                                                nscoord aCurPosXY,
                                                nscoord aPageIncrement,
                                                nscoord aIncrement)
{
  
  SetCoordAttribute(aContent, nsGkAtoms::curpos, aCurPosXY - aMinXY);
  SetScrollbarEnabled(aContent, aMaxXY - aMinXY);
  SetCoordAttribute(aContent, nsGkAtoms::maxpos, aMaxXY - aMinXY);
  SetCoordAttribute(aContent, nsGkAtoms::pageincrement, aPageIncrement);
  SetCoordAttribute(aContent, nsGkAtoms::increment, aIncrement);
}

bool
ScrollFrameHelper::ReflowFinished()
{
  nsAutoScriptBlocker scriptBlocker;
  mPostedReflowCallback = false;

  ScrollToRestoredPosition();

  
  
  nsPoint currentScrollPos = GetScrollPosition();
  ScrollToImpl(currentScrollPos, nsRect(currentScrollPos, nsSize(0, 0)));
  if (!mAsyncScroll && !mAsyncSmoothMSDScroll) {
    
    
    
    mDestination = GetScrollPosition();
  }

  if (NS_SUBTREE_DIRTY(mOuter) || !mUpdateScrollbarAttributes)
    return false;

  mUpdateScrollbarAttributes = false;

  
  nsPresContext* presContext = mOuter->PresContext();

  if (mMayHaveDirtyFixedChildren) {
    mMayHaveDirtyFixedChildren = false;
    nsIFrame* parentFrame = mOuter->GetParent();
    for (nsIFrame* fixedChild =
           parentFrame->GetFirstChild(nsIFrame::kFixedList);
         fixedChild; fixedChild = fixedChild->GetNextSibling()) {
      
      presContext->PresShell()->
        FrameNeedsReflow(fixedChild, nsIPresShell::eResize,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  nsRect scrolledContentRect = GetScrolledRect();
  nsSize scrollClampingScrollPort = GetScrollPositionClampingScrollPortSize();
  nscoord minX = scrolledContentRect.x;
  nscoord maxX = scrolledContentRect.XMost() - scrollClampingScrollPort.width;
  nscoord minY = scrolledContentRect.y;
  nscoord maxY = scrolledContentRect.YMost() - scrollClampingScrollPort.height;

  
  NS_ASSERTION(!mFrameIsUpdatingScrollbar, "We shouldn't be reentering here");
  mFrameIsUpdatingScrollbar = true;

  nsCOMPtr<nsIContent> vScroll =
    mVScrollbarBox ? mVScrollbarBox->GetContent() : nullptr;
  nsCOMPtr<nsIContent> hScroll =
    mHScrollbarBox ? mHScrollbarBox->GetContent() : nullptr;

  
  
  
  if (vScroll || hScroll) {
    nsWeakFrame weakFrame(mOuter);
    nsPoint scrollPos = GetScrollPosition();
    nsSize lineScrollAmount = GetLineScrollAmount();
    if (vScroll) {
      const double kScrollMultiplier =
        Preferences::GetInt("toolkit.scrollbox.verticalScrollDistance",
                            NS_DEFAULT_VERTICAL_SCROLL_DISTANCE);
      nscoord increment = lineScrollAmount.height * kScrollMultiplier;
      
      
      
      
      
      
      
      nscoord pageincrement = nscoord(scrollClampingScrollPort.height - increment);
      nscoord pageincrementMin = nscoord(float(scrollClampingScrollPort.height) * 0.8);
      FinishReflowForScrollbar(vScroll, minY, maxY, scrollPos.y,
                               std::max(pageincrement, pageincrementMin),
                               increment);
    }
    if (hScroll) {
      const double kScrollMultiplier =
        Preferences::GetInt("toolkit.scrollbox.horizontalScrollDistance",
                            NS_DEFAULT_HORIZONTAL_SCROLL_DISTANCE);
      nscoord increment = lineScrollAmount.width * kScrollMultiplier;
      FinishReflowForScrollbar(hScroll, minX, maxX, scrollPos.x,
                               nscoord(float(scrollClampingScrollPort.width) * 0.8),
                               increment);
    }
    NS_ENSURE_TRUE(weakFrame.IsAlive(), false);
  }

  mFrameIsUpdatingScrollbar = false;
  
  
  
  
  
  
  
  
  
  
  
  if (!mHScrollbarBox && !mVScrollbarBox)
    return false;
  CurPosAttributeChanged(mVScrollbarBox ? mVScrollbarBox->GetContent()
                                        : mHScrollbarBox->GetContent());
  return true;
}

void
ScrollFrameHelper::ReflowCallbackCanceled()
{
  mPostedReflowCallback = false;
}

bool
ScrollFrameHelper::UpdateOverflow()
{
  nsIScrollableFrame* sf = do_QueryFrame(mOuter);
  ScrollbarStyles ss = sf->GetScrollbarStyles();

  
  
  
  nsRect scrolledRect = GetScrolledRect();
  uint32_t overflowChange = GetOverflowChange(scrolledRect, mPrevScrolledRect);
  mPrevScrolledRect = scrolledRect;

  bool needReflow = false;
  nsPoint scrollPosition = GetScrollPosition();
  if (overflowChange & nsIScrollableFrame::HORIZONTAL) {
    if (ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN || scrollPosition.x) {
      needReflow = true;
    }
  }
  if (overflowChange & nsIScrollableFrame::VERTICAL) {
    if (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN || scrollPosition.y) {
      needReflow = true;
    }
  }

  if (needReflow) {
    
    
    
    
    
    mOuter->PresContext()->PresShell()->FrameNeedsReflow(
      mOuter, nsIPresShell::eResize, NS_FRAME_HAS_DIRTY_CHILDREN);
    
    
    
    mSkippedScrollbarLayout = true;
    return false;  
  }
  PostOverflowEvent();
  return mOuter->nsContainerFrame::UpdateOverflow();
}

void
ScrollFrameHelper::UpdateSticky()
{
  StickyScrollContainer* ssc = StickyScrollContainer::
    GetStickyScrollContainerForScrollFrame(mOuter);
  if (ssc) {
    nsIScrollableFrame* scrollFrame = do_QueryFrame(mOuter);
    ssc->UpdatePositions(scrollFrame->GetScrollPosition(), mOuter);
  }
}

void
ScrollFrameHelper::UpdatePrevScrolledRect()
{
  mPrevScrolledRect = GetScrolledRect();
}

void
ScrollFrameHelper::AdjustScrollbarRectForResizer(
                         nsIFrame* aFrame, nsPresContext* aPresContext,
                         nsRect& aRect, bool aHasResizer, bool aVertical)
{
  if ((aVertical ? aRect.width : aRect.height) == 0)
    return;

  
  
  nsRect resizerRect;
  if (aHasResizer) {
    resizerRect = mResizerBox->GetRect();
  }
  else {
    nsPoint offset;
    nsIWidget* widget = aFrame->GetNearestWidget(offset);
    nsIntRect widgetRect;
    if (!widget || !widget->ShowsResizeIndicator(&widgetRect))
      return;

    resizerRect = nsRect(aPresContext->DevPixelsToAppUnits(widgetRect.x) - offset.x,
                         aPresContext->DevPixelsToAppUnits(widgetRect.y) - offset.y,
                         aPresContext->DevPixelsToAppUnits(widgetRect.width),
                         aPresContext->DevPixelsToAppUnits(widgetRect.height));
  }

  if (resizerRect.Contains(aRect.BottomRight() - nsPoint(1, 1))) {
    if (aVertical) {
      aRect.height = std::max(0, resizerRect.y - aRect.y);
    } else {
      aRect.width = std::max(0, resizerRect.x - aRect.x);
    }
  } else if (resizerRect.Contains(aRect.BottomLeft() + nsPoint(1, -1))) {
    if (aVertical) {
      aRect.height = std::max(0, resizerRect.y - aRect.y);
    } else {
      nscoord xmost = aRect.XMost();
      aRect.x = std::max(aRect.x, resizerRect.XMost());
      aRect.width = xmost - aRect.x;
    }
  }
}

static void
AdjustOverlappingScrollbars(nsRect& aVRect, nsRect& aHRect)
{
  if (aVRect.IsEmpty() || aHRect.IsEmpty())
    return;

  const nsRect oldVRect = aVRect;
  const nsRect oldHRect = aHRect;
  if (oldVRect.Contains(oldHRect.BottomRight() - nsPoint(1, 1))) {
    aHRect.width = std::max(0, oldVRect.x - oldHRect.x);
  } else if (oldVRect.Contains(oldHRect.BottomLeft() - nsPoint(0, 1))) {
    nscoord overlap = std::min(oldHRect.width, oldVRect.XMost() - oldHRect.x);
    aHRect.x += overlap;
    aHRect.width -= overlap;
  }
  if (oldHRect.Contains(oldVRect.BottomRight() - nsPoint(1, 1))) {
    aVRect.height = std::max(0, oldHRect.y - oldVRect.y);
  }
}

void
ScrollFrameHelper::LayoutScrollbars(nsBoxLayoutState& aState,
                                        const nsRect& aContentArea,
                                        const nsRect& aOldScrollArea)
{
  NS_ASSERTION(!mSupppressScrollbarUpdate,
               "This should have been suppressed");

  bool hasResizer = HasResizer();
  bool scrollbarOnLeft = !IsScrollbarOnRight();

  
  if (mScrollCornerBox || mResizerBox) {
    NS_PRECONDITION(!mScrollCornerBox || mScrollCornerBox->IsBoxFrame(), "Must be a box frame!");

    nsRect r(0, 0, 0, 0);
    if (aContentArea.x != mScrollPort.x || scrollbarOnLeft) {
      
      r.x = aContentArea.x;
      r.width = mScrollPort.x - aContentArea.x;
      NS_ASSERTION(r.width >= 0, "Scroll area should be inside client rect");
    } else {
      
      r.width = aContentArea.XMost() - mScrollPort.XMost();
      r.x = aContentArea.XMost() - r.width;
      NS_ASSERTION(r.width >= 0, "Scroll area should be inside client rect");
    }
    if (aContentArea.y != mScrollPort.y) {
      NS_ERROR("top scrollbars not supported");
    } else {
      
      r.height = aContentArea.YMost() - mScrollPort.YMost();
      r.y = aContentArea.YMost() - r.height;
      NS_ASSERTION(r.height >= 0, "Scroll area should be inside client rect");
    }

    if (mScrollCornerBox) {
      nsBoxFrame::LayoutChildAt(aState, mScrollCornerBox, r);
    }

    if (hasResizer) {
      
      nscoord defaultSize = nsPresContext::CSSPixelsToAppUnits(15);
      nsSize resizerMinSize = mResizerBox->GetMinSize(aState);

      nscoord vScrollbarWidth = mVScrollbarBox ?
        mVScrollbarBox->GetPrefSize(aState).width : defaultSize;
      r.width = std::max(std::max(r.width, vScrollbarWidth), resizerMinSize.width);
      if (aContentArea.x == mScrollPort.x && !scrollbarOnLeft) {
        r.x = aContentArea.XMost() - r.width;
      }

      nscoord hScrollbarHeight = mHScrollbarBox ?
        mHScrollbarBox->GetPrefSize(aState).height : defaultSize;
      r.height = std::max(std::max(r.height, hScrollbarHeight), resizerMinSize.height);
      if (aContentArea.y == mScrollPort.y) {
        r.y = aContentArea.YMost() - r.height;
      }

      nsBoxFrame::LayoutChildAt(aState, mResizerBox, r);
    }
    else if (mResizerBox) {
      
      nsBoxFrame::LayoutChildAt(aState, mResizerBox, nsRect());
    }
  }

  nsPresContext* presContext = mScrolledFrame->PresContext();
  nsRect vRect;
  if (mVScrollbarBox) {
    NS_PRECONDITION(mVScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    vRect = mScrollPort;
    vRect.width = aContentArea.width - mScrollPort.width;
    vRect.x = scrollbarOnLeft ? aContentArea.x : mScrollPort.XMost();
    if (mHasVerticalScrollbar) {
      nsMargin margin;
      mVScrollbarBox->GetMargin(margin);
      vRect.Deflate(margin);
    }
    AdjustScrollbarRectForResizer(mOuter, presContext, vRect, hasResizer, true);
  }

  nsRect hRect;
  if (mHScrollbarBox) {
    NS_PRECONDITION(mHScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    hRect = mScrollPort;
    hRect.height = aContentArea.height - mScrollPort.height;
    hRect.y = true ? mScrollPort.YMost() : aContentArea.y;
    if (mHasHorizontalScrollbar) {
      nsMargin margin;
      mHScrollbarBox->GetMargin(margin);
      hRect.Deflate(margin);
    }
    AdjustScrollbarRectForResizer(mOuter, presContext, hRect, hasResizer, false);
  }

  if (!LookAndFeel::GetInt(LookAndFeel::eIntID_AllowOverlayScrollbarsOverlap)) {
    AdjustOverlappingScrollbars(vRect, hRect);
  }
  if (mVScrollbarBox) {
    nsBoxFrame::LayoutChildAt(aState, mVScrollbarBox, vRect);
  }
  if (mHScrollbarBox) {
    nsBoxFrame::LayoutChildAt(aState, mHScrollbarBox, hRect);
  }

  
  
  
  
  if (aOldScrollArea.Size() != mScrollPort.Size() &&
      !(mOuter->GetStateBits() & NS_FRAME_IS_DIRTY) &&
      mIsRoot) {
    mMayHaveDirtyFixedChildren = true;
  }

  
  mUpdateScrollbarAttributes = true;
  if (!mPostedReflowCallback) {
    aState.PresContext()->PresShell()->PostReflowCallback(this);
    mPostedReflowCallback = true;
  }
}

#if DEBUG
static bool ShellIsAlive(nsWeakPtr& aWeakPtr)
{
  nsCOMPtr<nsIPresShell> shell(do_QueryReferent(aWeakPtr));
  return !!shell;
}
#endif

void
ScrollFrameHelper::SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos)
{
  DebugOnly<nsWeakPtr> weakShell(
    do_GetWeakReference(mOuter->PresContext()->PresShell()));
  if (aMaxPos) {
    aContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, true);
  } else {
    aContent->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled,
                      NS_LITERAL_STRING("true"), true);
  }
  MOZ_ASSERT(ShellIsAlive(weakShell), "pres shell was destroyed by scrolling");
}

void
ScrollFrameHelper::SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom,
                                         nscoord aSize)
{
  DebugOnly<nsWeakPtr> weakShell(
    do_GetWeakReference(mOuter->PresContext()->PresShell()));
  
  aSize = nsPresContext::AppUnitsToIntCSSPixels(aSize);

  

  nsAutoString newValue;
  newValue.AppendInt(aSize);

  if (aContent->AttrValueIs(kNameSpaceID_None, aAtom, newValue, eCaseMatters))
    return;

  nsWeakFrame weakFrame(mOuter);
  nsCOMPtr<nsIContent> kungFuDeathGrip = aContent;
  aContent->SetAttr(kNameSpaceID_None, aAtom, newValue, true);
  MOZ_ASSERT(ShellIsAlive(weakShell), "pres shell was destroyed by scrolling");
  if (!weakFrame.IsAlive()) {
    return;
  }

  if (mScrollbarActivity) {
    nsRefPtr<ScrollbarActivity> scrollbarActivity(mScrollbarActivity);
    scrollbarActivity->ActivityOccurred();
  }
}

static void
ReduceRadii(nscoord aXBorder, nscoord aYBorder,
            nscoord& aXRadius, nscoord& aYRadius)
{
  
  
  if (aXRadius <= aXBorder || aYRadius <= aYBorder)
    return;

  
  double ratio = std::max(double(aXBorder) / aXRadius,
                        double(aYBorder) / aYRadius);
  aXRadius *= ratio;
  aYRadius *= ratio;
}









bool
ScrollFrameHelper::GetBorderRadii(const nsSize& aFrameSize,
                                  const nsSize& aBorderArea,
                                  Sides aSkipSides,
                                  nscoord aRadii[8]) const
{
  if (!mOuter->nsContainerFrame::GetBorderRadii(aFrameSize, aBorderArea,
                                                aSkipSides, aRadii)) {
    return false;
  }

  
  
  
  nsMargin sb = GetActualScrollbarSizes();
  nsMargin border = mOuter->GetUsedBorder();

  if (sb.left > 0 || sb.top > 0) {
    ReduceRadii(border.left, border.top,
                aRadii[NS_CORNER_TOP_LEFT_X],
                aRadii[NS_CORNER_TOP_LEFT_Y]);
  }

  if (sb.top > 0 || sb.right > 0) {
    ReduceRadii(border.right, border.top,
                aRadii[NS_CORNER_TOP_RIGHT_X],
                aRadii[NS_CORNER_TOP_RIGHT_Y]);
  }

  if (sb.right > 0 || sb.bottom > 0) {
    ReduceRadii(border.right, border.bottom,
                aRadii[NS_CORNER_BOTTOM_RIGHT_X],
                aRadii[NS_CORNER_BOTTOM_RIGHT_Y]);
  }

  if (sb.bottom > 0 || sb.left > 0) {
    ReduceRadii(border.left, border.bottom,
                aRadii[NS_CORNER_BOTTOM_LEFT_X],
                aRadii[NS_CORNER_BOTTOM_LEFT_Y]);
  }

  return true;
}

nsRect
ScrollFrameHelper::GetScrolledRect() const
{
  nsRect result =
    GetScrolledRectInternal(mScrolledFrame->GetScrollableOverflowRect(),
                            mScrollPort.Size());

  if (result.width < mScrollPort.width) {
    NS_WARNING("Scrolled rect smaller than scrollport?");
  }
  if (result.height < mScrollPort.height) {
    NS_WARNING("Scrolled rect smaller than scrollport?");
  }
  return result;
}

nsRect
ScrollFrameHelper::GetScrolledRectInternal(const nsRect& aScrolledFrameOverflowArea,
                                               const nsSize& aScrollPortSize) const
{
  uint8_t frameDir = IsLTR() ? NS_STYLE_DIRECTION_LTR : NS_STYLE_DIRECTION_RTL;

  
  
  if (mScrolledFrame->StyleTextReset()->mUnicodeBidi &
      NS_STYLE_UNICODE_BIDI_PLAINTEXT) {
    nsIFrame* childFrame = mScrolledFrame->GetFirstPrincipalChild();
    if (childFrame) {
      frameDir =
        (nsBidiPresUtils::ParagraphDirection(childFrame) == NSBIDI_LTR)
          ? NS_STYLE_DIRECTION_LTR : NS_STYLE_DIRECTION_RTL;
    }
  }

  return nsLayoutUtils::GetScrolledRect(mScrolledFrame,
                                        aScrolledFrameOverflowArea,
                                        aScrollPortSize, frameDir);
}

nsMargin
ScrollFrameHelper::GetActualScrollbarSizes() const
{
  nsRect r = mOuter->GetPaddingRect() - mOuter->GetPosition();

  return nsMargin(mScrollPort.y - r.y,
                  r.XMost() - mScrollPort.XMost(),
                  r.YMost() - mScrollPort.YMost(),
                  mScrollPort.x - r.x);
}

void
ScrollFrameHelper::SetScrollbarVisibility(nsIFrame* aScrollbar, bool aVisible)
{
  nsScrollbarFrame* scrollbar = do_QueryFrame(aScrollbar);
  if (scrollbar) {
    
    nsIScrollbarMediator* mediator = scrollbar->GetScrollbarMediator();
    if (mediator) {
      
      mediator->VisibilityChanged(aVisible);
    }
  }
}

nscoord
ScrollFrameHelper::GetCoordAttribute(nsIFrame* aBox, nsIAtom* aAtom,
                                         nscoord aDefaultValue,
                                         nscoord* aRangeStart,
                                         nscoord* aRangeLength)
{
  if (aBox) {
    nsIContent* content = aBox->GetContent();

    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, aAtom, value);
    if (!value.IsEmpty())
    {
      nsresult error;
      
      nscoord result = nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
      nscoord halfPixel = nsPresContext::CSSPixelsToAppUnits(0.5f);
      
      
      *aRangeStart = result - halfPixel;
      *aRangeLength = halfPixel*2 - 1;
      return result;
    }
  }

  
  *aRangeStart = aDefaultValue;
  *aRangeLength = 0;
  return aDefaultValue;
}

nsPresState*
ScrollFrameHelper::SaveState() const
{
  nsIScrollbarMediator* mediator = do_QueryFrame(GetScrolledFrame());
  if (mediator) {
    
    return nullptr;
  }

  
  
  if (!mHasBeenScrolled && !mDidHistoryRestore) {
    return nullptr;
  }

  nsPresState* state = new nsPresState();
  
  
  
  
  
  nsPoint pt = GetLogicalScrollPosition();
  if (mRestorePos.y != -1 && pt == mLastPos) {
    pt = mRestorePos;
  }
  state->SetScrollState(pt);
  state->SetResolution(mResolution);
  state->SetScaleToResolution(mScaleToResolution);
  return state;
}

void
ScrollFrameHelper::RestoreState(nsPresState* aState)
{
  mRestorePos = aState->GetScrollState();
  mDidHistoryRestore = true;
  mLastPos = mScrolledFrame ? GetLogicalScrollPosition() : nsPoint(0,0);
  mResolution = aState->GetResolution();
  mIsResolutionSet = true;
  mScaleToResolution = aState->GetScaleToResolution();

  
  MOZ_ASSERT(mIsRoot || !mScaleToResolution);

  if (mIsRoot) {
    nsIPresShell* presShell = mOuter->PresContext()->PresShell();
    if (mScaleToResolution) {
      presShell->SetResolutionAndScaleTo(mResolution);
    } else {
      presShell->SetResolution(mResolution);
    }
  }
}

void
ScrollFrameHelper::PostScrolledAreaEvent()
{
  if (mScrolledAreaEvent.IsPending()) {
    return;
  }
  mScrolledAreaEvent = new ScrolledAreaEvent(this);
  nsContentUtils::AddScriptRunner(mScrolledAreaEvent.get());
}




NS_IMETHODIMP
ScrollFrameHelper::ScrolledAreaEvent::Run()
{
  if (mHelper) {
    mHelper->FireScrolledAreaEvent();
  }
  return NS_OK;
}

void
ScrollFrameHelper::FireScrolledAreaEvent()
{
  mScrolledAreaEvent.Forget();

  InternalScrollAreaEvent event(true, NS_SCROLLEDAREACHANGED, nullptr);
  nsPresContext *prescontext = mOuter->PresContext();
  nsIContent* content = mOuter->GetContent();

  event.mArea = mScrolledFrame->GetScrollableOverflowRectRelativeToParent();

  nsIDocument *doc = content->GetCurrentDoc();
  if (doc) {
    EventDispatcher::Dispatch(doc, prescontext, &event, nullptr);
  }
}

uint32_t
nsIScrollableFrame::GetPerceivedScrollingDirections() const
{
  nscoord oneDevPixel = GetScrolledFrame()->PresContext()->AppUnitsPerDevPixel();
  uint32_t directions = GetScrollbarVisibility();
  nsRect scrollRange = GetScrollRange();
  if (scrollRange.width >= oneDevPixel) {
    directions |= HORIZONTAL;
  }
  if (scrollRange.height >= oneDevPixel) {
    directions |= VERTICAL;
  }
  return directions;
}




class SnappingEdgeCallback {
public:
  virtual void AddHorizontalEdge(nscoord aEdge) = 0;
  virtual void AddVerticalEdge(nscoord aEdge) = 0;
  virtual void AddHorizontalEdgeInterval(const nsRect &aScrollRange,
                                         nscoord aInterval,
                                         nscoord aOffset) = 0;
  virtual void AddVerticalEdgeInterval(const nsRect &aScrollRange,
                                       nscoord aInterval,
                                       nscoord aOffset) = 0;
};





class CalcSnapPoints : public SnappingEdgeCallback {
public:
  CalcSnapPoints(nsIScrollableFrame::ScrollUnit aUnit,
                 const nsPoint& aDestination,
                 const nsPoint& aStartPos);
  virtual void AddHorizontalEdge(nscoord aEdge) override;
  virtual void AddVerticalEdge(nscoord aEdge) override;
  virtual void AddHorizontalEdgeInterval(const nsRect &aScrollRange,
                                         nscoord aInterval, nscoord aOffset)
                                         override;
  virtual void AddVerticalEdgeInterval(const nsRect &aScrollRange,
                                       nscoord aInterval, nscoord aOffset)
                                       override;
  void AddEdge(nscoord aEdge,
               nscoord aDestination,
               nscoord aStartPos,
               nscoord aScrollingDirection,
               nscoord* aBestEdge,
               bool* aEdgeFound);
  void AddEdgeInterval(nscoord aInterval,
                       nscoord aMinPos,
                       nscoord aMaxPos,
                       nscoord aOffset,
                       nscoord aDestination,
                       nscoord aStartPos,
                       nscoord aScrollingDirection,
                       nscoord* aBestEdge,
                       bool* aEdgeFound);
  nsPoint GetBestEdge() const;
protected:
  nsIScrollableFrame::ScrollUnit mUnit;
  nsPoint mDestination;            
  nsPoint mStartPos;               
  nsIntPoint mScrollingDirection;  
  nsPoint mBestEdge;               
  bool mHorizontalEdgeFound;       
  bool mVerticalEdgeFound;         
};

CalcSnapPoints::CalcSnapPoints(nsIScrollableFrame::ScrollUnit aUnit,
                               const nsPoint& aDestination,
                               const nsPoint& aStartPos)
{
  mUnit = aUnit;
  mDestination = aDestination;
  mStartPos = aStartPos;

  nsPoint direction = aDestination - aStartPos;
  mScrollingDirection = nsIntPoint(0,0);
  if (direction.x < 0) {
    mScrollingDirection.x = -1;
  }
  if (direction.x > 0) {
    mScrollingDirection.x = 1;
  }
  if (direction.y < 0) {
    mScrollingDirection.y = -1;
  }
  if (direction.y > 0) {
    mScrollingDirection.y = 1;
  }
  mBestEdge = aDestination;
  mHorizontalEdgeFound = false;
  mVerticalEdgeFound = false;
}

nsPoint
CalcSnapPoints::GetBestEdge() const
{
  return nsPoint(mVerticalEdgeFound ? mBestEdge.x : mStartPos.x,
                 mHorizontalEdgeFound ? mBestEdge.y : mStartPos.y);
}

void
CalcSnapPoints::AddHorizontalEdge(nscoord aEdge)
{
  AddEdge(aEdge, mDestination.y, mStartPos.y, mScrollingDirection.y, &mBestEdge.y,
          &mHorizontalEdgeFound);
}

void
CalcSnapPoints::AddVerticalEdge(nscoord aEdge)
{
  AddEdge(aEdge, mDestination.x, mStartPos.x, mScrollingDirection.x, &mBestEdge.x,
          &mVerticalEdgeFound);
}

void
CalcSnapPoints::AddHorizontalEdgeInterval(const nsRect &aScrollRange,
                                          nscoord aInterval, nscoord aOffset)
{
  AddEdgeInterval(aInterval, aScrollRange.y, aScrollRange.YMost(), aOffset,
                  mDestination.y, mStartPos.y, mScrollingDirection.y,
                  &mBestEdge.y, &mHorizontalEdgeFound);
}

void
CalcSnapPoints::AddVerticalEdgeInterval(const nsRect &aScrollRange,
                                        nscoord aInterval, nscoord aOffset)
{
  AddEdgeInterval(aInterval, aScrollRange.x, aScrollRange.XMost(), aOffset,
                  mDestination.x, mStartPos.x, mScrollingDirection.x,
                  &mBestEdge.x, &mVerticalEdgeFound);
}

void
CalcSnapPoints::AddEdge(nscoord aEdge, nscoord aDestination, nscoord aStartPos,
                        nscoord aScrollingDirection, nscoord* aBestEdge,
                        bool *aEdgeFound)
{
  
  
  
  
  
  if (mUnit != nsIScrollableFrame::DEVICE_PIXELS) {
    
    
    if (aScrollingDirection == 0) {
      
      return;
    }
    
    
    
    
    
    if (mUnit != nsIScrollableFrame::WHOLE) {
      
      
      nscoord direction = (aEdge - aStartPos) * aScrollingDirection;
      if (direction <= 0) {
        
        return;
      }
    }
  }
  if (!*aEdgeFound) {
    *aBestEdge = aEdge;
    *aEdgeFound = true;
    return;
  }
  if (mUnit == nsIScrollableFrame::DEVICE_PIXELS ||
      mUnit == nsIScrollableFrame::LINES) {
    if (std::abs(aEdge - aDestination) < std::abs(*aBestEdge - aDestination)) {
      *aBestEdge = aEdge;
    }
  } else if (mUnit == nsIScrollableFrame::PAGES) {
    
    nscoord overshoot = (aEdge - aDestination) * aScrollingDirection;
    
    nscoord curOvershoot = (*aBestEdge - aDestination) * aScrollingDirection;

    
    
    if (overshoot < 0 && (overshoot > curOvershoot || curOvershoot >= 0)) {
      *aBestEdge = aEdge;
    }
    
    
    if (overshoot > 0 && overshoot < curOvershoot) {
      *aBestEdge = aEdge;
    }
  } else if (mUnit == nsIScrollableFrame::WHOLE) {
    
    if (aScrollingDirection > 0 && aEdge > *aBestEdge) {
      *aBestEdge = aEdge;
    } else if (aScrollingDirection < 0 && aEdge < *aBestEdge) {
      *aBestEdge = aEdge;
    }
  } else {
    NS_ERROR("Invalid scroll mode");
    return;
  }
}

void
CalcSnapPoints::AddEdgeInterval(nscoord aInterval, nscoord aMinPos,
                                nscoord aMaxPos, nscoord aOffset,
                                nscoord aDestination, nscoord aStartPos,
                                nscoord aScrollingDirection,
                                nscoord* aBestEdge, bool *aEdgeFound)
{
  if (aInterval == 0) {
    
    
    return;
  }

  
  

  
  
  
  
  nscoord clamped = std::max(std::min(aDestination, aMaxPos), aMinPos);

  
  
  nscoord r = (clamped + aOffset) % aInterval;
  if (r < aMinPos) {
    r += aInterval;
  }
  nscoord edge = clamped - r;
  if (edge >= aMinPos && edge <= aMaxPos) {
    AddEdge(edge, aDestination, aStartPos, aScrollingDirection, aBestEdge,
            aEdgeFound);
  }
  edge += aInterval;
  if (edge >= aMinPos && edge <= aMaxPos) {
    AddEdge(edge, aDestination, aStartPos, aScrollingDirection, aBestEdge,
            aEdgeFound);
  }
}

static void
ScrollSnapHelper(SnappingEdgeCallback& aCallback, nsIFrame* aFrame,
                 nsIFrame* aScrolledFrame,
                 const nsPoint &aScrollSnapDestination) {
  nsIFrame::ChildListIterator childLists(aFrame);
  for (; !childLists.IsDone(); childLists.Next()) {
    nsFrameList::Enumerator childFrames(childLists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* f = childFrames.get();

      const nsStyleDisplay* styleDisplay = f->StyleDisplay();
      size_t coordCount = styleDisplay->mScrollSnapCoordinate.Length();

      if (coordCount) {
        nsRect frameRect = f->GetRect();
        nsPoint offset = f->GetOffsetTo(aScrolledFrame);
        nsRect edgesRect = nsRect(offset, frameRect.Size());
        for (size_t coordNum = 0; coordNum < coordCount; coordNum++) {
          const nsStyleBackground::Position &coordPosition =
            f->StyleDisplay()->mScrollSnapCoordinate[coordNum];
          nsPoint coordPoint = edgesRect.TopLeft() - aScrollSnapDestination;
          coordPoint += nsPoint(coordPosition.mXPosition.mLength,
                                coordPosition.mYPosition.mLength);
          if (coordPosition.mXPosition.mHasPercent) {
            coordPoint.x += NSToCoordRound(coordPosition.mXPosition.mPercent *
                                           frameRect.width);
          }
          if (coordPosition.mYPosition.mHasPercent) {
            coordPoint.y += NSToCoordRound(coordPosition.mYPosition.mPercent *
                                           frameRect.height);
          }

          aCallback.AddVerticalEdge(coordPoint.x);
          aCallback.AddHorizontalEdge(coordPoint.y);
        }
      }

      ScrollSnapHelper(aCallback, f, aScrolledFrame, aScrollSnapDestination);
    }
  }
}

bool
ScrollFrameHelper::GetSnapPointForDestination(nsIScrollableFrame::ScrollUnit aUnit,
                                              nsPoint aStartPos,
                                              nsPoint &aDestination)
{
  ScrollbarStyles styles = GetScrollbarStylesFromFrame();
  if (styles.mScrollSnapTypeY == NS_STYLE_SCROLL_SNAP_TYPE_NONE &&
      styles.mScrollSnapTypeX == NS_STYLE_SCROLL_SNAP_TYPE_NONE) {
    return false;
  }

  nsSize scrollPortSize = mScrollPort.Size();
  nsRect scrollRange = GetScrollRangeForClamping();

  nsPoint destPos = nsPoint(styles.mScrollSnapDestinationX.mLength,
                            styles.mScrollSnapDestinationY.mLength);
  if (styles.mScrollSnapDestinationX.mHasPercent) {
    destPos.x += NSToCoordFloorClamped(styles.mScrollSnapDestinationX.mPercent
                                       * scrollPortSize.width);
  }

  if (styles.mScrollSnapDestinationY.mHasPercent) {
    destPos.y += NSToCoordFloorClamped(styles.mScrollSnapDestinationY.mPercent
                                       * scrollPortSize.height);
  }

  CalcSnapPoints calcSnapPoints(aUnit, aDestination, aStartPos);

  if (styles.mScrollSnapPointsX.GetUnit() != eStyleUnit_None) {
    nscoord interval = nsRuleNode::ComputeCoordPercentCalc(styles.mScrollSnapPointsX,
                                                           scrollPortSize.width);
    calcSnapPoints.AddVerticalEdgeInterval(scrollRange, interval, destPos.x);
  }
  if (styles.mScrollSnapPointsY.GetUnit() != eStyleUnit_None) {
    nscoord interval = nsRuleNode::ComputeCoordPercentCalc(styles.mScrollSnapPointsY,
                                                           scrollPortSize.width);
    calcSnapPoints.AddHorizontalEdgeInterval(scrollRange, interval, destPos.y);
  }

  ScrollSnapHelper(calcSnapPoints, mScrolledFrame, mScrolledFrame, destPos);
  bool snapped = false;
  nsPoint finalPos = calcSnapPoints.GetBestEdge();
  nscoord proximityThreshold =
    Preferences::GetInt("layout.css.scroll-snap.proximity-threshold", 0);
  proximityThreshold = nsPresContext::CSSPixelsToAppUnits(proximityThreshold);
  if (styles.mScrollSnapTypeY == NS_STYLE_SCROLL_SNAP_TYPE_PROXIMITY &&
      std::abs(aDestination.y - finalPos.y) > proximityThreshold) {
    finalPos.y = aDestination.y;
  } else {
    snapped = true;
  }
  if (styles.mScrollSnapTypeX == NS_STYLE_SCROLL_SNAP_TYPE_PROXIMITY &&
      std::abs(aDestination.x - finalPos.x) > proximityThreshold) {
    finalPos.x = aDestination.x;
  } else {
    snapped = true;
  }
  if (snapped) {
    aDestination = finalPos;
  }
  return snapped;
}
