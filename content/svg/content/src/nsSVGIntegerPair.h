



































#ifndef __NS_SVGINTEGERPAIR_H__
#define __NS_SVGINTEGERPAIR_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedInteger.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"

class nsISMILAnimationElement;
class nsSMILValue;

class nsSVGIntegerPair
{

public:
  enum PairIndex {
    eFirst,
    eSecond
  };

  void Init(PRUint8 aAttrEnum = 0xff, PRInt32 aValue1 = 0, PRInt32 aValue2 = 0) {
    mAnimVal[0] = mBaseVal[0] = aValue1;
    mAnimVal[1] = mBaseVal[1] = aValue2;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue) const;

  void SetBaseValue(PRInt32 aValue, PairIndex aIndex, nsSVGElement *aSVGElement);
  void SetBaseValues(PRInt32 aValue1, PRInt32 aValue2, nsSVGElement *aSVGElement);
  PRInt32 GetBaseValue(PairIndex aIndex) const
    { return mBaseVal[aIndex == eFirst ? 0 : 1]; }
  void SetAnimValue(const PRInt32 aValue[2], nsSVGElement *aSVGElement);
  PRInt32 GetAnimValue(PairIndex aIndex) const
    { return mAnimVal[aIndex == eFirst ? 0 : 1]; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                PairIndex aIndex,
                                nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  PRInt32 mAnimVal[2];
  PRInt32 mBaseVal[2];
  PRUint8 mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedInteger : public nsIDOMSVGAnimatedInteger
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedInteger)

    DOMAnimatedInteger(nsSVGIntegerPair* aVal, PairIndex aIndex, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement), mIndex(aIndex) {}

    nsSVGIntegerPair* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    PairIndex mIndex; 

    NS_IMETHOD GetBaseVal(PRInt32* aResult)
      { *aResult = mVal->GetBaseValue(mIndex); return NS_OK; }
    NS_IMETHOD SetBaseVal(PRInt32 aValue)
      {
        mVal->SetBaseValue(aValue, mIndex, mSVGElement);
        return NS_OK;
      }

    
    
    NS_IMETHOD GetAnimVal(PRInt32* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->GetAnimValue(mIndex);
      return NS_OK;
    }
  };

  struct SMILIntegerPair : public nsISMILAttr
  {
  public:
    SMILIntegerPair(nsSVGIntegerPair* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGIntegerPair* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
