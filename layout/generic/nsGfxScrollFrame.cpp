






#include "nsGfxScrollFrame.h"

#include "base/compiler_specific.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsView.h"
#include "nsIScrollable.h"
#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsContentList.h"
#include "nsIDocumentInlines.h"
#include "nsFontMetrics.h"
#include "nsBoxLayoutState.h"
#include "nsINodeInfo.h"
#include "nsScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsITextControlFrame.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsAutoPtr.h"
#include "nsPresState.h"
#include "nsIHTMLDocument.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsBidiUtils.h"
#include "mozilla/ContentEvents.h"
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
#include <algorithm>
#include <cstdlib> 
#include <cmath> 

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layout;





nsIFrame*
NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot)
{
  return new (aPresShell) nsHTMLScrollFrame(aPresShell, aContext, aIsRoot);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLScrollFrame)

nsHTMLScrollFrame::nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, bool aIsRoot)
  : nsContainerFrame(aContext),
    mInner(ALLOW_THIS_IN_INITIALIZER_LIST(this), aIsRoot)
{
}

void
nsHTMLScrollFrame::ScrollbarActivityStarted() const
{
  if (mInner.mScrollbarActivity) {
    mInner.mScrollbarActivity->ActivityStarted();
  }
}

void
nsHTMLScrollFrame::ScrollbarActivityStopped() const
{
  if (mInner.mScrollbarActivity) {
    mInner.mScrollbarActivity->ActivityStopped();
  }
}

nsresult
nsHTMLScrollFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsHTMLScrollFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                            uint32_t aFilter)
{
  mInner.AppendAnonymousContentTo(aElements, aFilter);
}

void
nsHTMLScrollFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  mInner.Destroy();
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsHTMLScrollFrame::SetInitialChildList(ChildListID  aListID,
                                       nsFrameList& aChildList)
{
  nsresult rv = nsContainerFrame::SetInitialChildList(aListID, aChildList);
  mInner.ReloadChildFrames();
  return rv;
}


