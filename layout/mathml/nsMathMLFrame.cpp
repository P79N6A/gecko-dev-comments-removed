




#include "nsMathMLFrame.h"

#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "nsLayoutUtils.h"
#include "nsNameSpaceManager.h"
#include "nsMathMLChar.h"
#include "nsCSSPseudoElements.h"
#include "nsMathMLElement.h"


#include "nsStyleSet.h"
#include "nsAutoPtr.h"
#include "nsDisplayList.h"
#include "nsRenderingContext.h"

using namespace mozilla;
using namespace mozilla::gfx;

eMathMLFrameType
nsMathMLFrame::GetMathMLFrameType()
{
  
  if (mEmbellishData.coreFrame)
    return GetMathMLFrameTypeFor(mEmbellishData.coreFrame);

  
  if (mPresentationData.baseFrame)
    return GetMathMLFrameTypeFor(mPresentationData.baseFrame);

  
  return eMathMLFrameType_Ordinary;  
}

NS_IMETHODIMP
nsMathMLFrame::InheritAutomaticData(nsIFrame* aParent) 
{
  mEmbellishData.flags = 0;
  mEmbellishData.coreFrame = nullptr;
  mEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
  mEmbellishData.leadingSpace = 0;
  mEmbellishData.trailingSpace = 0;

  mPresentationData.flags = 0;
  mPresentationData.baseFrame = nullptr;

  
  nsPresentationData parentData;
  GetPresentationDataFrom(aParent, parentData);

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
  mPresentationData.flags |= NS_MATHML_SHOW_BOUNDING_METRICS;
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLFrame::UpdatePresentationData(uint32_t        aFlagsValues,
                                      uint32_t        aWhichFlags)
{
  NS_ASSERTION(NS_MATHML_IS_COMPRESSED(aWhichFlags) ||
               NS_MATHML_IS_DTLS_SET(aWhichFlags),
               "aWhichFlags should only be compression or dtls flag");

  if (NS_MATHML_IS_COMPRESSED(aWhichFlags)) {
    
    if (NS_MATHML_IS_COMPRESSED(aFlagsValues)) {
      
      mPresentationData.flags |= NS_MATHML_COMPRESSED;
    }
    
  }
  
  
  if (NS_MATHML_IS_DTLS_SET(aWhichFlags)) {
    if (NS_MATHML_IS_DTLS_SET(aFlagsValues)) {
      mPresentationData.flags |= NS_MATHML_DTLS;
    } else {
      mPresentationData.flags &= ~NS_MATHML_DTLS;
    }
  }
  return NS_OK;
}





 void
nsMathMLFrame::ResolveMathMLCharStyle(nsPresContext*  aPresContext,
                                      nsIContent*      aContent,
                                      nsStyleContext*  aParentStyleContext,
                                      nsMathMLChar*    aMathMLChar)
{
  nsCSSPseudoElements::Type pseudoType =
    nsCSSPseudoElements::ePseudo_mozMathAnonymous; 
  nsRefPtr<nsStyleContext> newStyleContext;
  newStyleContext = aPresContext->StyleSet()->
    ResolvePseudoElementStyle(aContent->AsElement(), pseudoType,
                              aParentStyleContext, nullptr);

  aMathMLChar->SetStyleContext(newStyleContext);
}

 void
nsMathMLFrame::GetEmbellishDataFrom(nsIFrame*        aFrame,
                                    nsEmbellishData& aEmbellishData)
{
  
  aEmbellishData.flags = 0;
  aEmbellishData.coreFrame = nullptr;
  aEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
  aEmbellishData.leadingSpace = 0;
  aEmbellishData.trailingSpace = 0;

  if (aFrame && aFrame->IsFrameOfType(nsIFrame::eMathML)) {
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(aFrame);
    if (mathMLFrame) {
      mathMLFrame->GetEmbellishData(aEmbellishData);
    }
  }
}



 void
nsMathMLFrame::GetPresentationDataFrom(nsIFrame*           aFrame,
                                       nsPresentationData& aPresentationData,
                                       bool                aClimbTree)
{
  
  aPresentationData.flags = 0;
  aPresentationData.baseFrame = nullptr;

  nsIFrame* frame = aFrame;
  while (frame) {
    if (frame->IsFrameOfType(nsIFrame::eMathML)) {
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(frame);
      if (mathMLFrame) {
        mathMLFrame->GetPresentationData(aPresentationData);
        break;
      }
    }
    
    if (!aClimbTree) {
      break;
    }
    
    nsIContent* content = frame->GetContent();
    NS_ASSERTION(content || !frame->GetParent(), 
                 "dangling frame without a content node"); 
    if (!content)
      break;

    if (content->IsMathMLElement(nsGkAtoms::math)) {
      break;
    }
    frame = frame->GetParent();
  }
  NS_WARN_IF_FALSE(frame && frame->GetContent(),
                   "bad MathML markup - could not find the top <math> element");
}

 void
nsMathMLFrame::GetRuleThickness(nsRenderingContext& aRenderingContext,
                                nsFontMetrics*      aFontMetrics,
                                nscoord&             aRuleThickness)
{
  nscoord xHeight = aFontMetrics->XHeight();
  char16_t overBar = 0x00AF;
  nsBoundingMetrics bm =
    nsLayoutUtils::AppUnitBoundsOfString(&overBar, 1, *aFontMetrics,
                                         aRenderingContext);
  aRuleThickness = bm.ascent + bm.descent;
  if (aRuleThickness <= 0 || aRuleThickness >= xHeight) {
    
    GetRuleThickness(aFontMetrics, aRuleThickness);
  }
}

 void
nsMathMLFrame::GetAxisHeight(nsRenderingContext& aRenderingContext,
                             nsFontMetrics*      aFontMetrics,
                             nscoord&             aAxisHeight)
{
  gfxFont* mathFont = aFontMetrics->GetThebesFontGroup()->GetFirstMathFont();
  if (mathFont) {
    aAxisHeight =
      mathFont->GetMathConstant(gfxFontEntry::AxisHeight,
                                aFontMetrics->AppUnitsPerDevPixel());
    return;
  }

  nscoord xHeight = aFontMetrics->XHeight();
  char16_t minus = 0x2212; 
  nsBoundingMetrics bm =
    nsLayoutUtils::AppUnitBoundsOfString(&minus, 1, *aFontMetrics,
                                         aRenderingContext);
  aAxisHeight = bm.ascent - (bm.ascent + bm.descent)/2;
  if (aAxisHeight <= 0 || aAxisHeight >= xHeight) {
    
    GetAxisHeight(aFontMetrics, aAxisHeight);
  }
}

 nscoord
nsMathMLFrame::CalcLength(nsPresContext*   aPresContext,
                          nsStyleContext*   aStyleContext,
                          const nsCSSValue& aCSSValue,
                          float             aFontSizeInflation)
{
  NS_ASSERTION(aCSSValue.IsLengthUnit(), "not a length unit");

  if (aCSSValue.IsFixedLengthUnit()) {
    return aCSSValue.GetFixedLength(aPresContext);
  }
  if (aCSSValue.IsPixelLengthUnit()) {
    return aCSSValue.GetPixelLength();
  }

  nsCSSUnit unit = aCSSValue.GetUnit();

  if (eCSSUnit_EM == unit) {
    const nsStyleFont* font = aStyleContext->StyleFont();
    return NSToCoordRound(aCSSValue.GetFloatValue() * (float)font->mFont.size);
  }
  else if (eCSSUnit_XHeight == unit) {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForStyleContext(aStyleContext,
                                                 getter_AddRefs(fm),
                                                 aFontSizeInflation);
    nscoord xHeight = fm->XHeight();
    return NSToCoordRound(aCSSValue.GetFloatValue() * (float)xHeight);
  }

  
  NS_ERROR("Unsupported unit");
  return 0;
}

 void
nsMathMLFrame::ParseNumericValue(const nsString&   aString,
                                 nscoord*          aLengthValue,
                                 uint32_t          aFlags,
                                 nsPresContext*    aPresContext,
                                 nsStyleContext*   aStyleContext,
                                 float             aFontSizeInflation)
{
  nsCSSValue cssValue;

  if (!nsMathMLElement::ParseNumericValue(aString, cssValue, aFlags,
                                          aPresContext->Document())) {
    
    
    return;
  }

  nsCSSUnit unit = cssValue.GetUnit();

  if (unit == eCSSUnit_Percent || unit == eCSSUnit_Number) {
    
    *aLengthValue = NSToCoordRound(*aLengthValue * (unit == eCSSUnit_Percent ?
                                                    cssValue.GetPercentValue() :
                                                    cssValue.GetFloatValue()));
    return;
  }
  
  
  *aLengthValue = CalcLength(aPresContext, aStyleContext, cssValue,
                             aFontSizeInflation);
}






struct
nsCSSMapping {
  int32_t        compatibility;
  const nsIAtom* attrAtom;
  const char*    cssProperty;
};

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
class nsDisplayMathMLBoundingMetrics : public nsDisplayItem {
public:
  nsDisplayMathMLBoundingMetrics(nsDisplayListBuilder* aBuilder,
                                 nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLBoundingMetrics);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLBoundingMetrics() {
    MOZ_COUNT_DTOR(nsDisplayMathMLBoundingMetrics);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("MathMLBoundingMetrics", TYPE_MATHML_BOUNDING_METRICS)
private:
  nsRect    mRect;
};

void nsDisplayMathMLBoundingMetrics::Paint(nsDisplayListBuilder* aBuilder,
                                           nsRenderingContext* aCtx)
{
  DrawTarget* drawTarget = aRenderingContext->GetDrawTarget();
  Rect r = NSRectToRect(mRect + ToReferenceFrame(),
                        mFrame->PresContext()->AppUnitsPerDevPixel());
  ColorPattern blue(ToDeviceColor(Color(0.f, 0.f, 1.f, 1.f)));
  drawTarget->StrokeRect(r, blue);
}

nsresult
nsMathMLFrame::DisplayBoundingMetrics(nsDisplayListBuilder* aBuilder,
                                      nsIFrame* aFrame, const nsPoint& aPt,
                                      const nsBoundingMetrics& aMetrics,
                                      const nsDisplayListSet& aLists) {
  if (!NS_MATHML_PAINT_BOUNDING_METRICS(mPresentationData.flags))
    return NS_OK;
    
  nscoord x = aPt.x + aMetrics.leftBearing;
  nscoord y = aPt.y - aMetrics.ascent;
  nscoord w = aMetrics.rightBearing - aMetrics.leftBearing;
  nscoord h = aMetrics.ascent + aMetrics.descent;

  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayMathMLBoundingMetrics(aBuilder, this, nsRect(x,y,w,h)));
}
#endif

class nsDisplayMathMLBar : public nsDisplayItem {
public:
  nsDisplayMathMLBar(nsDisplayListBuilder* aBuilder,
                     nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aBuilder, aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLBar);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLBar() {
    MOZ_COUNT_DTOR(nsDisplayMathMLBar);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("MathMLBar", TYPE_MATHML_BAR)
private:
  nsRect    mRect;
};

void nsDisplayMathMLBar::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx)
{
  
  DrawTarget* drawTarget = aCtx->GetDrawTarget();
  Rect rect = NSRectToSnappedRect(mRect + ToReferenceFrame(),
                                  mFrame->PresContext()->AppUnitsPerDevPixel(),
                                  *drawTarget);
  ColorPattern color(ToDeviceColor(
                       mFrame->GetVisitedDependentColor(eCSSProperty_color)));
  drawTarget->FillRect(rect, color);
}

void
nsMathMLFrame::DisplayBar(nsDisplayListBuilder* aBuilder,
                          nsIFrame* aFrame, const nsRect& aRect,
                          const nsDisplayListSet& aLists) {
  if (!aFrame->StyleVisibility()->IsVisible() || aRect.IsEmpty())
    return;

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayMathMLBar(aBuilder, aFrame, aRect));
}

