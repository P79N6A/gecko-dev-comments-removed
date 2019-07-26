




#include "nsINameSpaceManager.h"
#include "nsMathMLFrame.h"
#include "nsMathMLChar.h"
#include "nsCSSPseudoElements.h"


#include "nsStyleSet.h"
#include "nsAutoPtr.h"
#include "nsDisplayList.h"
#include "nsRenderingContext.h"
#include "nsContentUtils.h"
#include "nsIScriptError.h"

eMathMLFrameType
nsMathMLFrame::GetMathMLFrameType()
{
  
  if (mEmbellishData.coreFrame)
    return GetMathMLFrameTypeFor(mEmbellishData.coreFrame);

  
  if (mPresentationData.baseFrame)
    return GetMathMLFrameTypeFor(mPresentationData.baseFrame);

  
  return eMathMLFrameType_Ordinary;  
}



 void
nsMathMLFrame::FindAttrDisplaystyle(nsIContent*         aContent,
                                    nsPresentationData& aPresentationData)
{
  NS_ASSERTION(aContent->Tag() == nsGkAtoms::mstyle_ ||
               aContent->Tag() == nsGkAtoms::mtable_ ||
               aContent->Tag() == nsGkAtoms::math, "bad caller");
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_false, &nsGkAtoms::_true, nullptr};
  
  switch (aContent->FindAttrValueIn(kNameSpaceID_None,
    nsGkAtoms::displaystyle_, strings, eCaseMatters)) {
  case 0:
    aPresentationData.flags &= ~NS_MATHML_DISPLAYSTYLE;
    aPresentationData.flags |= NS_MATHML_EXPLICIT_DISPLAYSTYLE;
    break;
  case 1:
    aPresentationData.flags |= NS_MATHML_DISPLAYSTYLE;
    aPresentationData.flags |= NS_MATHML_EXPLICIT_DISPLAYSTYLE;
    break;
  }
  
}


 void
nsMathMLFrame::FindAttrDirectionality(nsIContent*         aContent,
                                      nsPresentationData& aPresentationData)
{
  NS_ASSERTION(aContent->Tag() == nsGkAtoms::math ||
               aContent->Tag() == nsGkAtoms::mrow_ ||
               aContent->Tag() == nsGkAtoms::mstyle_ ||
               aContent->Tag() == nsGkAtoms::mi_ ||
               aContent->Tag() == nsGkAtoms::mn_ ||
               aContent->Tag() == nsGkAtoms::mo_ ||
               aContent->Tag() == nsGkAtoms::mtext_ ||
               aContent->Tag() == nsGkAtoms::ms_, "bad caller");

  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::ltr, &nsGkAtoms::rtl, nullptr};

  
  switch (aContent->FindAttrValueIn(kNameSpaceID_None,
                                    nsGkAtoms::dir, strings, eCaseMatters))
    {
    case 0:
      aPresentationData.flags &= ~NS_MATHML_RTL;
      break;
    case 1:
      aPresentationData.flags |= NS_MATHML_RTL;
      break;
    }
  
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
  mPresentationData.mstyle = nullptr;

  
  nsPresentationData parentData;
  GetPresentationDataFrom(aParent, parentData);
  mPresentationData.mstyle = parentData.mstyle;
  if (NS_MATHML_IS_DISPLAYSTYLE(parentData.flags)) {
    mPresentationData.flags |= NS_MATHML_DISPLAYSTYLE;
  }
  if (NS_MATHML_IS_RTL(parentData.flags)) {
    mPresentationData.flags |= NS_MATHML_RTL;
  }

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
  mPresentationData.flags |= NS_MATHML_SHOW_BOUNDING_METRICS;
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLFrame::UpdatePresentationData(uint32_t        aFlagsValues,
                                      uint32_t        aWhichFlags)
{
  NS_ASSERTION(NS_MATHML_IS_DISPLAYSTYLE(aWhichFlags) ||
               NS_MATHML_IS_COMPRESSED(aWhichFlags),
               "aWhichFlags should only be displaystyle or compression flag"); 

  
  if (NS_MATHML_IS_DISPLAYSTYLE(aWhichFlags)) {
    
    if (NS_MATHML_IS_DISPLAYSTYLE(aFlagsValues)) {
      mPresentationData.flags |= NS_MATHML_DISPLAYSTYLE;
    }
    else {
      mPresentationData.flags &= ~NS_MATHML_DISPLAYSTYLE;
    }
  }
  if (NS_MATHML_IS_COMPRESSED(aWhichFlags)) {
    
    if (NS_MATHML_IS_COMPRESSED(aFlagsValues)) {
      
      mPresentationData.flags |= NS_MATHML_COMPRESSED;
    }
    
  }
  return NS_OK;
}





 void