NS_IMETHODIMP
nsHTMLScrollFrame::AppendFrames(ChildListID  aListID,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  mFrames.AppendFrames(nullptr, aFrameList);
  mInner.ReloadChildFrames();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScrollFrame::InsertFrames(ChildListID aListID,
                                nsIFrame* aPrevFrame,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");
  mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);
  mInner.ReloadChildFrames();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScrollFrame::RemoveFrame(ChildListID aListID,
                               nsIFrame* aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "Only main list supported");
  mFrames.DestroyFrame(aOldFrame);
  mInner.ReloadChildFrames();
  return NS_OK;
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
      aState->mReflowState.mComputedPadding.LeftRight();
  }
  nscoord contentHeight = aState->mReflowState.ComputedHeight();
  if (contentHeight == NS_UNCONSTRAINEDSIZE) {
    contentHeight = aDesiredInsideBorderSize.height -
      aState->mReflowState.mComputedPadding.TopBottom();
  }

  contentWidth  = aState->mReflowState.ApplyMinMaxWidth(contentWidth);
  contentHeight = aState->mReflowState.ApplyMinMaxHeight(contentHeight);
  return nsSize(contentWidth + aState->mReflowState.mComputedPadding.LeftRight(),
                contentHeight + aState->mReflowState.mComputedPadding.TopBottom());
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
                             bool aForce, nsresult* aResult)
{
  *aResult = NS_OK;

  if ((aState->mStyles.mVertical == NS_STYLE_OVERFLOW_HIDDEN && aAssumeVScroll) ||
      (aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN && aAssumeHScroll)) {
    NS_ASSERTION(!aForce, "Shouldn't be forcing a hidden scrollbar to show!");
    return false;
  }

  if (aAssumeVScroll != aState->mReflowedContentsWithVScrollbar ||
      (aAssumeHScroll != aState->mReflowedContentsWithHScrollbar &&
       ScrolledContentDependsOnHeight(aState))) {
    nsresult rv = ReflowScrolledFrame(aState, aAssumeHScroll, aAssumeVScroll,
                                      aKidMetrics, false);
    if (NS_FAILED(rv)) {
      *aResult = rv;
      return false;
    }
  }

  nsSize vScrollbarMinSize(0, 0);
  nsSize vScrollbarPrefSize(0, 0);
  if (mInner.mVScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mInner.mVScrollbarBox,
                        &vScrollbarMinSize,
                        aAssumeVScroll ? &vScrollbarPrefSize : nullptr, true);
  }
  nscoord vScrollbarDesiredWidth = aAssumeVScroll ? vScrollbarPrefSize.width : 0;
  nscoord vScrollbarMinHeight = aAssumeVScroll ? vScrollbarMinSize.height : 0;

  nsSize hScrollbarMinSize(0, 0);
  nsSize hScrollbarPrefSize(0, 0);
  if (mInner.mHScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mInner.mHScrollbarBox,
                        &hScrollbarMinSize,
                        aAssumeHScroll ? &hScrollbarPrefSize : nullptr, false);
  }
  nscoord hScrollbarDesiredHeight = aAssumeHScroll ? hScrollbarPrefSize.height : 0;
  nscoord hScrollbarMinWidth = aAssumeHScroll ? hScrollbarMinSize.width : 0;

  
  
  nsSize desiredInsideBorderSize;
  desiredInsideBorderSize.width = vScrollbarDesiredWidth +
    std::max(aKidMetrics->width, hScrollbarMinWidth);
  desiredInsideBorderSize.height = hScrollbarDesiredHeight +
    std::max(aKidMetrics->height, vScrollbarMinHeight);
  aState->mInsideBorderSize =
    ComputeInsideBorderSize(aState, desiredInsideBorderSize);
  nsSize scrollPortSize = nsSize(std::max(0, aState->mInsideBorderSize.width - vScrollbarDesiredWidth),
                                 std::max(0, aState->mInsideBorderSize.height - hScrollbarDesiredHeight));

  if (!aForce) {
    nsRect scrolledRect =
      mInner.GetScrolledRectInternal(aState->mContentsOverflowAreas.ScrollableOverflow(),
                                     scrollPortSize);
    nscoord oneDevPixel = aState->mBoxState.PresContext()->DevPixelsToAppUnits(1);

    
    if (aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
      bool wantHScrollbar =
        aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.XMost() >= scrollPortSize.width + oneDevPixel ||
        scrolledRect.x <= -oneDevPixel;
      if (scrollPortSize.width < hScrollbarMinSize.width)
        wantHScrollbar = false;
      if (wantHScrollbar != aAssumeHScroll)
        return false;
    }

    
    if (aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
      bool wantVScrollbar =
        aState->mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.YMost() >= scrollPortSize.height + oneDevPixel ||
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
  if (!mInner.IsScrollbarOnRight()) {
    scrollPortOrigin.x += vScrollbarActualWidth;
  }
  mInner.mScrollPort = nsRect(scrollPortOrigin, scrollPortSize);
  return true;
}

bool
nsHTMLScrollFrame::ScrolledContentDependsOnHeight(ScrollReflowState* aState)
{
  
  
  return (mInner.mScrolledFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT) ||
    aState->mReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
    aState->mReflowState.mComputedMinHeight > 0 ||
    aState->mReflowState.mComputedMaxHeight != NS_UNCONSTRAINEDSIZE;
}

nsresult
nsHTMLScrollFrame::ReflowScrolledFrame(ScrollReflowState* aState,
                                       bool aAssumeHScroll,
                                       bool aAssumeVScroll,
                                       nsHTMLReflowMetrics* aMetrics,
                                       bool aFirstPass)
{
  
  
  nscoord paddingLR = aState->mReflowState.mComputedPadding.LeftRight();

  nscoord availWidth = aState->mReflowState.ComputedWidth() + paddingLR;

  nscoord computedHeight = aState->mReflowState.ComputedHeight();
  nscoord computedMinHeight = aState->mReflowState.mComputedMinHeight;
  nscoord computedMaxHeight = aState->mReflowState.mComputedMaxHeight;
  if (!ShouldPropagateComputedHeightToScrolledContent()) {
    computedHeight = NS_UNCONSTRAINEDSIZE;
    computedMinHeight = 0;
    computedMaxHeight = NS_UNCONSTRAINEDSIZE;
  }
  if (aAssumeHScroll) {
    nsSize hScrollbarPrefSize;
    GetScrollbarMetrics(aState->mBoxState, mInner.mHScrollbarBox,
                        nullptr, &hScrollbarPrefSize, false);
    if (computedHeight != NS_UNCONSTRAINEDSIZE)
      computedHeight = std::max(0, computedHeight - hScrollbarPrefSize.height);
    computedMinHeight = std::max(0, computedMinHeight - hScrollbarPrefSize.height);
    if (computedMaxHeight != NS_UNCONSTRAINEDSIZE)
      computedMaxHeight = std::max(0, computedMaxHeight - hScrollbarPrefSize.height);
  }

  if (aAssumeVScroll) {
    nsSize vScrollbarPrefSize;
    GetScrollbarMetrics(aState->mBoxState, mInner.mVScrollbarBox,
                        nullptr, &vScrollbarPrefSize, true);
    availWidth = std::max(0, availWidth - vScrollbarPrefSize.width);
  }

  nsPresContext* presContext = PresContext();

  
  nsHTMLReflowState kidReflowState(presContext, aState->mReflowState,
                                   mInner.mScrolledFrame,
                                   nsSize(availWidth, NS_UNCONSTRAINEDSIZE),
                                   -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  kidReflowState.Init(presContext, -1, -1, nullptr,
                      &aState->mReflowState.mComputedPadding);
  kidReflowState.mFlags.mAssumingHScrollbar = aAssumeHScroll;
  kidReflowState.mFlags.mAssumingVScrollbar = aAssumeVScroll;
  kidReflowState.SetComputedHeight(computedHeight);
  kidReflowState.mComputedMinHeight = computedMinHeight;
  kidReflowState.mComputedMaxHeight = computedMaxHeight;

  
  
  bool didHaveHorizontalScrollbar = mInner.mHasHorizontalScrollbar;
  bool didHaveVerticalScrollbar = mInner.mHasVerticalScrollbar;
  mInner.mHasHorizontalScrollbar = aAssumeHScroll;
  mInner.mHasVerticalScrollbar = aAssumeVScroll;

  nsReflowStatus status;
  nsresult rv = ReflowChild(mInner.mScrolledFrame, presContext, *aMetrics,
                            kidReflowState, 0, 0,
                            NS_FRAME_NO_MOVE_FRAME, status);

  mInner.mHasHorizontalScrollbar = didHaveHorizontalScrollbar;
  mInner.mHasVerticalScrollbar = didHaveVerticalScrollbar;

  
  
  
  
  
  FinishReflowChild(mInner.mScrolledFrame, presContext,
                    &kidReflowState, *aMetrics, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_SIZE_VIEW);

  
  
  
  
  
  
  
  aMetrics->UnionOverflowAreasWithDesiredBounds();

  aState->mContentsOverflowAreas = aMetrics->mOverflowAreas;
  aState->mReflowedContentsWithHScrollbar = aAssumeHScroll;
  aState->mReflowedContentsWithVScrollbar = aAssumeVScroll;

  return rv;
}

bool
nsHTMLScrollFrame::GuessHScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mHorizontal != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL;

  return mInner.mHasHorizontalScrollbar;
}

bool
nsHTMLScrollFrame::GuessVScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mVertical != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL;

  
  
  
  if (mInner.mHadNonInitialReflow) {
    return mInner.mHasVerticalScrollbar;
  }

  
  
  if (InInitialReflow())
    return false;

  if (mInner.mIsRoot) {
    nsIFrame *f = mInner.mScrolledFrame->GetFirstPrincipalChild();
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
  
  
  
  
  
  
  return !mInner.mIsRoot && (GetStateBits() & NS_FRAME_FIRST_REFLOW);
}

nsresult
nsHTMLScrollFrame::ReflowContents(ScrollReflowState* aState,
                                  const nsHTMLReflowMetrics& aDesiredSize)
{
  nsHTMLReflowMetrics kidDesiredSize(aDesiredSize.mFlags);
  nsresult rv = ReflowScrolledFrame(aState, GuessHScrollbarNeeded(*aState),
      GuessVScrollbarNeeded(*aState), &kidDesiredSize, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  
  
  

  
  
  

  
  
  if ((aState->mReflowedContentsWithHScrollbar || aState->mReflowedContentsWithVScrollbar) &&
      aState->mStyles.mVertical != NS_STYLE_OVERFLOW_SCROLL &&
      aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL) {
    nsSize insideBorderSize =
      ComputeInsideBorderSize(aState,
                              nsSize(kidDesiredSize.width, kidDesiredSize.height));
    nsRect scrolledRect =
      mInner.GetScrolledRectInternal(kidDesiredSize.ScrollableOverflow(),
                                     insideBorderSize);
    if (nsRect(nsPoint(0, 0), insideBorderSize).Contains(scrolledRect)) {
      
      rv = ReflowScrolledFrame(aState, false, false,
                               &kidDesiredSize, false);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  

  
  
  if (TryLayout(aState, &kidDesiredSize, aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, false, &rv))
    return NS_OK;
  if (TryLayout(aState, &kidDesiredSize, !aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, false, &rv))
    return NS_OK;

  
  
  
  
  bool newVScrollbarState = !aState->mReflowedContentsWithVScrollbar;
  if (TryLayout(aState, &kidDesiredSize, false, newVScrollbarState, false, &rv))
    return NS_OK;
  if (TryLayout(aState, &kidDesiredSize, true, newVScrollbarState, false, &rv))
    return NS_OK;

  
  
  
  TryLayout(aState, &kidDesiredSize,
            aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN,
            aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN,
            true, &rv);
  return rv;
}

void
nsHTMLScrollFrame::PlaceScrollArea(const ScrollReflowState& aState,
                                   const nsPoint& aScrollPosition)
{
  nsIFrame *scrolledFrame = mInner.mScrolledFrame;
  
  scrolledFrame->SetPosition(mInner.mScrollPort.TopLeft() - aScrollPosition);

  nsRect scrolledArea;
  
  nsSize portSize = mInner.mScrollPort.Size();
  nsRect scrolledRect =
    mInner.GetScrolledRectInternal(aState.mContentsOverflowAreas.ScrollableOverflow(),
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
  if (ss.mVertical != NS_STYLE_OVERFLOW_SCROLL || !mInner.mVScrollbarBox)
    return 0;

  
  
  nsBoxLayoutState bls(PresContext(), aRenderingContext, 0);
  nsSize vScrollbarPrefSize(0, 0);
  GetScrollbarMetrics(bls, mInner.mVScrollbarBox,
                      nullptr, &vScrollbarPrefSize, true);
  return vScrollbarPrefSize.width;
}

 nscoord
nsHTMLScrollFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = mInner.mScrolledFrame->GetMinWidth(aRenderingContext);
  DISPLAY_MIN_WIDTH(this, result);
  return result + GetIntrinsicVScrollbarWidth(aRenderingContext);
}

 nscoord
nsHTMLScrollFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = mInner.mScrolledFrame->GetPrefWidth(aRenderingContext);
  DISPLAY_PREF_WIDTH(this, result);
  return NSCoordSaturatingAdd(result, GetIntrinsicVScrollbarWidth(aRenderingContext));
}

NS_IMETHODIMP
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
      nsCOMPtr<nsIContent> frameContent =
        do_QueryInterface(win->GetFrameElementInternal());
      if (frameContent &&
          frameContent->NodeInfo()->Equals(nsGkAtoms::browser, kNameSpaceID_XUL))
        return frameContent;
    }
  }

  return nullptr;
}

