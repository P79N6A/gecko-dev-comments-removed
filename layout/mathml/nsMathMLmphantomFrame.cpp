







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmphantomFrame.h"





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
