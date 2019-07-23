





































#include "nsINameSpaceManager.h"
#include "nsMathMLFrame.h"
#include "nsMathMLChar.h"
#include "nsCSSAnonBoxes.h"


#include "nsIDocument.h"
#include "nsStyleSet.h"
#include "nsIStyleSheet.h"
#include "nsICSSStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSRule.h"
#include "nsICSSStyleRule.h"
#include "nsStyleChangeList.h"
#include "nsFrameManager.h"
#include "nsNetUtil.h"
#include "nsIURI.h"
#include "nsContentCID.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsStyleUtil.h"
#include "nsDisplayList.h"
#include "nsAttrName.h"

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
               aContent->Tag() == nsGkAtoms::mtable_, "bad caller");
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_false, &nsGkAtoms::_true, nsnull};
  
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

NS_IMETHODIMP
nsMathMLFrame::InheritAutomaticData(nsIFrame* aParent) 
{
  mEmbellishData.flags = 0;
  mEmbellishData.coreFrame = nsnull;
  mEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
  mEmbellishData.leftSpace = 0;
  mEmbellishData.rightSpace = 0;

  mPresentationData.flags = 0;
  mPresentationData.baseFrame = nsnull;
  mPresentationData.mstyle = nsnull;

  
  nsPresentationData parentData;
  GetPresentationDataFrom(aParent, parentData);
  mPresentationData.mstyle = parentData.mstyle;
  if (NS_MATHML_IS_DISPLAYSTYLE(parentData.flags)) {
    mPresentationData.flags |= NS_MATHML_DISPLAYSTYLE;
  }

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
  mPresentationData.flags |= NS_MATHML_SHOW_BOUNDING_METRICS;
#endif

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLFrame::UpdatePresentationData(PRUint32        aFlagsValues,
                                      PRUint32        aWhichFlags)
{
  
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
                                      PRBool           aIsMutableChar)
{
  nsIAtom* pseudoStyle = (aIsMutableChar) ?
    nsCSSPseudoElements::mozMathStretchy :
    nsCSSPseudoElements::mozMathAnonymous; 
  nsRefPtr<nsStyleContext> newStyleContext;
  newStyleContext = aPresContext->StyleSet()->
    ResolvePseudoStyleFor(aContent, pseudoStyle, aParentStyleContext);

  if (newStyleContext)
    aMathMLChar->SetStyleContext(newStyleContext);
}

 void
nsMathMLFrame::GetEmbellishDataFrom(nsIFrame*        aFrame,
                                    nsEmbellishData& aEmbellishData)
{
  
  aEmbellishData.flags = 0;
  aEmbellishData.coreFrame = nsnull;
  aEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
  aEmbellishData.leftSpace = 0;
  aEmbellishData.rightSpace = 0;

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
                                       PRBool              aClimbTree)
{
  
  aPresentationData.flags = 0;
  aPresentationData.baseFrame = nsnull;
  aPresentationData.mstyle = nsnull;

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
      break;
    }
    frame = frame->GetParent();
  }
  NS_WARN_IF_FALSE(frame && frame->GetContent(),
                   "bad MathML markup - could not find the top <math> element");
}


 PRBool