NS_IMETHODIMP
nsHTMLScrollFrame::Reflow(nsPresContext*           aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsHTMLScrollFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  mInner.HandleScrollbarStyleSwitching();

  ScrollReflowState state(this, aReflowState);
  
  
  if (!mInner.mVScrollbarBox || mInner.mNeverHasVerticalScrollbar)
    state.mStyles.mVertical = NS_STYLE_OVERFLOW_HIDDEN;
  if (!mInner.mHScrollbarBox || mInner.mNeverHasHorizontalScrollbar)
    state.mStyles.mHorizontal = NS_STYLE_OVERFLOW_HIDDEN;

  
  bool reflowHScrollbar = true;
  bool reflowVScrollbar = true;
  bool reflowScrollCorner = true;
  if (!aReflowState.ShouldReflowAllKids()) {
    #define NEEDS_REFLOW(frame_) \
      ((frame_) && NS_SUBTREE_DIRTY(frame_))

    reflowHScrollbar = NEEDS_REFLOW(mInner.mHScrollbarBox);
    reflowVScrollbar = NEEDS_REFLOW(mInner.mVScrollbarBox);
    reflowScrollCorner = NEEDS_REFLOW(mInner.mScrollCornerBox) ||
                         NEEDS_REFLOW(mInner.mResizerBox);

    #undef NEEDS_REFLOW
  }

  if (mInner.mIsRoot) {
    mInner.mCollapsedResizer = true;

    nsIContent* browserRoot = GetBrowserRoot(mContent);
    if (browserRoot) {
      bool showResizer = browserRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::showresizer);
      reflowScrollCorner = showResizer == mInner.mCollapsedResizer;
      mInner.mCollapsedResizer = !showResizer;
    }
  }

  nsRect oldScrollAreaBounds = mInner.mScrollPort;
  nsRect oldScrolledAreaBounds =
    mInner.mScrolledFrame->GetScrollableOverflowRectRelativeToParent();
  nsPoint oldScrollPosition = mInner.GetScrollPosition();

  state.mComputedBorder = aReflowState.mComputedBorderPadding -
    aReflowState.mComputedPadding;

  nsresult rv = ReflowContents(&state, aDesiredSize);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  PlaceScrollArea(state, oldScrollPosition);
  if (!mInner.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mInner);
    mInner.mPostedReflowCallback = true;
  }

  bool didHaveHScrollbar = mInner.mHasHorizontalScrollbar;
  bool didHaveVScrollbar = mInner.mHasVerticalScrollbar;
  mInner.mHasHorizontalScrollbar = state.mShowHScrollbar;
  mInner.mHasVerticalScrollbar = state.mShowVScrollbar;
  nsRect newScrollAreaBounds = mInner.mScrollPort;
  nsRect newScrolledAreaBounds =
    mInner.mScrolledFrame->GetScrollableOverflowRectRelativeToParent();
  if (mInner.mSkippedScrollbarLayout ||
      reflowHScrollbar || reflowVScrollbar || reflowScrollCorner ||
      (GetStateBits() & NS_FRAME_IS_DIRTY) ||
      didHaveHScrollbar != state.mShowHScrollbar ||
      didHaveVScrollbar != state.mShowVScrollbar ||
      !oldScrollAreaBounds.IsEqualEdges(newScrollAreaBounds) ||
      !oldScrolledAreaBounds.IsEqualEdges(newScrolledAreaBounds)) {
    if (!mInner.mSupppressScrollbarUpdate) {
      mInner.mSkippedScrollbarLayout = false;
      mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, state.mShowHScrollbar);
      mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, state.mShowVScrollbar);
      
      nsRect insideBorderArea =
        nsRect(nsPoint(state.mComputedBorder.left, state.mComputedBorder.top),
               state.mInsideBorderSize);
      mInner.LayoutScrollbars(state.mBoxState, insideBorderArea,
                              oldScrollAreaBounds);
    } else {
      mInner.mSkippedScrollbarLayout = true;
    }
  }

  aDesiredSize.width = state.mInsideBorderSize.width +
    state.mComputedBorder.LeftRight();
  aDesiredSize.height = state.mInsideBorderSize.height +
    state.mComputedBorder.TopBottom();

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (mInner.IsIgnoringViewportClipping()) {
    aDesiredSize.mOverflowAreas.UnionWith(
      state.mContentsOverflowAreas + mInner.mScrolledFrame->GetPosition());
  }

  mInner.UpdateSticky();
  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  if (!InInitialReflow() && !mInner.mHadNonInitialReflow) {
    mInner.mHadNonInitialReflow = true;
  }

  if (mInner.mIsRoot && !oldScrolledAreaBounds.IsEqualEdges(newScrolledAreaBounds)) {
    mInner.PostScrolledAreaEvent();
  }

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  mInner.PostOverflowEvent();
  return rv;
}




#ifdef DEBUG
NS_IMETHODIMP
nsHTMLScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLScroll"), aResult);
}
#endif

#ifdef ACCESSIBILITY
a11y::AccType
nsHTMLScrollFrame::AccessibleType()
{
  
  
  if (mContent->IsRootOfNativeAnonymousSubtree() ||
      GetScrollbarStyles() ==
        ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, NS_STYLE_OVERFLOW_HIDDEN) ) {
    return a11y::eNoType;
  }

  return a11y::eHyperTextType;
}
#endif

NS_QUERYFRAME_HEAD(nsHTMLScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollbarOwner)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)



nsIFrame*
NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     bool aIsRoot, bool aClipAllDescendants)
{
  return new (aPresShell) nsXULScrollFrame(aPresShell, aContext, aIsRoot,
                                           aClipAllDescendants);
}

NS_IMPL_FRAMEARENA_HELPERS(nsXULScrollFrame)

nsXULScrollFrame::nsXULScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext,
                                   bool aIsRoot, bool aClipAllDescendants)
  : nsBoxFrame(aShell, aContext, aIsRoot),
    mInner(ALLOW_THIS_IN_INITIALIZER_LIST(this), aIsRoot)
{
  SetLayoutManager(nullptr);
  mInner.mClipAllDescendants = aClipAllDescendants;
}

void
nsXULScrollFrame::ScrollbarActivityStarted() const
{
  if (mInner.mScrollbarActivity) {
    mInner.mScrollbarActivity->ActivityStarted();
  }
}

void
nsXULScrollFrame::ScrollbarActivityStopped() const
{
  if (mInner.mScrollbarActivity) {
    mInner.mScrollbarActivity->ActivityStopped();
  }
}

nsMargin
nsGfxScrollFrameInner::GetDesiredScrollbarSizes(nsBoxLayoutState* aState)
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
nsGfxScrollFrameInner::GetNondisappearingScrollbarWidth(nsBoxLayoutState* aState)
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
      nsIntSize size;
      nsRenderingContext* rendContext = aState->GetRenderingContext();
      if (rendContext) {
        bool canOverride = true;
        theme->GetMinimumWidgetSize(rendContext,
                                    mVScrollbarBox,
                                    NS_THEME_SCROLLBAR_NON_DISAPPEARING,
                                    &size,
                                    &canOverride);
        if (size.width) {
          return aState->PresContext()->DevPixelsToAppUnits(size.width);
        }
      }
    }
  }

  return GetDesiredScrollbarSizes(aState).LeftRight();
}

void
nsGfxScrollFrameInner::HandleScrollbarStyleSwitching()
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

nsresult
nsXULScrollFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsXULScrollFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                           uint32_t aFilter)
{
  mInner.AppendAnonymousContentTo(aElements, aFilter);
}

void
nsXULScrollFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mInner.Destroy();
  nsBoxFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsXULScrollFrame::SetInitialChildList(ChildListID     aListID,
                                      nsFrameList&    aChildList)
{
  nsresult rv = nsBoxFrame::SetInitialChildList(aListID, aChildList);
  mInner.ReloadChildFrames();
  return rv;
}


NS_IMETHODIMP
nsXULScrollFrame::AppendFrames(ChildListID     aListID,
                               nsFrameList&    aFrameList)
{
  nsresult rv = nsBoxFrame::AppendFrames(aListID, aFrameList);
  mInner.ReloadChildFrames();
  return rv;
}

NS_IMETHODIMP
nsXULScrollFrame::InsertFrames(ChildListID     aListID,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  nsresult rv = nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
  mInner.ReloadChildFrames();
  return rv;
}

NS_IMETHODIMP
nsXULScrollFrame::RemoveFrame(ChildListID     aListID,
                              nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aListID, aOldFrame);
  mInner.ReloadChildFrames();
  return rv;
}

nsSplittableType
nsXULScrollFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

