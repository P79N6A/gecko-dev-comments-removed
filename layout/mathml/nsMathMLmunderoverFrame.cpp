




#include "nsMathMLmunderoverFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsMathMLmmultiscriptsFrame.h"
#include <algorithm>







nsIFrame*
NS_NewMathMLmunderoverFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmunderoverFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmunderoverFrame)

nsMathMLmunderoverFrame::~nsMathMLmunderoverFrame()
{
}

nsresult
nsMathMLmunderoverFrame::AttributeChanged(int32_t         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          int32_t         aModType)
{
  if (nsGkAtoms::accent_ == aAttribute ||
      nsGkAtoms::accentunder_ == aAttribute) {
    
    
    return ReLayoutChildren(GetParent());
  }

  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::UpdatePresentationData(uint32_t        aFlagsValues,
                                                uint32_t        aFlagsToUpdate)
{
  nsMathMLContainerFrame::UpdatePresentationData(aFlagsValues, aFlagsToUpdate);
  
  if (NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_INLINE) {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;
  }
  else {
    mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;
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

uint8_t
nsMathMLmunderoverFrame::ScriptIncrement(nsIFrame* aFrame)
{
  nsIFrame* child = mFrames.FirstChild();
  if (!aFrame || aFrame == child) {
    return 0;
  }
  child = child->GetNextSibling();
  if (aFrame == child) {
    if (mContent->Tag() == nsGkAtoms::mover_) {
      return mIncrementOver ? 1 : 0;
    }
    return mIncrementUnder ? 1 : 0;
  }
  if (child && aFrame == child->GetNextSibling()) {
    
    return mIncrementOver ? 1 : 0;
  }
  return 0;  
}

NS_IMETHODIMP
nsMathMLmunderoverFrame::TransmitAutomaticData()
{
  
  
  

  


























  nsIFrame* overscriptFrame = nullptr;
  nsIFrame* underscriptFrame = nullptr;
  nsIFrame* baseFrame = mFrames.FirstChild();
  nsIAtom* tag = mContent->Tag();

  if (baseFrame) {
    if (tag == nsGkAtoms::munder_ ||
        tag == nsGkAtoms::munderover_) {
      underscriptFrame = baseFrame->GetNextSibling();
    } else {
      NS_ASSERTION(tag == nsGkAtoms::mover_, "mContent->Tag() not recognized");
      overscriptFrame = baseFrame->GetNextSibling();
    }
  }
  if (underscriptFrame &&
      tag == nsGkAtoms::munderover_) {
    overscriptFrame = underscriptFrame->GetNextSibling();

  }

  
  
  
  mPresentationData.baseFrame = baseFrame;
  GetEmbellishDataFrom(baseFrame, mEmbellishData);

  
  
  nsEmbellishData embellishData;
  nsAutoString value;
  if (tag == nsGkAtoms::munder_ ||
      tag == nsGkAtoms::munderover_) {
    GetEmbellishDataFrom(underscriptFrame, embellishData);
    if (NS_MATHML_EMBELLISH_IS_ACCENT(embellishData.flags)) {
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTUNDER;
    } else {
      mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTUNDER;
    }    

    
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accentunder_, value)) {
      if (value.EqualsLiteral("true")) {
        mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTUNDER;
      } else if (value.EqualsLiteral("false")) {
        mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTUNDER;
      }
    }
  }

  
  
  if (tag == nsGkAtoms::mover_ ||
      tag == nsGkAtoms::munderover_) {
    GetEmbellishDataFrom(overscriptFrame, embellishData);
    if (NS_MATHML_EMBELLISH_IS_ACCENT(embellishData.flags)) {
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER;
    } else {
      mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER;
    }

    
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accent_, value)) {
      if (value.EqualsLiteral("true")) {
        mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENTOVER;
      } else if (value.EqualsLiteral("false")) {
        mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENTOVER;
      }
    }
  }

  bool subsupDisplay =
    NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
    StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_INLINE;

  
  if (subsupDisplay) {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_ALL_CHILDREN_HORIZONTALLY;
  }

  
  
  

  















  if (tag == nsGkAtoms::mover_ ||
      tag == nsGkAtoms::munderover_) {
    uint32_t compress = NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)
      ? NS_MATHML_COMPRESSED : 0;
    mIncrementOver =
      !NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags) ||
      subsupDisplay;
    SetIncrementScriptLevel(tag == nsGkAtoms::mover_ ? 1 : 2, mIncrementOver);
    if (mIncrementOver) {
      PropagateFrameFlagFor(overscriptFrame,
                            NS_FRAME_MATHML_SCRIPT_DESCENDANT);
    }
    PropagatePresentationDataFor(overscriptFrame, compress, compress);
  }
  



  if (tag == nsGkAtoms::munder_ ||
      tag == nsGkAtoms::munderover_) {
    mIncrementUnder =
      !NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags) ||
      subsupDisplay;
    SetIncrementScriptLevel(1, mIncrementUnder);
    if (mIncrementUnder) {
      PropagateFrameFlagFor(underscriptFrame,
                            NS_FRAME_MATHML_SCRIPT_DESCENDANT);
    }
    PropagatePresentationDataFor(underscriptFrame,
                                 NS_MATHML_COMPRESSED,
                                 NS_MATHML_COMPRESSED);
  }
  return NS_OK;
}




















 nsresult
