





#include "nsMathMLmfracFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsDisplayList.h"
#include "gfxContext.h"
#include "nsMathMLElement.h"
#include <algorithm>







#define THIN_FRACTION_LINE                   0.5f
#define THIN_FRACTION_LINE_MINIMUM_PIXELS    1  // minimum of 1 pixel

#define THICK_FRACTION_LINE                  2.0f
#define THICK_FRACTION_LINE_MINIMUM_PIXELS   2  // minimum of 2 pixels

nsIFrame*
NS_NewMathMLmfracFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmfracFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmfracFrame)

nsMathMLmfracFrame::~nsMathMLmfracFrame()
{
}

eMathMLFrameType
nsMathMLmfracFrame::GetMathMLFrameType()
{
  
  return eMathMLFrameType_Inner;
}

uint8_t
nsMathMLmfracFrame::ScriptIncrement(nsIFrame* aFrame)
{
  if (!StyleFont()->mMathDisplay &&
      aFrame && (mFrames.FirstChild() == aFrame ||
                 mFrames.LastChild() == aFrame)) {
    return 1;
  }
  return 0;
}

NS_IMETHODIMP
nsMathMLmfracFrame::TransmitAutomaticData()
{
  
  
  UpdatePresentationDataFromChildAt(1,  1,
     NS_MATHML_COMPRESSED,
     NS_MATHML_COMPRESSED);

  
  
  if (!StyleFont()->mMathDisplay) {
    PropagateFrameFlagFor(mFrames.FirstChild(),
                          NS_FRAME_MATHML_SCRIPT_DESCENDANT);
    PropagateFrameFlagFor(mFrames.LastChild(),
                          NS_FRAME_MATHML_SCRIPT_DESCENDANT);
  }

  
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
      
    }
    else if (aThicknessAttribute.EqualsLiteral("thick")) {
      lineThickness = NSToCoordCeil(defaultThickness * THICK_FRACTION_LINE);
      minimumThickness = onePixel * THICK_FRACTION_LINE_MINIMUM_PIXELS;
      
      if (lineThickness < defaultThickness + onePixel)
        lineThickness = defaultThickness + onePixel;
    }
    else {
      
      lineThickness = defaultThickness;
      ParseNumericValue(aThicknessAttribute, &lineThickness,
                        nsMathMLElement::PARSE_ALLOW_UNITLESS,
                        aPresContext, aStyleContext);
    }
  }

  
  if (lineThickness && lineThickness < minimumThickness) 
    lineThickness = minimumThickness;

  return lineThickness;
}

void
nsMathMLmfracFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  
  
  
  if (mIsBevelled) {
    DisplaySlash(aBuilder, this, mLineRect, mLineThickness, aLists);
  } else {
    DisplayBar(aBuilder, this, mLineRect, aLists);
  }
}

 nsresult
nsMathMLmfracFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                    nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext,
                       false,
                       aDesiredSize,
                       true);
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
                          bool                 aPlaceOrigin,
                          nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext,
                       aPlaceOrigin,
                       aDesiredSize,
                       false);
}

