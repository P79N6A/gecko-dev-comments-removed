








































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"
#include "nsIServiceManager.h"
#include "nsIView.h"
#include "nsIScrollableView.h"
#include "nsIScrollable.h"
#include "nsIViewManager.h"
#include "nsHTMLContainerFrame.h"
#include "nsGfxScrollFrame.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIFontMetrics.h"
#include "nsIDocumentObserver.h"
#include "nsIDocument.h"
#include "nsBoxLayoutState.h"
#include "nsINodeInfo.h"
#include "nsIScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsITextControlFrame.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsNodeInfoManager.h"
#include "nsIURI.h"
#include "nsGUIEvent.h"
#include "nsContentCreatorFunctions.h"
#include "nsISupportsPrimitives.h"
#include "nsAutoPtr.h"
#include "nsPresState.h"
#include "nsIGlobalHistory3.h"
#include "nsDocShellCID.h"
#include "nsIHTMLDocument.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsDisplayList.h"
#include "nsBidiUtils.h"
#include "nsFrameManager.h"





nsIFrame*
NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot)
{
  return new (aPresShell) nsHTMLScrollFrame(aPresShell, aContext, aIsRoot);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLScrollFrame)

nsHTMLScrollFrame::nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot)
  : nsHTMLContainerFrame(aContext),
    mInner(this, aIsRoot, PR_FALSE)
{
}





nsIFrame* nsHTMLScrollFrame::GetScrolledFrame() const
{
  return mInner.GetScrolledFrame();
}

nsIScrollableView* nsHTMLScrollFrame::GetScrollableView()
{
  return mInner.GetScrollableView();
}

nsPoint nsHTMLScrollFrame::GetScrollPosition() const
{
   nsIScrollableView* s = mInner.GetScrollableView();
   nsPoint scrollPosition;
   s->GetScrollPosition(scrollPosition.x, scrollPosition.y);
   return scrollPosition;
}

void nsHTMLScrollFrame::ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags)
{
   nsIScrollableView* s = mInner.GetScrollableView();
   s->ScrollTo(aScrollPosition.x, aScrollPosition.y, aFlags);
}

nsGfxScrollFrameInner::ScrollbarStyles
nsHTMLScrollFrame::GetScrollbarStyles() const {
  return mInner.GetScrollbarStylesFromFrame();
}

nsMargin nsHTMLScrollFrame::GetDesiredScrollbarSizes(nsBoxLayoutState* aState) {
  return mInner.GetDesiredScrollbarSizes(aState);
}

void nsHTMLScrollFrame::SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible)
{
  mInner.mNeverHasVerticalScrollbar = !aVerticalVisible;
  mInner.mNeverHasHorizontalScrollbar = !aHorizontalVisible;
}

nsIBox* nsHTMLScrollFrame::GetScrollbarBox(PRBool aVertical)
{
  return aVertical ? mInner.mVScrollbarBox : mInner.mHScrollbarBox;
}

nsresult
nsHTMLScrollFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsHTMLScrollFrame::Destroy()
{
  mInner.Destroy();
  nsHTMLContainerFrame::Destroy();
}

NS_IMETHODIMP
nsHTMLScrollFrame::SetInitialChildList(nsIAtom*     aListName,
                                       nsFrameList& aChildList)
{
  nsresult rv = nsHTMLContainerFrame::SetInitialChildList(aListName, aChildList);
  mInner.CreateScrollableView();
  mInner.ReloadChildFrames();

  
  mInner.GetScrollableView()->AddScrollPositionListener(&mInner);

  return rv;
}


NS_IMETHODIMP
nsHTMLScrollFrame::AppendFrames(nsIAtom*  aListName,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(!aListName, "Only main list supported");
  mFrames.AppendFrames(nsnull, aFrameList);
  mInner.ReloadChildFrames();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScrollFrame::InsertFrames(nsIAtom*  aListName,
                                nsIFrame* aPrevFrame,
                                nsFrameList& aFrameList)
{
  NS_ASSERTION(!aListName, "Only main list supported");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");
  mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
  mInner.ReloadChildFrames();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLScrollFrame::RemoveFrame(nsIAtom*  aListName,
                               nsIFrame* aOldFrame)
{
  NS_ASSERTION(!aListName, "Only main list supported");
  mFrames.DestroyFrame(aOldFrame);
  mInner.ReloadChildFrames();
  return NS_OK;
}

nsSplittableType
nsHTMLScrollFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

PRIntn
nsHTMLScrollFrame::GetSkipSides() const
{
  return 0;
}

nsIAtom*
nsHTMLScrollFrame::GetType() const
{
  return nsGkAtoms::scrollFrame; 
}

void
nsHTMLScrollFrame::InvalidateInternal(const nsRect& aDamageRect,
                                      nscoord aX, nscoord aY, nsIFrame* aForChild,
                                      PRUint32 aFlags)
{
  if (aForChild) {
    if (aForChild == mInner.mScrolledFrame) {
      
      nsRect damage = aDamageRect + nsPoint(aX, aY);
      nsRect r;
      if (r.IntersectRect(damage, mInner.mScrollableView->View()->GetBounds())) {
        nsHTMLContainerFrame::InvalidateInternal(r, 0, 0, aForChild, aFlags);
      }
      if (mInner.mIsRoot && r != damage) {
        
        
        
        
        
        
        
        PresContext()->NotifyInvalidation(damage, aFlags);
      }
      return;
    } else if (aForChild == mInner.mHScrollbarBox) {
      if (!mInner.mHasHorizontalScrollbar) {
        
        
        
        return;
      }
    } else if (aForChild == mInner.mVScrollbarBox) {
      if (!mInner.mHasVerticalScrollbar) {
        
        
        
        return;
      }
    }
  }
 
  nsHTMLContainerFrame::InvalidateInternal(aDamageRect, aX, aY, aForChild, aFlags);
}







struct ScrollReflowState {
  const nsHTMLReflowState& mReflowState;
  nsBoxLayoutState mBoxState;
  nsGfxScrollFrameInner::ScrollbarStyles mStyles;
  nsMargin mComputedBorder;

  
  PRPackedBool mReflowedContentsWithHScrollbar;
  PRPackedBool mReflowedContentsWithVScrollbar;

  
  
  nsRect mScrollPortRect;
  
  nsSize mInsideBorderSize;
  
  PRPackedBool mShowHScrollbar;
  
  PRPackedBool mShowVScrollbar;

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

  aState->mReflowState.ApplyMinMaxConstraints(&contentWidth, &contentHeight);
  return nsSize(contentWidth + aState->mReflowState.mComputedPadding.LeftRight(),
                contentHeight + aState->mReflowState.mComputedPadding.TopBottom());
}

static void
GetScrollbarMetrics(nsBoxLayoutState& aState, nsIBox* aBox, nsSize* aMin,
                    nsSize* aPref, PRBool aVertical)
{
  NS_ASSERTION(aState.GetRenderingContext(),
               "Must have rendering context in layout state for size "
               "computations");
  
  if (aMin) {
    *aMin = aBox->GetMinSize(aState);
    nsBox::AddMargin(aBox, *aMin);
  }
 
  if (aPref) {
    *aPref = aBox->GetPrefSize(aState);
    nsBox::AddMargin(aBox, *aPref);
  }
}






















PRBool
nsHTMLScrollFrame::TryLayout(ScrollReflowState* aState,
                             nsHTMLReflowMetrics* aKidMetrics,
                             PRBool aAssumeHScroll, PRBool aAssumeVScroll,
                             PRBool aForce, nsresult* aResult)
{
  *aResult = NS_OK;

  if ((aState->mStyles.mVertical == NS_STYLE_OVERFLOW_HIDDEN && aAssumeVScroll) ||
      (aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN && aAssumeHScroll)) {
    NS_ASSERTION(!aForce, "Shouldn't be forcing a hidden scrollbar to show!");
    return PR_FALSE;
  }

  if (aAssumeVScroll != aState->mReflowedContentsWithVScrollbar ||
      (aAssumeHScroll != aState->mReflowedContentsWithHScrollbar &&
       ScrolledContentDependsOnHeight(aState))) {
    nsresult rv = ReflowScrolledFrame(aState, aAssumeHScroll, aAssumeVScroll,
                                      aKidMetrics, PR_FALSE);
    if (NS_FAILED(rv)) {
      *aResult = rv;
      return PR_FALSE;
    }
  }

  nsSize vScrollbarMinSize(0, 0);
  nsSize vScrollbarPrefSize(0, 0);
  if (mInner.mVScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mInner.mVScrollbarBox,
                        &vScrollbarMinSize,
                        aAssumeVScroll ? &vScrollbarPrefSize : nsnull, PR_TRUE);
  }
  nscoord vScrollbarDesiredWidth = aAssumeVScroll ? vScrollbarPrefSize.width : 0;
  nscoord vScrollbarMinHeight = aAssumeVScroll ? vScrollbarMinSize.height : 0;

  nsSize hScrollbarMinSize(0, 0);
  nsSize hScrollbarPrefSize(0, 0);
  if (mInner.mHScrollbarBox) {
    GetScrollbarMetrics(aState->mBoxState, mInner.mHScrollbarBox,
                        &hScrollbarMinSize,
                        aAssumeHScroll ? &hScrollbarPrefSize : nsnull, PR_FALSE);
  }
  nscoord hScrollbarDesiredHeight = aAssumeHScroll ? hScrollbarPrefSize.height : 0;
  nscoord hScrollbarMinWidth = aAssumeHScroll ? hScrollbarMinSize.width : 0;

  
  
  nsSize desiredInsideBorderSize;
  desiredInsideBorderSize.width = vScrollbarDesiredWidth +
    NS_MAX(aKidMetrics->width, hScrollbarMinWidth);
  desiredInsideBorderSize.height = hScrollbarDesiredHeight +
    NS_MAX(aKidMetrics->height, vScrollbarMinHeight);
  aState->mInsideBorderSize =
    ComputeInsideBorderSize(aState, desiredInsideBorderSize);
  nsSize scrollPortSize = nsSize(NS_MAX(0, aState->mInsideBorderSize.width - vScrollbarDesiredWidth),
                                 NS_MAX(0, aState->mInsideBorderSize.height - hScrollbarDesiredHeight));
                                                                                
  if (!aForce) {
    nsRect scrolledRect = mInner.GetScrolledRect(scrollPortSize);
    nscoord oneDevPixel = aState->mBoxState.PresContext()->DevPixelsToAppUnits(1);

    
    if (aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN) {
      PRBool wantHScrollbar =
        aState->mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.XMost() >= scrollPortSize.width + oneDevPixel ||
        scrolledRect.x <= -oneDevPixel;
      if (aState->mInsideBorderSize.height < hScrollbarMinSize.height ||
          scrollPortSize.width < hScrollbarMinSize.width)
        wantHScrollbar = PR_FALSE;
      if (wantHScrollbar != aAssumeHScroll)
        return PR_FALSE;
    }

    
    if (aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN) {
      PRBool wantVScrollbar =
        aState->mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL ||
        scrolledRect.YMost() >= scrollPortSize.height + oneDevPixel ||
        scrolledRect.y <= -oneDevPixel;
      if (aState->mInsideBorderSize.width < vScrollbarMinSize.width ||
          scrollPortSize.height < vScrollbarMinSize.height)
        wantVScrollbar = PR_FALSE;
      if (wantVScrollbar != aAssumeVScroll)
        return PR_FALSE;
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
  aState->mScrollPortRect = nsRect(scrollPortOrigin, scrollPortSize);
  return PR_TRUE;
}

PRBool
nsHTMLScrollFrame::ScrolledContentDependsOnHeight(ScrollReflowState* aState)
{
  
  
  return (mInner.mScrolledFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT) ||
    aState->mReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE ||
    aState->mReflowState.mComputedMinHeight > 0 ||
    aState->mReflowState.mComputedMaxHeight != NS_UNCONSTRAINEDSIZE;
}

nsresult
nsHTMLScrollFrame::ReflowScrolledFrame(ScrollReflowState* aState,
                                       PRBool aAssumeHScroll,
                                       PRBool aAssumeVScroll,
                                       nsHTMLReflowMetrics* aMetrics,
                                       PRBool aFirstPass)
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
    nsSize hScrollbarPrefSize = 
      mInner.mHScrollbarBox->GetPrefSize(const_cast<nsBoxLayoutState&>(aState->mBoxState));
    if (computedHeight != NS_UNCONSTRAINEDSIZE)
      computedHeight = NS_MAX(0, computedHeight - hScrollbarPrefSize.height);
    computedMinHeight = NS_MAX(0, computedMinHeight - hScrollbarPrefSize.height);
    if (computedMaxHeight != NS_UNCONSTRAINEDSIZE)
      computedMaxHeight = NS_MAX(0, computedMaxHeight - hScrollbarPrefSize.height);
  }

  if (aAssumeVScroll) {
    nsSize vScrollbarPrefSize = 
      mInner.mVScrollbarBox->GetPrefSize(const_cast<nsBoxLayoutState&>(aState->mBoxState));
    availWidth = NS_MAX(0, availWidth - vScrollbarPrefSize.width);
  }

  
  
  mInner.mScrolledFrame->
    SetProperty(nsGkAtoms::usedPaddingProperty,
                new nsMargin(aState->mReflowState.mComputedPadding),
                nsCSSOffsetState::DestroyMarginFunc);  
  
  nsPresContext* presContext = PresContext();
  
  nsHTMLReflowState kidReflowState(presContext, aState->mReflowState,
                                   mInner.mScrolledFrame,
                                   nsSize(availWidth, NS_UNCONSTRAINEDSIZE),
                                   -1, -1, PR_FALSE);
  kidReflowState.Init(presContext, -1, -1, nsnull,
                      &aState->mReflowState.mComputedPadding);
  kidReflowState.mFlags.mAssumingHScrollbar = aAssumeHScroll;
  kidReflowState.mFlags.mAssumingVScrollbar = aAssumeVScroll;
  kidReflowState.SetComputedHeight(computedHeight);
  kidReflowState.mComputedMinHeight = computedMinHeight;
  kidReflowState.mComputedMaxHeight = computedMaxHeight;

  nsReflowStatus status;
  nsresult rv = ReflowChild(mInner.mScrolledFrame, presContext, *aMetrics,
                            kidReflowState, 0, 0,
                            NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW, status);
  
  
  
  
  
  FinishReflowChild(mInner.mScrolledFrame, presContext,
                    &kidReflowState, *aMetrics, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_SIZE_VIEW);

  
  
  
  
  
  
  
  aMetrics->mOverflowArea.UnionRect(aMetrics->mOverflowArea,
                                    nsRect(0, 0, aMetrics->width, aMetrics->height));

  aState->mReflowedContentsWithHScrollbar = aAssumeHScroll;
  aState->mReflowedContentsWithVScrollbar = aAssumeVScroll;
  
  return rv;
}

PRBool
nsHTMLScrollFrame::GuessHScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mHorizontal != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL;

  return mInner.mHasHorizontalScrollbar;
}

