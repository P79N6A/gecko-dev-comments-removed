









































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmunderoverFrame.h"
#include "nsMathMLmsubsupFrame.h"





nsIFrame*
NS_NewMathMLmunderoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmunderoverFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmunderoverFrame)

nsMathMLmunderoverFrame::~nsMathMLmunderoverFrame()
{
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          PRInt32         aModType)
{
  if (nsGkAtoms::accent_ == aAttribute ||
      nsGkAtoms::accentunder_ == aAttribute) {
    
    
    return ReLayoutChildren(mParent);
  }

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::UpdatePresentationData(PRUint32        aFlagsValues,
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
nsMathMLmunderoverFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
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
nsMathMLmunderoverFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::TransmitAutomaticData()
{
  
  
  

  










  nsIFrame* overscriptFrame = nsnull;
  nsIFrame* underscriptFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    underscriptFrame = baseFrame->GetNextSibling();
  if (underscriptFrame)
    overscriptFrame = underscriptFrame->GetNextSibling();

  
  
  
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

  
  
  GetEmbellishDataFrom(overscriptFrame, embellishData);
  if (NS_MATHML_EMBELLISH_IS_ACCENT(embellishData.flags))
    mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER;
  else
    mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER;

  
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::accent_,
                                    strings, eCaseMatters)) {
    case 0: mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER; break;
    case 1: mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER; break;
  }

  
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;

  
  
  

  











  PRUint32 compress = NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)
    ? NS_MATHML_COMPRESSED : 0;
  SetIncrementScriptLevel(2, !NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags));
  PropagatePresentationDataFor(overscriptFrame,
    ~NS_MATHML_DISPLAYSTYLE | compress,
     NS_MATHML_DISPLAYSTYLE | compress);

  



  SetIncrementScriptLevel(1, !NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags));
  PropagatePresentationDataFor(underscriptFrame,
    ~NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED,
     NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED);

  return NS_OK;
}




















 nsresult
