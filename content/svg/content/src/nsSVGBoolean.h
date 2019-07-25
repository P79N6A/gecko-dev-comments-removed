



































#ifndef __NS_SVGBOOLEAN_H__
#define __NS_SVGBOOLEAN_H__

#include "nsIDOMSVGAnimatedBoolean.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGBoolean
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, bool aValue = false) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = PR_FALSE;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedBoolean(nsIDOMSVGAnimatedBoolean **aResult,
                                nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:

  bool mAnimVal;
  bool mBaseVal;
  bool mIsAnimated;
  PRUint8 mAttrEnum; 

public:
  struct DOMAnimatedBoolean : public nsIDOMSVGAnimatedBoolean
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedBoolean)

    DOMAnimatedBoolean(nsSVGBoolean* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGBoolean* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(bool* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(bool aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

    
    
    NS_IMETHOD GetAnimVal(bool* aResult)
    {
#ifdef MOZ_SMIL
      mSVGElement->FlushAnimations();
#endif
      *aResult = mVal->GetAnimValue();
      return NS_OK;
    }
  };

#ifdef MOZ_SMIL
  struct SMILBool : public nsISMILAttr
  {
  public:
    SMILBool(nsSVGBoolean* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    
    
    
    nsSVGBoolean* mVal;
    nsSVGElement* mSVGElement;
    
    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};
#endif 