void
nsMathMLFrame::GetRadicalParameters(nsFontMetrics* aFontMetrics,
                                    bool aDisplayStyle,
                                    nscoord& aRadicalRuleThickness,
                                    nscoord& aRadicalExtraAscender,
                                    nscoord& aRadicalVerticalGap)
{
  nscoord oneDevPixel = aFontMetrics->AppUnitsPerDevPixel();
  gfxFont* mathFont = aFontMetrics->GetThebesFontGroup()->GetFirstMathFont();

  
  if (mathFont) {
    aRadicalRuleThickness =
      mathFont->GetMathConstant(gfxFontEntry::RadicalRuleThickness,
                                oneDevPixel);
  } else {
    GetRuleThickness(aFontMetrics, aRadicalRuleThickness);
  }

  
  if (mathFont) {
    aRadicalExtraAscender =
      mathFont->GetMathConstant(gfxFontEntry::RadicalExtraAscender,
                                oneDevPixel);
  } else {
    
    
    nscoord em;
    GetEmHeight(aFontMetrics, em);
    aRadicalExtraAscender = nscoord(0.2f * em);
  }

  
  if (mathFont) {
    aRadicalVerticalGap =
      mathFont->GetMathConstant(aDisplayStyle ?
                                gfxFontEntry::RadicalDisplayStyleVerticalGap :
                                gfxFontEntry::RadicalVerticalGap,
                                oneDevPixel);
  } else {
    
    aRadicalVerticalGap = aRadicalRuleThickness +
      (aDisplayStyle ? aFontMetrics->XHeight() : aRadicalRuleThickness) / 4;
  }
}
