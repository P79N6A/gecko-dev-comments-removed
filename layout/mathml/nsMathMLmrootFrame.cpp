




#include "nsMathMLmrootFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include <algorithm>

using namespace mozilla;






#define NS_SQR_CHAR_STYLE_CONTEXT_INDEX   0

static const char16_t kSqrChar = char16_t(0x221A);

nsIFrame*
NS_NewMathMLmrootFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmrootFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmrootFrame)

nsMathMLmrootFrame::nsMathMLmrootFrame(nsStyleContext* aContext) :
  nsMathMLContainerFrame(aContext),
  mSqrChar(),
  mBarRect()
{
}

nsMathMLmrootFrame::~nsMathMLmrootFrame()
{
}

void
nsMathMLmrootFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  
  nsPresContext *presContext = PresContext();

  
  
  
  nsAutoString sqrChar; sqrChar.Assign(kSqrChar);
  mSqrChar.SetData(presContext, sqrChar);
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mSqrChar);
}

NS_IMETHODIMP
nsMathMLmrootFrame::TransmitAutomaticData()
{
  
  
  
  
  UpdatePresentationDataFromChildAt(1, 1,
                                    NS_MATHML_COMPRESSED,
                                    NS_MATHML_COMPRESSED);
  UpdatePresentationDataFromChildAt(0, 0,
     NS_MATHML_COMPRESSED, NS_MATHML_COMPRESSED);

  PropagateFrameFlagFor(mFrames.LastChild(),
                        NS_FRAME_MATHML_SCRIPT_DESCENDANT);

  return NS_OK;
}

void
nsMathMLmrootFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  
  
  
  if (!NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    mSqrChar.Display(aBuilder, this, aLists, 0);

    DisplayBar(aBuilder, this, mBarRect, aLists);

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    nsRect rect;
    mSqrChar.GetRect(rect);
    nsBoundingMetrics bm;
    mSqrChar.GetBoundingMetrics(bm);
    DisplayBoundingMetrics(aBuilder, this, rect.TopLeft(), bm, aLists);
#endif
  }
}

void
nsMathMLmrootFrame::GetRadicalXOffsets(nscoord aIndexWidth, nscoord aSqrWidth,
                                       nsFontMetrics* aFontMetrics,
                                       nscoord* aIndexOffset,
                                       nscoord* aSqrOffset)
{
  
  
  nscoord dxIndex, dxSqr;
  nscoord xHeight = aFontMetrics->XHeight();
  nscoord indexRadicalKern = NSToCoordRound(1.35f * xHeight);
  nscoord oneDevPixel = aFontMetrics->AppUnitsPerDevPixel();
  gfxFont* mathFont = aFontMetrics->GetThebesFontGroup()->GetFirstMathFont();
  if (mathFont) {
    indexRadicalKern =
      mathFont->GetMathConstant(gfxFontEntry::RadicalKernAfterDegree,
                                oneDevPixel);
    indexRadicalKern = -indexRadicalKern;
  }
  if (indexRadicalKern > aIndexWidth) {
    dxIndex = indexRadicalKern - aIndexWidth;
    dxSqr = 0;
  }
  else {
    dxIndex = 0;
    dxSqr = aIndexWidth - indexRadicalKern;
  }

  if (mathFont) {
    
    nscoord indexRadicalKernBefore = 0;
    indexRadicalKernBefore =
      mathFont->GetMathConstant(gfxFontEntry::RadicalKernBeforeDegree,
                                oneDevPixel);
    dxIndex += indexRadicalKernBefore;
    dxSqr += indexRadicalKernBefore;
  } else {
    
    nscoord minimumClearance = aSqrWidth / 2;
    if (dxIndex + aIndexWidth + minimumClearance > dxSqr + aSqrWidth) {
      if (aIndexWidth + minimumClearance < aSqrWidth) {
        dxIndex = aSqrWidth - (aIndexWidth + minimumClearance);
        dxSqr = 0;
      }
      else {
        dxIndex = 0;
        dxSqr = (aIndexWidth + minimumClearance) - aSqrWidth;
      }
    }
  }

  if (aIndexOffset)
    *aIndexOffset = dxIndex;
  if (aSqrOffset)
    *aSqrOffset = dxSqr;
}

