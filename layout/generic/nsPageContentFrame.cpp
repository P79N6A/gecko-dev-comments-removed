



































#include "nsPageContentFrame.h"
#include "nsPageFrame.h"
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

NS_IMETHODIMP
nsPageContentFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageContentFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;  

  
  
  
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

    
    ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);

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
        NS_FRAME_SET_INCOMPLETE(aStatus); 
      }
    }

    
    
    
    nsMargin padding(0,0,0,0);

    
    
    kidReflowState.mStylePadding->GetPadding(padding);

    
    if (NS_FRAME_OUTSIDE_CHILDREN & frame->GetStateBits()) {
      
      
      if (aDesiredSize.mOverflowArea.XMost() > aDesiredSize.width) {
        mPD->mPageContentXMost =
          aDesiredSize.mOverflowArea.XMost() +
          kidReflowState.mStyleBorder->GetBorderWidth(NS_SIDE_RIGHT) +
          padding.right;
      }
    }

    
    FinishReflowChild(frame, aPresContext, &kidReflowState, aDesiredSize, 0, 0, 0);

    NS_ASSERTION(aPresContext->IsDynamic() || !NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                  !frame->GetNextInFlow(), "bad child flow list");
  }
  
  mFixedContainer.Reflow(this, aPresContext, aReflowState,
                         aReflowState.availableWidth,
                         aReflowState.availableHeight,
                         PR_TRUE, PR_TRUE); 

  
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



NS_IMETHODIMP
nsPageContentFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;
  nsresult rv = ViewportFrame::BuildDisplayList(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);

  return Clip(aBuilder, set, aLists,
              nsRect(aBuilder->ToReferenceFrame(this), GetSize()));
}