PRBool
nsHTMLScrollFrame::GuessVScrollbarNeeded(const ScrollReflowState& aState)
{
  if (aState.mStyles.mVertical != NS_STYLE_OVERFLOW_AUTO)
    
    return aState.mStyles.mVertical == NS_STYLE_OVERFLOW_SCROLL;

  
  
  
  if (mInner.mHadNonInitialReflow) {
    return mInner.mHasVerticalScrollbar;
  }

  
  
  if (InInitialReflow())
    return PR_FALSE;

  if (mInner.mIsRoot) {
    
    
    PRBool hint;
    nsresult rv = mInner.GetVScrollbarHintFromGlobalHistory(&hint);
    if (NS_SUCCEEDED(rv))
      return hint;
    
    
    
    
    return PR_TRUE;
  }

  
  
  
  
  return PR_FALSE;
}

PRBool
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
      GuessVScrollbarNeeded(*aState), &kidDesiredSize, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  
  
  

  
  
  

  
  
  if ((aState->mReflowedContentsWithHScrollbar || aState->mReflowedContentsWithVScrollbar) &&
      aState->mStyles.mVertical != NS_STYLE_OVERFLOW_SCROLL &&
      aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL) {
    nsSize insideBorderSize =
      ComputeInsideBorderSize(aState,
                              nsSize(kidDesiredSize.width, kidDesiredSize.height));
    nsRect scrolledRect = mInner.GetScrolledRect(insideBorderSize);
    if (nsRect(nsPoint(0, 0), insideBorderSize).Contains(scrolledRect)) {
      
      rv = ReflowScrolledFrame(aState, PR_FALSE, PR_FALSE,
                               &kidDesiredSize, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  
  

  
  
  if (TryLayout(aState, &kidDesiredSize, aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, PR_FALSE, &rv))
    return NS_OK;
  if (TryLayout(aState, &kidDesiredSize, !aState->mReflowedContentsWithHScrollbar,
                aState->mReflowedContentsWithVScrollbar, PR_FALSE, &rv))
    return NS_OK;

  
  
  
  
  PRBool newVScrollbarState = !aState->mReflowedContentsWithVScrollbar;
  if (TryLayout(aState, &kidDesiredSize, PR_FALSE, newVScrollbarState, PR_FALSE, &rv))
    return NS_OK;
  if (TryLayout(aState, &kidDesiredSize, PR_TRUE, newVScrollbarState, PR_FALSE, &rv))
    return NS_OK;

  
  
  
  TryLayout(aState, &kidDesiredSize,
            aState->mStyles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN,
            aState->mStyles.mVertical != NS_STYLE_OVERFLOW_HIDDEN,
            PR_TRUE, &rv);
  return rv;
}

void
nsHTMLScrollFrame::PlaceScrollArea(const ScrollReflowState& aState)
{
  nsIView* scrollView = mInner.mScrollableView->View();
  nsIViewManager* vm = scrollView->GetViewManager();
  vm->MoveViewTo(scrollView, aState.mScrollPortRect.x, aState.mScrollPortRect.y);
  vm->ResizeView(scrollView, nsRect(nsPoint(0, 0), aState.mScrollPortRect.Size()),
                 PR_TRUE);

  nsIFrame *scrolledFrame = mInner.mScrolledFrame;
  nsIView *scrolledView = scrolledFrame->GetView();
  
  
  scrolledFrame->SetPosition(scrolledView->GetOffsetTo(GetView()));

  nsRect scrolledArea;
  
  scrolledArea.UnionRectIncludeEmpty(mInner.GetScrolledRect(aState.mScrollPortRect.Size()),
                                     nsRect(nsPoint(0,0), aState.mScrollPortRect.Size()));

  
  
  
  
  
  
  
  
  
  scrolledFrame->FinishAndStoreOverflow(&scrolledArea,
                                        scrolledFrame->GetSize());

  
  
  
  nsContainerFrame::SyncFrameViewAfterReflow(scrolledFrame->PresContext(),
                                             scrolledFrame,
                                             scrolledView,
                                             &scrolledArea,
                                             NS_FRAME_NO_MOVE_VIEW);
}

nscoord
nsHTMLScrollFrame::GetIntrinsicVScrollbarWidth(nsIRenderingContext *aRenderingContext)
{
  nsGfxScrollFrameInner::ScrollbarStyles ss = GetScrollbarStyles();
  if (ss.mVertical != NS_STYLE_OVERFLOW_SCROLL || !mInner.mVScrollbarBox)
    return 0;

  
  
  nsBoxLayoutState bls(PresContext(), aRenderingContext, 0);
  nsSize vScrollbarPrefSize(0, 0);
  GetScrollbarMetrics(bls, mInner.mVScrollbarBox,
                      nsnull, &vScrollbarPrefSize, PR_TRUE);
  return vScrollbarPrefSize.width;
}

 nscoord
nsHTMLScrollFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result = mInner.mScrolledFrame->GetMinWidth(aRenderingContext);
  DISPLAY_MIN_WIDTH(this, result);
  return result + GetIntrinsicVScrollbarWidth(aRenderingContext);
}

 nscoord
nsHTMLScrollFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
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

PRBool
nsHTMLScrollFrame::IsCollapsed(nsBoxLayoutState& aBoxLayoutState)
{
  
  return PR_FALSE;
}

NS_IMETHODIMP
nsHTMLScrollFrame::Reflow(nsPresContext*           aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsHTMLScrollFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  ScrollReflowState state(this, aReflowState);
  
  
  if (!mInner.mVScrollbarBox || mInner.mNeverHasVerticalScrollbar)
    state.mStyles.mVertical = NS_STYLE_OVERFLOW_HIDDEN;
  if (!mInner.mHScrollbarBox || mInner.mNeverHasHorizontalScrollbar)
    state.mStyles.mHorizontal = NS_STYLE_OVERFLOW_HIDDEN;

  
  PRBool reflowContents = PR_TRUE; 
  PRBool reflowHScrollbar = PR_TRUE;
  PRBool reflowVScrollbar = PR_TRUE;
  PRBool reflowScrollCorner = PR_TRUE;
  if (!aReflowState.ShouldReflowAllKids()) {
    #define NEEDS_REFLOW(frame_) \
      ((frame_) && NS_SUBTREE_DIRTY(frame_))

    reflowContents = NEEDS_REFLOW(mInner.mScrolledFrame);
    reflowHScrollbar = NEEDS_REFLOW(mInner.mHScrollbarBox);
    reflowVScrollbar = NEEDS_REFLOW(mInner.mVScrollbarBox);
    reflowScrollCorner = NEEDS_REFLOW(mInner.mScrollCornerBox);

    #undef NEEDS_REFLOW
  }

  nsRect oldScrollAreaBounds = mInner.mScrollableView->View()->GetBounds();
  nsRect oldScrolledAreaBounds = mInner.mScrolledFrame->GetView()->GetBounds();
  state.mComputedBorder = aReflowState.mComputedBorderPadding -
    aReflowState.mComputedPadding;

  nsresult rv = ReflowContents(&state, aDesiredSize);
  if (NS_FAILED(rv))
    return rv;
  
  PlaceScrollArea(state);
  if (!mInner.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mInner);
    mInner.mPostedReflowCallback = PR_TRUE;
  }

  PRBool didHaveHScrollbar = mInner.mHasHorizontalScrollbar;
  PRBool didHaveVScrollbar = mInner.mHasVerticalScrollbar;
  mInner.mHasHorizontalScrollbar = state.mShowHScrollbar;
  mInner.mHasVerticalScrollbar = state.mShowVScrollbar;
  nsRect newScrollAreaBounds = mInner.mScrollableView->View()->GetBounds();
  nsRect newScrolledAreaBounds = mInner.mScrolledFrame->GetView()->GetBounds();
  if (mInner.mSkippedScrollbarLayout ||
      reflowHScrollbar || reflowVScrollbar || reflowScrollCorner ||
      (GetStateBits() & NS_FRAME_IS_DIRTY) ||
      didHaveHScrollbar != state.mShowHScrollbar ||
      didHaveVScrollbar != state.mShowVScrollbar ||
      oldScrollAreaBounds != newScrollAreaBounds ||
      oldScrolledAreaBounds != newScrolledAreaBounds) {
    if (!mInner.mSupppressScrollbarUpdate) {
      mInner.mSkippedScrollbarLayout = PR_FALSE;
      mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, state.mShowHScrollbar);
      mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, state.mShowVScrollbar);
      
      nsRect insideBorderArea =
        nsRect(nsPoint(state.mComputedBorder.left, state.mComputedBorder.top),
               state.mInsideBorderSize);
      mInner.LayoutScrollbars(state.mBoxState, insideBorderArea,
                              oldScrollAreaBounds, state.mScrollPortRect);
    } else {
      mInner.mSkippedScrollbarLayout = PR_TRUE;
    }
  }

  aDesiredSize.width = state.mInsideBorderSize.width +
    state.mComputedBorder.LeftRight();
  aDesiredSize.height = state.mInsideBorderSize.height +
    state.mComputedBorder.TopBottom();

  aDesiredSize.mOverflowArea = nsRect(0, 0, aDesiredSize.width, aDesiredSize.height);

  CheckInvalidateSizeChange(aDesiredSize);

  FinishAndStoreOverflow(&aDesiredSize);

  if (!InInitialReflow() && !mInner.mHadNonInitialReflow) {
    mInner.mHadNonInitialReflow = PR_TRUE;
    if (mInner.mIsRoot) {
      
      
      mInner.SaveVScrollbarStateToGlobalHistory();
    }
  }

  if (mInner.mIsRoot && oldScrolledAreaBounds != newScrolledAreaBounds) {
    mInner.PostScrolledAreaEvent(newScrolledAreaBounds);
  }

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  mInner.PostOverflowEvent();
  return rv;
}




