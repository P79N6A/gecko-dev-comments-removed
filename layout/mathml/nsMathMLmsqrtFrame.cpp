




#include "nsMathMLmsqrtFrame.h"
#include "mozilla/gfx/2D.h"














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

void
nsMathMLmsqrtFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  AllocateMathMLChar(NOTATION_RADICAL);
  mNotationsToDraw |= NOTATION_RADICAL;
}

NS_IMETHODIMP
nsMathMLmsqrtFrame::InheritAutomaticData(nsIFrame* aParent)
{
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  return NS_OK;
}

nsresult
nsMathMLmsqrtFrame::AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType)
{
  return nsMathMLContainerFrame::
    AttributeChanged(aNameSpaceID, aAttribute, aModType);
}
