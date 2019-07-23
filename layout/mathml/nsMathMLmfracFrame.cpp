








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsMathMLmfencedFrame.h"
#include "nsMathMLmfracFrame.h"







#define THIN_FRACTION_LINE                   0.5f
#define THIN_FRACTION_LINE_MINIMUM_PIXELS    1  // minimum of 1 pixel

#define MEDIUM_FRACTION_LINE                 1.5f
#define MEDIUM_FRACTION_LINE_MINIMUM_PIXELS  2  // minimum of 2 pixels

#define THICK_FRACTION_LINE                  2.0f
#define THICK_FRACTION_LINE_MINIMUM_PIXELS   4  // minimum of 4 pixels


#define NS_SLASH_CHAR_STYLE_CONTEXT_INDEX   0

static const PRUnichar kSlashChar = PRUnichar('/');

nsIFrame*
NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmfracFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmfracFrame)

nsMathMLmfracFrame::~nsMathMLmfracFrame()
{
  if (mSlashChar) {
    delete mSlashChar;
    mSlashChar = nsnull;
  }
}

PRBool
nsMathMLmfracFrame::IsBevelled()
{
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::bevelled_,
               value);
  return value.EqualsLiteral("true");
}

NS_IMETHODIMP
nsMathMLmfracFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsMathMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  if (IsBevelled()) {
    
    mSlashChar = new nsMathMLChar();
    if (mSlashChar) {
      nsPresContext* presContext = PresContext();
    
      nsAutoString slashChar; slashChar.Assign(kSlashChar);
      mSlashChar->SetData(presContext, slashChar);
      ResolveMathMLCharStyle(presContext, mContent, mStyleContext, mSlashChar, PR_TRUE);
    }
  }

  return rv;
}

eMathMLFrameType
nsMathMLmfracFrame::GetMathMLFrameType()
{
  
  return eMathMLFrameType_Inner;
}

NS_IMETHODIMP
nsMathMLmfracFrame::TransmitAutomaticData()
{
  
  
  
  
  
  PRBool increment = !NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags);
  SetIncrementScriptLevel(0, increment);
  SetIncrementScriptLevel(1, increment);

  UpdatePresentationDataFromChildAt(0, -1,
    ~NS_MATHML_DISPLAYSTYLE,
     NS_MATHML_DISPLAYSTYLE);
  UpdatePresentationDataFromChildAt(1,  1,
     NS_MATHML_COMPRESSED,
     NS_MATHML_COMPRESSED);

  
  GetEmbellishDataFrom(mFrames.FirstChild(), mEmbellishData);
  if (NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags)) {
    
    
    mEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
  }

  return NS_OK;
}

nscoord 
nsMathMLmfracFrame::CalcLineThickness(nsPresContext*  aPresContext,
                                      nsStyleContext*  aStyleContext,
                                      nsString&        aThicknessAttribute,
                                      nscoord          onePixel,
                                      nscoord          aDefaultRuleThickness)
{
  nscoord defaultThickness = aDefaultRuleThickness;
  nscoord lineThickness = aDefaultRuleThickness;
  nscoord minimumThickness = onePixel;

  if (!aThicknessAttribute.IsEmpty()) {
    if (aThicknessAttribute.EqualsLiteral("thin")) {
      lineThickness = NSToCoordFloor(defaultThickness * THIN_FRACTION_LINE);
      minimumThickness = onePixel * THIN_FRACTION_LINE_MINIMUM_PIXELS;
      
      if (defaultThickness > onePixel && lineThickness > defaultThickness - onePixel)
        lineThickness = defaultThickness - onePixel;
    }
    else if (aThicknessAttribute.EqualsLiteral("medium")) {
      lineThickness = NSToCoordRound(defaultThickness * MEDIUM_FRACTION_LINE);
      minimumThickness = onePixel * MEDIUM_FRACTION_LINE_MINIMUM_PIXELS;
      
      if (lineThickness < defaultThickness + onePixel)
        lineThickness = defaultThickness + onePixel;
    }
    else if (aThicknessAttribute.EqualsLiteral("thick")) {
      lineThickness = NSToCoordCeil(defaultThickness * THICK_FRACTION_LINE);
      minimumThickness = onePixel * THICK_FRACTION_LINE_MINIMUM_PIXELS;
      
      if (lineThickness < defaultThickness + 2*onePixel)
        lineThickness = defaultThickness + 2*onePixel;
    }
    else { 
      nsCSSValue cssValue;
      if (ParseNumericValue(aThicknessAttribute, cssValue)) {
        nsCSSUnit unit = cssValue.GetUnit();
        if (eCSSUnit_Number == unit)
          lineThickness = nscoord(float(defaultThickness) * cssValue.GetFloatValue());
        else if (eCSSUnit_Percent == unit)
          lineThickness = nscoord(float(defaultThickness) * cssValue.GetPercentValue());
        else if (eCSSUnit_Null != unit)
          lineThickness = CalcLength(aPresContext, aStyleContext, cssValue);
      }
    }
  }

  
  if (lineThickness && lineThickness < minimumThickness) 
    lineThickness = minimumThickness;

  return lineThickness;
}

