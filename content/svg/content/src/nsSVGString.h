



































#ifndef __NS_SVGSTRING_H__
#define __NS_SVGSTRING_H__

#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGString
{

public:
  void Init(PRUint8 aAttrEnum) {
    mAnimVal.Truncate();
    mBaseVal.Truncate();
    mAttrEnum = aAttrEnum;
  }

  void SetBaseValue(const nsAString& aValue,
                    nsSVGElement *aSVGElement,
                    PRBool aDoSetAttr);
  const nsString &GetBaseValue() const
    { return mBaseVal; }
  const nsString &GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedString(nsIDOMSVGAnimatedString **aResult,
                               nsSVGElement* aSVGElement);

private:

  nsString mAnimVal;
  nsString mBaseVal;
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
      { aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetBaseVal(const nsAString & aValue)
      { mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); return NS_OK; }

    NS_IMETHOD GetAnimVal(nsAString & aResult)
      { aResult = mVal->GetAnimValue(); return NS_OK; }

  };
};
#endif 
