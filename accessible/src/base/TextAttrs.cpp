




#include "TextAttrs.h"

#include "HyperTextAccessibleWrap.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "StyleInfo.h"

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "nsFontMetrics.h"
#include "nsLayoutUtils.h"

using namespace mozilla;
using namespace mozilla::a11y;





void
TextAttrsMgr::GetAttributes(nsIPersistentProperties* aAttributes,
                            PRInt32* aStartHTOffset,
                            PRInt32* aEndHTOffset)
{
  
  
  
  
  
  
  NS_PRECONDITION(mHyperTextAcc &&
                  ((mOffsetAcc && mOffsetAccIdx != -1 &&
                    aStartHTOffset && aEndHTOffset) ||
                  (!mOffsetAcc && mOffsetAccIdx == -1 &&
                    !aStartHTOffset && !aEndHTOffset &&
                   mIncludeDefAttrs && aAttributes)),
                  "Wrong usage of TextAttrsMgr!");

  
  if (mOffsetAcc && nsAccUtils::IsEmbeddedObject(mOffsetAcc)) {
    for (PRInt32 childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
      Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aStartHTOffset)--;
    }

    PRUint32 childCount = mHyperTextAcc->ChildCount();
    for (PRUint32 childIdx = mOffsetAccIdx + 1; childIdx < childCount;
         childIdx++) {
      Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aEndHTOffset)++;
    }

    return;
  }

  
  
  nsIContent *hyperTextElm = mHyperTextAcc->GetContent();
  nsIFrame *rootFrame = mHyperTextAcc->GetFrame();
  NS_ASSERTION(rootFrame, "No frame for accessible!");
  if (!rootFrame)
    return;

  nsIContent *offsetNode = nsnull, *offsetElm = nsnull;
  nsIFrame *frame = nsnull;
  if (mOffsetAcc) {
    offsetNode = mOffsetAcc->GetContent();
    offsetElm = nsCoreUtils::GetDOMElementFor(offsetNode);
    frame = offsetElm->GetPrimaryFrame();
  }

  
  LangTextAttr langTextAttr(mHyperTextAcc, hyperTextElm, offsetNode);

  
  BGColorTextAttr bgColorTextAttr(rootFrame, frame);

  
  ColorTextAttr colorTextAttr(rootFrame, frame);

  
  FontFamilyTextAttr fontFamilyTextAttr(rootFrame, frame);

  
  FontSizeTextAttr fontSizeTextAttr(rootFrame, frame);

  
  FontStyleTextAttr fontStyleTextAttr(rootFrame, frame);

  
  FontWeightTextAttr fontWeightTextAttr(rootFrame, frame);

  
  TextDecorTextAttr textDecorTextAttr(rootFrame, frame);

  
  TextPosTextAttr textPosTextAttr(rootFrame, frame);

  TextAttr* attrArray[] =
  {
    &langTextAttr,
    &bgColorTextAttr,
    &colorTextAttr,
    &fontFamilyTextAttr,
    &fontSizeTextAttr,
    &fontStyleTextAttr,
    &fontWeightTextAttr,
    &textDecorTextAttr,
    &textPosTextAttr
  };

  
  if (aAttributes) {
    for (PRUint32 idx = 0; idx < ArrayLength(attrArray); idx++)
      attrArray[idx]->Expose(aAttributes, mIncludeDefAttrs);
  }

  
  if (mOffsetAcc)
    GetRange(attrArray, ArrayLength(attrArray), aStartHTOffset, aEndHTOffset);
}

