




#include "nsMathMLmencloseFrame.h"

#include "gfx2DGlue.h"
#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsWhitespaceTokenizer.h"

#include "nsDisplayList.h"
#include "gfxContext.h"
#include "nsMathMLChar.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;








static const char16_t kLongDivChar = ')';


static const char16_t kRadicalChar = 0x221A;


static const uint8_t kArrowHeadSize = 10;


static const uint8_t kPhasorangleWidth = 8;

nsIFrame*
NS_NewMathMLmencloseFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmencloseFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmencloseFrame)

nsMathMLmencloseFrame::nsMathMLmencloseFrame(nsStyleContext* aContext) :
  nsMathMLContainerFrame(aContext), mNotationsToDraw(0),
  mLongDivCharIndex(-1), mRadicalCharIndex(-1), mContentWidth(0)
{
}

nsMathMLmencloseFrame::~nsMathMLmencloseFrame()
{
}

nsresult nsMathMLmencloseFrame::AllocateMathMLChar(nsMencloseNotation mask)
{
  
  if ((mask == NOTATION_LONGDIV && mLongDivCharIndex >= 0) ||
      (mask == NOTATION_RADICAL && mRadicalCharIndex >= 0))
    return NS_OK;

  
  
  
  uint32_t i = mMathMLChar.Length();
  nsAutoString Char;

  if (!mMathMLChar.AppendElement())
    return NS_ERROR_OUT_OF_MEMORY;

  if (mask == NOTATION_LONGDIV) {
    Char.Assign(kLongDivChar);
    mLongDivCharIndex = i;
  } else if (mask == NOTATION_RADICAL) {
    Char.Assign(kRadicalChar);
    mRadicalCharIndex = i;
  }

  nsPresContext *presContext = PresContext();
  mMathMLChar[i].SetData(presContext, Char);
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mMathMLChar[i]);

  return NS_OK;
}





nsresult nsMathMLmencloseFrame::AddNotation(const nsAString& aNotation)
{
  nsresult rv;

  if (aNotation.EqualsLiteral("longdiv")) {
    rv = AllocateMathMLChar(NOTATION_LONGDIV);
    NS_ENSURE_SUCCESS(rv, rv);
    mNotationsToDraw |= NOTATION_LONGDIV;
  } else if (aNotation.EqualsLiteral("actuarial")) {
    mNotationsToDraw |= (NOTATION_RIGHT | NOTATION_TOP);
  } else if (aNotation.EqualsLiteral("radical")) {
    rv = AllocateMathMLChar(NOTATION_RADICAL);
    NS_ENSURE_SUCCESS(rv, rv);
    mNotationsToDraw |= NOTATION_RADICAL;
  } else if (aNotation.EqualsLiteral("box")) {
    mNotationsToDraw |= (NOTATION_LEFT | NOTATION_RIGHT |
                         NOTATION_TOP | NOTATION_BOTTOM);
  } else if (aNotation.EqualsLiteral("roundedbox")) {
    mNotationsToDraw |= NOTATION_ROUNDEDBOX;
  } else if (aNotation.EqualsLiteral("circle")) {
    mNotationsToDraw |= NOTATION_CIRCLE;
  } else if (aNotation.EqualsLiteral("left")) {
    mNotationsToDraw |= NOTATION_LEFT;
  } else if (aNotation.EqualsLiteral("right")) {
    mNotationsToDraw |= NOTATION_RIGHT;
  } else if (aNotation.EqualsLiteral("top")) {
    mNotationsToDraw |= NOTATION_TOP;
  } else if (aNotation.EqualsLiteral("bottom")) {
    mNotationsToDraw |= NOTATION_BOTTOM;
  } else if (aNotation.EqualsLiteral("updiagonalstrike")) {
    mNotationsToDraw |= NOTATION_UPDIAGONALSTRIKE;
  } else if (aNotation.EqualsLiteral("updiagonalarrow")) {
    mNotationsToDraw |= NOTATION_UPDIAGONALARROW;
  } else if (aNotation.EqualsLiteral("downdiagonalstrike")) {
    mNotationsToDraw |= NOTATION_DOWNDIAGONALSTRIKE;
  } else if (aNotation.EqualsLiteral("verticalstrike")) {
    mNotationsToDraw |= NOTATION_VERTICALSTRIKE;
  } else if (aNotation.EqualsLiteral("horizontalstrike")) {
    mNotationsToDraw |= NOTATION_HORIZONTALSTRIKE;
  } else if (aNotation.EqualsLiteral("madruwb")) {
    mNotationsToDraw |= (NOTATION_RIGHT | NOTATION_BOTTOM);
  } else if (aNotation.EqualsLiteral("phasorangle")) {
    mNotationsToDraw |= (NOTATION_BOTTOM | NOTATION_PHASORANGLE);
  }

  return NS_OK;
}




