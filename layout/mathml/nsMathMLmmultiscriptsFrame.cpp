








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmmultiscriptsFrame.h"





nsIFrame*
NS_NewMathMLmmultiscriptsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmmultiscriptsFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmmultiscriptsFrame)

nsMathMLmmultiscriptsFrame::~nsMathMLmmultiscriptsFrame()
{
}

NS_IMETHODIMP
nsMathMLmmultiscriptsFrame::TransmitAutomaticData()
{
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  
  
  
  UpdatePresentationDataFromChildAt(1, -1,
    ~NS_MATHML_DISPLAYSTYLE, NS_MATHML_DISPLAYSTYLE);

  
  
  
  PRInt32 count = 0;
  PRBool isSubScript = PR_FALSE;
  nsAutoTArray<nsIFrame*, 8> subScriptFrames;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (childFrame->GetContent()->Tag() == nsGkAtoms::mprescripts_) {
      
    }
    else if (0 == count) {
      
    }
    else {
      
      if (isSubScript) {
        
        subScriptFrames.AppendElement(childFrame);
      }
      else {
        
      }
      isSubScript = !isSubScript;
    }
    count++;
    childFrame = childFrame->GetNextSibling();
  }
  for (PRInt32 i = subScriptFrames.Length() - 1; i >= 0; i--) {
    childFrame = subScriptFrames[i];
    PropagatePresentationDataFor(childFrame,
      NS_MATHML_COMPRESSED, NS_MATHML_COMPRESSED);
  }

  return NS_OK;
}

void
nsMathMLmmultiscriptsFrame::ProcessAttributes()
{
  mSubScriptShift = 0;
  mSupScriptShift = 0;

  
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::subscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      mSubScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }
  
  GetAttribute(mContent, mPresentationData.mstyle,
               nsGkAtoms::superscriptshift_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) && cssValue.IsLengthUnit()) {
      mSupScriptShift = CalcLength(PresContext(), mStyleContext, cssValue);
    }
  }
}

 nsresult
