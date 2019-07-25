








































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"
#include "nsIServiceManager.h"
#include "nsIView.h"
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
#include "nsAccessibilityService.h"
#endif
#include "nsBidiUtils.h"
#include "nsFrameManager.h"
#include "nsIPrefService.h"
#include "nsILookAndFeel.h"
#include "mozilla/dom/Element.h"
#include "FrameLayerBuilder.h"
#include "nsSMILKeySpline.h"

using namespace mozilla;
using namespace mozilla::dom;





nsIFrame*
NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot)
{
  return new (aPresShell) nsHTMLScrollFrame(aPresShell, aContext, aIsRoot);
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLScrollFrame)

nsHTMLScrollFrame::nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot)
  : nsHTMLContainerFrame(aContext),
    mInner(this, aIsRoot)
{
}

nsresult
nsHTMLScrollFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsHTMLScrollFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                            PRUint32 aFilter)
{
  mInner.AppendAnonymousContentTo(aElements, aFilter);
}

void
nsHTMLScrollFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mInner.Destroy();
  nsHTMLContainerFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsHTMLScrollFrame::SetInitialChildList(nsIAtom*     aListName,
                                       nsFrameList& aChildList)
{
  nsresult rv = nsHTMLContainerFrame::SetInitialChildList(aListName, aChildList);
  mInner.ReloadChildFrames();
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
      
      nsRect parentDamage;
      
      
      
      nsRect displayport;
      PRBool usingDisplayport = nsLayoutUtils::GetDisplayPort(GetContent(),
                                                              &displayport);
      if (usingDisplayport) {
        parentDamage.IntersectRect(damage, displayport);
      } else {
        parentDamage.IntersectRect(damage, mInner.mScrollPort);
      }

      if (IsScrollingActive()) {
        
        
        
        
        
        
        
        
        
        nsRect thebesLayerDamage = damage + GetScrollPosition() - mInner.mScrollPosAtLastPaint;
        if (parentDamage == thebesLayerDamage) {
          
          nsHTMLContainerFrame::InvalidateInternal(parentDamage, 0, 0, aForChild, aFlags);
        } else {
          
          if (!(aFlags & INVALIDATE_NO_THEBES_LAYERS)) {
            nsHTMLContainerFrame::InvalidateInternal(thebesLayerDamage, 0, 0, aForChild,
                                                     aFlags | INVALIDATE_ONLY_THEBES_LAYERS);
          }
          if (!(aFlags & INVALIDATE_ONLY_THEBES_LAYERS) && !parentDamage.IsEmpty()) {
            nsHTMLContainerFrame::InvalidateInternal(parentDamage, 0, 0, aForChild,
                                                     aFlags | INVALIDATE_NO_THEBES_LAYERS);
          }
        }
      } else {
        if (!parentDamage.IsEmpty()) {
          nsHTMLContainerFrame::InvalidateInternal(parentDamage, 0, 0, aForChild, aFlags);
        }
      }

      if (mInner.mIsRoot && parentDamage != damage) {
        
        
        
        
        
        
        
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

  
  nsRect mContentsOverflowArea;
  PRPackedBool mReflowedContentsWithHScrollbar;
  PRPackedBool mReflowedContentsWithVScrollbar;

  
  
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
    nsRect scrolledRect =
      mInner.GetScrolledRectInternal(aState->mContentsOverflowArea, scrollPortSize);
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
  mInner.mScrollPort = nsRect(scrollPortOrigin, scrollPortSize);
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

  
  
  PRBool didHaveHorizontalScrollbar = mInner.mHasHorizontalScrollbar;
  PRBool didHaveVerticalScrollbar = mInner.mHasVerticalScrollbar;
  mInner.mHasHorizontalScrollbar = aAssumeHScroll;
  mInner.mHasVerticalScrollbar = aAssumeVScroll;

  nsReflowStatus status;
  nsresult rv = ReflowChild(mInner.mScrolledFrame, presContext, *aMetrics,
                            kidReflowState, 0, 0,
                            NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW, status);

  mInner.mHasHorizontalScrollbar = didHaveHorizontalScrollbar;
  mInner.mHasVerticalScrollbar = didHaveVerticalScrollbar;

  
  
  
  
  
  FinishReflowChild(mInner.mScrolledFrame, presContext,
                    &kidReflowState, *aMetrics, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_SIZE_VIEW);

  
  
  
  
  
  
  
  aMetrics->UnionOverflowAreasWithDesiredBounds();

  aState->mContentsOverflowArea = aMetrics->ScrollableOverflow();
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
    nsRect scrolledRect =
      mInner.GetScrolledRectInternal(kidDesiredSize.ScrollableOverflow(),
                                     insideBorderSize);
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
nsHTMLScrollFrame::PlaceScrollArea(const ScrollReflowState& aState,
                                   const nsPoint& aScrollPosition)
{
  nsIFrame *scrolledFrame = mInner.mScrolledFrame;
  
  scrolledFrame->SetPosition(mInner.mScrollPort.TopLeft() - aScrollPosition);

  nsRect scrolledArea;
  
  nsSize portSize = mInner.mScrollPort.Size();
  nsRect scrolledRect = mInner.GetScrolledRectInternal(aState.mContentsOverflowArea, portSize);
  scrolledArea.UnionRectIncludeEmpty(scrolledRect,
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

PRBool
nsHTMLScrollFrame::IsCollapsed(nsBoxLayoutState& aBoxLayoutState)
{
  
  return PR_FALSE;
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

  return nsnull;
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
    reflowScrollCorner = NEEDS_REFLOW(mInner.mScrollCornerBox) ||
                         NEEDS_REFLOW(mInner.mResizerBox);

    #undef NEEDS_REFLOW
  }

  if (mInner.mIsRoot) {
    mInner.mCollapsedResizer = PR_TRUE;

    nsIContent* browserRoot = GetBrowserRoot(mContent);
    if (browserRoot) {
      PRBool showResizer = browserRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::showresizer);
      reflowScrollCorner = showResizer == mInner.mCollapsedResizer;
      mInner.mCollapsedResizer = !showResizer;
    }
  }

  nsRect oldScrollAreaBounds = mInner.mScrollPort;
  nsRect oldScrolledAreaBounds =
    mInner.mScrolledFrame->GetScrollableOverflowRectRelativeToParent();
  
  
  
  nsIntPoint ptDevPx;
  nsPoint oldScrollPosition = mInner.GetScrollPosition();
  
  state.mComputedBorder = aReflowState.mComputedBorderPadding -
    aReflowState.mComputedPadding;

  nsresult rv = ReflowContents(&state, aDesiredSize);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  PlaceScrollArea(state, oldScrollPosition);
  if (!mInner.mPostedReflowCallback) {
    
    PresContext()->PresShell()->PostReflowCallback(&mInner);
    mInner.mPostedReflowCallback = PR_TRUE;
  }

  PRBool didHaveHScrollbar = mInner.mHasHorizontalScrollbar;
  PRBool didHaveVScrollbar = mInner.mHasVerticalScrollbar;
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
                              oldScrollAreaBounds);
    } else {
      mInner.mSkippedScrollbarLayout = PR_TRUE;
    }
  }

  aDesiredSize.width = state.mInsideBorderSize.width +
    state.mComputedBorder.LeftRight();
  aDesiredSize.height = state.mInsideBorderSize.height +
    state.mComputedBorder.TopBottom();

  aDesiredSize.SetOverflowAreasToDesiredBounds();

  CheckInvalidateSizeChange(aDesiredSize);

  FinishAndStoreOverflow(&aDesiredSize);

  if (!InInitialReflow() && !mInner.mHadNonInitialReflow) {
    mInner.mHadNonInitialReflow = PR_TRUE;
  }

  if (mInner.mIsRoot && oldScrolledAreaBounds != newScrolledAreaBounds) {
    mInner.PostScrolledAreaEvent();
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
already_AddRefed<nsAccessible>
nsHTMLScrollFrame::CreateAccessible()
{
  if (!IsFocusable()) {
    return nsnull;
  }
  
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    return accService->CreateHyperTextAccessible(mContent,
                                                 PresContext()->PresShell());
  }

  return nsnull;
}
#endif