nsMathMLmunderoverFrame::Place(nsRenderingContext& aRenderingContext,
                               bool                 aPlaceOrigin,
                               nsHTMLReflowMetrics& aDesiredSize)
{
  nsIAtom* tag = mContent->Tag();
  if (NS_MATHML_EMBELLISH_IS_MOVABLELIMITS(mEmbellishData.flags) &&
      StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_INLINE) {
    
    nscoord scriptSpace = nsPresContext::CSSPointsToAppUnits(0.5f);
    if (tag == nsGkAtoms::munderover_) {
      return nsMathMLmmultiscriptsFrame::PlaceMultiScript(PresContext(),
                                                          aRenderingContext,
                                                          aPlaceOrigin,
                                                          aDesiredSize,
                                                          this, 0, 0,
                                                          scriptSpace);
    } else if (tag == nsGkAtoms::munder_) {
      return nsMathMLmmultiscriptsFrame::PlaceMultiScript(PresContext(),
                                                          aRenderingContext,
                                                          aPlaceOrigin,
                                                          aDesiredSize,
                                                          this, 0, 0,
                                                          scriptSpace);
    } else {
      NS_ASSERTION(tag == nsGkAtoms::mover_, "mContent->Tag() not recognized");
      return nsMathMLmmultiscriptsFrame::PlaceMultiScript(PresContext(),
                                                          aRenderingContext,
                                                          aPlaceOrigin,
                                                          aDesiredSize,
                                                          this, 0, 0,
                                                          scriptSpace);
    }
    
  }

  
  

  nsBoundingMetrics bmBase, bmUnder, bmOver;
  nsHTMLReflowMetrics baseSize(aDesiredSize.GetWritingMode());
  nsHTMLReflowMetrics underSize(aDesiredSize.GetWritingMode());
  nsHTMLReflowMetrics overSize(aDesiredSize.GetWritingMode());
  nsIFrame* overFrame = nullptr;
  nsIFrame* underFrame = nullptr;
  nsIFrame* baseFrame = mFrames.FirstChild();
  underSize.SetTopAscent(0);
  overSize.SetTopAscent(0);
  bool haveError = false;
  if (baseFrame) {
    if (tag == nsGkAtoms::munder_ ||
        tag == nsGkAtoms::munderover_) {
      underFrame = baseFrame->GetNextSibling();
    } else if (tag == nsGkAtoms::mover_) {
      overFrame = baseFrame->GetNextSibling();
    }
  }
  if (underFrame && tag == nsGkAtoms::munderover_) {
    overFrame = underFrame->GetNextSibling();
  }
  
  if (tag == nsGkAtoms::munder_) {
    if (!baseFrame || !underFrame || underFrame->GetNextSibling()) {
      
      haveError = true;
    }
  }
  if (tag == nsGkAtoms::mover_) {
    if (!baseFrame || !overFrame || overFrame->GetNextSibling()) {
      
      haveError = true;
    }
  }
  if (tag == nsGkAtoms::munderover_) {
    if (!baseFrame || !underFrame || !overFrame || overFrame->GetNextSibling()) {
      
      haveError = true;
    }
  }
  if (haveError) {
    if (aPlaceOrigin) {
      ReportChildCountError();
    } 
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(baseFrame, baseSize, bmBase);
  if (underFrame) {
    GetReflowAndBoundingMetricsFor(underFrame, underSize, bmUnder);
  }
  if (overFrame) {
    GetReflowAndBoundingMetricsFor(overFrame, overSize, bmOver);
  }

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  nscoord xHeight = fm->XHeight();

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
    underDelta1 = std::max(bigOpSpacing2, (bigOpSpacing4 - bmUnder.ascent));
    underDelta2 = bigOpSpacing5;
  }
  else {
    
    

    
    underDelta1 = ruleThickness + onePixel/2;
    underDelta2 = ruleThickness;
  }
  
  if (!(bmUnder.ascent + bmUnder.descent)) {
    underDelta1 = 0;
    underDelta2 = 0;
  }

  nscoord overDelta1 = 0; 
  nscoord overDelta2 = 0; 

  if (!NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)) {    
    
    nscoord bigOpSpacing1, bigOpSpacing3, bigOpSpacing5, dummy; 
    GetBigOpSpacings (fm, 
                      bigOpSpacing1, dummy, 
                      bigOpSpacing3, dummy, 
                      bigOpSpacing5);
    overDelta1 = std::max(bigOpSpacing1, (bigOpSpacing3 - bmOver.descent));
    overDelta2 = bigOpSpacing5;

    
    
    
    if (bmOver.descent < 0)    
      overDelta1 = std::max(bigOpSpacing1, (bigOpSpacing3 - (bmOver.ascent + bmOver.descent)));
  }
  else {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    overDelta1 = ruleThickness + onePixel/2;
    if (bmBase.ascent < xHeight) {
      
      overDelta1 += xHeight - bmBase.ascent;
    }
    overDelta2 = ruleThickness;
  }
  
  if (!(bmOver.ascent + bmOver.descent)) {
    overDelta1 = 0;
    overDelta2 = 0;
  }

  nscoord dxBase = 0, dxOver = 0, dxUnder = 0;
  nsAutoString valueAlign;
  enum {
    center,
    left,
    right
  } alignPosition = center;

  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::align, valueAlign)) {
    if (valueAlign.EqualsLiteral("left")) {
      alignPosition = left;
    } else if (valueAlign.EqualsLiteral("right")) {
      alignPosition = right;
    }
  }

  
  

  
  
  
  nscoord overWidth = bmOver.width;
  if (!overWidth && (bmOver.rightBearing - bmOver.leftBearing > 0)) {
    overWidth = bmOver.rightBearing - bmOver.leftBearing;
    dxOver = -bmOver.leftBearing;
  }

  if (NS_MATHML_EMBELLISH_IS_ACCENTOVER(mEmbellishData.flags)) {
    mBoundingMetrics.width = bmBase.width; 
    if (alignPosition == center) {
      dxOver += correction;
    }
  }
  else {
    mBoundingMetrics.width = std::max(bmBase.width, overWidth);
    if (alignPosition == center) {
      dxOver += correction/2;
    }
  }
  
  if (alignPosition == center) {
    dxOver += (mBoundingMetrics.width - overWidth)/2;
    dxBase = (mBoundingMetrics.width - bmBase.width)/2;
  } else if (alignPosition == right) {
    dxOver += mBoundingMetrics.width - overWidth;
    dxBase = mBoundingMetrics.width - bmBase.width;
  }

  mBoundingMetrics.ascent = 
    bmBase.ascent + overDelta1 + bmOver.ascent + bmOver.descent;
  mBoundingMetrics.descent = bmBase.descent;
  mBoundingMetrics.leftBearing = 
    std::min(dxBase + bmBase.leftBearing, dxOver + bmOver.leftBearing);
  mBoundingMetrics.rightBearing = 
    std::max(dxBase + bmBase.rightBearing, dxOver + bmOver.rightBearing);

  
  
  
  
  
  

  nsBoundingMetrics bmAnonymousBase = mBoundingMetrics;
  nscoord ascentAnonymousBase =
    std::max(mBoundingMetrics.ascent + overDelta2,
           overSize.TopAscent() + bmOver.descent + overDelta1 + bmBase.ascent);
  ascentAnonymousBase = std::max(ascentAnonymousBase, baseSize.TopAscent());

  
  nscoord underWidth = bmUnder.width;
  if (!underWidth) {
    underWidth = bmUnder.rightBearing - bmUnder.leftBearing;
    dxUnder = -bmUnder.leftBearing;
  }

  nscoord maxWidth = std::max(bmAnonymousBase.width, underWidth);
  if (alignPosition == center &&
      !NS_MATHML_EMBELLISH_IS_ACCENTUNDER(mEmbellishData.flags)) {
    GetItalicCorrection(bmAnonymousBase, correction);
    dxUnder += -correction/2;
  }
  nscoord dxAnonymousBase = 0;
  if (alignPosition == center) {
    dxUnder += (maxWidth - underWidth)/2;
    dxAnonymousBase = (maxWidth - bmAnonymousBase.width)/2;
  } else if (alignPosition == right) {
    dxUnder += maxWidth - underWidth;
    dxAnonymousBase = maxWidth - bmAnonymousBase.width;
  }

  
  
  dxOver += dxAnonymousBase;
  dxBase += dxAnonymousBase;

  mBoundingMetrics.width =
    std::max(dxAnonymousBase + bmAnonymousBase.width, dxUnder + bmUnder.width);
  
  mBoundingMetrics.descent = 
    bmAnonymousBase.descent + underDelta1 + bmUnder.ascent + bmUnder.descent;
  mBoundingMetrics.leftBearing =
    std::min(dxAnonymousBase + bmAnonymousBase.leftBearing, dxUnder + bmUnder.leftBearing);
  mBoundingMetrics.rightBearing = 
    std::max(dxAnonymousBase + bmAnonymousBase.rightBearing, dxUnder + bmUnder.rightBearing);

  aDesiredSize.SetTopAscent(ascentAnonymousBase);
  aDesiredSize.Height() = aDesiredSize.TopAscent() +
    std::max(mBoundingMetrics.descent + underDelta2,
           bmAnonymousBase.descent + underDelta1 + bmUnder.ascent +
             underSize.Height() - underSize.TopAscent());
  aDesiredSize.Height() = std::max(aDesiredSize.Height(),
                               aDesiredSize.TopAscent() +
                               baseSize.Height() - baseSize.TopAscent());
  aDesiredSize.Width() = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.TopAscent();

  if (aPlaceOrigin) {
    nscoord dy;
    
    if (overFrame) {
      dy = aDesiredSize.TopAscent() - mBoundingMetrics.ascent + bmOver.ascent 
        - overSize.TopAscent();
      FinishReflowChild (overFrame, PresContext(), overSize, nullptr, dxOver, dy, 0);
    }
    
    dy = aDesiredSize.TopAscent() - baseSize.TopAscent();
    FinishReflowChild (baseFrame, PresContext(), baseSize, nullptr, dxBase, dy, 0);
    
    if (underFrame) {
      dy = aDesiredSize.TopAscent() + mBoundingMetrics.descent - bmUnder.descent 
        - underSize.TopAscent();
      FinishReflowChild (underFrame, PresContext(), underSize, nullptr,
                         dxUnder, dy, 0);
    }
  }
  return NS_OK;
}
