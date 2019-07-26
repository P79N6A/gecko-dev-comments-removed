




#ifndef __NS_SVGANGLE_H__
#define __NS_SVGANGLE_H__

#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGAngle.h"
#include "nsIDOMSVGAnimatedAngle.h"
#include "nsISMILAttr.h"
#include "nsSVGElement.h"
#include "nsWrapperCache.h"
#include "mozilla/Attributes.h"

class nsISMILAnimationElement;
class nsSMILValue;
class nsSVGOrientType;

namespace mozilla {
namespace dom {
class SVGAngle;
}
}

class nsSVGAngle
{
  friend class mozilla::dom::SVGAngle;

public:
  void Init(uint8_t aAttrEnum = 0xff,
            float aValue = 0,
            uint8_t aUnitType = nsIDOMSVGAngle::SVG_ANGLETYPE_UNSPECIFIED) {
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

  static nsresult ToDOMSVGAngle(nsIDOMSVGAngle **aResult);
  nsresult ToDOMAnimatedAngle(nsIDOMSVGAnimatedAngle **aResult,
                              nsSVGElement* aSVGElement);
  
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
  nsresult ToDOMBaseVal(nsIDOMSVGAngle **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(nsIDOMSVGAngle **aResult, nsSVGElement* aSVGElement);

public:
  struct DOMAnimatedAngle MOZ_FINAL : public nsIDOMSVGAnimatedAngle
  {
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(DOMAnimatedAngle)

    DOMAnimatedAngle(nsSVGAngle* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}
    virtual ~DOMAnimatedAngle();
    
    nsSVGAngle* mVal; 
    nsRefPtr<nsSVGElement> mSVGElement;

    NS_IMETHOD GetBaseVal(nsIDOMSVGAngle **aBaseVal)
      { return mVal->ToDOMBaseVal(aBaseVal, mSVGElement); }

    NS_IMETHOD GetAnimVal(nsIDOMSVGAngle **aAnimVal)
      { return mVal->ToDOMAnimVal(aAnimVal, mSVGElement); }
  };

  
  
  
  

  struct SMILOrient MOZ_FINAL : public nsISMILAttr
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

#endif 