NS_QUERYFRAME_HEAD(nsHTMLScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
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
    mInner(this, aIsRoot)
{
    SetLayoutManager(nsnull);
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

nsresult
nsXULScrollFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  return mInner.CreateAnonymousContent(aElements);
}

void
nsXULScrollFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                           PRUint32 aFilter)
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
nsXULScrollFrame::SetInitialChildList(nsIAtom*        aListName,
                                      nsFrameList&    aChildList)
{
  nsresult rv = nsBoxFrame::SetInitialChildList(aListName, aChildList);
  mInner.ReloadChildFrames();
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
    nsRect damage = aDamageRect + nsPoint(aX, aY);
    
    nsRect parentDamage;
    
    
    
    nsRect displayport;
    PRBool usingDisplayport = nsLayoutUtils::GetDisplayPort(GetContent(),
                                                            &displayport);
    if (usingDisplayport) {
      parentDamage.IntersectRect(damage, displayport);
    } else {
      parentDamage.IntersectRect(damage, mInner.mScrollPort);
    }

    if (IsScrollingActive()) {
      
      
      
      
      
      
      
      
      
      nsRect thebesLayerDamage = damage + GetScrollPosition() - mInner.mScrollPosAtLastPaint;
      if (parentDamage == thebesLayerDamage) {
        
        nsBoxFrame::InvalidateInternal(parentDamage, 0, 0, aForChild, aFlags);
      } else {
        
        if (!(aFlags & INVALIDATE_NO_THEBES_LAYERS)) {
          nsBoxFrame::InvalidateInternal(thebesLayerDamage, 0, 0, aForChild,
                                         aFlags | INVALIDATE_ONLY_THEBES_LAYERS);
        }
        if (!(aFlags & INVALIDATE_ONLY_THEBES_LAYERS) && !parentDamage.IsEmpty()) {
          nsBoxFrame::InvalidateInternal(parentDamage, 0, 0, aForChild,
                                         aFlags | INVALIDATE_NO_THEBES_LAYERS);
        }
      }
    } else {
      if (!parentDamage.IsEmpty()) {
        nsBoxFrame::InvalidateInternal(parentDamage, 0, 0, aForChild, aFlags);
      }
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
  PRBool widthSet, heightSet;
  nsIBox::AddCSSPrefSize(this, pref, widthSet, heightSet);
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
  PRBool widthSet, heightSet;
  nsIBox::AddCSSMinSize(aState, this, min, widthSet, heightSet);
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
  PRBool widthSet, heightSet;
  nsIBox::AddCSSMaxSize(this, maxSize, widthSet, heightSet);
  return maxSize;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsXULScrollFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("XULScroll"), aResult);
}
#endif

NS_IMETHODIMP
nsXULScrollFrame::DoLayout(nsBoxLayoutState& aState)
{
  PRUint32 flags = aState.LayoutFlags();
  nsresult rv = Layout(aState);
  aState.SetLayoutFlags(flags);

  nsBox::DoLayout(aState);
  return rv;
}

NS_QUERYFRAME_HEAD(nsXULScrollFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsIScrollableFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)
 


#define SMOOTH_SCROLL_PREF_NAME "general.smoothScroll"

const double kCurrentVelocityWeighting = 0.25;
const double kStopDecelerationWeighting = 0.4;
const double kSmoothScrollAnimationDuration = 150; 

class nsGfxScrollFrameInner::AsyncScroll {
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  AsyncScroll() {}
  ~AsyncScroll() {
    if (mScrollTimer) mScrollTimer->Cancel();
  }

  nsPoint PositionAt(TimeStamp aTime);
  nsSize VelocityAt(TimeStamp aTime); 

  void InitSmoothScroll(TimeStamp aTime, nsPoint aCurrentPos,
                        nsSize aCurrentVelocity, nsPoint aDestination);

  PRBool IsFinished(TimeStamp aTime) {
    return aTime > mStartTime + mDuration; 
  }

  nsCOMPtr<nsITimer> mScrollTimer;
  TimeStamp mStartTime;
  TimeDuration mDuration;
  nsPoint mStartPos;
  nsPoint mDestination;
  nsSMILKeySpline mTimingFunctionX;
  nsSMILKeySpline mTimingFunctionY;
  PRPackedBool mIsSmoothScroll;

protected:
  double ProgressAt(TimeStamp aTime) {
    return NS_MIN(1.0, NS_MAX(0.0, (aTime - mStartTime) / mDuration));
  }

  nscoord VelocityComponent(double aTimeProgress,
                            nsSMILKeySpline& aTimingFunction,
                            nscoord aStart, nscoord aDestination);

  
  
  void InitTimingFunction(nsSMILKeySpline& aTimingFunction,
                          nscoord aCurrentPos, nscoord aCurrentVelocity,
                          nscoord aDestination);
};

nsPoint
nsGfxScrollFrameInner::AsyncScroll::PositionAt(TimeStamp aTime) {
  double progressX = mTimingFunctionX.GetSplineValue(ProgressAt(aTime));
  double progressY = mTimingFunctionY.GetSplineValue(ProgressAt(aTime));
  return nsPoint((1 - progressX) * mStartPos.x + progressX * mDestination.x,
                 (1 - progressY) * mStartPos.y + progressY * mDestination.y);
}

nsSize
nsGfxScrollFrameInner::AsyncScroll::VelocityAt(TimeStamp aTime) {
  double timeProgress = ProgressAt(aTime);
  return nsSize(VelocityComponent(timeProgress, mTimingFunctionX,
                                  mStartPos.x, mDestination.x),
                VelocityComponent(timeProgress, mTimingFunctionY,
                                  mStartPos.y, mDestination.y));
}

void
nsGfxScrollFrameInner::AsyncScroll::InitSmoothScroll(TimeStamp aTime,
                                                     nsPoint aCurrentPos,
                                                     nsSize aCurrentVelocity,
                                                     nsPoint aDestination) {
  mStartTime = aTime;
  mStartPos = aCurrentPos;
  mDestination = aDestination;
  mDuration = TimeDuration::FromMilliseconds(kSmoothScrollAnimationDuration);
  InitTimingFunction(mTimingFunctionX, mStartPos.x, aCurrentVelocity.width, aDestination.x);
  InitTimingFunction(mTimingFunctionY, mStartPos.y, aCurrentVelocity.height, aDestination.y);
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
  return (slope * (aDestination - aStart) / (mDuration / oneSecond));
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

static PRBool
IsSmoothScrollingEnabled()
{
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRBool enabled;
    nsresult rv = prefs->GetBoolPref(SMOOTH_SCROLL_PREF_NAME, &enabled);
    if (NS_SUCCEEDED(rv)) {
      return enabled;
    }
  }
  return PR_FALSE;
}