NS_IMETHODIMP
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
  if (!mInner.mScrolledFrame)
    return 0;

  nscoord ascent = mInner.mScrolledFrame->GetBoxAscent(aState);
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

  nsSize pref = mInner.mScrolledFrame->GetPrefSize(aState);

  ScrollbarStyles styles = GetScrollbarStyles();

  

  if (mInner.mVScrollbarBox &&
      styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
    nsSize vSize = mInner.mVScrollbarBox->GetPrefSize(aState);
    nsBox::AddMargin(mInner.mVScrollbarBox, vSize);
    pref.width += vSize.width;
  }

  if (mInner.mHScrollbarBox &&
      styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL) {
    nsSize hSize = mInner.mHScrollbarBox->GetPrefSize(aState);
    nsBox::AddMargin(mInner.mHScrollbarBox, hSize);
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

  nsSize min = mInner.mScrolledFrame->GetMinSizeForScrollArea(aState);

  ScrollbarStyles styles = GetScrollbarStyles();

  if (mInner.mVScrollbarBox &&
      styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
     nsSize vSize = mInner.mVScrollbarBox->GetMinSize(aState);
     AddMargin(mInner.mVScrollbarBox, vSize);
     min.width += vSize.width;
     if (min.height < vSize.height)
        min.height = vSize.height;
  }

  if (mInner.mHScrollbarBox &&
      styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL) {
     nsSize hSize = mInner.mHScrollbarBox->GetMinSize(aState);
     AddMargin(mInner.mHScrollbarBox, hSize);
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

#ifdef DEBUG
NS_IMETHODIMP
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
  NS_QUERYFRAME_ENTRY(nsIScrollbarOwner)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)



#define SMOOTH_SCROLL_PREF_NAME "general.smoothScroll"

const double kCurrentVelocityWeighting = 0.25;
const double kStopDecelerationWeighting = 0.4;


class nsGfxScrollFrameInner::AsyncScroll MOZ_FINAL : public nsARefreshObserver {
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  AsyncScroll(nsPoint aStartPos)
    : mIsFirstIteration(true)
    , mStartPos(aStartPos)
    , mCallee(nullptr)
  {}

  ~AsyncScroll() {
    RemoveObserver();
  }

  nsPoint PositionAt(TimeStamp aTime);
  nsSize VelocityAt(TimeStamp aTime); 

  void InitSmoothScroll(TimeStamp aTime, nsPoint aDestination,
                        nsIAtom *aOrigin, const nsRect& aRange);
  void Init(const nsRect& aRange) {
    mRange = aRange;
  }

  bool IsFinished(TimeStamp aTime) {
    return aTime > mStartTime + mDuration; 
  }

  TimeStamp mStartTime;

  
  
  
  
  
  TimeStamp mPrevEventTime[3];
  bool mIsFirstIteration;

  
  
  
  
  
  
  
  nsCOMPtr<nsIAtom> mOrigin;
  int32_t mOriginMinMS;
  int32_t mOriginMaxMS;
  double  mIntervalRatio;

  TimeDuration mDuration;
  nsPoint mStartPos;
  nsPoint mDestination;
  
  nsRect mRange;
  nsSMILKeySpline mTimingFunctionX;
  nsSMILKeySpline mTimingFunctionY;
  bool mIsSmoothScroll;

protected:
  double ProgressAt(TimeStamp aTime) {
    return clamped((aTime - mStartTime) / mDuration, 0.0, 1.0);
  }

  nscoord VelocityComponent(double aTimeProgress,
                            nsSMILKeySpline& aTimingFunction,
                            nscoord aStart, nscoord aDestination);

  
  
  void InitTimingFunction(nsSMILKeySpline& aTimingFunction,
                          nscoord aCurrentPos, nscoord aCurrentVelocity,
                          nscoord aDestination);

  TimeDuration CalcDurationForEventTime(TimeStamp aTime, nsIAtom *aOrigin);



public:
  NS_INLINE_DECL_REFCOUNTING(AsyncScroll)

  




  bool SetRefreshObserver(nsGfxScrollFrameInner *aCallee) {
    NS_ASSERTION(aCallee && !mCallee, "AsyncScroll::SetRefreshObserver - Invalid usage.");

    if (!RefreshDriver(aCallee)->AddRefreshObserver(this, Flush_Style)) {
      return false;
    }

    mCallee = aCallee;
    return true;
  }

  virtual void WillRefresh(mozilla::TimeStamp aTime) {
    
    
    nsGfxScrollFrameInner::AsyncScrollCallback(mCallee, aTime);
  }

private:
  nsGfxScrollFrameInner *mCallee;

  nsRefreshDriver* RefreshDriver(nsGfxScrollFrameInner* aCallee) {
    return aCallee->mOuter->PresContext()->RefreshDriver();
  }

  




  void RemoveObserver() {
    if (mCallee) {
      RefreshDriver(mCallee)->RemoveRefreshObserver(this, Flush_Style);
    }
  }
};

nsPoint
nsGfxScrollFrameInner::AsyncScroll::PositionAt(TimeStamp aTime) {
  double progressX = mTimingFunctionX.GetSplineValue(ProgressAt(aTime));
  double progressY = mTimingFunctionY.GetSplineValue(ProgressAt(aTime));
  return nsPoint(NSToCoordRound((1 - progressX) * mStartPos.x + progressX * mDestination.x),
                 NSToCoordRound((1 - progressY) * mStartPos.y + progressY * mDestination.y));
}

nsSize
nsGfxScrollFrameInner::AsyncScroll::VelocityAt(TimeStamp aTime) {
  double timeProgress = ProgressAt(aTime);
  return nsSize(VelocityComponent(timeProgress, mTimingFunctionX,
                                  mStartPos.x, mDestination.x),
                VelocityComponent(timeProgress, mTimingFunctionY,
                                  mStartPos.y, mDestination.y));
}





TimeDuration
nsGfxScrollFrameInner::
AsyncScroll::CalcDurationForEventTime(TimeStamp aTime, nsIAtom *aOrigin) {
  if (!aOrigin){
    aOrigin = nsGkAtoms::other;
  }

  
  if (mIsFirstIteration || aOrigin != mOrigin) {
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
      
      

      
      TimeDuration maxDelta = TimeDuration::FromMilliseconds(mOriginMaxMS / mIntervalRatio);
      mPrevEventTime[0] = aTime              - maxDelta;
      mPrevEventTime[1] = mPrevEventTime[0]  - maxDelta;
      mPrevEventTime[2] = mPrevEventTime[1]  - maxDelta;
    }
  }

  
  int32_t eventsDeltaMs = (aTime - mPrevEventTime[2]).ToMilliseconds() / 3;
  mPrevEventTime[2] = mPrevEventTime[1];
  mPrevEventTime[1] = mPrevEventTime[0];
  mPrevEventTime[0] = aTime;

  
  
  
  
  
  int32_t durationMS = clamped<int32_t>(eventsDeltaMs * mIntervalRatio, mOriginMinMS, mOriginMaxMS);

  return TimeDuration::FromMilliseconds(durationMS);
}

void
nsGfxScrollFrameInner::AsyncScroll::InitSmoothScroll(TimeStamp aTime,
                                                     nsPoint aDestination,
                                                     nsIAtom *aOrigin,
                                                     const nsRect& aRange) {
  mRange = aRange;
  TimeDuration duration = CalcDurationForEventTime(aTime, aOrigin);
  nsSize currentVelocity(0, 0);
  if (!mIsFirstIteration) {
    
    
    
    if (aDestination == mDestination &&
        aTime + duration > mStartTime + mDuration)
      return;

    currentVelocity = VelocityAt(aTime);
    mStartPos = PositionAt(aTime);
  }
  mStartTime = aTime;
  mDuration = duration;
  mDestination = aDestination;
  InitTimingFunction(mTimingFunctionX, mStartPos.x, currentVelocity.width,
                     aDestination.x);
  InitTimingFunction(mTimingFunctionY, mStartPos.y, currentVelocity.height,
                     aDestination.y);
  mIsFirstIteration = false;
}


nscoord
nsGfxScrollFrameInner::AsyncScroll::VelocityComponent(double aTimeProgress,
                                                      nsSMILKeySpline& aTimingFunction,
                                                      nscoord aStart,
                                                      nscoord aDestination)
{
  double dt, dxy;
  aTimingFunction.GetSplineDerivativeValues(aTimeProgress, dt, dxy);
  if (dt == 0)
    return dxy >= 0 ? nscoord_MAX : nscoord_MIN;

  const TimeDuration oneSecond = TimeDuration::FromSeconds(1);
  double slope = dxy / dt;
  return NSToCoordRound(slope * (aDestination - aStart) / (mDuration / oneSecond));
}

void
nsGfxScrollFrameInner::AsyncScroll::InitTimingFunction(nsSMILKeySpline& aTimingFunction,
                                                       nscoord aCurrentPos,
                                                       nscoord aCurrentVelocity,
                                                       nscoord aDestination)
{
  if (aDestination == aCurrentPos || kCurrentVelocityWeighting == 0) {
    aTimingFunction.Init(0, 0, 1 - kStopDecelerationWeighting, 1);
    return;
  }

  const TimeDuration oneSecond = TimeDuration::FromSeconds(1);
  double slope = aCurrentVelocity * (mDuration / oneSecond) / (aDestination - aCurrentPos);
  double normalization = sqrt(1.0 + slope * slope);
  double dt = 1.0 / normalization * kCurrentVelocityWeighting;
  double dxy = slope / normalization * kCurrentVelocityWeighting;
  aTimingFunction.Init(dt, dxy, 1 - kStopDecelerationWeighting, 1);
}

static bool
IsSmoothScrollingEnabled()
{
  return Preferences::GetBool(SMOOTH_SCROLL_PREF_NAME, false);
}

class ScrollFrameActivityTracker MOZ_FINAL : public nsExpirationTracker<nsGfxScrollFrameInner,4> {
public:
  
  
  enum { TIMEOUT_MS = 1000 };
  ScrollFrameActivityTracker()
    : nsExpirationTracker<nsGfxScrollFrameInner,4>(TIMEOUT_MS) {}
  ~ScrollFrameActivityTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(nsGfxScrollFrameInner *aObject) {
    RemoveObject(aObject);
    aObject->MarkInactive();
  }
};

