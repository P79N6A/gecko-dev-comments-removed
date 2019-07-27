




#ifndef NS_SMILTIMEDELEMENT_H_
#define NS_SMILTIMEDELEMENT_H_

#include "mozilla/Move.h"
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

class nsSMILAnimationFunction;
class nsSMILTimeContainer;
class nsSMILTimeValue;
class nsIAtom;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}




class nsSMILTimedElement
{
public:
  nsSMILTimedElement();
  ~nsSMILTimedElement();

  typedef mozilla::dom::Element Element;

  



  void SetAnimationElement(mozilla::dom::SVGAnimationElement* aElement);

  



  nsSMILTimeContainer* GetTimeContainer();

  



  mozilla::dom::Element* GetTargetElement();

  



  







  nsresult BeginElementAt(double aOffsetSeconds);

  







  nsresult EndElementAt(double aOffsetSeconds);

  



  








  nsSMILTimeValue GetStartTime() const;

  




  nsSMILTimeValue GetSimpleDuration() const
  {
    return mSimpleDur;
  }

  



  



  












  nsSMILTimeValue GetHyperlinkTime() const;

  









  void AddInstanceTime(nsSMILInstanceTime* aInstanceTime, bool aIsBegin);

  












  void UpdateInstanceTime(nsSMILInstanceTime* aInstanceTime,
                          nsSMILTimeValue& aUpdatedTime,
                          bool aIsBegin);

  








  void RemoveInstanceTime(nsSMILInstanceTime* aInstanceTime, bool aIsBegin);

  









  void RemoveInstanceTimesForCreator(const nsSMILTimeValueSpec* aSpec,
                                     bool aIsBegin);

  











  void SetTimeClient(nsSMILAnimationFunction* aClient);

  






  void SampleAt(nsSMILTime aContainerTime);

  











  void SampleEndAt(nsSMILTime aContainerTime);

  





  void HandleContainerTimeChange();

  







  void Rewind();

  









  bool SetIsDisabled(bool aIsDisabled);

  


















  bool SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                 nsAttrValue& aResult, Element* aContextNode,
                 nsresult* aParseResult = nullptr);

  









  bool UnsetAttr(nsIAtom* aAttribute);

  








  void AddDependent(nsSMILTimeValueSpec& aDependent);

  





  void RemoveDependent(nsSMILTimeValueSpec& aDependent);

  











  bool IsTimeDependent(const nsSMILTimedElement& aOther) const;

  








  void BindToTree(nsIContent* aContextNode);

  



  void HandleTargetElementChange(mozilla::dom::Element* aNewTarget);

  



  void DissolveReferences() { Unlink(); }

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);
  void Unlink();

  typedef bool (*RemovalTestFunction)(nsSMILInstanceTime* aInstance);

