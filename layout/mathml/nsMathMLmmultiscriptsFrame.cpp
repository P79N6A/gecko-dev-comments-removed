





#include "nsMathMLmmultiscriptsFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include <algorithm>

using mozilla::WritingMode;








nsIFrame*
NS_NewMathMLmmultiscriptsFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmmultiscriptsFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmmultiscriptsFrame)

nsMathMLmmultiscriptsFrame::~nsMathMLmmultiscriptsFrame()
{
}

uint8_t
nsMathMLmmultiscriptsFrame::ScriptIncrement(nsIFrame* aFrame)
{
  if (!aFrame)
    return 0;
  if (mFrames.ContainsFrame(aFrame)) {
    if (mFrames.FirstChild() == aFrame ||
        aFrame->GetContent()->IsMathMLElement(nsGkAtoms::mprescripts_)) {
      return 0; 
    }
    return 1;
  }
  return 0; 
}

NS_IMETHODIMP
nsMathMLmmultiscriptsFrame::TransmitAutomaticData()
{
  
  mPresentationData.baseFrame = mFrames.FirstChild();
  GetEmbellishDataFrom(mPresentationData.baseFrame, mEmbellishData);

  
  
  

  int32_t count = 0;
  bool isSubScript = !mContent->IsMathMLElement(nsGkAtoms::msup_);

  nsAutoTArray<nsIFrame*, 8> subScriptFrames;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    if (childFrame->GetContent()->IsMathMLElement(nsGkAtoms::mprescripts_)) {
      
    } else if (0 == count) {
      
    } else {
      
      if (isSubScript) {
        
        subScriptFrames.AppendElement(childFrame);
      } else {
        
      }
      PropagateFrameFlagFor(childFrame, NS_FRAME_MATHML_SCRIPT_DESCENDANT);
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
  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(this);

  
  
  
  
  
  
  
  
  
  
  
  nsAutoString value;
  if (!mContent->IsMathMLElement(nsGkAtoms::msup_)) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::subscriptshift_, value);
    if (!value.IsEmpty()) {
      ParseNumericValue(value, &subScriptShift, 0, PresContext(),
                        mStyleContext, fontSizeInflation);
    }
  }
  
  
  
  
  
  
  
  
  
  
  
  if (!mContent->IsMathMLElement(nsGkAtoms::msub_)) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::superscriptshift_, value);
    if (!value.IsEmpty()) {
      ParseNumericValue(value, &supScriptShift, 0, PresContext(),
                        mStyleContext, fontSizeInflation);
    }
  }
  return PlaceMultiScript(PresContext(), aRenderingContext, aPlaceOrigin,
                          aDesiredSize, this, subScriptShift, supScriptShift,
                          fontSizeInflation);
}