void
TextAttrsMgr::GetRange(TextAttr* aAttrArray[], PRUint32 aAttrArrayLen,
                       PRInt32* aStartHTOffset, PRInt32* aEndHTOffset)
{
  
  for (PRInt32 childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
    Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);

    
    
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    nsIContent* currElm = nsCoreUtils::GetDOMElementFor(currAcc->GetContent());
    if (!currElm)
      return;

    bool offsetFound = false;
    for (PRUint32 attrIdx = 0; attrIdx < aAttrArrayLen; attrIdx++) {
      TextAttr* textAttr = aAttrArray[attrIdx];
      if (!textAttr->Equal(currElm)) {
        offsetFound = true;
        break;
      }
    }

    if (offsetFound)
      break;

    *(aStartHTOffset) -= nsAccUtils::TextLength(currAcc);
  }

  
  PRUint32 childLen = mHyperTextAcc->ChildCount();
  for (PRUint32 childIdx = mOffsetAccIdx + 1; childIdx < childLen; childIdx++) {
    Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    nsIContent* currElm = nsCoreUtils::GetDOMElementFor(currAcc->GetContent());
    if (!currElm)
      return;

    bool offsetFound = false;
    for (PRUint32 attrIdx = 0; attrIdx < aAttrArrayLen; attrIdx++) {
      TextAttr* textAttr = aAttrArray[attrIdx];

      
      
      if (!textAttr->Equal(currElm)) {
        offsetFound = true;
        break;
      }
    }

    if (offsetFound)
      break;

    (*aEndHTOffset) += nsAccUtils::TextLength(currAcc);
  }
}






TextAttrsMgr::LangTextAttr::
  LangTextAttr(HyperTextAccessible* aRoot,
               nsIContent* aRootElm, nsIContent* aElm) :
  TTextAttr<nsString>(!aElm), mRootContent(aRootElm)
{
  aRoot->Language(mRootNativeValue);
  mIsRootDefined =  !mRootNativeValue.IsEmpty();

  if (aElm)
    mIsDefined = GetLang(aElm, mNativeValue);
}

bool
TextAttrsMgr::LangTextAttr::
  GetValueFor(nsIContent* aElm, nsString* aValue)
{
  return GetLang(aElm, *aValue);
}

void
TextAttrsMgr::LangTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nsString& aValue)
{
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::language, aValue);
}

bool
TextAttrsMgr::LangTextAttr::
  GetLang(nsIContent* aElm, nsAString& aLang)
{
  nsCoreUtils::GetLanguageFor(aElm, mRootContent, aLang);
  return !aLang.IsEmpty();
}






TextAttrsMgr::BGColorTextAttr::
  BGColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscolor>(!aFrame), mRootFrame(aRootFrame)
{
  mIsRootDefined = GetColor(mRootFrame, &mRootNativeValue);
  if (aFrame)
    mIsDefined = GetColor(aFrame, &mNativeValue);
}

bool
TextAttrsMgr::BGColorTextAttr::
  GetValueFor(nsIContent* aElm, nscolor* aValue)
{
  nsIFrame* frame = aElm->GetPrimaryFrame();
  return frame ? GetColor(frame, aValue) : false;
}

void
TextAttrsMgr::BGColorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscolor& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatColor(aValue, formattedValue);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::backgroundColor,
                         formattedValue);
}

bool
TextAttrsMgr::BGColorTextAttr::
  GetColor(nsIFrame* aFrame, nscolor* aColor)
{
  const nsStyleBackground* styleBackground = aFrame->GetStyleBackground();

  if (NS_GET_A(styleBackground->mBackgroundColor) > 0) {
    *aColor = styleBackground->mBackgroundColor;
    return true;
  }

  nsIFrame *parentFrame = aFrame->GetParent();
  if (!parentFrame) {
    *aColor = aFrame->PresContext()->DefaultBackgroundColor();
    return true;
  }

  
  
  
  if (parentFrame == mRootFrame)
    return false;

  return GetColor(parentFrame, aColor);
}






TextAttrsMgr::ColorTextAttr::
  ColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscolor>(!aFrame)
{
  mRootNativeValue = aRootFrame->GetStyleColor()->mColor;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->GetStyleColor()->mColor;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::ColorTextAttr::
  GetValueFor(nsIContent* aElm, nscolor* aValue)
{
  nsIFrame* frame = aElm->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->GetStyleColor()->mColor;
    return true;
  }

  return false;
}

void
TextAttrsMgr::ColorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscolor& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatColor(aValue, formattedValue);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::color, formattedValue);
}






TextAttrsMgr::FontFamilyTextAttr::
  FontFamilyTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nsString>(!aFrame)
{
  mIsRootDefined = GetFontFamily(aRootFrame, mRootNativeValue);

  if (aFrame)
    mIsDefined = GetFontFamily(aFrame, mNativeValue);
}

