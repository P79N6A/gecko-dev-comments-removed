







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
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

  
  nsAutoString value;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::scriptlevel_, value);
  if (!value.IsEmpty()) {
    PRInt32 errorCode, userValue;
    userValue = value.ToInteger(&errorCode); 
    if (!errorCode) {
      if (value[0] != '+' && value[0] != '-') { 
        mPresentationData.flags |= NS_MATHML_EXPLICIT_SCRIPTLEVEL;
        mPresentationData.scriptLevel = userValue;
      }
      else {
        mPresentationData.scriptLevel += userValue; 
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmstyleFrame::TransmitAutomaticData()
{
  
  
  
  

  return NS_OK;
}




NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationData(PRInt32         aScriptLevelIncrement,
                                            PRUint32        aFlagsValues,
                                            PRUint32        aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }
  if (NS_MATHML_HAS_EXPLICIT_SCRIPTLEVEL(mPresentationData.flags)) {
    
    aScriptLevelIncrement = 0;
  }

  return nsMathMLContainerFrame::UpdatePresentationData(
    aScriptLevelIncrement, aFlagsValues, aWhichFlags);
}

NS_IMETHODIMP
nsMathMLmstyleFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                                       PRInt32         aLastIndex,
                                                       PRInt32         aScriptLevelIncrement,
                                                       PRUint32        aFlagsValues,
                                                       PRUint32        aWhichFlags)
{
  if (NS_MATHML_HAS_EXPLICIT_DISPLAYSTYLE(mPresentationData.flags)) {
    
    aWhichFlags &= ~NS_MATHML_DISPLAYSTYLE;
    aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
  }
  if (NS_MATHML_HAS_EXPLICIT_SCRIPTLEVEL(mPresentationData.flags)) {
    
    aScriptLevelIncrement = 0;
  }

  
  return
    nsMathMLContainerFrame::UpdatePresentationDataFromChildAt(
      aFirstIndex, aLastIndex, aScriptLevelIncrement,
      aFlagsValues, aWhichFlags); 
}

NS_IMETHODIMP
nsMathMLmstyleFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                      nsIAtom*        aAttribute,
                                      PRInt32         aModType)
{
  
  if (CommonAttributeChangedFor(PresContext(), mContent, aAttribute))
    return NS_OK;

  
  
  
  
  
  return ReLayoutChildren(mParent, NS_FRAME_IS_DIRTY);
}