#ifdef NS_DEBUG
NS_IMETHODIMP
nsHTMLScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("HTMLScroll"), aResult);
}
#endif

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsHTMLScrollFrame::GetAccessible(nsIAccessible** aAccessible)
{
  *aAccessible = nsnull;
  if (!IsFocusable()) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLGenericAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

void
nsHTMLScrollFrame::CurPosAttributeChanged(nsIContent* aChild,
                                          PRInt32 aModType)
{
  mInner.CurPosAttributeChanged(aChild);
}

NS_IMETHODIMP 
nsHTMLScrollFrame::PostScrolledAreaEventForCurrentArea()
{
  nsRect currentScrolledArea = mInner.mScrolledFrame->GetView()->GetBounds();
  mInner.PostScrolledAreaEvent(currentScrolledArea);

  return NS_OK;
}

NS_QUERYFRAME_HEAD(nsHTMLScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollableViewProvider)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)



nsIFrame*
NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot)
{
  return new (aPresShell) nsXULScrollFrame(aPresShell, aContext, aIsRoot);
}

NS_IMPL_FRAMEARENA_HELPERS(nsXULScrollFrame)

nsXULScrollFrame::nsXULScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot)
  : nsBoxFrame(aShell, aContext, aIsRoot),
    mInner(this, aIsRoot, PR_TRUE)
{
    SetLayoutManager(nsnull);
}





nsIFrame* nsXULScrollFrame::GetScrolledFrame() const
{
  return mInner.GetScrolledFrame();
}

nsIScrollableView* nsXULScrollFrame::GetScrollableView()
{
  return mInner.GetScrollableView();
}

nsPoint nsXULScrollFrame::GetScrollPosition() const
{
  nsIScrollableView* s = mInner.GetScrollableView();
  nsPoint scrollPosition;
  s->GetScrollPosition(scrollPosition.x, scrollPosition.y);
  return scrollPosition;
}

void nsXULScrollFrame::ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags)
{
  nsIScrollableView* s = mInner.GetScrollableView();
  s->ScrollTo(aScrollPosition.x, aScrollPosition.y, aFlags);
}

nsGfxScrollFrameInner::ScrollbarStyles
nsXULScrollFrame::GetScrollbarStyles() const {
  return mInner.GetScrollbarStylesFromFrame();
}

nsMargin nsXULScrollFrame::GetDesiredScrollbarSizes(nsBoxLayoutState* aState) {
  return mInner.GetDesiredScrollbarSizes(aState);
}

nsMargin nsGfxScrollFrameInner::GetDesiredScrollbarSizes(nsBoxLayoutState* aState) {
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

void nsXULScrollFrame::SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible)
{
  mInner.mNeverHasVerticalScrollbar = !aVerticalVisible;
  mInner.mNeverHasHorizontalScrollbar = !aHorizontalVisible;
}

nsIBox* nsXULScrollFrame::GetScrollbarBox(PRBool aVertical)
{
  return aVertical ? mInner.mVScrollbarBox : mInner.mHScrollbarBox;
}

nsresult
nsXULScrollFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsXULScrollFrame::Destroy()
{
  mInner.Destroy();
  nsBoxFrame::Destroy();
}

NS_IMETHODIMP
nsXULScrollFrame::SetInitialChildList(nsIAtom*        aListName,
                                      nsFrameList&    aChildList)
{
  nsresult rv = nsBoxFrame::SetInitialChildList(aListName, aChildList);

  mInner.CreateScrollableView();
  mInner.ReloadChildFrames();

  
  mInner.GetScrollableView()->AddScrollPositionListener(&mInner);

  return rv;
}


NS_IMETHODIMP
nsXULScrollFrame::AppendFrames(nsIAtom*        aListName,
                               nsFrameList&    aFrameList)
{
  nsresult rv = nsBoxFrame::AppendFrames(aListName, aFrameList);
  mInner.ReloadChildFrames();
  return rv;
}

NS_IMETHODIMP
nsXULScrollFrame::InsertFrames(nsIAtom*        aListName,
                               nsIFrame*       aPrevFrame,
                               nsFrameList&    aFrameList)
{
  nsresult rv = nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
  mInner.ReloadChildFrames();
  return rv;
}

