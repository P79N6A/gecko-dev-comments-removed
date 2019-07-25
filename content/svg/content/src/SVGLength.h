



































#ifndef MOZILLA_SVGLENGTH_H__
#define MOZILLA_SVGLENGTH_H__

#include "nsIDOMSVGLength.h"
#include "nsIContent.h"
#include "nsAString.h"
#include "nsContentUtils.h"

class nsSVGElement;

namespace mozilla {













class SVGLength
{
public:

  SVGLength()
#ifdef DEBUG
    : mValue(0.0f)
    , mUnit(nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN) 
#endif
  {}

  SVGLength(float aValue, PRUint8 aUnit)
    : mValue(aValue)
    , mUnit(aUnit)
  {
    NS_ASSERTION(IsValid(), "Constructed an invalid length");
  }

  SVGLength(const SVGLength &aOther)
    : mValue(aOther.mValue)
    , mUnit(aOther.mUnit)
  {}

  SVGLength& operator=(const SVGLength &rhs) {
    mValue = rhs.mValue;
    mUnit = rhs.mUnit;
    return *this;
  }

  PRBool operator==(const SVGLength &rhs) const {
    return mValue == rhs.mValue && mUnit == rhs.mUnit;
  }

  void GetValueAsString(nsAString& aValue) const;

  



  PRBool SetValueFromString(const nsAString& aValue);

  



  float GetValueInCurrentUnits() const {
    return mValue;
  }

  PRUint8 GetUnit() const {
    return mUnit;
  }

  void SetValueInCurrentUnits(float aValue) {
    mValue = aValue;
    NS_ASSERTION(IsValid(), "Set invalid SVGLength");
  }

  void SetValueAndUnit(float aValue, PRUint8 aUnit) {
    mValue = aValue;
    mUnit = aUnit;

    
    
    
    
    
    
    

    NS_ASSERTION(IsValidUnitType(mUnit), "Set invalid SVGLength");
  }

  



  float GetValueInUserUnits(const nsSVGElement *aElement, PRUint8 aAxis) const {
    return mValue * GetUserUnitsPerUnit(aElement, aAxis);
  }

  







  PRBool SetFromUserUnitValue(float aUserUnitValue,
                              nsSVGElement *aElement,
                              PRUint8 aAxis) {
    float uuPerUnit = GetUserUnitsPerUnit(aElement, aAxis);
    float value = aUserUnitValue / uuPerUnit;
    if (uuPerUnit > 0 && NS_FloatIsFinite(value)) {
      mValue = value;
      NS_ASSERTION(IsValid(), "Set invalid SVGLength");
      return PR_TRUE;
    }
    return PR_FALSE;
  }

  





  float GetValueInSpecifiedUnit(PRUint8 aUnit,
                                const nsSVGElement *aElement,
                                PRUint8 aAxis) const;

  






  PRBool ConvertToUnit(PRUint32 aUnit, nsSVGElement *aElement, PRUint8 aAxis) {
    float val = GetValueInSpecifiedUnit(aUnit, aElement, aAxis);
    if (NS_FloatIsFinite(val)) {
      mValue = val;
      mUnit = aUnit;
      NS_ASSERTION(IsValid(), "Set invalid SVGLength");
      return PR_TRUE;
    }
    return PR_FALSE;
  }

  PRBool IsPercentage() const {
    return mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE;
  }

  static PRBool IsValidUnitType(PRUint16 unit) {
    return unit > nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN &&
           unit <= nsIDOMSVGLength::SVG_LENGTHTYPE_PC;
  }

private:

#ifdef DEBUG
  PRBool IsValid() const {
    return NS_FloatIsFinite(mValue) && IsValidUnitType(mUnit);
  }
#endif

  






  float GetUserUnitsPerUnit(const nsSVGElement *aElement, PRUint8 aAxis) const;

  



  static float GetUserUnitsPerInch()
  {
    return 96.0;
  }

  








  static float GetUserUnitsPerPercent(const nsSVGElement *aElement, PRUint8 aAxis);

  float mValue;
  PRUint8 mUnit;
};

} 

#endif 
