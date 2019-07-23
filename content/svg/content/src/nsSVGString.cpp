



































#include "nsSVGString.h"

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGString::DOMAnimatedString, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGString::DOMAnimatedString)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGString::DOMAnimatedString)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGString::DOMAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedString)
NS_INTERFACE_MAP_END



void
nsSVGString::SetBaseValue(const nsAString& aValue,
                          nsSVGElement *aSVGElement,
                          PRBool aDoSetAttr)
{
  mAnimVal = mBaseVal = aValue;
  aSVGElement->DidChangeString(mAttrEnum, aDoSetAttr);
}

nsresult
nsSVGString::ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                                 nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedString(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