NS_IMETHODIMP
nsXULScrollFrame::RemoveFrame(nsIAtom*        aListName,
                              nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
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

PRIntn
nsXULScrollFrame::GetSkipSides() const
{
  return 0;
}

nsIAtom*
nsXULScrollFrame::GetType() const
{
  return nsGkAtoms::scrollFrame; 
}

void
nsXULScrollFrame::InvalidateInternal(const nsRect& aDamageRect,
                                     nscoord aX, nscoord aY, nsIFrame* aForChild,
                                     PRUint32 aFlags)
{
  if (aForChild == mInner.mScrolledFrame) {
    
    nsRect r;
    if (r.IntersectRect(aDamageRect + nsPoint(aX, aY),
                        mInner.mScrollableView->View()->GetBounds())) {
      nsBoxFrame::InvalidateInternal(r, 0, 0, aForChild, aFlags);
    }
    return;
  }
  
  nsBoxFrame::InvalidateInternal(aDamageRect, aX, aY, aForChild, aFlags);
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

  nsGfxScrollFrameInner::ScrollbarStyles styles = GetScrollbarStyles();

  

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
  nsIBox::AddCSSPrefSize(aState, this, pref);
  return pref;
}

nsSize
nsXULScrollFrame::GetMinSize(nsBoxLayoutState& aState)
{
#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  nsSize min = mInner.mScrolledFrame->GetMinSizeForScrollArea(aState);

  nsGfxScrollFrameInner::ScrollbarStyles styles = GetScrollbarStyles();
     
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
  nsIBox::AddCSSMinSize(aState, this, min);
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
  nsIBox::AddCSSMaxSize(aState, this, maxSize);
  return maxSize;
}

#if 0 
 nscoord
nsXULScrollFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nsStyleUnit widthUnit = GetStylePosition()->mWidth.GetUnit();
  if (widthUnit == eStyleUnit_Percent || widthUnit == eStyleUnit_Auto) {
    nsMargin border = aReflowState.mComputedBorderPadding;
    aDesiredSize.mMaxElementWidth = border.right + border.left;
    mMaxElementWidth = aDesiredSize.mMaxElementWidth;
  } else {
    NS_NOTYETIMPLEMENTED("Use the info from the scrolled frame");
#if 0
    
    if (aDesiredSize.mMaxElementWidth == -1)
      aDesiredSize.mMaxElementWidth = mMaxElementWidth;
    else
      mMaxElementWidth = aDesiredSize.mMaxElementWidth;
#endif
  }
  return 0;
}
#endif

#ifdef NS_DEBUG
NS_IMETHODIMP
nsXULScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("XULScroll"), aResult);
}
#endif

void nsXULScrollFrame::CurPosAttributeChanged(nsIContent* aChild, PRInt32 aModType)
{
  mInner.CurPosAttributeChanged(aChild);
}

NS_IMETHODIMP
nsXULScrollFrame::DoLayout(nsBoxLayoutState& aState)
{
  PRUint32 flags = aState.LayoutFlags();
  nsresult rv = Layout(aState);
  aState.SetLayoutFlags(flags);

  nsBox::DoLayout(aState);
  return rv;
}

NS_IMETHODIMP 
nsXULScrollFrame::PostScrolledAreaEventForCurrentArea()
{
  nsRect currentScrolledArea = mInner.mScrolledFrame->GetView()->GetBounds();
  mInner.PostScrolledAreaEvent(currentScrolledArea);

  return NS_OK;
}

NS_QUERYFRAME_HEAD(nsXULScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollableViewProvider)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)
 


nsGfxScrollFrameInner::nsGfxScrollFrameInner(nsContainerFrame* aOuter,
                                             PRBool aIsRoot,
                                             PRBool aIsXUL)
  : mScrollableView(nsnull),
    mHScrollbarBox(nsnull),
    mVScrollbarBox(nsnull),
    mScrolledFrame(nsnull),
    mScrollCornerBox(nsnull),
    mOuter(aOuter),
    mRestoreRect(-1, -1, -1, -1),
    mLastPos(-1, -1),
    mNeverHasVerticalScrollbar(PR_FALSE),
    mNeverHasHorizontalScrollbar(PR_FALSE),
    mHasVerticalScrollbar(PR_FALSE), 
    mHasHorizontalScrollbar(PR_FALSE),
    mViewInitiatedScroll(PR_FALSE),
    mFrameInitiatedScroll(PR_FALSE),
    mDidHistoryRestore(PR_FALSE),
    mIsRoot(aIsRoot),
    mIsXUL(aIsXUL),
    mSupppressScrollbarUpdate(PR_FALSE),
    mSkippedScrollbarLayout(PR_FALSE),
    mDidLoadHistoryVScrollbarHint(PR_FALSE),
    mHistoryVScrollbarHint(PR_FALSE),
    mHadNonInitialReflow(PR_FALSE),
    mHorizontalOverflow(PR_FALSE),
    mVerticalOverflow(PR_FALSE),
    mPostedReflowCallback(PR_FALSE),
    mMayHaveDirtyFixedChildren(PR_FALSE),
    mUpdateScrollbarAttributes(PR_FALSE)
{
}

nsGfxScrollFrameInner::~nsGfxScrollFrameInner()
{
}

NS_IMPL_QUERY_INTERFACE1(nsGfxScrollFrameInner, nsIScrollPositionListener)

