



































#include "nsPageContentFrame.h"
#include "nsPageFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIDeviceContext.h"
#include "nsReadableUtils.h"
#include "nsSimplePageSequence.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPageContentFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageContentFrame)

 nsSize
nsPageContentFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                                nsSize aCBSize, nscoord aAvailableWidth,
                                nsSize aMargin, nsSize aBorder, nsSize aPadding,
                                PRBool aShrinkWrap)
{
  NS_ASSERTION(mPD, "Pages are supposed to have page data");
  nscoord height = (!mPD || mPD->mReflowSize.height == NS_UNCONSTRAINEDSIZE)
                   ? NS_UNCONSTRAINEDSIZE
                   : (mPD->mReflowSize.height - mPD->mReflowMargin.TopBottom());
  return nsSize(aAvailableWidth, height);
}

NS_IMETHODIMP
nsPageContentFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageContentFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;  
  nsresult rv = NS_OK;

  if (GetPrevInFlow() && (GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsresult rv = aPresContext->PresShell()->FrameConstructor()
                    ->ReplicateFixedFrames(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  if (mFrames.NotEmpty()) {
    nsIFrame* frame = mFrames.FirstChild();
    nsSize  maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame, maxSize);
    kidReflowState.SetComputedHeight(aReflowState.availableHeight);

    mPD->mPageContentSize  = aReflowState.availableWidth;

    
    rv = ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    nsMargin padding(0,0,0,0);

    
    
    kidReflowState.mStylePadding->GetPadding(padding);

    
    if (frame->HasOverflowRect()) {
      
      
      if (aDesiredSize.mOverflowArea.XMost() > aDesiredSize.width) {
        mPD->mPageContentXMost =
          aDesiredSize.mOverflowArea.XMost() +
          kidReflowState.mStyleBorder->GetActualBorderWidth(NS_SIDE_RIGHT) +
          padding.right;
      }
    }

    
    FinishReflowChild(frame, aPresContext, &kidReflowState, aDesiredSize, 0, 0, 0);

    NS_ASSERTION(aPresContext->IsDynamic() || !NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                  !frame->GetNextInFlow(), "bad child flow list");
  }
  
  nsReflowStatus fixedStatus = NS_FRAME_COMPLETE;
  mFixedContainer.Reflow(this, aPresContext, aReflowState, fixedStatus,
                         aReflowState.availableWidth,
                         aReflowState.availableHeight,
                         PR_FALSE, PR_TRUE, PR_TRUE); 
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(fixedStatus), "fixed frames can be truncated, but not incomplete");

  
  aDesiredSize.width = aReflowState.availableWidth;
  if (aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE) {
    aDesiredSize.height = aReflowState.availableHeight;
  }

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

nsIAtom*
nsPageContentFrame::GetType() const
{
  return nsGkAtoms::pageContentFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsPageContentFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("PageContent"), aResult);
}
#endif

 PRBool
nsPageContentFrame::IsContainingBlock() const
{
  return PR_TRUE;
}
