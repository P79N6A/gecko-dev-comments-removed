



































#include "nsSVGInteger.h"

NS_IMPL_ADDREF(nsSVGInteger::DOMAnimatedInteger)
NS_IMPL_RELEASE(nsSVGInteger::DOMAnimatedInteger)

NS_INTERFACE_MAP_BEGIN(nsSVGInteger::DOMAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedInteger)
NS_INTERFACE_MAP_END



nsresult
nsSVGInteger::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  nsAutoString s;
  s.Assign(aValueAsString);
  PRInt32 err;
  PRInt32 val = s.ToInteger(&err);
  nsresult rv = static_cast<nsresult>(err);
  NS_ENSURE_SUCCESS(rv, rv);
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