class ScrollFrameActivityTracker : public nsExpirationTracker<nsGfxScrollFrameInner,4> {
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

static ScrollFrameActivityTracker *gScrollFrameActivityTracker = nsnull;

nsGfxScrollFrameInner::nsGfxScrollFrameInner(nsContainerFrame* aOuter,
                                             PRBool aIsRoot)
  : mHScrollbarBox(nsnull)
  , mVScrollbarBox(nsnull)
  , mScrolledFrame(nsnull)
  , mScrollCornerBox(nsnull)
  , mResizerBox(nsnull)
  , mOuter(aOuter)
  , mAsyncScroll(nsnull)
  , mDestination(0, 0)
  , mScrollPosAtLastPaint(0, 0)
  , mRestorePos(-1, -1)
  , mLastPos(-1, -1)
  , mNeverHasVerticalScrollbar(PR_FALSE)
  , mNeverHasHorizontalScrollbar(PR_FALSE)
  , mHasVerticalScrollbar(PR_FALSE)
  , mHasHorizontalScrollbar(PR_FALSE)
  , mFrameIsUpdatingScrollbar(PR_FALSE)
  , mDidHistoryRestore(PR_FALSE)
  , mIsRoot(aIsRoot)
  , mSupppressScrollbarUpdate(PR_FALSE)
  , mSkippedScrollbarLayout(PR_FALSE)
  , mHadNonInitialReflow(PR_FALSE)
  , mHorizontalOverflow(PR_FALSE)
  , mVerticalOverflow(PR_FALSE)
  , mPostedReflowCallback(PR_FALSE)
  , mMayHaveDirtyFixedChildren(PR_FALSE)
  , mUpdateScrollbarAttributes(PR_FALSE)
  , mCollapsedResizer(PR_FALSE)
  , mShouldBuildLayer(PR_FALSE)
{
  
  PRBool canOverlap;
  nsPresContext* presContext = mOuter->PresContext();
  presContext->LookAndFeel()->
    GetMetric(nsILookAndFeel::eMetric_ScrollbarsCanOverlapContent, canOverlap);
  mScrollbarsCanOverlapContent = canOverlap;
  mScrollingActive = IsAlwaysActive();
}

nsGfxScrollFrameInner::~nsGfxScrollFrameInner()
{
  if (mActivityExpirationState.IsTracked()) {
    gScrollFrameActivityTracker->RemoveObject(this);
  }
  if (gScrollFrameActivityTracker &&
      gScrollFrameActivityTracker->IsEmpty()) {
    delete gScrollFrameActivityTracker;
    gScrollFrameActivityTracker = nsnull;
  }
  delete mAsyncScroll;
}

static nscoord
Clamp(nscoord aLower, nscoord aVal, nscoord aUpper)
{
  if (aVal < aLower)
    return aLower;
  if (aVal > aUpper)
    return aUpper;
  return aVal;
}

nsPoint
nsGfxScrollFrameInner::ClampScrollPosition(const nsPoint& aPt) const
{
  nsRect range = GetScrollRange();
  return nsPoint(Clamp(range.x, aPt.x, range.XMost()),
                 Clamp(range.y, aPt.y, range.YMost()));
}




void
nsGfxScrollFrameInner::AsyncScrollCallback(nsITimer *aTimer, void* anInstance)
{
  nsGfxScrollFrameInner* self = static_cast<nsGfxScrollFrameInner*>(anInstance);
  if (!self || !self->mAsyncScroll)
    return;

  if (self->mAsyncScroll->mIsSmoothScroll) {
    TimeStamp now = TimeStamp::Now();
    nsPoint destination = self->mAsyncScroll->PositionAt(now);
    if (self->mAsyncScroll->IsFinished(now)) {
      delete self->mAsyncScroll;
      self->mAsyncScroll = nsnull;
    }

    self->ScrollToImpl(destination);
  } else {
    delete self->mAsyncScroll;
    self->mAsyncScroll = nsnull;

    self->ScrollToImpl(self->mDestination);
  }
}





void
nsGfxScrollFrameInner::ScrollTo(nsPoint aScrollPosition,
                                nsIScrollableFrame::ScrollMode aMode)
{
  mDestination = ClampScrollPosition(aScrollPosition);

  if (aMode == nsIScrollableFrame::INSTANT) {
    
    
    delete mAsyncScroll;
    mAsyncScroll = nsnull;
    ScrollToImpl(mDestination);
    return;
  }

  TimeStamp now = TimeStamp::Now();
  nsPoint currentPosition = GetScrollPosition();
  nsSize currentVelocity(0, 0);
  PRBool isSmoothScroll = (aMode == nsIScrollableFrame::SMOOTH) &&
                          IsSmoothScrollingEnabled();

  if (mAsyncScroll) {
    if (mAsyncScroll->mIsSmoothScroll) {
      currentPosition = mAsyncScroll->PositionAt(now);
      currentVelocity = mAsyncScroll->VelocityAt(now);
    }
  } else {
    mAsyncScroll = new AsyncScroll;
    if (mAsyncScroll) {
      mAsyncScroll->mScrollTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (!mAsyncScroll->mScrollTimer) {
        delete mAsyncScroll;
        mAsyncScroll = nsnull;
      }
    }
    if (!mAsyncScroll) {
      
      ScrollToImpl(mDestination);
      return;
    }
    if (isSmoothScroll) {
      mAsyncScroll->mScrollTimer->InitWithFuncCallback(
        AsyncScrollCallback, this, 1000 / 60,
        nsITimer::TYPE_REPEATING_SLACK);
    } else {
      mAsyncScroll->mScrollTimer->InitWithFuncCallback(
        AsyncScrollCallback, this, 0, nsITimer::TYPE_ONE_SHOT);
    }
  }

  mAsyncScroll->mIsSmoothScroll = isSmoothScroll;

  if (isSmoothScroll) {
    mAsyncScroll->InitSmoothScroll(now, currentPosition, currentVelocity,
                                   aScrollPosition);
  }
}



static void AdjustViews(nsIFrame* aFrame)
{
  nsIView* view = aFrame->GetView();
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

  nsIAtom* childListName = nsnull;
  PRInt32  childListIndex = 0;
  do {
    
    nsIFrame* childFrame = aFrame->GetFirstChild(childListName);
    while (childFrame) {
      AdjustViews(childFrame);

      
      childFrame = childFrame->GetNextSibling();
    }

    
    
    do {
      childListName = aFrame->GetAdditionalChildListName(childListIndex++);
    } while (childListName == nsGkAtoms::popupList);
  } while (childListName);
}

static PRBool
CanScrollWithBlitting(nsIFrame* aFrame)
{
  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    if (f->GetStyleDisplay()->HasTransform()) {
      return PR_FALSE;
    }
#ifdef MOZ_SVG
    if (nsSVGIntegrationUtils::UsingEffectsForFrame(f) ||
        f->IsFrameOfType(nsIFrame::eSVG)) {
      return PR_FALSE;
    }
#endif
    nsIScrollableFrame* sf = do_QueryFrame(f);
    if (sf && nsLayoutUtils::HasNonZeroCorner(f->GetStyleBorder()->mBorderRadius))
      return PR_FALSE;
    if (nsLayoutUtils::IsPopup(f))
      break;
  }
  return PR_TRUE;
}

static void
InvalidateFixedBackgroundFramesFromList(nsDisplayListBuilder* aBuilder,
                                        nsIFrame* aMovingFrame,
                                        const nsDisplayList& aList)
{
  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    nsDisplayList* sublist = item->GetList();
    if (sublist) {
      InvalidateFixedBackgroundFramesFromList(aBuilder, aMovingFrame, *sublist);
      continue;
    }
    nsIFrame* f = item->GetUnderlyingFrame();
    if (f &&
        item->IsVaryingRelativeToMovingFrame(aBuilder, aMovingFrame)) {
      if (FrameLayerBuilder::NeedToInvalidateFixedDisplayItem(aBuilder, item)) {
        
        f->Invalidate(item->GetVisibleRect() - item->ToReferenceFrame());
      }
    }
  }
}

