



































#ifndef __NS_SVGNUMBER2_H__
#define __NS_SVGNUMBER2_H__

#include "nsIDOMSVGNumber.h"
#include "nsIDOMSVGAnimatedNumber.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGNumber2
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, float aValue = 0) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(float aValue, nsSVGElement *aSVGElement, PRBool aDoSetAttr);
  float GetBaseValue()
    { return mBaseVal; }
  float GetAnimValue()
    { return mAnimVal; }

  nsresult ToDOMAnimatedNumber(nsIDOMSVGAnimatedNumber **aResult,
                               nsSVGElement* aSVGElement);

private:

  float mAnimVal;
  float mBaseVal;
  PRUint8 mAttrEnum; 

  struct DOMAnimatedNumber : public nsIDOMSVGAnimatedNumber
  {
    NS_DECL_ISUPPORTS

    DOMAnimatedNumber(nsSVGNumber2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGNumber2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(float* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(float aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); return NS_OK; }
    NS_IMETHOD GetAnimVal(float* aResult)
      { *aResult = mVal->GetAnimValue(); return NS_OK; }

  };

};
#endif 
