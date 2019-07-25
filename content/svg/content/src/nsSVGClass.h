



































#ifndef __NS_SVGCLASS_H__
#define __NS_SVGCLASS_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsString.h"
#include "nsDOMError.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 

class nsSVGStylableElement;

class nsSVGClass
{

public:
  void Init() {
    mAnimVal = nsnull;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGStylableElement *aSVGElement,
                    bool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, const nsSVGStylableElement *aSVGElement) const;

  void SetAnimValue(const nsAString& aValue, nsSVGStylableElement *aSVGElement);
  void GetAnimValue(nsAString& aValue, const nsSVGStylableElement *aSVGElement) const;
  bool IsAnimated() const
    { return !!mAnimVal; }

  nsresult ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                               nsSVGStylableElement *aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGStylableElement *aSVGElement);
#endif 

private:

  nsAutoPtr<nsString> mAnimVal;

public:
  struct DOMAnimatedString : public nsIDOMSVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedString)

    DOMAnimatedString(nsSVGClass *aVal, nsSVGStylableElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGClass* mVal; 
    nsRefPtr<nsSVGStylableElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsAString& aResult)
      { mVal->GetBaseValue(aResult, mSVGElement); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString& aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, true); return NS_OK; }

    NS_IMETHOD GetAnimVal(nsAString& aResult);
  };
#ifdef MOZ_SMIL
  struct SMILString : public nsISMILAttr
  {
  public:
    SMILString(nsSVGClass *aVal, nsSVGStylableElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGClass* mVal;
    nsSVGStylableElement* mSVGElement;

    
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