static void
InvalidateFixedBackgroundFrames(nsIFrame* aRootFrame,
                                nsIFrame* aMovingFrame,
                                const nsRect& aUpdateRect)
{
  if (!aMovingFrame->PresContext()->MayHaveFixedBackgroundFrames())
    return;

  NS_ASSERTION(aRootFrame != aMovingFrame,
               "The root frame shouldn't be the one that's moving, that makes no sense");

  
  nsDisplayListBuilder builder(aRootFrame, nsDisplayListBuilder::OTHER, PR_TRUE);
  builder.EnterPresShell(aRootFrame, aUpdateRect);
  nsDisplayList list;
  nsresult rv =
    aRootFrame->BuildDisplayListForStackingContext(&builder, aUpdateRect, &list);
  builder.LeavePresShell(aRootFrame, aUpdateRect);
  if (NS_FAILED(rv))
    return;

  nsRegion visibleRegion(aUpdateRect);
  list.ComputeVisibilityForRoot(&builder, &visibleRegion);

  InvalidateFixedBackgroundFramesFromList(&builder, aMovingFrame, list);
  list.DeleteAll();
}

PRBool nsGfxScrollFrameInner::IsAlwaysActive() const
{
  
  
  
  
  return mIsRoot && mOuter->PresContext()->IsRootContentDocument();
}

void nsGfxScrollFrameInner::MarkInactive()
{
  if (IsAlwaysActive() || !mScrollingActive)
    return;

  mScrollingActive = PR_FALSE;
  mOuter->InvalidateFrameSubtree();
}

void nsGfxScrollFrameInner::MarkActive()
{
  if (IsAlwaysActive())
    return;

  mScrollingActive = PR_TRUE;
  if (mActivityExpirationState.IsTracked()) {
    gScrollFrameActivityTracker->MarkUsed(this);
  } else {
    if (!gScrollFrameActivityTracker) {
      gScrollFrameActivityTracker = new ScrollFrameActivityTracker();
    }
    gScrollFrameActivityTracker->AddObject(this);
  }
}

void nsGfxScrollFrameInner::ScrollVisual()
{
  nsRootPresContext* rootPresContext = mOuter->PresContext()->GetRootPresContext();
  if (!rootPresContext) {
    return;
  }

  rootPresContext->RequestUpdatePluginGeometry(mOuter);

  AdjustViews(mScrolledFrame);
  
  
  PRUint32 flags = nsIFrame::INVALIDATE_REASON_SCROLL_REPAINT;
  PRBool canScrollWithBlitting = CanScrollWithBlitting(mOuter);
  if (IsScrollingActive()) {
    if (!canScrollWithBlitting) {
      MarkInactive();
    } else {
      flags |= nsIFrame::INVALIDATE_NO_THEBES_LAYERS;
    }
  }
  if (canScrollWithBlitting) {
    MarkActive();
  }

  nsRect invalidateRect, displayport;
  invalidateRect =
    (nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &displayport)) ?
    displayport : mScrollPort;

  mOuter->InvalidateWithFlags(invalidateRect, flags);

  if (flags & nsIFrame::INVALIDATE_NO_THEBES_LAYERS) {
    nsIFrame* displayRoot = nsLayoutUtils::GetDisplayRootFrame(mOuter);
    nsRect update =
      GetScrollPortRect() + mOuter->GetOffsetToCrossDoc(displayRoot);
    update = update.ConvertAppUnitsRoundOut(
      mOuter->PresContext()->AppUnitsPerDevPixel(),
      displayRoot->PresContext()->AppUnitsPerDevPixel());
    InvalidateFixedBackgroundFrames(displayRoot, mScrolledFrame, update);
  }
}

static PRInt32
ClampInt(nscoord aLower, nscoord aVal, nscoord aUpper, nscoord aAppUnitsPerPixel)
{
  PRInt32 low = NSToIntCeil(float(aLower)/aAppUnitsPerPixel);
  PRInt32 high = NSToIntFloor(float(aUpper)/aAppUnitsPerPixel);
  PRInt32 v = NSToIntRound(float(aVal)/aAppUnitsPerPixel);
  NS_ASSERTION(low <= high, "No integers in range; 0 is supposed to be in range");
  if (v < low)
    return low;
  if (v > high)
    return high;
  return v;
}

nsPoint
nsGfxScrollFrameInner::ClampAndRestrictToDevPixels(const nsPoint& aPt,
                                                   nsIntPoint* aPtDevPx) const
{
  nsPresContext* presContext = mOuter->PresContext();
  nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  
  
  
  nsRect scrollRange = GetScrollRange();
  *aPtDevPx = nsIntPoint(ClampInt(scrollRange.x, aPt.x, scrollRange.XMost(), appUnitsPerDevPixel),
                         ClampInt(scrollRange.y, aPt.y, scrollRange.YMost(), appUnitsPerDevPixel));
  return nsPoint(NSIntPixelsToAppUnits(aPtDevPx->x, appUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(aPtDevPx->y, appUnitsPerDevPixel));
}

void
nsGfxScrollFrameInner::ScrollToImpl(nsPoint aPt)
{
  nsPresContext* presContext = mOuter->PresContext();
  nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsIntPoint ptDevPx;
  nsPoint pt = ClampAndRestrictToDevPixels(aPt, &ptDevPx);

  nsPoint curPos = GetScrollPosition();
  if (pt == curPos) {
    return;
  }
  nsIntPoint curPosDevPx(NSAppUnitsToIntPixels(curPos.x, appUnitsPerDevPixel),
                         NSAppUnitsToIntPixels(curPos.y, appUnitsPerDevPixel));
  
  
  NS_ASSERTION(curPosDevPx.x*appUnitsPerDevPixel == curPos.x,
               "curPos.x not a multiple of device pixels");
  NS_ASSERTION(curPosDevPx.y*appUnitsPerDevPixel == curPos.y,
               "curPos.y not a multiple of device pixels");

  
  for (PRUint32 i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->ScrollPositionWillChange(pt.x, pt.y);
  }
  
  
  mScrolledFrame->SetPosition(mScrollPort.TopLeft() - pt);

  
  ScrollVisual();

  presContext->PresShell()->SynthesizeMouseMove(PR_TRUE);
  UpdateScrollbarPosition();
  PostScrollEvent();

  
  for (PRUint32 i = 0; i < mListeners.Length(); i++) {
    mListeners[i]->ScrollPositionDidChange(pt.x, pt.y);
  }
}

static void
AppendToTop(nsDisplayListBuilder* aBuilder, nsDisplayList* aDest,
            nsDisplayList* aSource, nsIFrame* aSourceFrame, PRBool aOwnLayer)
{
  if (aOwnLayer) {
    aDest->AppendNewToTop(
        new (aBuilder) nsDisplayOwnLayer(aBuilder, aSourceFrame, aSource));
  } else {
    aDest->AppendToTop(aSource);
  }  
}

nsresult
nsGfxScrollFrameInner::AppendScrollPartsTo(nsDisplayListBuilder*          aBuilder,
                                           const nsRect&                  aDirtyRect,
                                           const nsDisplayListSet&        aLists,
                                           const nsDisplayListCollection& aDest,
                                           PRBool&                        aCreateLayer)
{
  nsresult rv = NS_OK;
  PRBool hasResizer = HasResizer();
  for (nsIFrame* kid = mOuter->GetFirstChild(nsnull); kid; kid = kid->GetNextSibling()) {
    if (kid != mScrolledFrame) {
      if (kid == mResizerBox && hasResizer) {
        
        continue;
      }
      rv = mOuter->BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aDest,
                                            nsIFrame::DISPLAY_CHILD_FORCE_STACKING_CONTEXT);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      ::AppendToTop(aBuilder, aLists.BorderBackground(),
                    aDest.PositionedDescendants(), kid,
                    aCreateLayer);
    }
  }
  return rv;
}

PRBool
nsGfxScrollFrameInner::ShouldBuildLayer() const
{
  return mShouldBuildLayer;
}

