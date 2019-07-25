





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"

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

  
  nsMathMLFrame::FindAttrDirectionality(mContent, mPresentationData);

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmstyleFrame::TransmitAutomaticData()
{
  return TransmitAutomaticDataForMrowLikeElement();
}




NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationData(uint32_t        aFlagsValues,
                                            uint32_t        aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }

  return nsMathMLContainerFrame::UpdatePresentationData(aFlagsValues, aWhichFlags);
}

NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                                       int32_t         aLastIndex,
                                                       uint32_t        aFlagsValues,
                                                       uint32_t        aWhichFlags)
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
nsMathMLmstyleFrame::AttributeChanged(int32_t         aNameSpaceID,
                                      nsIAtom*        aAttribute,
                                      int32_t         aModType)
{
  
  
  
  
  
  return ReLayoutChildren(mParent);
}
