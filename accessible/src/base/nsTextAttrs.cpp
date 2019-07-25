





































#include "nsTextAttrs.h"

#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "nsHyperTextAccessibleWrap.h"

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "nsFontMetrics.h"







struct nsCSSTextAttrMapItem
{
  const char* mCSSName;
  const char* mCSSValue;
  nsIAtom** mAttrName;
  const char* mAttrValue;
};




const char* const kAnyValue = nsnull;
const char* const kCopyValue = nsnull;

static nsCSSTextAttrMapItem gCSSTextAttrsMap[] =
{
  
  { "color",             kAnyValue,       &nsAccessibilityAtoms::color,                 kCopyValue },
  { "font-family",       kAnyValue,       &nsAccessibilityAtoms::fontFamily,            kCopyValue },
  { "font-style",        kAnyValue,       &nsAccessibilityAtoms::fontStyle,             kCopyValue },
  { "text-decoration",   "line-through",  &nsAccessibilityAtoms::textLineThroughStyle,  "solid" },
  { "text-decoration",   "underline",     &nsAccessibilityAtoms::textUnderlineStyle,    "solid" },
  { "vertical-align",    kAnyValue,       &nsAccessibilityAtoms::textPosition,          kCopyValue }
};




nsTextAttrsMgr::nsTextAttrsMgr(nsHyperTextAccessible *aHyperTextAcc,
                               PRBool aIncludeDefAttrs,
                               nsAccessible *aOffsetAcc,
                               PRInt32 aOffsetAccIdx) :
  mHyperTextAcc(aHyperTextAcc), mIncludeDefAttrs(aIncludeDefAttrs),
  mOffsetAcc(aOffsetAcc), mOffsetAccIdx(aOffsetAccIdx)
{
}

nsresult
nsTextAttrsMgr::GetAttributes(nsIPersistentProperties *aAttributes,
                              PRInt32 *aStartHTOffset,
                              PRInt32 *aEndHTOffset)
{
  
  
  
  
  
  
  NS_PRECONDITION(mHyperTextAcc &&
                  ((mOffsetAcc && mOffsetAccIdx != -1 &&
                    aStartHTOffset && aEndHTOffset) ||
                  (!mOffsetAcc && mOffsetAccIdx == -1 &&
                    !aStartHTOffset && !aEndHTOffset &&
                   mIncludeDefAttrs && aAttributes)),
                  "Wrong usage of nsTextAttrsMgr!");

  
  if (mOffsetAcc && nsAccUtils::IsEmbeddedObject(mOffsetAcc)) {
    for (PRInt32 childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
      nsAccessible *currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aStartHTOffset)--;
    }

    PRInt32 childCount = mHyperTextAcc->GetChildCount();
    for (PRInt32 childIdx = mOffsetAccIdx + 1; childIdx < childCount;
         childIdx++) {
      nsAccessible *currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aEndHTOffset)++;
    }

    return NS_OK;
  }

  
  
  nsIContent *hyperTextElm = mHyperTextAcc->GetContent();
  nsIFrame *rootFrame = mHyperTextAcc->GetFrame();
  NS_ASSERTION(rootFrame, "No frame for accessible!");
  if (!rootFrame)
    return NS_OK;

  nsIContent *offsetNode = nsnull, *offsetElm = nsnull;
  nsIFrame *frame = nsnull;
  if (mOffsetAcc) {
    offsetNode = mOffsetAcc->GetContent();
    offsetElm = nsCoreUtils::GetDOMElementFor(offsetNode);
    frame = offsetElm->GetPrimaryFrame();
  }

  nsTPtrArray<nsITextAttr> textAttrArray(10);

  
  nsLangTextAttr langTextAttr(mHyperTextAcc, hyperTextElm, offsetNode);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&langTextAttr));

  
  nsCSSTextAttr colorTextAttr(0, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&colorTextAttr));

  
  nsCSSTextAttr fontFamilyTextAttr(1, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&fontFamilyTextAttr));

  
  nsCSSTextAttr fontStyleTextAttr(2, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&fontStyleTextAttr));

  
  nsCSSTextAttr lineThroughTextAttr(3, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&lineThroughTextAttr));

  
  nsCSSTextAttr underlineTextAttr(4, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&underlineTextAttr));

  
  nsCSSTextAttr posTextAttr(5, hyperTextElm, offsetElm);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&posTextAttr));

  
  nsBGColorTextAttr bgColorTextAttr(rootFrame, frame);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&bgColorTextAttr));

  
  nsFontSizeTextAttr fontSizeTextAttr(rootFrame, frame);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&fontSizeTextAttr));

  
  nsFontWeightTextAttr fontWeightTextAttr(rootFrame, frame);
  textAttrArray.AppendElement(static_cast<nsITextAttr*>(&fontWeightTextAttr));

  
  if (aAttributes) {
    PRUint32 len = textAttrArray.Length();
    for (PRUint32 idx = 0; idx < len; idx++) {
      nsITextAttr *textAttr = textAttrArray[idx];

      nsAutoString value;
      if (textAttr->GetValue(value, mIncludeDefAttrs))
        nsAccUtils::SetAccAttr(aAttributes, textAttr->GetName(), value);
    }
  }

  nsresult rv = NS_OK;

  
  if (mOffsetAcc)
    rv = GetRange(textAttrArray, aStartHTOffset, aEndHTOffset);

  textAttrArray.Clear();
  return rv;
}