nsresult
nsGfxScrollFrameInner::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  nsresult rv = mOuter->DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aBuilder->IsPaintingToWindow()) {
    mScrollPosAtLastPaint = GetScrollPosition();
    if (IsScrollingActive() && !CanScrollWithBlitting(mOuter)) {
      MarkInactive();
    }
  }

  if (aBuilder->GetIgnoreScrollFrame() == mOuter) {
    
    
    
    return mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame,
                                            aDirtyRect, aLists);
  }

  
  
  
  
  
  
  
  
  PRBool createLayersForScrollbars = mIsRoot &&
    mOuter->PresContext()->IsRootContentDocument();

  nsDisplayListCollection scrollParts;
  if (!mScrollbarsCanOverlapContent) {
    
    
    
    
    AppendScrollPartsTo(aBuilder, aDirtyRect, aLists,
                        scrollParts, createLayersForScrollbars);
  }

  
  
  
  nsRect dirtyRect;
  
  
  
  
  
  dirtyRect.IntersectRect(aDirtyRect, mScrollPort);

  
  PRBool usingDisplayport =
    nsLayoutUtils::GetDisplayPort(mOuter->GetContent(), &dirtyRect);

  nsDisplayListCollection set;

  nsPresContext* presContext = mOuter->PresContext();

  
  
  
  
  
  
  
  
  
  nsRect scrollRange = GetScrollRange();
  ScrollbarStyles styles = GetScrollbarStylesFromFrame();
  mShouldBuildLayer =
     (XRE_GetProcessType() == GeckoProcessType_Content &&
     (styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN ||
      styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN) &&
     (scrollRange.width > 0 ||
      scrollRange.height > 0) &&
     (!mIsRoot || !mOuter->PresContext()->IsRootContentDocument()));

  if (ShouldBuildLayer()) {
    nsDisplayList list;
    if (usingDisplayport) {
      
      
      
      
      
      

      rv = mScrolledFrame->BuildDisplayListForStackingContext(
        aBuilder, dirtyRect + mOuter->GetOffsetTo(mScrolledFrame), &list);

      nsDisplayScrollLayer* layerItem = new (aBuilder) nsDisplayScrollLayer(
        aBuilder, &list, mScrolledFrame, mOuter);
      set.Content()->AppendNewToTop(layerItem);
    } else {
      
      
      

      nsDisplayScrollLayer* layerItem = new (aBuilder) nsDisplayScrollInfoLayer(
        aBuilder, &list, mScrolledFrame, mOuter);
      set.Content()->AppendNewToTop(layerItem);

      rv = mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame, dirtyRect, set);
    }
  } else {
    rv = mOuter->BuildDisplayListForChild(aBuilder, mScrolledFrame, dirtyRect, set);
  }

  NS_ENSURE_SUCCESS(rv, rv);
  nsRect clip;
  clip = mScrollPort + aBuilder->ToReferenceFrame(mOuter);

  nscoord radii[8];
  
  
  mOuter->GetPaddingBoxBorderRadii(radii);

  
  
  
  
  
  rv = mOuter->OverflowClip(aBuilder, set, aLists, clip, radii,
                            PR_TRUE, mIsRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mScrollbarsCanOverlapContent) {
    AppendScrollPartsTo(aBuilder, aDirtyRect, aLists,
                       scrollParts, createLayersForScrollbars);
  }

  if (HasResizer()) {
    rv = mOuter->BuildDisplayListForChild(aBuilder, mResizerBox, aDirtyRect, scrollParts,
                                          nsIFrame::DISPLAY_CHILD_FORCE_STACKING_CONTEXT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    
    ::AppendToTop(aBuilder,
                  mIsRoot ? aLists.PositionedDescendants() : aLists.Content(),
                  scrollParts.PositionedDescendants(), mResizerBox,
                  createLayersForScrollbars);
  }

  return NS_OK;
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
      
      
      
      
      return result;
    }
  } else {
    const nsStyleDisplay *disp = mOuter->GetStyleDisplay();
    result.mHorizontal = disp->mOverflowX;
    result.mVertical = disp->mOverflowY;
  }

  return result;
}

static nscoord
AlignToDevPixelRoundingToZero(nscoord aVal, PRInt32 aAppUnitsPerDevPixel)
{
  return (aVal/aAppUnitsPerDevPixel)*aAppUnitsPerDevPixel;
}

nsRect
nsGfxScrollFrameInner::GetScrollRange() const
{
  nsRect range = GetScrolledRect();
  range.width -= mScrollPort.width;
  range.height -= mScrollPort.height;

  nsPresContext* presContext = mOuter->PresContext();
  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  range.width =
    AlignToDevPixelRoundingToZero(range.XMost(), appUnitsPerDevPixel) - range.x;
  range.height =
    AlignToDevPixelRoundingToZero(range.YMost(), appUnitsPerDevPixel) - range.y;
  range.x = AlignToDevPixelRoundingToZero(range.x, appUnitsPerDevPixel);
  range.y = AlignToDevPixelRoundingToZero(range.y, appUnitsPerDevPixel);
  return range;
}

static void
AdjustForWholeDelta(PRInt32 aDelta, nscoord* aCoord)
{
  if (aDelta < 0) {
    *aCoord = nscoord_MIN;
  } else if (aDelta > 0) {
    *aCoord = nscoord_MAX;
  }
}