nsMathMLFrame::GetAttribute(nsIContent* aContent,
                            nsIFrame*   aMathMLmstyleFrame,
                            nsIAtom*    aAttributeAtom,
                            nsString&   aValue)
{
  
  if (aContent && aContent->GetAttr(kNameSpaceID_None, aAttributeAtom,
                                    aValue)) {
    return PR_TRUE;
  }

  
  if (!aMathMLmstyleFrame) {
    return PR_FALSE;
  }

  nsIFrame* mstyleParent = aMathMLmstyleFrame->GetParent();

  nsPresentationData mstyleParentData;
  mstyleParentData.mstyle = nsnull;

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
nsMathMLFrame::GetRuleThickness(nsIRenderingContext& aRenderingContext,
                                nsIFontMetrics*      aFontMetrics,
                                nscoord&             aRuleThickness)
{
  
  
#ifdef NS_DEBUG
  nsCOMPtr<nsIFontMetrics> currFontMetrics;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(currFontMetrics));
  NS_ASSERTION(currFontMetrics->Font().Equals(aFontMetrics->Font()),
      "unexpected state");
#endif
  nscoord xHeight;
  aFontMetrics->GetXHeight(xHeight);
  PRUnichar overBar = 0x00AF;
  nsBoundingMetrics bm;
  nsresult rv = aRenderingContext.GetBoundingMetrics(&overBar, PRUint32(1), bm);
  if (NS_SUCCEEDED(rv)) {
    aRuleThickness = bm.ascent + bm.descent;
  }
  if (NS_FAILED(rv) || aRuleThickness <= 0 || aRuleThickness >= xHeight) {
    
    GetRuleThickness(aFontMetrics, aRuleThickness);
  }

#if 0
  nscoord oldRuleThickness;
  GetRuleThickness(aFontMetrics, oldRuleThickness);

  PRUnichar sqrt = 0xE063; 
  rv = aRenderingContext.GetBoundingMetrics(&sqrt, PRUint32(1), bm);
  nscoord sqrtrule = bm.ascent; 

  printf("xheight:%4d rule:%4d oldrule:%4d  sqrtrule:%4d\n",
          xHeight, aRuleThickness, oldRuleThickness, sqrtrule);
#endif
}

 void
nsMathMLFrame::GetAxisHeight(nsIRenderingContext& aRenderingContext,
                             nsIFontMetrics*      aFontMetrics,
                             nscoord&             aAxisHeight)
{
  
  
#ifdef NS_DEBUG
  nsCOMPtr<nsIFontMetrics> currFontMetrics;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(currFontMetrics));
  NS_ASSERTION(currFontMetrics->Font().Equals(aFontMetrics->Font()),
	"unexpected state");
#endif
  nscoord xHeight;
  aFontMetrics->GetXHeight(xHeight);
  PRUnichar minus = 0x2212; 
  nsBoundingMetrics bm;
  nsresult rv = aRenderingContext.GetBoundingMetrics(&minus, PRUint32(1), bm);
  if (NS_SUCCEEDED(rv)) {
    aAxisHeight = bm.ascent - (bm.ascent + bm.descent)/2;
  }
  if (NS_FAILED(rv) || aAxisHeight <= 0 || aAxisHeight >= xHeight) {
    
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
    return aPresContext->TwipsToAppUnits(aCSSValue.GetLengthTwips());
  }

  nsCSSUnit unit = aCSSValue.GetUnit();

  if (eCSSUnit_Pixel == unit) {
    return nsPresContext::CSSPixelsToAppUnits(aCSSValue.GetFloatValue());
  }
  else if (eCSSUnit_EM == unit) {
    const nsStyleFont* font = aStyleContext->GetStyleFont();
    return NSToCoordRound(aCSSValue.GetFloatValue() * (float)font->mFont.size);
  }
  else if (eCSSUnit_XHeight == unit) {
    nscoord xHeight;
    const nsStyleFont* font = aStyleContext->GetStyleFont();
    nsCOMPtr<nsIFontMetrics> fm = aPresContext->GetMetricsFor(font->mFont);
    fm->GetXHeight(xHeight);
    return NSToCoordRound(aCSSValue.GetFloatValue() * (float)xHeight);
  }

  return 0;
}

 PRBool
