




#include "nsMathMLmspaceFrame.h"
#include "nsMathMLElement.h"
#include "mozilla/gfx/2D.h"
#include <algorithm>






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
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::width, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mWidth,
                      nsMathMLElement::PARSE_ALLOW_NEGATIVE,
                      aPresContext, mStyleContext);
  }

  
  
  
  
  
  
  
  
  
  
  mHeight = 0;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::height, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mHeight, 0,
                      aPresContext, mStyleContext);
  }

  
  
  
  
  
  
  
  
  
  
  mDepth = 0;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::depth_, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &mDepth, 0,
                      aPresContext, mStyleContext);
  }
}

void
nsMathMLmspaceFrame::Reflow(nsPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsReflowStatus&          aStatus)
{
  ProcessAttributes(aPresContext);

  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.width = mWidth;
  mBoundingMetrics.ascent = mHeight;
  mBoundingMetrics.descent = mDepth;
  mBoundingMetrics.leftBearing = 0;
  mBoundingMetrics.rightBearing = mBoundingMetrics.width;

  aDesiredSize.SetTopAscent(mHeight);
  aDesiredSize.Width() = std::max(0, mBoundingMetrics.width);
  aDesiredSize.Height() = aDesiredSize.TopAscent() + mDepth;
  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

 nsresult
nsMathMLmspaceFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                     nsHTMLReflowMetrics& aDesiredSize)
{
  ProcessAttributes(PresContext());
  mBoundingMetrics = nsBoundingMetrics();
  mBoundingMetrics.width = mWidth;
  aDesiredSize.Width() = std::max(0, mBoundingMetrics.width);
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  return NS_OK;
}