void
nsGfxScrollFrameInner::ScrollBy(nsIntPoint aDelta,
                                nsIScrollableFrame::ScrollUnit aUnit,
                                nsIScrollableFrame::ScrollMode aMode,
                                nsIntPoint* aOverflow)
{
  nsSize deltaMultiplier;
  switch (aUnit) {
  case nsIScrollableFrame::DEVICE_PIXELS: {
    nscoord appUnitsPerDevPixel =
      mOuter->PresContext()->AppUnitsPerDevPixel();
    deltaMultiplier = nsSize(appUnitsPerDevPixel, appUnitsPerDevPixel);
    break;
  }
  case nsIScrollableFrame::LINES: {
    deltaMultiplier = GetLineScrollAmount();
    break;
  }
  case nsIScrollableFrame::PAGES: {
    deltaMultiplier = GetPageScrollAmount();
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
  ScrollTo(newPos, aMode);

  if (aOverflow) {
    nsPoint clampAmount = mDestination - newPos;
    float appUnitsPerDevPixel = mOuter->PresContext()->AppUnitsPerDevPixel();
    *aOverflow = nsIntPoint(
        NSAppUnitsToIntPixels(PR_ABS(clampAmount.x), appUnitsPerDevPixel),
        NSAppUnitsToIntPixels(PR_ABS(clampAmount.y), appUnitsPerDevPixel));
  }
}

nsSize
nsGfxScrollFrameInner::GetLineScrollAmount() const
{
  const nsStyleFont* font = mOuter->GetStyleFont();
  const nsFont& f = font->mFont;
  nsCOMPtr<nsIFontMetrics> fm = mOuter->PresContext()->GetMetricsFor(f);
  NS_ASSERTION(fm, "FontMetrics is null, assuming fontHeight == 1 appunit");
  nscoord fontHeight = 1;
  if (fm) {
    fm->GetHeight(fontHeight);
  }

  return nsSize(fontHeight, fontHeight);
}

nsSize
nsGfxScrollFrameInner::GetPageScrollAmount() const
{
  nsSize lineScrollAmount = GetLineScrollAmount();
  
  
  return nsSize(
    mScrollPort.width - NS_MIN(mScrollPort.width/10, 2*lineScrollAmount.width),
    mScrollPort.height - NS_MIN(mScrollPort.height/10, 2*lineScrollAmount.height));
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
      ScrollTo(scrollToPos, nsIScrollableFrame::INSTANT);
      
      
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
  mResizerBox = nsnull;

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
      } else if (content->Tag() == nsGkAtoms::resizer) {
        NS_ASSERTION(!mResizerBox, "Found multiple resizers");
        mResizerBox = frame;
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

  
  
  
  
  
  if (presContext->Document()->IsBeingUsedAsImage() ||
      (!presContext->IsDynamic() &&
       !(mIsRoot && presContext->HasPaginatedScrolling()))) {
    mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = PR_TRUE;
    return NS_OK;
  }

  
  PRInt8 resizeStyle = mOuter->GetStyleDisplay()->mResize;
  PRBool isResizable = resizeStyle != NS_STYLE_RESIZE_NONE;

  nsIScrollableFrame *scrollable = do_QueryFrame(mOuter);

  
  
  
  
  
  PRBool canHaveHorizontal;
  PRBool canHaveVertical;
  if (!mIsRoot) {
    ScrollbarStyles styles = scrollable->GetScrollbarStyles();
    canHaveHorizontal = styles.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN;
    canHaveVertical = styles.mVertical != NS_STYLE_OVERFLOW_HIDDEN;
    if (!canHaveHorizontal && !canHaveVertical && !isResizable) {
      
      return NS_OK;
    }
  } else {
    canHaveHorizontal = PR_TRUE;
    canHaveVertical = PR_TRUE;
  }

  
  nsITextControlFrame* textFrame = do_QueryFrame(parent);
  if (textFrame) {
    
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textAreaElement(do_QueryInterface(parent->GetContent()));
    if (!textAreaElement) {
      mNeverHasVerticalScrollbar = mNeverHasHorizontalScrollbar = PR_TRUE;
      return NS_OK;
    }
  }

  nsNodeInfoManager *nodeInfoManager =
    presContext->Document()->NodeInfoManager();
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollbar, nsnull,
                                          kNameSpaceID_XUL);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  if (canHaveHorizontal) {
    nsCOMPtr<nsINodeInfo> ni = nodeInfo;
    NS_TrustedNewXULElement(getter_AddRefs(mHScrollbarContent), ni.forget());
    mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("horizontal"), PR_FALSE);
    mHScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                NS_LITERAL_STRING("always"), PR_FALSE);
    if (!aElements.AppendElement(mHScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveVertical) {
    nsCOMPtr<nsINodeInfo> ni = nodeInfo;
    NS_TrustedNewXULElement(getter_AddRefs(mVScrollbarContent), ni.forget());
    mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::orient,
                                NS_LITERAL_STRING("vertical"), PR_FALSE);
    mVScrollbarContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                NS_LITERAL_STRING("always"), PR_FALSE);
    if (!aElements.AppendElement(mVScrollbarContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (isResizable) {
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::resizer, nsnull,
                                            kNameSpaceID_XUL);
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
    mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, dir, PR_FALSE);

    if (mIsRoot) {
      nsIContent* browserRoot = GetBrowserRoot(mOuter->GetContent());
      mCollapsedResizer = !(browserRoot &&
                            browserRoot->HasAttr(kNameSpaceID_None, nsGkAtoms::showresizer));
    }
    else {
      mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::element,
                                    NS_LITERAL_STRING("_parent"), PR_FALSE);
    }

    mResizerContent->SetAttr(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                  NS_LITERAL_STRING("always"), PR_FALSE);

    if (!aElements.AppendElement(mResizerContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  if (canHaveHorizontal && canHaveVertical) {
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::scrollcorner, nsnull,
                                            kNameSpaceID_XUL);
    NS_TrustedNewXULElement(getter_AddRefs(mScrollCornerContent), nodeInfo.forget());
    if (!aElements.AppendElement(mScrollCornerContent))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsGfxScrollFrameInner::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                                PRUint32 aFilter)
{
  aElements.MaybeAppendElement(mHScrollbarContent);
  aElements.MaybeAppendElement(mVScrollbarContent);
  aElements.MaybeAppendElement(mScrollCornerContent);
  aElements.MaybeAppendElement(mResizerContent);
}

void
nsGfxScrollFrameInner::Destroy()
{
  
  nsContentUtils::DestroyAnonymousContent(&mHScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mVScrollbarContent);
  nsContentUtils::DestroyAnonymousContent(&mScrollCornerContent);
  nsContentUtils::DestroyAnonymousContent(&mResizerContent);

  if (mPostedReflowCallback) {
    mOuter->PresContext()->PresShell()->CancelReflowCallback(this);
    mPostedReflowCallback = PR_FALSE;
  }
}







void
nsGfxScrollFrameInner::UpdateScrollbarPosition()
{
  mFrameIsUpdatingScrollbar = PR_TRUE;

  nsPoint pt = GetScrollPosition();
  if (mVScrollbarBox) {
    SetCoordAttribute(mVScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      pt.y - GetScrolledRect().y);
  }
  if (mHScrollbarBox) {
    SetCoordAttribute(mHScrollbarBox->GetContent(), nsGkAtoms::curpos,
                      pt.x - GetScrolledRect().x);
  }

  mFrameIsUpdatingScrollbar = PR_FALSE;
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

  nsPoint dest;
  dest.x = GetCoordAttribute(mHScrollbarBox, nsGkAtoms::curpos,
                             -scrolledRect.x) +
           scrolledRect.x;
  dest.y = GetCoordAttribute(mVScrollbarBox, nsGkAtoms::curpos,
                             -scrolledRect.y) +
           scrolledRect.y;

  
  if (mAsyncScroll && dest == GetScrollPosition()) {
    return;
  }

  PRBool isSmooth = aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::smooth);
  if (isSmooth) {
    
    
    
    
    UpdateScrollbarPosition();
  }
  ScrollTo(dest,
           isSmooth ? nsIScrollableFrame::SMOOTH : nsIScrollableFrame::INSTANT);
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
nsXULScrollFrame::AddHorizontalScrollbar(nsBoxLayoutState& aState, PRBool aOnBottom)
{
  if (!mInner.mHScrollbarBox)
    return PR_TRUE;

  return AddRemoveScrollbar(aState, aOnBottom, PR_TRUE, PR_TRUE);
}

PRBool
nsXULScrollFrame::AddVerticalScrollbar(nsBoxLayoutState& aState, PRBool aOnRight)
{
  if (!mInner.mVScrollbarBox)
    return PR_TRUE;

  return AddRemoveScrollbar(aState, aOnRight, PR_FALSE, PR_TRUE);
}

void
nsXULScrollFrame::RemoveHorizontalScrollbar(nsBoxLayoutState& aState, PRBool aOnBottom)
{
  
#ifdef DEBUG
  PRBool result =
#endif
  AddRemoveScrollbar(aState, aOnBottom, PR_TRUE, PR_FALSE);
  NS_ASSERTION(result, "Removing horizontal scrollbar failed to fit??");
}

void
nsXULScrollFrame::RemoveVerticalScrollbar(nsBoxLayoutState& aState, PRBool aOnRight)
{
  
#ifdef DEBUG
  PRBool result =
#endif
  AddRemoveScrollbar(aState, aOnRight, PR_FALSE, PR_FALSE);
  NS_ASSERTION(result, "Removing vertical scrollbar failed to fit??");
}

PRBool
nsXULScrollFrame::AddRemoveScrollbar(nsBoxLayoutState& aState,
                                     PRBool aOnRightOrBottom, PRBool aHorizontal, PRBool aAdd)
{
  if (aHorizontal) {
     if (mInner.mNeverHasHorizontalScrollbar || !mInner.mHScrollbarBox)
       return PR_FALSE;

     nsSize hSize = mInner.mHScrollbarBox->GetPrefSize(aState);
     nsBox::AddMargin(mInner.mHScrollbarBox, hSize);

     mInner.SetScrollbarVisibility(mInner.mHScrollbarBox, aAdd);

     PRBool hasHorizontalScrollbar;
     PRBool fit = AddRemoveScrollbar(hasHorizontalScrollbar,
                                     mInner.mScrollPort.y,
                                     mInner.mScrollPort.height,
                                     hSize.height, aOnRightOrBottom, aAdd);
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
     PRBool fit = AddRemoveScrollbar(hasVerticalScrollbar,
                                     mInner.mScrollPort.x,
                                     mInner.mScrollPort.width,
                                     vSize.width, aOnRightOrBottom, aAdd);
     mInner.mHasVerticalScrollbar = hasVerticalScrollbar;    
     if (!fit)
        mInner.SetScrollbarVisibility(mInner.mVScrollbarBox, !aAdd);

     return fit;
  }
}

PRBool
nsXULScrollFrame::AddRemoveScrollbar(PRBool& aHasScrollbar, nscoord& aXY,
                                     nscoord& aSize, nscoord aSbSize,
                                     PRBool aOnRightOrBottom, PRBool aAdd)
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
       return PR_TRUE;
   }

   aHasScrollbar = PR_FALSE;
   return PR_FALSE;
}

