



































#ifndef __NS_SVGNUMBER2_H__
#define __NS_SVGNUMBER2_H__

#include "nsIDOMSVGNumber.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
class nsSMILValue;
class nsISMILType;
#endif 

class nsSVGNumber2
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, float aValue = 0) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = PR_FALSE;
    mIsBaseSet = PR_FALSE;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(float aValue, nsSVGElement *aSVGElement, PRBool aDoSetAttr);
  float GetBaseValue() const
    { return mBaseVal; }
  void SetAnimValue(float aValue, nsSVGElement *aSVGElement);
  float GetAnimValue() const
    { return mAnimVal; }

  
  
  
  
  
  PRBool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                               nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:

  float mAnimVal;
  float mBaseVal;
  PRUint8 mAttrEnum; 
  PRPackedBool mIsAnimated;
  PRPackedBool mIsBaseSet;

public:
  struct DOMAnimatedNumber : public nsIDOMSVGAnimatedNumber
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedNumber)

    DOMAnimatedNumber(nsSVGNumber2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGNumber2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(float* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(float aValue)
      {
        NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
        mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE);
        return NS_OK;
      }

    
    
    NS_IMETHOD GetAnimVal(float* aResult)
    {
#ifdef MOZ_SMIL
      mSVGElement->FlushAnimations();
#endif
      *aResult = mVal->GetAnimValue();
      return NS_OK;
    }
  };

#ifdef MOZ_SMIL
  struct SMILNumber : public nsISMILAttr
  {
  public:
    SMILNumber(nsSVGNumber2* aVal, nsSVGElement* aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGNumber2* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     PRBool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};

#endif 
