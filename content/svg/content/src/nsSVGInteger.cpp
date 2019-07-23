



































#include "nsSVGInteger.h"


NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGInteger::DOMAnimatedInteger)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGInteger::DOMAnimatedInteger)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGInteger::DOMAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedInteger)
NS_INTERFACE_MAP_END



nsresult
nsSVGInteger::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_FAILURE;
  
  char *rest;
  PRInt32 val = strtol(str, &rest, 10);
  if (rest == str || *rest != '\0') {
    return NS_ERROR_FAILURE;
  }

  mBaseVal = mAnimVal = val;
  return NS_OK;
}

void
nsSVGInteger::GetBaseValueString(nsAString & aValueAsString)
{
  nsAutoString s;
  s.AppendInt(mBaseVal);
  aValueAsString.Assign(s);
}

void
nsSVGInteger::SetBaseValue(int aValue,
                           nsSVGElement *aSVGElement,
                           PRBool aDoSetAttr)
{
  mAnimVal = mBaseVal = aValue;
  aSVGElement->DidChangeInteger(mAttrEnum, aDoSetAttr);
}

nsresult
nsSVGInteger::ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                   nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedInteger(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