void nsMathMLmencloseFrame::InitNotations()
{
  mNotationsToDraw = 0;
  mLongDivCharIndex = mRadicalCharIndex = -1;
  mMathMLChar.Clear();

  nsAutoString value;

  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::notation_, value)) {
    
    nsWhitespaceTokenizer tokenizer(value);

    while (tokenizer.hasMoreTokens())
      AddNotation(tokenizer.nextToken());

    if (IsToDraw(NOTATION_UPDIAGONALARROW)) {
      
      
      
      
      mNotationsToDraw &= ~NOTATION_UPDIAGONALSTRIKE;
    }
  } else {
    
    if (NS_FAILED(AllocateMathMLChar(NOTATION_LONGDIV)))
      return;
    mNotationsToDraw = NOTATION_LONGDIV;
  }
}

NS_IMETHODIMP
nsMathMLmencloseFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsMathMLContainerFrame::InheritAutomaticData(aParent);

  mPresentationData.flags |= NS_MATHML_STRETCH_ALL_CHILDREN_VERTICALLY;

  InitNotations();

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmencloseFrame::TransmitAutomaticData()
{
  if (IsToDraw(NOTATION_RADICAL)) {
    
    UpdatePresentationDataFromChildAt(0, -1,
                                      NS_MATHML_COMPRESSED,
                                      NS_MATHML_COMPRESSED);
  }

  return NS_OK;
}

void
nsMathMLmencloseFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  
  
  nsMathMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);

  if (NS_MATHML_HAS_ERROR(mPresentationData.flags))
    return;

  nsRect mencloseRect = nsIFrame::GetRect();
  mencloseRect.x = mencloseRect.y = 0;

  if (IsToDraw(NOTATION_RADICAL)) {
    mMathMLChar[mRadicalCharIndex].Display(aBuilder, this, aLists, 0);

    nsRect rect;
    mMathMLChar[mRadicalCharIndex].GetRect(rect);
    rect.MoveBy(StyleVisibility()->mDirection ? -mContentWidth : rect.width, 0);
    rect.SizeTo(mContentWidth, mRadicalRuleThickness);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_PHASORANGLE)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                mRuleThickness, NOTATION_PHASORANGLE);
  }

  if (IsToDraw(NOTATION_LONGDIV)) {
    mMathMLChar[mLongDivCharIndex].Display(aBuilder, this, aLists, 1);

    nsRect rect;
    mMathMLChar[mLongDivCharIndex].GetRect(rect);
    rect.SizeTo(rect.width + mContentWidth, mRuleThickness);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_TOP)) {
    nsRect rect(0, 0, mencloseRect.width, mRuleThickness);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_BOTTOM)) {
    nsRect rect(0, mencloseRect.height - mRuleThickness,
                mencloseRect.width, mRuleThickness);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_LEFT)) {
    nsRect rect(0, 0, mRuleThickness, mencloseRect.height);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_RIGHT)) {
    nsRect rect(mencloseRect.width - mRuleThickness, 0,
                mRuleThickness, mencloseRect.height);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_ROUNDEDBOX)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                    mRuleThickness, NOTATION_ROUNDEDBOX);
  }

  if (IsToDraw(NOTATION_CIRCLE)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                    mRuleThickness, NOTATION_CIRCLE);
  }

  if (IsToDraw(NOTATION_UPDIAGONALSTRIKE)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                    mRuleThickness, NOTATION_UPDIAGONALSTRIKE);
  }

  if (IsToDraw(NOTATION_UPDIAGONALARROW)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                    mRuleThickness, NOTATION_UPDIAGONALARROW);
  }

  if (IsToDraw(NOTATION_DOWNDIAGONALSTRIKE)) {
    DisplayNotation(aBuilder, this, mencloseRect, aLists,
                    mRuleThickness, NOTATION_DOWNDIAGONALSTRIKE);
  }

  if (IsToDraw(NOTATION_HORIZONTALSTRIKE)) {
    nsRect rect(0, mencloseRect.height / 2 - mRuleThickness / 2,
                mencloseRect.width, mRuleThickness);
    DisplayBar(aBuilder, this, rect, aLists);
  }

  if (IsToDraw(NOTATION_VERTICALSTRIKE)) {
    nsRect rect(mencloseRect.width / 2 - mRuleThickness / 2, 0,
                mRuleThickness, mencloseRect.height);
    DisplayBar(aBuilder, this, rect, aLists);
  }
}

 nsresult
nsMathMLmencloseFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                       nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext, false, aDesiredSize, true);
}

 nsresult
nsMathMLmencloseFrame::Place(nsRenderingContext& aRenderingContext,
                             bool                 aPlaceOrigin,
                             nsHTMLReflowMetrics& aDesiredSize)
{
  return PlaceInternal(aRenderingContext, aPlaceOrigin, aDesiredSize, false);
}

 nsresult
nsMathMLmencloseFrame::PlaceInternal(nsRenderingContext& aRenderingContext,
                                     bool                 aPlaceOrigin,
                                     nsHTMLReflowMetrics& aDesiredSize,
                                     bool                 aWidthOnly)
{
  
  
  
  nsHTMLReflowMetrics baseSize(aDesiredSize.GetWritingMode());
  nsresult rv =
    nsMathMLContainerFrame::Place(aRenderingContext, false, baseSize);

  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
      DidReflowChildren(GetFirstPrincipalChild());
      return rv;
    }

  nsBoundingMetrics bmBase = baseSize.mBoundingMetrics;
  nscoord dx_left = 0, dx_right = 0;
  nsBoundingMetrics bmLongdivChar, bmRadicalChar;
  nscoord radicalAscent = 0, radicalDescent = 0;
  nscoord longdivAscent = 0, longdivDescent = 0;
  nscoord psi = 0;
  nscoord leading = 0;

  
  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        fontSizeInflation);
  GetRuleThickness(aRenderingContext, fm, mRuleThickness);
  if (mRuleThickness < onePixel) {
    mRuleThickness = onePixel;
  }

  char16_t one = '1';
  nsBoundingMetrics bmOne =
    nsLayoutUtils::AppUnitBoundsOfString(&one, 1, *fm, aRenderingContext);

  
  
  

  
  nscoord padding = 3 * mRuleThickness;
  nscoord delta = padding % onePixel;
  if (delta)
    padding += onePixel - delta; 

  if (IsToDraw(NOTATION_LONGDIV) || IsToDraw(NOTATION_RADICAL)) {
    GetRadicalParameters(fm, StyleFont()->mMathDisplay ==
                         NS_MATHML_DISPLAYSTYLE_BLOCK,
                         mRadicalRuleThickness, leading, psi);

    
    if (mRadicalRuleThickness < onePixel) {
      mRadicalRuleThickness = onePixel;
    }

    
    
    delta = psi % onePixel;
    if (delta) {
      psi += onePixel - delta; 
    }
  }

  
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    dx_left = padding;

  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    dx_right = padding;

  
  if (IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_UPDIAGONALARROW) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_RADICAL) ||
      IsToDraw(NOTATION_LONGDIV) ||
      IsToDraw(NOTATION_PHASORANGLE)) {
      
      bmBase.ascent = std::max(bmOne.ascent, bmBase.ascent);
      bmBase.descent = std::max(0, bmBase.descent);
  }

  mBoundingMetrics.ascent = bmBase.ascent;
  mBoundingMetrics.descent = bmBase.descent;
    
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_CIRCLE))
    mBoundingMetrics.ascent += padding;
  
  if (IsToDraw(NOTATION_ROUNDEDBOX) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_CIRCLE))
    mBoundingMetrics.descent += padding;

   
   
  if (IsToDraw(NOTATION_PHASORANGLE)) {
    nscoord phasorangleWidth = kPhasorangleWidth * mRuleThickness;
    
    dx_left = std::max(dx_left, phasorangleWidth);
  }

  
  
  
  if (IsToDraw(NOTATION_UPDIAGONALARROW)) {
    
    nscoord arrowHeadSize = kArrowHeadSize * mRuleThickness;

    
    
    
    
    dx_right = std::max(dx_right, arrowHeadSize);
    mBoundingMetrics.ascent = std::max(mBoundingMetrics.ascent, arrowHeadSize);
  }

  
  
  
  
  if (IsToDraw(NOTATION_CIRCLE)) {
    double ratio = (sqrt(2.0) - 1.0) / 2.0;
    nscoord padding2;

    
    padding2 = ratio * bmBase.width;

    dx_left = std::max(dx_left, padding2);
    dx_right = std::max(dx_right, padding2);

    
    padding2 = ratio * (bmBase.ascent + bmBase.descent);

    mBoundingMetrics.ascent = std::max(mBoundingMetrics.ascent,
                                     bmBase.ascent + padding2);
    mBoundingMetrics.descent = std::max(mBoundingMetrics.descent,
                                      bmBase.descent + padding2);
  }

  
  
  if (IsToDraw(NOTATION_LONGDIV)) {
    if (aWidthOnly) {
        nscoord longdiv_width = mMathMLChar[mLongDivCharIndex].
          GetMaxWidth(PresContext(), aRenderingContext, fontSizeInflation);

        
        dx_left = std::max(dx_left, longdiv_width);
    } else {
      
      
      nsBoundingMetrics contSize = bmBase;
      contSize.ascent = mRuleThickness;
      contSize.descent = bmBase.ascent + bmBase.descent + psi;

      
      mMathMLChar[mLongDivCharIndex].Stretch(PresContext(), aRenderingContext,
                                             fontSizeInflation,
                                             NS_STRETCH_DIRECTION_VERTICAL,
                                             contSize, bmLongdivChar,
                                             NS_STRETCH_LARGER, false);
      mMathMLChar[mLongDivCharIndex].GetBoundingMetrics(bmLongdivChar);

      
      dx_left = std::max(dx_left, bmLongdivChar.width);

      
      longdivAscent = bmBase.ascent + psi + mRuleThickness;
      longdivDescent = std::max(bmBase.descent,
                              (bmLongdivChar.ascent + bmLongdivChar.descent -
                               longdivAscent));

      mBoundingMetrics.ascent = std::max(mBoundingMetrics.ascent,
                                       longdivAscent);
      mBoundingMetrics.descent = std::max(mBoundingMetrics.descent,
                                        longdivDescent);
    }
  }

  
  
  if (IsToDraw(NOTATION_RADICAL)) {
    nscoord *dx_leading = StyleVisibility()->mDirection ? &dx_right : &dx_left;
    
    if (aWidthOnly) {
      nscoord radical_width = mMathMLChar[mRadicalCharIndex].
        GetMaxWidth(PresContext(), aRenderingContext, fontSizeInflation);
      
      
      *dx_leading = std::max(*dx_leading, radical_width);
    } else {
      
      
      nsBoundingMetrics contSize = bmBase;
      contSize.ascent = mRadicalRuleThickness;
      contSize.descent = bmBase.ascent + bmBase.descent + psi;

      
      mMathMLChar[mRadicalCharIndex].Stretch(PresContext(), aRenderingContext,
                                             fontSizeInflation,
                                             NS_STRETCH_DIRECTION_VERTICAL,
                                             contSize, bmRadicalChar,
                                             NS_STRETCH_LARGER,
                                             StyleVisibility()->mDirection);
      mMathMLChar[mRadicalCharIndex].GetBoundingMetrics(bmRadicalChar);

      
      *dx_leading = std::max(*dx_leading, bmRadicalChar.width);

      
      radicalAscent = bmBase.ascent + psi + mRadicalRuleThickness;
      radicalDescent = std::max(bmBase.descent,
                              (bmRadicalChar.ascent + bmRadicalChar.descent -
                               radicalAscent));

      mBoundingMetrics.ascent = std::max(mBoundingMetrics.ascent,
                                       radicalAscent);
      mBoundingMetrics.descent = std::max(mBoundingMetrics.descent,
                                        radicalDescent);
    }
  }

  
  
  if (IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      (IsToDraw(NOTATION_LEFT) && IsToDraw(NOTATION_RIGHT))) {
    
    dx_left = dx_right = std::max(dx_left, dx_right);
  }

  
  
  mBoundingMetrics.width = dx_left + bmBase.width + dx_right;

  mBoundingMetrics.leftBearing = std::min(0, dx_left + bmBase.leftBearing);
  mBoundingMetrics.rightBearing =
    std::max(mBoundingMetrics.width, dx_left + bmBase.rightBearing);
  
  aDesiredSize.Width() = mBoundingMetrics.width;

  aDesiredSize.SetBlockStartAscent(std::max(mBoundingMetrics.ascent,
                                            baseSize.BlockStartAscent()));
  aDesiredSize.Height() = aDesiredSize.BlockStartAscent() +
    std::max(mBoundingMetrics.descent,
             baseSize.Height() - baseSize.BlockStartAscent());

  if (IsToDraw(NOTATION_LONGDIV) || IsToDraw(NOTATION_RADICAL)) {
    nscoord desiredSizeAscent = aDesiredSize.BlockStartAscent();
    nscoord desiredSizeDescent = aDesiredSize.Height() -
                                 aDesiredSize.BlockStartAscent();
    
    if (IsToDraw(NOTATION_LONGDIV)) {
      desiredSizeAscent = std::max(desiredSizeAscent,
                                 longdivAscent + leading);
      desiredSizeDescent = std::max(desiredSizeDescent,
                                  longdivDescent + mRuleThickness);
    }
    
    if (IsToDraw(NOTATION_RADICAL)) {
      desiredSizeAscent = std::max(desiredSizeAscent,
                                 radicalAscent + leading);
      desiredSizeDescent = std::max(desiredSizeDescent,
                                    radicalDescent + mRadicalRuleThickness);
    }

    aDesiredSize.SetBlockStartAscent(desiredSizeAscent);
    aDesiredSize.Height() = desiredSizeAscent + desiredSizeDescent;
  }
    
  if (IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX) ||
      (IsToDraw(NOTATION_TOP) && IsToDraw(NOTATION_BOTTOM))) {
    
    nscoord dy = std::max(aDesiredSize.BlockStartAscent() - bmBase.ascent,
                          aDesiredSize.Height() -
                          aDesiredSize.BlockStartAscent() - bmBase.descent);

    aDesiredSize.SetBlockStartAscent(bmBase.ascent + dy);
    aDesiredSize.Height() = aDesiredSize.BlockStartAscent() + bmBase.descent + dy;
  }

  
  if (IsToDraw(NOTATION_TOP) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_UPDIAGONALARROW) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX))
    mBoundingMetrics.ascent = aDesiredSize.BlockStartAscent();
  
  if (IsToDraw(NOTATION_BOTTOM) ||
      IsToDraw(NOTATION_RIGHT) ||
      IsToDraw(NOTATION_LEFT) ||
      IsToDraw(NOTATION_UPDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_UPDIAGONALARROW) ||
      IsToDraw(NOTATION_DOWNDIAGONALSTRIKE) ||
      IsToDraw(NOTATION_VERTICALSTRIKE) ||
      IsToDraw(NOTATION_CIRCLE) ||
      IsToDraw(NOTATION_ROUNDEDBOX))
    mBoundingMetrics.descent = aDesiredSize.Height() - aDesiredSize.BlockStartAscent();

  
  
  if (IsToDraw(NOTATION_PHASORANGLE))
    mBoundingMetrics.ascent = std::max(mBoundingMetrics.ascent, 2 * kPhasorangleWidth * mRuleThickness - mBoundingMetrics.descent);
  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;
  
  mReference.x = 0;
  mReference.y = aDesiredSize.BlockStartAscent();

  if (aPlaceOrigin) {
    
    
    if (IsToDraw(NOTATION_LONGDIV))
      mMathMLChar[mLongDivCharIndex].SetRect(nsRect(dx_left -
                                                    bmLongdivChar.width,
                                                    aDesiredSize.BlockStartAscent() -
                                                    longdivAscent,
                                                    bmLongdivChar.width,
                                                    bmLongdivChar.ascent +
                                                    bmLongdivChar.descent));

    if (IsToDraw(NOTATION_RADICAL)) {
      nscoord dx = (StyleVisibility()->mDirection ?
                    dx_left + bmBase.width : dx_left - bmRadicalChar.width);

      mMathMLChar[mRadicalCharIndex].SetRect(nsRect(dx,
                                                    aDesiredSize.BlockStartAscent() -
                                                    radicalAscent,
                                                    bmRadicalChar.width,
                                                    bmRadicalChar.ascent +
                                                    bmRadicalChar.descent));
    }

    mContentWidth = bmBase.width;

    
    
    PositionRowChildFrames(dx_left, aDesiredSize.BlockStartAscent());
  }

  return NS_OK;
}

