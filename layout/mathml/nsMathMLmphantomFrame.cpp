





#include "nsMathMLmphantomFrame.h"
#include "mozilla/gfx/2D.h"





nsIFrame*
NS_NewMathMLmphantomFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmphantomFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmphantomFrame)

nsMathMLmphantomFrame::~nsMathMLmphantomFrame()
{
}

NS_IMETHODIMP
nsMathMLmphantomFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}
