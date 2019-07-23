






































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

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

PRBool
nsMathMLmspaceFrame::IsLeaf() const
{
  return PR_TRUE;
}

void
nsMathMLmspaceFrame::ProcessAttributes(nsPresContext* aPresContext)
{
  







  nsAutoString value;
  nsCSSValue cssValue;

  
  mWidth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::width,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mWidth = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }

  
  mHeight = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::height,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mHeight = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }

  
  mDepth = 0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::depth_,
               value);
  if (!value.IsEmpty()) {
    if ((ParseNumericValue(value, cssValue) ||
         ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue)) &&
         cssValue.IsLengthUnit()) {
      mDepth = CalcLength(aPresContext, mStyleContext, cssValue);
    }
  }
}

NS_IMETHODIMP
nsMathMLmspaceFrame::Reflow(nsPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus)
{
  ProcessAttributes(aPresContext);

  mBoundingMetrics.Clear();
  mBoundingMetrics.width = mWidth;
  mBoundingMetrics.ascent = mHeight;
  mBoundingMetrics.descent = mDepth;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = mWidth;

  aDesiredSize.ascent = mHeight;
  aDesiredSize.width = mWidth;
  aDesiredSize.height = aDesiredSize.ascent + mDepth;
  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}
