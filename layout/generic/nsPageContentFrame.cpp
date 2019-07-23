



































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




inline PRBool
nsPageContentFrame::IsFixedPlaceholder(nsIFrame* aFrame)
{
  if (!aFrame || nsGkAtoms::placeholderFrame != aFrame->GetType())
    return PR_FALSE;

  return static_cast<nsPlaceholderFrame*>(aFrame)->GetOutOfFlowFrame()
           ->GetParent() == this;
}





inline nsFrameList
nsPageContentFrame::StealFixedPlaceholders(nsIFrame* aDocRoot)
{
  nsPresContext* presContext = PresContext();
  nsFrameList list;
  if (GetPrevInFlow()) {
    for (nsIFrame* f = aDocRoot->GetFirstChild(nsnull);
        IsFixedPlaceholder(f); f = aDocRoot->GetFirstChild(nsnull)) {
      nsresult rv = static_cast<nsContainerFrame*>(aDocRoot)
                      ->StealFrame(presContext, f);
      NS_ENSURE_SUCCESS(rv, list);
      list.AppendFrame(nsnull, f);
    }
  }
  return list;
}




static inline nsresult
ReplaceFixedPlaceholders(nsIFrame*    aDocRoot,
                         nsFrameList& aPlaceholderList)
{
  nsresult rv = NS_OK;
  if (aPlaceholderList.NotEmpty()) {
    rv = static_cast<nsContainerFrame*>(aDocRoot)
           ->AddFrames(aPlaceholderList.FirstChild(), nsnull);
  }
  return rv;
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

  
  
  
  nsPageContentFrame* prevPageContentFrame = static_cast<nsPageContentFrame*>
                                               (GetPrevInFlow());
  if (mFrames.IsEmpty() && prevPageContentFrame) {
    
    nsIFrame* overflow = prevPageContentFrame->GetOverflowFrames(aPresContext, PR_TRUE);
    NS_ASSERTION(overflow && !overflow->GetNextSibling(),
                 "must have doc root as pageContentFrame's only child");
    nsHTMLContainerFrame::ReparentFrameView(aPresContext, overflow, prevPageContentFrame, this);
    
    
    
    mFrames.InsertFrames(this, nsnull, overflow);
    nsresult rv = aPresContext->PresShell()->FrameConstructor()
                    ->ReplicateFixedFrames(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (mFrames.NotEmpty()) {
    nsIFrame* frame = mFrames.FirstChild();
    nsSize  maxSize(aReflowState.availableWidth, aReflowState.availableHeight);
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame, maxSize);

    mPD->mPageContentSize  = aReflowState.availableWidth;

    
    nsFrameList stolenPlaceholders = StealFixedPlaceholders(frame);

    
    rv = ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = ReplaceFixedPlaceholders(frame, stolenPlaceholders);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
      nsIFrame* nextFrame = frame->GetNextInFlow();
      NS_ASSERTION(nextFrame || aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
        "If it's incomplete and has no nif yet, it must flag a nif reflow.");
      if (!nextFrame) {
        nsresult rv = nsHTMLContainerFrame::CreateNextInFlow(aPresContext,
                                              this, frame, nextFrame);
        NS_ENSURE_SUCCESS(rv, rv);
        frame->SetNextSibling(nextFrame->GetNextSibling());
        nextFrame->SetNextSibling(nsnull);
        SetOverflowFrames(aPresContext, nextFrame);
        
        
        
        
      }
      if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
        nextFrame->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      }
    }

    
    
    
    nsMargin padding(0,0,0,0);

    
    
    kidReflowState.mStylePadding->GetPadding(padding);

    
    if (NS_FRAME_OUTSIDE_CHILDREN & frame->GetStateBits()) {
      
      
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