bool
TextAttrsMgr::FontFamilyTextAttr::
  GetValueFor(nsIContent* aElm, nsString* aValue)
{
  nsIFrame* frame = aElm->GetPrimaryFrame();
  return frame ? GetFontFamily(frame, *aValue) : false;
}

void
TextAttrsMgr::FontFamilyTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nsString& aValue)
{
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_family, aValue);
}

bool
TextAttrsMgr::FontFamilyTextAttr::
  GetFontFamily(nsIFrame* aFrame, nsString& aFamily)
{
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));

  gfxFontGroup* fontGroup = fm->GetThebesFontGroup();
  gfxFont* font = fontGroup->GetFontAt(0);
  gfxFontEntry* fontEntry = font->GetFontEntry();
  aFamily = fontEntry->FamilyName();
  return true;
}






TextAttrsMgr::FontSizeTextAttr::
  FontSizeTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscoord>(!aFrame)
{
  mDC = aRootFrame->PresContext()->DeviceContext();

  mRootNativeValue = aRootFrame->GetStyleFont()->mSize;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->GetStyleFont()->mSize;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontSizeTextAttr::
  GetValueFor(nsIContent* aElm, nscoord* aValue)
{
  nsIFrame* frame = aElm->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->GetStyleFont()->mSize;
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontSizeTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscoord& aValue)
{
  
  
  
  
  
  
  
  
  float px =
    NSAppUnitsToFloatPixels(aValue, nsDeviceContext::AppUnitsPerCSSPixel());
  
  int pts = NS_lround(px*3/4);

  nsAutoString value;
  value.AppendInt(pts);
  value.Append(NS_LITERAL_STRING("pt"));

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_size, value);
}






TextAttrsMgr::FontStyleTextAttr::
  FontStyleTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscoord>(!aFrame)
{
  mRootNativeValue = aRootFrame->GetStyleFont()->mFont.style;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->GetStyleFont()->mFont.style;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontStyleTextAttr::
  GetValueFor(nsIContent* aContent, nscoord* aValue)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->GetStyleFont()->mFont.style;
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontStyleTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscoord& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatFontStyle(aValue, formattedValue);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_style, formattedValue);
}






TextAttrsMgr::FontWeightTextAttr::
  FontWeightTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<PRInt32>(!aFrame)
{
  mRootNativeValue = GetFontWeight(aRootFrame);
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = GetFontWeight(aFrame);
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontWeightTextAttr::
  GetValueFor(nsIContent* aElm, PRInt32* aValue)
{
  nsIFrame* frame = aElm->GetPrimaryFrame();
  if (frame) {
    *aValue = GetFontWeight(frame);
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontWeightTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const PRInt32& aValue)
{
  nsAutoString formattedValue;
  formattedValue.AppendInt(aValue);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::fontWeight, formattedValue);
}

PRInt32
TextAttrsMgr::FontWeightTextAttr::
  GetFontWeight(nsIFrame* aFrame)
{
  
  
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));

  gfxFontGroup *fontGroup = fm->GetThebesFontGroup();
  gfxFont *font = fontGroup->GetFontAt(0);

  
  
  
  
  
  if (font->IsSyntheticBold())
    return 700;

#ifdef MOZ_PANGO
  
  
  
  return font->GetStyle()->weight;
#else
  
  
  
  
  
  
  gfxFontEntry *fontEntry = font->GetFontEntry();
  return fontEntry->Weight();
#endif
}






TextAttrsMgr::TextDecorValue::
  TextDecorValue(nsIFrame* aFrame)
{
  const nsStyleTextReset* textReset = aFrame->GetStyleTextReset();
  mStyle = textReset->GetDecorationStyle();

  bool isForegroundColor = false;
  textReset->GetDecorationColor(mColor, isForegroundColor);
  if (isForegroundColor)
    mColor = aFrame->GetStyleColor()->mColor;

  mLine = textReset->mTextDecorationLine &
    (NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE |
     NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH);
}

