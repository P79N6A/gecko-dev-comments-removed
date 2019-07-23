








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmsubsupFrame.h"





nsIFrame*
NS_NewMathMLmsubsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmsubsupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmsubsupFrame)

nsMathMLmsubsupFrame::~nsMathMLmsubsupFrame()
{
}

NS_IMETHODIMP
nsMathMLmsubsupFrame::TransmitAutomaticData()
{
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  
  
  
  
  
  
  UpdatePresentationDataFromChildAt(1, -1,
    ~NS_MATHML_DISPLAYSTYLE,
     NS_MATHML_DISPLAYSTYLE);
  UpdatePresentationDataFromChildAt(1,  1,
     NS_MATHML_COMPRESSED,
     NS_MATHML_COMPRESSED);

  return NS_OK;
}

 nsresult
nsMathMLmsubsupFrame::Place(nsIRenderingContext& aRenderingContext,
                            PRBool               aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  
  nscoord scriptSpace = 0;

  
  nsAutoString value;
  nscoord subScriptShift = 0;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::subscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      subScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }
  
  nscoord supScriptShift = 0;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::superscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      supScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }

  return nsMathMLmsubsupFrame::PlaceSubSupScript(PresContext(),
                                                 aRenderingContext,
                                                 aPlaceOrigin,
                                                 aDesiredSize,
                                                 this,
                                                 subScriptShift,
                                                 supScriptShift,
                                                 scriptSpace);
}



