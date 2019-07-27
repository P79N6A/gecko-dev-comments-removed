





#include "SVGAnimatedPointList.h"
#include "DOMSVGPointList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSMILValue.h"
#include "SVGPointListSMILType.h"



namespace mozilla {

nsresult
SVGAnimatedPointList::SetBaseValueString(const nsAString& aValue)
{
  SVGPointList newBaseValue;

  
  
  

  nsresult rv = newBaseValue.SetValueFromString(aValue);

  
  
  
  
  

  DOMSVGPointList *baseValWrapper =
    DOMSVGPointList::GetDOMWrapperIfExists(GetBaseValKey());
  if (baseValWrapper) {
    baseValWrapper->InternalListWillChangeTo(newBaseValue);
  }

  DOMSVGPointList* animValWrapper = nullptr;
  if (!IsAnimating()) {  
    animValWrapper = DOMSVGPointList::GetDOMWrapperIfExists(GetAnimValKey());
    if (animValWrapper) {
      animValWrapper->InternalListWillChangeTo(newBaseValue);
    }
  }

  

  
  
  

  nsresult rv2 = mBaseVal.CopyFrom(newBaseValue);
  if (NS_FAILED(rv2)) {
    
    
    if (baseValWrapper) {
      baseValWrapper->InternalListWillChangeTo(mBaseVal);
    }
    if (animValWrapper) {
      animValWrapper->InternalListWillChangeTo(mBaseVal);
    }
    return rv2;
  }
  return rv;
}

void
SVGAnimatedPointList::ClearBaseValue()
{
  

  DOMSVGPointList *baseValWrapper =
    DOMSVGPointList::GetDOMWrapperIfExists(GetBaseValKey());
  if (baseValWrapper) {
    baseValWrapper->InternalListWillChangeTo(SVGPointList());
  }

  if (!IsAnimating()) { 
    DOMSVGPointList *animValWrapper =
      DOMSVGPointList::GetDOMWrapperIfExists(GetAnimValKey());
    if (animValWrapper) {
      animValWrapper->InternalListWillChangeTo(SVGPointList());
    }
  }

  mBaseVal.Clear();
  
}

nsresult
SVGAnimatedPointList::SetAnimValue(const SVGPointList& aNewAnimValue,
                                   nsSVGElement *aElement)
{
  
  
  
  
  
  
  
  
  
  
  
  

  

  DOMSVGPointList *domWrapper =
    DOMSVGPointList::GetDOMWrapperIfExists(GetAnimValKey());
  if (domWrapper) {
    domWrapper->InternalListWillChangeTo(aNewAnimValue);
  }
  if (!mAnimVal) {
    mAnimVal = new SVGPointList();
  }
  nsresult rv = mAnimVal->CopyFrom(aNewAnimValue);
  if (NS_FAILED(rv)) {
    
    
    ClearAnimValue(aElement);
    return rv;
  }
  aElement->DidAnimatePointList();
  return NS_OK;
}

void
SVGAnimatedPointList::ClearAnimValue(nsSVGElement *aElement)
{
  

  DOMSVGPointList *domWrapper =
    DOMSVGPointList::GetDOMWrapperIfExists(GetAnimValKey());
  if (domWrapper) {
    
    
    
    domWrapper->InternalListWillChangeTo(mBaseVal);
  }
  mAnimVal = nullptr;
  aElement->DidAnimatePointList();
}

nsISMILAttr*
SVGAnimatedPointList::ToSMILAttr(nsSVGElement *aElement)
{
  return new SMILAnimatedPointList(this, aElement);
}

nsresult
SVGAnimatedPointList::
  SMILAnimatedPointList::ValueFromString(const nsAString& aStr,
                               const dom::SVGAnimationElement* ,
                               nsSMILValue& aValue,
                               bool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SVGPointListSMILType::sSingleton);
  SVGPointListAndInfo *list = static_cast<SVGPointListAndInfo*>(val.mU.mPtr);
  nsresult rv = list->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    list->SetInfo(mElement);
    aValue.Swap(val);
  }
  aPreventCachingOfSandwich = false;
  return rv;
}

nsSMILValue
SVGAnimatedPointList::SMILAnimatedPointList::GetBaseValue() const
{
  
  
  
  nsSMILValue val;

  nsSMILValue tmp(&SVGPointListSMILType::sSingleton);
  SVGPointListAndInfo *list = static_cast<SVGPointListAndInfo*>(tmp.mU.mPtr);
  nsresult rv = list->CopyFrom(mVal->mBaseVal);
  if (NS_SUCCEEDED(rv)) {
    list->SetInfo(mElement);
    val.Swap(tmp);
  }
  return val;
}

nsresult
SVGAnimatedPointList::SMILAnimatedPointList::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGPointListSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGPointListSMILType::sSingleton) {
    mVal->SetAnimValue(*static_cast<SVGPointListAndInfo*>(aValue.mU.mPtr),
                       mElement);
  }
  return NS_OK;
}

void
SVGAnimatedPointList::SMILAnimatedPointList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement);
  }
}

} 