static ScrollFrameActivityTracker *gScrollFrameActivityTracker = nullptr;

nsGfxScrollFrameInner::nsGfxScrollFrameInner(nsContainerFrame* aOuter,
                                             bool aIsRoot)
  : mHScrollbarBox(nullptr)
  , mVScrollbarBox(nullptr)
  , mScrolledFrame(nullptr)
  , mScrollCornerBox(nullptr)
  , mResizerBox(nullptr)
  , mOuter(aOuter)
  , mAsyncScroll(nullptr)
  , mDestination(0, 0)
  , mScrollPosAtLastPaint(0, 0)
  , mRestorePos(-1, -1)
  , mLastPos(-1, -1)
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
  , mCollapsedResizer(false)
  , mShouldBuildLayer(false)
  , mHasBeenScrolled(false)
{
  mScrollingActive = IsAlwaysActive();

  if (LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0) {
    mScrollbarActivity = new ScrollbarActivity(do_QueryFrame(aOuter));
  }

  EnsureImageVisPrefsCached();
}

nsGfxScrollFrameInner::~nsGfxScrollFrameInner()
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
nsGfxScrollFrameInner::AsyncScrollCallback(void* anInstance, mozilla::TimeStamp aTime)
{
  nsGfxScrollFrameInner* self = static_cast<nsGfxScrollFrameInner*>(anInstance);
  if (!self || !self->mAsyncScroll)
    return;

  nsRect range = self->mAsyncScroll->mRange;
  if (self->mAsyncScroll->mIsSmoothScroll) {
    if (!self->mAsyncScroll->IsFinished(aTime)) {
      nsPoint destination = self->mAsyncScroll->PositionAt(aTime);
      
      
      
      nsRect intermediateRange =
        nsRect(self->GetScrollPosition(), nsSize()).UnionEdges(range);
      self->ScrollToImpl(destination, intermediateRange);
      
      return;
    }
  }

  
  self->mAsyncScroll = nullptr;
  nsWeakFrame weakFrame(self->mOuter);
  self->ScrollToImpl(self->mDestination, range);
  if (!weakFrame.IsAlive()) {
    return;
  }
  
  
  self->mDestination = self->GetScrollPosition();
}

void
nsGfxScrollFrameInner::ScrollToCSSPixels(const CSSIntPoint& aScrollPosition)
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
  ScrollTo(pt, nsIScrollableFrame::INSTANT, &range);
  
}

void
nsGfxScrollFrameInner::ScrollToCSSPixelsApproximate(const CSSPoint& aScrollPosition)
{
  nsPoint pt = CSSPoint::ToAppUnits(aScrollPosition);
  nscoord halfRange = nsPresContext::CSSPixelsToAppUnits(1000);
  nsRect range(pt.x - halfRange, pt.y - halfRange, 2*halfRange - 1, 2*halfRange - 1);
  ScrollTo(pt, nsIScrollableFrame::INSTANT, &range);
  
}

CSSIntPoint
nsGfxScrollFrameInner::GetScrollPositionCSSPixels()
{
  return CSSIntPoint::FromAppUnitsRounded(GetScrollPosition());
}





