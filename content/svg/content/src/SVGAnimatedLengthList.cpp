




#include "SVGAnimatedLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSMILValue.h"
#include "SVGLengthListSMILType.h"

namespace mozilla {

nsresult
SVGAnimatedLengthList::SetBaseValueString(const nsAString& aValue)
{
  SVGLengthList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalBaseValListWillChangeTo(newBaseValue);
  }

  
  
  

  rv = mBaseVal.CopyFrom(newBaseValue);
  if (NS_FAILED(rv) && domWrapper) {
    
    
    domWrapper->InternalBaseValListWillChangeTo(mBaseVal);
  }
  return rv;
}

void
SVGAnimatedLengthList::ClearBaseValue(uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    domWrapper->InternalBaseValListWillChangeTo(SVGLengthList());
  }
  mBaseVal.Clear();
  
}

nsresult
SVGAnimatedLengthList::SetAnimValue(const SVGLengthList& aNewAnimValue,
                                    nsSVGElement *aElement,
                                    uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeTo(aNewAnimValue);
  }
  if (!mAnimVal) {
    mAnimVal = new SVGLengthList();
  }
  nsresult rv = mAnimVal->CopyFrom(aNewAnimValue);
  if (NS_FAILED(rv)) {
    
    
    ClearAnimValue(aElement, aAttrEnum);
    return rv;
  }
  aElement->DidAnimateLengthList(aAttrEnum);
  return NS_OK;
}

void
SVGAnimatedLengthList::ClearAnimValue(nsSVGElement *aElement,
                                      uint32_t aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeTo(mBaseVal);
  }
  mAnimVal = nullptr;
  aElement->DidAnimateLengthList(aAttrEnum);
}

nsISMILAttr*
SVGAnimatedLengthList::ToSMILAttr(nsSVGElement *aSVGElement,
                                  uint8_t aAttrEnum,
                                  uint8_t aAxis,
                                  bool aCanZeroPadList)
{
  return new SMILAnimatedLengthList(this, aSVGElement, aAttrEnum, aAxis, aCanZeroPadList);
}

nsresult
SVGAnimatedLengthList::
  SMILAnimatedLengthList::ValueFromString(const nsAString& aStr,
                               const dom::SVGAnimationElement* ,
                               nsSMILValue& aValue,
                               bool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SVGLengthListSMILType::sSingleton);
  SVGLengthListAndInfo *llai = static_cast<SVGLengthListAndInfo*>(val.mU.mPtr);
  nsresult rv = llai->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    llai->SetInfo(mElement, mAxis, mCanZeroPadList);
    aValue.Swap(val);

    
    
    
    
    
    
    
    
    
    
    
    
    

    aPreventCachingOfSandwich = false;
    for (uint32_t i = 0; i < llai->Length(); ++i) {
      uint8_t unit = (*llai)[i].GetUnit();
      if (unit == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE ||
          unit == nsIDOMSVGLength::SVG_LENGTHTYPE_EMS ||
          unit == nsIDOMSVGLength::SVG_LENGTHTYPE_EXS) {
        aPreventCachingOfSandwich = true;
        break;
      }
    }
  }
  return rv;
}

nsSMILValue
SVGAnimatedLengthList::SMILAnimatedLengthList::GetBaseValue() const
{
  
  
  
  nsSMILValue val;

  nsSMILValue tmp(&SVGLengthListSMILType::sSingleton);
  SVGLengthListAndInfo *llai = static_cast<SVGLengthListAndInfo*>(tmp.mU.mPtr);
  nsresult rv = llai->CopyFrom(mVal->mBaseVal);
  if (NS_SUCCEEDED(rv)) {
    llai->SetInfo(mElement, mAxis, mCanZeroPadList);
    val.Swap(tmp);
  }
  return val;
}

nsresult
SVGAnimatedLengthList::SMILAnimatedLengthList::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGLengthListSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGLengthListSMILType::sSingleton) {
    mVal->SetAnimValue(*static_cast<SVGLengthListAndInfo*>(aValue.mU.mPtr),
                       mElement,
                       mAttrEnum);
  }
  return NS_OK;
}

void
SVGAnimatedLengthList::SMILAnimatedLengthList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement, mAttrEnum);
  }
}

} 