NS_IMETHODIMP
nsMathMLmfracFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  nsresult rv = nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  if (mSlashChar) {
    
    rv = mSlashChar->Display(aBuilder, this, aLists);
  } else {
    rv = DisplayBar(aBuilder, this, mLineRect, aLists);
  }

  return rv;
}

NS_IMETHODIMP
nsMathMLmfracFrame::Reflow(nsPresContext*          aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  if (mSlashChar) {
    
    return nsMathMLmfencedFrame::doReflow(aPresContext, aReflowState,
                                          aDesiredSize, aStatus, this,
                                          nsnull, nsnull, mSlashChar, 1);
  }

  
  return nsMathMLContainerFrame::Reflow(aPresContext, aDesiredSize,
                                        aReflowState, aStatus);
}

 nscoord
nsMathMLmfracFrame::GetIntrinsicWidth(nsIRenderingContext* aRenderingContext)
{
  if (mSlashChar) {
    
    return nsMathMLmfencedFrame::doGetIntrinsicWidth(aRenderingContext, this,
                                                     nsnull, nsnull,
                                                     mSlashChar, 1);
  }

  
  return nsMathMLContainerFrame::GetIntrinsicWidth(aRenderingContext);  
}

nscoord
nsMathMLmfracFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap) return 0;

  if (mSlashChar) {
    nsRect rect;
    mSlashChar->GetRect(rect);
    rect.MoveBy(gap, 0);
    mSlashChar->SetRect(rect);
  }
  else {
    mLineRect.MoveBy(gap, 0);
  }
  return gap;
}

 nsresult
