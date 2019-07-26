





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsRenderingContext.h"

#include "nsMathMLmmultiscriptsFrame.h"
#include <algorithm>








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

  
  
  

  if (mContent->Tag() == nsGkAtoms::msup_)
    return NS_OK;

  int32_t count = 0;
  bool isSubScript = true;

  nsAutoTArray<nsIFrame*, 8> subScriptFrames;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (childFrame->GetContent()->Tag() == nsGkAtoms::mprescripts_) {
      
    } else if (0 == count) {
      
    } else {
      
      if (isSubScript) {
        
        subScriptFrames.AppendElement(childFrame);
      } else {
        
      }
      isSubScript = !isSubScript;
    }
    count++;
    childFrame = childFrame->GetNextSibling();
  }
  for (int32_t i = subScriptFrames.Length() - 1; i >= 0; i--) {
    childFrame = subScriptFrames[i];
    PropagatePresentationDataFor(childFrame,
      NS_MATHML_COMPRESSED, NS_MATHML_COMPRESSED);
  }

  return NS_OK;
}

 nsresult
nsMathMLmmultiscriptsFrame::Place(nsRenderingContext& aRenderingContext,
                                  bool                 aPlaceOrigin,
                                  nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord subScriptShift = 0;
  nscoord supScriptShift = 0;
  nsIAtom* tag = mContent->Tag();

  
  
  
  
  
  
  
  
  
  
  
  nsAutoString value;
  if (tag != nsGkAtoms::msup_) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::subscriptshift_, value);
    if (!value.IsEmpty()) {
      ParseNumericValue(value, &subScriptShift, 0, PresContext(),
                        mStyleContext);
    }
  }
  
  
  
  
  
  
  
  
  
  
  
  if (tag != nsGkAtoms::msub_) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::superscriptshift_, value);
    if (!value.IsEmpty()) {
      ParseNumericValue(value, &supScriptShift, 0, PresContext(),
                        mStyleContext);
    }
  }
  
  
  nscoord scriptSpace = nsPresContext::CSSPointsToAppUnits(0.5f);

  return PlaceMultiScript(PresContext(), aRenderingContext, aPlaceOrigin,
                          aDesiredSize, this, subScriptShift, supScriptShift,
                          scriptSpace);
}



