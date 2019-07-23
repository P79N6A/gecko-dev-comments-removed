



































#include "nsSVGNumber2.h"
#include "nsTextFormatter.h"
#include "prdtoa.h"

NS_IMPL_ADDREF(nsSVGNumber2::DOMAnimatedNumber)
NS_IMPL_RELEASE(nsSVGNumber2::DOMAnimatedNumber)

NS_INTERFACE_MAP_BEGIN(nsSVGNumber2::DOMAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedNumber)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedNumber)
NS_INTERFACE_MAP_END



nsresult
nsSVGNumber2::SetBaseValueString(const nsAString &aValueAsString,
                                 nsSVGElement *aSVGElement,
                                 PRBool aDoSetAttr)
{
  nsAutoString s;
  s.Assign(aValueAsString);
  PRInt32 err;
  float val = s.ToFloat(&err);
  nsresult rv = static_cast<nsresult>(err);
  NS_ENSURE_SUCCESS(rv, rv);
  mBaseVal = mAnimVal = val;
  return NS_OK;
}

void
nsSVGNumber2::GetBaseValueString(nsAString & aValueAsString)
{
  nsAutoString s;
  s.AppendFloat(mBaseVal);
  aValueAsString.Assign(s);
}

void
nsSVGNumber2::SetBaseValue(float aValue,
                           nsSVGElement *aSVGElement,
                           PRBool aDoSetAttr)
{
  mAnimVal = mBaseVal = aValue;
  aSVGElement->DidChangeNumber(mAttrEnum, aDoSetAttr);
}

nsresult
nsSVGNumber2::ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                                  nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedNumber(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}

