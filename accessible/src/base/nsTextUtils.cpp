





































#include "nsTextUtils.h"

#include "nsAccessNode.h"
#include "nsAccessibilityUtils.h"




PRBool
nsLangTextAttr::equal(nsIDOMElement *aElm)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aElm));
  if (!content)
    return PR_FALSE;

  nsAutoString lang;
  nsAccUtils::GetLanguageFor(content, mRootContent, lang);

  return lang == mLang;
}







struct nsCSSTextAttrMapItem
{
  const char* mCSSName;
  const char* mCSSValue;
  const char* mAttrName;
  const char* mAttrValue;
};





const char* const kAnyValue = nsnull;
const char* const kCopyName = nsnull;
const char* const kCopyValue = nsnull;

static nsCSSTextAttrMapItem gCSSTextAttrsMap[] = {
  
  { "color",             kAnyValue,       kCopyName,                  kCopyValue },
  { "font-family",       kAnyValue,       kCopyName,                  kCopyValue },
  { "font-size",         kAnyValue,       kCopyName,                  kCopyValue },
  { "font-style",        kAnyValue,       kCopyName,                  kCopyValue },
  { "font-weight",       kAnyValue,       kCopyName,                  kCopyValue },
  { "text-decoration",   "line-through",  "text-line-through-style",  "solid" },
  { "text-decoration",   "underline",     "text-underline-style",     "solid" },
  { "text-align",        kAnyValue,       kCopyName,                  kCopyValue },
  { "text-indent",       kAnyValue,       kCopyName,                  kCopyValue },
  { "vertical-align",    kAnyValue,       "text-position",            kCopyValue }
};

nsCSSTextAttr::nsCSSTextAttr(PRBool aIncludeDefAttrValue, nsIDOMElement *aElm,
                             nsIDOMElement *aRootElm) :
  mIndex(-1), mIncludeDefAttrValue(aIncludeDefAttrValue)
{
  nsAccessNode::GetComputedStyleDeclaration(EmptyString(), aElm,
                                            getter_AddRefs(mStyleDecl));

  if (!mIncludeDefAttrValue)
    nsAccessNode::GetComputedStyleDeclaration(EmptyString(), aRootElm,
                                              getter_AddRefs(mDefStyleDecl));
}

PRBool
nsCSSTextAttr::equal(nsIDOMElement *aElm)
{
  if (!aElm || !mStyleDecl)
    return PR_FALSE;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> currStyleDecl;
  nsAccessNode::GetComputedStyleDeclaration(EmptyString(), aElm,
                                            getter_AddRefs(currStyleDecl));
  if (!currStyleDecl)
    return PR_FALSE;

  NS_ConvertASCIItoUTF16 cssName(gCSSTextAttrsMap[mIndex].mCSSName);

  nsAutoString currValue;
  nsresult rv = currStyleDecl->GetPropertyValue(cssName, currValue);
  if (NS_FAILED(rv))
    return PR_FALSE;

  nsAutoString value;
  rv = mStyleDecl->GetPropertyValue(cssName, value);
  return NS_SUCCEEDED(rv) && value == currValue;
}

PRBool
nsCSSTextAttr::iterate()
{
  return ++mIndex < static_cast<PRInt32>(NS_ARRAY_LENGTH(gCSSTextAttrsMap));
}

PRBool
nsCSSTextAttr::get(nsACString& aName, nsAString& aValue)
{
  if (!mStyleDecl)
    return PR_FALSE;

  NS_ConvertASCIItoUTF16 cssName(gCSSTextAttrsMap[mIndex].mCSSName);
  nsresult rv = mStyleDecl->GetPropertyValue(cssName, aValue);
  if (NS_FAILED(rv))
    return PR_FALSE;

  
  
  
  if (!mIncludeDefAttrValue) {
    if (!mDefStyleDecl)
      return PR_FALSE;

    nsAutoString defValue;
    mDefStyleDecl->GetPropertyValue(cssName, defValue);
    if (defValue == aValue)
      return PR_FALSE;
  }

  
  
  const char *cssValue = gCSSTextAttrsMap[mIndex].mCSSValue;
  if (cssValue != kAnyValue && !aValue.EqualsASCII(cssValue))
    return PR_FALSE;

  
  if (gCSSTextAttrsMap[mIndex].mAttrName != kCopyName)
    aName = gCSSTextAttrsMap[mIndex].mAttrName;
  else
    aName = gCSSTextAttrsMap[mIndex].mCSSName;

  
  const char *attrValue = gCSSTextAttrsMap[mIndex].mAttrValue;
  if (attrValue != kCopyValue)
    AppendASCIItoUTF16(attrValue, aValue);

  return PR_TRUE;
}




nsBackgroundTextAttr::nsBackgroundTextAttr(nsIFrame *aFrame,
                                           nsIFrame *aRootFrame) :
  mFrame(aFrame), mRootFrame(aRootFrame)
{
}

PRBool
nsBackgroundTextAttr::equal(nsIDOMElement *aElm)
{
  nsIFrame *frame = nsAccUtils::GetFrameFor(aElm);
  if (!frame)
    return PR_FALSE;

  return getColor(mFrame) == getColor(frame);    
}

PRBool
nsBackgroundTextAttr::get(nsAString& aValue)
{
  
  
  nscolor color = getColor(mFrame);
  if (mRootFrame && color == getColor(mRootFrame))
    return PR_FALSE;

  
  nsAutoString value;
  value.AppendLiteral("rgb(");
  value.AppendInt(NS_GET_R(color));
  value.AppendLiteral(", ");
  value.AppendInt(NS_GET_G(color));
  value.AppendLiteral(", ");
  value.AppendInt(NS_GET_B(color));
  value.Append(')');

  aValue = value;
  return PR_TRUE;
}

nscolor
nsBackgroundTextAttr::getColor(nsIFrame *aFrame)
{
  const nsStyleBackground *styleBackground = aFrame->GetStyleBackground();

  if (!styleBackground->IsTransparent())
    return styleBackground->mBackgroundColor;

  nsIFrame *parentFrame = aFrame->GetParent();
  if (!parentFrame)
    return styleBackground->mBackgroundColor;

  
  
  
  if (parentFrame == mRootFrame)
    return getColor(mRootFrame);

  return getColor(parentFrame);
}
