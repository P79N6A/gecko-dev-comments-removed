




#ifndef __NS_SVGNUMBER2_H__
#define __NS_SVGNUMBER2_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "nsMathUtils.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimatedNumber.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

class nsSVGNumber2
{

public:
  void Init(uint8_t aAttrEnum = 0xff, float aValue = 0) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(float aValue, nsSVGElement *aSVGElement);
  float GetBaseValue() const
    { return mBaseVal; }
  void SetAnimValue(float aValue, nsSVGElement *aSVGElement);
  float GetAnimValue() const
    { return mAnimVal; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  already_AddRefed<mozilla::dom::SVGAnimatedNumber>
  ToDOMAnimatedNumber(nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  float mAnimVal;
  float mBaseVal;
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedNumber MOZ_FINAL : public mozilla::dom::SVGAnimatedNumber
  {
    DOMAnimatedNumber(nsSVGNumber2* aVal, nsSVGElement* aSVGElement)
      : mozilla::dom::SVGAnimatedNumber(aSVGElement)
      , mVal(aVal)
    {}
    virtual ~DOMAnimatedNumber();

    nsSVGNumber2* mVal; 

    virtual float BaseVal() MOZ_OVERRIDE
    {
      return mVal->GetBaseValue();
    }
    virtual void SetBaseVal(float aValue) MOZ_OVERRIDE
    {
      MOZ_ASSERT(NS_finite(aValue));
      mVal->SetBaseValue(aValue, mSVGElement);
    }

    
    
    virtual float AnimVal() MOZ_OVERRIDE
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue();
    }
  };

  struct SMILNumber : public nsISMILAttr
  {
  public:
    SMILNumber(nsSVGNumber2* aVal, nsSVGElement* aSVGElement)
    : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGNumber2* mVal;
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
