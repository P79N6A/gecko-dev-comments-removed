




#ifndef MOZILLA_SVGLENGTH_H__
#define MOZILLA_SVGLENGTH_H__

#include "nsDebug.h"
#include "nsIDOMSVGLength.h"
#include "nsMathUtils.h"

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

  SVGLength(float aValue, uint8_t aUnit)
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

  bool operator==(const SVGLength &rhs) const {
    return mValue == rhs.mValue && mUnit == rhs.mUnit;
  }

  void GetValueAsString(nsAString& aValue) const;

  



  bool SetValueFromString(const nsAString& aValue);

  



  float GetValueInCurrentUnits() const {
    return mValue;
  }

  uint8_t GetUnit() const {
    return mUnit;
  }

  void SetValueInCurrentUnits(float aValue) {
    mValue = aValue;
    NS_ASSERTION(IsValid(), "Set invalid SVGLength");
  }

  void SetValueAndUnit(float aValue, uint8_t aUnit) {
    mValue = aValue;
    mUnit = aUnit;

    
    
    
    
    
    
    

    NS_ASSERTION(IsValidUnitType(mUnit), "Set invalid SVGLength");
  }

  



  float GetValueInUserUnits(const nsSVGElement *aElement, uint8_t aAxis) const {
    return mValue * GetUserUnitsPerUnit(aElement, aAxis);
  }

  







  bool SetFromUserUnitValue(float aUserUnitValue,
                              nsSVGElement *aElement,
                              uint8_t aAxis) {
    float uuPerUnit = GetUserUnitsPerUnit(aElement, aAxis);
    float value = aUserUnitValue / uuPerUnit;
    if (uuPerUnit > 0 && NS_finite(value)) {
      mValue = value;
      NS_ASSERTION(IsValid(), "Set invalid SVGLength");
      return true;
    }
    return false;
  }

  





  float GetValueInSpecifiedUnit(uint8_t aUnit,
                                const nsSVGElement *aElement,
                                uint8_t aAxis) const;

  






  bool ConvertToUnit(uint32_t aUnit, nsSVGElement *aElement, uint8_t aAxis) {
    float val = GetValueInSpecifiedUnit(aUnit, aElement, aAxis);
    if (NS_finite(val)) {
      mValue = val;
      mUnit = aUnit;
      NS_ASSERTION(IsValid(), "Set invalid SVGLength");
      return true;
    }
    return false;
  }

  bool IsPercentage() const {
    return mUnit == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE;
  }

  static bool IsValidUnitType(uint16_t unit) {
    return unit > nsIDOMSVGLength::SVG_LENGTHTYPE_UNKNOWN &&
           unit <= nsIDOMSVGLength::SVG_LENGTHTYPE_PC;
  }

private:

#ifdef DEBUG
  bool IsValid() const {
    return NS_finite(mValue) && IsValidUnitType(mUnit);
  }
#endif

  






  float GetUserUnitsPerUnit(const nsSVGElement *aElement, uint8_t aAxis) const;

  



  static float GetUserUnitsPerInch()
  {
    return 96.0;
  }

  








  static float GetUserUnitsPerPercent(const nsSVGElement *aElement, uint8_t aAxis);

  float mValue;
  uint8_t mUnit;
};

} 

#endif 
