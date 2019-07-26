



#include "nsPageContentFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsSimplePageSequenceFrame.h"

nsPageContentFrame*
NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPageContentFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageContentFrame)

void
nsPageContentFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageContentFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;  

  if (GetPrevInFlow() && (GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsresult rv = aPresContext->PresShell()->FrameConstructor()
                    ->ReplicateFixedFrames(this);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  
  
  
  nsSize  maxSize(aReflowState.ComputedWidth(),
                  aReflowState.ComputedHeight());
  SetSize(maxSize);
 
  
  
  
  if (mFrames.NotEmpty()) {
    nsIFrame* frame = mFrames.FirstChild();
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame, maxSize);
    kidReflowState.SetComputedHeight(maxSize.height);

    
    ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);

    
    
    
    nsMargin padding(0,0,0,0);

    
    
    kidReflowState.mStylePadding->GetPadding(padding);

    
    
    
    
    if (frame->HasOverflowAreas()) {
      
      
      nscoord xmost = aDesiredSize.ScrollableOverflow().XMost();
      if (xmost > aDesiredSize.Width()) {
        nscoord widthToFit = xmost + padding.right +
          kidReflowState.mStyleBorder->GetComputedBorderWidth(NS_SIDE_RIGHT);
        float ratio = float(maxSize.width) / widthToFit;
        NS_ASSERTION(ratio >= 0.0 && ratio < 1.0, "invalid shrink-to-fit ratio");
        mPD->mShrinkToFitRatio = std::min(mPD->mShrinkToFitRatio, ratio);
      }
    }

    
    FinishReflowChild(frame, aPresContext, aDesiredSize, &kidReflowState, 0, 0, 0);

    NS_ASSERTION(aPresContext->IsDynamic() || !NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                  !frame->GetNextInFlow(), "bad child flow list");
  }

  
  nsReflowStatus fixedStatus = NS_FRAME_COMPLETE;
  ReflowAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, fixedStatus);
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(fixedStatus), "fixed frames can be truncated, but not incomplete");

  
  aDesiredSize.Width() = aReflowState.ComputedWidth();
  if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
    aDesiredSize.Height() = aReflowState.ComputedHeight();
  }

  FinishAndStoreOverflow(&aDesiredSize);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
nsPageContentFrame::GetType() const
{
  return nsGkAtoms::pageContentFrame; 
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsPageContentFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("PageContent"), aResult);
}
#endif
