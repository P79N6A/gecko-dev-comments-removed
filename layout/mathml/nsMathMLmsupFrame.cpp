







































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmsupFrame.h"





nsIFrame*
NS_NewMathMLmsupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmsupFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmsupFrame)

nsMathMLmsupFrame::~nsMathMLmsupFrame()
{
}

NS_IMETHODIMP
nsMathMLmsupFrame::TransmitAutomaticData()
{
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  
  
  
  
  
  UpdatePresentationDataFromChildAt(1, -1,
    ~NS_MATHML_DISPLAYSTYLE,
     NS_MATHML_DISPLAYSTYLE);

  return NS_OK;
}

 nsresult
nsMathMLmsupFrame::Place(nsIRenderingContext& aRenderingContext,
                         PRBool               aPlaceOrigin,
                         nsHTMLReflowMetrics& aDesiredSize)
{
  
  nscoord scriptSpace = PresContext()->PointsToAppUnits(0.5f); 

  
  nsAutoString value;
  nscoord supScriptShift = 0;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::superscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      supScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }

  return nsMathMLmsupFrame::PlaceSuperScript(PresContext(), 
                                             aRenderingContext,
                                             aPlaceOrigin,
                                             aDesiredSize,
                                             this,
                                             supScriptShift,
                                             scriptSpace);
}



nsresult
nsMathMLmsupFrame::PlaceSuperScript(nsPresContext*      aPresContext,
                                    nsIRenderingContext& aRenderingContext,
                                    PRBool               aPlaceOrigin,
                                    nsHTMLReflowMetrics& aDesiredSize,
                                    nsMathMLContainerFrame* aFrame,
                                    nscoord              aUserSupScriptShift,
                                    nscoord              aScriptSpace)
{
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  aScriptSpace = PR_MAX(onePixel, aScriptSpace);

  
  

  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics supScriptSize;
  nsBoundingMetrics bmBase, bmSupScript;
  nsIFrame* supScriptFrame = nsnull;
  nsIFrame* baseFrame = aFrame->GetFirstChild(nsnull);
  if (baseFrame)
    supScriptFrame = baseFrame->GetNextSibling();
  if (!baseFrame || !supScriptFrame || supScriptFrame->GetNextSibling()) {
    
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);

  
  nscoord supDrop;
  GetSupDropFromChild(supScriptFrame, supDrop);
  
  nscoord minSupScriptShift = bmBase.ascent - supDrop;

  
  
  
  
  
  nscoord xHeight = 0;
  nsCOMPtr<nsIFontMetrics> fm =
    aPresContext->GetMetricsFor(baseFrame->GetStyleFont()->mFont);

  fm->GetXHeight (xHeight);
  nscoord minShiftFromXHeight = (nscoord) 
    (bmSupScript.descent + (1.0f/4.0f) * xHeight);
  nscoord italicCorrection;
  GetItalicCorrection(bmBase, italicCorrection);

  
  
  
  
  
  
  nscoord supScriptShift1, supScriptShift2, supScriptShift3;
  
  GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);

  if (0 < aUserSupScriptShift) {
    
    float scaler2 = ((float) supScriptShift2) / supScriptShift1;
    float scaler3 = ((float) supScriptShift3) / supScriptShift1;
    supScriptShift1 = 
      PR_MAX(supScriptShift1, aUserSupScriptShift);
    supScriptShift2 = NSToCoordRound(scaler2 * supScriptShift1);
    supScriptShift3 = NSToCoordRound(scaler3 * supScriptShift1);
  }

  
  
  nscoord supScriptShift;
  nsPresentationData presentationData;
  aFrame->GetPresentationData (presentationData);
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

  
  
  nscoord actualSupScriptShift = 
    PR_MAX(minSupScriptShift,PR_MAX(supScriptShift,minShiftFromXHeight));

  
  nsBoundingMetrics boundingMetrics;
  boundingMetrics.ascent =
    PR_MAX(bmBase.ascent, (bmSupScript.ascent + actualSupScriptShift));
  boundingMetrics.descent =
    PR_MAX(bmBase.descent, (bmSupScript.descent - actualSupScriptShift));

  
  
  
  
  italicCorrection += onePixel;
  boundingMetrics.width = bmBase.width + italicCorrection +
                          bmSupScript.width + aScriptSpace;
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = bmBase.width + italicCorrection +
                                 bmSupScript.rightBearing;
  aFrame->SetBoundingMetrics(boundingMetrics);

  
  aDesiredSize.ascent =
    PR_MAX(baseSize.ascent, (supScriptSize.ascent + actualSupScriptShift));
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(baseSize.height - baseSize.ascent,
           (supScriptSize.height - supScriptSize.ascent - actualSupScriptShift));
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    
    dx = 0; dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild (baseFrame, aPresContext, nsnull, baseSize, dx, dy, 0);
    
    dx = bmBase.width + italicCorrection;
    dy = aDesiredSize.ascent - (supScriptSize.ascent + actualSupScriptShift);
    FinishReflowChild (supScriptFrame, aPresContext, nsnull, supScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