void
nsGfxScrollFrameInner::ScrollToWithOrigin(nsPoint aScrollPosition,
                                          nsIScrollableFrame::ScrollMode aMode,
                                          nsIAtom *aOrigin,
                                          const nsRect* aRange)
{
  nsRect scrollRange = GetScrollRangeForClamping();
  mDestination = scrollRange.ClampPoint(aScrollPosition);

  nsRect range = aRange ? *aRange : nsRect(aScrollPosition, nsSize(0, 0));

  if (aMode == nsIScrollableFrame::INSTANT) {
    
    
    mAsyncScroll = nullptr;
    nsWeakFrame weakFrame(mOuter);
    ScrollToImpl(mDestination, range);
    if (!weakFrame.IsAlive()) {
      return;
    }
    
    
    mDestination = GetScrollPosition();
    return;
  }

  TimeStamp now = TimeStamp::Now();
  bool isSmoothScroll = (aMode == nsIScrollableFrame::SMOOTH) &&
                          IsSmoothScrollingEnabled();

  if (!mAsyncScroll) {
    mAsyncScroll = new AsyncScroll(GetScrollPosition());
    if (!mAsyncScroll->SetRefreshObserver(this)) {
      mAsyncScroll = nullptr;
      
      nsWeakFrame weakFrame(mOuter);
      ScrollToImpl(mDestination, range);
      if (!weakFrame.IsAlive()) {
        return;
      }
      
      
      mDestination = GetScrollPosition();
      return;
    }
  }

  mAsyncScroll->mIsSmoothScroll = isSmoothScroll;

  if (isSmoothScroll) {
    mAsyncScroll->InitSmoothScroll(now, mDestination, aOrigin, range);
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
CanScrollWithBlitting(nsIFrame* aFrame)
{
  if (aFrame->GetStateBits() & NS_SCROLLFRAME_INVALIDATE_CONTENTS_ON_SCROLL)
    return false;

  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    if (nsSVGIntegrationUtils::UsingEffectsForFrame(f) ||
        f->IsFrameOfType(nsIFrame::eSVG) ||
        f->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) {
      return false;
    }
    if (nsLayoutUtils::IsPopup(f))
      break;
  }
  return true;
}

bool nsGfxScrollFrameInner::IsIgnoringViewportClipping() const
{
  if (!mIsRoot)
    return false;
  nsSubDocumentFrame* subdocFrame = static_cast<nsSubDocumentFrame*>
    (nsLayoutUtils::GetCrossDocParentFrame(mOuter->PresContext()->PresShell()->GetRootFrame()));
  return subdocFrame && !subdocFrame->ShouldClipSubdocument();
}

bool nsGfxScrollFrameInner::ShouldClampScrollPosition() const
{
  if (!mIsRoot)
    return true;
  nsSubDocumentFrame* subdocFrame = static_cast<nsSubDocumentFrame*>
    (nsLayoutUtils::GetCrossDocParentFrame(mOuter->PresContext()->PresShell()->GetRootFrame()));
  return !subdocFrame || subdocFrame->ShouldClampScrollPosition();
}

bool nsGfxScrollFrameInner::IsAlwaysActive() const
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

void nsGfxScrollFrameInner::MarkInactive()
{
  if (IsAlwaysActive() || !mScrollingActive)
    return;

  mScrollingActive = false;
  mOuter->InvalidateFrameSubtree();
}

void nsGfxScrollFrameInner::MarkActive()
{
  mScrollingActive = true;
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

void nsGfxScrollFrameInner::ScrollVisual(nsPoint aOldScrolledFramePos)
{
  
  
  
  
  mHasBeenScrolled = true;

  AdjustViews(mScrolledFrame);
  
  
  bool canScrollWithBlitting = CanScrollWithBlitting(mOuter);
  mOuter->RemoveStateBits(NS_SCROLLFRAME_INVALIDATE_CONTENTS_ON_SCROLL);
  if (IsScrollingActive()) {
    if (!canScrollWithBlitting) {
      MarkInactive();
    }
  }
  if (canScrollWithBlitting) {
    MarkActive();
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
nsGfxScrollFrameInner::ScrollActivityCallback(nsITimer *aTimer, void* anInstance)
{
  nsGfxScrollFrameInner* self = static_cast<nsGfxScrollFrameInner*>(anInstance);

  
  self->mScrollActivityTimer->Cancel();
  self->mScrollActivityTimer = nullptr;
  self->mOuter->PresContext()->PresShell()->SynthesizeMouseMove(true);
}


void
nsGfxScrollFrameInner::ScheduleSyntheticMouseMove()
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
nsGfxScrollFrameInner::ScrollToImpl(nsPoint aPt, const nsRect& aRange)
{
  nsPresContext* presContext = mOuter->PresContext();
  nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  
  
  gfxSize scale = FrameLayerBuilder::GetThebesLayerScaleForFrame(mScrolledFrame);
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

  
  ScrollVisual(oldScrollFramePos);

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
}

static void
AppendToTop(nsDisplayListBuilder* aBuilder, nsDisplayList* aDest,
            nsDisplayList* aSource, nsIFrame* aSourceFrame, bool aOwnLayer)
{
  if (aSource->IsEmpty())
    return;
  if (aOwnLayer) {
    aDest->AppendNewToTop(
        new (aBuilder) nsDisplayOwnLayer(aBuilder, aSourceFrame, aSource));
  } else {
    aDest->AppendToTop(aSource);
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
nsGfxScrollFrameInner::AppendScrollPartsTo(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists,
                                           bool&                   aCreateLayer,
                                           bool                    aPositioned)
{
  bool overlayScrollbars =
    LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0;

  nsAutoTArray<nsIFrame*, 3> scrollParts;
  for (nsIFrame* kid = mOuter->GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
    if (kid == mScrolledFrame ||
        (kid->IsPositioned() || overlayScrollbars) != aPositioned)
      continue;

    scrollParts.AppendElement(kid);
  }

  scrollParts.Sort(HoveredStateComparator());

  for (uint32_t i = 0; i < scrollParts.Length(); ++i) {
    nsDisplayListCollection partList;
    mOuter->BuildDisplayListForChild(
      aBuilder, scrollParts[i], aDirtyRect, partList,
      nsIFrame::DISPLAY_CHILD_FORCE_STACKING_CONTEXT);

    
    
    bool appendToPositioned = aPositioned &&
                              !(scrollParts[i] == mResizerBox && !mIsRoot);

    nsDisplayList* dest = appendToPositioned ?
      aLists.PositionedDescendants() : aLists.BorderBackground();

    
    
    ::AppendToTop(aBuilder, dest,
                  partList.PositionedDescendants(), scrollParts[i],
                  aCreateLayer);
  }
}

bool
nsGfxScrollFrameInner::ShouldBuildLayer() const
{
  return mShouldBuildLayer;
}

class ScrollLayerWrapper : public nsDisplayWrapper
{
public:
  ScrollLayerWrapper(nsIFrame* aScrollFrame, nsIFrame* aScrolledFrame)
    : mCount(0)
    , mProps(aScrolledFrame->Properties())
    , mScrollFrame(aScrollFrame)
    , mScrolledFrame(aScrolledFrame)
  {
    SetCount(0);
  }

  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame,
                                  nsDisplayList* aList) {
    SetCount(++mCount);
    return new (aBuilder) nsDisplayScrollLayer(aBuilder, aList, mScrolledFrame, mScrolledFrame, mScrollFrame);
  }

  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {

    SetCount(++mCount);
    return new (aBuilder) nsDisplayScrollLayer(aBuilder, aItem, aItem->Frame(), mScrolledFrame, mScrollFrame);
  }

protected:
  void SetCount(intptr_t aCount) {
    mProps.Set(nsIFrame::ScrollLayerCount(), reinterpret_cast<void*>(aCount));
  }

  intptr_t mCount;
  FrameProperties mProps;
  nsIFrame* mScrollFrame;
  nsIFrame* mScrolledFrame;
};

 bool nsGfxScrollFrameInner::sImageVisPrefsCached = false;
 uint32_t nsGfxScrollFrameInner::sHorzExpandScrollPort = 0;
 uint32_t nsGfxScrollFrameInner::sVertExpandScrollPort = 1;
 int32_t nsGfxScrollFrameInner::sHorzScrollFraction = 2;
 int32_t nsGfxScrollFrameInner::sVertScrollFraction = 2;

 void
nsGfxScrollFrameInner::EnsureImageVisPrefsCached()
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
nsGfxScrollFrameInner::ExpandRect(const nsRect& aRect) const
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

void
nsGfxScrollFrameInner::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  if (aBuilder->IsForImageVisibility()) {
    mLastUpdateImagesPos = GetScrollPosition();
  }

  mOuter->DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (aBuilder->IsPaintingToWindow()) {
    mScrollPosAtLastPaint = GetScrollPosition();
    if (IsScrollingActive() && !CanScrollWithBlitting(mOuter)) {
      MarkInactive();
    }
    if (IsScrollingActive()) {
      if (mScrollPosForLayerPixelAlignment == nsPoint(-1,-1)) {
        mScrollPosForLayerPixelAlignment = mScrollPosAtLastPaint;
      }
    } else {
      mScrollPosForLayerPixelAlignment = nsPoint(-1,-1);
    }
  }

  if (aBuilder->GetIgnoreScrollFrame() == mOuter || IsIgnoringViewportClipping()) {
    
    
    
    mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame,
                                     aDirtyRect, aLists);
    return;
  }

  
  
  
  
  
  
  
  
  bool createLayersForScrollbars = mIsRoot &&
    mOuter->PresContext()->IsRootContentDocument();

  
  
  
  
  
  
  
  AppendScrollPartsTo(aBuilder, aDirtyRect, aLists, createLayersForScrollbars,
                      false);

  
  
  
  nsRect dirtyRect;
  
  
  
  
  
  dirtyRect.IntersectRect(aDirtyRect, mScrollPort);

  
  nsRect displayPort;
  bool usingDisplayport =
    nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &displayPort) &&
    !aBuilder->IsForEventDelivery();
  if (usingDisplayport) {
    dirtyRect = displayPort;
  }

  if (aBuilder->IsForImageVisibility()) {
    
    
    
    
    dirtyRect = ExpandRect(dirtyRect);
  }

  nsDisplayListCollection scrolledContent;
  {
    DisplayListClipState::AutoSaveRestore clipState(aBuilder);

    if (usingDisplayport) {
      nsRect clip = displayPort + aBuilder->ToReferenceFrame(mOuter);
      if (mClipAllDescendants) {
        clipState.ClipContentDescendants(clip);
      } else {
        clipState.ClipContainingBlockDescendants(clip, nullptr);
      }
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
  }

  
  
  
  
  
  if (usingDisplayport) {
    mShouldBuildLayer = true;
  } else {
    nsRect scrollRange = GetScrollRange();
    ScrollbarStyles styles = GetScrollbarStylesFromFrame();
    bool hasScrollableOverflow =
      (scrollRange.width > 0 || scrollRange.height > 0) &&
      ((styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN && mHScrollbarBox) ||
       (styles.mVertical   != NS_STYLE_OVERFLOW_HIDDEN && mVScrollbarBox));
    
    bool wantSubAPZC = (XRE_GetProcessType() == GeckoProcessType_Content);
    mShouldBuildLayer =
      wantSubAPZC &&
      hasScrollableOverflow &&
      (!mIsRoot || !mOuter->PresContext()->IsRootContentDocument());
  }

  if (ShouldBuildLayer()) {
    
    
    ScrollLayerWrapper wrapper(mOuter, mScrolledFrame);

    if (usingDisplayport) {
      
      
      
      wrapper.WrapListsInPlace(aBuilder, mOuter, scrolledContent);
    }

    
    
    
    nsDisplayScrollInfoLayer* layerItem = new (aBuilder) nsDisplayScrollInfoLayer(
      aBuilder, mScrolledFrame, mOuter);
    scrolledContent.BorderBackground()->AppendNewToBottom(layerItem);
  }
  scrolledContent.MoveTo(aLists);

  
  AppendScrollPartsTo(aBuilder, aDirtyRect, aLists, createLayersForScrollbars,
                      true);
}

bool
nsGfxScrollFrameInner::IsRectNearlyVisible(const nsRect& aRect) const
{
  
  nsRect displayPort;
  bool usingDisplayport = nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &displayPort);
  return aRect.Intersects(ExpandRect(usingDisplayport ? displayPort : mScrollPort));
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
nsGfxScrollFrameInner::GetScrollbarStylesFromFrame() const
{
  nsPresContext* presContext = mOuter->PresContext();
  if (!presContext->IsDynamic() &&
      !(mIsRoot && presContext->HasPaginatedScrolling())) {
    return ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, NS_STYLE_OVERFLOW_HIDDEN);
  }

  if (!mIsRoot) {
    const nsStyleDisplay* disp = mOuter->StyleDisplay();
    return ScrollbarStyles(disp->mOverflowX, disp->mOverflowY);
  }

  ScrollbarStyles result = presContext->GetViewportOverflowOverride();
  nsCOMPtr<nsISupports> container = presContext->GetContainer();
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
nsGfxScrollFrameInner::GetScrollRange() const
{
  return GetScrollRange(mScrollPort.width, mScrollPort.height);
}

nsRect
nsGfxScrollFrameInner::GetScrollRange(nscoord aWidth, nscoord aHeight) const
{
  nsRect range = GetScrolledRect();
  range.width = std::max(range.width - aWidth, 0);
  range.height = std::max(range.height - aHeight, 0);
  return range;
}

nsRect
nsGfxScrollFrameInner::GetScrollRangeForClamping() const
{
  if (!ShouldClampScrollPosition()) {
    return nsRect(nscoord_MIN/2, nscoord_MIN/2,
                  nscoord_MAX - nscoord_MIN/2, nscoord_MAX - nscoord_MIN/2);
  }
  nsSize scrollPortSize = GetScrollPositionClampingScrollPortSize();
  return GetScrollRange(scrollPortSize.width, scrollPortSize.height);
}

nsSize
nsGfxScrollFrameInner::GetScrollPositionClampingScrollPortSize() const
{
  nsIPresShell* presShell = mOuter->PresContext()->PresShell();
  if (mIsRoot && presShell->IsScrollPositionClampingScrollPortSizeSet()) {
    return presShell->GetScrollPositionClampingScrollPortSize();
  }
  return mScrollPort.Size();
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
nsGfxScrollFrameInner::ScrollBy(nsIntPoint aDelta,
                                nsIScrollableFrame::ScrollUnit aUnit,
                                nsIScrollableFrame::ScrollMode aMode,
                                nsIntPoint* aOverflow,
                                nsIAtom *aOrigin)
{
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

  nsPoint newPos = mDestination +
    nsPoint(aDelta.x*deltaMultiplier.width, aDelta.y*deltaMultiplier.height);
  
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
}

nsSize
nsGfxScrollFrameInner::GetLineScrollAmount() const
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
nsGfxScrollFrameInner::GetPageScrollAmount() const
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
nsGfxScrollFrameInner::ScrollToRestoredPosition()
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
nsGfxScrollFrameInner::FireScrollPortEvent()
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
  return nsEventDispatcher::Dispatch(mOuter->GetContent(),
                                     mOuter->PresContext(), &event);
}

void
nsGfxScrollFrameInner::ReloadChildFrames()
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
      } else if (content->Tag() == nsGkAtoms::resizer) {
        NS_ASSERTION(!mResizerBox, "Found multiple resizers");
        mResizerBox = frame;
      } else if (content->Tag() == nsGkAtoms::scrollcorner) {
        
        NS_ASSERTION(!mScrollCornerBox, "Found multiple scrollcorners");
        mScrollCornerBox = frame;
      }
    }

    frame = frame->GetNextSibling();
  }
}

