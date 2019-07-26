




#ifndef __NS_SVGBOOLEAN_H__
#define __NS_SVGBOOLEAN_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAnimatedBoolean.h"
#include "nsISMILAttr.h"
#include "nsISupportsImpl.h"
#include "nsSVGElement.h"

class nsISMILAnimationElement;
class nsSMILValue;

class nsSVGBoolean
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, bool aValue = false) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
  }

  nsresult SetBaseValueAtom(const nsIAtom* aValue, nsSVGElement *aSVGElement);
  nsIAtom* GetBaseValueAtom() const;

  void SetBaseValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(bool aValue, nsSVGElement *aSVGElement);
  bool GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedBoolean(nsIDOMSVGAnimatedBoolean **aResult,
                                nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:

  bool mAnimVal;
  bool mBaseVal;
  bool mIsAnimated;
  PRUint8 mAttrEnum; 

public:
  struct DOMAnimatedBoolean : public nsIDOMSVGAnimatedBoolean
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedBoolean)

    DOMAnimatedBoolean(nsSVGBoolean* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGBoolean* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(bool* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(bool aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

    
    
    NS_IMETHOD GetAnimVal(bool* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->GetAnimValue();
      return NS_OK;
    }
  };

  struct SMILBool : public nsISMILAttr
  {
  public:
    SMILBool(nsSVGBoolean* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    
    
    
    nsSVGBoolean* mVal;
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
