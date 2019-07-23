



































#ifndef __NS_SVGINTEGER_H__
#define __NS_SVGINTEGER_H__

#include "nsIDOMSVGAnimatedInteger.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGInteger
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, PRInt32 aValue = 0) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
    mIsAnimated = PR_FALSE;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(PRInt32 aValue, nsSVGElement *aSVGElement, PRBool aDoSetAttr);
  PRInt32 GetBaseValue() const
    { return mBaseVal; }

  void SetAnimValue(int aValue, nsSVGElement *aSVGElement);
  int GetAnimValue(nsSVGElement *aSVGElement) const
  {
  #ifdef MOZ_SMIL
    aSVGElement->FlushAnimations();
  #endif
    return mAnimVal;
  }
  
  nsresult ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 
  
private:

  PRInt32 mAnimVal;
  PRInt32 mBaseVal;
  PRUint8 mAttrEnum; 
  PRPackedBool mIsAnimated;

  struct DOMAnimatedInteger : public nsIDOMSVGAnimatedInteger
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedInteger)

    DOMAnimatedInteger(nsSVGInteger* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGInteger* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(PRInt32* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(PRInt32 aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); return NS_OK; }
    NS_IMETHOD GetAnimVal(PRInt32* aResult)
      { *aResult = mVal->GetAnimValue(mSVGElement); return NS_OK; }

  };

#ifdef MOZ_SMIL
  struct SMILInteger : public nsISMILAttr
  {
  public:
    SMILInteger(nsSVGInteger* aVal, nsSVGElement* aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGInteger* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};

#endif 