nsresult
nsMathMLmmultiscriptsFrame::PlaceMultiScript(nsPresContext*      aPresContext,
                                        nsRenderingContext& aRenderingContext,
                                        bool                 aPlaceOrigin,
                                        nsHTMLReflowMetrics& aDesiredSize,
                                        nsMathMLContainerFrame* aFrame,
                                        nscoord              aUserSubScriptShift,
                                        nscoord              aUserSupScriptShift,
                                        nscoord              aScriptSpace)
{
  nsIAtom* tag = aFrame->GetContent()->Tag();

  
  
  
  if (tag == nsGkAtoms::mover_)
    tag = nsGkAtoms::msup_;
  else if (tag == nsGkAtoms::munder_)
    tag = nsGkAtoms::msub_;
  else if (tag  == nsGkAtoms::munderover_)
    tag = nsGkAtoms::msubsup_;

  nsBoundingMetrics bmFrame;
  nsHTMLReflowMetrics frameSize;

  nscoord minShiftFromXHeight, subDrop, supDrop;

  
  
  
  

  nsIFrame* baseFrame = aFrame->GetFirstPrincipalChild();

  if (!baseFrame) {
    if (tag == nsGkAtoms::mmultiscripts_)
      aFrame->ReportErrorToConsole("NoBase");
    else
      aFrame->ReportChildCountError();
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }


  
  const nsStyleFont* font = aFrame->StyleFont();
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(baseFrame, getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  nscoord xHeight = fm->XHeight();

  nscoord ruleSize;
  GetRuleThickness (aRenderingContext, fm, ruleSize);

  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  aScriptSpace = std::max(onePixel, aScriptSpace);

  
  

  
  
  
  
  nscoord subScriptShift1, subScriptShift2;

  
  GetSubScriptShifts (fm, subScriptShift1, subScriptShift2);
  nscoord subScriptShift;
  if (tag == nsGkAtoms::msub_) {
    subScriptShift = subScriptShift1;
  } else {
    subScriptShift = std::max(subScriptShift1, subScriptShift2);
  }
  if (0 < aUserSubScriptShift) {
    
    subScriptShift = std::max(subScriptShift, aUserSubScriptShift);
  }

  
  

  
  
  
  
  
  
  nscoord supScriptShift1, supScriptShift2, supScriptShift3;
  
  GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);

  
  
  nsPresentationData presentationData;
  aFrame->GetPresentationData(presentationData);
  nscoord supScriptShift;
  if ( font->mScriptLevel == 0 &&
       NS_MATHML_IS_DISPLAYSTYLE(presentationData.flags) &&
      !NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    
    supScriptShift = supScriptShift1;
  } else if (NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
    
    supScriptShift = supScriptShift3;
  } else {
    
    supScriptShift = supScriptShift2;
  }

  if (0 < aUserSupScriptShift) {
    
    supScriptShift = std::max(supScriptShift, aUserSupScriptShift);
  }

  
  
  

  nscoord width = 0, prescriptsWidth = 0, rightBearing = 0;
  nscoord minSubScriptShift = 0, minSupScriptShift = 0;
  nscoord trySubScriptShift = subScriptShift;
  nscoord trySupScriptShift = supScriptShift;
  nscoord maxSubScriptShift = subScriptShift;
  nscoord maxSupScriptShift = supScriptShift;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics subScriptSize;
  nsHTMLReflowMetrics supScriptSize;
  baseFrame = nullptr;
  nsIFrame* subScriptFrame = nullptr;
  nsIFrame* supScriptFrame = nullptr;
  nsIFrame* prescriptsFrame = nullptr; 

  bool firstPrescriptsPair = false;
  nsBoundingMetrics bmBase, bmSubScript, bmSupScript;
  nscoord italicCorrection = 0;

  nsBoundingMetrics boundingMetrics;
  boundingMetrics.width = 0;
  boundingMetrics.ascent = boundingMetrics.descent = -0x7FFFFFFF;
  nscoord ascent = -0x7FFFFFFF, descent = -0x7FFFFFFF;
  aDesiredSize.width = aDesiredSize.height = 0;

  int32_t count = 0;
  bool foundNoneTag = false;

  
  
  bool isSubScript = (tag != nsGkAtoms::msup_);

  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  while (childFrame) {
    nsIAtom* childTag = childFrame->GetContent()->Tag();
    if (childTag == nsGkAtoms::mprescripts_) {
      if (tag != nsGkAtoms::mmultiscripts_) {
        if (aPlaceOrigin) {
          aFrame->ReportInvalidChildError(childTag);
        }
        return aFrame->ReflowError(aRenderingContext, aDesiredSize);
      }
      if (prescriptsFrame) {
        
        
        if (aPlaceOrigin) {
          aFrame->ReportErrorToConsole("DuplicateMprescripts");
        }
        return aFrame->ReflowError(aRenderingContext, aDesiredSize);
      }
      if (!isSubScript) {
        if (aPlaceOrigin) {
          aFrame->ReportErrorToConsole("SubSupMismatch");
        }
        return aFrame->ReflowError(aRenderingContext, aDesiredSize);
      }

      prescriptsFrame = childFrame;
      firstPrescriptsPair = true;
    } else if (0 == count) {
      

      if (childTag == nsGkAtoms::none) {
        if (tag == nsGkAtoms::mmultiscripts_) {
          if (aPlaceOrigin) {
            aFrame->ReportErrorToConsole("NoBase");
          }
          return aFrame->ReflowError(aRenderingContext, aDesiredSize);
        } else {
          
          foundNoneTag = true;
        }
      }
      baseFrame = childFrame;
      GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);

      if (tag != nsGkAtoms::msub_) {
        
        
        GetItalicCorrection(bmBase, italicCorrection);
        
        
        
        italicCorrection += onePixel;
      }

      
      
      
      boundingMetrics.width = bmBase.width + italicCorrection;
      boundingMetrics.rightBearing = bmBase.rightBearing;
      boundingMetrics.leftBearing = bmBase.leftBearing; 
    } else {
      
      if ( childTag == nsGkAtoms::none) {
        foundNoneTag = true;
      }

      if (isSubScript) {
        
        subScriptFrame = childFrame;
        GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
        
        GetSubDropFromChild (subScriptFrame, subDrop);
        
        minSubScriptShift = bmBase.descent + subDrop;
        trySubScriptShift = std::max(minSubScriptShift,subScriptShift);
        boundingMetrics.descent =
          std::max(boundingMetrics.descent,bmSubScript.descent);
        descent = std::max(descent,subScriptSize.height - subScriptSize.ascent);
        if (bmSubScript.width)
          width = bmSubScript.width + aScriptSpace;
        rightBearing = bmSubScript.rightBearing;

        if (tag == nsGkAtoms::msub_) {
          boundingMetrics.rightBearing = boundingMetrics.width + rightBearing;
          boundingMetrics.width += width;

          
          
          nscoord minShiftFromXHeight = (nscoord) 
            (bmSubScript.ascent - (4.0f/5.0f) * xHeight);
          maxSubScriptShift = std::max(trySubScriptShift,minShiftFromXHeight);

          maxSubScriptShift = std::max(maxSubScriptShift, trySubScriptShift);
          trySubScriptShift = subScriptShift;
        }
      } else {
        
        supScriptFrame = childFrame;
        GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);
        
        GetSupDropFromChild (supScriptFrame, supDrop);
        
        minSupScriptShift = bmBase.ascent - supDrop;
        
        
        minShiftFromXHeight = NSToCoordRound
          ((bmSupScript.descent + (1.0f/4.0f) * xHeight));
        trySupScriptShift =
          std::max(minSupScriptShift,std::max(minShiftFromXHeight,supScriptShift));
        boundingMetrics.ascent =
          std::max(boundingMetrics.ascent,bmSupScript.ascent);
        ascent = std::max(ascent,supScriptSize.ascent);
        if (bmSupScript.width)
          width = std::max(width, bmSupScript.width + aScriptSpace);
        rightBearing = std::max(rightBearing, bmSupScript.rightBearing);

        if (!prescriptsFrame) { 
          boundingMetrics.rightBearing = boundingMetrics.width + rightBearing;
          boundingMetrics.width += width;
        } else {
          prescriptsWidth += width;
          if (firstPrescriptsPair) {
            firstPrescriptsPair = false;
            boundingMetrics.leftBearing =
              std::min(bmSubScript.leftBearing, bmSupScript.leftBearing);
          }
        }
        width = rightBearing = 0;

        
        
        
        if (tag == nsGkAtoms::mmultiscripts_ || 
            tag == nsGkAtoms::msubsup_) {
          nscoord gap =
            (trySupScriptShift - bmSupScript.descent) -
            (bmSubScript.ascent - trySubScriptShift);
          if (gap < 4.0f * ruleSize) {
            
            trySubScriptShift += NSToCoordRound ((4.0f * ruleSize) - gap);
          }

          
          
          gap = NSToCoordRound ((4.0f/5.0f) * xHeight -
                  (trySupScriptShift - bmSupScript.descent));
          if (gap > 0) {
            trySupScriptShift += gap;
            trySubScriptShift -= gap;
          }
        }

        maxSubScriptShift = std::max(maxSubScriptShift, trySubScriptShift);
        maxSupScriptShift = std::max(maxSupScriptShift, trySupScriptShift);

        trySubScriptShift = subScriptShift;
        trySupScriptShift = supScriptShift;
      }

      isSubScript = !isSubScript;
    }
    count++;
    childFrame = childFrame->GetNextSibling();
  }

  
  if ((count != 2 && (tag == nsGkAtoms::msup_ || tag == nsGkAtoms::msub_)) ||
      (count != 3 && tag == nsGkAtoms::msubsup_) || !baseFrame ||
      (foundNoneTag && tag != nsGkAtoms::mmultiscripts_) ||
      (!isSubScript && tag == nsGkAtoms::mmultiscripts_)) {
    
    if (aPlaceOrigin) {
      if ((count != 2 && (tag == nsGkAtoms::msup_ || 
          tag == nsGkAtoms::msub_)) ||
          (count != 3 && tag == nsGkAtoms::msubsup_ )) {
        aFrame->ReportChildCountError();
      } else if (foundNoneTag && tag != nsGkAtoms::mmultiscripts_) {
        aFrame->ReportInvalidChildError(nsGkAtoms::none);
      } else if (!baseFrame) {
        aFrame->ReportErrorToConsole("NoBase");
      } else {
        aFrame->ReportErrorToConsole("SubSupMismatch");
      }
    }
    return aFrame->ReflowError(aRenderingContext, aDesiredSize);
  }

  
  boundingMetrics.rightBearing += prescriptsWidth;
  boundingMetrics.width += prescriptsWidth;

  
  boundingMetrics.ascent =
    std::max(boundingMetrics.ascent+maxSupScriptShift,bmBase.ascent);
  boundingMetrics.descent =
    std::max(boundingMetrics.descent+maxSubScriptShift,bmBase.descent);
  aFrame->SetBoundingMetrics(boundingMetrics);

  
  aDesiredSize.ascent =
    std::max(ascent+maxSupScriptShift,baseSize.ascent);
  aDesiredSize.height = aDesiredSize.ascent +
    std::max(descent+maxSubScriptShift,baseSize.height - baseSize.ascent);
  aDesiredSize.width = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.ascent));

  
  

  
  
  

  if (aPlaceOrigin) {
    nscoord dx = 0, dy = 0;

    
    
    
    if (tag == nsGkAtoms::msub_ || tag == nsGkAtoms::msup_)
      count = 1;
    else
      count = 0;
    childFrame = prescriptsFrame;
    do {
      if (!childFrame) { 
        
        childFrame = baseFrame;
        dy = aDesiredSize.ascent - baseSize.ascent;
        FinishReflowChild (baseFrame, aPresContext, nullptr, baseSize,
                           aFrame->MirrorIfRTL(aDesiredSize.width,
                                               baseSize.width,
                                               dx),
                           dy, 0);
        dx += bmBase.width + italicCorrection;
      } else if (prescriptsFrame != childFrame) {
        
        if (0 == count) {
          subScriptFrame = childFrame;
          count = 1;
        } else if (1 == count) {
          if (tag != nsGkAtoms::msub_)
            supScriptFrame = childFrame;
          count = 0;

          
          
          if (subScriptFrame)
            GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
          if (supScriptFrame)
            GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);

          
          width = std::max(subScriptSize.width, supScriptSize.width);

          if (subScriptFrame) {
            dy = aDesiredSize.ascent - subScriptSize.ascent +
              maxSubScriptShift;
            FinishReflowChild (subScriptFrame, aPresContext, nullptr,
                               subScriptSize,
                               aFrame->MirrorIfRTL(aDesiredSize.width,
                                                   subScriptSize.width,
                                                   dx + (width-subScriptSize.width)/2),
                               dy, 0);
          }

          if (supScriptFrame) {
            dy = aDesiredSize.ascent - supScriptSize.ascent -
              maxSupScriptShift;
            FinishReflowChild (supScriptFrame, aPresContext, nullptr,
                               supScriptSize,
                               aFrame->MirrorIfRTL(aDesiredSize.width,
                                                   supScriptSize.width,
                                                   dx + (width-supScriptSize.width)/2),
                               dy, 0);
          }
          dx += width + aScriptSpace;
        }
      }
      childFrame = childFrame->GetNextSibling();
    } while (prescriptsFrame != childFrame);
  }

  return NS_OK;
}
