




































#ifndef NS_SMILTIMEDELEMENT_H_
#define NS_SMILTIMEDELEMENT_H_

#include "nsSMILInterval.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILMilestone.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILRepeatCount.h"
#include "nsSMILTypes.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsAutoPtr.h"
#include "nsAttrValue.h"

class nsISMILAnimationElement;
class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimeValue;
class nsIAtom;




class nsSMILTimedElement
{
public:
  nsSMILTimedElement();
  ~nsSMILTimedElement();

  typedef mozilla::dom::Element Element;

  



  void SetAnimationElement(nsISMILAnimationElement* aElement);

  



  nsSMILTimeContainer* GetTimeContainer();

  



  mozilla::dom::Element* GetTargetElement()
  {
    return mAnimationElement ?
        mAnimationElement->GetTargetElementContent() :
        nsnull;
  }

  



  







  nsresult BeginElementAt(double aOffsetSeconds);

  







  nsresult EndElementAt(double aOffsetSeconds);

  



  








  nsSMILTimeValue GetStartTime() const;

  




  nsSMILTimeValue GetSimpleDuration() const
  {
    return mSimpleDur;
  }

  



  









  void AddInstanceTime(nsSMILInstanceTime* aInstanceTime, PRBool aIsBegin);

  












  void UpdateInstanceTime(nsSMILInstanceTime* aInstanceTime,
                          nsSMILTimeValue& aUpdatedTime,
                          PRBool aIsBegin);

  








  void RemoveInstanceTime(nsSMILInstanceTime* aInstanceTime, PRBool aIsBegin);

  









  void RemoveInstanceTimesForCreator(const nsSMILTimeValueSpec* aSpec,
                                     PRBool aIsBegin);

  











  void SetTimeClient(nsSMILAnimationFunction* aClient);

  






  void SampleAt(nsSMILTime aContainerTime);

  











  void SampleEndAt(nsSMILTime aContainerTime);

  





  void HandleContainerTimeChange();

  







  void Rewind();

  


















  PRBool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                 nsAttrValue& aResult, Element* aContextNode,
                 nsresult* aParseResult = nsnull);

  









  PRBool UnsetAttr(nsIAtom* aAttribute);

  








  void AddDependent(nsSMILTimeValueSpec& aDependent);

  





  void RemoveDependent(nsSMILTimeValueSpec& aDependent);

  











  PRBool IsTimeDependent(const nsSMILTimedElement& aOther) const;

  








  void BindToTree(nsIContent* aContextNode);

  



  void HandleTargetElementChange(mozilla::dom::Element* aNewTarget);

  



  void DissolveReferences() { Unlink(); }

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

  typedef PRBool (*RemovalTestFunction)(nsSMILInstanceTime* aInstance);