nsMathMLmmultiscriptsFrame::Place(nsIRenderingContext& aRenderingContext,
                                  PRBool               aPlaceOrigin,
                                  nsHTMLReflowMetrics& aDesiredSize)
{
  
  

  nscoord minShiftFromXHeight, subDrop, supDrop;

  
  
  
  

  ProcessAttributes();

  
  const nsStyleFont* font = GetStyleFont();
  aRenderingContext.SetFont(font->mFont, nsnull,
                            PresContext()->GetUserFontSet());
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));

  nscoord xHeight;
  fm->GetXHeight (xHeight);

  nscoord ruleSize;
  GetRuleThickness (aRenderingContext, fm, ruleSize);

  
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  nscoord scriptSpace = PR_MAX(PresContext()->PointsToAppUnits(0.5f), onePixel);

  
  

  
  
  
  
  nscoord subScriptShift1, subScriptShift2;

  
  GetSubScriptShifts (fm, subScriptShift1, subScriptShift2);
  if (0 < mSubScriptShift) {
    
    float scaler = ((float) subScriptShift2) / subScriptShift1;
    subScriptShift1 = PR_MAX(subScriptShift1, mSubScriptShift);
    subScriptShift2 = NSToCoordRound(scaler * subScriptShift1);
  }
  
  nscoord subScriptShift = PR_MAX(subScriptShift1,subScriptShift2);

  
  

  
  
  
  
  
  
  nscoord supScriptShift1, supScriptShift2, supScriptShift3;
  
  GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);
  if (0 < mSupScriptShift) {
    
    float scaler2 = ((float) supScriptShift2) / supScriptShift1;
    float scaler3 = ((float) supScriptShift3) / supScriptShift1;
    supScriptShift1 = PR_MAX(supScriptShift1, mSupScriptShift);
    supScriptShift2 = NSToCoordRound(scaler2 * supScriptShift1);
    supScriptShift3 = NSToCoordRound(scaler3 * supScriptShift1);
  }

  
  
  nscoord supScriptShift;
  if ( font->mScriptLevel == 0 &&
       NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags) &&
      !NS_MATHML_IS_COMPRESSED(mPresentationData.flags)) {
    
    supScriptShift = supScriptShift1;
  }
  else if (NS_MATHML_IS_COMPRESSED(mPresentationData.flags)) {
    
    supScriptShift = supScriptShift3;
  }
  else {
    
    supScriptShift = supScriptShift2;
  }

  
  
  

  nscoord width = 0, prescriptsWidth = 0, rightBearing = 0;
  nsIFrame* mprescriptsFrame = nsnull; 
  PRBool isSubScript = PR_FALSE;
  nscoord minSubScriptShift = 0, minSupScriptShift = 0;
  nscoord trySubScriptShift = subScriptShift;
  nscoord trySupScriptShift = supScriptShift;
  nscoord maxSubScriptShift = subScriptShift;
  nscoord maxSupScriptShift = supScriptShift;
  PRInt32 count = 0;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsHTMLReflowMetrics supScriptSize;
  nsIFrame* baseFrame = nsnull;
  nsIFrame* subScriptFrame = nsnull;
  nsIFrame* supScriptFrame = nsnull;

  PRBool firstPrescriptsPair = PR_FALSE;
  nsBoundingMetrics bmBase, bmSubScript, bmSupScript;
  nscoord italicCorrection = 0;

  mBoundingMetrics.width = 0;
  mBoundingMetrics.ascent = mBoundingMetrics.descent = -0x7FFFFFFF;
  nscoord ascent = -0x7FFFFFFF, descent = -0x7FFFFFFF;
  aDesiredSize.width = aDesiredSize.height = 0;

  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (childFrame->GetContent()->Tag() == nsGkAtoms::mprescripts_) {
      if (mprescriptsFrame) {
        
        
        return ReflowError(aRenderingContext, aDesiredSize);
      }
      mprescriptsFrame = childFrame;
      firstPrescriptsPair = PR_TRUE;
    }
    else {
      if (0 == count) {
        
        baseFrame = childFrame;
        GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
        GetItalicCorrection(bmBase, italicCorrection);
        
        italicCorrection += onePixel;

        
        
        
        mBoundingMetrics.width = bmBase.width + italicCorrection;
        mBoundingMetrics.rightBearing = bmBase.rightBearing;
        mBoundingMetrics.leftBearing = bmBase.leftBearing; 
      }
      else {
        
        if (isSubScript) {
          
          subScriptFrame = childFrame;
          GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
          
          GetSubDropFromChild (subScriptFrame, subDrop);
          
          minSubScriptShift = bmBase.descent + subDrop;
          trySubScriptShift = PR_MAX(minSubScriptShift,subScriptShift);
          mBoundingMetrics.descent =
            PR_MAX(mBoundingMetrics.descent,bmSubScript.descent);
          descent = PR_MAX(descent,subScriptSize.height - subScriptSize.ascent);
          width = bmSubScript.width + scriptSpace;
          rightBearing = bmSubScript.rightBearing;
        }
        else {
          
          supScriptFrame = childFrame;
          GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);
          
          GetSupDropFromChild (supScriptFrame, supDrop);
          
          minSupScriptShift = bmBase.ascent - supDrop;
          
          
          minShiftFromXHeight = NSToCoordRound
            ((bmSupScript.descent + (1.0f/4.0f) * xHeight));
          trySupScriptShift =
            PR_MAX(minSupScriptShift,PR_MAX(minShiftFromXHeight,supScriptShift));
          mBoundingMetrics.ascent =
            PR_MAX(mBoundingMetrics.ascent,bmSupScript.ascent);
          ascent = PR_MAX(ascent,supScriptSize.ascent);
          width = PR_MAX(width, bmSupScript.width + scriptSpace);
          rightBearing = PR_MAX(rightBearing, bmSupScript.rightBearing);

          if (!mprescriptsFrame) { 
            mBoundingMetrics.rightBearing = mBoundingMetrics.width + rightBearing;
            mBoundingMetrics.width += width;
          }
          else {
            prescriptsWidth += width;
            if (firstPrescriptsPair) {
              firstPrescriptsPair = PR_FALSE;
              mBoundingMetrics.leftBearing =
                PR_MIN(bmSubScript.leftBearing, bmSupScript.leftBearing);
            }
          }
          width = rightBearing = 0;

          
          
          
          nscoord gap =
            (trySupScriptShift - bmSupScript.descent) -
            (bmSubScript.ascent - trySubScriptShift);
          if (gap < 4.0f * ruleSize) {
            
            trySubScriptShift += NSToCoordRound ((4.0f * ruleSize) - gap);
          }

          
          
          gap = NSToCoordRound ((4.0f/5.0f) * xHeight -
                  (trySupScriptShift - bmSupScript.descent));
          if (gap > 0.0f) {
            trySupScriptShift += gap;
            trySubScriptShift -= gap;
          }
          
          maxSubScriptShift = PR_MAX(maxSubScriptShift, trySubScriptShift);
          maxSupScriptShift = PR_MAX(maxSupScriptShift, trySupScriptShift);

          trySubScriptShift = subScriptShift;
          trySupScriptShift = supScriptShift;
        }
      }

      isSubScript = !isSubScript;
    }
    count++;
    childFrame = childFrame->GetNextSibling();
  }
  
  if ((0 != width) || !baseFrame || !subScriptFrame || !supScriptFrame) {
    
    return ReflowError(aRenderingContext, aDesiredSize);
  }

  
  mBoundingMetrics.rightBearing += prescriptsWidth;
  mBoundingMetrics.width += prescriptsWidth;

  
  mBoundingMetrics.ascent =
    PR_MAX(mBoundingMetrics.ascent+maxSupScriptShift,bmBase.ascent);
  mBoundingMetrics.descent =
    PR_MAX(mBoundingMetrics.descent+maxSubScriptShift,bmBase.descent);

  
  aDesiredSize.ascent =
    PR_MAX(ascent+maxSupScriptShift,baseSize.ascent);
  aDesiredSize.height = aDesiredSize.ascent +
    PR_MAX(descent+maxSubScriptShift,baseSize.height - baseSize.ascent);
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  

  
  
  

  if (aPlaceOrigin) {
    nscoord dx = 0, dy = 0;

    count = 0;
    childFrame = mprescriptsFrame;
    do {
      if (!childFrame) { 
        
        childFrame = baseFrame;
        dy = aDesiredSize.ascent - baseSize.ascent;
        FinishReflowChild (baseFrame, PresContext(), nsnull, baseSize, dx, dy, 0);
        dx += bmBase.width + italicCorrection;
      }
      else if (mprescriptsFrame != childFrame) {
        
        if (0 == count) {
          subScriptFrame = childFrame;
          count = 1;
        }
        else if (1 == count) {
          supScriptFrame = childFrame;
          count = 0;

          
          
          GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
          GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);

          
          width = PR_MAX(subScriptSize.width, supScriptSize.width);

          dy = aDesiredSize.ascent - subScriptSize.ascent +
            maxSubScriptShift;
          FinishReflowChild (subScriptFrame, PresContext(), nsnull, subScriptSize,
                             dx + (width-subScriptSize.width)/2, dy, 0);

          dy = aDesiredSize.ascent - supScriptSize.ascent -
            maxSupScriptShift;
          FinishReflowChild (supScriptFrame, PresContext(), nsnull, supScriptSize,
                             dx + (width-supScriptSize.width)/2, dy, 0);

          dx += width + scriptSpace;
        }
      }
      childFrame = childFrame->GetNextSibling();
    } while (mprescriptsFrame != childFrame);
  }

  return NS_OK;
}
