



































#include "nsSVGString.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILStringType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGString::DOMAnimatedString, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGString::DOMAnimatedString)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGString::DOMAnimatedString)

DOMCI_DATA(SVGAnimatedString, nsSVGString::DOMAnimatedString)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGString::DOMAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedString)
NS_INTERFACE_MAP_END



void
nsSVGString::SetBaseValue(const nsAString& aValue,
                          nsSVGElement *aSVGElement,
                          PRBool aDoSetAttr)
{
  NS_ASSERTION(aSVGElement, "Null element passed to SetBaseValue");

  if (aDoSetAttr) {
    aSVGElement->SetStringBaseValue(mAttrEnum, aValue);
  }
#ifdef MOZ_SMIL
  if (mAnimVal) {
    aSVGElement->AnimationNeedsResample();
  }
#endif

  aSVGElement->DidChangeString(mAttrEnum);
}

void
nsSVGString::GetAnimValue(nsAString& aResult, const nsSVGElement *aSVGElement) const
{
  if (mAnimVal) {
    aResult = *mAnimVal;
    return;
  }

  aSVGElement->GetStringBaseValue(mAttrEnum, aResult);
}

void
nsSVGString::SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement)
{
  if (aSVGElement->IsStringAnimatable(mAttrEnum)) {
    if (!mAnimVal) {
      mAnimVal = new nsString();
    }
    *mAnimVal = aValue;
    aSVGElement->DidAnimateString(mAttrEnum);
  }
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

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGString::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILString(this, aSVGElement);
}

nsresult
nsSVGString::SMILString::ValueFromString(const nsAString& aStr,
                                         const nsISMILAnimationElement* ,
                                         nsSMILValue& aValue,
                                         PRBool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SMILStringType::sSingleton);

  *static_cast<nsAString*>(val.mU.mPtr) = aStr;
  aValue.Swap(val);
  aPreventCachingOfSandwich = PR_FALSE;
  return NS_OK;
}

nsSMILValue
nsSVGString::SMILString::GetBaseValue() const
{
  nsSMILValue val(&SMILStringType::sSingleton);
  mSVGElement->GetStringBaseValue(mVal->mAttrEnum, *static_cast<nsAString*>(val.mU.mPtr));
  return val;
}

void
nsSVGString::SMILString::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->mAnimVal = nsnull;
    mSVGElement->DidAnimateString(mVal->mAttrEnum);
  }
}

nsresult
nsSVGString::SMILString::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILStringType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILStringType::sSingleton) {
    mVal->SetAnimValue(*static_cast<nsAString*>(aValue.mU.mPtr), mSVGElement);
  }
  return NS_OK;
}
#endif 