protected:
  
  typedef nsTArray<nsAutoPtr<nsSMILTimeValueSpec> > TimeValueSpecList;
  typedef nsTArray<nsRefPtr<nsSMILInstanceTime> >   InstanceTimeList;
  typedef nsTArray<nsAutoPtr<nsSMILInterval> >      IntervalList;
  typedef nsPtrHashKey<nsSMILTimeValueSpec> TimeValueSpecPtrKey;
  typedef nsTHashtable<TimeValueSpecPtrKey> TimeValueSpecHashSet;

  
  class InstanceTimeComparator {
    public:
      PRBool Equals(const nsSMILInstanceTime* aElem1,
                    const nsSMILInstanceTime* aElem2) const;
      PRBool LessThan(const nsSMILInstanceTime* aElem1,
                      const nsSMILInstanceTime* aElem2) const;
  };

  struct NotifyTimeDependentsParams {
    nsSMILInterval*      mCurrentInterval;
    nsSMILTimeContainer* mTimeContainer;
  };

  
  template <class TestFunctor>
  void RemoveInstanceTimes(InstanceTimeList& aArray, TestFunctor& aTest);

  
  
  

  nsresult          SetBeginSpec(const nsAString& aBeginSpec,
                                 Element* aContextNode,
                                 RemovalTestFunction aRemove);
  nsresult          SetEndSpec(const nsAString& aEndSpec,
                               Element* aContextNode,
                               RemovalTestFunction aRemove);
  nsresult          SetSimpleDuration(const nsAString& aDurSpec);
  nsresult          SetMin(const nsAString& aMinSpec);
  nsresult          SetMax(const nsAString& aMaxSpec);
  nsresult          SetRestart(const nsAString& aRestartSpec);
  nsresult          SetRepeatCount(const nsAString& aRepeatCountSpec);
  nsresult          SetRepeatDur(const nsAString& aRepeatDurSpec);
  nsresult          SetFillMode(const nsAString& aFillModeSpec);

  void              UnsetBeginSpec(RemovalTestFunction aRemove);
  void              UnsetEndSpec(RemovalTestFunction aRemove);
  void              UnsetSimpleDuration();
  void              UnsetMin();
  void              UnsetMax();
  void              UnsetRestart();
  void              UnsetRepeatCount();
  void              UnsetRepeatDur();
  void              UnsetFillMode();

  nsresult          SetBeginOrEndSpec(const nsAString& aSpec,
                                      Element* aContextNode,
                                      PRBool aIsBegin,
                                      RemovalTestFunction aRemove);
  void              ClearSpecs(TimeValueSpecList& aSpecs,
                               InstanceTimeList& aInstances,
                               RemovalTestFunction aRemove);
  void              ClearIntervalProgress();
  void              DoSampleAt(nsSMILTime aContainerTime, PRBool aEndOnly);

  













  PRBool ApplyEarlyEnd(const nsSMILTimeValue& aSampleTime);

  




  void Reset();

  




  void DoPostSeek();

  




  void UnpreserveInstanceTimes(InstanceTimeList& aList);

  





  void FilterHistory();

  
  
  void FilterIntervals();
  void FilterInstanceTimes(InstanceTimeList& aList);

  





















  PRBool            GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    const nsSMILInterval* aReplacedInterval,
                                    const nsSMILInstanceTime* aFixedBeginTime,
                                    nsSMILInterval& aResult) const;
  nsSMILInstanceTime* GetNextGreater(const InstanceTimeList& aList,
                                     const nsSMILTimeValue& aBase,
                                     PRInt32& aPosition) const;
  nsSMILInstanceTime* GetNextGreaterOrEqual(const InstanceTimeList& aList,
                                            const nsSMILTimeValue& aBase,
                                            PRInt32& aPosition) const;
  nsSMILTimeValue   CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd) const;
  nsSMILTimeValue   GetRepeatDuration() const;
  nsSMILTimeValue   ApplyMinAndMax(const nsSMILTimeValue& aDuration) const;
  nsSMILTime        ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           PRUint32& aRepeatIteration);
  nsSMILInstanceTime* CheckForEarlyEnd(
                        const nsSMILTimeValue& aContainerTime) const;
  void              UpdateCurrentInterval(PRBool aForceChangeNotice = PR_FALSE);
  void              SampleSimpleTime(nsSMILTime aActiveTime);
  void              SampleFillValue();
  void              AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
                        double aOffsetSeconds, PRBool aIsBegin);
  void              RegisterMilestone();
  PRBool            GetNextMilestone(nsSMILMilestone& aNextMilestone) const;

  
  
  
  
  
  
  void              NotifyNewInterval();
  void              NotifyChangedInterval(nsSMILInterval* aInterval,
                                          PRBool aBeginObjectChanged,
                                          PRBool aEndObjectChanged);

  void              FireTimeEventAsync(PRUint32 aMsg, PRInt32 aDetail);
  const nsSMILInstanceTime* GetEffectiveBeginInstance() const;
  const nsSMILInterval* GetPreviousInterval() const;
  PRBool            HasPlayed() const { return !mOldIntervals.IsEmpty(); }
  PRBool            EndHasEventConditions() const;

  
  
  
  void ResetCurrentInterval()
  {
    if (mCurrentInterval) {
      
      nsAutoPtr<nsSMILInterval> interval(mCurrentInterval);
      interval->Unlink();
    }
  }

  
  PR_STATIC_CALLBACK(PLDHashOperator) NotifyNewIntervalCallback(
      TimeValueSpecPtrKey* aKey, void* aData);

  
  
  
  nsISMILAnimationElement*        mAnimationElement; 
                                                     
  TimeValueSpecList               mBeginSpecs; 
  TimeValueSpecList               mEndSpecs; 

  nsSMILTimeValue                 mSimpleDur;

  nsSMILRepeatCount               mRepeatCount;
  nsSMILTimeValue                 mRepeatDur;

  nsSMILTimeValue                 mMin;
  nsSMILTimeValue                 mMax;

  enum nsSMILFillMode
  {
    FILL_REMOVE,
    FILL_FREEZE
  };
  nsSMILFillMode                  mFillMode;
  static nsAttrValue::EnumTable   sFillModeTable[];

  enum nsSMILRestartMode
  {
    RESTART_ALWAYS,
    RESTART_WHENNOTACTIVE,
    RESTART_NEVER
  };
  nsSMILRestartMode               mRestartMode;
  static nsAttrValue::EnumTable   sRestartModeTable[];

  InstanceTimeList                mBeginInstances;
  InstanceTimeList                mEndInstances;
  PRUint32                        mInstanceSerialIndex;

  nsSMILAnimationFunction*        mClient;
  nsAutoPtr<nsSMILInterval>       mCurrentInterval;
  IntervalList                    mOldIntervals;
  PRUint32                        mCurrentRepeatIteration;
  nsSMILMilestone                 mPrevRegisteredMilestone;
  static const nsSMILMilestone    sMaxMilestone;
  static const PRUint8            sMaxNumIntervals;
  static const PRUint8            sMaxNumInstanceTimes;

  
  
  
  
  
  
  TimeValueSpecHashSet mTimeDependents;

  



  enum nsSMILElementState
  {
    STATE_STARTUP,
    STATE_WAITING,
    STATE_ACTIVE,
    STATE_POSTACTIVE
  };
  nsSMILElementState              mElementState;

  enum nsSMILSeekState
  {
    SEEK_NOT_SEEKING,
    SEEK_FORWARD_FROM_ACTIVE,
    SEEK_FORWARD_FROM_INACTIVE,
    SEEK_BACKWARD_FROM_ACTIVE,
    SEEK_BACKWARD_FROM_INACTIVE
  };
  nsSMILSeekState                 mSeekState;
};

#endif 
