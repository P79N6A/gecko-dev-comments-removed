




#ifndef NS_SMILANIMATIONFUNCTION_H_
#define NS_SMILANIMATIONFUNCTION_H_

#include "nsISMILAttr.h"
#include "nsGkAtoms.h"
#include "nsString.h"
#include "nsSMILTargetIdentifier.h"
#include "nsSMILTimeValue.h"
#include "nsSMILKeySpline.h"
#include "nsSMILValue.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsAttrValue.h"
#include "nsSMILTypes.h"

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}













class nsSMILAnimationFunction
{
public:
  nsSMILAnimationFunction();

  



  void SetAnimationElement(mozilla::dom::SVGAnimationElement* aAnimationElement);

  












  virtual bool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nullptr);

  





  virtual bool UnsetAttr(nsIAtom* aAttribute);

  








  void SampleAt(nsSMILTime aSampleTime,
                const nsSMILTimeValue& aSimpleDuration,
                uint32_t aRepeatIteration);

  








  void SampleLastValue(uint32_t aRepeatIteration);

  








  void Activate(nsSMILTime aBeginTime);

  








  void Inactivate(bool aIsFrozen);

  








  void ComposeResult(const nsISMILAttr& aSMILAttr, nsSMILValue& aResult);

  










  int8_t CompareTo(const nsSMILAnimationFunction* aOther) const;

  





  





  bool IsActiveOrFrozen() const
  {
    





    return (mIsActive || mIsFrozen);
  }

  







  virtual bool WillReplace() const;

  










  bool HasChanged() const;

  







  void ClearHasChanged()
  {
    NS_ABORT_IF_FALSE(HasChanged(),
                      "clearing mHasChanged flag, when it's already false");
    NS_ABORT_IF_FALSE(!IsActiveOrFrozen(),
                      "clearing mHasChanged flag for active animation");
    mHasChanged = false;
  }

  











  bool UpdateCachedTarget(const nsSMILTargetIdentifier& aNewTarget);

  




  bool WasSkippedInPrevSample() const {
    return mWasSkippedInPrevSample;
  }

  




  void SetWasSkipped() {
    mWasSkippedInPrevSample = true;
  }

  
  class Comparator {
    public:
      bool Equals(const nsSMILAnimationFunction* aElem1,
                    const nsSMILAnimationFunction* aElem2) const {
        return (aElem1->CompareTo(aElem2) == 0);
      }
      bool LessThan(const nsSMILAnimationFunction* aElem1,
                      const nsSMILAnimationFunction* aElem2) const {
        return (aElem1->CompareTo(aElem2) < 0);
      }
  };

