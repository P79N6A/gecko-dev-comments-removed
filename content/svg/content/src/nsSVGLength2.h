



































#ifndef __NS_SVGLENGTH2_H__
#define __NS_SVGLENGTH2_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsSVGUtils.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGLength2
{
  
  
  struct DOMBaseVal;
  struct DOMAnimVal;
  friend struct DOMBaseVal;
  friend struct DOMAnimVal;

public:
  void Init(PRUint8 aCtxType = nsSVGUtils::XY,
            PRUint8 aAttrEnum = 0xff,
            float aValue = 0,
            PRUint8 aUnitType = nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER) {
    mAnimVal = mBaseVal = aValue;
    mSpecifiedUnitType = aUnitType;
    mAttrEnum = aAttrEnum;
    mCtxType = aCtxType;
    mIsAnimated = PR_FALSE;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              PRBool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);
  void GetAnimValueString(nsAString& aValue);

  float GetBaseValue(nsSVGElement* aSVGElement)
    { return ConvertToUserUnits(mBaseVal, aSVGElement); }
  float GetAnimValue(nsSVGElement* aSVGElement)
    { return ConvertToUserUnits(mAnimVal, aSVGElement); }

  PRUint8 GetCtxType() { return mCtxType; }
  PRUint8 GetSpecifiedUnitType() { return mSpecifiedUnitType; }
  float GetAnimValInSpecifiedUnits() { return mAnimVal; }
  float GetBaseValInSpecifiedUnits() { return mBaseVal; }

  float GetBaseValue(nsSVGSVGElement* aProvider)
    { return ConvertToUserUnits(mBaseVal, aProvider); }
  float GetAnimValue(nsSVGSVGElement* aProvider)
    { return ConvertToUserUnits(mAnimVal, aProvider); }
  
  nsresult ToDOMBaseVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimatedLength(nsIDOMSVGAnimatedLength **aResult,
                               nsSVGElement* aSVGElement);

private:
  
  float mAnimVal;
  float mBaseVal;
  PRUint8 mSpecifiedUnitType;
  PRUint8 mAttrEnum; 
  PRUint8 mCtxType; 
  PRPackedBool mIsAnimated;
  
  float GetMMPerPixel(nsSVGSVGElement *aCtx);
  float GetAxisLength(nsSVGSVGElement *aCtx);
  float ConvertToUserUnits(float aValue, nsSVGElement *aSVGElement);
  float ConvertToUserUnits(float aValue, nsSVGSVGElement *aProvider);
  void SetBaseValue(float aValue, nsSVGElement *aSVGElement);
  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  void NewValueSpecifiedUnits(PRUint16 aUnitType, float aValue,
                              nsSVGElement *aSVGElement);
  void ConvertToSpecifiedUnits(PRUint16 aUnitType, nsSVGElement *aSVGElement);

  struct DOMBaseVal : public nsIDOMSVGLength
  {
    NS_DECL_ISUPPORTS

    DOMBaseVal(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
      { *aResult = mVal->mSpecifiedUnitType; return NS_OK; }

    NS_IMETHOD GetValue(float* aResult)
      { *aResult = mVal->GetBaseValue(mSVGElement); return NS_OK; }
    NS_IMETHOD SetValue(float aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

    NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
      { *aResult = mVal->mBaseVal; return NS_OK; }
    NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
      { mVal->SetBaseValueInSpecifiedUnits(aValue, mSVGElement);
        return NS_OK; }

    NS_IMETHOD SetValueAsString(const nsAString& aValue)
      { return mVal->SetBaseValueString(aValue, mSVGElement, PR_TRUE); }
    NS_IMETHOD GetValueAsString(nsAString& aValue)
      { mVal->GetBaseValueString(aValue); return NS_OK; }

    NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                      float valueInSpecifiedUnits)
      { mVal->NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits,
                                     mSVGElement);
        return NS_OK; }

    NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
      { mVal->ConvertToSpecifiedUnits(unitType, mSVGElement); return NS_OK; }
  };

  struct DOMAnimVal : public nsIDOMSVGLength
  {
    NS_DECL_ISUPPORTS

    DOMAnimVal(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
      { *aResult = mVal->mSpecifiedUnitType; return NS_OK; }

    NS_IMETHOD GetValue(float* aResult)
      { *aResult = mVal->GetAnimValue(mSVGElement); return NS_OK; }
    NS_IMETHOD SetValue(float aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
      { *aResult = mVal->mAnimVal; return NS_OK; }
    NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD SetValueAsString(const nsAString& aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD GetValueAsString(nsAString& aValue)
      { mVal->GetAnimValueString(aValue); return NS_OK; }

    NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                      float valueInSpecifiedUnits)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

  struct DOMAnimatedLength : public nsIDOMSVGAnimatedLength
  {
    NS_DECL_ISUPPORTS

    DOMAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGLength **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGLength **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };
};

#endif