nsresult
nsTextAttrsMgr::GetRange(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                         PRInt32 *aStartHTOffset, PRInt32 *aEndHTOffset)
{
  PRUint32 attrLen = aTextAttrArray.Length();

  
  for (PRInt32 childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
    nsAccessible *currAcc = mHyperTextAcc->GetChildAt(childIdx);

    
    
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    nsIContent *currElm = nsCoreUtils::GetDOMElementFor(currAcc->GetContent());
    NS_ENSURE_STATE(currElm);

    PRBool offsetFound = PR_FALSE;
    for (PRUint32 attrIdx = 0; attrIdx < attrLen; attrIdx++) {
      nsITextAttr *textAttr = aTextAttrArray[attrIdx];
      if (!textAttr->Equal(currElm)) {
        offsetFound = PR_TRUE;
        break;
      }
    }

    if (offsetFound)
      break;

    *(aStartHTOffset) -= nsAccUtils::TextLength(currAcc);
  }

  
  PRInt32 childLen = mHyperTextAcc->GetChildCount();
  for (PRInt32 childIdx = mOffsetAccIdx + 1; childIdx < childLen; childIdx++) {
    nsAccessible *currAcc = mHyperTextAcc->GetChildAt(childIdx);
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    nsIContent *currElm = nsCoreUtils::GetDOMElementFor(currAcc->GetContent());
    NS_ENSURE_STATE(currElm);

    PRBool offsetFound = PR_FALSE;
    for (PRUint32 attrIdx = 0; attrIdx < attrLen; attrIdx++) {
      nsITextAttr *textAttr = aTextAttrArray[attrIdx];

      
      
      if (!textAttr->Equal(currElm)) {
        offsetFound = PR_TRUE;
        break;
      }
    }

    if (offsetFound)
      break;

    (*aEndHTOffset) += nsAccUtils::TextLength(currAcc);
  }

  return NS_OK;
}




nsLangTextAttr::nsLangTextAttr(nsHyperTextAccessible *aRootAcc, 
                               nsIContent *aRootContent, nsIContent *aContent) :
  nsTextAttr<nsAutoString>(aContent == nsnull), mRootContent(aRootContent)
{
  nsresult rv = aRootAcc->GetLanguage(mRootNativeValue);
  mIsRootDefined = NS_SUCCEEDED(rv) && !mRootNativeValue.IsEmpty();

  if (aContent)
    mIsDefined = GetLang(aContent, mNativeValue);
}

PRBool
nsLangTextAttr::GetValueFor(nsIContent *aElm, nsAutoString *aValue)
{
  return GetLang(aElm, *aValue);
}

void
nsLangTextAttr::Format(const nsAutoString& aValue, nsAString& aFormattedValue)
{
  aFormattedValue = aValue;
}

PRBool
nsLangTextAttr::GetLang(nsIContent *aContent, nsAString& aLang)
{
  nsCoreUtils::GetLanguageFor(aContent, mRootContent, aLang);
  return !aLang.IsEmpty();
}






nsCSSTextAttr::nsCSSTextAttr(PRUint32 aIndex, nsIContent *aRootContent,
                             nsIContent *aContent) :
  nsTextAttr<nsAutoString>(aContent == nsnull), mIndex(aIndex)
{
  mIsRootDefined = GetValueFor(aRootContent, &mRootNativeValue);

  if (aContent)
    mIsDefined = GetValueFor(aContent, &mNativeValue);
}

nsIAtom*
nsCSSTextAttr::GetName()
{
  return *gCSSTextAttrsMap[mIndex].mAttrName;
}

PRBool
nsCSSTextAttr::GetValueFor(nsIContent *aContent, nsAutoString *aValue)
{
  nsCOMPtr<nsIDOMCSSStyleDeclaration> currStyleDecl =
    nsCoreUtils::GetComputedStyleDeclaration(EmptyString(), aContent);
  if (!currStyleDecl)
    return PR_FALSE;

  NS_ConvertASCIItoUTF16 cssName(gCSSTextAttrsMap[mIndex].mCSSName);

  nsresult rv = currStyleDecl->GetPropertyValue(cssName, *aValue);
  if (NS_FAILED(rv))
    return PR_TRUE;

  const char *cssValue = gCSSTextAttrsMap[mIndex].mCSSValue;
  if (cssValue != kAnyValue && !aValue->EqualsASCII(cssValue))
    return PR_FALSE;

  return PR_TRUE;
}