nscoord
nsMathMLmencloseFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = nsMathMLContainerFrame::FixInterFrameSpacing(aDesiredSize);
  if (!gap)
    return 0;

  
  nsRect rect;
  for (uint32_t i = 0; i < mMathMLChar.Length(); i++) {
    mMathMLChar[i].GetRect(rect);
    rect.MoveBy(gap, 0);
    mMathMLChar[i].SetRect(rect);
  }

  return gap;
}

nsresult
nsMathMLmencloseFrame::AttributeChanged(int32_t         aNameSpaceID,
                                        nsIAtom*        aAttribute,
                                        int32_t         aModType)
{
  if (aAttribute == nsGkAtoms::notation_) {
    InitNotations();
  }

  return nsMathMLContainerFrame::
    AttributeChanged(aNameSpaceID, aAttribute, aModType);
}




nsStyleContext*
nsMathMLmencloseFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  int32_t len = mMathMLChar.Length();
  if (aIndex >= 0 && aIndex < len)
    return mMathMLChar[aIndex].GetStyleContext();
  else
    return nullptr;
}

void
nsMathMLmencloseFrame::SetAdditionalStyleContext(int32_t          aIndex, 
                                                 nsStyleContext*  aStyleContext)
{
  int32_t len = mMathMLChar.Length();
  if (aIndex >= 0 && aIndex < len)
    mMathMLChar[aIndex].SetStyleContext(aStyleContext);
}

