



































#ifndef __NS_SVGENUM_H__
#define __NS_SVGENUM_H__

#include "nsIDOMSVGAnimatedEnum.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

typedef PRUint8 nsSVGEnumValue;

struct nsSVGEnumMapping {
  nsIAtom **mKey;
  nsSVGEnumValue mVal;
};

class nsSVGEnum
{
public:
  void Init(PRUint8 aAttrEnum, PRUint16 aValue) {
    mAnimVal = mBaseVal = PRUint8(aValue);
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement);
  void GetBaseValueString(nsAString& aValue,
                          nsSVGElement *aSVGElement);

  nsresult SetBaseValue(PRUint16 aValue,
                        nsSVGElement *aSVGElement);
  PRUint16 GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(PRUint16 aValue, nsSVGElement *aSVGElement);
  PRUint16 GetAnimValue() const
    { return mAnimVal; }
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  nsresult ToDOMAnimatedEnum(nsIDOMSVGAnimatedEnumeration **aResult,
                             nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:
  nsSVGEnumValue mAnimVal;
  nsSVGEnumValue mBaseVal;
  PRUint8 mAttrEnum; 
  bool mIsAnimated;
  bool mIsBaseSet;

  nsSVGEnumMapping *GetMapping(nsSVGElement *aSVGElement);

public:
  struct DOMAnimatedEnum : public nsIDOMSVGAnimatedEnumeration
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedEnum)

    DOMAnimatedEnum(nsSVGEnum* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGEnum *mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(PRUint16* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(PRUint16 aValue)
      { return mVal->SetBaseValue(aValue, mSVGElement); }

    
    
    NS_IMETHOD GetAnimVal(PRUint16* aResult)
    {
#ifdef MOZ_SMIL
      mSVGElement->FlushAnimations();
#endif
      *aResult = mVal->GetAnimValue();
      return NS_OK;
    }
  };

#ifdef MOZ_SMIL
  struct SMILEnum : public nsISMILAttr
  {
  public:
    SMILEnum(nsSVGEnum* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGEnum* mVal;
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
