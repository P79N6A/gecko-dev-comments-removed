



































#include "SVGAnimatedNumberList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGNumberListSMILType.h"
#endif 

namespace mozilla {

nsresult
SVGAnimatedNumberList::SetBaseValueString(const nsAString& aValue)
{
  SVGNumberList newBaseValue;
  nsresult rv = newBaseValue.SetValueFromString(aValue);
  if (NS_FAILED(rv)) {
    return rv;
  }

  DOMSVGAnimatedNumberList *domWrapper =
    DOMSVGAnimatedNumberList::GetDOMWrapperIfExists(this);
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
SVGAnimatedNumberList::ClearBaseValue(PRUint32 aAttrEnum)
{
  DOMSVGAnimatedNumberList *domWrapper =
    DOMSVGAnimatedNumberList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    domWrapper->InternalBaseValListWillChangeTo(SVGNumberList());
  }
  mBaseVal.Clear();
  
}

nsresult
SVGAnimatedNumberList::SetAnimValue(const SVGNumberList& aNewAnimValue,
                                    nsSVGElement *aElement,
                                    PRUint32 aAttrEnum)
{
  DOMSVGAnimatedNumberList *domWrapper =
    DOMSVGAnimatedNumberList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeTo(aNewAnimValue);
  }
  if (!mAnimVal) {
    mAnimVal = new SVGNumberList();
  }
  nsresult rv = mAnimVal->CopyFrom(aNewAnimValue);
  if (NS_FAILED(rv)) {
    
    
    ClearAnimValue(aElement, aAttrEnum);
    return rv;
  }
  aElement->DidAnimateNumberList(aAttrEnum);
  return NS_OK;
}

void
SVGAnimatedNumberList::ClearAnimValue(nsSVGElement *aElement,
                                      PRUint32 aAttrEnum)
{
  DOMSVGAnimatedNumberList *domWrapper =
    DOMSVGAnimatedNumberList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeTo(mBaseVal);
  }
  mAnimVal = nsnull;
  aElement->DidAnimateNumberList(aAttrEnum);
}

#ifdef MOZ_SMIL
nsISMILAttr*
SVGAnimatedNumberList::ToSMILAttr(nsSVGElement *aSVGElement,
                                  PRUint8 aAttrEnum)
{
  return new SMILAnimatedNumberList(this, aSVGElement, aAttrEnum);
}

nsresult
SVGAnimatedNumberList::
  SMILAnimatedNumberList::ValueFromString(const nsAString& aStr,
                               const nsISMILAnimationElement* ,
                               nsSMILValue& aValue,
                               PRBool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SVGNumberListSMILType::sSingleton);
  SVGNumberListAndInfo *nlai = static_cast<SVGNumberListAndInfo*>(val.mU.mPtr);
  nsresult rv = nlai->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    nlai->SetInfo(mElement);
    aValue.Swap(val);
  }
  aPreventCachingOfSandwich = PR_FALSE;
  return rv;
}

nsSMILValue
SVGAnimatedNumberList::SMILAnimatedNumberList::GetBaseValue() const
{
  
  
  
  nsSMILValue val;

  nsSMILValue tmp(&SVGNumberListSMILType::sSingleton);
  SVGNumberListAndInfo *nlai = static_cast<SVGNumberListAndInfo*>(tmp.mU.mPtr);
  nsresult rv = nlai->CopyFrom(mVal->mBaseVal);
  if (NS_SUCCEEDED(rv)) {
    nlai->SetInfo(mElement);
    val.Swap(tmp);
  }
  return val;
}

nsresult
SVGAnimatedNumberList::SMILAnimatedNumberList::SetAnimValue(const nsSMILValue& aValue)
{
  NS_ASSERTION(aValue.mType == &SVGNumberListSMILType::sSingleton,
               "Unexpected type to assign animated value");
  if (aValue.mType == &SVGNumberListSMILType::sSingleton) {
    mVal->SetAnimValue(*static_cast<SVGNumberListAndInfo*>(aValue.mU.mPtr),
                       mElement,
                       mAttrEnum);
  }
  return NS_OK;
}

void
SVGAnimatedNumberList::SMILAnimatedNumberList::ClearAnimValue()
{
  if (mVal->mAnimVal) {
    mVal->ClearAnimValue(mElement, mAttrEnum);
  }
}
#endif 

} 
