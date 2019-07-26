




#ifndef __NS_SVGANGLE_H__
#define __NS_SVGANGLE_H__

#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsISMILAttr.h"
#include "mozilla/Attributes.h"

class nsISupports;
class nsSMILValue;
class nsSVGElement;

namespace mozilla {


static const unsigned short SVG_ANGLETYPE_UNKNOWN     = 0;
static const unsigned short SVG_ANGLETYPE_UNSPECIFIED = 1;
static const unsigned short SVG_ANGLETYPE_DEG         = 2;
static const unsigned short SVG_ANGLETYPE_RAD         = 3;
static const unsigned short SVG_ANGLETYPE_GRAD        = 4;

namespace dom {
class nsSVGOrientType;
class SVGAngle;
class SVGAnimatedAngle;
class SVGAnimationElement;
}
}

class nsSVGAngle
{
  friend class mozilla::dom::SVGAngle;
  friend class mozilla::dom::SVGAnimatedAngle;

public:
  void Init(uint8_t aAttrEnum = 0xff,
            float aValue = 0,
            uint8_t aUnitType = mozilla::SVG_ANGLETYPE_UNSPECIFIED) {
    mAnimVal = mBaseVal = aValue;
    mAnimValUnit = mBaseValUnit = aUnitType;
    mAttrEnum = aAttrEnum;
    mIsAnimated = false;
  }

  nsresult SetBaseValueString(const nsAString& aValue,
                              nsSVGElement *aSVGElement,
                              bool aDoSetAttr);
  void GetBaseValueString(nsAString& aValue) const;
  void GetAnimValueString(nsAString& aValue) const;

  float GetBaseValue() const
    { return mBaseVal * GetDegreesPerUnit(mBaseValUnit); }
  float GetAnimValue() const
    { return mAnimVal * GetDegreesPerUnit(mAnimValUnit); }

  void SetBaseValue(float aValue, nsSVGElement *aSVGElement, bool aDoSetAttr);
  void SetAnimValue(float aValue, uint8_t aUnit, nsSVGElement *aSVGElement);

  uint8_t GetBaseValueUnit() const { return mBaseValUnit; }
  uint8_t GetAnimValueUnit() const { return mAnimValUnit; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }

  static nsresult ToDOMSVGAngle(nsISupports **aResult);
  already_AddRefed<mozilla::dom::SVGAnimatedAngle>
    ToDOMAnimatedAngle(nsSVGElement* aSVGElement);
  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

  static float GetDegreesPerUnit(uint8_t aUnit);

private:

  float mAnimVal;
  float mBaseVal;
  uint8_t mAnimValUnit;
  uint8_t mBaseValUnit;
  uint8_t mAttrEnum; 
  bool mIsAnimated;

  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  nsresult NewValueSpecifiedUnits(uint16_t aUnitType, float aValue,
                                  nsSVGElement *aSVGElement);
  nsresult ConvertToSpecifiedUnits(uint16_t aUnitType, nsSVGElement *aSVGElement);
  already_AddRefed<mozilla::dom::SVGAngle> ToDOMBaseVal(nsSVGElement* aSVGElement);
  already_AddRefed<mozilla::dom::SVGAngle> ToDOMAnimVal(nsSVGElement* aSVGElement);

public:
  
  
  
  

  struct SMILOrient MOZ_FINAL : public nsISMILAttr
  {
  public:
    SMILOrient(mozilla::dom::nsSVGOrientType* aOrientType,
               nsSVGAngle* aAngle,
               nsSVGElement* aSVGElement)
      : mOrientType(aOrientType)
      , mAngle(aAngle)
      , mSVGElement(aSVGElement)
    {}

    
    
    
    mozilla::dom::nsSVGOrientType* mOrientType;
    nsSVGAngle* mAngle;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const mozilla::dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue& aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};

#endif 
