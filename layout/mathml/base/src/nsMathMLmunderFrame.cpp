








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmunderFrame.h"
#include "nsMathMLmsubFrame.h"





nsIFrame*
NS_NewMathMLmunderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmunderFrame(aContext);
}

nsMathMLmunderFrame::~nsMathMLmunderFrame()
{
}

NS_IMETHODIMP
nsMathMLmunderFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                      nsIAtom*        aAttribute,
                                      PRInt32         aModType)
{
  if (nsGkAtoms::accentunder_ == aAttribute) {
    
    
    return ReLayoutChildren(mParent, NS_FRAME_IS_DIRTY);
  }

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

NS_IMETHODIMP
nsMathMLmunderFrame::UpdatePresentationData(PRInt32         aScriptLevelIncrement,
                                            PRUint32        aFlagsValues,
                                            PRUint32        aFlagsToUpdate)
{
  nsMathMLContainerFrame::UpdatePresentationData(
    aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
  
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;
  }
  else {
    mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmunderFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                                       PRInt32         aLastIndex,
                                                       PRInt32         aScriptLevelIncrement,
                                                       PRUint32        aFlagsValues,
                                                       PRUint32        aFlagsToUpdate)
{
  
  
  
  
  
  
  
  

  
  PRInt32 index = 0;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if ((index >= aFirstIndex) &&
        ((aLastIndex <= 0) || ((aLastIndex > 0) && (index <= aLastIndex)))) {
      if (index > 0) {
        
        aFlagsToUpdate &= ~NS_MATHML_DISPLAYSTYLE;
        aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
      }
      PropagatePresentationDataFor(childFrame,
        aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
    }
    index++;
    childFrame = childFrame->GetNextSibling();
  }
  return NS_OK;

  
  
}

NS_IMETHODIMP
nsMathMLmunderFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmunderFrame::TransmitAutomaticData()
{
  
  
  

  















  nsIFrame* underscriptFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    underscriptFrame = baseFrame->GetNextSibling();

  
  
  
  mPresentationData.baseFrame = baseFrame;
  GetEmbellishDataFrom(baseFrame, mEmbellishData);

  
  
  nsEmbellishData embellishData;
  GetEmbellishDataFrom(underscriptFrame, embellishData);
  if (NS_MATHML_EMBELLISH_IS_ACCENT(embellishData.flags))
    mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTUNDER;
  else
    mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTUNDER;

  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_true, &nsGkAtoms::_false, nsnull};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::accentunder_,
                                    strings, eCaseMatters)) {
    case 0: mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTUNDER; break;
    case 1: mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTUNDER; break;
  }

  
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  
  
  

  






  PRInt32 increment = NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)
    ? 0 : 1;
  PropagatePresentationDataFor(underscriptFrame, increment,
    ~NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED,
     NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED);

  return NS_OK;
}




















NS_IMETHODIMP
nsMathMLmunderFrame::Place(nsIRenderingContext& aRenderingContext,
                           PRBool               aPlaceOrigin,
                           nsHTMLReflowMetrics& aDesiredSize)
{
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
    
    return nsMathMLmsubFrame::PlaceSubScript(PresContext(),
                                             aRenderingContext,
                                             aPlaceOrigin,
                                             aDesiredSize,
                                             this, 0, PresContext()->PointsToAppUnits(0.5f));
  }

  
  

  nsBoundingMetrics bmBase, bmUnder;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics underSize;
  nsIFrame* underFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    underFrame = baseFrame->GetNextSibling();
  if (!baseFrame || !underFrame || underFrame->GetNextSibling()) {
    
    NS_WARNING("invalid markup");
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(underFrame, underSize, bmUnder);

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  

  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull);
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));

  nscoord xHeight = 0;
  fm->GetXHeight (xHeight);

  nscoord ruleThickness;
  GetRuleThickness (aRenderingContext, fm, ruleThickness);

  
  

  nscoord correction = 0;
  GetItalicCorrection (bmBase, correction);

  nscoord delta1 = 0; 
  nscoord delta2 = 0; 
  if (!NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)) {    
    
    nscoord bigOpSpacing2, bigOpSpacing4, bigOpSpacing5, dummy; 
    GetBigOpSpacings (fm, 
                      dummy, bigOpSpacing2, 
                      dummy, bigOpSpacing4, 
                      bigOpSpacing5);
    delta1 = PR_MAX(bigOpSpacing2, (bigOpSpacing4 - bmUnder.ascent));
    delta2 = bigOpSpacing5;
  }
  else {
    
    

    
    delta1 = ruleThickness + onePixel/2;
    delta2 = ruleThickness;
  }
  
  if (!(bmUnder.ascent + bmUnder.descent)) delta1 = 0;

  nscoord dxBase, dxUnder;
  nscoord maxWidth = PR_MAX(bmBase.width, bmUnder.width);
  if (NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)) {    
    dxUnder = (maxWidth - bmUnder.width)/2;
  }
  else {
    dxUnder = -correction/2 + (maxWidth - bmUnder.width)/2;
  }
  dxBase = (maxWidth - bmBase.width)/2;

  mBoundingMetrics.width =
    PR_MAX(dxBase + bmBase.width, dxUnder + bmUnder.width);
  mBoundingMetrics.ascent = bmBase.ascent;
  mBoundingMetrics.descent = 
    bmBase.descent + delta1 + bmUnder.ascent + bmUnder.descent;
  mBoundingMetrics.leftBearing = 
    PR_MIN(dxBase + bmBase.leftBearing, dxUnder + bmUnder.leftBearing);
  mBoundingMetrics.rightBearing = 
    PR_MAX(dxBase + bmBase.rightBearing, dxUnder + bmUnder.rightBearing);

  aDesiredSize.ascent = baseSize.ascent;
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(mBoundingMetrics.descent + delta2,
           bmBase.descent + delta1 + bmUnder.ascent +
             underSize.height - underSize.ascent);
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    nscoord dy = 0;
    
    FinishReflowChild(baseFrame, PresContext(), nsnull, baseSize, dxBase, dy, 0);
    
    dy = aDesiredSize.ascent + mBoundingMetrics.descent - bmUnder.descent - underSize.ascent;
    FinishReflowChild(underFrame, PresContext(), nsnull, underSize, dxUnder, dy, 0);
  }

  return NS_OK;
}

