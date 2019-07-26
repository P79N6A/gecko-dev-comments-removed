




#ifndef __NS_SVGINTEGER_H__
#define __NS_SVGINTEGER_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedInteger.h"
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

  already_AddRefed<nsIDOMSVGAnimatedInteger>
    ToDOMAnimatedInteger(nsSVGElement* aSVGElement);
  nsresult ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
  
private:

  int32_t mAnimVal;
  int32_t mBaseVal;
  uint8_t mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

public:
  struct DOMAnimatedInteger MOZ_FINAL : public nsIDOMSVGAnimatedInteger
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedInteger)

    DOMAnimatedInteger(nsSVGInteger* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimatedInteger();

    nsSVGInteger* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(int32_t* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(int32_t aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

    
    
    NS_IMETHOD GetAnimVal(int32_t* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->GetAnimValue();
      return NS_OK;
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
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
