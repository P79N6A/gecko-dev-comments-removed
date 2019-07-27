




#include "SVGAnimatedPathSegList.h"
#include "DOMSVGPathSegList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#include "nsSMILValue.h"
#include "SVGPathSegListSMILType.h"



namespace mozilla {

nsresult
SVGAnimatedPathSegList::SetBaseValueString(const nsAString& aValue)
{
  SVGPathData newBaseValue;

  
  
  

  nsresult rv = newBaseValue.SetValueFromString(aValue);

  
  
  
  
  

  DOMSVGPathSegList *baseValWrapper =
    DOMSVGPathSegList::GetDOMWrapperIfExists(GetBaseValKey());
  if (baseValWrapper) {
    baseValWrapper->InternalListWillChangeTo(newBaseValue);
  }

  DOMSVGPathSegList* animValWrapper = nullptr;
  if (!IsAnimating()) {  
    animValWrapper = DOMSVGPathSegList::GetDOMWrapperIfExists(GetAnimValKey());
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
SVGAnimatedPathSegList::ClearBaseValue()
{
  

  DOMSVGPathSegList *baseValWrapper =
    DOMSVGPathSegList::GetDOMWrapperIfExists(GetBaseValKey());
  if (baseValWrapper) {
    baseValWrapper->InternalListWillChangeTo(SVGPathData());
  }

  if (!IsAnimating()) { 
    DOMSVGPathSegList *animValWrapper =
      DOMSVGPathSegList::GetDOMWrapperIfExists(GetAnimValKey());
    if (animValWrapper) {
      animValWrapper->InternalListWillChangeTo(SVGPathData());
    }
  }

  mBaseVal.Clear();
  
}

nsresult
SVGAnimatedPathSegList::SetAnimValue(const SVGPathData& aNewAnimValue,
                                     nsSVGElement *aElement)
{
  
  
  
  
  
  
  
  
  
  

  

  DOMSVGPathSegList *domWrapper =
    DOMSVGPathSegList::GetDOMWrapperIfExists(GetAnimValKey());
  if (domWrapper) {
    domWrapper->InternalListWillChangeTo(aNewAnimValue);
  }
  if (!mAnimVal) {
    mAnimVal = new SVGPathData();
  }
  nsresult rv = mAnimVal->CopyFrom(aNewAnimValue);
  if (NS_FAILED(rv)) {
    
    
    ClearAnimValue(aElement);
  }
  aElement->DidAnimatePathSegList();
  return rv;
}

void
SVGAnimatedPathSegList::ClearAnimValue(nsSVGElement *aElement)
{
  

  DOMSVGPathSegList *domWrapper =
    DOMSVGPathSegList::GetDOMWrapperIfExists(GetAnimValKey());
  if (domWrapper) {
    
    
    
    domWrapper->InternalListWillChangeTo(mBaseVal);
  }
  mAnimVal = nullptr;
  aElement->DidAnimatePathSegList();
}

nsISMILAttr*
SVGAnimatedPathSegList::ToSMILAttr(nsSVGElement *aElement)
{
  return new SMILAnimatedPathSegList(this, aElement);
}

nsresult
SVGAnimatedPathSegList::
  SMILAnimatedPathSegList::ValueFromString(const nsAString& aStr,
                               const dom::SVGAnimationElement* ,
                               nsSMILValue& aValue,
                               bool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(SVGPathSegListSMILType::Singleton());
  SVGPathDataAndInfo *list = static_cast<SVGPathDataAndInfo*>(val.mU.mPtr);
  nsresult rv = list->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    list->SetElement(mElement);
    aValue.Swap(val);
  }
  aPreventCachingOfSandwich = false;
  return rv;
}

nsSMILValue
SVGAnimatedPathSegList::SMILAnimatedPathSegList::GetBaseValue() const
{
  
  
  
  nsSMILValue val;

  nsSMILValue tmp(SVGPathSegListSMILType::Singleton());
  SVGPathDataAndInfo *list = static_cast<SVGPathDataAndInfo*>(tmp.mU.mPtr);
  nsresult rv = list->CopyFrom(mVal->mBaseVal);
  if (NS_SUCCEEDED(rv)) {
    list->SetElement(mElement);
    val.Swap(tmp);
  }
  return val;
}

nsresult
SVGAnimatedPathSegList::SMILAnimatedPathSegList::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == SVGPathSegListSMILType::Singleton(),
               "Unexpected type to assign animated value");
  if (aValue.mType == SVGPathSegListSMILType::Singleton()) {
    mVal->SetAnimValue(*static_cast<SVGPathDataAndInfo*>(aValue.mU.mPtr),
                       mElement);
  }
  return NS_OK;
}

void
SVGAnimatedPathSegList::SMILAnimatedPathSegList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement);
  }
}

size_t
SVGAnimatedPathSegList::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t total = mBaseVal.SizeOfExcludingThis(aMallocSizeOf);
  if (mAnimVal) {
    mAnimVal->SizeOfIncludingThis(aMallocSizeOf);
  }
  return total;
}

} 
