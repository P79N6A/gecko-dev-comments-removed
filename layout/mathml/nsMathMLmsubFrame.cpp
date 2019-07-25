








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"

#include "nsMathMLmsubFrame.h"





nsIFrame*
NS_NewMathMLmsubFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmsubFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmsubFrame)

nsMathMLmsubFrame::~nsMathMLmsubFrame()
{
}

NS_IMETHODIMP
nsMathMLmsubFrame::TransmitAutomaticData()
{
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  
  
  
  
  UpdatePresentationDataFromChildAt(1, -1,
    ~NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED,
     NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED);

  return NS_OK;
}

 nsresult
nsMathMLmsubFrame::Place (nsRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  
  nscoord scriptSpace = nsPresContext::CSSPointsToAppUnits(0.5f); 

  
  nscoord subScriptShift = 0;
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::subscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      subScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }

  return nsMathMLmsubFrame::PlaceSubScript(PresContext(), 
                                           aRenderingContext,
                                           aPlaceOrigin,
                                           aDesiredSize,
                                           this,
                                           subScriptShift,
                                           scriptSpace);
}



nsresult
nsMathMLmsubFrame::PlaceSubScript (nsPresContext*      aPresContext,
                                   nsRenderingContext& aRenderingContext,
                                   PRBool               aPlaceOrigin,
                                   nsHTMLReflowMetrics& aDesiredSize,
                                   nsMathMLContainerFrame* aFrame,
                                   nscoord              aUserSubScriptShift,
                                   nscoord              aScriptSpace)
{
  
  aScriptSpace = NS_MAX(nsPresContext::CSSPixelsToAppUnits(1), aScriptSpace);

  
  

  nsBoundingMetrics bmBase, bmSubScript;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsIFrame* baseFrame = aFrame->GetFirstChild(nsnull);
  nsIFrame* subScriptFrame = nsnull;
  if (baseFrame)
    subScriptFrame = baseFrame->GetNextSibling();
  if (!baseFrame || !subScriptFrame || subScriptFrame->GetNextSibling()) {
    
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);

  
  nscoord subDrop;
  GetSubDropFromChild(subScriptFrame, subDrop);
  
  nscoord minSubScriptShift = bmBase.descent + subDrop;

  
  
  
  
  
  nscoord xHeight = 0;
  nsRefPtr<nsFontMetrics> fm =
    aPresContext->GetMetricsFor(baseFrame->GetStyleFont()->mFont);

  fm->GetXHeight (xHeight);
  nscoord minShiftFromXHeight = (nscoord) 
    (bmSubScript.ascent - (4.0f/5.0f) * xHeight);

  
  
  
  
  nscoord subScriptShift, dummy;
  
  GetSubScriptShifts (fm, subScriptShift, dummy);

  subScriptShift = 
    NS_MAX(subScriptShift, aUserSubScriptShift);

  
  
  nscoord actualSubScriptShift = 
    NS_MAX(minSubScriptShift,NS_MAX(subScriptShift,minShiftFromXHeight));
  
  nsBoundingMetrics boundingMetrics;
  boundingMetrics.ascent = 
    NS_MAX(bmBase.ascent, bmSubScript.ascent - actualSubScriptShift);
  boundingMetrics.descent = 
    NS_MAX(bmBase.descent, bmSubScript.descent + actualSubScriptShift);

  
  boundingMetrics.width = bmBase.width + bmSubScript.width + aScriptSpace;
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = NS_MAX(bmBase.rightBearing, bmBase.width +
    NS_MAX(bmSubScript.width + aScriptSpace, bmSubScript.rightBearing));
  aFrame->SetBoundingMetrics (boundingMetrics);

  
  aDesiredSize.ascent = 
    NS_MAX(baseSize.ascent, subScriptSize.ascent - actualSubScriptShift);
  aDesiredSize.height = aDesiredSize.ascent +
    NS_MAX(baseSize.height - baseSize.ascent,
           subScriptSize.height - subScriptSize.ascent + actualSubScriptShift);
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    
    dx = 0; dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild (baseFrame, aPresContext, nsnull, baseSize, dx, dy, 0);
    
    dx = bmBase.width; 
    dy = aDesiredSize.ascent - (subScriptSize.ascent - actualSubScriptShift);
    FinishReflowChild (subScriptFrame, aPresContext, nsnull, subScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
