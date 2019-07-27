




#ifndef __NS_SVGINTEGER_H__
#define __NS_SVGINTEGER_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "SVGAnimatedInteger.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"

class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

class nsSVGInteger
{

public:
  void Init(uint8_t aAttrEnum = 0xff, int32_t aValue = 0) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(int32_t aValue, nsSVGElement *aSVGElement);
  int32_t GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(int aValue, nsSVGElement *aSVGElement);
  int GetAnimValue() const
    { return mAnimVal; }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  already_AddRefed<mozilla::dom::SVGAnimatedInteger>
    ToDOMAnimatedInteger(nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
  
private:

  int32_t mAnimVal;
  int32_t mBaseVal;
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedInteger MOZ_FINAL : public mozilla::dom::SVGAnimatedInteger
  {
    DOMAnimatedInteger(nsSVGInteger* aVal, nsSVGElement* aSVGElement)
      : mozilla::dom::SVGAnimatedInteger(aSVGElement)
      , mVal(aVal)
    {}
    virtual ~DOMAnimatedInteger();

    nsSVGInteger* mVal; 

    virtual int32_t BaseVal() MOZ_OVERRIDE
    {
      return mVal->GetBaseValue();
    }
    virtual void SetBaseVal(int32_t aValue) MOZ_OVERRIDE
    {
      mVal->SetBaseValue(aValue, mSVGElement);
    }

    
    
    virtual int32_t AnimVal() MOZ_OVERRIDE
    {
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue();
    }
  };

  struct SMILInteger : public nsISMILAttr
  {
  public:
    SMILInteger(nsSVGInteger* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGInteger* mVal;
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
