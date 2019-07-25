



































#include "SVGAnimatedPathSegList.h"
#include "DOMSVGPathSegList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGPathSegListSMILType.h"
#endif 



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

  DOMSVGPathSegList* animValWrapper = nsnull;
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
  mAnimVal = nsnull;
  aElement->DidAnimatePathSegList();
}

#ifdef MOZ_SMIL
nsISMILAttr*
SVGAnimatedPathSegList::ToSMILAttr(nsSVGElement *aElement)
{
  return new SMILAnimatedPathSegList(this, aElement);
}

nsresult
SVGAnimatedPathSegList::
  SMILAnimatedPathSegList::ValueFromString(const nsAString& aStr,
                               const nsISMILAnimationElement* ,
                               nsSMILValue& aValue,
                               PRBool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SVGPathSegListSMILType::sSingleton);
  SVGPathDataAndOwner *list = static_cast<SVGPathDataAndOwner*>(val.mU.mPtr);
  nsresult rv = list->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    list->SetElement(mElement);
    aValue.Swap(val);
  }
  aPreventCachingOfSandwich = PR_FALSE;
  return rv;
}

nsSMILValue
SVGAnimatedPathSegList::SMILAnimatedPathSegList::GetBaseValue() const
{
  
  
  
  nsSMILValue val;

  nsSMILValue tmp(&SVGPathSegListSMILType::sSingleton);
  SVGPathDataAndOwner *list = static_cast<SVGPathDataAndOwner*>(tmp.mU.mPtr);
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
  NS_ASSERTION(aValue.mType == &SVGPathSegListSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGPathSegListSMILType::sSingleton) {
    mVal->SetAnimValue(*static_cast<SVGPathDataAndOwner*>(aValue.mU.mPtr),
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
#endif 

} 
