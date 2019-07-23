





































#ifndef NS_SMILANIMATIONFUNCTION_H_
#define NS_SMILANIMATIONFUNCTION_H_

#include "nsISMILAttr.h"
#include "nsGkAtoms.h"
#include "nsString.h"
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

  







  PRBool WillReplace() const;

  









  PRBool HasChanged() const;

  
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

  void     ScaleSimpleProgress(double& aProgress);
  void     ScaleIntervalProgress(double& aProgress, PRUint32 aIntervalIndex,
                                 PRUint32 aNumIntervals);

  
  
  virtual PRBool             HasAttr(nsIAtom* aAttName) const;
  virtual const nsAttrValue* GetAttr(nsIAtom* aAttName) const;
  virtual PRBool             GetAttr(nsIAtom* aAttName,
                                     nsAString& aResult) const;

  PRBool   ParseAttr(nsIAtom* aAttName, const nsISMILAttr& aSMILAttr,
                     nsSMILValue& aResult) const;
  nsresult GetValues(const nsISMILAttr& aSMILAttr,
                     nsSMILValueArray& aResult);
  void     UpdateValuesArray();
  PRBool   IsToAnimation() const;
  PRBool   IsAdditive() const;
  void     CheckKeyTimes(PRUint32 aNumValues);
  void     CheckKeySplines(PRUint32 aNumValues);


  
  

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

  nsSMILTime                    mBeginTime; 

  
  
  
  
  nsISMILAnimationElement*      mAnimationElement;

  
  
  
  PRUint16                      mErrorFlags;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsSMILValue                   mFrozenValue;
};

#endif 
