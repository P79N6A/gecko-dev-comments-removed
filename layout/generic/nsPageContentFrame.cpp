



#include "nsPageContentFrame.h"
#include "nsPageFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsReadableUtils.h"
#include "nsSimplePageSequence.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewPageContentFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPageContentFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageContentFrame)

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

  
  
  
  nsSize  maxSize(aReflowState.ComputedWidth(),
                  aReflowState.ComputedHeight());
  SetSize(maxSize);
 
  
  
  
  if (mFrames.NotEmpty()) {
    nsIFrame* frame = mFrames.FirstChild();
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame, maxSize);
    kidReflowState.SetComputedHeight(maxSize.height);

    mPD->mPageContentSize = maxSize.width;

    
    rv = ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, 0, 0, 0, aStatus);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    nsMargin padding(0,0,0,0);

    
    
    kidReflowState.mStylePadding->GetPadding(padding);

    
    
    
    
    if (frame->HasOverflowAreas()) {
      
      
      nscoord xmost = aDesiredSize.ScrollableOverflow().XMost();
      if (xmost > aDesiredSize.width) {
        mPD->mPageContentXMost =
          xmost +
          kidReflowState.mStyleBorder->GetComputedBorderWidth(NS_SIDE_RIGHT) +
          padding.right;
      }
    }

    
    FinishReflowChild(frame, aPresContext, &kidReflowState, aDesiredSize, 0, 0, 0);

    NS_ASSERTION(aPresContext->IsDynamic() || !NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                  !frame->GetNextInFlow(), "bad child flow list");
  }

  
  nsReflowStatus fixedStatus = NS_FRAME_COMPLETE;
  ReflowAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, fixedStatus);
  NS_ASSERTION(NS_FRAME_IS_COMPLETE(fixedStatus), "fixed frames can be truncated, but not incomplete");

  
  aDesiredSize.width = aReflowState.ComputedWidth();
  if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
    aDesiredSize.height = aReflowState.ComputedHeight();
  }

  FinishAndStoreOverflow(&aDesiredSize);

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