void
nsXULScrollFrame::LayoutScrollArea(nsBoxLayoutState& aState,
                                   const nsPoint& aScrollPosition)
{
  PRUint32 oldflags = aState.LayoutFlags();
  nsRect childRect = nsRect(mInner.mScrollPort.TopLeft() - aScrollPosition,
                            mInner.mScrollPort.Size());
  PRInt32 flags = NS_FRAME_NO_MOVE_VIEW;

  nsRect originalRect = mInner.mScrolledFrame->GetRect();
  nsRect originalVisOverflow = mInner.mScrolledFrame->GetVisualOverflowRect();

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
    childRect.width = NS_MAX(childRect.width, mInner.mScrollPort.width);
    childRect.height = NS_MAX(childRect.height, mInner.mScrollPort.height);

    
    
    
    ClampAndSetBounds(aState, childRect, aScrollPosition, PR_TRUE);
  }

  nsRect finalRect = mInner.mScrolledFrame->GetRect();
  nsRect finalVisOverflow = mInner.mScrolledFrame->GetVisualOverflowRect();
  
  
  
  if (originalRect.TopLeft() != finalRect.TopLeft() ||
      originalVisOverflow.TopLeft() != finalVisOverflow.TopLeft())
  {
    
    
    mInner.mScrolledFrame->Invalidate(
      originalVisOverflow + originalRect.TopLeft() - finalRect.TopLeft());
    mInner.mScrolledFrame->Invalidate(finalVisOverflow);
  } else if (!originalVisOverflow.IsExactEqual(finalVisOverflow)) {
    
    
    mInner.mScrolledFrame->CheckInvalidateSizeChange(
      originalRect, originalVisOverflow, finalRect.Size());
    mInner.mScrolledFrame->InvalidateRectDifference(
      originalVisOverflow, finalVisOverflow);
  }

  aState.SetLayoutFlags(oldflags);

}

void nsGfxScrollFrameInner::PostOverflowEvent()
{
  if (mAsyncScrollPortEvent.IsPending())
    return;

  
  nsSize scrollportSize = mScrollPort.Size();
  nsSize childSize = GetScrolledRect().Size();

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

  return frame->GetStyleVisibility()->mDirection != NS_STYLE_DIRECTION_RTL;
}

PRBool
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

  nsRect oldScrollAreaBounds = mInner.mScrollPort;
  nsPoint oldScrollPosition = mInner.GetLogicalScrollPosition();

  
  mInner.mScrollPort = clientRect;

  
























  ScrollbarStyles styles = GetScrollbarStyles();

  
  if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasHorizontalScrollbar = PR_TRUE;
  if (styles.mVertical == NS_STYLE_OVERFLOW_SCROLL)
     mInner.mHasVerticalScrollbar = PR_TRUE;

  if (mInner.mHasHorizontalScrollbar)
     AddHorizontalScrollbar(aState, scrollbarBottom);

  if (mInner.mHasVerticalScrollbar)
     AddVerticalScrollbar(aState, scrollbarRight);
     
  
  LayoutScrollArea(aState, oldScrollPosition);
  
  
  PRBool needsLayout = PR_FALSE;

  
  if (styles.mVertical != NS_STYLE_OVERFLOW_SCROLL) {
    
    nsRect scrolledRect = mInner.GetScrolledRect();

    
      if (scrolledRect.height <= mInner.mScrollPort.height
          || styles.mVertical != NS_STYLE_OVERFLOW_AUTO) {
        if (mInner.mHasVerticalScrollbar) {
          
          
          RemoveVerticalScrollbar(aState, scrollbarRight);
          needsLayout = PR_TRUE;
        }
      } else {
        if (!mInner.mHasVerticalScrollbar) {
          
          
          if (AddVerticalScrollbar(aState, scrollbarRight))
            needsLayout = PR_TRUE;
        }
    }

    
    if (needsLayout) {
       nsBoxLayoutState resizeState(aState);
       LayoutScrollArea(resizeState, oldScrollPosition);
       needsLayout = PR_FALSE;
    }
  }


  
  if (styles.mHorizontal != NS_STYLE_OVERFLOW_SCROLL)
  {
    
    nsRect scrolledRect = mInner.GetScrolledRect();

    
    
    if ((scrolledRect.width > mInner.mScrollPort.width)
        && styles.mHorizontal == NS_STYLE_OVERFLOW_AUTO) {

      if (!mInner.mHasHorizontalScrollbar) {
           
          if (AddHorizontalScrollbar(aState, scrollbarBottom))
             needsLayout = PR_TRUE;

           
           
           
           
           
           
           
      }
    } else {
        
        
      if (mInner.mHasHorizontalScrollbar) {
        RemoveHorizontalScrollbar(aState, scrollbarBottom);
        needsLayout = PR_TRUE;
      }
    }
  }

  
  if (needsLayout) {
     nsBoxLayoutState resizeState(aState);
     LayoutScrollArea(resizeState, oldScrollPosition);
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
    RemoveHorizontalScrollbar(aState, scrollbarBottom);
    needsLayout = PR_TRUE;
  }
  
  if (mInner.mHasVerticalScrollbar &&
      (vMinSize.height > clientRect.height - hMinSize.height
       || vMinSize.width > clientRect.width)) {
    RemoveVerticalScrollbar(aState, scrollbarRight);
    needsLayout = PR_TRUE;
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

  
  ScrollToImpl(GetScrollPosition());

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

  nsRect scrolledContentRect = GetScrolledRect();
  nscoord minX = scrolledContentRect.x;
  nscoord maxX = scrolledContentRect.XMost() - mScrollPort.width;
  nscoord minY = scrolledContentRect.y;
  nscoord maxY = scrolledContentRect.YMost() - mScrollPort.height;

  
  NS_ASSERTION(!mFrameIsUpdatingScrollbar, "We shouldn't be reentering here");
  mFrameIsUpdatingScrollbar = PR_TRUE;

  nsCOMPtr<nsIContent> vScroll =
    mVScrollbarBox ? mVScrollbarBox->GetContent() : nsnull;
  nsCOMPtr<nsIContent> hScroll =
    mHScrollbarBox ? mHScrollbarBox->GetContent() : nsnull;

  
  
  if (vScroll || hScroll) {
    nsWeakFrame weakFrame(mOuter);
    nsPoint scrollPos = GetScrollPosition();
    
    if (vScroll) {
      nscoord fontHeight = GetLineScrollAmount().height;
      
      
      
      
      
      
      nscoord pageincrement = nscoord(mScrollPort.height - fontHeight);
      nscoord pageincrementMin = nscoord(float(mScrollPort.height) * 0.8);
      FinishReflowForScrollbar(vScroll, minY, maxY, scrollPos.y,
                               NS_MAX(pageincrement,pageincrementMin),
                               fontHeight);
    }
    if (hScroll) {
      FinishReflowForScrollbar(hScroll, minX, maxX, scrollPos.x,
                               nscoord(float(mScrollPort.width) * 0.8),
                               nsPresContext::CSSPixelsToAppUnits(10));
    }
    NS_ENSURE_TRUE(weakFrame.IsAlive(), PR_FALSE);
  }

  mFrameIsUpdatingScrollbar = PR_FALSE;
  
  
  
  
  
  
  
  
  
  
  
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
                                nsIFrame* aBox, const nsRect& aRect,
                                PRBool aScrollbarIsBeingHidden)
{
  
  
  
  
  
  
  
  
  
  
  PRBool rectChanged = aBox->GetRect() != aRect;
  if (rectChanged) {
    if (aScrollbarIsBeingHidden) {
      aBox->GetParent()->Invalidate(aBox->GetVisualOverflowRect() +
                                    aBox->GetPosition());
    } else {
      aBox->InvalidateFrameSubtree();
    }
  }
  nsBoxFrame::LayoutChildAt(aState, aBox, aRect);
  if (rectChanged) {
    if (aScrollbarIsBeingHidden) {
      aBox->GetParent()->Invalidate(aBox->GetVisualOverflowRect() +
                                    aBox->GetPosition());
    } else {
      aBox->InvalidateFrameSubtree();
    }
  }
}