nsresult
nsMathMLmmultiscriptsFrame::PlaceMultiScript(nsPresContext*      aPresContext,
                                        nsRenderingContext& aRenderingContext,
                                        bool                 aPlaceOrigin,
                                        nsHTMLReflowMetrics& aDesiredSize,
                                        nsMathMLContainerFrame* aFrame,
                                        nscoord              aUserSubScriptShift,
                                        nscoord              aUserSupScriptShift,
                                        float                aFontSizeInflation)
{
  nsIAtom* tag = aFrame->GetContent()->NodeInfo()->NameAtom();

  
  
  
  if (aFrame->GetContent()->IsMathMLElement(nsGkAtoms::mover_))
    tag = nsGkAtoms::msup_;
  else if (aFrame->GetContent()->IsMathMLElement(nsGkAtoms::munder_))
    tag = nsGkAtoms::msub_;
  else if (aFrame->GetContent()->IsMathMLElement(nsGkAtoms::munderover_))
    tag = nsGkAtoms::msubsup_;

  nsBoundingMetrics bmFrame;

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
  nsLayoutUtils::GetFontMetricsForFrame(baseFrame, getter_AddRefs(fm),
                                        aFontSizeInflation);

  nscoord xHeight = fm->XHeight();

  nscoord oneDevPixel = fm->AppUnitsPerDevPixel();
  gfxFont* mathFont = fm->GetThebesFontGroup()->GetFirstMathFont();
  
  nscoord scriptSpace;
  if (mathFont) {
    scriptSpace =
      mathFont->GetMathConstant(gfxFontEntry::SpaceAfterScript, oneDevPixel);
  } else {
    
    scriptSpace = nsPresContext::CSSPointsToAppUnits(0.5f);
  }

  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  scriptSpace = std::max(onePixel, scriptSpace);

  
  

  nscoord subScriptShift;
  if (mathFont) {
    
    
    subScriptShift =
      mathFont->GetMathConstant(gfxFontEntry::SubscriptShiftDown, oneDevPixel);
  } else {
    
    
    
    
    nscoord subScriptShift1, subScriptShift2;
    
    GetSubScriptShifts (fm, subScriptShift1, subScriptShift2);
    if (tag == nsGkAtoms::msub_) {
      subScriptShift = subScriptShift1;
    } else {
      subScriptShift = std::max(subScriptShift1, subScriptShift2);
    }
  }

  if (0 < aUserSubScriptShift) {
    
    subScriptShift = std::max(subScriptShift, aUserSubScriptShift);
  }

  
  

  nscoord supScriptShift;
  nsPresentationData presentationData;
  aFrame->GetPresentationData(presentationData);
  if (mathFont) {
    
    
    supScriptShift = mathFont->
      GetMathConstant(NS_MATHML_IS_COMPRESSED(presentationData.flags) ?
                      gfxFontEntry::SuperscriptShiftUpCramped :
                      gfxFontEntry::SuperscriptShiftUp,
                      oneDevPixel);
  } else {
    
    
    
    
    
    
    nscoord supScriptShift1, supScriptShift2, supScriptShift3;
    
    GetSupScriptShifts (fm, supScriptShift1, supScriptShift2, supScriptShift3);

    
    
    if (font->mScriptLevel == 0 &&
        font->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK &&
        !NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
      
      supScriptShift = supScriptShift1;
    } else if (NS_MATHML_IS_COMPRESSED(presentationData.flags)) {
      
      supScriptShift = supScriptShift3;
    } else {
      
      supScriptShift = supScriptShift2;
    }
  }

  if (0 < aUserSupScriptShift) {
    
    supScriptShift = std::max(supScriptShift, aUserSupScriptShift);
  }

  
  
  

  const WritingMode wm(aDesiredSize.GetWritingMode());
  nscoord width = 0, prescriptsWidth = 0, rightBearing = 0;
  nscoord minSubScriptShift = 0, minSupScriptShift = 0;
  nscoord trySubScriptShift = subScriptShift;
  nscoord trySupScriptShift = supScriptShift;
  nscoord maxSubScriptShift = subScriptShift;
  nscoord maxSupScriptShift = supScriptShift;
  nsHTMLReflowMetrics baseSize(wm);
  nsHTMLReflowMetrics subScriptSize(wm);
  nsHTMLReflowMetrics supScriptSize(wm);
  nsHTMLReflowMetrics multiSubSize(wm), multiSupSize(wm);
  baseFrame = nullptr;
  nsIFrame* subScriptFrame = nullptr;
  nsIFrame* supScriptFrame = nullptr;
  nsIFrame* prescriptsFrame = nullptr; 

  bool firstPrescriptsPair = false;
  nsBoundingMetrics bmBase, bmSubScript, bmSupScript, bmMultiSub, bmMultiSup;
  multiSubSize.SetBlockStartAscent(-0x7FFFFFFF);
  multiSupSize.SetBlockStartAscent(-0x7FFFFFFF);
  bmMultiSub.ascent = bmMultiSup.ascent = -0x7FFFFFFF;
  bmMultiSub.descent = bmMultiSup.descent = -0x7FFFFFFF;
  nscoord italicCorrection = 0;

  nsBoundingMetrics boundingMetrics;
  boundingMetrics.width = 0;
  boundingMetrics.ascent = boundingMetrics.descent = -0x7FFFFFFF;
  aDesiredSize.Width() = aDesiredSize.Height() = 0;

  int32_t count = 0;
  bool foundNoneTag = false;

  
  
  bool isSubScript = (tag != nsGkAtoms::msup_);

  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  while (childFrame) {
    if (childFrame->GetContent()->IsMathMLElement(nsGkAtoms::mprescripts_)) {
      if (tag != nsGkAtoms::mmultiscripts_) {
        if (aPlaceOrigin) {
          aFrame->ReportInvalidChildError(nsGkAtoms::mprescripts_);
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
      

      if (childFrame->GetContent()->IsMathMLElement(nsGkAtoms::none)) {
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

      
      
      boundingMetrics.width = bmBase.width;
      boundingMetrics.rightBearing = bmBase.rightBearing;
      boundingMetrics.leftBearing = bmBase.leftBearing; 
    } else {
      
      if (childFrame->GetContent()->IsMathMLElement(nsGkAtoms::none)) {
        foundNoneTag = true;
      }

      if (isSubScript) {
        
        subScriptFrame = childFrame;
        GetReflowAndBoundingMetricsFor(subScriptFrame, subScriptSize, bmSubScript);
        
        GetSubDropFromChild (subScriptFrame, subDrop, aFontSizeInflation);
        
        minSubScriptShift = bmBase.descent + subDrop;
        trySubScriptShift = std::max(minSubScriptShift,subScriptShift);
        multiSubSize.SetBlockStartAscent(
           std::max(multiSubSize.BlockStartAscent(),
                    subScriptSize.BlockStartAscent()));
        bmMultiSub.ascent = std::max(bmMultiSub.ascent, bmSubScript.ascent);
        bmMultiSub.descent = std::max(bmMultiSub.descent, bmSubScript.descent);
        multiSubSize.Height() = 
          std::max(multiSubSize.Height(),
                   subScriptSize.Height() - subScriptSize.BlockStartAscent());
        if (bmSubScript.width)
          width = bmSubScript.width + scriptSpace;
        rightBearing = bmSubScript.rightBearing;

        if (tag == nsGkAtoms::msub_) {
          boundingMetrics.rightBearing = boundingMetrics.width + rightBearing;
          boundingMetrics.width += width;

          nscoord subscriptTopMax;
          if (mathFont) {
            subscriptTopMax =
              mathFont->GetMathConstant(gfxFontEntry::SubscriptTopMax,
                                        oneDevPixel);
          } else {
            
            
            subscriptTopMax = NSToCoordRound((4.0f/5.0f) * xHeight);
          }
          nscoord minShiftFromXHeight = bmSubScript.ascent - subscriptTopMax;
          maxSubScriptShift = std::max(trySubScriptShift,minShiftFromXHeight);

          maxSubScriptShift = std::max(maxSubScriptShift, trySubScriptShift);
          trySubScriptShift = subScriptShift;
        }
      } else {
        
        supScriptFrame = childFrame;
        GetReflowAndBoundingMetricsFor(supScriptFrame, supScriptSize, bmSupScript);
        
        GetSupDropFromChild (supScriptFrame, supDrop, aFontSizeInflation);
        
        minSupScriptShift = bmBase.ascent - supDrop;
        nscoord superscriptBottomMin;
        if (mathFont) {
          superscriptBottomMin =
            mathFont->GetMathConstant(gfxFontEntry::SuperscriptBottomMin,
                                      oneDevPixel);
        } else {
          
          
          superscriptBottomMin = NSToCoordRound((1.0f / 4.0f) * xHeight);
        }
        minShiftFromXHeight = bmSupScript.descent + superscriptBottomMin;
        trySupScriptShift = std::max(minSupScriptShift,
                                     std::max(minShiftFromXHeight,
                                              supScriptShift));
        multiSupSize.SetBlockStartAscent(
          std::max(multiSupSize.BlockStartAscent(),
                   supScriptSize.BlockStartAscent()));
        bmMultiSup.ascent = std::max(bmMultiSup.ascent, bmSupScript.ascent);
        bmMultiSup.descent = std::max(bmMultiSup.descent, bmSupScript.descent);
        multiSupSize.Height() =
          std::max(multiSupSize.Height(),
                   supScriptSize.Height() - supScriptSize.BlockStartAscent());

        if (bmSupScript.width)
          width = std::max(width, bmSupScript.width + scriptSpace);

        if (!prescriptsFrame) { 
          rightBearing = std::max(rightBearing,
                                  italicCorrection + bmSupScript.rightBearing);
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
          nscoord subSuperscriptGapMin;
          if (mathFont) {
            subSuperscriptGapMin =
              mathFont->GetMathConstant(gfxFontEntry::SubSuperscriptGapMin,
                                        oneDevPixel);
          } else {
            nscoord ruleSize;
            GetRuleThickness(aRenderingContext, fm, ruleSize);
            subSuperscriptGapMin = 4 * ruleSize;
          }
          nscoord gap =
            (trySupScriptShift - bmSupScript.descent) -
            (bmSubScript.ascent - trySubScriptShift);
          if (gap < subSuperscriptGapMin) {
            
            trySubScriptShift += subSuperscriptGapMin - gap;
          }

          
          
          nscoord superscriptBottomMaxWithSubscript;
          if (mathFont) {
            superscriptBottomMaxWithSubscript = mathFont->
              GetMathConstant(gfxFontEntry::SuperscriptBottomMaxWithSubscript,
                              oneDevPixel);
          } else {
            superscriptBottomMaxWithSubscript =
              NSToCoordRound((4.0f / 5.0f) * xHeight);
          }
          gap = superscriptBottomMaxWithSubscript -
            (trySupScriptShift - bmSupScript.descent);
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

  
  
  if (!subScriptFrame)
    maxSubScriptShift = 0;
  if (!supScriptFrame)
    maxSupScriptShift = 0;

  
  if (tag == nsGkAtoms::msub_) {
    boundingMetrics.ascent = std::max(bmBase.ascent,
                                      bmMultiSub.ascent - maxSubScriptShift);
  } else {
    boundingMetrics.ascent =
      std::max(bmBase.ascent, (bmMultiSup.ascent + maxSupScriptShift));
  }
  if (tag == nsGkAtoms::msup_) {
    boundingMetrics.descent = std::max(bmBase.descent,
                                       bmMultiSup.descent - maxSupScriptShift);
  } else {
    boundingMetrics.descent =
      std::max(bmBase.descent, (bmMultiSub.descent + maxSubScriptShift));
  }
  aFrame->SetBoundingMetrics(boundingMetrics);

  
  aDesiredSize.SetBlockStartAscent(
    std::max(baseSize.BlockStartAscent(),
             std::max(multiSubSize.BlockStartAscent() - maxSubScriptShift,
                      multiSupSize.BlockStartAscent() + maxSupScriptShift)));
  aDesiredSize.Height() = aDesiredSize.BlockStartAscent() +
    std::max(baseSize.Height() - baseSize.BlockStartAscent(),
             std::max(multiSubSize.Height() + maxSubScriptShift,
                      multiSupSize.Height() - maxSupScriptShift));
  aDesiredSize.Width() = boundingMetrics.width;
  aDesiredSize.mBoundingMetrics = boundingMetrics;

  aFrame->SetReference(nsPoint(0, aDesiredSize.BlockStartAscent()));

  
  

  
  
  

  if (aPlaceOrigin) {
    nscoord dx = 0, dy = 0;

    
    
    
    if (tag == nsGkAtoms::msub_ || tag == nsGkAtoms::msup_)
      count = 1;
    else
      count = 0;
    childFrame = prescriptsFrame;
    bool isPreScript = true;
    do {
      if (!childFrame) { 
        isPreScript = false;
        
        childFrame = baseFrame;
        dy = aDesiredSize.BlockStartAscent() - baseSize.BlockStartAscent();
        FinishReflowChild (baseFrame, aPresContext, baseSize, nullptr,
                           aFrame->MirrorIfRTL(aDesiredSize.Width(),
                                               baseSize.Width(),
                                               dx),
                           dy, 0);
        dx += bmBase.width;
      } else if (prescriptsFrame == childFrame) {
        
        prescriptsFrame->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);
      } else {
        
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

          width = std::max(subScriptSize.Width(), supScriptSize.Width());

          if (subScriptFrame) {
            nscoord x = dx;
            
            
            if (isPreScript)
              x += width - subScriptSize.Width();
            dy = aDesiredSize.BlockStartAscent() - subScriptSize.BlockStartAscent() +
              maxSubScriptShift;
            FinishReflowChild (subScriptFrame, aPresContext, subScriptSize,
                               nullptr,
                               aFrame->MirrorIfRTL(aDesiredSize.Width(),
                                                   subScriptSize.Width(),
                                                   x),
                               dy, 0);
          }

          if (supScriptFrame) {
            nscoord x = dx;
            if (isPreScript) {
              x += width - supScriptSize.Width();
            } else {
              
              x += italicCorrection;
            }
            dy = aDesiredSize.BlockStartAscent() - supScriptSize.BlockStartAscent() -
              maxSupScriptShift;
            FinishReflowChild (supScriptFrame, aPresContext, supScriptSize,
                               nullptr,
                               aFrame->MirrorIfRTL(aDesiredSize.Width(),
                                                   supScriptSize.Width(),
                                                   x),
                               dy, 0);
          }
          dx += width + scriptSpace;
        }
      }
      childFrame = childFrame->GetNextSibling();
    } while (prescriptsFrame != childFrame);
  }

  return NS_OK;
}