void
nsMathMLmrootFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  nsReflowStatus childStatus;

  aDesiredSize.ClearSize();
  aDesiredSize.SetBlockStartAscent(0);

  nsBoundingMetrics bmSqr, bmBase, bmIndex;
  nsRenderingContext& renderingContext = *aReflowState.rendContext;

  
  

  int32_t count = 0;
  nsIFrame* baseFrame = nullptr;
  nsIFrame* indexFrame = nullptr;
  nsHTMLReflowMetrics baseSize(aReflowState);
  nsHTMLReflowMetrics indexSize(aReflowState);
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    
    nsHTMLReflowMetrics childDesiredSize(aReflowState,
                                         aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    WritingMode wm = childFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    ReflowChild(childFrame, aPresContext,
                     childDesiredSize, childReflowState, childStatus);
    
    if (0 == count) {
      
      baseFrame = childFrame;
      baseSize = childDesiredSize;
      bmBase = childDesiredSize.mBoundingMetrics;
    }
    else if (1 == count) {
      
      indexFrame = childFrame;
      indexSize = childDesiredSize;
      bmIndex = childDesiredSize.mBoundingMetrics;
    }
    count++;
    childFrame = childFrame->GetNextSibling();
  }
  if (2 != count) {
    
    ReportChildCountError();
    ReflowError(renderingContext, aDesiredSize);
    aStatus = NS_FRAME_COMPLETE;
    NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
    
    DidReflowChildren(mFrames.FirstChild(), childFrame);
    return;
  }

  
  

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  renderingContext.SetFont(fm);

  nscoord ruleThickness, leading, psi;
  GetRadicalParameters(fm, StyleFont()->mMathDisplay ==
                       NS_MATHML_DISPLAYSTYLE_BLOCK,
                       ruleThickness, leading, psi);

  
  char16_t one = '1';
  nsBoundingMetrics bmOne = renderingContext.GetBoundingMetrics(&one, 1);
  if (bmOne.ascent > bmBase.ascent)
    psi += bmOne.ascent - bmBase.ascent;

  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  if (ruleThickness < onePixel) {
    ruleThickness = onePixel;
  }

  
  
  nscoord delta = psi % onePixel;
  if (delta)
    psi += onePixel - delta; 

  
  nsBoundingMetrics contSize = bmBase;
  contSize.descent = bmBase.ascent + bmBase.descent + psi;
  contSize.ascent = ruleThickness;

  
  nsBoundingMetrics radicalSize;
  mSqrChar.Stretch(aPresContext, renderingContext,
                   NS_STRETCH_DIRECTION_VERTICAL, 
                   contSize, radicalSize,
                   NS_STRETCH_LARGER,
                   StyleVisibility()->mDirection);
  
  
  mSqrChar.GetBoundingMetrics(bmSqr);

  
  
  mBoundingMetrics.ascent = bmBase.ascent + psi + ruleThickness;
  mBoundingMetrics.descent = 
    std::max(bmBase.descent,
           (bmSqr.ascent + bmSqr.descent - mBoundingMetrics.ascent));
  mBoundingMetrics.width = bmSqr.width + bmBase.width;
  mBoundingMetrics.leftBearing = bmSqr.leftBearing;
  mBoundingMetrics.rightBearing = bmSqr.width + 
    std::max(bmBase.width, bmBase.rightBearing); 

  aDesiredSize.SetBlockStartAscent(mBoundingMetrics.ascent + leading);
  aDesiredSize.Height() = aDesiredSize.BlockStartAscent() +
    std::max(baseSize.Height() - baseSize.BlockStartAscent(),
             mBoundingMetrics.descent + ruleThickness);
  aDesiredSize.Width() = mBoundingMetrics.width;

  
  
  
  
  
  float raiseIndexPercent = 0.6f;
  gfxFont* mathFont = fm->GetThebesFontGroup()->GetFirstMathFont();
  if (mathFont) {
    raiseIndexPercent =
      mathFont->GetMathConstant(gfxFontEntry::RadicalDegreeBottomRaisePercent);
  }
  nscoord raiseIndexDelta = NSToCoordRound(raiseIndexPercent *
                                           (bmSqr.ascent + bmSqr.descent));
  nscoord indexRaisedAscent = mBoundingMetrics.ascent 
    - (bmSqr.ascent + bmSqr.descent) 
    + raiseIndexDelta + bmIndex.ascent + bmIndex.descent; 

  nscoord indexClearance = 0;
  if (mBoundingMetrics.ascent < indexRaisedAscent) {
    indexClearance = 
      indexRaisedAscent - mBoundingMetrics.ascent; 
    mBoundingMetrics.ascent = indexRaisedAscent;
    nscoord descent = aDesiredSize.Height() - aDesiredSize.BlockStartAscent();
    aDesiredSize.SetBlockStartAscent(mBoundingMetrics.ascent + leading);
    aDesiredSize.Height() = aDesiredSize.BlockStartAscent() + descent;
  }

  nscoord dxIndex, dxSqr;
  GetRadicalXOffsets(bmIndex.width, bmSqr.width, fm, &dxIndex, &dxSqr);

  mBoundingMetrics.width = dxSqr + bmSqr.width + bmBase.width;
  mBoundingMetrics.leftBearing = 
    std::min(dxIndex + bmIndex.leftBearing, dxSqr + bmSqr.leftBearing);
  mBoundingMetrics.rightBearing = dxSqr + bmSqr.width +
    std::max(bmBase.width, bmBase.rightBearing);

  aDesiredSize.Width() = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  GatherAndStoreOverflow(&aDesiredSize);

  
  nscoord dx = dxIndex;
  nscoord dy = aDesiredSize.BlockStartAscent() -
    (indexRaisedAscent + indexSize.BlockStartAscent() - bmIndex.ascent);
  FinishReflowChild(indexFrame, aPresContext, indexSize, nullptr,
                    MirrorIfRTL(aDesiredSize.Width(), indexSize.Width(), dx),
                    dy, 0);

  
  dx = dxSqr;
  dy = indexClearance + leading; 
  mSqrChar.SetRect(nsRect(MirrorIfRTL(aDesiredSize.Width(), bmSqr.width, dx),
                          dy, bmSqr.width, bmSqr.ascent + bmSqr.descent));
  dx += bmSqr.width;
  mBarRect.SetRect(MirrorIfRTL(aDesiredSize.Width(), bmBase.width, dx),
                   dy, bmBase.width, ruleThickness);

  
  dy = aDesiredSize.BlockStartAscent() - baseSize.BlockStartAscent();
  FinishReflowChild(baseFrame, aPresContext, baseSize, nullptr,
                    MirrorIfRTL(aDesiredSize.Width(), baseSize.Width(), dx),
                    dy, 0);

  mReference.x = 0;
  mReference.y = aDesiredSize.BlockStartAscent();

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

 void
