





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"

#include "nsMathMLmsubFrame.h"
#include <algorithm>





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
                          bool                 aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  
  nscoord scriptSpace = nsPresContext::CSSPointsToAppUnits(0.5f); 

  
  
  
  
  
  
  
  
  
  
  
  nscoord subScriptShift = 0;
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::subscriptshift_, value);
  if (!value.IsEmpty()) {
    ParseNumericValue(value, &subScriptShift, 0, PresContext(), mStyleContext);
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
                                   bool                 aPlaceOrigin,
                                   nsHTMLReflowMetrics& aDesiredSize,
                                   nsMathMLContainerFrame* aFrame,
                                   nscoord              aUserSubScriptShift,
                                   nscoord              aScriptSpace)
{
  
  aScriptSpace = std::max(nsPresContext::CSSPixelsToAppUnits(1), aScriptSpace);

  
  

  nsBoundingMetrics bmBase, bmSubScript;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsIFrame* baseFrame = aFrame->GetFirstPrincipalChild();
  nsIFrame* subScriptFrame = nullptr;
  if (baseFrame)
    subScriptFrame = baseFrame->GetNextSibling();
  if (!baseFrame || !subScriptFrame || subScriptFrame->GetNextSibling()) {
    
    if (aPlaceOrigin) {
      aFrame->ReportChildCountError();
    }
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);

  
  nscoord subDrop;
  GetSubDropFromChild(subScriptFrame, subDrop);
  
  nscoord minSubScriptShift = bmBase.descent + subDrop;

  
  
  
  
  
  nscoord xHeight = 0;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(baseFrame, getter_AddRefs(fm));

  xHeight = fm->XHeight();
  nscoord minShiftFromXHeight = (nscoord) 
    (bmSubScript.ascent - (4.0f/5.0f) * xHeight);

  
  
  
  
  nscoord subScriptShift, dummy;
  
  GetSubScriptShifts (fm, subScriptShift, dummy);

  subScriptShift = 
    std::max(subScriptShift, aUserSubScriptShift);

  
  
  nscoord actualSubScriptShift = 
    std::max(minSubScriptShift,std::max(subScriptShift,minShiftFromXHeight));
  
  nsBoundingMetrics boundingMetrics;
  boundingMetrics.ascent = 
    std::max(bmBase.ascent, bmSubScript.ascent - actualSubScriptShift);
  boundingMetrics.descent = 
    std::max(bmBase.descent, bmSubScript.descent + actualSubScriptShift);

  
  boundingMetrics.width = bmBase.width + bmSubScript.width + aScriptSpace;
  boundingMetrics.leftBearing = bmBase.leftBearing;
  boundingMetrics.rightBearing = std::max(bmBase.rightBearing, bmBase.width +
    std::max(bmSubScript.width + aScriptSpace, bmSubScript.rightBearing));
  aFrame->SetBoundingMetrics (boundingMetrics);

  
  aDesiredSize.ascent = 
    std::max(baseSize.ascent, subScriptSize.ascent - actualSubScriptShift);
  aDesiredSize.height = aDesiredSize.ascent +
    std::max(baseSize.height - baseSize.ascent,
           subScriptSize.height - subScriptSize.ascent + actualSubScriptShift);
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  if (aPlaceOrigin) {
    nscoord dx, dy;
    
    dx = aFrame->MirrorIfRTL(aDesiredSize.width, baseSize.width, 0);
    dy = aDesiredSize.ascent - baseSize.ascent;
    FinishReflowChild (baseFrame, aPresContext, nullptr, baseSize, dx, dy, 0);
    
    dx = aFrame->MirrorIfRTL(aDesiredSize.width, subScriptSize.width,
                             bmBase.width);
    dy = aDesiredSize.ascent - (subScriptSize.ascent - actualSubScriptShift);
    FinishReflowChild (subScriptFrame, aPresContext, nullptr, subScriptSize, dx, dy, 0);
  }

  return NS_OK;
}
