



































#include "nsSVGEnum.h"
#include "nsIAtom.h"
#include "nsSVGElement.h"

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGEnum::DOMAnimatedEnum, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGEnum::DOMAnimatedEnum)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGEnum::DOMAnimatedEnum)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGEnum::DOMAnimatedEnum)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedEnumeration)
NS_INTERFACE_MAP_END

nsSVGEnumMapping *
nsSVGEnum::GetMapping(nsSVGElement *aSVGElement)
{
  nsSVGElement::EnumAttributesInfo info = aSVGElement->GetEnumInfo();

  NS_ASSERTION(info.mEnumCount > 0 && mAttrEnum < info.mEnumCount,
               "mapping request for a non-attrib enum");

  return info.mEnumInfo[mAttrEnum].mMapping;
}

nsresult
nsSVGEnum::SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr)
{
  nsCOMPtr<nsIAtom> valAtom = do_GetAtom(aValue);

  nsSVGEnumMapping *tmp = GetMapping(aSVGElement);

  while (tmp && tmp->mKey) {
    if (valAtom == *(tmp->mKey)) {
      mBaseVal = mAnimVal = tmp->mVal;
      return NS_OK;
    }
    tmp++;
  }

  
  NS_WARNING("unknown enumeration key");
  return NS_ERROR_FAILURE;
}

void
nsSVGEnum::GetBaseValueString(nsAString& aValue, nsSVGElement *aSVGElement)
{
  nsSVGEnumMapping *tmp = GetMapping(aSVGElement);

  while (tmp && tmp->mKey) {
    if (mBaseVal == tmp->mVal) {
      (*tmp->mKey)->ToString(aValue);
      return;
    }
    tmp++;
  }
  NS_ERROR("unknown enumeration value");
}

nsresult
nsSVGEnum::SetBaseValue(PRUint16 aValue,
                        nsSVGElement *aSVGElement,
                        PRBool aDoSetAttr)
{
  nsSVGEnumMapping *tmp = GetMapping(aSVGElement);

  while (tmp && tmp->mKey) {
    if (tmp->mVal == aValue) {
      mAnimVal = mBaseVal = PRUint8(aValue);
      aSVGElement->DidChangeEnum(mAttrEnum, aDoSetAttr);
      return NS_OK;
    }
    tmp++;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsSVGEnum::ToDOMAnimatedEnum(nsIDOMSVGAnimatedEnumeration **aResult,
                             nsSVGElement *aSVGElement)
{
  *aResult = new DOMAnimatedEnum(this, aSVGElement);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(*aResult);
  return NS_OK;
}