nsresult
nsGfxScrollFrameInner::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  nsresult rv = mOuter->DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (aBuilder->GetIgnoreScrollFrame() == mOuter) {
    
    
    
    return mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame,
                                            aDirtyRect, aLists);
  }

  
  
  
  
  nsIFrame* kid = mOuter->GetFirstChild(nsnull);
  while (kid) {
    if (kid != mScrolledFrame) {
      rv = mOuter->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    kid = kid->GetNextSibling();
  }

  
  
  
  nsRect frameClip = mScrollableView->View()->GetBounds();
  nsRect dirtyRect;
  
  
  
  
  
  dirtyRect.IntersectRect(aDirtyRect, frameClip);
  
  nsDisplayListCollection set;
  rv = mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame, dirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  nsRect clip = frameClip + aBuilder->ToReferenceFrame(mOuter);
  
  
  
  
  
  rv = mOuter->OverflowClip(aBuilder, set, aLists, clip, PR_TRUE, mIsRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsGfxScrollFrameInner::CreateScrollableView()
{
  nsIView* outerView = mOuter->GetView();
  NS_ASSERTION(outerView, "scrollframes must have views");
  nsIViewManager* viewManager = outerView->GetViewManager();
  mScrollableView = viewManager->CreateScrollableView(mOuter->GetRect(), outerView);
  if (!mScrollableView)
    return;

  nsIView* view = mScrollableView->View();

  
  viewManager->InsertChild(outerView, view, nsnull, PR_TRUE);
}

static void HandleScrollPref(nsIScrollable *aScrollable, PRInt32 aOrientation,
                             PRUint8& aValue)
{
  PRInt32 pref;
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

nsIView*
nsGfxScrollFrameInner::GetParentViewForChildFrame(nsIFrame* aFrame) const
{
  if (aFrame->GetContent() == mOuter->GetContent()) {
    NS_ASSERTION(mScrollableView, "Scrollable view should have been created by now");
    
    return mScrollableView->View();
  }
  
  return mOuter->GetView();
}

nsGfxScrollFrameInner::ScrollbarStyles
nsGfxScrollFrameInner::GetScrollbarStylesFromFrame() const
{
  ScrollbarStyles result;

  nsPresContext* presContext = mOuter->PresContext();
  if (!presContext->IsDynamic() &&
      !(mIsRoot && presContext->HasPaginatedScrolling())) {
    return ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, NS_STYLE_OVERFLOW_HIDDEN);
  }

  if (mIsRoot) {
    result = presContext->GetViewportOverflowOverride();

    nsCOMPtr<nsISupports> container = presContext->GetContainer();
    nsCOMPtr<nsIScrollable> scrollable = do_QueryInterface(container);
    if (scrollable) {
      HandleScrollPref(scrollable, nsIScrollable::ScrollOrientation_X,
                       result.mHorizontal);
      HandleScrollPref(scrollable, nsIScrollable::ScrollOrientation_Y,
                       result.mVertical);
    }
  } else {
    const nsStyleDisplay *disp = mOuter->GetStyleDisplay();
    result.mHorizontal = disp->mOverflowX;
    result.mVertical = disp->mOverflowY;
  }

  NS_ASSERTION(result.mHorizontal != NS_STYLE_OVERFLOW_VISIBLE &&
               result.mHorizontal != NS_STYLE_OVERFLOW_CLIP &&
               result.mVertical != NS_STYLE_OVERFLOW_VISIBLE &&
               result.mVertical != NS_STYLE_OVERFLOW_CLIP,
               "scrollbars should not have been created");
  return result;
}

  






void
nsGfxScrollFrameInner::ScrollToRestoredPosition()
{
  nsIScrollableView* scrollingView = GetScrollableView();
  if (!scrollingView) {
    return;
  }
  if (mRestoreRect.y == -1 || mLastPos.x == -1 || mLastPos.y == -1) {
    return;
  }
  
  
  
  nscoord x = 0;
  nscoord y = 0;
  scrollingView->GetScrollPosition(x, y);

  
  if (x == mLastPos.x && y == mLastPos.y) {
    nsRect childRect(0, 0, 0, 0);
    nsIView* child = nsnull;
    nsresult rv = scrollingView->GetScrolledView(child);
    if (NS_SUCCEEDED(rv) && child)
      childRect = child->GetBounds();

    PRInt32 cx, cy, x, y;
    scrollingView->GetScrollPosition(cx,cy);

    x = (int)mRestoreRect.x;
    y = (int)mRestoreRect.y;

    
    
    
    if (y != cy || x != cx) {
      scrollingView->ScrollTo(x, y, 0);
      
      
      scrollingView->GetScrollPosition(mLastPos.x, mLastPos.y);
    } else {
      
      mRestoreRect.y = -1;
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

  
  nsSize scrollportSize = GetScrollPortSize();
  nsSize childSize = GetScrolledRect(scrollportSize).Size();

  PRBool newVerticalOverflow = childSize.height > scrollportSize.height;
  PRBool vertChanged = mVerticalOverflow != newVerticalOverflow;

  PRBool newHorizontalOverflow = childSize.width > scrollportSize.width;
  PRBool horizChanged = mHorizontalOverflow != newHorizontalOverflow;

  if (!vertChanged && !horizChanged) {
    return NS_OK;
  }

  
  
  PRBool both = vertChanged && horizChanged &&
                newVerticalOverflow == newHorizontalOverflow;
  nsScrollPortEvent::orientType orient;
  if (both) {
    orient = nsScrollPortEvent::both;
    mHorizontalOverflow = newHorizontalOverflow;
    mVerticalOverflow = newVerticalOverflow;
  }
  else if (vertChanged) {
    orient = nsScrollPortEvent::vertical;
    mVerticalOverflow = newVerticalOverflow;
    if (horizChanged) {
      
      
      
      PostOverflowEvent();
    }
  }
  else {
    orient = nsScrollPortEvent::horizontal;
    mHorizontalOverflow = newHorizontalOverflow;
  }

  nsScrollPortEvent event(PR_TRUE,
                          (orient == nsScrollPortEvent::horizontal ?
                           mHorizontalOverflow : mVerticalOverflow) ?
                            NS_SCROLLPORT_OVERFLOW : NS_SCROLLPORT_UNDERFLOW,
                          nsnull);
  event.orient = orient;
  return nsEventDispatcher::Dispatch(mOuter->GetContent(),
                                     mOuter->PresContext(), &event);
}

void
nsGfxScrollFrameInner::ReloadChildFrames()
{
  mScrolledFrame = nsnull;
  mHScrollbarBox = nsnull;
  mVScrollbarBox = nsnull;
  mScrollCornerBox = nsnull;

  nsIFrame* frame = mOuter->GetFirstChild(nsnull);
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
      } else {
        
        NS_ASSERTION(!mScrollCornerBox, "Found multiple scrollcorners");
        mScrollCornerBox = frame;
      }
    }

    frame = frame->GetNextSibling();
  }
}
  
nsresult
nsGfxScrollFrameInner::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  nsPresContext* presContext = mOuter->PresContext();
  nsIFrame* parent = mOuter->GetParent();

  
  
  if (!presContext->IsDynamic()) {
    
    
    if (!(mIsRoot && presContext->HasPaginatedScrolling())) {
      mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = PR_TRUE;
      return NS_OK;
    }
  }

  nsIScrollableFrame *scrollable = do_QueryFrame(mOuter);

  
  
  
  
  
  
  
  
  

  
  
  
  
  ScrollbarStyles styles = scrollable->GetScrollbarStyles();
  PRBool canHaveHorizontal = styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN;
  PRBool canHaveVertical = styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN;
  if (!canHaveHorizontal && !canHaveVertical) {
    
    return NS_OK;
  }

  
  nsITextControlFrame* textFrame = do_QueryFrame(parent);
  if (textFrame) {
    
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textAreaElement(do_QueryInterface(parent->GetContent()));
    if (!textAreaElement) {
      mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = PR_TRUE;
      return NS_OK;
    }
  }

  nsresult rv;

  nsNodeInfoManager *nodeInfoManager =
    presContext->Document()->NodeInfoManager();
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollbar, nsnull,
                                          kNameSpaceID_XUL);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  if (canHaveHorizontal) {
    rv = NS_NewElement(getter_AddRefs(mHScrollbarContent),
                       kNameSpaceID_XUL, nodeInfo, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("horizontal"), PR_FALSE);
    if (!aElements.AppendElement(mHScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveVertical) {
    rv = NS_NewElement(getter_AddRefs(mVScrollbarContent),
                       kNameSpaceID_XUL, nodeInfo, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("vertical"), PR_FALSE);
    if (!aElements.AppendElement(mVScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveHorizontal && canHaveVertical) {
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollcorner, nsnull,
                                            kNameSpaceID_XUL);
    rv = NS_NewElement(getter_AddRefs(mScrollCornerContent),
                       kNameSpaceID_XUL, nodeInfo, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aElements.AppendElement(mScrollCornerContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsGfxScrollFrameInner::Destroy()
{
  
  nsContentUtils::DestroyAnonymousContent(&mHScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mVScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mScrollCornerContent);

  mScrollEvent.Revoke();
  mAsyncScrollPortEvent.Revoke();
  if (mPostedReflowCallback) {
    mOuter->PresContext()->PresShell()->CancelReflowCallback(this);
    mPostedReflowCallback = PR_FALSE;
  }
  mScrolledAreaEventDispatcher.Revoke();
  nsIScrollableView *view = GetScrollableView();
  NS_ASSERTION(view, "unexpected null pointer");
  if (view)
    view->RemoveScrollPositionListener(this);
}

NS_IMETHODIMP
nsGfxScrollFrameInner::ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
   
   return NS_OK;
}







void
nsGfxScrollFrameInner::InternalScrollPositionDidChange(nscoord aX, nscoord aY)
{
  if (mVScrollbarBox)
    SetCoordAttribute(mVScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      aY - GetScrolledRect(GetScrollPortSize()).y);
  
  if (mHScrollbarBox)
    SetCoordAttribute(mHScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      aX - GetScrolledRect(GetScrollPortSize()).x);
}

void
nsGfxScrollFrameInner::ViewPositionDidChange(nsIScrollableView* aScrollable,
                                             nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  
  nsPoint childOffset = mScrolledFrame->GetView()->GetOffsetTo(mOuter->GetView());
  mScrolledFrame->SetPosition(childOffset);

  nsRootPresContext* rootPresContext = mOuter->PresContext()->RootPresContext();
  
  
  
  
  if (mOuter->GetWindow() ==
      rootPresContext->FrameManager()->GetRootFrame()->GetWindow()) {
    rootPresContext->GetPluginGeometryUpdates(mOuter, aConfigurations);
  }
}




NS_IMETHODIMP
nsGfxScrollFrameInner::ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY)
{
  NS_ASSERTION(!mViewInitiatedScroll, "Cannot reenter ScrollPositionDidChange");

  mViewInitiatedScroll = PR_TRUE;
  InternalScrollPositionDidChange(aX, aY);
  mViewInitiatedScroll = PR_FALSE;

  PostScrollEvent();

  return NS_OK;
}

void nsGfxScrollFrameInner::CurPosAttributeChanged(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "aContent must not be null");
  NS_ASSERTION((mHScrollbarBox && mHScrollbarBox->GetContent() == aContent) ||
               (mVScrollbarBox && mVScrollbarBox->GetContent() == aContent),
               "unexpected child");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mViewInitiatedScroll || mFrameInitiatedScroll) return;

  nsRect scrolledRect = GetScrolledRect(GetScrollPortSize());

  nscoord x = GetCoordAttribute(mHScrollbarBox, nsGkAtoms::curpos,
                                -scrolledRect.x) +
              scrolledRect.x;
  nscoord y = GetCoordAttribute(mVScrollbarBox, nsGkAtoms::curpos,
                                -scrolledRect.y) +
              scrolledRect.y;

  
  
  
  
  
  nscoord curPosX=0, curPosY=0;
  nsIScrollableView* s = GetScrollableView();
  if (s) {
    s->GetScrollPosition(curPosX, curPosY);
    if (x == curPosX && y == curPosY)
      return;

    PRBool isSmooth = aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::smooth);
        
    if (isSmooth) {
      
      
      
      

      NS_ASSERTION(!mFrameInitiatedScroll, "Unexpected reentry");
      
      
      mFrameInitiatedScroll = PR_TRUE;
      InternalScrollPositionDidChange(curPosX, curPosY);
      mFrameInitiatedScroll = PR_FALSE;
    }
    ScrollbarChanged(mOuter->PresContext(), x, y, isSmooth ? NS_VMREFRESH_SMOOTHSCROLL : 0);
  }
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

  nsScrollbarEvent event(PR_TRUE, NS_SCROLL_EVENT, nsnull);
  nsEventStatus status = nsEventStatus_eIgnore;
  nsIContent* content = mOuter->GetContent();
  nsPresContext* prescontext = mOuter->PresContext();
  
  
  if (mIsRoot) {
    nsIDocument* doc = content->GetCurrentDoc();
    if (doc) {
      nsEventDispatcher::Dispatch(doc, prescontext, &event, nsnull,  &status);
    }
  } else {
    
    
    event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
    nsEventDispatcher::Dispatch(content, prescontext, &event, nsnull, &status);
  }
}

void
nsGfxScrollFrameInner::PostScrollEvent()
{
  if (mScrollEvent.IsPending())
    return;

  nsRefPtr<ScrollEvent> ev = new ScrollEvent(this);
  if (NS_FAILED(NS_DispatchToCurrentThread(ev))) {
    NS_WARNING("failed to dispatch ScrollEvent");
  } else {
    mScrollEvent = ev;
  }
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

PRBool
nsXULScrollFrame::AddHorizontalScrollbar(nsBoxLayoutState& aState,
                                         nsRect& aScrollAreaSize, PRBool aOnTop)
{
  if (!mInner.mHScrollbarBox)
    return PR_TRUE;

  return AddRemoveScrollbar(aState, aScrollAreaSize, aOnTop, PR_TRUE, PR_TRUE);
}

PRBool
nsXULScrollFrame::AddVerticalScrollbar(nsBoxLayoutState& aState,
                                       nsRect& aScrollAreaSize, PRBool aOnRight)
{
  if (!mInner.mVScrollbarBox)
    return PR_TRUE;

  return AddRemoveScrollbar(aState, aScrollAreaSize, aOnRight, PR_FALSE, PR_TRUE);
}

void
nsXULScrollFrame::RemoveHorizontalScrollbar(nsBoxLayoutState& aState,
                                            nsRect& aScrollAreaSize, PRBool aOnTop)
{
  
#ifdef DEBUG
  PRBool result =
#endif
  AddRemoveScrollbar(aState, aScrollAreaSize, aOnTop, PR_TRUE, PR_FALSE);
  NS_ASSERTION(result, "Removing horizontal scrollbar failed to fit??");
}

void
nsXULScrollFrame::RemoveVerticalScrollbar(nsBoxLayoutState& aState,
                                          nsRect& aScrollAreaSize, PRBool aOnRight)
{
  
#ifdef DEBUG
  PRBool result =
#endif
  AddRemoveScrollbar(aState, aScrollAreaSize, aOnRight, PR_FALSE, PR_FALSE);
  NS_ASSERTION(result, "Removing vertical scrollbar failed to fit??");
}

PRBool
nsXULScrollFrame::AddRemoveScrollbar(nsBoxLayoutState& aState, nsRect& aScrollAreaSize,
                                     PRBool aOnTop, PRBool aHorizontal, PRBool aAdd)
{
  if (aHorizontal) {
     if (mInner.mNeverHasHorizontalScrollbar || !mInner.mHScrollbarBox)
       return PR_FALSE;

     nsSize hSize = mInner.mHScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mInner.mHScrollbarBox, hSize);

     mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, aAdd);

     PRBool hasHorizontalScrollbar;
     PRBool fit = AddRemoveScrollbar(hasHorizontalScrollbar, aScrollAreaSize.y, aScrollAreaSize.height, hSize.height, aOnTop, aAdd);
     mInner.mHasHorizontalScrollbar = hasHorizontalScrollbar;    
     if (!fit)
        mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, !aAdd);

     return fit;
  } else {
     if (mInner.mNeverHasVerticalScrollbar || !mInner.mVScrollbarBox)
       return PR_FALSE;

     nsSize vSize = mInner.mVScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mInner.mVScrollbarBox, vSize);

     mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, aAdd);

     PRBool hasVerticalScrollbar;
     PRBool fit = AddRemoveScrollbar(hasVerticalScrollbar, aScrollAreaSize.x, aScrollAreaSize.width, vSize.width, aOnTop, aAdd);
     mInner.mHasVerticalScrollbar = hasVerticalScrollbar;    
     if (!fit)
        mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, !aAdd);

     return fit;
  }
}

