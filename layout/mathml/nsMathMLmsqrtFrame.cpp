









































#include "nsMathMLmsqrtFrame.h"














nsIFrame*
NS_NewMathMLmsqrtFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmsqrtFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmsqrtFrame)

nsMathMLmsqrtFrame::nsMathMLmsqrtFrame(nsStyleContext* aContext) :
  nsMathMLmencloseFrame(aContext)
{
}

nsMathMLmsqrtFrame::~nsMathMLmsqrtFrame()
{
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  AllocateMathMLChar(NOTATION_RADICAL);
  mNotationsToDraw |= NOTATION_RADICAL;

  return rv;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::InheritAutomaticData(nsIFrame* aParent)
{
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  return nsMathMLContainerFrame::
    AttributeChanged(aNameSpaceID, aAttribute, aModType);
}
