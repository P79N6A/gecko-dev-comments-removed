





































#include "nsTextAttrs.h"

#include "nsAccessNode.h"
#include "nsHyperTextAccessibleWrap.h"

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "nsIThebesFontMetrics.h"







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
                               nsIDOMNode *aHyperTextNode,
                               PRBool aIncludeDefAttrs,
                               nsIDOMNode *aOffsetNode) :
  mHyperTextAcc(aHyperTextAcc), mHyperTextNode(aHyperTextNode),
  mIncludeDefAttrs(aIncludeDefAttrs), mOffsetNode(aOffsetNode)
{
}

nsresult
nsTextAttrsMgr::GetAttributes(nsIPersistentProperties *aAttributes,
                              PRInt32 *aStartHTOffset,
                              PRInt32 *aEndHTOffset)
{
  
  
  
  
  
  
  NS_PRECONDITION(mHyperTextAcc && mHyperTextNode &&
                  ((mOffsetNode && aStartHTOffset && aEndHTOffset) ||
                  (!mOffsetNode && !aStartHTOffset && !aEndHTOffset &&
                   mIncludeDefAttrs && aAttributes)),
                  "Wrong usage of nsTextAttrsMgr!");

  nsCOMPtr<nsIDOMElement> hyperTextElm =
    nsCoreUtils::GetDOMElementFor(mHyperTextNode);
  nsCOMPtr<nsIDOMElement> offsetElm;
  if (mOffsetNode)
    offsetElm = nsCoreUtils::GetDOMElementFor(mOffsetNode);

  nsIFrame *rootFrame = nsCoreUtils::GetFrameFor(hyperTextElm);
  nsIFrame *frame = nsnull;
  if (offsetElm)
    frame = nsCoreUtils::GetFrameFor(offsetElm);

  nsTPtrArray<nsITextAttr> textAttrArray(10);

  
  nsLangTextAttr langTextAttr(mHyperTextAcc, mHyperTextNode, mOffsetNode);
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

  
  if (mOffsetNode)
    rv = GetRange(textAttrArray, aStartHTOffset, aEndHTOffset);

  textAttrArray.Clear();
  return rv;
}

nsresult
nsTextAttrsMgr::GetRange(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                         PRInt32 *aStartHTOffset, PRInt32 *aEndHTOffset)
{
  nsCOMPtr<nsIDOMElement> rootElm =
    nsCoreUtils::GetDOMElementFor(mHyperTextNode);
  NS_ENSURE_STATE(rootElm);

  nsCOMPtr<nsIDOMNode> tmpNode(mOffsetNode);
  nsCOMPtr<nsIDOMNode> currNode(mOffsetNode);

  PRUint32 len = aTextAttrArray.Length();

  
  
  
  
  
  
  

  
  while (currNode && currNode != rootElm) {
    nsCOMPtr<nsIDOMElement> currElm(nsCoreUtils::GetDOMElementFor(currNode));
    NS_ENSURE_STATE(currElm);

    if (currNode != mOffsetNode) {
      PRBool stop = PR_FALSE;
      for (PRUint32 idx = 0; idx < len; idx++) {
        nsITextAttr *textAttr = aTextAttrArray[idx];
        if (!textAttr->Equal(currElm)) {

          PRInt32 startHTOffset = 0;
          nsCOMPtr<nsIAccessible> startAcc;
          nsresult rv = mHyperTextAcc->
            DOMPointToHypertextOffset(tmpNode, -1, &startHTOffset,
                                      getter_AddRefs(startAcc));
          NS_ENSURE_SUCCESS(rv, rv);

          if (!startAcc)
            startHTOffset = 0;

          if (startHTOffset > *aStartHTOffset)
            *aStartHTOffset = startHTOffset;

          stop = PR_TRUE;
          break;
        }
      }
      if (stop)
        break;
    }

    currNode->GetPreviousSibling(getter_AddRefs(tmpNode));
    if (tmpNode) {
      
      
      FindStartOffsetInSubtree(aTextAttrArray, tmpNode, currNode,
                               aStartHTOffset);
    }

    currNode->GetParentNode(getter_AddRefs(tmpNode));
    currNode.swap(tmpNode);
  }

  
  PRBool moveIntoSubtree = PR_TRUE;
  currNode = mOffsetNode;

  while (currNode && currNode != rootElm) {
    nsCOMPtr<nsIDOMElement> currElm(nsCoreUtils::GetDOMElementFor(currNode));
    NS_ENSURE_STATE(currElm);

    
    
    PRBool stop = PR_FALSE;
    for (PRUint32 idx = 0; idx < len; idx++) {
      nsITextAttr *textAttr = aTextAttrArray[idx];
      if (!textAttr->Equal(currElm)) {

        PRInt32 endHTOffset = 0;
        nsresult rv = mHyperTextAcc->
          DOMPointToHypertextOffset(currNode, -1, &endHTOffset);
        NS_ENSURE_SUCCESS(rv, rv);

        if (endHTOffset < *aEndHTOffset)
          *aEndHTOffset = endHTOffset;

        stop = PR_TRUE;
        break;
      }
    }

    if (stop)
      break;

    if (moveIntoSubtree) {
      
      
      currNode->GetFirstChild(getter_AddRefs(tmpNode));
      if (tmpNode)
        FindEndOffsetInSubtree(aTextAttrArray, tmpNode, aEndHTOffset);
    }

    currNode->GetNextSibling(getter_AddRefs(tmpNode));
    moveIntoSubtree = PR_TRUE;
    if (!tmpNode) {
      currNode->GetParentNode(getter_AddRefs(tmpNode));
      moveIntoSubtree = PR_FALSE;
    }

    currNode.swap(tmpNode);
  }

  return NS_OK;
}