PRBool
nsXULScrollFrame::AddRemoveScrollbar(PRBool& aHasScrollbar, nscoord& aXY,
                                     nscoord& aSize, nscoord aSbSize,
                                     PRBool aRightOrBottom, PRBool aAdd)
{ 
   nscoord size = aSize;
   nscoord xy = aXY;

   if (size != NS_INTRINSICSIZE) {
     if (aAdd) {
        size -= aSbSize;
        if (!aRightOrBottom && size >= 0)
          xy += aSbSize;
     } else {
        size += aSbSize;
        if (!aRightOrBottom)
          xy -= aSbSize;
     }
   }

   
   if (size >= 0) {
       aHasScrollbar = aAdd;
       aSize = size;
       aXY = xy;
       return PR_TRUE;
   }

   aHasScrollbar = PR_FALSE;
   return PR_FALSE;
}

void
nsXULScrollFrame::LayoutScrollArea(nsBoxLayoutState& aState, const nsRect& aRect)
{
  nsIView* scrollView = mInner.mScrollableView->View();
  nsIViewManager* vm = scrollView->GetViewManager();
  vm->MoveViewTo(scrollView, aRect.x, aRect.y);
  vm->ResizeView(scrollView, nsRect(nsPoint(0, 0), aRect.Size()), PR_TRUE);

  PRUint32 oldflags = aState.LayoutFlags();
  nsPoint childOffset =
    mInner.mScrolledFrame->GetView()->GetOffsetTo(GetView());
  nsRect childRect = nsRect(childOffset, aRect.Size());

  PRInt32 flags = NS_FRAME_NO_MOVE_VIEW;

  nsSize minSize = mInner.mScrolledFrame->GetMinSize(aState);
  
  if (minSize.height > childRect.height)
    childRect.height = minSize.height;
  
  if (minSize.width > childRect.width)
    childRect.width = minSize.width;

  aState.SetLayoutFlags(flags);
  mInner.mScrolledFrame->SetBounds(aState, childRect);
  mInner.mScrolledFrame->Layout(aState);

  childRect = mInner.mScrolledFrame->GetRect();

  if (childRect.width < aRect.width || childRect.height < aRect.height)
  {
    childRect.width = NS_MAX(childRect.width, aRect.width);
    childRect.height = NS_MAX(childRect.height, aRect.height);

    
    
    mInner.mScrolledFrame->SetBounds(aState, childRect);
    mInner.mScrolledFrame->ClearOverflowRect();
  }

  aState.SetLayoutFlags(oldflags);

}

void nsGfxScrollFrameInner::PostOverflowEvent()
{
  if (mAsyncScrollPortEvent.IsPending())
    return;

  
  nsSize scrollportSize = GetScrollPortSize();
  nsSize childSize = GetScrolledRect(scrollportSize).Size();

  PRBool newVerticalOverflow = childSize.height > scrollportSize.height;
  PRBool vertChanged = mVerticalOverflow != newVerticalOverflow;

  PRBool newHorizontalOverflow = childSize.width > scrollportSize.width;
  PRBool horizChanged = mHorizontalOverflow != newHorizontalOverflow;

  if (!vertChanged && !horizChanged) {
    return;
  }

  nsRefPtr<AsyncScrollPortEvent> ev = new AsyncScrollPortEvent(this);
  if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev)))
    mAsyncScrollPortEvent = ev;
}

PRBool
nsGfxScrollFrameInner::IsLTR() const
{
  

  nsIFrame *frame = mOuter;
  
  if (mIsRoot) {
    
    nsPresContext *presContext = mOuter->PresContext();
    nsIDocument *document = presContext->Document();
    nsIContent *root = document->GetRootContent();

    
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(document);
    if (htmlDoc) {
      nsIContent *bodyContent = htmlDoc->GetBodyContentExternal();
      if (bodyContent)
        root = bodyContent; 
    }

    if (root) {
      nsIFrame *rootsFrame =
        presContext->PresShell()->GetPrimaryFrameFor(root);
      if (rootsFrame)
        frame = rootsFrame;
    }
  }

  return frame->GetStyleVisibility()->mDirection != NS_STYLE_DIRECTION_RTL;
}

PRBool
nsGfxScrollFrameInner::IsScrollbarOnRight() const
{
  nsPresContext *presContext = mOuter->PresContext();
  switch (presContext->GetCachedIntPref(kPresContext_ScrollbarSide)) {
    default:
    case 0: 
      return presContext->GetCachedIntPref(kPresContext_BidiDirection)
             == IBMBIDI_TEXTDIRECTION_LTR;
    case 1: 
      return IsLTR();
    case 2: 
      return PR_TRUE;
    case 3: 
      return PR_FALSE;
  }
}





nsresult
nsXULScrollFrame::Layout(nsBoxLayoutState& aState)
{
  PRBool scrollbarRight = mInner.IsScrollbarOnRight();
  PRBool scrollbarBottom = PR_TRUE;

  
  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);

  
  nsRect scrollAreaRect(clientRect);

  
























  ScrollbarStyles styles = GetScrollbarStyles();

  
  if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasHorizontalScrollbar = PR_TRUE;
  if (styles.mVertical == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasVerticalScrollbar = PR_TRUE;

  if (mInner.mHasHorizontalScrollbar)
     AddHorizontalScrollbar(aState, scrollAreaRect, scrollbarBottom);

  if (mInner.mHasVerticalScrollbar)
     AddVerticalScrollbar(aState, scrollAreaRect, scrollbarRight);
     
  nsRect oldScrollAreaBounds = mInner.mScrollableView->View()->GetBounds();

  
  LayoutScrollArea(aState, scrollAreaRect);
  
  
  PRBool needsLayout = PR_FALSE;

  
  if (styles.mVertical != NS_STYLE_OVERFLOW_SCROLL) {
    
    nsRect scrolledRect = mInner.GetScrolledRect(scrollAreaRect.Size());
    nsSize scrolledContentSize(scrolledRect.XMost(), scrolledRect.YMost());

    
      if (scrolledContentSize.height <= scrollAreaRect.height
          || styles.mVertical != NS_STYLE_OVERFLOW_AUTO) {
        if (mInner.mHasVerticalScrollbar) {
          
          
          RemoveVerticalScrollbar(aState, scrollAreaRect, scrollbarRight);
          needsLayout = PR_TRUE;
        }
      } else {
        if (!mInner.mHasVerticalScrollbar) {
          
          
          if (AddVerticalScrollbar(aState, scrollAreaRect, scrollbarRight))
            needsLayout = PR_TRUE;
        }
    }

    
    if (needsLayout) {
       nsBoxLayoutState resizeState(aState);
       LayoutScrollArea(resizeState, scrollAreaRect);
       needsLayout = PR_FALSE;
    }
  }


  
  if (styles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL)
  {
    
    nsRect scrolledRect = mInner.GetScrolledRect(scrollAreaRect.Size());
    nsSize scrolledContentSize(scrolledRect.XMost(), scrolledRect.YMost());

    
    
    if (scrolledContentSize.width > scrollAreaRect.width
        && styles.mHorizontal == NS_STYLE_OVERFLOW_AUTO) {

      if (!mInner.mHasHorizontalScrollbar) {
           
          if (AddHorizontalScrollbar(aState, scrollAreaRect, scrollbarBottom))
             needsLayout = PR_TRUE;

           
           
           
           
           
           
           
      }
    } else {
        
        
      if (mInner.mHasHorizontalScrollbar) {
        RemoveHorizontalScrollbar(aState, scrollAreaRect, scrollbarBottom);
        needsLayout = PR_TRUE;
      }
    }
  }

  
  if (needsLayout) {
     nsBoxLayoutState resizeState(aState);
     LayoutScrollArea(resizeState, scrollAreaRect);
     needsLayout = PR_FALSE;
  }
    
  
  nsSize hMinSize(0, 0);
  if (mInner.mHScrollbarBox && mInner.mHasHorizontalScrollbar) {
    GetScrollbarMetrics(aState, mInner.mHScrollbarBox, &hMinSize, nsnull, PR_FALSE);
  }
  nsSize vMinSize(0, 0);
  if (mInner.mVScrollbarBox && mInner.mHasVerticalScrollbar) {
    GetScrollbarMetrics(aState, mInner.mVScrollbarBox, &vMinSize, nsnull, PR_TRUE);
  }

  
  
  
  
  
  if (mInner.mHasHorizontalScrollbar &&
      (hMinSize.width > clientRect.width - vMinSize.width
       || hMinSize.height > clientRect.height)) {
    RemoveHorizontalScrollbar(aState, scrollAreaRect, scrollbarBottom);
    needsLayout = PR_TRUE;
  }
  
  if (mInner.mHasVerticalScrollbar &&
      (vMinSize.height > clientRect.height - hMinSize.height
       || vMinSize.width > clientRect.width)) {
    RemoveVerticalScrollbar(aState, scrollAreaRect, scrollbarRight);
    needsLayout = PR_TRUE;
  }

  
  if (needsLayout) {
    nsBoxLayoutState resizeState(aState);
    LayoutScrollArea(resizeState, scrollAreaRect);
  }

  if (!mInner.mSupppressScrollbarUpdate) { 
    mInner.LayoutScrollbars(aState, clientRect, oldScrollAreaBounds, scrollAreaRect);
  }
  if (!mInner.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mInner);
    mInner.mPostedReflowCallback = PR_TRUE;
  }
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    mInner.mHadNonInitialReflow = PR_TRUE;
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

