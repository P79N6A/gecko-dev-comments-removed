



































#include "SVGAnimatedLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "nsSVGElement.h"
#include "nsSVGAttrTearoffTable.h"
#ifdef MOZ_SMIL
#include "nsSMILValue.h"
#include "SVGLengthListSMILType.h"
#endif 

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
SVGAnimatedLengthList::ClearBaseValue(PRUint32 aAttrEnum)
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
                                    PRUint32 aAttrEnum)
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
                                      PRUint32 aAttrEnum)
{
  DOMSVGAnimatedLengthList *domWrapper =
    DOMSVGAnimatedLengthList::GetDOMWrapperIfExists(this);
  if (domWrapper) {
    
    
    
    
    
    domWrapper->InternalAnimValListWillChangeTo(mBaseVal);
  }
  mAnimVal = nsnull;
  aElement->DidAnimateLengthList(aAttrEnum);
}

#ifdef MOZ_SMIL
nsISMILAttr*
SVGAnimatedLengthList::ToSMILAttr(nsSVGElement *aSVGElement,
                                  PRUint8 aAttrEnum,
                                  PRUint8 aAxis,
                                  bool aCanZeroPadList)
{
  return new SMILAnimatedLengthList(this, aSVGElement, aAttrEnum, aAxis, aCanZeroPadList);
}

nsresult
SVGAnimatedLengthList::
  SMILAnimatedLengthList::ValueFromString(const nsAString& aStr,
                               const nsISMILAnimationElement* ,
                               nsSMILValue& aValue,
                               bool& aPreventCachingOfSandwich) const
{
  nsSMILValue val(&SVGLengthListSMILType::sSingleton);
  SVGLengthListAndInfo *llai = static_cast<SVGLengthListAndInfo*>(val.mU.mPtr);
  nsresult rv = llai->SetValueFromString(aStr);
  if (NS_SUCCEEDED(rv)) {
    llai->SetInfo(mElement, mAxis, mCanZeroPadList);
    aValue.Swap(val);

    
    
    
    
    
    
    
    
    
    
    
    
    

    aPreventCachingOfSandwich = PR_FALSE;
    for (PRUint32 i = 0; i < llai->Length(); ++i) {
      PRUint8 unit = (*llai)[i].GetUnit();
      if (unit == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE ||
          unit == nsIDOMSVGLength::SVG_LENGTHTYPE_EMS ||
          unit == nsIDOMSVGLength::SVG_LENGTHTYPE_EXS) {
        aPreventCachingOfSandwich = PR_TRUE;
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
#endif 

} 
