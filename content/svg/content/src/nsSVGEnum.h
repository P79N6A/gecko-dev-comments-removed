



































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
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue,
                          nsSVGElement *aSVGElement);

  nsresult SetBaseValue(PRUint16 aValue,
                        nsSVGElement *aSVGElement,
                        PRBool aDoSetAttr);

  PRUint16 GetBaseValue() const
    { return mBaseVal; }
  PRUint16 GetAnimValue() const
    { return mAnimVal; }

  nsresult ToDOMAnimatedEnum(nsIDOMSVGAnimatedEnumeration **aResult,
                             nsSVGElement* aSVGElement);

private:
  nsSVGEnumValue mAnimVal;
  nsSVGEnumValue mBaseVal;
  PRUint8 mAttrEnum; 

  nsSVGEnumMapping *GetMapping(nsSVGElement *aSVGElement);

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
      { return mVal->SetBaseValue(aValue, mSVGElement, PR_TRUE); }
    NS_IMETHOD GetAnimVal(PRUint16* aResult)
      { *aResult = mVal->GetAnimValue(); return NS_OK; }
  };
};

#endif 
