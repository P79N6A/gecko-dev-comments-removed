









































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsRenderingContext.h"

#include "nsMathMLmfracFrame.h"
#include "nsDisplayList.h"
#include "gfxContext.h"







#define THIN_FRACTION_LINE                   0.5f
#define THIN_FRACTION_LINE_MINIMUM_PIXELS    1  // minimum of 1 pixel

#define MEDIUM_FRACTION_LINE                 1.5f
#define MEDIUM_FRACTION_LINE_MINIMUM_PIXELS  2  // minimum of 2 pixels

#define THICK_FRACTION_LINE                  2.0f
#define THICK_FRACTION_LINE_MINIMUM_PIXELS   4  // minimum of 4 pixels

nsIFrame*
NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmfracFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmfracFrame)

nsMathMLmfracFrame::~nsMathMLmfracFrame()
{
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

  mIsBevelled = IsBevelled();

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
  
  
  
  if (mIsBevelled) {
    rv = DisplaySlash(aBuilder, this, mLineRect, mLineThickness, aLists);
  } else {
    rv = DisplayBar(aBuilder, this, mLineRect, aLists);
  }

  return rv;
}

 nsresult
nsMathMLmfracFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                    nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext,
                       PR_FALSE,
                       aDesiredSize,
                       PR_TRUE);
}

nscoord
nsMathMLmfracFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap) return 0;

  mLineRect.MoveBy(gap, 0);
  return gap;
}

 nsresult
nsMathMLmfracFrame::Place(nsRenderingContext& aRenderingContext,
                          PRBool               aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext,
                       aPlaceOrigin,
                       aDesiredSize,
                       PR_FALSE);
}

