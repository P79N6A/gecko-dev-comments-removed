






































#include "nsCOMPtr.h"
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"

nsLeafFrame::~nsLeafFrame()
{
}

 nscoord
nsLeafFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetIntrinsicWidth();
  return result;
}

 nscoord
nsLeafFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  result = GetIntrinsicWidth();
  return result;
}

NS_IMETHODIMP
nsLeafFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsLeafFrame");
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsLeafFrame::Reflow: aMaxSize=%d,%d",
                  aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  
  

  NS_ASSERTION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
               "Shouldn't have unconstrained stuff here");

  aMetrics.width = aReflowState.ComputedWidth();
  if (NS_INTRINSICSIZE != aReflowState.mComputedHeight) {
    aMetrics.height = aReflowState.mComputedHeight;
  } else {
    aMetrics.height = GetIntrinsicHeight();
    
    aMetrics.height = NS_CSS_MINMAX(aMetrics.height,
                                    aReflowState.mComputedMinHeight,
                                    aReflowState.mComputedMaxHeight);
  }
  
  AddBordersAndPadding(aReflowState, aMetrics);
  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("exit nsLeafFrame::Reflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);

  aMetrics.mOverflowArea =
    nsRect(0, 0, aMetrics.width, aMetrics.height);
  FinishAndStoreOverflow(&aMetrics);
  return NS_OK;
}

nscoord
nsLeafFrame::GetIntrinsicHeight()
{
  NS_NOTREACHED("Someone didn't override Reflow");
  return 0;
}



void
nsLeafFrame::AddBordersAndPadding(const nsHTMLReflowState& aReflowState,
                                  nsHTMLReflowMetrics& aMetrics)
{
  aMetrics.width += aReflowState.mComputedBorderPadding.LeftRight();
  aMetrics.height += aReflowState.mComputedBorderPadding.TopBottom();
}

void
nsLeafFrame::SizeToAvailSize(const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  aDesiredSize.width  = aReflowState.availableWidth; 
  aDesiredSize.height = aReflowState.availableHeight;
  aDesiredSize.mOverflowArea =
    nsRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  FinishAndStoreOverflow(&aDesiredSize);  
}