protected:
  
  typedef nsTArray<nsAutoPtr<nsSMILTimeValueSpec> > TimeValueSpecList;
  typedef nsTArray<nsRefPtr<nsSMILInstanceTime> >   InstanceTimeList;
  typedef nsTArray<nsAutoPtr<nsSMILInterval> >      IntervalList;
  typedef nsPtrHashKey<nsSMILTimeValueSpec> TimeValueSpecPtrKey;
  typedef nsTHashtable<TimeValueSpecPtrKey> TimeValueSpecHashSet;

  
  class InstanceTimeComparator {
    public:
      bool Equals(const nsSMILInstanceTime* aElem1,
                    const nsSMILInstanceTime* aElem2) const;
      bool LessThan(const nsSMILInstanceTime* aElem1,
                      const nsSMILInstanceTime* aElem2) const;
  };

  struct NotifyTimeDependentsParams {
    nsSMILTimedElement*  mTimedElement;
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
                                      bool aIsBegin,
                                      RemovalTestFunction aRemove);
  void              ClearSpecs(TimeValueSpecList& aSpecs,
                               InstanceTimeList& aInstances,
                               RemovalTestFunction aRemove);
  void              ClearIntervals();
  void              DoSampleAt(nsSMILTime aContainerTime, bool aEndOnly);

  













  bool ApplyEarlyEnd(const nsSMILTimeValue& aSampleTime);

  




  void Reset();

  









  void ClearTimingState(RemovalTestFunction aRemove);

  







  void RebuildTimingState(RemovalTestFunction aRemove);

  




  void DoPostSeek();

  




  void UnpreserveInstanceTimes(InstanceTimeList& aList);

  





  void FilterHistory();

  
  
  void FilterIntervals();
  void FilterInstanceTimes(InstanceTimeList& aList);

  





















  bool              GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    const nsSMILInterval* aReplacedInterval,
                                    const nsSMILInstanceTime* aFixedBeginTime,
                                    nsSMILInterval& aResult) const;
  nsSMILInstanceTime* GetNextGreater(const InstanceTimeList& aList,
                                     const nsSMILTimeValue& aBase,
                                     int32_t& aPosition) const;
  nsSMILInstanceTime* GetNextGreaterOrEqual(const InstanceTimeList& aList,
                                            const nsSMILTimeValue& aBase,
                                            int32_t& aPosition) const;
  nsSMILTimeValue   CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd) const;
  nsSMILTimeValue   GetRepeatDuration() const;
  nsSMILTimeValue   ApplyMinAndMax(const nsSMILTimeValue& aDuration) const;
  nsSMILTime        ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           uint32_t& aRepeatIteration);
  nsSMILInstanceTime* CheckForEarlyEnd(
                        const nsSMILTimeValue& aContainerTime) const;
  void              UpdateCurrentInterval(bool aForceChangeNotice = false);
  void              SampleSimpleTime(nsSMILTime aActiveTime);
  void              SampleFillValue();
  nsresult          AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
                        double aOffsetSeconds, bool aIsBegin);
  void              RegisterMilestone();
  bool              GetNextMilestone(nsSMILMilestone& aNextMilestone) const;

  
  
  
  
  
  
  void              NotifyNewInterval();
  void              NotifyChangedInterval(nsSMILInterval* aInterval,
                                          bool aBeginObjectChanged,
                                          bool aEndObjectChanged);

  void              FireTimeEventAsync(uint32_t aMsg, int32_t aDetail);
  const nsSMILInstanceTime* GetEffectiveBeginInstance() const;
  const nsSMILInterval* GetPreviousInterval() const;
  bool              HasPlayed() const { return !mOldIntervals.IsEmpty(); }
  bool              HasClientInFillRange() const;
  bool              EndHasEventConditions() const;
  bool              AreEndTimesDependentOn(
                      const nsSMILInstanceTime* aBase) const;

  
  
  
  void ResetCurrentInterval()
  {
    if (mCurrentInterval) {
      
      nsAutoPtr<nsSMILInterval> interval(mozilla::Move(mCurrentInterval));
      interval->Unlink();
    }
  }

  
  static PLDHashOperator NotifyNewIntervalCallback(
      TimeValueSpecPtrKey* aKey, void* aData);

  
  
  
  mozilla::dom::SVGAnimationElement* mAnimationElement; 
                                                        
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
  uint32_t                        mInstanceSerialIndex;

  nsSMILAnimationFunction*        mClient;
  nsAutoPtr<nsSMILInterval>       mCurrentInterval;
  IntervalList                    mOldIntervals;
  uint32_t                        mCurrentRepeatIteration;
  nsSMILMilestone                 mPrevRegisteredMilestone;
  static const nsSMILMilestone    sMaxMilestone;
  static const uint8_t            sMaxNumIntervals;
  static const uint8_t            sMaxNumInstanceTimes;

  
  
  
  
  
  
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

  
  class AutoIntervalUpdateBatcher;
  bool mDeferIntervalUpdates;
  bool mDoDeferredUpdate; 
                          
  bool mIsDisabled;

  
  class AutoIntervalUpdater;

  
  uint8_t              mDeleteCount;
  uint8_t              mUpdateIntervalRecursionDepth;
  static const uint8_t sMaxUpdateIntervalRecursionDepth;
};

inline void
ImplCycleCollectionUnlink(nsSMILTimedElement& aField)
{
  aField.Unlink();
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsSMILTimedElement& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aField.Traverse(&aCallback);
}

#endif 