class nsDisplayNotation : public nsDisplayItem
{
public:
  nsDisplayNotation(nsDisplayListBuilder* aBuilder,
                    nsIFrame* aFrame, const nsRect& aRect,
                    nscoord aThickness, nsMencloseNotation aType)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect), 
      mThickness(aThickness), mType(aType) {
    MOZ_COUNT_CTOR(nsDisplayNotation);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayNotation() {
    MOZ_COUNT_DTOR(nsDisplayNotation);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("MathMLMencloseNotation", TYPE_MATHML_MENCLOSE_NOTATION)

private:
  nsRect             mRect;
  nscoord            mThickness;
  nsMencloseNotation mType;
};

void nsDisplayNotation::Paint(nsDisplayListBuilder* aBuilder,
                              nsRenderingContext* aCtx)
{
  DrawTarget& aDrawTarget = *aCtx->GetDrawTarget();
  nsPresContext* presContext = mFrame->PresContext();

  Float strokeWidth = presContext->AppUnitsToGfxUnits(mThickness);

  Rect rect = NSRectToRect(mRect + ToReferenceFrame(),
                           presContext->AppUnitsPerDevPixel());
  rect.Deflate(strokeWidth / 2.f);

  ColorPattern color(ToDeviceColor(
                       mFrame->GetVisitedDependentColor(eCSSProperty_color)));

  StrokeOptions strokeOptions(strokeWidth);

  switch(mType)
  {
    case NOTATION_CIRCLE: {
      RefPtr<Path> ellipse =
        MakePathForEllipse(aDrawTarget, rect.Center(), rect.Size());
      aDrawTarget.Stroke(ellipse, color, strokeOptions);
      return;
    }
    case NOTATION_ROUNDEDBOX: {
      Float radius = 3 * strokeWidth;
      RectCornerRadii radii(radius, radius);
      RefPtr<Path> roundedRect =
        MakePathForRoundedRect(aDrawTarget, rect, radii, true);
      aDrawTarget.Stroke(roundedRect, color, strokeOptions);
      return;
    }
    case NOTATION_UPDIAGONALSTRIKE: {
      aDrawTarget.StrokeLine(rect.BottomLeft(), rect.TopRight(),
                             color, strokeOptions);
      return;
    }
    case NOTATION_DOWNDIAGONALSTRIKE: {
      aDrawTarget.StrokeLine(rect.TopLeft(), rect.BottomRight(),
                             color, strokeOptions);
      return;
    }
    case NOTATION_UPDIAGONALARROW: {
      
      
      Float W = rect.Width(); gfxFloat H = rect.Height();
      Float l = sqrt(W*W + H*H);
      Float f = Float(kArrowHeadSize) * strokeWidth / l;
      Float w = W * f; gfxFloat h = H * f;

      
      aDrawTarget.StrokeLine(rect.BottomLeft(),
                             rect.TopRight() + Point(-.7*w, .7*h),
                             color, strokeOptions);

      
      RefPtr<PathBuilder> builder = aDrawTarget.CreatePathBuilder();
      builder->MoveTo(rect.TopRight());
      builder->LineTo(rect.TopRight() + Point(-w -.4*h, std::max(-strokeWidth / 2.0, h - .4*w)));
      builder->LineTo(rect.TopRight() + Point(-.7*w, .7*h));
      builder->LineTo(rect.TopRight() + Point(std::min(strokeWidth / 2.0, -w + .4*h), h + .4*w));
      builder->Close();
      RefPtr<Path> path = builder->Finish();
      aDrawTarget.Fill(path, color);
      return;
    }
    case NOTATION_PHASORANGLE: {
      
      
      
      Float w = Float(kPhasorangleWidth) * strokeWidth;
      Float H = 2 * w;

      
      aDrawTarget.StrokeLine(rect.BottomLeft(),
                             rect.BottomLeft() + Point(w, -H),
                             color, strokeOptions);
      return;
    }
    default:
      NS_NOTREACHED("This notation can not be drawn using nsDisplayNotation");
  }
}

void
nsMathMLmencloseFrame::DisplayNotation(nsDisplayListBuilder* aBuilder,
                                       nsIFrame* aFrame, const nsRect& aRect,
                                       const nsDisplayListSet& aLists,
                                       nscoord aThickness,
                                       nsMencloseNotation aType)
{
  if (!aFrame->StyleVisibility()->IsVisible() || aRect.IsEmpty() ||
      aThickness <= 0)
    return;

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayNotation(aBuilder, aFrame, aRect, aThickness, aType));
}
