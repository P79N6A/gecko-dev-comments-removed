



































#ifndef __NS_SVGBOOLEAN_H__
#define __NS_SVGBOOLEAN_H__

#include "nsIDOMSVGAnimatedBoolean.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGBoolean
{

public:
  void Init(PRUint8 aAttrEnum = 0xff, PRBool aValue = PR_FALSE) {
    mAnimVal = mBaseVal = aValue;
    mAttrEnum = aAttrEnum;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(PRBool aValue, nsSVGElement *aSVGElement);
  PRBool GetBaseValue() const
    { return mBaseVal; }
  PRBool GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedBoolean(nsIDOMSVGAnimatedBoolean **aResult,
                                nsSVGElement* aSVGElement);

private:

  PRPackedBool mAnimVal;
  PRPackedBool mBaseVal;
  PRUint8 mAttrEnum; 

  struct DOMAnimatedBoolean : public nsIDOMSVGAnimatedBoolean
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedBoolean)

    DOMAnimatedBoolean(nsSVGBoolean* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGBoolean* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(PRBool* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(PRBool aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }
    NS_IMETHOD GetAnimVal(PRBool* aResult)
      { *aResult = mVal->GetAnimValue(); return NS_OK; }

  };

};
#endif 
