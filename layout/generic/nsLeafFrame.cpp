






































#include "nsCOMPtr.h"
#include "nsLeafFrame.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"

nsLeafFrame::~nsLeafFrame()
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsLeafFrame)

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

 nsSize
nsLeafFrame::ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder,
                             nsSize aPadding, PRBool aShrinkWrap)
{
  return nsSize(GetIntrinsicWidth(), GetIntrinsicHeight());
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

  DoReflow(aPresContext, aMetrics, aReflowState, aStatus);

  FinishAndStoreOverflow(&aMetrics);
  return NS_OK;
}

NS_IMETHODIMP
nsLeafFrame::DoReflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus)
{
  NS_ASSERTION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
               "Shouldn't have unconstrained stuff here "
               "Thanks to the rules of reflow");
  NS_ASSERTION(NS_INTRINSICSIZE != aReflowState.ComputedHeight(),
               "Shouldn't have unconstrained stuff here "
               "thanks to ComputeAutoSize");  

  aMetrics.width = aReflowState.ComputedWidth();
  aMetrics.height = aReflowState.ComputedHeight();
  
  AddBordersAndPadding(aReflowState, aMetrics);
  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("exit nsLeafFrame::DoReflow: size=%d,%d",
                  aMetrics.width, aMetrics.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);

  aMetrics.mOverflowArea =
    nsRect(0, 0, aMetrics.width, aMetrics.height);
  
  return NS_OK;
}

nscoord
nsLeafFrame::GetIntrinsicHeight()
{
  NS_NOTREACHED("Someone didn't override Reflow or ComputeAutoSize");
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
