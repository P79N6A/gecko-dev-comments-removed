




#ifndef __NS_SVGNUMBERPAIR_H__
#define __NS_SVGNUMBERPAIR_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsISMILAttr.h"
#include "nsMathUtils.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

class nsSVGNumberPair
{

public:
  enum PairIndex {
    eFirst,
    eSecond
  };

  void Init(uint8_t aAttrEnum = 0xff, float aValue1 = 0, float aValue2 = 0) {
    mAnimVal[0] = mBaseVal[0] = aValue1;
    mAnimVal[1] = mBaseVal[1] = aValue2;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue) const;

  void SetBaseValue(float aValue, PairIndex aIndex, nsSVGElement *aSVGElement);
  void SetBaseValues(float aValue1, float aValue2, nsSVGElement *aSVGElement);
  float GetBaseValue(PairIndex aIndex) const
    { return mBaseVal[aIndex == eFirst ? 0 : 1]; }
  void SetAnimValue(const float aValue[2], nsSVGElement *aSVGElement);
  float GetAnimValue(PairIndex aIndex) const
    { return mAnimVal[aIndex == eFirst ? 0 : 1]; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber** aResult,
                               PairIndex aIndex,
                               nsSVGElement* aSVGElement);
  already_AddRefed<nsIDOMSVGAnimatedNumber>
    ToDOMAnimatedNumber(PairIndex aIndex,
                        nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  float mAnimVal[2];
  float mBaseVal[2];
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedNumber MOZ_FINAL : public nsIDOMSVGAnimatedNumber
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedNumber)

    DOMAnimatedNumber(nsSVGNumberPair* aVal, PairIndex aIndex, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement), mIndex(aIndex) {}
    virtual ~DOMAnimatedNumber();

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
        mVal->SetBaseValue(aValue, mIndex, mSVGElement);
        return NS_OK;
      }

    
    
    NS_IMETHOD GetAnimVal(float* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->GetAnimValue(mIndex);
      return NS_OK;
    }
  };

  struct SMILNumberPair : public nsISMILAttr
  {
  public:
    SMILNumberPair(nsSVGNumberPair* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGNumberPair* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const mozilla::dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