PRBool
nsGfxScrollFrameInner::ReflowFinished()
{
  mPostedReflowCallback = PR_FALSE;

  ScrollToRestoredPosition();

  if (NS_SUBTREE_DIRTY(mOuter) || !mUpdateScrollbarAttributes)
    return PR_FALSE;

  mUpdateScrollbarAttributes = PR_FALSE;

  
  nsPresContext* presContext = mOuter->PresContext();

  if (mMayHaveDirtyFixedChildren) {
    mMayHaveDirtyFixedChildren = PR_FALSE;
    nsIFrame* parentFrame = mOuter->GetParent();
    for (nsIFrame* fixedChild =
           parentFrame->GetFirstChild(nsGkAtoms::fixedList);
         fixedChild; fixedChild = fixedChild->GetNextSibling()) {
      
      presContext->PresShell()->
        FrameNeedsReflow(fixedChild, nsIPresShell::eResize,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }

  nsIScrollableView* scrollable = GetScrollableView();
  nsRect scrollArea = scrollable->View()->GetBounds();

  const nsStyleFont* font = mOuter->GetStyleFont();
  const nsFont& f = font->mFont;
  nsCOMPtr<nsIFontMetrics> fm = presContext->GetMetricsFor(f);
  nscoord fontHeight = 1;
  NS_ASSERTION(fm,"FontMetrics is null assuming fontHeight == 1");
  if (fm)
    fm->GetHeight(fontHeight);
  scrollable->SetLineHeight(fontHeight);

  nsRect scrolledContentRect = GetScrolledRect(scrollArea.Size());
  nscoord minX = scrolledContentRect.x;
  nscoord maxX = scrolledContentRect.XMost() - scrollArea.width;
  nscoord minY = scrolledContentRect.y;
  nscoord maxY = scrolledContentRect.YMost() - scrollArea.height;

  
  NS_ASSERTION(!mFrameInitiatedScroll, "We shouldn't be reentering here");
  mFrameInitiatedScroll = PR_TRUE;

  nsCOMPtr<nsIContent> vScroll =
    mVScrollbarBox ? mVScrollbarBox->GetContent() : nsnull;
  nsCOMPtr<nsIContent> hScroll =
    mHScrollbarBox ? mHScrollbarBox->GetContent() : nsnull;

  
  
  if (vScroll || hScroll) {
    nsWeakFrame weakFrame(mOuter);
    nscoord curPosX, curPosY;
    scrollable->GetScrollPosition(curPosX, curPosY);
    if (vScroll) {
      
      
      
      
      
      
      nscoord pageincrement = nscoord(scrollArea.height - fontHeight);
      nscoord pageincrementMin = nscoord(float(scrollArea.height) * 0.8);
      FinishReflowForScrollbar(vScroll, minY, maxY, curPosY,
                               NS_MAX(pageincrement,pageincrementMin),
                               fontHeight);
    }
    if (hScroll) {
      FinishReflowForScrollbar(hScroll, minX, maxX, curPosX,
                               nscoord(float(scrollArea.width) * 0.8),
                               nsPresContext::CSSPixelsToAppUnits(10));
    }
    NS_ENSURE_TRUE(weakFrame.IsAlive(), PR_FALSE);
  }

  mFrameInitiatedScroll = PR_FALSE;
  
  
  
  
  
  
  
  
  
  
  
  if (!mHScrollbarBox && !mVScrollbarBox)
    return PR_FALSE;
  CurPosAttributeChanged(mVScrollbarBox ? mVScrollbarBox->GetContent()
                                        : mHScrollbarBox->GetContent());
  return PR_TRUE;
}

void
nsGfxScrollFrameInner::ReflowCallbackCanceled()
{
  mPostedReflowCallback = PR_FALSE;
}

static void LayoutAndInvalidate(nsBoxLayoutState& aState,
                                nsIFrame* aBox, const nsRect& aRect)
{
  
  
  
  PRBool rectChanged = aBox->GetRect() != aRect;
  if (rectChanged)
    aBox->Invalidate(aBox->GetOverflowRect());
  nsBoxFrame::LayoutChildAt(aState, aBox, aRect);
  if (rectChanged)
    aBox->Invalidate(aBox->GetOverflowRect());
}

static void AdjustScrollbarRect(nsIView* aView, nsPresContext* aPresContext,
                                nsRect& aRect, PRBool aVertical)
{
  if ((aVertical ? aRect.width : aRect.height) == 0)
    return;

  nsPoint offset;
  nsIWidget* widget = aView->GetNearestWidget(&offset);

  nsIntRect widgetRect;
  if (!widget->ShowsResizeIndicator(&widgetRect))
    return;

  nsRect resizerRect =
      nsRect(aPresContext->DevPixelsToAppUnits(widgetRect.x) - offset.x,
             aPresContext->DevPixelsToAppUnits(widgetRect.y) - offset.y,
             aPresContext->DevPixelsToAppUnits(widgetRect.width),
             aPresContext->DevPixelsToAppUnits(widgetRect.height));

  if (!resizerRect.Contains(aRect.BottomRight() - nsPoint(1, 1)))
    return;

  if (aVertical)
    aRect.height = NS_MAX(0, resizerRect.y - aRect.y);
  else
    aRect.width = NS_MAX(0, resizerRect.x - aRect.x);
}

void
nsGfxScrollFrameInner::LayoutScrollbars(nsBoxLayoutState& aState,
                                        const nsRect& aContentArea,
                                        const nsRect& aOldScrollArea,
                                        const nsRect& aScrollArea)
{
  NS_ASSERTION(!mSupppressScrollbarUpdate,
               "This should have been suppressed");
    
  nsIView* view = mOuter->GetView();
  nsPresContext* presContext = mScrolledFrame->PresContext();
  if (mVScrollbarBox) {
    NS_PRECONDITION(mVScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    nsRect vRect(aScrollArea);
    vRect.width = aContentArea.width - aScrollArea.width;
    vRect.x = IsScrollbarOnRight() ? aScrollArea.XMost() : aContentArea.x;
    nsMargin margin;
    mVScrollbarBox->GetMargin(margin);
    vRect.Deflate(margin);
    AdjustScrollbarRect(view, presContext, vRect, PR_TRUE);
    LayoutAndInvalidate(aState, mVScrollbarBox, vRect);
  }

  if (mHScrollbarBox) {
    NS_PRECONDITION(mHScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    nsRect hRect(aScrollArea);
    hRect.height = aContentArea.height - aScrollArea.height;
    hRect.y = PR_TRUE ? aScrollArea.YMost() : aContentArea.y;
    nsMargin margin;
    mHScrollbarBox->GetMargin(margin);
    hRect.Deflate(margin);
    AdjustScrollbarRect(view, presContext, hRect, PR_FALSE);
    LayoutAndInvalidate(aState, mHScrollbarBox, hRect);
  }

  
  if (mScrollCornerBox) {
    NS_PRECONDITION(mScrollCornerBox->IsBoxFrame(), "Must be a box frame!");
    nsRect r(0, 0, 0, 0);
    if (aContentArea.x != aScrollArea.x) {
      
      r.x = aContentArea.x;
      r.width = aScrollArea.x - aContentArea.x;
      NS_ASSERTION(r.width >= 0, "Scroll area should be inside client rect");
    } else {
      
      r.x = aScrollArea.XMost();
      r.width = aContentArea.XMost() - aScrollArea.XMost();
      NS_ASSERTION(r.width >= 0, "Scroll area should be inside client rect");
    }
    if (aContentArea.y != aScrollArea.y) {
      
      r.y = aContentArea.y;
      r.height = aScrollArea.y - aContentArea.y;
      NS_ASSERTION(r.height >= 0, "Scroll area should be inside client rect");
    } else {
      
      r.y = aScrollArea.YMost();
      r.height = aContentArea.YMost() - aScrollArea.YMost();
      NS_ASSERTION(r.height >= 0, "Scroll area should be inside client rect");
    }
    LayoutAndInvalidate(aState, mScrollCornerBox, r);
  }

  
  
  
  
  if (aOldScrollArea.Size() != aScrollArea.Size() && 
      !(mOuter->GetStateBits() & NS_FRAME_IS_DIRTY) &&
      mIsRoot) {
    mMayHaveDirtyFixedChildren = PR_TRUE;
  }
  
  
  mUpdateScrollbarAttributes = PR_TRUE;
  if (!mPostedReflowCallback) {
    aState.PresContext()->PresShell()->PostReflowCallback(this);
    mPostedReflowCallback = PR_TRUE;
  }
}

void
nsGfxScrollFrameInner::ScrollbarChanged(nsPresContext* aPresContext, nscoord aX, nscoord aY, PRUint32 aFlags)
{
  nsIScrollableView* scrollable = GetScrollableView();
  scrollable->ScrollTo(aX, aY, aFlags);
 
}

void
nsGfxScrollFrameInner::SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos)
{
  if (aMaxPos) {
    aContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::disabled, PR_TRUE);
  } else {
    aContent->SetAttr(kNameSpaceID_None, nsGkAtoms::disabled,
                      NS_LITERAL_STRING("true"), PR_TRUE);
  }
}

void
nsGfxScrollFrameInner::SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom,
                                         nscoord aSize)
{
  
  aSize = nsPresContext::AppUnitsToIntCSSPixels(aSize);

  

  nsAutoString newValue;
  newValue.AppendInt(aSize);

  if (aContent->AttrValueIs(kNameSpaceID_None, aAtom, newValue, eCaseMatters))
    return;

  aContent->SetAttr(kNameSpaceID_None, aAtom, newValue, PR_TRUE);
}

nsRect
nsGfxScrollFrameInner::GetScrolledRect(const nsSize& aScrollPortSize) const
{
  nsRect result = mScrolledFrame->GetOverflowRect();
  nscoord x1 = result.x, x2 = result.XMost(),
          y1 = result.y, y2 = result.YMost();
  if (y1 < 0)
    y1 = 0;
  if (IsLTR() || mIsXUL) {
    if (x1 < 0)
      x1 = 0;
  } else {
    if (x2 > aScrollPortSize.width)
      x2 = aScrollPortSize.width;
    
    
    
    
    
    
    nscoord extraWidth = NS_MAX(0, mScrolledFrame->GetSize().width - aScrollPortSize.width);
    x2 += extraWidth;
  }

  return nsRect(x1, y1, x2 - x1, y2 - y1);
}

nsMargin
nsGfxScrollFrameInner::GetActualScrollbarSizes() const {
  nsMargin border;
  mOuter->GetBorder(border);
  nsRect r(nsPoint(0,0), mOuter->GetSize());
  r.Deflate(border);
  nsRect scrollArea = mScrollableView->View()->GetBounds();

  return nsMargin(scrollArea.x - r.x, scrollArea.y - r.y,
                  r.XMost() - scrollArea.XMost(),
                  r.YMost() - scrollArea.YMost());
}

void
nsGfxScrollFrameInner::SetScrollbarVisibility(nsIBox* aScrollbar, PRBool aVisible)
{
  if (!aScrollbar)
    return;

  nsIScrollbarFrame* scrollbar = do_QueryFrame(aScrollbar);
  if (scrollbar) {
    
    nsIScrollbarMediator* mediator = scrollbar->GetScrollbarMediator();
    if (mediator) {
      
      mediator->VisibilityChanged(aVisible);
    }
  }
}

PRInt32
nsGfxScrollFrameInner::GetCoordAttribute(nsIBox* aBox, nsIAtom* atom, PRInt32 defaultValue)
{
  if (aBox) {
    nsIContent* content = aBox->GetContent();

    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, atom, value);
    if (!value.IsEmpty())
    {
      PRInt32 error;

      
      defaultValue = nsPresContext::CSSPixelsToAppUnits(value.ToInteger(&error));
    }
  }

  return defaultValue;
}

