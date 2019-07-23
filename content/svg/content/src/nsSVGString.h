



































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
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    PRBool aDoSetAttr);
  void GetBaseValue(nsAString& aValue, nsSVGElement* aSVGElement) const
    { aSVGElement->GetStringBaseValue(mAttrEnum, aValue); }

  void GetAnimValue(nsAString& aValue, const nsSVGElement* aSVGElement) const;

  nsresult ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                               nsSVGElement* aSVGElement);

private:

  nsAutoPtr<nsString> mAnimVal;
  PRUint8 mAttrEnum; 

  struct DOMAnimatedString : public nsIDOMSVGAnimatedString
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedString)

    DOMAnimatedString(nsSVGString* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    nsSVGString* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsAString & aResult)
      { mVal->GetBaseValue(aResult, mSVGElement); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString & aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); return NS_OK; }

    NS_IMETHOD GetAnimVal(nsAString & aResult)
      { mVal->GetAnimValue(aResult, mSVGElement); return NS_OK; }

  };
};
#endif 