nsMathMLmunderoverFrame::Place(nsIRenderingContext& aRenderingContext,
                               PRBool               aPlaceOrigin,
                               nsHTMLReflowMetrics& aDesiredSize)
{
  if ( NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
    
    return nsMathMLmsubsupFrame::PlaceSubSupScript(PresContext(),
                                                   aRenderingContext,
                                                   aPlaceOrigin,
                                                   aDesiredSize,
                                                   this, 0, 0, PresContext()->PointsToAppUnits(0.5f));
  }

  
  

  nsBoundingMetrics bmBase, bmUnder, bmOver;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics underSize;
  nsHTMLReflowMetrics overSize;
  nsIFrame* overFrame = nsnull;
  nsIFrame* underFrame = nsnull;
  nsIFrame* baseFrame = mFrames.FirstChild();
  if (baseFrame)
    underFrame = baseFrame->GetNextSibling();
  if (underFrame)
    overFrame = underFrame->GetNextSibling();
  if (!baseFrame || !underFrame || !overFrame || overFrame->GetNextSibling()) {
    
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(underFrame, underSize, bmUnder);
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

  
  

  nscoord underDelta1 = 0; 
  nscoord underDelta2 = 0; 

  if (!NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)) {
    
    nscoord bigOpSpacing2, bigOpSpacing4, bigOpSpacing5, dummy; 
    GetBigOpSpacings (fm, 
                      dummy, bigOpSpacing2, 
                      dummy, bigOpSpacing4, 
                      bigOpSpacing5);
    underDelta1 = PR_MAX(bigOpSpacing2, (bigOpSpacing4 - bmUnder.ascent));
    underDelta2 = bigOpSpacing5;
  }
  else {
    
    

    
    underDelta1 = ruleThickness + onePixel/2;
    underDelta2 = ruleThickness;
  }
  
  if (!(bmUnder.ascent + bmUnder.descent)) underDelta1 = 0;

  nscoord overDelta1 = 0; 
  nscoord overDelta2 = 0; 

  if (!NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)) {    
    
    nscoord bigOpSpacing1, bigOpSpacing3, bigOpSpacing5, dummy; 
    GetBigOpSpacings (fm, 
                      bigOpSpacing1, dummy, 
                      bigOpSpacing3, dummy, 
                      bigOpSpacing5);
    overDelta1 = PR_MAX(bigOpSpacing1, (bigOpSpacing3 - bmOver.descent));
    overDelta2 = bigOpSpacing5;

    
    
    
    if (bmOver.descent < 0)    
      overDelta1 = PR_MAX(bigOpSpacing1, (bigOpSpacing3 - (bmOver.ascent + bmOver.descent)));
  }
  else {
    
    overDelta1 = ruleThickness + onePixel/2;
    if (bmBase.ascent < xHeight) { 
      overDelta1 += xHeight - bmBase.ascent;
    }
    overDelta2 = ruleThickness;
  }
  
  if (!(bmOver.ascent + bmOver.descent)) overDelta1 = 0;

  nscoord dxBase, dxOver = 0, dxUnder = 0;

  
  

  
  
  
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
  dxBase = (mBoundingMetrics.width - bmBase.width)/2;

  mBoundingMetrics.ascent = 
    bmBase.ascent + overDelta1 + bmOver.ascent + bmOver.descent;
  mBoundingMetrics.descent = 
    bmBase.descent + underDelta1 + bmUnder.ascent + bmUnder.descent;
  mBoundingMetrics.leftBearing = 
    PR_MIN(dxBase + bmBase.leftBearing, dxOver + bmOver.leftBearing);
  mBoundingMetrics.rightBearing = 
    PR_MAX(dxBase + bmBase.rightBearing, dxOver + bmOver.rightBearing);

  
  
  
  
  
  

  nsBoundingMetrics bmAnonymousBase = mBoundingMetrics;
  nscoord ascentAnonymousBase =
    PR_MAX(mBoundingMetrics.ascent + overDelta2,
           overSize.ascent + bmOver.descent + overDelta1 + bmBase.ascent);

  GetItalicCorrection(bmAnonymousBase, correction);

  
  nscoord underWidth = bmUnder.width;
  if (!underWidth) {
    underWidth = bmUnder.rightBearing - bmUnder.leftBearing;
    dxUnder = -bmUnder.leftBearing;
  }

  nscoord maxWidth = PR_MAX(bmAnonymousBase.width, underWidth);
  if (NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)) {
    dxUnder += (maxWidth - underWidth)/2;;
  }
  else {
    dxUnder += -correction/2 + (maxWidth - underWidth)/2;
  }
  nscoord dxAnonymousBase = (maxWidth - bmAnonymousBase.width)/2;

  
  
  dxOver += dxAnonymousBase;
  dxBase += dxAnonymousBase;

  mBoundingMetrics.width =
    PR_MAX(dxAnonymousBase + bmAnonymousBase.width, dxUnder + bmUnder.width);
  mBoundingMetrics.leftBearing =
    PR_MIN(dxAnonymousBase + bmAnonymousBase.leftBearing, dxUnder + bmUnder.leftBearing);
  mBoundingMetrics.rightBearing = 
    PR_MAX(dxAnonymousBase + bmAnonymousBase.rightBearing, dxUnder + bmUnder.rightBearing);

  aDesiredSize.ascent = ascentAnonymousBase;
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(mBoundingMetrics.descent + underDelta2,
           bmAnonymousBase.descent + underDelta1 + bmUnder.ascent +
             underSize.height - underSize.ascent);
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    nscoord dy;
    
    dy = aDesiredSize.ascent - mBoundingMetrics.ascent + bmOver.ascent - overSize.ascent;
    FinishReflowChild (overFrame, PresContext(), nsnull, overSize, dxOver, dy, 0);
    
    dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild (baseFrame, PresContext(), nsnull, baseSize, dxBase, dy, 0);
    
    dy = aDesiredSize.ascent + mBoundingMetrics.descent - bmUnder.descent - underSize.ascent;
    FinishReflowChild (underFrame, PresContext(), nsnull, underSize, dxUnder, dy, 0);
  }
  return NS_OK;
}
