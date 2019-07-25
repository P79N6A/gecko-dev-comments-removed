



































#ifndef __NS_SVGNUMBERPAIR_H__
#define __NS_SVGNUMBERPAIR_H__

#include "nsIDOMSVGNumber.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"
#include "nsMathUtils.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
class nsSMILValue;
class nsISMILType;
#endif 

class nsSVGNumberPair
{

public:
  enum PairIndex {
    eFirst,
    eSecond
  };

  void Init(PRUint8 aAttrEnum = 0xff, float aValue1 = 0, float aValue2 = 0) {
    mAnimVal[0] = mBaseVal[0] = aValue1;
    mAnimVal[1] = mBaseVal[1] = aValue2;
    mAttrEnum = aAttrEnum;
    mIsAnimated = PR_FALSE;
    mIsBaseSet = PR_FALSE;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(float aValue, PairIndex aIndex, nsSVGElement *aSVGElement, bool aDoSetAttr);
  void SetBaseValues(float aValue1, float aValue2, nsSVGElement *aSVGElement, bool aDoSetAttr);
  float GetBaseValue(PairIndex aIndex) const
    { return mBaseVal[aIndex == eFirst ? 0 : 1]; }
  void SetAnimValue(const float aValue[2], nsSVGElement *aSVGElement);
  float GetAnimValue(PairIndex aIndex) const
    { return mAnimVal[aIndex == eFirst ? 0 : 1]; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                               PairIndex aIndex,
                               nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:

  float mAnimVal[2];
  float mBaseVal[2];
  PRUint8 mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedNumber : public nsIDOMSVGAnimatedNumber
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedNumber)

    DOMAnimatedNumber(nsSVGNumberPair* aVal, PairIndex aIndex, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement), mIndex(aIndex) {}

    nsSVGNumberPair* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    PairIndex mIndex; 

    NS_IMETHOD GetBaseVal(float* aResult)
      { *aResult = mVal->GetBaseValue(mIndex); return NS_OK; }
    NS_IMETHOD SetBaseVal(float aValue)
      {
        if (!NS_finite(aValue)) {
          return NS_ERROR_ILLEGAL_VALUE;
        }
        mVal->SetBaseValue(aValue, mIndex, mSVGElement, PR_TRUE);
        return NS_OK;
      }

    
    
    NS_IMETHOD GetAnimVal(float* aResult)
    {
#ifdef MOZ_SMIL
      mSVGElement->FlushAnimations();
#endif
      *aResult = mVal->GetAnimValue(mIndex);
      return NS_OK;
    }
  };

#ifdef MOZ_SMIL
  struct SMILNumberPair : public nsISMILAttr
  {
  public:
    SMILNumberPair(nsSVGNumberPair* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGNumberPair* mVal;
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