PRBool
nsTextAttrsMgr::FindEndOffsetInSubtree(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                                       nsIDOMNode *aCurrNode,
                                       PRInt32 *aHTOffset)
{
  if (!aCurrNode)
    return PR_FALSE;

  nsCOMPtr<nsIDOMElement> currElm(nsCoreUtils::GetDOMElementFor(aCurrNode));
  if (!currElm)
    return PR_FALSE;

  
  
  PRUint32 len = aTextAttrArray.Length();
  for (PRUint32 idx = 0; idx < len; idx++) {
    nsITextAttr *textAttr = aTextAttrArray[idx];
    if (!textAttr->Equal(currElm)) {
      PRInt32 endHTOffset = 0;
      nsresult rv = mHyperTextAcc->
        DOMPointToHypertextOffset(aCurrNode, -1, &endHTOffset);
      NS_ENSURE_SUCCESS(rv, PR_FALSE);

      if (endHTOffset < *aHTOffset)
        *aHTOffset = endHTOffset;

      return PR_TRUE;
    }
  }

  
  nsCOMPtr<nsIDOMNode> nextNode;
  aCurrNode->GetFirstChild(getter_AddRefs(nextNode));
  if (nextNode) {
    PRBool res = FindEndOffsetInSubtree(aTextAttrArray, nextNode, aHTOffset);
    if (res)
      return res;
  }

  aCurrNode->GetNextSibling(getter_AddRefs(nextNode));
  if (nextNode) {
    if (FindEndOffsetInSubtree(aTextAttrArray, nextNode, aHTOffset))
      return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsTextAttrsMgr::FindStartOffsetInSubtree(const nsTPtrArray<nsITextAttr>& aTextAttrArray,
                                         nsIDOMNode *aCurrNode,
                                         nsIDOMNode *aPrevNode,
                                         PRInt32 *aHTOffset)
{
  if (!aCurrNode)
    return PR_FALSE;

  
  nsCOMPtr<nsIDOMNode> nextNode;
  aCurrNode->GetLastChild(getter_AddRefs(nextNode));
  if (nextNode) {
    if (FindStartOffsetInSubtree(aTextAttrArray, nextNode, aPrevNode, aHTOffset))
      return PR_TRUE;
  }

  nsCOMPtr<nsIDOMElement> currElm(nsCoreUtils::GetDOMElementFor(aCurrNode));
  if (!currElm)
    return PR_FALSE;

  
  
  PRUint32 len = aTextAttrArray.Length();
  for (PRUint32 idx = 0; idx < len; idx++) {
    nsITextAttr *textAttr = aTextAttrArray[idx];
    if (!textAttr->Equal(currElm)) {

      PRInt32 startHTOffset = 0;
      nsCOMPtr<nsIAccessible> startAcc;
      nsresult rv = mHyperTextAcc->
        DOMPointToHypertextOffset(aPrevNode, -1, &startHTOffset,
                                  getter_AddRefs(startAcc));
      NS_ENSURE_SUCCESS(rv, PR_FALSE);

      if (!startAcc)
        startHTOffset = 0;

      if (startHTOffset > *aHTOffset)
        *aHTOffset = startHTOffset;

      return PR_TRUE;
    }
  }

  
  aCurrNode->GetPreviousSibling(getter_AddRefs(nextNode));
  if (nextNode) {
    if (FindStartOffsetInSubtree(aTextAttrArray, nextNode, aCurrNode, aHTOffset))
      return PR_TRUE;
  }

  return PR_FALSE;
}




nsLangTextAttr::nsLangTextAttr(nsHyperTextAccessible *aRootAcc, 
                               nsIDOMNode *aRootNode, nsIDOMNode *aNode) :
  nsTextAttr<nsAutoString>(aNode == nsnull)
{
  mRootContent = do_QueryInterface(aRootNode);

  nsresult rv = aRootAcc->GetLanguage(mRootNativeValue);
  mIsRootDefined = NS_SUCCEEDED(rv) && !mRootNativeValue.IsEmpty();

  if (aNode) {
    nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
    mIsDefined = GetLang(content, mNativeValue);
  }
}

PRBool
nsLangTextAttr::GetValueFor(nsIDOMElement *aElm, nsAutoString *aValue)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(aElm);
  return GetLang(content, *aValue);
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




nsCSSTextAttr::nsCSSTextAttr(PRUint32 aIndex, nsIDOMElement *aRootElm,
                             nsIDOMElement *aElm) :
  nsTextAttr<nsAutoString>(aElm == nsnull), mIndex(aIndex)
{
  mIsRootDefined = GetValueFor(aRootElm, &mRootNativeValue);

  if (aElm)
    mIsDefined = GetValueFor(aElm, &mNativeValue);
}

nsIAtom*
nsCSSTextAttr::GetName()
{
  return *gCSSTextAttrsMap[mIndex].mAttrName;
}

PRBool
nsCSSTextAttr::GetValueFor(nsIDOMElement *aElm, nsAutoString *aValue)
{
  nsCOMPtr<nsIDOMCSSStyleDeclaration> currStyleDecl;
  nsCoreUtils::GetComputedStyleDeclaration(EmptyString(), aElm,
                                           getter_AddRefs(currStyleDecl));
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
nsBGColorTextAttr::GetValueFor(nsIDOMElement *aElm, nscolor *aValue)
{
  nsIFrame *frame = nsCoreUtils::GetFrameFor(aElm);
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

  if (NS_GET_A(styleBackground->mFallbackBackgroundColor) > 0) {
    *aColor = styleBackground->mFallbackBackgroundColor;
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
nsFontSizeTextAttr::GetValueFor(nsIDOMElement *aElm, nscoord *aValue)
{
  nsIFrame *frame = nsCoreUtils::GetFrameFor(aElm);
  if (!frame)
    return PR_FALSE;
  
  *aValue = GetFontSize(frame);
  return PR_TRUE;
}

void
nsFontSizeTextAttr::Format(const nscoord& aValue, nsAString& aFormattedValue)
{
  
  
  
  
  
  
  
  
  float inches = static_cast<float>(aValue) /
    static_cast<float>(mDC->AppUnitsPerInch());
  int pts = static_cast<int>(inches * 72 + .5); 

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
nsFontWeightTextAttr::GetValueFor(nsIDOMElement *aElm, PRInt32 *aValue)
{
  nsIFrame *frame = nsCoreUtils::GetFrameFor(aElm);
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

  nsCOMPtr<nsIFontMetrics> fm;
  aFrame->PresContext()->DeviceContext()->
    GetMetricsFor(styleFont->mFont, aFrame->GetStyleVisibility()->mLangGroup,
                  fs, *getter_AddRefs(fm));

  nsCOMPtr<nsIThebesFontMetrics> tfm = do_QueryInterface(fm);
  gfxFontGroup *fontGroup = tfm->GetThebesFontGroup();
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
