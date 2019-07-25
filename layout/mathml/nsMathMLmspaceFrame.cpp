





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"

#include "nsMathMLmspaceFrame.h"






nsIFrame*
NS_NewMathMLmspaceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmspaceFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmspaceFrame)

nsMathMLmspaceFrame::~nsMathMLmspaceFrame()
{
}

bool
nsMathMLmspaceFrame::IsLeaf() const
{
  return true;
}

void
nsMathMLmspaceFrame::ProcessAttributes(nsPresContext* aPresContext)
{
  nsAutoString value;

  
  
  
  
  
  
  
  
  
  
  
  
  mWidth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::width,
               value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mWidth,
                      nsMathMLElement::PARSE_ALLOW_NEGATIVE,
                      aPresContext, mStyleContext);
  }

  
  
  
  
  
  
  
  
  
  
  mHeight = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::height,
               value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mHeight, 0,
                      aPresContext, mStyleContext);
  }

  
  
  
  
  
  
  
  
  
  
  mDepth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::depth_,
               value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mDepth, 0,
                      aPresContext, mStyleContext);
  }
}

NS_IMETHODIMP
nsMathMLmspaceFrame::Reflow(nsPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus)
{
  ProcessAttributes(aPresContext);
  
  

  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.width = NS_MAX(0, mWidth);
  mBoundingMetrics.ascent = mHeight;
  mBoundingMetrics.descent = mDepth;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = mBoundingMetrics.width;

  aDesiredSize.ascent = mHeight;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.height = aDesiredSize.ascent + mDepth;
  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}
