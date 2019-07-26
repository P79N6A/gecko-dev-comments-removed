





#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsRenderingContext.h"

#include "nsMathMLmrootFrame.h"
#include <algorithm>















#define NS_SQR_CHAR_STYLE_CONTEXT_INDEX   0

static const PRUnichar kSqrChar = PRUnichar(0x221A);

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
nsMathMLmrootFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);
  
  nsPresContext *presContext = PresContext();

  
  
  
  nsAutoString sqrChar; sqrChar.Assign(kSqrChar);
  mSqrChar.SetData(presContext, sqrChar);
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mSqrChar, true);
}

NS_IMETHODIMP
nsMathMLmrootFrame::TransmitAutomaticData()
{
  
  
  
  
  UpdatePresentationDataFromChildAt(1, 1,
    ~NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED,
     NS_MATHML_DISPLAYSTYLE | NS_MATHML_COMPRESSED);
  UpdatePresentationDataFromChildAt(0, 0,
     NS_MATHML_COMPRESSED, NS_MATHML_COMPRESSED);

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

static void
GetRadicalXOffsets(nscoord aIndexWidth, nscoord aSqrWidth,
                   nsFontMetrics* aFontMetrics,
                   nscoord* aIndexOffset, nscoord* aSqrOffset)
{
  
  
  nscoord dxIndex, dxSqr;
  nscoord xHeight = aFontMetrics->XHeight();
  nscoord indexRadicalKern = NSToCoordRound(1.35f * xHeight);
  if (indexRadicalKern > aIndexWidth) {
    dxIndex = indexRadicalKern - aIndexWidth;
    dxSqr = 0;
  }
  else {
    dxIndex = 0;
    dxSqr = aIndexWidth - indexRadicalKern;
  }
  
  nscoord minimumClearance = aSqrWidth/2;
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

  if (aIndexOffset)
    *aIndexOffset = dxIndex;
  if (aSqrOffset)
    *aSqrOffset = dxSqr;
}

NS_IMETHODIMP
nsMathMLmrootFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  nsresult rv = NS_OK;
  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  nsReflowStatus childStatus;

  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;

  nsBoundingMetrics bmSqr, bmBase, bmIndex;
  nsRenderingContext& renderingContext = *aReflowState.rendContext;

  
  

  int32_t count = 0;
  nsIFrame* baseFrame = nullptr;
  nsIFrame* indexFrame = nullptr;
  nsHTMLReflowMetrics baseSize;
  nsHTMLReflowMetrics indexSize;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    
    nsHTMLReflowMetrics childDesiredSize(aDesiredSize.mFlags
                                         | NS_REFLOW_CALC_BOUNDING_METRICS);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext,
                     childDesiredSize, childReflowState, childStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(mFrames.FirstChild(), childFrame);
      return rv;
    }
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
    rv = ReflowError(renderingContext, aDesiredSize);
    aStatus = NS_FRAME_COMPLETE;
    NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
    
    DidReflowChildren(mFrames.FirstChild(), childFrame);
    return rv;
  }

  
  

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  renderingContext.SetFont(fm);

  
  
  
  
  nscoord ruleThickness, leading, em;
  GetRuleThickness(renderingContext, fm, ruleThickness);

  PRUnichar one = '1';
  nsBoundingMetrics bmOne = renderingContext.GetBoundingMetrics(&one, 1);

  
  
  GetEmHeight(fm, em);
  leading = nscoord(0.2f * em); 

  
  
  nscoord phi = 0, psi = 0;
  if (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags))
    phi = fm->XHeight();
  else
    phi = ruleThickness;
  psi = ruleThickness + phi/4;

  
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
                   NS_MATHML_IS_RTL(mPresentationData.flags));
  
  
  mSqrChar.GetBoundingMetrics(bmSqr);

  
  
  mBoundingMetrics.ascent = bmBase.ascent + psi + ruleThickness;
  mBoundingMetrics.descent = 
    std::max(bmBase.descent,
           (bmSqr.ascent + bmSqr.descent - mBoundingMetrics.ascent));
  mBoundingMetrics.width = bmSqr.width + bmBase.width;
  mBoundingMetrics.leftBearing = bmSqr.leftBearing;
  mBoundingMetrics.rightBearing = bmSqr.width + 
    std::max(bmBase.width, bmBase.rightBearing); 

  aDesiredSize.ascent = mBoundingMetrics.ascent + leading;
  aDesiredSize.height = aDesiredSize.ascent +
    std::max(baseSize.height - baseSize.ascent,
           mBoundingMetrics.descent + ruleThickness);
  aDesiredSize.width = mBoundingMetrics.width;

  
  
  
  
  
  nscoord raiseIndexDelta = NSToCoordRound(0.6f * (bmSqr.ascent + bmSqr.descent));
  nscoord indexRaisedAscent = mBoundingMetrics.ascent 
    - (bmSqr.ascent + bmSqr.descent) 
    + raiseIndexDelta + bmIndex.ascent + bmIndex.descent; 

  nscoord indexClearance = 0;
  if (mBoundingMetrics.ascent < indexRaisedAscent) {
    indexClearance = 
      indexRaisedAscent - mBoundingMetrics.ascent; 
    mBoundingMetrics.ascent = indexRaisedAscent;
    nscoord descent = aDesiredSize.height - aDesiredSize.ascent;
    aDesiredSize.ascent = mBoundingMetrics.ascent + leading;
    aDesiredSize.height = aDesiredSize.ascent + descent;
  }

  nscoord dxIndex, dxSqr;
  GetRadicalXOffsets(bmIndex.width, bmSqr.width, fm, &dxIndex, &dxSqr);

  mBoundingMetrics.width = dxSqr + bmSqr.width + bmBase.width;
  mBoundingMetrics.leftBearing = 
    std::min(dxIndex + bmIndex.leftBearing, dxSqr + bmSqr.leftBearing);
  mBoundingMetrics.rightBearing = dxSqr + bmSqr.width +
    std::max(bmBase.width, bmBase.rightBearing);

  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  GatherAndStoreOverflow(&aDesiredSize);

  
  nscoord dx = dxIndex;
  nscoord dy = aDesiredSize.ascent - (indexRaisedAscent + indexSize.ascent - bmIndex.ascent);
  FinishReflowChild(indexFrame, aPresContext, nullptr, indexSize,
                    MirrorIfRTL(aDesiredSize.width, indexSize.width, dx),
                    dy, 0);

  
  dx = dxSqr;
  dy = indexClearance + leading; 
  mSqrChar.SetRect(nsRect(MirrorIfRTL(aDesiredSize.width, bmSqr.width, dx),
                          dy, bmSqr.width, bmSqr.ascent + bmSqr.descent));
  dx += bmSqr.width;
  mBarRect.SetRect(MirrorIfRTL(aDesiredSize.width, bmBase.width, dx),
                   dy, bmBase.width, ruleThickness);

  
  dy = aDesiredSize.ascent - baseSize.ascent;
  FinishReflowChild(baseFrame, aPresContext, nullptr, baseSize,
                    MirrorIfRTL(aDesiredSize.width, baseSize.width, dx),
                    dy, 0);

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

 nscoord
nsMathMLmrootFrame::GetIntrinsicWidth(nsRenderingContext* aRenderingContext)
{
  nsIFrame* baseFrame = mFrames.FirstChild();
  nsIFrame* indexFrame = nullptr;
  if (baseFrame)
    indexFrame = baseFrame->GetNextSibling();
  if (!indexFrame || indexFrame->GetNextSibling()) {
    nsHTMLReflowMetrics desiredSize;
    ReflowError(*aRenderingContext, desiredSize);
    return desiredSize.width;
  }

  nscoord baseWidth =
    nsLayoutUtils::IntrinsicForContainer(aRenderingContext, baseFrame,
                                         nsLayoutUtils::PREF_WIDTH);
  nscoord indexWidth =
    nsLayoutUtils::IntrinsicForContainer(aRenderingContext, indexFrame,
                                         nsLayoutUtils::PREF_WIDTH);
  nscoord sqrWidth = mSqrChar.GetMaxWidth(PresContext(), *aRenderingContext);

  nscoord dxSqr;
  GetRadicalXOffsets(indexWidth, sqrWidth, aRenderingContext->FontMetrics(),
                     nullptr, &dxSqr);

  return dxSqr + sqrWidth + baseWidth;
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
