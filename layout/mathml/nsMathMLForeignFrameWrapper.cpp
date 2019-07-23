










































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsFrame.h"
#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLForeignFrameWrapper.h"

NS_QUERYFRAME_HEAD(nsMathMLForeignFrameWrapper)
  NS_QUERYFRAME_ENTRY(nsMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

nsIFrame*
NS_NewMathMLForeignFrameWrapper(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLForeignFrameWrapper(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLForeignFrameWrapper)

NS_IMETHODIMP
nsMathMLForeignFrameWrapper::Reflow(nsPresContext*          aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aReflowState,
                                    nsReflowStatus&          aStatus)
{
  
  nsresult rv = nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  mBoundingMetrics.Clear();
  mBoundingMetrics.ascent = aDesiredSize.ascent;
  mBoundingMetrics.descent = aDesiredSize.height - aDesiredSize.ascent;
  mBoundingMetrics.width = aDesiredSize.width;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = aDesiredSize.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return rv;
}
