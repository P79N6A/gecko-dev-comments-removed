




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
#include "mozilla/gfx/Rect.h"

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

namespace mozilla {
namespace dom {

class UserSpaceMetrics
{
public:
  virtual ~UserSpaceMetrics() {}

  virtual float GetEmLength() const = 0;
  virtual float GetExLength() const = 0;
  virtual float GetAxisLength(uint8_t aCtxType) const = 0;
};

class UserSpaceMetricsWithSize : public UserSpaceMetrics
{
public:
  virtual gfx::Size GetSize() const = 0;
  virtual float GetAxisLength(uint8_t aCtxType) const MOZ_OVERRIDE;
};

class SVGElementMetrics : public UserSpaceMetrics
{
public:
  explicit SVGElementMetrics(nsSVGElement* aSVGElement,
                             mozilla::dom::SVGSVGElement* aCtx = nullptr);

  virtual float GetEmLength() const MOZ_OVERRIDE;
  virtual float GetExLength() const MOZ_OVERRIDE;
  virtual float GetAxisLength(uint8_t aCtxType) const MOZ_OVERRIDE;

private:
  bool EnsureCtx() const;

  nsSVGElement* mSVGElement;
  mutable mozilla::dom::SVGSVGElement* mCtx;
};

class NonSVGFrameUserSpaceMetrics : public UserSpaceMetricsWithSize
{
public:
  explicit NonSVGFrameUserSpaceMetrics(nsIFrame* aFrame);

  virtual float GetEmLength() const MOZ_OVERRIDE;
  virtual float GetExLength() const MOZ_OVERRIDE;
  virtual gfx::Size GetSize() const MOZ_OVERRIDE;

private:
  nsIFrame* mFrame;
};

}
}

class nsSVGLength2
{
  friend class mozilla::dom::SVGAnimatedLength;
  friend class mozilla::DOMSVGLength;
  typedef mozilla::dom::UserSpaceMetrics UserSpaceMetrics;
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
  float GetAnimValue(mozilla::dom::SVGSVGElement* aCtx) const
    { return mAnimVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }
  float GetAnimValue(const UserSpaceMetrics& aMetrics) const
    { return mAnimVal / GetUnitScaleFactor(aMetrics, mSpecifiedUnitType); }

  uint8_t GetCtxType() const { return mCtxType; }
  uint8_t GetSpecifiedUnitType() const { return mSpecifiedUnitType; }
  bool IsPercentage() const
    { return mSpecifiedUnitType == nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE; }
  float GetAnimValInSpecifiedUnits() const { return mAnimVal; }
  float GetBaseValInSpecifiedUnits() const { return mBaseVal; }

  float GetBaseValue(mozilla::dom::SVGSVGElement* aCtx) const
    { return mBaseVal / GetUnitScaleFactor(aCtx, mSpecifiedUnitType); }

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

  float GetUnitScaleFactor(nsIFrame *aFrame, uint8_t aUnitType) const;
  float GetUnitScaleFactor(const UserSpaceMetrics& aMetrics, uint8_t aUnitType) const;
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
