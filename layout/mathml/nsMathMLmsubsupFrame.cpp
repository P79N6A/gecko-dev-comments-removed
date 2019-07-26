





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsRenderingContext.h"

#include "nsMathMLmsubsupFrame.h"
#include <algorithm>





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
nsMathMLmsubsupFrame::Place(nsRenderingContext& aRenderingContext,
                            bool                 aPlaceOrigin,
                            nsHTMLReflowMetrics& aDesiredSize)
{
  
  nscoord scriptSpace = 0;

  
  
  
  
  
  
  
  
  
  
  
  nsAutoString value;
  nscoord subScriptShift = 0;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::subscriptshift_, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &subScriptShift, 0, PresContext(), mStyleContext);
  }
  
  
  
  
  
  
  
  
  
  
  
  nscoord supScriptShift = 0;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::superscriptshift_, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &supScriptShift, 0, PresContext(), mStyleContext);
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
                                        nsRenderingContext& aRenderingContext,
                                        bool                 aPlaceOrigin,
                                        nsHTMLReflowMetrics& aDesiredSize,
                                        nsMathMLContainerFrame* aFrame,
                                        nscoord              aUserSubScriptShift,
                                        nscoord              aUserSupScriptShift,
                                        nscoord              aScriptSpace)
{
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  aScriptSpace = std::max(onePixel, aScriptSpace);

  
  

  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsHTMLReflowMetrics supScriptSize;
  nsBoundingMetrics bmBase, bmSubScript, bmSupScript;
  nsIFrame* subScriptFrame = nullptr;
  nsIFrame* supScriptFrame = nullptr;
  nsIFrame* baseFrame = aFrame->GetFirstPrincipalChild();
  if (baseFrame)
    subScriptFrame = baseFrame->GetNextSibling();
  if (subScriptFrame)
    supScriptFrame = subScriptFrame->GetNextSibling();
  if (!baseFrame || !subScriptFrame || !supScriptFrame ||
      supScriptFrame->GetNextSibling()) {
    
    if (aPlaceOrigin) {
      aFrame->ReportChildCountError();
    }
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

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(baseFrame, getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  
  nscoord xHeight = fm->XHeight();

  nscoord ruleSize;
  GetRuleThickness (aRenderingContext, fm, ruleSize);

  
  GetSubScriptShifts (fm, subScriptShift1, subScriptShift2);

  if (0 < aUserSubScriptShift) {
    
    float scaler = ((float) subScriptShift2) / subScriptShift1;
    subScriptShift1 = std::max(subScriptShift1, aUserSubScriptShift);
    subScriptShift2 = NSToCoordRound(scaler * subScriptShift1);
  }

  
  
  nscoord subScriptShift =
    std::max(minSubScriptShift,std::max(subScriptShift1,subScriptShift2));

  
  
  
  

  
  
  nscoord minShiftFromXHeight = (nscoord)
    (bmSupScript.descent + (1.0f/4.0f) * xHeight);

  
  
  
  
  
  
  nscoord supScriptShift1, supScriptShift2, supScriptShift3;
  
  GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);
  if (0 < aUserSupScriptShift) {
    
    float scaler2 = ((float) supScriptShift2) / supScriptShift1;
    float scaler3 = ((float) supScriptShift3) / supScriptShift1;
    supScriptShift1 = std::max(supScriptShift1, aUserSupScriptShift);
    supScriptShift2 = NSToCoordRound(scaler2 * supScriptShift1);
    supScriptShift3 = NSToCoordRound(scaler3 * supScriptShift1);
  }

  
  
  nscoord supScriptShift;
  nsPresentationData presentationData;
  aFrame->GetPresentationData(presentationData);
  if ( aFrame->StyleFont()->mScriptLevel == 0 &&
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
    std::max(minSupScriptShift,std::max(supScriptShift,minShiftFromXHeight));

  
  
  
  
  

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
    std::max(bmBase.ascent, (bmSupScript.ascent + supScriptShift));
  boundingMetrics.descent =
   std::max(bmBase.descent, (bmSubScript.descent + subScriptShift));

  
  
  
  
  nscoord italicCorrection;
  GetItalicCorrection(bmBase, italicCorrection);
  italicCorrection += onePixel;
  boundingMetrics.width = bmBase.width + aScriptSpace +
    std::max((italicCorrection + bmSupScript.width), bmSubScript.width);
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = bmBase.width +
    std::max((italicCorrection + bmSupScript.rightBearing), bmSubScript.rightBearing);
  aFrame->SetBoundingMetrics(boundingMetrics);

  
  aDesiredSize.ascent =
    std::max(baseSize.ascent, 
       std::max(subScriptSize.ascent - subScriptShift,
              supScriptSize.ascent + supScriptShift));
  aDesiredSize.height = aDesiredSize.ascent +
    std::max(baseSize.height - baseSize.ascent,
       std::max(subScriptSize.height - subScriptSize.ascent + subScriptShift, 
              supScriptSize.height - subScriptSize.ascent - supScriptShift));
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    
    dx = aFrame->MirrorIfRTL(aDesiredSize.width, baseSize.width, 0);
    dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild(baseFrame, aPresContext, nullptr,
                      baseSize, dx, dy, 0);
    
    dx = aFrame->MirrorIfRTL(aDesiredSize.width, subScriptSize.width,
                             bmBase.width);
    dy = aDesiredSize.ascent - (subScriptSize.ascent - subScriptShift);
    FinishReflowChild(subScriptFrame, aPresContext, nullptr,
                      subScriptSize, dx, dy, 0);
    
    dx = aFrame->MirrorIfRTL(aDesiredSize.width, supScriptSize.width,
                             bmBase.width + italicCorrection);
    dy = aDesiredSize.ascent - (supScriptSize.ascent + supScriptShift);
    FinishReflowChild(supScriptFrame, aPresContext, nullptr,
                      supScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
