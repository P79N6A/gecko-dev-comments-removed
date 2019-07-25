



































#include "nsSVGEnum.h"
#include "nsIAtom.h"
#include "nsSVGElement.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SMILEnumType.h"
#endif 

using namespace mozilla;

NS_SVG_VAL_IMPL_CYCLE_COLLECTION(nsSVGEnum::DOMAnimatedEnum, mSVGElement)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsSVGEnum::DOMAnimatedEnum)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsSVGEnum::DOMAnimatedEnum)

DOMCI_DATA(SVGAnimatedEnumeration, nsSVGEnum::DOMAnimatedEnum)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGEnum::DOMAnimatedEnum)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedEnumeration)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGAnimatedEnumeration)
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
                              bool aDoSetAttr)
{
  nsCOMPtr<nsIAtom> valAtom = do_GetAtom(aValue);

  nsSVGEnumMapping *mapping = GetMapping(aSVGElement);

  while (mapping && mapping->mKey) {
    if (valAtom == *(mapping->mKey)) {
      mIsBaseSet = PR_TRUE;
      if (mBaseVal != mapping->mVal) {
        mBaseVal = mapping->mVal;
        if (!mIsAnimated) {
          mAnimVal = mBaseVal;
        }
#ifdef MOZ_SMIL
        else {
          aSVGElement->AnimationNeedsResample();
        }
#endif
        
        
        
      }
      return NS_OK;
    }
    mapping++;
  }

  
  NS_WARNING("unknown enumeration key");
  return NS_ERROR_DOM_SYNTAX_ERR;
}

void
nsSVGEnum::GetBaseValueString(nsAString& aValue, nsSVGElement *aSVGElement)
{
  nsSVGEnumMapping *mapping = GetMapping(aSVGElement);

  while (mapping && mapping->mKey) {
    if (mBaseVal == mapping->mVal) {
      (*mapping->mKey)->ToString(aValue);
      return;
    }
    mapping++;
  }
  NS_ERROR("unknown enumeration value");
}

nsresult
nsSVGEnum::SetBaseValue(PRUint16 aValue,
                        nsSVGElement *aSVGElement,
                        bool aDoSetAttr)
{
  nsSVGEnumMapping *mapping = GetMapping(aSVGElement);

  while (mapping && mapping->mKey) {
    if (mapping->mVal == aValue) {
      mIsBaseSet = PR_TRUE;
      if (mBaseVal != PRUint8(aValue)) {
        mBaseVal = PRUint8(aValue);
        if (!mIsAnimated) {
          mAnimVal = mBaseVal;
        }
#ifdef MOZ_SMIL
        else {
          aSVGElement->AnimationNeedsResample();
        }
#endif
        aSVGElement->DidChangeEnum(mAttrEnum, aDoSetAttr);
      }
      return NS_OK;
    }
    mapping++;
  }
  return NS_ERROR_DOM_SYNTAX_ERR;
}

void
nsSVGEnum::SetAnimValue(PRUint16 aValue, nsSVGElement *aSVGElement)
{
  mAnimVal = aValue;
  mIsAnimated = PR_TRUE;
  aSVGElement->DidAnimateEnum(mAttrEnum);
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

#ifdef MOZ_SMIL
nsISMILAttr*
nsSVGEnum::ToSMILAttr(nsSVGElement *aSVGElement)
{
  return new SMILEnum(this, aSVGElement);
}

nsresult
nsSVGEnum::SMILEnum::ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* ,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const
{
  nsCOMPtr<nsIAtom> valAtom = do_GetAtom(aStr);
  nsSVGEnumMapping *mapping = mVal->GetMapping(mSVGElement);

  while (mapping && mapping->mKey) {
    if (valAtom == *(mapping->mKey)) {
      nsSMILValue val(&SMILEnumType::sSingleton);
      val.mU.mUint = mapping->mVal;
      aValue = val;
      aPreventCachingOfSandwich = PR_FALSE;
      return NS_OK;
    }
    mapping++;
  }
  
  
  NS_WARNING("unknown enumeration key");
  return NS_ERROR_FAILURE;
}

nsSMILValue
nsSVGEnum::SMILEnum::GetBaseValue() const
{
  nsSMILValue val(&SMILEnumType::sSingleton);
  val.mU.mUint = mVal->mBaseVal;
  return val;
}

void
nsSVGEnum::SMILEnum::ClearAnimValue()
{
  if (mVal->mIsAnimated) {
    mVal->SetAnimValue(mVal->mBaseVal, mSVGElement);
    mVal->mIsAnimated = PR_FALSE;
  }
}

nsresult
nsSVGEnum::SMILEnum::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SMILEnumType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SMILEnumType::sSingleton) {
    NS_ABORT_IF_FALSE(aValue.mU.mUint <= USHRT_MAX,
                      "Very large enumerated value - too big for PRUint16");
    mVal->SetAnimValue(PRUint16(aValue.mU.mUint), mSVGElement);
  }
  return NS_OK;
}
#endif 