nsresult
nsGfxScrollFrameInner::CreateAnonymousContent(
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
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollbar, nullptr,
                                          kNameSpaceID_XUL,
                                          nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  if (canHaveHorizontal) {
    nsCOMPtr<nsINodeInfo> ni = nodeInfo;
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
    nsCOMPtr<nsINodeInfo> ni = nodeInfo;
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
    nsCOMPtr<nsINodeInfo> nodeInfo;
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
nsGfxScrollFrameInner::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                                uint32_t aFilter)
{
  aElements.MaybeAppendElement(mHScrollbarContent);
  aElements.MaybeAppendElement(mVScrollbarContent);
  aElements.MaybeAppendElement(mScrollCornerContent);
  aElements.MaybeAppendElement(mResizerContent);
}

void
nsGfxScrollFrameInner::Destroy()
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
nsGfxScrollFrameInner::UpdateScrollbarPosition()
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

void nsGfxScrollFrameInner::CurPosAttributeChanged(nsIContent* aContent)
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
nsGfxScrollFrameInner::ScrollEvent::Run()
{
  if (mInner)
    mInner->FireScrollEvent();
  return NS_OK;
}

void
nsGfxScrollFrameInner::FireScrollEvent()
{
  mScrollEvent.Forget();

  WidgetGUIEvent event(true, NS_SCROLL_EVENT, nullptr);
  nsEventStatus status = nsEventStatus_eIgnore;
  nsIContent* content = mOuter->GetContent();
  nsPresContext* prescontext = mOuter->PresContext();
  
  
  if (mIsRoot) {
    nsIDocument* doc = content->GetCurrentDoc();
    if (doc) {
      nsEventDispatcher::Dispatch(doc, prescontext, &event, nullptr,  &status);
    }
  } else {
    
    
    event.mFlags.mBubbles = false;
    nsEventDispatcher::Dispatch(content, prescontext, &event, nullptr, &status);
  }
}

void
nsGfxScrollFrameInner::PostScrollEvent()
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
nsGfxScrollFrameInner::AsyncScrollPortEvent::Run()
{
  if (mInner) {
    mInner->mOuter->PresContext()->GetPresShell()->
      FlushPendingNotifications(Flush_InterruptibleLayout);
  }
  return mInner ? mInner->FireScrollPortEvent() : NS_OK;
}

bool
nsXULScrollFrame::AddHorizontalScrollbar(nsBoxLayoutState& aState, bool aOnBottom)
{
  if (!mInner.mHScrollbarBox)
    return true;

  return AddRemoveScrollbar(aState, aOnBottom, true, true);
}

bool
nsXULScrollFrame::AddVerticalScrollbar(nsBoxLayoutState& aState, bool aOnRight)
{
  if (!mInner.mVScrollbarBox)
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
     if (mInner.mNeverHasHorizontalScrollbar || !mInner.mHScrollbarBox)
       return false;

     nsSize hSize = mInner.mHScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mInner.mHScrollbarBox, hSize);

     mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, aAdd);

     bool hasHorizontalScrollbar;
     bool fit = AddRemoveScrollbar(hasHorizontalScrollbar,
                                     mInner.mScrollPort.y,
                                     mInner.mScrollPort.height,
                                     hSize.height, aOnRightOrBottom, aAdd);
     mInner.mHasHorizontalScrollbar = hasHorizontalScrollbar;    
     if (!fit)
        mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, !aAdd);

     return fit;
  } else {
     if (mInner.mNeverHasVerticalScrollbar || !mInner.mVScrollbarBox)
       return false;

     nsSize vSize = mInner.mVScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mInner.mVScrollbarBox, vSize);

     mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, aAdd);

     bool hasVerticalScrollbar;
     bool fit = AddRemoveScrollbar(hasVerticalScrollbar,
                                     mInner.mScrollPort.x,
                                     mInner.mScrollPort.width,
                                     vSize.width, aOnRightOrBottom, aAdd);
     mInner.mHasVerticalScrollbar = hasVerticalScrollbar;    
     if (!fit)
        mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, !aAdd);

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
  nsRect childRect = nsRect(mInner.mScrollPort.TopLeft() - aScrollPosition,
                            mInner.mScrollPort.Size());
  int32_t flags = NS_FRAME_NO_MOVE_VIEW;

  nsSize minSize = mInner.mScrolledFrame->GetMinSize(aState);

  if (minSize.height > childRect.height)
    childRect.height = minSize.height;

  if (minSize.width > childRect.width)
    childRect.width = minSize.width;

  aState.SetLayoutFlags(flags);
  ClampAndSetBounds(aState, childRect, aScrollPosition);
  mInner.mScrolledFrame->Layout(aState);

  childRect = mInner.mScrolledFrame->GetRect();

  if (childRect.width < mInner.mScrollPort.width ||
      childRect.height < mInner.mScrollPort.height)
  {
    childRect.width = std::max(childRect.width, mInner.mScrollPort.width);
    childRect.height = std::max(childRect.height, mInner.mScrollPort.height);

    
    
    
    ClampAndSetBounds(aState, childRect, aScrollPosition, true);
  }

  aState.SetLayoutFlags(oldflags);

}

void nsGfxScrollFrameInner::PostOverflowEvent()
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
nsGfxScrollFrameInner::IsLTR() const
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

  return frame->StyleVisibility()->mDirection != NS_STYLE_DIRECTION_RTL;
}

bool
nsGfxScrollFrameInner::IsScrollbarOnRight() const
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





