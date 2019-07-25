





































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

class nsISMILAnimationElement;













class nsSMILAnimationFunction
{
public:
  nsSMILAnimationFunction();

  



  void SetAnimationElement(nsISMILAnimationElement* aAnimationElement);

  












  virtual PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                         nsAttrValue& aResult, nsresult* aParseResult = nsnull);

  





  virtual PRBool UnsetAttr(nsIAtom* aAttribute);

  








  void SampleAt(nsSMILTime aSampleTime,
                const nsSMILTimeValue& aSimpleDuration,
                PRUint32 aRepeatIteration);

  








  void SampleLastValue(PRUint32 aRepeatIteration);

  








  void Activate(nsSMILTime aBeginTime);

  








  void Inactivate(PRBool aIsFrozen);

  








  void ComposeResult(const nsISMILAttr& aSMILAttr, nsSMILValue& aResult);

  










  PRInt8 CompareTo(const nsSMILAnimationFunction* aOther) const;

  





  





  PRBool IsActiveOrFrozen() const
  {
    





    return (mIsActive || mIsFrozen);
  }

  







  virtual PRBool WillReplace() const;

  










  PRBool HasChanged() const;

  







  void ClearHasChanged()
  {
    NS_ABORT_IF_FALSE(HasChanged(),
                      "clearing mHasChanged flag, when it's already PR_FALSE");
    NS_ABORT_IF_FALSE(!IsActiveOrFrozen(),
                      "clearing mHasChanged flag for active animation");
    mHasChanged = PR_FALSE;
  }

  











  PRBool UpdateCachedTarget(const nsSMILTargetIdentifier& aNewTarget);

  
  class Comparator {
    public:
      PRBool Equals(const nsSMILAnimationFunction* aElem1,
                    const nsSMILAnimationFunction* aElem2) const {
        return (aElem1->CompareTo(aElem2) == 0);
      }
      PRBool LessThan(const nsSMILAnimationFunction* aElem1,
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

  
  PRBool                 GetAccumulate() const;
  PRBool                 GetAdditive() const;
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
  



  double   ScaleIntervalProgress(double aProgress, PRUint32 aIntervalIndex);

  
  
  virtual PRBool             HasAttr(nsIAtom* aAttName) const;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  virtual PRBool             GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const;

  PRBool   ParseAttr(nsIAtom* aAttName, const nsISMILAttr& aSMILAttr,
                     nsSMILValue& aResult,
                     PRBool& aPreventCachingOfSandwich) const;

  virtual nsresult GetValues(const nsISMILAttr& aSMILAttr,
                             nsSMILValueArray& aResult);

  virtual void CheckValueListDependentAttrs(PRUint32 aNumValues);
  void         CheckKeyTimes(PRUint32 aNumValues);
  void         CheckKeySplines(PRUint32 aNumValues);

  
  
  
  virtual PRBool TreatSingleValueAsStatic() const {
    return HasAttr(nsGkAtoms::values);
  }

  inline PRBool IsToAnimation() const {
    return !HasAttr(nsGkAtoms::values) &&
            HasAttr(nsGkAtoms::to) &&
           !HasAttr(nsGkAtoms::from);
  }

  inline PRBool IsAdditive() const {
    







    PRBool isByAnimation = (!HasAttr(nsGkAtoms::values) &&
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

  inline void SetAccumulateErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_ACCUMULATE, aNewValue);
  }
  inline void SetAdditiveErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_ADDITIVE, aNewValue);
  }
  inline void SetCalcModeErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_CALC_MODE, aNewValue);
  }
  inline void SetKeyTimesErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_KEY_TIMES, aNewValue);
  }
  inline void SetKeySplinesErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_KEY_SPLINES, aNewValue);
  }
  inline void SetKeyPointsErrorFlag(PRBool aNewValue) {
    SetErrorFlag(BF_KEY_POINTS, aNewValue);
  }
  
  inline void SetErrorFlag(AnimationAttributeIdx aField, PRBool aValue) {
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

  PRPackedBool                  mIsActive;
  PRPackedBool                  mIsFrozen;

  
  
  
  
  
  nsSMILTime                    mSampleTime; 
  nsSMILTimeValue               mSimpleDuration;
  PRUint32                      mRepeatIteration;
  PRPackedBool                  mLastValue;
  PRPackedBool                  mHasChanged;
  PRPackedBool                  mValueNeedsReparsingEverySample;

  nsSMILTime                    mBeginTime; 

  
  
  
  
  nsISMILAnimationElement*      mAnimationElement;

  
  
  
  PRUint16                      mErrorFlags;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsSMILValue                   mFrozenValue;

  
  
  
  nsSMILWeakTargetIdentifier    mLastTarget;
};

#endif 