static nsIURI* GetDocURI(nsIFrame* aFrame)
{
  nsIPresShell* shell = aFrame->PresContext()->GetPresShell();
  if (!shell)
    return nsnull;
  nsIDocument* doc = shell->GetDocument();
  if (!doc)
    return nsnull;
  return doc->GetDocumentURI();
}

void
nsGfxScrollFrameInner::SaveVScrollbarStateToGlobalHistory()
{
  NS_ASSERTION(mIsRoot, "Only use this on viewports");

  
  
  if (mDidLoadHistoryVScrollbarHint &&
      (mHistoryVScrollbarHint == mHasVerticalScrollbar))
    return;

  nsIURI* uri = GetDocURI(mOuter);
  if (!uri)
    return;

  nsCOMPtr<nsIGlobalHistory3> history(do_GetService(NS_GLOBALHISTORY2_CONTRACTID));
  if (!history)
    return;
  
  PRUint32 flags = 0;
  if (mHasVerticalScrollbar) {
    flags |= NS_GECKO_FLAG_NEEDS_VERTICAL_SCROLLBAR;
  }
  history->SetURIGeckoFlags(uri, flags);
  
}

nsresult
nsGfxScrollFrameInner::GetVScrollbarHintFromGlobalHistory(PRBool* aVScrollbarNeeded)
{
  NS_ASSERTION(mIsRoot, "Only use this on viewports");
  NS_ASSERTION(!mDidLoadHistoryVScrollbarHint,
               "Should only load a hint once, it can be expensive");

  nsIURI* uri = GetDocURI(mOuter);
  if (!uri)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIGlobalHistory3> history(do_GetService(NS_GLOBALHISTORY2_CONTRACTID));
  if (!history)
    return NS_ERROR_FAILURE;
  
  PRUint32 flags;
  nsresult rv = history->GetURIGeckoFlags(uri, &flags);
  if (NS_FAILED(rv))
    return rv;

  *aVScrollbarNeeded = (flags & NS_GECKO_FLAG_NEEDS_VERTICAL_SCROLLBAR) != 0;
  mDidLoadHistoryVScrollbarHint = PR_TRUE;
  mHistoryVScrollbarHint = *aVScrollbarNeeded;
  return NS_OK;
}

nsPresState*
nsGfxScrollFrameInner::SaveState(nsIStatefulFrame::SpecialStateID aStateID)
{
  
  
  if (mIsRoot && aStateID == nsIStatefulFrame::eNoID) {
    return nsnull;
  }

  nsIScrollbarMediator* mediator = do_QueryFrame(GetScrolledFrame());
  if (mediator) {
    
    return nsnull;
  }

  nsIScrollableView* scrollingView = GetScrollableView();
  PRInt32 x,y;
  scrollingView->GetScrollPosition(x,y);
  
  if (!x && !y) {
    return nsnull;
  }

  nsIView* child = nsnull;
  scrollingView->GetScrolledView(child);
  if (!child) {
    return nsnull;
  }

  nsRect childRect = child->GetBounds();
  childRect.x = x;
  childRect.y = y;
  nsPresState* state = new nsPresState();
  if (!state) {
    return nsnull;
  }

  state->SetScrollState(childRect);

  return state;
}

void
nsGfxScrollFrameInner::RestoreState(nsPresState* aState)
{
  mRestoreRect = aState->GetScrollState();
  mLastPos.x = -1;
  mLastPos.y = -1;
  mDidHistoryRestore = PR_TRUE;
  nsIScrollableView* scrollingView = GetScrollableView();
  if (scrollingView) {
    scrollingView->GetScrollPosition(mLastPos.x, mLastPos.y);
  } else {
    mLastPos = nsPoint(0, 0);
  }
}

void
nsGfxScrollFrameInner::PostScrolledAreaEvent(nsRect &aScrolledArea)
{
  if (mScrolledAreaEventDispatcher.IsPending()) {
    mScrolledAreaEventDispatcher.get()->mScrolledArea = aScrolledArea;
    return;
  }

  nsRefPtr<ScrolledAreaEventDispatcher> dp = new ScrolledAreaEventDispatcher(this);
  if (!dp) {
    NS_WARNING("OOM while posting NS_SCROLLEDAREACHANGED");
    return;
  }

  dp->mScrolledArea = aScrolledArea;

  if (NS_FAILED(NS_DispatchToCurrentThread(dp))) {
    NS_WARNING("Failed to dispatch ScrolledAreaEventDispatcher");
  } else {
    mScrolledAreaEventDispatcher = dp;
  }  
}




NS_IMETHODIMP
nsGfxScrollFrameInner::ScrolledAreaEventDispatcher::Run()
{
  if (mScrollFrameInner)
    mScrollFrameInner->FireScrolledAreaEvent(mScrolledArea);
  return NS_OK;
}

void
nsGfxScrollFrameInner::FireScrolledAreaEvent(nsRect &aScrolledArea)
{
  mScrolledAreaEventDispatcher.Forget();

  nsScrollAreaEvent event(PR_TRUE, NS_SCROLLEDAREACHANGED, nsnull);
  nsPresContext *prescontext = mOuter->PresContext();
  nsIContent* content = mOuter->GetContent();

  event.mArea = aScrolledArea;

  nsIDocument *doc = content->GetCurrentDoc();
  if (doc) {
    nsEventDispatcher::Dispatch(doc, prescontext, &event, nsnull);
  }
}


