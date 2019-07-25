



































#ifndef __NS_SVGANGLE_H__
#define __NS_SVGANGLE_H__

#include "nsIDOMSVGAngle.h"
#include "nsIDOMSVGAnimatedAngle.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"

class nsSVGOrientType;

class nsSVGAngle
{
  friend class DOMSVGAngle;

public:
  void Init(PRUint8 aAttrEnum = 0xff,
            float aValue = 0,
            PRUint8 aUnitType = nsIDOMSVGAngle::SVG_ANGLETYPE_UNSPECIFIED) {
    mAnimVal = mBaseVal = aValue;
    mAnimValUnit = mBaseValUnit = aUnitType;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);
  void GetAnimValueString(nsAString& aValue);

  float GetBaseValue() const
    { return mBaseVal * GetDegreesPerUnit(mBaseValUnit); }
  float GetAnimValue() const
    { return mAnimVal * GetDegreesPerUnit(mAnimValUnit); }

  void SetBaseValue(float aValue, nsSVGElement *aSVGElement);
  void SetAnimValue(float aValue, PRUint8 aUnit, nsSVGElement *aSVGElement);

  PRUint8 GetBaseValueUnit() const { return mBaseValUnit; }
  PRUint8 GetAnimValueUnit() const { return mAnimValUnit; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }

  static nsresult ToDOMSVGAngle(nsIDOMSVGAngle **aResult);
  nsresult ToDOMAnimatedAngle(nsIDOMSVGAnimatedAngle **aResult,
                              nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

  static float GetDegreesPerUnit(PRUint8 aUnit);

private:
  
  float mAnimVal;
  float mBaseVal;
  PRUint8 mAnimValUnit;
  PRUint8 mBaseValUnit;
  PRUint8 mAttrEnum; 
  bool mIsAnimated;
  
  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  nsresult NewValueSpecifiedUnits(PRUint16 aUnitType, float aValue,
                                  nsSVGElement *aSVGElement);
  nsresult ConvertToSpecifiedUnits(PRUint16 aUnitType, nsSVGElement *aSVGElement);
  nsresult ToDOMBaseVal(nsIDOMSVGAngle **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGAngle **aResult, nsSVGElement* aSVGElement);

public:
  struct DOMBaseVal : public nsIDOMSVGAngle
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGAngle* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
      { *aResult = mVal->mBaseValUnit; return NS_OK; }

    NS_IMETHOD GetValue(float* aResult)
      { *aResult = mVal->GetBaseValue(); return NS_OK; }
    NS_IMETHOD SetValue(float aValue)
      { mVal->SetBaseValue(aValue, mSVGElement); return NS_OK; }

    NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
      { *aResult = mVal->mBaseVal; return NS_OK; }
    NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
      { mVal->SetBaseValueInSpecifiedUnits(aValue, mSVGElement);
        return NS_OK; }

    NS_IMETHOD SetValueAsString(const nsAString& aValue)
      { return mVal->SetBaseValueString(aValue, mSVGElement, true); }
    NS_IMETHOD GetValueAsString(nsAString& aValue)
      { mVal->GetBaseValueString(aValue); return NS_OK; }

    NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                      float valueInSpecifiedUnits)
      { return mVal->NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits,
                                     mSVGElement); }

    NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
      { return mVal->ConvertToSpecifiedUnits(unitType, mSVGElement); }
  };

  struct DOMAnimVal : public nsIDOMSVGAngle
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGAngle* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
      { *aResult = mVal->mAnimValUnit; return NS_OK; }

    NS_IMETHOD GetValue(float* aResult)
      { *aResult = mVal->GetAnimValue(); return NS_OK; }
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

  struct DOMAnimatedAngle : public nsIDOMSVGAnimatedAngle
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedAngle)

    DOMAnimatedAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    
    nsSVGAngle* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGAngle **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGAngle **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

  
  
  
  

  struct SMILOrient : public nsISMILAttr
  {
  public:
    SMILOrient(nsSVGOrientType* aOrientType,
               nsSVGAngle* aAngle,
               nsSVGElement* aSVGElement)
      : mOrientType(aOrientType)
      , mAngle(aAngle)
      , mSVGElement(aSVGElement)
    {}

    
    
    
    nsSVGOrientType* mOrientType;
    nsSVGAngle* mAngle;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

nsresult
NS_NewDOMSVGAngle(nsIDOMSVGAngle** result);

#endif 