TextAttrsMgr::TextDecorTextAttr::
  TextDecorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<TextDecorValue>(!aFrame)
{
  mRootNativeValue = TextDecorValue(aRootFrame);
  mIsRootDefined = mRootNativeValue.IsDefined();

  if (aFrame) {
    mNativeValue = TextDecorValue(aFrame);
    mIsDefined = mNativeValue.IsDefined();
  }
}

bool
TextAttrsMgr::TextDecorTextAttr::
  GetValueFor(nsIContent* aContent, TextDecorValue* aValue)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame) {
    *aValue = TextDecorValue(frame);
    return aValue->IsDefined();
  }

  return false;
}

void
TextAttrsMgr::TextDecorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const TextDecorValue& aValue)
{
  if (aValue.IsUnderline()) {
    nsAutoString formattedStyle;
    StyleInfo::FormatTextDecorationStyle(aValue.Style(), formattedStyle);
    nsAccUtils::SetAccAttr(aAttributes,
                           nsGkAtoms::textUnderlineStyle,
                           formattedStyle);

    nsAutoString formattedColor;
    StyleInfo::FormatColor(aValue.Color(), formattedColor);
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textUnderlineColor,
                           formattedColor);
    return;
  }

  if (aValue.IsLineThrough()) {
    nsAutoString formattedStyle;
    StyleInfo::FormatTextDecorationStyle(aValue.Style(), formattedStyle);
    nsAccUtils::SetAccAttr(aAttributes,
                           nsGkAtoms::textLineThroughStyle,
                           formattedStyle);

    nsAutoString formattedColor;
    StyleInfo::FormatColor(aValue.Color(), formattedColor);
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textLineThroughColor,
                           formattedColor);
  }
}





TextAttrsMgr::TextPosTextAttr::
  TextPosTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<TextPosValue>(!aFrame)
{
  mRootNativeValue = GetTextPosValue(aRootFrame);
  mIsRootDefined = mRootNativeValue != eTextPosNone;

  if (aFrame) {
    mNativeValue = GetTextPosValue(aFrame);
    mIsDefined = mNativeValue != eTextPosNone;
  }
}

bool
TextAttrsMgr::TextPosTextAttr::
  GetValueFor(nsIContent* aContent, TextPosValue* aValue)
{
  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (frame) {
    *aValue = GetTextPosValue(frame);
    return *aValue != eTextPosNone;
  }

  return false;
}

void
TextAttrsMgr::TextPosTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const TextPosValue& aValue)
{
  switch (aValue) {
    case eTextPosBaseline:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("baseline"));
      break;

    case eTextPosSub:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("sub"));
      break;

    case eTextPosSuper:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("super"));
      break;
  }
}

TextAttrsMgr::TextPosValue
TextAttrsMgr::TextPosTextAttr::
  GetTextPosValue(nsIFrame* aFrame) const
{
  const nsStyleCoord& styleCoord = aFrame->GetStyleTextReset()->mVerticalAlign;
  switch (styleCoord.GetUnit()) {
    case eStyleUnit_Enumerated:
      switch (styleCoord.GetIntValue()) {
        case NS_STYLE_VERTICAL_ALIGN_BASELINE:
          return eTextPosBaseline;
        case NS_STYLE_VERTICAL_ALIGN_SUB:
          return eTextPosSub;
        case NS_STYLE_VERTICAL_ALIGN_SUPER:
          return eTextPosSuper;

        
        
        
        
        
        
        
        

        default:
          break;
      }
      return eTextPosNone;

    case eStyleUnit_Percent:
    {
      float percentValue = styleCoord.GetPercentValue();
      return percentValue > 0 ?
        eTextPosSuper :
        (percentValue < 0 ? eTextPosSub : eTextPosBaseline);
    }

    case eStyleUnit_Coord:
    {
       nscoord coordValue = styleCoord.GetCoordValue();
       return coordValue > 0 ?
         eTextPosSuper :
         (coordValue < 0 ? eTextPosSub : eTextPosBaseline);
    }
  }

  const nsIContent* content = aFrame->GetContent();
  if (content && content->IsHTML()) {
    const nsIAtom* tagName = content->Tag();
    if (tagName == nsGkAtoms::sup) 
      return eTextPosSuper;
    if (tagName == nsGkAtoms::sub) 
      return eTextPosSub;
  }

  return eTextPosNone;
}