nsresult
nsMathMLmfracFrame::PlaceInternal(nsRenderingContext& aRenderingContext,
                                  bool                 aPlaceOrigin,
                                  nsHTMLReflowMetrics& aDesiredSize,
                                  bool                 aWidthOnly)
{
  
  
  nsBoundingMetrics bmNum, bmDen;
  nsHTMLReflowMetrics sizeNum(aDesiredSize.GetWritingMode());
  nsHTMLReflowMetrics sizeDen(aDesiredSize.GetWritingMode());
  nsIFrame* frameDen = nullptr;
  nsIFrame* frameNum = mFrames.FirstChild();
  if (frameNum) 
    frameDen = frameNum->GetNextSibling();
  if (!frameNum || !frameDen || frameDen->GetNextSibling()) {
    
    if (aPlaceOrigin) {
      ReportChildCountError();
    }
    return ReflowError(aRenderingContext, aDesiredSize);
  }
  GetReflowAndBoundingMetricsFor(frameNum, sizeNum, bmNum);
  GetReflowAndBoundingMetricsFor(frameDen, sizeDen, bmDen);

  nsPresContext* presContext = PresContext();
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  nscoord defaultRuleThickness, axisHeight;
  GetRuleThickness(aRenderingContext, fm, defaultRuleThickness);
  GetAxisHeight(aRenderingContext, fm, axisHeight);

  bool outermostEmbellished = false;
  if (mEmbellishData.coreFrame) {
    nsEmbellishData parentData;
    GetEmbellishDataFrom(GetParent(), parentData);
    outermostEmbellished = parentData.coreFrame != mEmbellishData.coreFrame;
  }

  
  nsAutoString value;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::linethickness_, value);
  mLineThickness = CalcLineThickness(presContext, mStyleContext, value,
                                     onePixel, defaultRuleThickness);

  
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::bevelled_, value);
  mIsBevelled = value.EqualsLiteral("true");

  if (!mIsBevelled) {
    mLineRect.height = mLineThickness;

    
    
    
    
    
    nscoord leftSpace = onePixel;
    nscoord rightSpace = onePixel;
    if (outermostEmbellished) {
      nsEmbellishData coreData;
      GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);
      leftSpace += StyleVisibility()->mDirection ?
                     coreData.trailingSpace : coreData.leadingSpace;
      rightSpace += StyleVisibility()->mDirection ?
                      coreData.leadingSpace : coreData.trailingSpace;
    }

    
    
    nscoord numShift = 0;
    nscoord denShift = 0;

    
    nscoord numShift1, numShift2, numShift3;
    nscoord denShift1, denShift2;

    GetNumeratorShifts(fm, numShift1, numShift2, numShift3);
    GetDenominatorShifts(fm, denShift1, denShift2);
    if (StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK) {
      
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
      

      
      minClearance = StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK ?
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
    

    

    
    
    
    
 
    
    
    
    
     minClearance = StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK ?
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

    
    

    
    
    nscoord width = std::max(bmNum.width, bmDen.width);
    nscoord dxNum = leftSpace + (width - sizeNum.Width())/2;
    nscoord dxDen = leftSpace + (width - sizeDen.Width())/2;
    width += leftSpace + rightSpace;

    
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::numalign_, value);
    if (value.EqualsLiteral("left"))
      dxNum = leftSpace;
    else if (value.EqualsLiteral("right"))
      dxNum = width - rightSpace - sizeNum.Width();

    
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::denomalign_, value);
    if (value.EqualsLiteral("left"))
      dxDen = leftSpace;
    else if (value.EqualsLiteral("right"))
      dxDen = width - rightSpace - sizeDen.Width();

    mBoundingMetrics.rightBearing =
      std::max(dxNum + bmNum.rightBearing, dxDen + bmDen.rightBearing);
    if (mBoundingMetrics.rightBearing < width - rightSpace)
      mBoundingMetrics.rightBearing = width - rightSpace;
    mBoundingMetrics.leftBearing =
      std::min(dxNum + bmNum.leftBearing, dxDen + bmDen.leftBearing);
    if (mBoundingMetrics.leftBearing > leftSpace)
      mBoundingMetrics.leftBearing = leftSpace;
    mBoundingMetrics.ascent = bmNum.ascent + numShift;
    mBoundingMetrics.descent = bmDen.descent + denShift;
    mBoundingMetrics.width = width;

    aDesiredSize.SetTopAscent(sizeNum.TopAscent() + numShift);
    aDesiredSize.Height() = aDesiredSize.TopAscent() +
      sizeDen.Height() - sizeDen.TopAscent() + denShift;
    aDesiredSize.Width() = mBoundingMetrics.width;
    aDesiredSize.mBoundingMetrics = mBoundingMetrics;

    mReference.x = 0;
    mReference.y = aDesiredSize.TopAscent();

    if (aPlaceOrigin) {
      nscoord dy;
      
      dy = 0;
      FinishReflowChild(frameNum, presContext, sizeNum, nullptr, dxNum, dy, 0);
      
      dy = aDesiredSize.Height() - sizeDen.Height();
      FinishReflowChild(frameDen, presContext, sizeDen, nullptr, dxDen, dy, 0);
      
      dy = aDesiredSize.TopAscent() - (axisHeight + actualRuleThickness/2);
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
      std::min(2 * mLineThickness, slashMaxWidthConstant);

    nscoord leadingSpace = padding;
    nscoord trailingSpace = padding;
    if (outermostEmbellished) {
      nsEmbellishData coreData;
      GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);
      leadingSpace += coreData.leadingSpace;
      trailingSpace += coreData.trailingSpace;
    }
    nscoord delta;
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    delta = std::max(bmDen.ascent - bmNum.ascent,
                   bmNum.descent - bmDen.descent) / 2;
    if (delta > 0) {
      numShift += delta;
      denShift += delta;
    }

    if (StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK) {
      delta = std::min(bmDen.ascent + bmDen.descent,
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
        std::min(slashMaxWidthConstant,
               (mBoundingMetrics.ascent + mBoundingMetrics.descent) /
               slashRatio);
    }

    
    if (StyleVisibility()->mDirection) {
      mBoundingMetrics.leftBearing = trailingSpace + bmDen.leftBearing;
      mBoundingMetrics.rightBearing = trailingSpace + bmDen.width + mLineRect.width + bmNum.rightBearing;
    } else {
      mBoundingMetrics.leftBearing = leadingSpace + bmNum.leftBearing;
      mBoundingMetrics.rightBearing = leadingSpace + bmNum.width + mLineRect.width + bmDen.rightBearing;
    }
    mBoundingMetrics.width =
      leadingSpace + bmNum.width + mLineRect.width + bmDen.width +
      trailingSpace;

    
    aDesiredSize.SetTopAscent(mBoundingMetrics.ascent + padding);
    aDesiredSize.Height() =
      mBoundingMetrics.ascent + mBoundingMetrics.descent + 2 * padding;
    aDesiredSize.Width() = mBoundingMetrics.width;
    aDesiredSize.mBoundingMetrics = mBoundingMetrics;

    mReference.x = 0;
    mReference.y = aDesiredSize.TopAscent();
    
    if (aPlaceOrigin) {
      nscoord dx, dy;

      
      dx = MirrorIfRTL(aDesiredSize.Width(), sizeNum.Width(),
                       leadingSpace);
      dy = aDesiredSize.TopAscent() - numShift - sizeNum.TopAscent();
      FinishReflowChild(frameNum, presContext, sizeNum, nullptr, dx, dy, 0);

      
      dx = MirrorIfRTL(aDesiredSize.Width(), mLineRect.width,
                       leadingSpace + bmNum.width);
      dy = aDesiredSize.TopAscent() - mBoundingMetrics.ascent;
      mLineRect.SetRect(dx, dy,
                        mLineRect.width, aDesiredSize.Height() - 2 * padding);

      
      dx = MirrorIfRTL(aDesiredSize.Width(), sizeDen.Width(),
                       leadingSpace + bmNum.width + mLineRect.width);
      dy = aDesiredSize.TopAscent() + denShift - sizeDen.TopAscent();
      FinishReflowChild(frameDen, presContext, sizeDen, nullptr, dx, dy, 0);
    }

  }

  return NS_OK;
}

