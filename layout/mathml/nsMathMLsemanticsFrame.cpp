





































#include "nsMathMLsemanticsFrame.h"





nsIFrame*
NS_NewMathMLsemanticsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLsemanticsFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLsemanticsFrame)

nsMathMLsemanticsFrame::~nsMathMLsemanticsFrame()
{
}

NS_IMETHODIMP
nsMathMLsemanticsFrame::TransmitAutomaticData()
{
  
  
  
  
  
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  return NS_OK;
}