nsresult
nsMathMLmfracFrame::PlaceInternal(nsRenderingContext& aRenderingContext,
                                  PRBool               aPlaceOrigin,
                                  nsHTMLReflowMetrics& aDesiredSize,
                                  PRBool               aWidthOnly)
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

  aRenderingContext.SetFont(GetStyleFont()->mFont,
                            presContext->GetUserFontSet());
  nsFontMetrics* fm = aRenderingContext.FontMetrics();

  nscoord defaultRuleThickness, axisHeight;
  GetRuleThickness(aRenderingContext, fm, defaultRuleThickness);
  GetAxisHeight(aRenderingContext, fm, axisHeight);

  nsEmbellishData coreData;
  GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);

  
  nsAutoString value;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::linethickness_,
               value);

  mLineThickness = CalcLineThickness(presContext, mStyleContext, value,
                                     onePixel, defaultRuleThickness);

  if (!mIsBevelled) {
    mLineRect.height = mLineThickness;
    
    
    
    
    
    
    nscoord leftSpace = NS_MAX(onePixel, coreData.leftSpace);
    nscoord rightSpace = NS_MAX(onePixel, coreData.rightSpace);

    
    
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

    nscoord actualRuleThickness =  mLineThickness;

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

    
    

    
    
    nscoord width = NS_MAX(bmNum.width, bmDen.width);
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
      NS_MAX(dxNum + bmNum.rightBearing, dxDen + bmDen.rightBearing);
    if (mBoundingMetrics.rightBearing < width - rightSpace)
      mBoundingMetrics.rightBearing = width - rightSpace;
    mBoundingMetrics.leftBearing =
      NS_MIN(dxNum + bmNum.leftBearing, dxDen + bmDen.leftBearing);
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
      mLineRect.SetRect(leftSpace, dy, width - (leftSpace + rightSpace),
                        actualRuleThickness);
    }
  } else {
    nscoord numShift = 0.0;
    nscoord denShift = 0.0;
    nscoord padding = 3 * defaultRuleThickness;
    nscoord slashRatio = 3;

    
    nscoord em = fm->EmHeight();
    nscoord slashMaxWidthConstant = 2 * em;

    
    
    nscoord slashMinHeight = slashRatio *
      NS_MIN(2 * mLineThickness, slashMaxWidthConstant);

    nscoord leftSpace = NS_MAX(padding, coreData.leftSpace);
    nscoord rightSpace = NS_MAX(padding, coreData.rightSpace);
    nscoord delta;
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    delta = NS_MAX(bmDen.ascent - bmNum.ascent,
                   bmNum.descent - bmDen.descent) / 2;
    if (delta > 0) {
      numShift += delta;
      denShift += delta;
    }

    if (NS_MATHML_IS_DISPLAYSTYLE(mPresentationData.flags)) {
      delta = NS_MIN(bmDen.ascent + bmDen.descent,
                     bmNum.ascent + bmNum.descent) / 2;
      numShift += delta;
      denShift += delta;
    } else {
      nscoord xHeight = fm->XHeight();
      numShift += xHeight / 2;
      denShift += xHeight / 4;
    }
   
    
    mBoundingMetrics.ascent = bmNum.ascent + numShift;
    mBoundingMetrics.descent = bmDen.descent + denShift;

    
    
    
    delta = (slashMinHeight -
             (mBoundingMetrics.ascent + mBoundingMetrics.descent)) / 2;
    if (delta > 0) {
      mBoundingMetrics.ascent += delta;
      mBoundingMetrics.descent += delta;
    }

    
    if (aWidthOnly) {
      mLineRect.width = mLineThickness + slashMaxWidthConstant;
    } else {
      mLineRect.width = mLineThickness +
        NS_MIN(slashMaxWidthConstant,
               (mBoundingMetrics.ascent + mBoundingMetrics.descent) /
               slashRatio);
    }

    
    mBoundingMetrics.leftBearing = leftSpace + bmNum.leftBearing;
    mBoundingMetrics.rightBearing =
      leftSpace + bmNum.width + mLineRect.width + bmDen.rightBearing;
    mBoundingMetrics.width =
      leftSpace + bmNum.width + mLineRect.width + bmDen.width + rightSpace;

    
    aDesiredSize.ascent = mBoundingMetrics.ascent + padding;
    aDesiredSize.height =
      mBoundingMetrics.ascent + mBoundingMetrics.descent + 2 * padding;
    aDesiredSize.width = mBoundingMetrics.width;
    aDesiredSize.mBoundingMetrics = mBoundingMetrics;

    mReference.x = 0;
    mReference.y = aDesiredSize.ascent;
    
    if (aPlaceOrigin) {
      FinishReflowChild(frameNum, presContext, nsnull, sizeNum,
                        leftSpace,
                        aDesiredSize.ascent - numShift - sizeNum.ascent, 0);

      mLineRect.SetRect(leftSpace + bmNum.width,
                        aDesiredSize.ascent - mBoundingMetrics.ascent,
                        mLineRect.width,
                        aDesiredSize.height - 2 * padding);

      FinishReflowChild(frameDen, presContext, nsnull, sizeDen,
                        leftSpace + bmNum.width + mLineRect.width,
                        aDesiredSize.ascent + denShift - sizeDen.ascent, 0);
    }

  }

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmfracFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (nsGkAtoms::bevelled_ == aAttribute) {
    mIsBevelled = IsBevelled();
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

class nsDisplayMathMLSlash : public nsDisplayItem {
public:
  nsDisplayMathMLSlash(nsDisplayListBuilder* aBuilder,
                       nsIFrame* aFrame, const nsRect& aRect,
                       nscoord aThickness)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect), mThickness(aThickness) {
    MOZ_COUNT_CTOR(nsDisplayMathMLSlash);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLSlash() {
    MOZ_COUNT_DTOR(nsDisplayMathMLSlash);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLSlash", TYPE_MATHML_SLASH)

private:
  nsRect    mRect;
  nscoord   mThickness;
};

void nsDisplayMathMLSlash::Paint(nsDisplayListBuilder* aBuilder,
                                 nsRenderingContext* aCtx)
{
  
  nsPresContext* presContext = mFrame->PresContext();
  gfxRect rect = presContext->AppUnitsToGfxUnits(mRect + ToReferenceFrame());
  
  
  aCtx->SetColor(mFrame->GetStyleColor()->mColor);
 
  
  gfxContext *gfxCtx = aCtx->ThebesContext();
  gfxPoint delta = gfxPoint(presContext->AppUnitsToGfxUnits(mThickness), 0);
  gfxCtx->NewPath();
  gfxCtx->MoveTo(rect.BottomLeft());
  gfxCtx->LineTo(rect.BottomLeft() + delta);
  gfxCtx->LineTo(rect.TopRight());
  gfxCtx->LineTo(rect.TopRight() - delta);
  gfxCtx->ClosePath();
  gfxCtx->Fill();
}

nsresult
nsMathMLmfracFrame::DisplaySlash(nsDisplayListBuilder* aBuilder,
                                 nsIFrame* aFrame, const nsRect& aRect,
                                 nscoord aThickness,
                                 const nsDisplayListSet& aLists) {
  if (!aFrame->GetStyleVisibility()->IsVisible() || aRect.IsEmpty())
    return NS_OK;
  
  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayMathMLSlash(aBuilder, aFrame, aRect, aThickness));
}