nsresult
nsMathMLmsubsupFrame::PlaceSubSupScript(nsPresContext*      aPresContext,
                                        nsIRenderingContext& aRenderingContext,
                                        PRBool               aPlaceOrigin,
                                        nsHTMLReflowMetrics& aDesiredSize,
                                        nsMathMLContainerFrame* aFrame,
                                        nscoord              aUserSubScriptShift,
                                        nscoord              aUserSupScriptShift,
                                        nscoord              aScriptSpace)
{
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  aScriptSpace = NS_MAX(onePixel, aScriptSpace);

  
  

  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsHTMLReflowMetrics supScriptSize;
  nsBoundingMetrics bmBase, bmSubScript, bmSupScript;
  nsIFrame* subScriptFrame = nsnull;
  nsIFrame* supScriptFrame = nsnull;
  nsIFrame* baseFrame = aFrame->GetFirstChild(nsnull);
  if (baseFrame)
    subScriptFrame = baseFrame->GetNextSibling();
  if (subScriptFrame)
    supScriptFrame = subScriptFrame->GetNextSibling();
  if (!baseFrame || !subScriptFrame || !supScriptFrame ||
      supScriptFrame->GetNextSibling()) {
    
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
  GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);

  
  nscoord subDrop;
  GetSubDropFromChild(subScriptFrame, subDrop);
  
  nscoord minSubScriptShift = bmBase.descent + subDrop;

  
  nscoord supDrop;
  GetSupDropFromChild(supScriptFrame, supDrop);
  
  nscoord minSupScriptShift = bmBase.ascent - supDrop;

  
  
  

  
  
  
  

  
  
  
  
  nscoord subScriptShift1, subScriptShift2;

  aRenderingContext.SetFont(baseFrame->GetStyleFont()->mFont, nsnull,
                            aPresContext->GetUserFontSet());
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));

  
  nscoord xHeight;
  fm->GetXHeight (xHeight);

  nscoord ruleSize;
  GetRuleThickness (aRenderingContext, fm, ruleSize);

  
  GetSubScriptShifts (fm, subScriptShift1, subScriptShift2);

  if (0 < aUserSubScriptShift) {
    
    float scaler = ((float) subScriptShift2) / subScriptShift1;
    subScriptShift1 = NS_MAX(subScriptShift1, aUserSubScriptShift);
    subScriptShift2 = NSToCoordRound(scaler * subScriptShift1);
  }

  
  
  nscoord subScriptShift =
    NS_MAX(minSubScriptShift,NS_MAX(subScriptShift1,subScriptShift2));

  
  
  
  

  
  
  nscoord minShiftFromXHeight = (nscoord)
    (bmSupScript.descent + (1.0f/4.0f) * xHeight);

  
  
  
  
  
  
  nscoord supScriptShift1, supScriptShift2, supScriptShift3;
  
  GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);
  if (0 < aUserSupScriptShift) {
    
    float scaler2 = ((float) supScriptShift2) / supScriptShift1;
    float scaler3 = ((float) supScriptShift3) / supScriptShift1;
    supScriptShift1 = NS_MAX(supScriptShift1, aUserSupScriptShift);
    supScriptShift2 = NSToCoordRound(scaler2 * supScriptShift1);
    supScriptShift3 = NSToCoordRound(scaler3 * supScriptShift1);
  }

  
  
  nscoord supScriptShift;
  nsPresentationData presentationData;
  aFrame->GetPresentationData(presentationData);
  if ( aFrame->GetStyleFont()->mScriptLevel == 0 &&
       NS_MATHML_IS_DISPLAYSTYLE(presentationData.flags) &&
      !NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    
    supScriptShift = supScriptShift1;
  }
  else if (NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    
    supScriptShift = supScriptShift3;
  }
  else {
    
    supScriptShift = supScriptShift2;
  }

  
  
  supScriptShift =
    NS_MAX(minSupScriptShift,NS_MAX(supScriptShift,minShiftFromXHeight));

  
  
  
  
  

  nscoord gap =
    (supScriptShift - bmSupScript.descent) -
    (bmSubScript.ascent - subScriptShift);
  if (gap < 4.0f * ruleSize) {
    
    subScriptShift += NSToCoordRound ((4.0f * ruleSize) - gap);
  }

  
  
  gap = NSToCoordRound ((4.0f/5.0f) * xHeight -
                        (supScriptShift - bmSupScript.descent));
  if (gap > 0) {
    supScriptShift += gap;
    subScriptShift -= gap;
  }

  
  
  

  
  nsBoundingMetrics boundingMetrics;
  boundingMetrics.ascent =
    NS_MAX(bmBase.ascent, (bmSupScript.ascent + supScriptShift));
  boundingMetrics.descent =
   NS_MAX(bmBase.descent, (bmSubScript.descent + subScriptShift));

  
  
  
  
  nscoord italicCorrection;
  GetItalicCorrection(bmBase, italicCorrection);
  italicCorrection += onePixel;
  boundingMetrics.width = bmBase.width + aScriptSpace +
    NS_MAX((italicCorrection + bmSupScript.width), bmSubScript.width);
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = bmBase.width +
    NS_MAX((italicCorrection + bmSupScript.rightBearing), bmSubScript.rightBearing);
  aFrame->SetBoundingMetrics(boundingMetrics);

  
  aDesiredSize.ascent =
    NS_MAX(baseSize.ascent, 
       NS_MAX(subScriptSize.ascent - subScriptShift,
              supScriptSize.ascent + supScriptShift));
  aDesiredSize.height = aDesiredSize.ascent +
    NS_MAX(baseSize.height - baseSize.ascent,
       NS_MAX(subScriptSize.height - subScriptSize.ascent + subScriptShift, 
              supScriptSize.height - subScriptSize.ascent - supScriptShift));
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    
    dx = 0; dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild(baseFrame, aPresContext, nsnull,
                      baseSize, dx, dy, 0);
    
    dx = bmBase.width;
    dy = aDesiredSize.ascent - (subScriptSize.ascent - subScriptShift);
    FinishReflowChild(subScriptFrame, aPresContext, nsnull,
                      subScriptSize, dx, dy, 0);
    
    dx = bmBase.width + italicCorrection;
    dy = aDesiredSize.ascent - (supScriptSize.ascent + supScriptShift);
    FinishReflowChild(supScriptFrame, aPresContext, nsnull,
                      supScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