nsMathMLmrootFrame::GetIntrinsicISizeMetrics(nsRenderingContext* aRenderingContext, nsHTMLReflowMetrics& aDesiredSize)
{
  nsIFrame* baseFrame = mFrames.FirstChild();
  nsIFrame* indexFrame = nullptr;
  if (baseFrame)
    indexFrame = baseFrame->GetNextSibling();
  if (!indexFrame || indexFrame->GetNextSibling()) {
    ReflowError(*aRenderingContext, aDesiredSize);
    return;
  }

  nscoord baseWidth =
    nsLayoutUtils::IntrinsicForContainer(aRenderingContext, baseFrame,
                                         nsLayoutUtils::PREF_ISIZE);
  nscoord indexWidth =
    nsLayoutUtils::IntrinsicForContainer(aRenderingContext, indexFrame,
                                         nsLayoutUtils::PREF_ISIZE);
  nscoord sqrWidth = mSqrChar.GetMaxWidth(PresContext(), *aRenderingContext);

  nscoord dxSqr;
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  GetRadicalXOffsets(indexWidth, sqrWidth, fm, nullptr, &dxSqr);

  nscoord width = dxSqr + sqrWidth + baseWidth;

  aDesiredSize.Width() = width;
  aDesiredSize.mBoundingMetrics.width = width;
  aDesiredSize.mBoundingMetrics.leftBearing = 0;
  aDesiredSize.mBoundingMetrics.rightBearing = width;
}



nsStyleContext*
nsMathMLmrootFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  switch (aIndex) {
  case NS_SQR_CHAR_STYLE_CONTEXT_INDEX:
    return mSqrChar.GetStyleContext();
    break;
  default:
    return nullptr;
  }
}

void
nsMathMLmrootFrame::SetAdditionalStyleContext(int32_t          aIndex, 
                                              nsStyleContext*  aStyleContext)
{
  switch (aIndex) {
  case NS_SQR_CHAR_STYLE_CONTEXT_INDEX:
    mSqrChar.SetStyleContext(aStyleContext);
    break;
  }
}
