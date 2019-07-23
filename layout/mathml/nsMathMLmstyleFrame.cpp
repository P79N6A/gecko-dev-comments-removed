







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmstyleFrame.h"





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

  
  nsMathMLFrame::FindAttrDisplaystyle(mContent, mPresentationData);

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmstyleFrame::TransmitAutomaticData()
{
  
  
  
  

  return NS_OK;
}




NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationData(PRUint32        aFlagsValues,
                                            PRUint32        aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  return nsMathMLContainerFrame::UpdatePresentationData(aFlagsValues, aWhichFlags);
}

NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                                       PRInt32         aLastIndex,
                                                       PRUint32        aFlagsValues,
                                                       PRUint32        aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  
  return
    nsMathMLContainerFrame::UpdatePresentationDataFromChildAt(
      aFirstIndex, aLastIndex, aFlagsValues, aWhichFlags); 
}

NS_IMETHODIMP
nsMathMLmstyleFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                      nsIAtom*        aAttribute,
                                      PRInt32         aModType)
{
  
  
  
  
  
  return ReLayoutChildren(mParent);
}
