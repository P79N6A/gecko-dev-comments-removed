



































#include "nsSVGBoolean.h"

NS_IMPL_ADDREF(nsSVGBoolean::DOMAnimatedBoolean)
NS_IMPL_RELEASE(nsSVGBoolean::DOMAnimatedBoolean)

NS_INTERFACE_MAP_BEGIN(nsSVGBoolean::DOMAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedBoolean)
NS_INTERFACE_MAP_END



nsresult
nsSVGBoolean::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  PRBool val;

  if (aValueAsString.EqualsLiteral("true"))
    val = PR_TRUE;
  else if (aValueAsString.EqualsLiteral("false"))
    val = PR_FALSE;
  else
    return NS_ERROR_FAILURE;

  mBaseVal = mAnimVal = val;
  return NS_OK;
}

void
nsSVGBoolean::GetBaseValueString(nsAString & aValueAsString)
{
  aValueAsString.Assign(mBaseVal
                        ? NS_LITERAL_STRING("true")
                        : NS_LITERAL_STRING("false"));
}

void
nsSVGBoolean::SetBaseValue(int aValue,
                           nsSVGElement *aSVGElement,
                           PRBool aDoSetAttr)
{
  mAnimVal = mBaseVal = aValue;
  aSVGElement->DidChangeBoolean(mAttrEnum, aDoSetAttr);
}

nsresult
nsSVGBoolean::ToDOMAnimatedBoolean(nsIDOMSVGAnimatedBoolean **aResult,
                                   nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedBoolean(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

