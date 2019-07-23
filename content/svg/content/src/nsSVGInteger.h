



































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
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);

  void SetBaseValue(PRInt32 aValue, nsSVGElement *aSVGElement, PRBool aDoSetAttr);
  PRInt32 GetBaseValue() const
    { return mBaseVal; }
  PRInt32 GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedInteger(nsIDOMSVGAnimatedInteger **aResult,
                                nsSVGElement* aSVGElement);

private:

  PRInt32 mAnimVal;
  PRInt32 mBaseVal;
  PRUint8 mAttrEnum; 

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
      { *aResult = mVal->GetAnimValue(); return NS_OK; }

  };

};
#endif 
