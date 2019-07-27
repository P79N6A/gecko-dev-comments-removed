




#ifndef __NS_SVGINTEGERPAIR_H__
#define __NS_SVGINTEGERPAIR_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimatedInteger.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

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

  already_AddRefed<mozilla::dom::SVGAnimatedInteger>
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
  struct DOMAnimatedInteger MOZ_FINAL : public mozilla::dom::SVGAnimatedInteger
  {
    DOMAnimatedInteger(nsSVGIntegerPair* aVal, PairIndex aIndex,
                       nsSVGElement* aSVGElement)
      : mozilla::dom::SVGAnimatedInteger(aSVGElement)
      , mVal(aVal)
      , mIndex(aIndex)
    {}
    virtual ~DOMAnimatedInteger();

    nsSVGIntegerPair* mVal; 
    PairIndex mIndex; 

    virtual int32_t BaseVal() MOZ_OVERRIDE
    {
      return mVal->GetBaseValue(mIndex);
    }
    virtual void SetBaseVal(int32_t aValue) MOZ_OVERRIDE
    {
      mVal->SetBaseValue(aValue, mIndex, mSVGElement);
    }

    
    
    virtual int32_t AnimVal() MOZ_OVERRIDE
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue(mIndex);
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
                                     const mozilla::dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};

#endif 
