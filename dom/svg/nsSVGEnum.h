




#ifndef __NS_SVGENUM_H__
#define __NS_SVGENUM_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/SVGAnimatedEnumeration.h"

class nsIAtom;
class nsSMILValue;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}

typedef uint8_t nsSVGEnumValue;

struct nsSVGEnumMapping {
  nsIAtom **mKey;
  nsSVGEnumValue mVal;
};

class nsSVGEnum
{
public:
  void Init(uint8_t aAttrEnum, uint16_t aValue) {
    mAnimVal = mBaseVal = uint8_t(aValue);
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueAtom(const nsIAtom* aValue, nsSVGElement *aSVGElement);
  nsIAtom* GetBaseValueAtom(nsSVGElement *aSVGElement);
  nsresult SetBaseValue(uint16_t aValue,
                        nsSVGElement *aSVGElement);
  uint16_t GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(uint16_t aValue, nsSVGElement *aSVGElement);
  uint16_t GetAnimValue() const
    { return mAnimVal; }
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  already_AddRefed<mozilla::dom::SVGAnimatedEnumeration>
  ToDOMAnimatedEnum(nsSVGElement* aSVGElement);

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:
  nsSVGEnumValue mAnimVal;
  nsSVGEnumValue mBaseVal;
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

  nsSVGEnumMapping *GetMapping(nsSVGElement *aSVGElement);

public:
  struct DOMAnimatedEnum MOZ_FINAL : public mozilla::dom::SVGAnimatedEnumeration
  {
    DOMAnimatedEnum(nsSVGEnum* aVal, nsSVGElement *aSVGElement)
      : mozilla::dom::SVGAnimatedEnumeration(aSVGElement)
      , mVal(aVal)
    {}
    virtual ~DOMAnimatedEnum();

    nsSVGEnum *mVal; 

    using mozilla::dom::SVGAnimatedEnumeration::SetBaseVal;
    virtual uint16_t BaseVal() MOZ_OVERRIDE
    {
      return mVal->GetBaseValue();
    }
    virtual void SetBaseVal(uint16_t aBaseVal,
                            mozilla::ErrorResult& aRv) MOZ_OVERRIDE
    {
      aRv = mVal->SetBaseValue(aBaseVal, mSVGElement);
    }
    virtual uint16_t AnimVal() MOZ_OVERRIDE
    {
      
      
      
      mSVGElement->FlushAnimations();
      return mVal->GetAnimValue();
    }
  };

  struct SMILEnum : public nsISMILAttr
  {
  public:
    SMILEnum(nsSVGEnum* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGEnum* mVal;
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