nsMathMLFrame::ParseNamedSpaceValue(nsIFrame*   aMathMLmstyleFrame,
                                    nsString&   aString,
                                    nsCSSValue& aCSSValue)
{
  aCSSValue.Reset();
  aString.CompressWhitespace(); 
  if (!aString.Length()) return PR_FALSE;

  
  PRInt32 i = 0;
  nsIAtom* namedspaceAtom = nsnull;
  if (aString.EqualsLiteral("veryverythinmathspace")) {
    i = 1;
    namedspaceAtom = nsGkAtoms::veryverythinmathspace_;
  }
  else if (aString.EqualsLiteral("verythinmathspace")) {
    i = 2;
    namedspaceAtom = nsGkAtoms::verythinmathspace_;
  }
  else if (aString.EqualsLiteral("thinmathspace")) {
    i = 3;
    namedspaceAtom = nsGkAtoms::thinmathspace_;
  }
  else if (aString.EqualsLiteral("mediummathspace")) {
    i = 4;
    namedspaceAtom = nsGkAtoms::mediummathspace_;
  }
  else if (aString.EqualsLiteral("thickmathspace")) {
    i = 5;
    namedspaceAtom = nsGkAtoms::thickmathspace_;
  }
  else if (aString.EqualsLiteral("verythickmathspace")) {
    i = 6;
    namedspaceAtom = nsGkAtoms::verythickmathspace_;
  }
  else if (aString.EqualsLiteral("veryverythickmathspace")) {
    i = 7;
    namedspaceAtom = nsGkAtoms::veryverythickmathspace_;
  }

  if (0 != i) {
    if (aMathMLmstyleFrame) {
      
      
      nsAutoString value;
      GetAttribute(nsnull, aMathMLmstyleFrame, namedspaceAtom, value);
      if (!value.IsEmpty()) {
        if (ParseNumericValue(value, aCSSValue) &&
            aCSSValue.IsLengthUnit()) {
          return PR_TRUE;
        }
      }
    }

    
    aCSSValue.SetFloatValue(float(i)/float(18), eCSSUnit_EM);
    return PR_TRUE;
  }

  return PR_FALSE;
}






static const PRInt32 kMathMLversion1 = 1;
static const PRInt32 kMathMLversion2 = 2;

struct
nsCSSMapping {
  PRInt32        compatibility;
  const nsIAtom* attrAtom;
  const char*    cssProperty;
};

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
class nsDisplayMathMLBoundingMetrics : public nsDisplayItem {
public:
  nsDisplayMathMLBoundingMetrics(nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLBoundingMetrics);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLBoundingMetrics() {
    MOZ_COUNT_DTOR(nsDisplayMathMLBoundingMetrics);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLBoundingMetrics")
private:
  nsRect    mRect;
};

void nsDisplayMathMLBoundingMetrics::Paint(nsDisplayListBuilder* aBuilder,
                                           nsIRenderingContext* aCtx)
{
  aCtx->SetColor(NS_RGB(0,0,255));
  aCtx->DrawRect(mRect + aBuilder->ToReferenceFrame(mFrame));
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
      nsDisplayMathMLBoundingMetrics(this, nsRect(x,y,w,h)));
}
#endif

class nsDisplayMathMLBar : public nsDisplayItem {
public:
  nsDisplayMathMLBar(nsIFrame* aFrame, const nsRect& aRect)
    : nsDisplayItem(aFrame), mRect(aRect) {
    MOZ_COUNT_CTOR(nsDisplayMathMLBar);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLBar() {
    MOZ_COUNT_DTOR(nsDisplayMathMLBar);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("MathMLBar")
private:
  nsRect    mRect;
};

void nsDisplayMathMLBar::Paint(nsDisplayListBuilder* aBuilder,
                               nsIRenderingContext* aCtx)
{
  
  aCtx->SetColor(mFrame->GetStyleColor()->mColor);
  aCtx->FillRect(mRect + aBuilder->ToReferenceFrame(mFrame));
}

nsresult
nsMathMLFrame::DisplayBar(nsDisplayListBuilder* aBuilder,
                          nsIFrame* aFrame, const nsRect& aRect,
                          const nsDisplayListSet& aLists) {
  if (!aFrame->GetStyleVisibility()->IsVisible() || aRect.IsEmpty())
    return NS_OK;

  return aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayMathMLBar(aFrame, aRect));
}
