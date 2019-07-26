




#ifndef __NS_SVGINTEGERPAIR_H__
#define __NS_SVGINTEGERPAIR_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedInteger.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsISMILAnimationElement;
class nsSMILValue;

class nsSVGIntegerPair
{

public:
  enum PairIndex {
    eFirst,
    eSecond
  };

  void Init(uint8_t aAttrEnum = 0xff, int32_t aValue1 = 0, int32_t aValue2 = 0) {
    mAnimVal[0] = mBaseVal[0] = aValue1;
    mAnimVal[1] = mBaseVal[1] = aValue2;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue) const;

  void SetBaseValue(int32_t aValue, PairIndex aIndex, nsSVGElement *aSVGElement);
  void SetBaseValues(int32_t aValue1, int32_t aValue2, nsSVGElement *aSVGElement);
  int32_t GetBaseValue(PairIndex aIndex) const
    { return mBaseVal[aIndex == eFirst ? 0 : 1]; }
  void SetAnimValue(const int32_t aValue[2], nsSVGElement *aSVGElement);
  int32_t GetAnimValue(PairIndex aIndex) const
    { return mAnimVal[aIndex == eFirst ? 0 : 1]; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                PairIndex aIndex,
                                nsSVGElement* aSVGElement);
  already_AddRefed<nsIDOMSVGAnimatedInteger>
    ToDOMAnimatedInteger(PairIndex aIndex,
                         nsSVGElement* aSVGElement);
   
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  int32_t mAnimVal[2];
  int32_t mBaseVal[2];
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedInteger MOZ_FINAL : public nsIDOMSVGAnimatedInteger
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedInteger)

    DOMAnimatedInteger(nsSVGIntegerPair* aVal, PairIndex aIndex, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement), mIndex(aIndex) {}
    virtual ~DOMAnimatedInteger();

    nsSVGIntegerPair* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    PairIndex mIndex; 

    NS_IMETHOD GetBaseVal(int32_t* aResult)
      { *aResult = mVal->GetBaseValue(mIndex); return NS_OK; }
    NS_IMETHOD SetBaseVal(int32_t aValue)
      {
        mVal->SetBaseValue(aValue, mIndex, mSVGElement);
        return NS_OK;
      }

    
    
    NS_IMETHOD GetAnimVal(int32_t* aResult)
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
