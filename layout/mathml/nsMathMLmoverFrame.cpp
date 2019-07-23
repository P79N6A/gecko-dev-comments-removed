









































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmoverFrame.h"
#include "nsMathMLmsupFrame.h"





nsIFrame*
NS_NewMathMLmoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmoverFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmoverFrame)

nsMathMLmoverFrame::~nsMathMLmoverFrame()
{
}

NS_IMETHODIMP
nsMathMLmoverFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (nsGkAtoms::accent_ == aAttribute) {
    
    
    return ReLayoutChildren(mParent);
  }

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

NS_IMETHODIMP
nsMathMLmoverFrame::UpdatePresentationData(PRUint32        aFlagsValues,
                                           PRUint32        aFlagsToUpdate)
{
  nsMathMLContainerFrame::UpdatePresentationData(aFlagsValues, aFlagsToUpdate);
  
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
nsMathMLmoverFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                                      PRInt32         aLastIndex,
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
      PropagatePresentationDataFor(childFrame, aFlagsValues, aFlagsToUpdate);
    }
    index++;
    childFrame = childFrame->GetNextSibling();
  }
  return NS_OK;

  
  
}

NS_IMETHODIMP
nsMathMLmoverFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmoverFrame::TransmitAutomaticData()
{
  
  
  

  















  nsIFrame* overscriptFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    overscriptFrame = baseFrame->GetNextSibling();

  
  
  
  mPresentationData.baseFrame = baseFrame;
  GetEmbellishDataFrom(baseFrame, mEmbellishData);

  
  
  nsEmbellishData embellishData;
  GetEmbellishDataFrom(overscriptFrame, embellishData);
  if (NS_MATHML_EMBELLISH_IS_ACCENT(embellishData.flags))
    mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER;
  else
    mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER;

  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_true, &nsGkAtoms::_false, nsnull};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::accent_,
                                    strings, eCaseMatters)) {
    case 0: mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER; break;
    case 1: mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER; break;
  }

  
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  
  
  

  








  SetIncrementScriptLevel(1, !NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags));
  PRUint32 compress = NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)
    ? NS_MATHML_COMPRESSED : 0;
  PropagatePresentationDataFor(overscriptFrame,
    ~NS_MATHML_DISPLAYSTYLE | compress,
     NS_MATHML_DISPLAYSTYLE | compress);

  return NS_OK;
}



















 nsresult
nsMathMLmoverFrame::Place(nsIRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{ 
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
    
    return nsMathMLmsupFrame::PlaceSuperScript(PresContext(),
                                               aRenderingContext,
                                               aPlaceOrigin,
                                               aDesiredSize,
                                               this, 0, PresContext()->PointsToAppUnits(0.5f));
  }

  
  

  nsBoundingMetrics bmBase, bmOver;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics overSize;
  nsIFrame* overFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    overFrame = baseFrame->GetNextSibling();
  if (!baseFrame || !overFrame || overFrame->GetNextSibling()) {
    
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(overFrame, overSize, bmOver);

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  

  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull,
                            PresContext()->GetUserFontSet());
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
  if (!NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)) {    
    
    nscoord bigOpSpacing1, bigOpSpacing3, bigOpSpacing5, dummy; 
    GetBigOpSpacings (fm, 
                      bigOpSpacing1, dummy, 
                      bigOpSpacing3, dummy, 
                      bigOpSpacing5);
    delta1 = PR_MAX(bigOpSpacing1, (bigOpSpacing3 - bmOver.descent));
    delta2 = bigOpSpacing5;

    
    
    
    if (bmOver.descent < 0)    
      delta1 = PR_MAX(bigOpSpacing1, (bigOpSpacing3 - (bmOver.ascent + bmOver.descent)));
  }
  else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    delta1 = ruleThickness + onePixel/2; 
    if (bmBase.ascent < xHeight) { 
      
      delta1 += xHeight - bmBase.ascent;
    }
    delta2 = ruleThickness;
  }
  
  if (!(bmOver.ascent + bmOver.descent)) delta1 = 0;

  nscoord dxBase, dxOver = 0;

  
  
  
  nscoord overWidth = bmOver.width;
  if (!overWidth && (bmOver.rightBearing - bmOver.leftBearing > 0)) {
    overWidth = bmOver.rightBearing - bmOver.leftBearing;
    dxOver = -bmOver.leftBearing;
  }

  if (NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)) {
    mBoundingMetrics.width = bmBase.width; 
    dxOver += correction + (mBoundingMetrics.width - overWidth)/2;
  }
  else {
    mBoundingMetrics.width = PR_MAX(bmBase.width, overWidth);
    dxOver += correction/2 + (mBoundingMetrics.width - overWidth)/2;
  }
  dxBase = (mBoundingMetrics.width - bmBase.width) / 2;

  mBoundingMetrics.ascent = 
    bmOver.ascent + bmOver.descent + delta1 + bmBase.ascent;
  mBoundingMetrics.descent = bmBase.descent;
  mBoundingMetrics.leftBearing = 
    PR_MIN(dxBase + bmBase.leftBearing, dxOver + bmOver.leftBearing);
  mBoundingMetrics.rightBearing = 
    PR_MAX(dxBase + bmBase.rightBearing, dxOver + bmOver.rightBearing);

  aDesiredSize.ascent = 
    PR_MAX(mBoundingMetrics.ascent + delta2,
           overSize.ascent + bmOver.descent + delta1 + bmBase.ascent);
  aDesiredSize.height = aDesiredSize.ascent +
    baseSize.height - baseSize.ascent;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    
    nscoord dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild (baseFrame, PresContext(), nsnull, baseSize, dxBase, dy, 0);
    
    dy = aDesiredSize.ascent - 
      mBoundingMetrics.ascent + bmOver.ascent - overSize.ascent;
    FinishReflowChild (overFrame, PresContext(), nsnull, overSize, dxOver, dy, 0);
  }
  return NS_OK;
}
