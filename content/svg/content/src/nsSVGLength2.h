



































#ifndef __NS_SVGLENGTH2_H__
#define __NS_SVGLENGTH2_H__

#include "nsIDOMSVGLength.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsSVGUtils.h"
#include "nsSVGElement.h"
#include "nsDOMError.h"
#include "nsMathUtils.h"

#include "nsISMILAttr.h"
class nsSMILValue;
class nsISMILType;

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
    mIsAnimated = false;
    mIsBaseSet = false;
  }

  nsSVGLength2& operator=(const nsSVGLength2& aLength) {
    mBaseVal = aLength.mBaseVal;
    mAnimVal = aLength.mAnimVal;
    mSpecifiedUnitType = aLength.mSpecifiedUnitType;
    mIsAnimated = aLength.mIsAnimated;
    mIsBaseSet = aLength.mIsBaseSet;
    return *this;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue);
  void GetAnimValueString(nsAString& aValue);

  float GetBaseValue(nsSVGElement* aSVGElement) const
    { return mBaseVal / GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType); }
  float GetAnimValue(nsSVGElement* aSVGElement) const
    { return mAnimVal / GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType); }
  float GetAnimValue(nsIFrame* aFrame) const
    { return mAnimVal / GetUnitScaleFactor(aFrame, mSpecifiedUnitType); }

  PRUint8 GetCtxType() const { return mCtxType; }
  PRUint8 GetSpecifiedUnitType() const { return mSpecifiedUnitType; }
  bool IsPercentage() const
    { return mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }

  float GetBaseValue(nsSVGSVGElement* aCtx) const
    { return mBaseVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }
  float GetAnimValue(nsSVGSVGElement* aCtx) const
    { return mAnimVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }

  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }
  
  nsresult ToDOMAnimatedLength(nsIDOMSVGAnimatedLength **aResult,
                               nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:
  
  float mAnimVal;
  float mBaseVal;
  PRUint8 mSpecifiedUnitType;
  PRUint8 mAttrEnum; 
  PRUint8 mCtxType; 
  bool mIsAnimated:1;
  bool mIsBaseSet:1;
  
  static float GetMMPerPixel() { return MM_PER_INCH_FLOAT / 96; }
  float GetAxisLength(nsIFrame *aNonSVGFrame) const;
  static float GetEmLength(nsIFrame *aFrame)
    { return nsSVGUtils::GetFontSize(aFrame); }
  static float GetExLength(nsIFrame *aFrame)
    { return nsSVGUtils::GetFontXHeight(aFrame); }
  float GetUnitScaleFactor(nsIFrame *aFrame, PRUint8 aUnitType) const;

  float GetMMPerPixel(nsSVGSVGElement *aCtx) const;
  float GetAxisLength(nsSVGSVGElement *aCtx) const;
  static float GetEmLength(nsSVGElement *aSVGElement)
    { return nsSVGUtils::GetFontSize(aSVGElement); }
  static float GetExLength(nsSVGElement *aSVGElement)
    { return nsSVGUtils::GetFontXHeight(aSVGElement); }
  float GetUnitScaleFactor(nsSVGElement *aSVGElement, PRUint8 aUnitType) const;
  float GetUnitScaleFactor(nsSVGSVGElement *aCtx, PRUint8 aUnitType) const;

  
  void SetBaseValue(float aValue, nsSVGElement *aSVGElement);
  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  void SetAnimValue(float aValue, nsSVGElement *aSVGElement);
  void SetAnimValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  nsresult NewValueSpecifiedUnits(PRUint16 aUnitType, float aValue,
                                  nsSVGElement *aSVGElement);
  nsresult ConvertToSpecifiedUnits(PRUint16 aUnitType, nsSVGElement *aSVGElement);
  nsresult ToDOMBaseVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGLength **aResult, nsSVGElement* aSVGElement);

  struct DOMBaseVal : public nsIDOMSVGLength
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMBaseVal)

    DOMBaseVal(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMBaseVal();
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
      { *aResult = mVal->mSpecifiedUnitType; return NS_OK; }

    NS_IMETHOD GetValue(float* aResult)
      { *aResult = mVal->GetBaseValue(mSVGElement); return NS_OK; }
    NS_IMETHOD SetValue(float aValue)
      {
        if (!NS_finite(aValue)) {
          return NS_ERROR_ILLEGAL_VALUE;
        }
        mVal->SetBaseValue(aValue, mSVGElement);
        return NS_OK;
      }

    NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
      { *aResult = mVal->mBaseVal; return NS_OK; }
    NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
      {
        if (!NS_finite(aValue)) {
          return NS_ERROR_ILLEGAL_VALUE;
        }
        mVal->SetBaseValueInSpecifiedUnits(aValue, mSVGElement);
        return NS_OK;
      }

    NS_IMETHOD SetValueAsString(const nsAString& aValue)
      { return mVal->SetBaseValueString(aValue, mSVGElement, true); }
    NS_IMETHOD GetValueAsString(nsAString& aValue)
      { mVal->GetBaseValueString(aValue); return NS_OK; }

    NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                      float valueInSpecifiedUnits)
      {
        return mVal->NewValueSpecifiedUnits(unitType, valueInSpecifiedUnits,
                                            mSVGElement); }

    NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
      { return mVal->ConvertToSpecifiedUnits(unitType, mSVGElement); }
  };

  struct DOMAnimVal : public nsIDOMSVGLength
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimVal)

    DOMAnimVal(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimVal();
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;
    
    
    
    NS_IMETHOD GetUnitType(PRUint16* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->mSpecifiedUnitType;
      return NS_OK;
    }

    NS_IMETHOD GetValue(float* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->GetAnimValue(mSVGElement);
      return NS_OK;
    }
    NS_IMETHOD SetValue(float aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD GetValueInSpecifiedUnits(float* aResult)
    {
      mSVGElement->FlushAnimations();
      *aResult = mVal->mAnimVal;
      return NS_OK;
    }
    NS_IMETHOD SetValueInSpecifiedUnits(float aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD SetValueAsString(const nsAString& aValue)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
    NS_IMETHOD GetValueAsString(nsAString& aValue)
    {
      mSVGElement->FlushAnimations();
      mVal->GetAnimValueString(aValue);
      return NS_OK;
    }

    NS_IMETHOD NewValueSpecifiedUnits(PRUint16 unitType,
                                      float valueInSpecifiedUnits)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }

    NS_IMETHOD ConvertToSpecifiedUnits(PRUint16 unitType)
      { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  };

public:
  struct DOMAnimatedLength : public nsIDOMSVGAnimatedLength
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedLength)

    DOMAnimatedLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimatedLength();
    
    nsSVGLength2* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGLength **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGLength **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

  struct SMILLength : public nsISMILAttr
  {
  public:
    SMILLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGLength2* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const nsISMILAnimationElement* aSrcElement,
                                     nsSMILValue &aValue,
                                     bool& aPreventCachingOfSandwich) const;
    virtual nsSMILValue GetBaseValue() const;
    virtual void ClearAnimValue();
    virtual nsresult SetAnimValue(const nsSMILValue& aValue);
  };
};

#endif 