void
nsGfxScrollFrameInner::AdjustScrollbarRectForResizer(
                         nsIFrame* aFrame, nsPresContext* aPresContext,
                         nsRect& aRect, PRBool aHasResizer, PRBool aVertical)
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
    aRect.height = NS_MAX(0, resizerRect.y - aRect.y);
  else
    aRect.width = NS_MAX(0, resizerRect.x - aRect.x);
}

void
nsGfxScrollFrameInner::LayoutScrollbars(nsBoxLayoutState& aState,
                                        const nsRect& aContentArea,
                                        const nsRect& aOldScrollArea)
{
  NS_ASSERTION(!mSupppressScrollbarUpdate,
               "This should have been suppressed");

  PRBool hasResizer = HasResizer();
  PRBool scrollbarOnLeft = !IsScrollbarOnRight();

  
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
      LayoutAndInvalidate(aState, mScrollCornerBox, r, PR_FALSE);
    }

    if (hasResizer) {
      
      nsSize resizerSize;
      nscoord defaultSize = nsPresContext::CSSPixelsToAppUnits(15);
      resizerSize.width =
        mVScrollbarBox ? mVScrollbarBox->GetMinSize(aState).width : defaultSize;
      if (resizerSize.width > r.width) {
        r.width = resizerSize.width;
        if (aContentArea.x == mScrollPort.x && !scrollbarOnLeft)
          r.x = aContentArea.XMost() - r.width;
      }

      resizerSize.height =
        mHScrollbarBox ? mHScrollbarBox->GetMinSize(aState).height : defaultSize;
      if (resizerSize.height > r.height) {
        r.height = resizerSize.height;
        if (aContentArea.y == mScrollPort.y)
          r.y = aContentArea.YMost() - r.height;
      }

      LayoutAndInvalidate(aState, mResizerBox, r, PR_FALSE);
    }
    else if (mResizerBox) {
      
      LayoutAndInvalidate(aState, mResizerBox, nsRect(), PR_FALSE);
    }
  }

  nsPresContext* presContext = mScrolledFrame->PresContext();
  if (mVScrollbarBox) {
    NS_PRECONDITION(mVScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    nsRect vRect(mScrollPort);
    vRect.width = aContentArea.width - mScrollPort.width;
    vRect.x = scrollbarOnLeft ? aContentArea.x : mScrollPort.XMost();
    nsMargin margin;
    mVScrollbarBox->GetMargin(margin);
    vRect.Deflate(margin);
    AdjustScrollbarRectForResizer(mOuter, presContext, vRect, hasResizer, PR_TRUE);
    LayoutAndInvalidate(aState, mVScrollbarBox, vRect, !mHasVerticalScrollbar);
  }

  if (mHScrollbarBox) {
    NS_PRECONDITION(mHScrollbarBox->IsBoxFrame(), "Must be a box frame!");
    nsRect hRect(mScrollPort);
    hRect.height = aContentArea.height - mScrollPort.height;
    hRect.y = PR_TRUE ? mScrollPort.YMost() : aContentArea.y;
    nsMargin margin;
    mHScrollbarBox->GetMargin(margin);
    hRect.Deflate(margin);
    AdjustScrollbarRectForResizer(mOuter, presContext, hRect, hasResizer, PR_FALSE);
    LayoutAndInvalidate(aState, mHScrollbarBox, hRect, !mHasHorizontalScrollbar);
  }

  
  
  
  
  if (aOldScrollArea.Size() != mScrollPort.Size() && 
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

static void
ReduceRadii(nscoord aXBorder, nscoord aYBorder,
            nscoord& aXRadius, nscoord& aYRadius)
{
  
  
  if (aXRadius <= aXBorder || aYRadius <= aYBorder)
    return;

  
  double ratio = NS_MAX(double(aXBorder) / aXRadius,
                        double(aYBorder) / aYRadius);
  aXRadius *= ratio;
  aYRadius *= ratio;
}









PRBool
nsGfxScrollFrameInner::GetBorderRadii(nscoord aRadii[8]) const
{
  if (!mOuter->nsContainerFrame::GetBorderRadii(aRadii))
    return PR_FALSE;

  
  
  
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

  return PR_TRUE;
}

nsRect
nsGfxScrollFrameInner::GetScrolledRect() const
{
  nsRect result =
    GetScrolledRectInternal(mScrolledFrame->GetScrollableOverflowRect(),
                            mScrollPort.Size());

  NS_ASSERTION(result.width >= mScrollPort.width,
               "Scrolled rect smaller than scrollport?");
  NS_ASSERTION(result.height >= mScrollPort.height,
               "Scrolled rect smaller than scrollport?");
  return result;
}

nsRect
nsGfxScrollFrameInner::GetScrolledRectInternal(const nsRect& aScrolledFrameOverflowArea,
                                               const nsSize& aScrollPortSize) const
{
  nscoord x1 = aScrolledFrameOverflowArea.x,
          x2 = aScrolledFrameOverflowArea.XMost(),
          y1 = aScrolledFrameOverflowArea.y,
          y2 = aScrolledFrameOverflowArea.YMost();
  if (y1 < 0)
    y1 = 0;
  if (IsLTR()) {
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
nsGfxScrollFrameInner::GetActualScrollbarSizes() const
{
  nsRect r = mOuter->GetPaddingRect() - mOuter->GetPosition();

  return nsMargin(mScrollPort.x - r.x, mScrollPort.y - r.y,
                  r.XMost() - mScrollPort.XMost(),
                  r.YMost() - mScrollPort.YMost());
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

  nsPoint scrollPos = GetLogicalScrollPosition();
  
  if (scrollPos == nsPoint(0,0)) {
    return nsnull;
  }

  nsPresState* state = new nsPresState();
  if (!state) {
    return nsnull;
  }
 
  state->SetScrollState(scrollPos);

  return state;
}

void
nsGfxScrollFrameInner::RestoreState(nsPresState* aState)
{
  mRestorePos = aState->GetScrollState();
  mLastPos.x = -1;
  mLastPos.y = -1;
  mDidHistoryRestore = PR_TRUE;
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

  nsScrollAreaEvent event(PR_TRUE, NS_SCROLLEDAREACHANGED, nsnull);
  nsPresContext *prescontext = mOuter->PresContext();
  nsIContent* content = mOuter->GetContent();

  event.mArea = mScrolledFrame->GetScrollableOverflowRectRelativeToParent();

  nsIDocument *doc = content->GetCurrentDoc();
  if (doc) {
    nsEventDispatcher::Dispatch(doc, prescontext, &event, nsnull);
  }
}
