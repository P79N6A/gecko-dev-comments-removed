




#ifndef __NS_SVGLENGTH2_H__
#define __NS_SVGLENGTH2_H__

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"
#include "nsCoord.h"
#include "nsCycleCollectionParticipant.h"
#include "nsError.h"
#include "nsIDOMSVGLength.h"
#include "nsISMILAttr.h"
#include "nsMathUtils.h"
#include "nsSVGElement.h"
#include "SVGContentUtils.h"

class nsIFrame;
class nsSMILValue;

namespace mozilla {
class DOMSVGLength;
namespace dom {
class SVGAnimatedLength;
class SVGAnimationElement;
class SVGSVGElement;
}
}

class nsSVGLength2
{
  friend class mozilla::dom::SVGAnimatedLength;
  friend class mozilla::DOMSVGLength;
public:
  void Init(uint8_t aCtxType = SVGContentUtils::XY,
            uint8_t aAttrEnum = 0xff,
            float aValue = 0,
            uint8_t aUnitType = nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER) {
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
  void GetBaseValueString(nsAString& aValue) const;
  void GetAnimValueString(nsAString& aValue) const;

  float GetBaseValue(nsSVGElement* aSVGElement) const
    { return mBaseVal / GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType); }
  float GetAnimValue(nsSVGElement* aSVGElement) const
    { return mAnimVal / GetUnitScaleFactor(aSVGElement, mSpecifiedUnitType); }
  float GetAnimValue(nsIFrame* aFrame) const
    { return mAnimVal / GetUnitScaleFactor(aFrame, mSpecifiedUnitType); }

  uint8_t GetCtxType() const { return mCtxType; }
  uint8_t GetSpecifiedUnitType() const { return mSpecifiedUnitType; }
  bool IsPercentage() const
    { return mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }

  float GetBaseValue(mozilla::dom::SVGSVGElement* aCtx) const
    { return mBaseVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }
  float GetAnimValue(mozilla::dom::SVGSVGElement* aCtx) const
    { return mAnimVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }

  bool HasBaseVal() const {
    return mIsBaseSet;
  }
  
  
  
  
  
  bool IsExplicitlySet() const
    { return mIsAnimated || mIsBaseSet; }

  already_AddRefed<mozilla::dom::SVGAnimatedLength>
  ToDOMAnimatedLength(nsSVGElement* aSVGElement);

  
  nsISMILAttr* ToSMILAttr(nsSVGElement* aSVGElement);

private:
  
  float mAnimVal;
  float mBaseVal;
  uint8_t mSpecifiedUnitType;
  uint8_t mAttrEnum; 
  uint8_t mCtxType; 
  bool mIsAnimated:1;
  bool mIsBaseSet:1;
  
  static float GetMMPerPixel() { return MM_PER_INCH_FLOAT / 96; }
  float GetAxisLength(nsIFrame *aNonSVGFrame) const;
  static float GetEmLength(nsIFrame *aFrame)
    { return SVGContentUtils::GetFontSize(aFrame); }
  static float GetExLength(nsIFrame *aFrame)
    { return SVGContentUtils::GetFontXHeight(aFrame); }
  float GetUnitScaleFactor(nsIFrame *aFrame, uint8_t aUnitType) const;

  float GetMMPerPixel(mozilla::dom::SVGSVGElement *aCtx) const;
  float GetAxisLength(mozilla::dom::SVGSVGElement *aCtx) const;
  static float GetEmLength(nsSVGElement *aSVGElement)
    { return SVGContentUtils::GetFontSize(aSVGElement); }
  static float GetExLength(nsSVGElement *aSVGElement)
    { return SVGContentUtils::GetFontXHeight(aSVGElement); }
  float GetUnitScaleFactor(nsSVGElement *aSVGElement, uint8_t aUnitType) const;
  float GetUnitScaleFactor(mozilla::dom::SVGSVGElement *aCtx, uint8_t aUnitType) const;

  
  void SetBaseValue(float aValue, nsSVGElement *aSVGElement, bool aDoSetAttr);
  void SetBaseValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement,
                                    bool aDoSetAttr);
  void SetAnimValue(float aValue, nsSVGElement *aSVGElement);
  void SetAnimValueInSpecifiedUnits(float aValue, nsSVGElement *aSVGElement);
  nsresult NewValueSpecifiedUnits(uint16_t aUnitType, float aValue,
                                  nsSVGElement *aSVGElement);
  nsresult ConvertToSpecifiedUnits(uint16_t aUnitType, nsSVGElement *aSVGElement);
  nsresult ToDOMBaseVal(mozilla::DOMSVGLength **aResult, nsSVGElement* aSVGElement);
  nsresult ToDOMAnimVal(mozilla::DOMSVGLength **aResult, nsSVGElement* aSVGElement);

public:
  struct SMILLength : public nsISMILAttr
  {
  public:
    SMILLength(nsSVGLength2* aVal, nsSVGElement *aSVGElement)
      : mVal(aVal), mSVGElement(aSVGElement) {}

    
    
    
    nsSVGLength2* mVal;
    nsSVGElement* mSVGElement;

    
    virtual nsresult ValueFromString(const nsAString& aStr,
                                     const mozilla::dom::SVGAnimationElement* aSrcElement,
                                     nsSMILValue &aValue,
                                     bool& aPreventCachingOfSandwich) const MOZ_OVERRIDE;
    virtual nsSMILValue GetBaseValue() const MOZ_OVERRIDE;
    virtual void ClearAnimValue() MOZ_OVERRIDE;
    virtual nsresult SetAnimValue(const nsSMILValue& aValue) MOZ_OVERRIDE;
  };
};

#endif 
