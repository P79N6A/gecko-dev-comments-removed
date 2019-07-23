



































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
  NS_ConvertUTF16toUTF8 value(aValueAsString);
  const char *str = value.get();

  if (NS_IsAsciiWhitespace(*str))
    return NS_ERROR_FAILURE;
  
  char *rest;
  float val = float(PR_strtod(str, &rest));
  if (rest == str || *rest != '\0') {
    return NS_ERROR_FAILURE;
  }

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