nsresult
nsXULScrollFrame::Layout(nsBoxLayoutState& aState)
{
  bool scrollbarRight = mInner.IsScrollbarOnRight();
  bool scrollbarBottom = true;

  
  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);

  nsRect oldScrollAreaBounds = mInner.mScrollPort;
  nsPoint oldScrollPosition = mInner.GetLogicalScrollPosition();

  
  mInner.mScrollPort = clientRect;

  
























  ScrollbarStyles styles = GetScrollbarStyles();

  
  if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasHorizontalScrollbar = true;
  if (styles.mVertical == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasVerticalScrollbar = true;

  if (mInner.mHasHorizontalScrollbar)
     AddHorizontalScrollbar(aState, scrollbarBottom);

  if (mInner.mHasVerticalScrollbar)
     AddVerticalScrollbar(aState, scrollbarRight);

  
  LayoutScrollArea(aState, oldScrollPosition);

  
  bool needsLayout = false;

  
  if (styles.mVertical != NS_STYLE_OVERFLOW_SCROLL) {
    
    nsRect scrolledRect = mInner.GetScrolledRect();

    
      if (scrolledRect.height <= mInner.mScrollPort.height
          || styles.mVertical != NS_STYLE_OVERFLOW_AUTO) {
        if (mInner.mHasVerticalScrollbar) {
          
          
          RemoveVerticalScrollbar(aState, scrollbarRight);
          needsLayout = true;
        }
      } else {
        if (!mInner.mHasVerticalScrollbar) {
          
          
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
    
    nsRect scrolledRect = mInner.GetScrolledRect();

    
    
    if ((scrolledRect.width > mInner.mScrollPort.width)
        && styles.mHorizontal == NS_STYLE_OVERFLOW_AUTO) {

      if (!mInner.mHasHorizontalScrollbar) {
           
          if (AddHorizontalScrollbar(aState, scrollbarBottom))
             needsLayout = true;

           
           
           
           
           
           

      }
    } else {
        
        
      if (mInner.mHasHorizontalScrollbar) {
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
  if (mInner.mHScrollbarBox && mInner.mHasHorizontalScrollbar) {
    GetScrollbarMetrics(aState, mInner.mHScrollbarBox, &hMinSize, nullptr, false);
  }
  nsSize vMinSize(0, 0);
  if (mInner.mVScrollbarBox && mInner.mHasVerticalScrollbar) {
    GetScrollbarMetrics(aState, mInner.mVScrollbarBox, &vMinSize, nullptr, true);
  }

  
  
  
  
  
  if (mInner.mHasHorizontalScrollbar &&
      (hMinSize.width > clientRect.width - vMinSize.width
       || hMinSize.height > clientRect.height)) {
    RemoveHorizontalScrollbar(aState, scrollbarBottom);
    needsLayout = true;
  }
  
  if (mInner.mHasVerticalScrollbar &&
      (vMinSize.height > clientRect.height - hMinSize.height
       || vMinSize.width > clientRect.width)) {
    RemoveVerticalScrollbar(aState, scrollbarRight);
    needsLayout = true;
  }

  
  if (needsLayout) {
    nsBoxLayoutState resizeState(aState);
    LayoutScrollArea(resizeState, oldScrollPosition);
  }

  if (!mInner.mSupppressScrollbarUpdate) {
    mInner.LayoutScrollbars(aState, clientRect, oldScrollAreaBounds);
  }
  if (!mInner.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mInner);
    mInner.mPostedReflowCallback = true;
  }
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    mInner.mHadNonInitialReflow = true;
  }

  mInner.UpdateSticky();

  
  
  nsIFrame* f = mInner.mScrolledFrame->GetContentInsertionFrame();
  if (nsLayoutUtils::GetAsBlock(f)) {
    nsRect origRect = f->GetRect();
    nsRect clippedRect = origRect;
    clippedRect.MoveBy(mInner.mScrollPort.TopLeft());
    clippedRect.IntersectRect(clippedRect, mInner.mScrollPort);
    nsOverflowAreas overflow = f->GetOverflowAreas();
    f->FinishAndStoreOverflow(overflow, clippedRect.Size());
    clippedRect.MoveTo(origRect.TopLeft());
    f->SetRect(clippedRect);
  }

  mInner.PostOverflowEvent();
  return NS_OK;
}

void
nsGfxScrollFrameInner::FinishReflowForScrollbar(nsIContent* aContent,
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
nsGfxScrollFrameInner::ReflowFinished()
{
  nsAutoScriptBlocker scriptBlocker;
  mPostedReflowCallback = false;

  ScrollToRestoredPosition();

  
  
  nsPoint currentScrollPos = GetScrollPosition();
  ScrollToImpl(currentScrollPos, nsRect(currentScrollPos, nsSize(0, 0)));
  if (!mAsyncScroll) {
    
    
    
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
  nscoord minX = scrolledContentRect.x;
  nscoord maxX = scrolledContentRect.XMost() - mScrollPort.width;
  nscoord minY = scrolledContentRect.y;
  nscoord maxY = scrolledContentRect.YMost() - mScrollPort.height;

  
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
      
      
      
      
      
      
      
      nscoord pageincrement = nscoord(mScrollPort.height - increment);
      nscoord pageincrementMin = nscoord(float(mScrollPort.height) * 0.8);
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
                               nscoord(float(mScrollPort.width) * 0.8),
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
nsGfxScrollFrameInner::ReflowCallbackCanceled()
{
  mPostedReflowCallback = false;
}

bool
nsGfxScrollFrameInner::UpdateOverflow()
{
  nsIScrollableFrame* sf = do_QueryFrame(mOuter);
  ScrollbarStyles ss = sf->GetScrollbarStyles();

  if (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN ||
      ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN ||
      GetScrollPosition() != nsPoint()) {
    
    
    
    
    
    mOuter->PresContext()->PresShell()->FrameNeedsReflow(
      mOuter, nsIPresShell::eResize, NS_FRAME_HAS_DIRTY_CHILDREN);
    
    
    
    mSkippedScrollbarLayout = true;
    return false;  
  }
  return mOuter->nsContainerFrame::UpdateOverflow();
}

void
nsGfxScrollFrameInner::UpdateSticky()
{
  StickyScrollContainer* ssc = StickyScrollContainer::
    GetStickyScrollContainerForScrollFrame(mOuter);
  if (ssc) {
    nsIScrollableFrame* scrollFrame = do_QueryFrame(mOuter);
    ssc->UpdatePositions(scrollFrame->GetScrollPosition(), mOuter);
  }
}

void
nsGfxScrollFrameInner::AdjustScrollbarRectForResizer(
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

  if (!resizerRect.Contains(aRect.BottomRight() - nsPoint(1, 1)))
    return;

  if (aVertical)
    aRect.height = std::max(0, resizerRect.y - aRect.y);
  else
    aRect.width = std::max(0, resizerRect.x - aRect.x);
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
nsGfxScrollFrameInner::LayoutScrollbars(nsBoxLayoutState& aState,
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
nsGfxScrollFrameInner::SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos)
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
nsGfxScrollFrameInner::SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom,
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
nsGfxScrollFrameInner::GetBorderRadii(nscoord aRadii[8]) const
{
  if (!mOuter->nsContainerFrame::GetBorderRadii(aRadii))
    return false;

  
  
  
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
nsGfxScrollFrameInner::GetScrolledRect() const
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
nsGfxScrollFrameInner::GetScrolledRectInternal(const nsRect& aScrolledFrameOverflowArea,
                                               const nsSize& aScrollPortSize) const
{
  return nsLayoutUtils::GetScrolledRect(mScrolledFrame,
      aScrolledFrameOverflowArea, aScrollPortSize,
      IsLTR() ? NS_STYLE_DIRECTION_LTR : NS_STYLE_DIRECTION_RTL);
}

nsMargin
nsGfxScrollFrameInner::GetActualScrollbarSizes() const
{
  nsRect r = mOuter->GetPaddingRect() - mOuter->GetPosition();

  return nsMargin(mScrollPort.y - r.y,
                  r.XMost() - mScrollPort.XMost(),
                  r.YMost() - mScrollPort.YMost(),
                  mScrollPort.x - r.x);
}

void
nsGfxScrollFrameInner::SetScrollbarVisibility(nsIFrame* aScrollbar, bool aVisible)
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
nsGfxScrollFrameInner::GetCoordAttribute(nsIFrame* aBox, nsIAtom* aAtom,
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
nsGfxScrollFrameInner::SaveState()
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
  return state;
}

void
nsGfxScrollFrameInner::RestoreState(nsPresState* aState)
{
  mRestorePos = aState->GetScrollState();
  mDidHistoryRestore = true;
  mLastPos = mScrolledFrame ? GetLogicalScrollPosition() : nsPoint(0,0);
}

void
nsGfxScrollFrameInner::PostScrolledAreaEvent()
{
  if (mScrolledAreaEvent.IsPending()) {
    return;
  }
  mScrolledAreaEvent = new ScrolledAreaEvent(this);
  nsContentUtils::AddScriptRunner(mScrolledAreaEvent.get());
}




NS_IMETHODIMP
nsGfxScrollFrameInner::ScrolledAreaEvent::Run()
{
  if (mInner) {
    mInner->FireScrolledAreaEvent();
  }
  return NS_OK;
}

void
nsGfxScrollFrameInner::FireScrolledAreaEvent()
{
  mScrolledAreaEvent.Forget();

  InternalScrollAreaEvent event(true, NS_SCROLLEDAREACHANGED, nullptr);
  nsPresContext *prescontext = mOuter->PresContext();
  nsIContent* content = mOuter->GetContent();

  event.mArea = mScrolledFrame->GetScrollableOverflowRectRelativeToParent();

  nsIDocument *doc = content->GetCurrentDoc();
  if (doc) {
    nsEventDispatcher::Dispatch(doc, prescontext, &event, nullptr);
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
