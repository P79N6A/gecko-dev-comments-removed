




#include "nsMathMLmstyleFrame.h"
#include "mozilla/gfx/2D.h"





nsIFrame*
NS_NewMathMLmstyleFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmstyleFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmstyleFrame)

nsMathMLmstyleFrame::~nsMathMLmstyleFrame()
{
}

NS_IMETHODIMP
nsMathMLmstyleFrame::InheritAutomaticData(nsIFrame* aParent) 
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  
  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;
  mPresentationData.mstyle = this;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmstyleFrame::TransmitAutomaticData()
{
  return TransmitAutomaticDataForMrowLikeElement();
}

NS_IMETHODIMP
nsMathMLmstyleFrame::AttributeChanged(int32_t         aNameSpaceID,
                                      nsIAtom*        aAttribute,
                                      int32_t         aModType)
{
  
  
  
  
  
  return ReLayoutChildren(mParent);
}