protected:
  
  typedef nsTArray<nsSMILValue> nsSMILValueArray;

  
  enum nsSMILCalcMode
  {
    CALC_LINEAR,
    CALC_DISCRETE,
    CALC_PACED,
    CALC_SPLINE
  };

  
  nsSMILTime GetBeginTime() const { return mBeginTime; }

  
  bool                   GetAccumulate() const;
  bool                   GetAdditive() const;
  virtual nsSMILCalcMode GetCalcMode() const;

  
  nsresult SetAccumulate(const nsAString& aAccumulate, nsAttrValue& aResult);
  nsresult SetAdditive(const nsAString& aAdditive, nsAttrValue& aResult);
  nsresult SetCalcMode(const nsAString& aCalcMode, nsAttrValue& aResult);
  nsresult SetKeyTimes(const nsAString& aKeyTimes, nsAttrValue& aResult);
  nsresult SetKeySplines(const nsAString& aKeySplines, nsAttrValue& aResult);

  
  void     UnsetAccumulate();
  void     UnsetAdditive();
  void     UnsetCalcMode();
  void     UnsetKeyTimes();
  void     UnsetKeySplines();

  
  virtual nsresult InterpolateResult(const nsSMILValueArray& aValues,
                                     nsSMILValue& aResult,
                                     nsSMILValue& aBaseValue);
  nsresult AccumulateResult(const nsSMILValueArray& aValues,
                            nsSMILValue& aResult);

  nsresult ComputePacedPosition(const nsSMILValueArray& aValues,
                                double aSimpleProgress,
                                double& aIntervalProgress,
                                const nsSMILValue*& aFrom,
                                const nsSMILValue*& aTo);
  double   ComputePacedTotalDistance(const nsSMILValueArray& aValues) const;

  



  double   ScaleSimpleProgress(double aProgress, nsSMILCalcMode aCalcMode);
  



  double   ScaleIntervalProgress(double aProgress, uint32_t aIntervalIndex);

  
  
  virtual bool               HasAttr(nsIAtom* aAttName) const;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  virtual bool               GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const;

  bool     ParseAttr(nsIAtom* aAttName, const nsISMILAttr& aSMILAttr,
                     nsSMILValue& aResult,
                     bool& aPreventCachingOfSandwich) const;

  virtual nsresult GetValues(const nsISMILAttr& aSMILAttr,
                             nsSMILValueArray& aResult);

  virtual void CheckValueListDependentAttrs(uint32_t aNumValues);
  void         CheckKeyTimes(uint32_t aNumValues);
  void         CheckKeySplines(uint32_t aNumValues);

  virtual bool IsToAnimation() const {
    return !HasAttr(nsGkAtoms::values) &&
            HasAttr(nsGkAtoms::to) &&
           !HasAttr(nsGkAtoms::from);
  }

  
  
  virtual bool IsValueFixedForSimpleDuration() const;

  inline bool IsAdditive() const {
    







    bool isByAnimation = (!HasAttr(nsGkAtoms::values) &&
                             HasAttr(nsGkAtoms::by) &&
                            !HasAttr(nsGkAtoms::from));
    return !IsToAnimation() && (GetAdditive() || isByAnimation);
  }

  
  
  
  
  enum AnimationAttributeIdx {
    BF_ACCUMULATE  = 0,
    BF_ADDITIVE    = 1,
    BF_CALC_MODE   = 2,
    BF_KEY_TIMES   = 3,
    BF_KEY_SPLINES = 4,
    BF_KEY_POINTS  = 5 
  };

  inline void SetAccumulateErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_ACCUMULATE, aNewValue);
  }
  inline void SetAdditiveErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_ADDITIVE, aNewValue);
  }
  inline void SetCalcModeErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_CALC_MODE, aNewValue);
  }
  inline void SetKeyTimesErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_KEY_TIMES, aNewValue);
  }
  inline void SetKeySplinesErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_KEY_SPLINES, aNewValue);
  }
  inline void SetKeyPointsErrorFlag(bool aNewValue) {
    SetErrorFlag(BF_KEY_POINTS, aNewValue);
  }
  inline void SetErrorFlag(AnimationAttributeIdx aField, bool aValue) {
    if (aValue) {
      mErrorFlags |=  (0x01 << aField);
    } else {
      mErrorFlags &= ~(0x01 << aField);
    }
  }

  
  

  static nsAttrValue::EnumTable sAdditiveTable[];
  static nsAttrValue::EnumTable sCalcModeTable[];
  static nsAttrValue::EnumTable sAccumulateTable[];

  nsTArray<double>              mKeyTimes;
  nsTArray<nsSMILKeySpline>     mKeySplines;

  
  
  
  
  
  nsSMILTime                    mSampleTime; 
  nsSMILTimeValue               mSimpleDuration;
  uint32_t                      mRepeatIteration;

  nsSMILTime                    mBeginTime; 

  
  
  
  
  mozilla::dom::SVGAnimationElement* mAnimationElement;

  
  
  
  uint16_t                      mErrorFlags;

  
  
  
  nsSMILWeakTargetIdentifier    mLastTarget;

  
  bool mIsActive:1;
  bool mIsFrozen:1;
  bool mLastValue:1;
  bool mHasChanged:1;
  bool mValueNeedsReparsingEverySample:1;
  bool mPrevSampleWasSingleValueAnimation:1;
  bool mWasSkippedInPrevSample:1;
};

#endif 