nsMathMLFrame::ResolveMathMLCharStyle(nsPresContext*  aPresContext,
                                      nsIContent*      aContent,
                                      nsStyleContext*  aParentStyleContext,
                                      nsMathMLChar*    aMathMLChar,
                                      bool             aIsMutableChar)
{
  nsCSSPseudoElements::Type pseudoType = (aIsMutableChar) ?
    nsCSSPseudoElements::ePseudo_mozMathStretchy :
    nsCSSPseudoElements::ePseudo_mozMathAnonymous; 
  nsRefPtr<nsStyleContext> newStyleContext;
  newStyleContext = aPresContext->StyleSet()->
    ResolvePseudoElementStyle(aContent->AsElement(), pseudoType,
                              aParentStyleContext);

  if (newStyleContext)
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
  aPresentationData.mstyle = nullptr;

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

    if (content->Tag() == nsGkAtoms::math) {
      const nsStyleDisplay* display = frame->GetStyleDisplay();
      if (display->mDisplay == NS_STYLE_DISPLAY_BLOCK) {
        aPresentationData.flags |= NS_MATHML_DISPLAYSTYLE;
      }
      FindAttrDisplaystyle(content, aPresentationData);
      FindAttrDirectionality(content, aPresentationData);
      aPresentationData.mstyle = frame->GetFirstContinuation();
      break;
    }
    frame = frame->GetParent();
  }
  NS_WARN_IF_FALSE(frame && frame->GetContent(),
                   "bad MathML markup - could not find the top <math> element");
}


 bool
nsMathMLFrame::GetAttribute(nsIContent* aContent,
                            nsIFrame*   aMathMLmstyleFrame,
                            nsIAtom*    aAttributeAtom,
                            nsString&   aValue)
{
  
  if (aContent && aContent->GetAttr(kNameSpaceID_None, aAttributeAtom,
                                    aValue)) {
    return true;
  }

  
  if (!aMathMLmstyleFrame) {
    return false;
  }

  nsIFrame* mstyleParent = aMathMLmstyleFrame->GetParent();

  nsPresentationData mstyleParentData;
  mstyleParentData.mstyle = nullptr;

  if (mstyleParent) {
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(mstyleParent);
    if (mathMLFrame) {
      mathMLFrame->GetPresentationData(mstyleParentData);
    }
  }

  
  return GetAttribute(aMathMLmstyleFrame->GetContent(),
                      mstyleParentData.mstyle, aAttributeAtom, aValue);
}

 void
nsMathMLFrame::GetRuleThickness(nsRenderingContext& aRenderingContext,
                                nsFontMetrics*      aFontMetrics,
                                nscoord&             aRuleThickness)
{
  
  
  NS_ASSERTION(aRenderingContext.FontMetrics()->Font().
               Equals(aFontMetrics->Font()),
               "unexpected state");

  nscoord xHeight = aFontMetrics->XHeight();
  PRUnichar overBar = 0x00AF;
  nsBoundingMetrics bm = aRenderingContext.GetBoundingMetrics(&overBar, 1);
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
  
  
  NS_ASSERTION(aRenderingContext.FontMetrics()->Font().
               Equals(aFontMetrics->Font()),
               "unexpected state");

  nscoord xHeight = aFontMetrics->XHeight();
  PRUnichar minus = 0x2212; 
  nsBoundingMetrics bm = aRenderingContext.GetBoundingMetrics(&minus, 1);
  aAxisHeight = bm.ascent - (bm.ascent + bm.descent)/2;
  if (aAxisHeight <= 0 || aAxisHeight >= xHeight) {
    
    GetAxisHeight(aFontMetrics, aAxisHeight);
  }
}

 nscoord
nsMathMLFrame::CalcLength(nsPresContext*   aPresContext,
                          nsStyleContext*   aStyleContext,
                          const nsCSSValue& aCSSValue)
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
    const nsStyleFont* font = aStyleContext->GetStyleFont();
    return NSToCoordRound(aCSSValue.GetFloatValue() * (float)font->mFont.size);
  }
  else if (eCSSUnit_XHeight == unit) {
    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForStyleContext(aStyleContext,
                                                 getter_AddRefs(fm));
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
                                 nsStyleContext*   aStyleContext)
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
  
  
  *aLengthValue = CalcLength(aPresContext, aStyleContext, cssValue);
}






static const int32_t kMathMLversion1 = 1;
static const int32_t kMathMLversion2 = 2;

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
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLBoundingMetrics", TYPE_MATHML_BOUNDING_METRICS)
private:
  nsRect    mRect;
};

void nsDisplayMathMLBoundingMetrics::Paint(nsDisplayListBuilder* aBuilder,
                                           nsRenderingContext* aCtx)
{
  aCtx->SetColor(NS_RGB(0,0,255));
  aCtx->DrawRect(mRect + ToReferenceFrame());
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
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLBar", TYPE_MATHML_BAR)
private:
  nsRect    mRect;
};

void nsDisplayMathMLBar::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx)
{
  
  aCtx->SetColor(mFrame->GetVisitedDependentColor(eCSSProperty_color));
  aCtx->FillRect(mRect + ToReferenceFrame());
}

void
nsMathMLFrame::DisplayBar(nsDisplayListBuilder* aBuilder,
                          nsIFrame* aFrame, const nsRect& aRect,
                          const nsDisplayListSet& aLists) {
  if (!aFrame->GetStyleVisibility()->IsVisible() || aRect.IsEmpty())
    return;

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayMathMLBar(aBuilder, aFrame, aRect));
}