nsMathMLmfracFrame::Place(nsIRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  
  
  nsBoundingMetrics bmNum, bmDen;
  nsHTMLReflowMetrics sizeNum;
  nsHTMLReflowMetrics sizeDen;
  nsIFrame* frameDen = nsnull;
  nsIFrame* frameNum = mFrames.FirstChild();
  if (frameNum) 
    frameDen = frameNum->GetNextSibling();
  if (!frameNum || !frameDen || frameDen->GetNextSibling()) {
    
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(frameNum, sizeNum, bmNum);
  GetReflowAndBoundingMetricsFor(frameDen, sizeDen, bmDen);

  
  

  nsPresContext* presContext = PresContext();
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull,
                            presContext->GetUserFontSet());
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));

  nscoord defaultRuleThickness, axisHeight;
  GetRuleThickness(aRenderingContext, fm, defaultRuleThickness);
  GetAxisHeight(aRenderingContext, fm, axisHeight);

  
  
  
  
  
  nsEmbellishData coreData;
  GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);
  nscoord leftSpace = PR_MAX(onePixel, coreData.leftSpace);
  nscoord rightSpace = PR_MAX(onePixel, coreData.rightSpace);

  
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::linethickness_, value);
  mLineRect.height = CalcLineThickness(presContext, mStyleContext, value,
                                       onePixel, defaultRuleThickness);
  nscoord numShift = 0;
  nscoord denShift = 0;

  
  nscoord numShift1, numShift2, numShift3;
  nscoord denShift1, denShift2;

  GetNumeratorShifts(fm, numShift1, numShift2, numShift3);
  GetDenominatorShifts(fm, denShift1, denShift2);
  if (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
    
    numShift = numShift1;
    denShift = denShift1;
  }
  else {
    numShift = (0 < mLineRect.height) ? numShift2 : numShift3;
    denShift = denShift2;
  }

  nscoord minClearance = 0;
  nscoord actualClearance = 0;

  nscoord actualRuleThickness =  mLineRect.height;

  if (0 == actualRuleThickness) {
    

    
    minClearance = (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) ?
      7 * defaultRuleThickness : 3 * defaultRuleThickness;
    actualClearance =
      (numShift - bmNum.descent) - (bmDen.ascent - denShift);
    
    if (actualClearance < minClearance) {
      nscoord halfGap = (minClearance - actualClearance)/2;
      numShift += halfGap;
      denShift += halfGap;
    }
  }
  else {
    

    

    
    


 
    
    
    
    
     minClearance = (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) ?
      3 * defaultRuleThickness : defaultRuleThickness + onePixel;

    
    actualClearance =
      (numShift - bmNum.descent) - (axisHeight + actualRuleThickness/2);
    if (actualClearance < minClearance) {
      numShift += (minClearance - actualClearance);
    }
    
    actualClearance =
      (axisHeight - actualRuleThickness/2) - (bmDen.ascent - denShift);
    if (actualClearance < minClearance) {
      denShift += (minClearance - actualClearance);
    }
  }

  
  

  
  
  nscoord width = PR_MAX(bmNum.width, bmDen.width);
  nscoord dxNum = leftSpace + (width - sizeNum.width)/2;
  nscoord dxDen = leftSpace + (width - sizeDen.width)/2;
  width += leftSpace + rightSpace;

  
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::numalign_,
               value);
  if (value.EqualsLiteral("left"))
    dxNum = leftSpace;
  else if (value.EqualsLiteral("right"))
    dxNum = width - rightSpace - sizeNum.width;

  
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::denomalign_,
               value);
  if (value.EqualsLiteral("left"))
    dxDen = leftSpace;
  else if (value.EqualsLiteral("right"))
    dxDen = width - rightSpace - sizeDen.width;

  mBoundingMetrics.rightBearing =
    PR_MAX(dxNum + bmNum.rightBearing, dxDen + bmDen.rightBearing);
  if (mBoundingMetrics.rightBearing < width - rightSpace)
    mBoundingMetrics.rightBearing = width - rightSpace;
  mBoundingMetrics.leftBearing =
    PR_MIN(dxNum + bmNum.leftBearing, dxDen + bmDen.leftBearing);
  if (mBoundingMetrics.leftBearing > leftSpace)
    mBoundingMetrics.leftBearing = leftSpace;
  mBoundingMetrics.ascent = bmNum.ascent + numShift;
  mBoundingMetrics.descent = bmDen.descent + denShift;
  mBoundingMetrics.width = width;

  aDesiredSize.ascent = sizeNum.ascent + numShift;
  aDesiredSize.height = aDesiredSize.ascent +
                        sizeDen.height - sizeDen.ascent + denShift;
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  if (aPlaceOrigin) {
    nscoord dy;
    
    dy = 0;
    FinishReflowChild(frameNum, presContext, nsnull, sizeNum, dxNum, dy, 0);
    
    dy = aDesiredSize.height - sizeDen.height;
    FinishReflowChild(frameDen, presContext, nsnull, sizeDen, dxDen, dy, 0);
    
    dy = aDesiredSize.ascent - (axisHeight + actualRuleThickness/2);
    mLineRect.SetRect(leftSpace, dy, width - (leftSpace + rightSpace), actualRuleThickness);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfracFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (nsGkAtoms::bevelled_ == aAttribute) {
    if (!IsBevelled()) {
      
      if (mSlashChar) {
        delete mSlashChar;
        mSlashChar = nsnull;
      }
    }
    else {
      
      if (!mSlashChar) {
        mSlashChar = new nsMathMLChar();
        if (mSlashChar) {
          nsPresContext* presContext = PresContext();
          nsAutoString slashChar; slashChar.Assign(kSlashChar);
          mSlashChar->SetData(presContext, slashChar);
          ResolveMathMLCharStyle(presContext, mContent, mStyleContext, mSlashChar, PR_TRUE);
        }
      }
    }
  }
  return nsMathMLContainerFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

NS_IMETHODIMP
nsMathMLmfracFrame::UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                                      PRInt32         aLastIndex,
                                                      PRUint32        aFlagsValues,
                                                      PRUint32        aFlagsToUpdate)
{
  
  
#if 0
  
  
  
  

  
  

  aFlagsToUpdate &= ~NS_MATHML_DISPLAYSTYLE;
  aFlagsValues &= ~NS_MATHML_DISPLAYSTYLE;
#endif
  return nsMathMLContainerFrame::
    UpdatePresentationDataFromChildAt(aFirstIndex, aLastIndex,
      aFlagsValues, aFlagsToUpdate);
}



nsStyleContext*
nsMathMLmfracFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  if (!mSlashChar) {
    return nsnull;
  }
  switch (aIndex) {
  case NS_SLASH_CHAR_STYLE_CONTEXT_INDEX:
    return mSlashChar->GetStyleContext();
    break;
  default:
    return nsnull;
  }
}

void
nsMathMLmfracFrame::SetAdditionalStyleContext(PRInt32          aIndex, 
                                              nsStyleContext*  aStyleContext)
{
  if (!mSlashChar) {
    return;
  }
  switch (aIndex) {
  case NS_SLASH_CHAR_STYLE_CONTEXT_INDEX:
    mSlashChar->SetStyleContext(aStyleContext);
    break;
  }
}
