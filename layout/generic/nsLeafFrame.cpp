






#include "nsLeafFrame.h"
#include "nsPresContext.h"

nsLeafFrame::~nsLeafFrame()
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsLeafFrame)

 nscoord
nsLeafFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetIntrinsicWidth();
  return result;
}

 nscoord
nsLeafFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  result = GetIntrinsicWidth();
  return result;
}

 nsSize
nsLeafFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder,
                             nsSize aPadding, bool aShrinkWrap)
{
  return nsSize(GetIntrinsicWidth(), GetIntrinsicHeight());
}

void
nsLeafFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsLeafFrame");
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("enter nsLeafFrame::Reflow: aMaxSize=%d,%d",
                  aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  DoReflow(aPresContext, aMetrics, aReflowState, aStatus);

  FinishAndStoreOverflow(&aMetrics);
}

void
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

  aMetrics.Width() = aReflowState.ComputedWidth();
  aMetrics.Height() = aReflowState.ComputedHeight();
  
  AddBordersAndPadding(aReflowState, aMetrics);
  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                 ("exit nsLeafFrame::DoReflow: size=%d,%d",
                  aMetrics.Width(), aMetrics.Height()));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);

  aMetrics.SetOverflowAreasToDesiredBounds();
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
  aMetrics.Width() += aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  aMetrics.Height() += aReflowState.ComputedPhysicalBorderPadding().TopBottom();
}

void
nsLeafFrame::SizeToAvailSize(const nsHTMLReflowState& aReflowState,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  aDesiredSize.Width() = aReflowState.AvailableWidth(); 
  aDesiredSize.Height() = aReflowState.AvailableHeight();
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);  
}
