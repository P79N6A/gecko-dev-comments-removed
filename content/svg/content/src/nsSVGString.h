



































#ifndef __NS_SVGSTRING_H__
#define __NS_SVGSTRING_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGString
{

public:
  void Init(PRUint8 aAttrEnum) {
    mAnimVal = nsnull;
    mAttrEnum = aAttrEnum;
    mIsBaseSet = PR_FALSE;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, nsSVGElement *aSVGElement) const
    { aSVGElement->GetStringBaseValue(mAttrEnum, aValue); }

  void SetAnimValue(const nsAString& aValue, nsSVGElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGElement *aSVGElement) const;

  
  
  
  
  
  bool IsExplicitlySet() const
    { return !!mAnimVal || mIsBaseSet; }

  nsresult ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                               nsSVGElement *aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement *aSVGElement);
#endif 

private:

  nsAutoPtr<nsString> mAnimVal;
  PRUint8 mAttrEnum; 
  bool mIsBaseSet;

public:
  struct DOMAnimatedString : public nsIDOMSVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedString)

    DOMAnimatedString(nsSVGString *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGString* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsAString & aResult)
      { mVal->GetBaseValue(aResult, mSVGElement); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString & aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); return NS_OK; }

    NS_IMETHOD GetAnimVal(nsAString & aResult)
    { 
#ifdef MOZ_SMIL
      mSVGElement->FlushAnimations();
#endif
      mVal->GetAnimValue(aResult, mSVGElement); return NS_OK;
    }

  };
#ifdef MOZ_SMIL
  struct SMILString : public nsISMILAttr
  {
  public:
    SMILString(nsSVGString *aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGString* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement *aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};
#endif 