class nsDisplayMathMLSlash : public nsDisplayItem {
public:
  nsDisplayMathMLSlash(nsDisplayListBuilder* aBuilder,
                       nsIFrame* aFrame, const nsRect& aRect,
                       nscoord aThickness, bool aRTL)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect), mThickness(aThickness),
      mRTL(aRTL) {
    MOZ_COUNT_CTOR(nsDisplayMathMLSlash);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLSlash() {
    MOZ_COUNT_DTOR(nsDisplayMathMLSlash);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("MathMLSlash", TYPE_MATHML_SLASH)

private:
  nsRect    mRect;
  nscoord   mThickness;
  bool      mRTL;
};

void nsDisplayMathMLSlash::Paint(nsDisplayListBuilder* aBuilder,
                                 nsRenderingContext* aCtx)
{
  
  nsPresContext* presContext = mFrame->PresContext();
  gfxRect rect = presContext->AppUnitsToGfxUnits(mRect + ToReferenceFrame());
  
  
  aCtx->SetColor(mFrame->GetVisitedDependentColor(eCSSProperty_color));
 
  
  gfxContext *gfxCtx = aCtx->ThebesContext();
  gfxPoint delta = gfxPoint(presContext->AppUnitsToGfxUnits(mThickness), 0);
  gfxCtx->NewPath();

  if (mRTL) {
    gfxCtx->MoveTo(rect.TopLeft());
    gfxCtx->LineTo(rect.TopLeft() + delta);
    gfxCtx->LineTo(rect.BottomRight());
    gfxCtx->LineTo(rect.BottomRight() - delta);
  } else {
    gfxCtx->MoveTo(rect.BottomLeft());
    gfxCtx->LineTo(rect.BottomLeft() + delta);
    gfxCtx->LineTo(rect.TopRight());
    gfxCtx->LineTo(rect.TopRight() - delta);
  }

  gfxCtx->ClosePath();
  gfxCtx->Fill();
}

void
nsMathMLmfracFrame::DisplaySlash(nsDisplayListBuilder* aBuilder,
                                 nsIFrame* aFrame, const nsRect& aRect,
                                 nscoord aThickness,
                                 const nsDisplayListSet& aLists) {
  if (!aFrame->StyleVisibility()->IsVisible() || aRect.IsEmpty())
    return;

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayMathMLSlash(aBuilder, aFrame, aRect, aThickness,
                         StyleVisibility()->mDirection));
}