void
nsCSSTextAttr::Format(const nsAutoString& aValue, nsAString& aFormattedValue)
{
  const char *attrValue = gCSSTextAttrsMap[mIndex].mAttrValue;
  if (attrValue != kCopyValue)
    AppendASCIItoUTF16(attrValue, aFormattedValue);
  else
    aFormattedValue = aValue;
}






nsBGColorTextAttr::nsBGColorTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame) :
  nsTextAttr<nscolor>(aFrame == nsnull), mRootFrame(aRootFrame)
{
  mIsRootDefined = GetColor(mRootFrame, &mRootNativeValue);
  if (aFrame)
    mIsDefined = GetColor(aFrame, &mNativeValue);
}

PRBool
nsBGColorTextAttr::GetValueFor(nsIContent *aContent, nscolor *aValue)
{
  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame)
    return PR_FALSE;

  return GetColor(frame, aValue);
}

void
nsBGColorTextAttr::Format(const nscolor& aValue, nsAString& aFormattedValue)
{
  
  nsAutoString value;
  value.AppendLiteral("rgb(");
  value.AppendInt(NS_GET_R(aValue));
  value.AppendLiteral(", ");
  value.AppendInt(NS_GET_G(aValue));
  value.AppendLiteral(", ");
  value.AppendInt(NS_GET_B(aValue));
  value.Append(')');

  aFormattedValue = value;
}

PRBool
nsBGColorTextAttr::GetColor(nsIFrame *aFrame, nscolor *aColor)
{
  const nsStyleBackground *styleBackground = aFrame->GetStyleBackground();

  if (NS_GET_A(styleBackground->mBackgroundColor) > 0) {
    *aColor = styleBackground->mBackgroundColor;
    return PR_TRUE;
  }

  nsIFrame *parentFrame = aFrame->GetParent();
  if (!parentFrame) {
    *aColor = aFrame->PresContext()->DefaultBackgroundColor();
    return PR_TRUE;
  }

  
  
  
  if (parentFrame == mRootFrame)
    return PR_FALSE;

  return GetColor(parentFrame, aColor);
}






nsFontSizeTextAttr::nsFontSizeTextAttr(nsIFrame *aRootFrame, nsIFrame *aFrame) :
  nsTextAttr<nscoord>(aFrame == nsnull)
{
  mDC = aRootFrame->PresContext()->DeviceContext();

  mRootNativeValue = GetFontSize(aRootFrame);
  mIsRootDefined = PR_TRUE;

  if (aFrame) {
    mNativeValue = GetFontSize(aFrame);
    mIsDefined = PR_TRUE;
  }
}

PRBool
nsFontSizeTextAttr::GetValueFor(nsIContent *aContent, nscoord *aValue)
{
  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame)
    return PR_FALSE;
  
  *aValue = GetFontSize(frame);
  return PR_TRUE;
}

void
nsFontSizeTextAttr::Format(const nscoord& aValue, nsAString& aFormattedValue)
{
  
  
  
  
  
  
  
  
  float px =
    NSAppUnitsToFloatPixels(aValue, nsIDeviceContext::AppUnitsPerCSSPixel());
  
  int pts = NS_lround(px*3/4);

  nsAutoString value;
  value.AppendInt(pts);
  value.Append(NS_LITERAL_STRING("pt"));
  aFormattedValue = value;
}

nscoord
nsFontSizeTextAttr::GetFontSize(nsIFrame *aFrame)
{
  nsStyleFont* styleFont =
    (nsStyleFont*)(aFrame->GetStyleDataExternal(eStyleStruct_Font));

  return styleFont->mSize;
}






nsFontWeightTextAttr::nsFontWeightTextAttr(nsIFrame *aRootFrame,
                                           nsIFrame *aFrame) :
  nsTextAttr<PRInt32>(aFrame == nsnull)
{
  mRootNativeValue = GetFontWeight(aRootFrame);
  mIsRootDefined = PR_TRUE;

  if (aFrame) {
    mNativeValue = GetFontWeight(aFrame);
    mIsDefined = PR_TRUE;
  }
}

PRBool
nsFontWeightTextAttr::GetValueFor(nsIContent *aContent, PRInt32 *aValue)
{
  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame)
    return PR_FALSE;

  *aValue = GetFontWeight(frame);
  return PR_TRUE;
}

void
nsFontWeightTextAttr::Format(const PRInt32& aValue, nsAString& aFormattedValue)
{
  nsAutoString value;
  value.AppendInt(aValue);
  aFormattedValue = value;
}

PRInt32
nsFontWeightTextAttr::GetFontWeight(nsIFrame *aFrame)
{
  
  
  nsStyleFont* styleFont =
    (nsStyleFont*)(aFrame->GetStyleDataExternal(eStyleStruct_Font));

  gfxUserFontSet *fs = aFrame->PresContext()->GetUserFontSet();

  nsRefPtr<nsFontMetrics> fm;
  aFrame->PresContext()->DeviceContext()->
    GetMetricsFor(styleFont->mFont, aFrame->GetStyleVisibility()->mLanguage,
                  fs, *getter_AddRefs(fm));

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
