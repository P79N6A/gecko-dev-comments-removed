



































#ifndef __NS_SVGLENGTH2_H__
#define __NS_SVGLENGTH2_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsSVGUtils.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
class nsSMILValue;
class nsISMILType;
#endif 

class nsIFrame;

class nsSVGLength2
{

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

  float GetBaseValue(nsSVGElement* aSVGElement) const
    { return mBaseVal / GetUnitScaleFactor(aSVGElement); }
  float GetAnimValue(nsSVGElement* aSVGElement) const
    { return mAnimVal / GetUnitScaleFactor(aSVGElement); }
  float GetAnimValue(nsIFrame* aFrame) const
    { return mAnimVal / GetUnitScaleFactor(aFrame); }

  PRUint8 GetCtxType() const { return mCtxType; }
  PRUint8 GetSpecifiedUnitType() const { return mSpecifiedUnitType; }
  PRBool IsPercentage() const
    { return mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }

  float GetBaseValue(nsSVGSVGElement* aCtx) const
    { return mBaseVal / GetUnitScaleFactor(aCtx); }
  float GetAnimValue(nsSVGSVGElement* aCtx) const
    { return mAnimVal / GetUnitScaleFactor(aCtx); }
  
  nsresult ToDOMAnimatedLength(nsIDOMSVGAnimatedLength **aResult,
                               nsSVGElement* aSVGElement);
#ifdef MOZ_SMIL
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);
#endif 

private:
  
  float mAnimVal;
  float mBaseVal;
  PRUint8 mSpecifiedUnitType;
  PRUint8 mAttrEnum; 
  PRUint8 mCtxType; 
  PRPackedBool mIsAnimated;
  
  float GetMMPerPixel(nsIFrame *aNonSVGFrame) const;
  float GetAxisLength(nsIFrame *aNonSVGFrame) const;
  float GetEmLength(nsIFrame *aFrame) const
    { return nsSVGUtils::GetFontSize(aFrame); }
  float GetExLength(nsIFrame *aFrame) const
    { return nsSVGUtils::GetFontXHeight(aFrame); }
  float GetUnitScaleFactor(nsIFrame *aFrame) const;

  float GetMMPerPixel(nsSVGSVGElement *aCtx) const;
  float GetAxisLength(nsSVGSVGElement *aCtx) const;
  float GetEmLength(nsSVGElement *aSVGElement) const
    { return nsSVGUtils::GetFontSize(aSVGElement); }
  float GetExLength(nsSVGElement *aSVGElement) const
    { return nsSVGUtils::GetFontXHeight(aSVGElement); }
  float GetUnitScaleFactor(nsSVGElement *aSVGElement) const;
  float GetUnitScaleFactor(nsSVGSVGElement *aCtx) const;
  void SetBaseValue(float aValue, nsSVGElement *aSVGElement);
  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  void SetAnimValue(float aValue, nsSVGElement *aSVGElement);
  void NewValueSpecifiedUnits(PRUint16 aUnitType, float aValue,
                              nsSVGElement *aSVGElement);
  void ConvertToSpecifiedUnits(PRUint16 aUnitType, nsSVGElement *aSVGElement);
  nsresult ToDOMBaseVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);

  struct DOMBaseVal : public nsIDOMSVGLength
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

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
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

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
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedLength)

    DOMAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGLength **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGLength **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

#ifdef MOZ_SMIL
  struct SMILLength : public nsISMILAttr
  {
  public:
    SMILLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGLength2* mVal;
    nsSVGElement* mSVGElement;


    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue &aValue) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
#endif 
};

#endif 
