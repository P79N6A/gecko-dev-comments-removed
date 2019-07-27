




#include "mozilla/DebugOnly.h"

#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILTimedElement.h"
#include "nsAttrValueInlines.h"
#include "nsSMILAnimationFunction.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsSMILTimeContainer.h"
#include "nsGkAtoms.h"
#include "nsReadableUtils.h"
#include "nsMathUtils.h"
#include "nsThreadUtils.h"
#include "nsIPresShell.h"
#include "prdtoa.h"
#include "plstr.h"
#include "prtime.h"
#include "nsString.h"
#include "mozilla/AutoRestore.h"
#include "nsCharSeparatedTokenizer.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;












bool
nsSMILTimedElement::InstanceTimeComparator::Equals(
    const nsSMILInstanceTime* aElem1,
    const nsSMILInstanceTime* aElem2) const
{
  NS_ABORT_IF_FALSE(aElem1 && aElem2,
      "Trying to compare null instance time pointers");
  NS_ABORT_IF_FALSE(aElem1->Serial() && aElem2->Serial(),
      "Instance times have not been assigned serial numbers");
  NS_ABORT_IF_FALSE(aElem1 == aElem2 || aElem1->Serial() != aElem2->Serial(),
      "Serial numbers are not unique");

  return aElem1->Serial() == aElem2->Serial();
}

bool
nsSMILTimedElement::InstanceTimeComparator::LessThan(
    const nsSMILInstanceTime* aElem1,
    const nsSMILInstanceTime* aElem2) const
{
  NS_ABORT_IF_FALSE(aElem1 && aElem2,
      "Trying to compare null instance time pointers");
  NS_ABORT_IF_FALSE(aElem1->Serial() && aElem2->Serial(),
      "Instance times have not been assigned serial numbers");

  int8_t cmp = aElem1->Time().CompareTo(aElem2->Time());
  return cmp == 0 ? aElem1->Serial() < aElem2->Serial() : cmp < 0;
}




namespace
{
  class AsyncTimeEventRunner : public nsRunnable
  {
  protected:
    nsRefPtr<nsIContent> mTarget;
    uint32_t             mMsg;
    int32_t              mDetail;

  public:
    AsyncTimeEventRunner(nsIContent* aTarget, uint32_t aMsg, int32_t aDetail)
      : mTarget(aTarget), mMsg(aMsg), mDetail(aDetail)
    {
    }

    NS_IMETHOD Run()
    {
      InternalSMILTimeEvent event(true, mMsg);
      event.detail = mDetail;

      nsPresContext* context = nullptr;
      nsIDocument* doc = mTarget->GetCurrentDoc();
      if (doc) {
        nsCOMPtr<nsIPresShell> shell = doc->GetShell();
        if (shell) {
          context = shell->GetPresContext();
        }
      }

      return EventDispatcher::Dispatch(mTarget, context, &event);
    }
  };
}











class MOZ_STACK_CLASS nsSMILTimedElement::AutoIntervalUpdateBatcher
{
public:
  explicit AutoIntervalUpdateBatcher(nsSMILTimedElement& aTimedElement)
    : mTimedElement(aTimedElement),
      mDidSetFlag(!aTimedElement.mDeferIntervalUpdates)
  {
    mTimedElement.mDeferIntervalUpdates = true;
  }

  ~AutoIntervalUpdateBatcher()
  {
    if (!mDidSetFlag)
      return;

    mTimedElement.mDeferIntervalUpdates = false;

    if (mTimedElement.mDoDeferredUpdate) {
      mTimedElement.mDoDeferredUpdate = false;
      mTimedElement.UpdateCurrentInterval();
    }
  }

private:
  nsSMILTimedElement& mTimedElement;
  bool mDidSetFlag;
};











class MOZ_STACK_CLASS nsSMILTimedElement::AutoIntervalUpdater
{
public:
  explicit AutoIntervalUpdater(nsSMILTimedElement& aTimedElement)
    : mTimedElement(aTimedElement) { }

  ~AutoIntervalUpdater()
  {
    mTimedElement.UpdateCurrentInterval();
  }

private:
  nsSMILTimedElement& mTimedElement;
};






template <class TestFunctor>
void
nsSMILTimedElement::RemoveInstanceTimes(InstanceTimeList& aArray,
                                        TestFunctor& aTest)
{
  InstanceTimeList newArray;
  for (uint32_t i = 0; i < aArray.Length(); ++i) {
    nsSMILInstanceTime* item = aArray[i].get();
    if (aTest(item, i)) {
      
      
      
      
      
      
      
      
      
      NS_ABORT_IF_FALSE(!GetPreviousInterval() ||
        item != GetPreviousInterval()->End(),
        "Removing end instance time of previous interval");
      item->Unlink();
    } else {
      newArray.AppendElement(item);
    }
  }
  aArray.Clear();
  aArray.SwapElements(newArray);
}




nsAttrValue::EnumTable nsSMILTimedElement::sFillModeTable[] = {
      {"remove", FILL_REMOVE},
      {"freeze", FILL_FREEZE},
      {nullptr, 0}
};

nsAttrValue::EnumTable nsSMILTimedElement::sRestartModeTable[] = {
      {"always", RESTART_ALWAYS},
      {"whenNotActive", RESTART_WHENNOTACTIVE},
      {"never", RESTART_NEVER},
      {nullptr, 0}
};

const nsSMILMilestone nsSMILTimedElement::sMaxMilestone(INT64_MAX, false);




const uint8_t nsSMILTimedElement::sMaxNumIntervals = 20;
const uint8_t nsSMILTimedElement::sMaxNumInstanceTimes = 100;



const uint8_t nsSMILTimedElement::sMaxUpdateIntervalRecursionDepth = 20;




nsSMILTimedElement::nsSMILTimedElement()
:
  mAnimationElement(nullptr),
  mFillMode(FILL_REMOVE),
  mRestartMode(RESTART_ALWAYS),
  mInstanceSerialIndex(0),
  mClient(nullptr),
  mCurrentInterval(nullptr),
  mCurrentRepeatIteration(0),
  mPrevRegisteredMilestone(sMaxMilestone),
  mElementState(STATE_STARTUP),
  mSeekState(SEEK_NOT_SEEKING),
  mDeferIntervalUpdates(false),
  mDoDeferredUpdate(false),
  mIsDisabled(false),
  mDeleteCount(0),
  mUpdateIntervalRecursionDepth(0)
{
  mSimpleDur.SetIndefinite();
  mMin.SetMillis(0L);
  mMax.SetIndefinite();
}

nsSMILTimedElement::~nsSMILTimedElement()
{
  
  for (uint32_t i = 0; i < mBeginInstances.Length(); ++i) {
    mBeginInstances[i]->Unlink();
  }
  mBeginInstances.Clear();
  for (uint32_t i = 0; i < mEndInstances.Length(); ++i) {
    mEndInstances[i]->Unlink();
  }
  mEndInstances.Clear();

  
  
  
  ClearIntervals();

  
  
  
  
  NS_ABORT_IF_FALSE(!mDeferIntervalUpdates,
      "Interval updates should no longer be blocked when an nsSMILTimedElement "
      "disappears");
  NS_ABORT_IF_FALSE(!mDoDeferredUpdate,
      "There should no longer be any pending updates when an "
      "nsSMILTimedElement disappears");
}

void
nsSMILTimedElement::SetAnimationElement(SVGAnimationElement* aElement)
{
  NS_ABORT_IF_FALSE(aElement, "NULL owner element");
  NS_ABORT_IF_FALSE(!mAnimationElement, "Re-setting owner");
  mAnimationElement = aElement;
}

nsSMILTimeContainer*
nsSMILTimedElement::GetTimeContainer()
{
  return mAnimationElement ? mAnimationElement->GetTimeContainer() : nullptr;
}

dom::Element*
nsSMILTimedElement::GetTargetElement()
{
  return mAnimationElement ?
      mAnimationElement->GetTargetElementContent() :
      nullptr;
}

















nsresult
nsSMILTimedElement::BeginElementAt(double aOffsetSeconds)
{
  nsSMILTimeContainer* container = GetTimeContainer();
  if (!container)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = container->GetCurrentTime();
  return AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, true);
}

nsresult
nsSMILTimedElement::EndElementAt(double aOffsetSeconds)
{
  nsSMILTimeContainer* container = GetTimeContainer();
  if (!container)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = container->GetCurrentTime();
  return AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, false);
}




nsSMILTimeValue
nsSMILTimedElement::GetStartTime() const
{
  return mElementState == STATE_WAITING || mElementState == STATE_ACTIVE
         ? mCurrentInterval->Begin()->Time()
         : nsSMILTimeValue();
}




nsSMILTimeValue
nsSMILTimedElement::GetHyperlinkTime() const
{
  nsSMILTimeValue hyperlinkTime; 

  if (mElementState == STATE_ACTIVE) {
    hyperlinkTime = mCurrentInterval->Begin()->Time();
  } else if (!mBeginInstances.IsEmpty()) {
    hyperlinkTime = mBeginInstances[0]->Time();
  }

  return hyperlinkTime;
}




void
nsSMILTimedElement::AddInstanceTime(nsSMILInstanceTime* aInstanceTime,
                                    bool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to add null instance time");

  
  
  if (mElementState != STATE_ACTIVE && !aIsBegin &&
      aInstanceTime->IsDynamic())
  {
    
    
    NS_ABORT_IF_FALSE(!aInstanceTime->GetBaseInterval(),
        "Dynamic instance time has a base interval--we probably need to unlink"
        " it if we're not going to use it");
    return;
  }

  aInstanceTime->SetSerial(++mInstanceSerialIndex);
  InstanceTimeList& instanceList = aIsBegin ? mBeginInstances : mEndInstances;
  nsRefPtr<nsSMILInstanceTime>* inserted =
    instanceList.InsertElementSorted(aInstanceTime, InstanceTimeComparator());
  if (!inserted) {
    NS_WARNING("Insufficient memory to insert instance time");
    return;
  }

  UpdateCurrentInterval();
}

void
nsSMILTimedElement::UpdateInstanceTime(nsSMILInstanceTime* aInstanceTime,
                                       nsSMILTimeValue& aUpdatedTime,
                                       bool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to update null instance time");

  
  
  
  
  aInstanceTime->DependentUpdate(aUpdatedTime);
  InstanceTimeList& instanceList = aIsBegin ? mBeginInstances : mEndInstances;
  instanceList.Sort(InstanceTimeComparator());

  
  
  
  
  
  
  
  
  
  
  
  bool changedCurrentInterval = mCurrentInterval &&
    (mCurrentInterval->Begin() == aInstanceTime ||
     mCurrentInterval->End() == aInstanceTime);

  UpdateCurrentInterval(changedCurrentInterval);
}

void
nsSMILTimedElement::RemoveInstanceTime(nsSMILInstanceTime* aInstanceTime,
                                       bool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to remove null instance time");

  
  
  if (aInstanceTime->ShouldPreserve()) {
    aInstanceTime->Unlink();
    return;
  }

  InstanceTimeList& instanceList = aIsBegin ? mBeginInstances : mEndInstances;
  mozilla::DebugOnly<bool> found =
    instanceList.RemoveElementSorted(aInstanceTime, InstanceTimeComparator());
  NS_ABORT_IF_FALSE(found, "Couldn't find instance time to delete");

  UpdateCurrentInterval();
}

namespace
{
  class MOZ_STACK_CLASS RemoveByCreator
  {
  public:
    explicit RemoveByCreator(const nsSMILTimeValueSpec* aCreator) : mCreator(aCreator)
    { }

    bool operator()(nsSMILInstanceTime* aInstanceTime, uint32_t )
    {
      if (aInstanceTime->GetCreator() != mCreator)
        return false;

      
      
      if (aInstanceTime->ShouldPreserve()) {
        aInstanceTime->Unlink();
        return false;
      }

      return true;
    }

  private:
    const nsSMILTimeValueSpec* mCreator;
  };
}

void
nsSMILTimedElement::RemoveInstanceTimesForCreator(
    const nsSMILTimeValueSpec* aCreator, bool aIsBegin)
{
  NS_ABORT_IF_FALSE(aCreator, "Creator not set");

  InstanceTimeList& instances = aIsBegin ? mBeginInstances : mEndInstances;
  RemoveByCreator removeByCreator(aCreator);
  RemoveInstanceTimes(instances, removeByCreator);

  UpdateCurrentInterval();
}

void
nsSMILTimedElement::SetTimeClient(nsSMILAnimationFunction* aClient)
{
  
  
  
  

  mClient = aClient;
}

void
nsSMILTimedElement::SampleAt(nsSMILTime aContainerTime)
{
  if (mIsDisabled)
    return;

  
  mPrevRegisteredMilestone = sMaxMilestone;

  DoSampleAt(aContainerTime, false);
}

void
nsSMILTimedElement::SampleEndAt(nsSMILTime aContainerTime)
{
  if (mIsDisabled)
    return;

  
  mPrevRegisteredMilestone = sMaxMilestone;

  
  
  
  
  
  
  
  
  
  if (mElementState == STATE_ACTIVE || mElementState == STATE_STARTUP) {
    DoSampleAt(aContainerTime, true); 
  } else {
    
    
    RegisterMilestone();
  }
}

void
nsSMILTimedElement::DoSampleAt(nsSMILTime aContainerTime, bool aEndOnly)
{
  NS_ABORT_IF_FALSE(mAnimationElement,
      "Got sample before being registered with an animation element");
  NS_ABORT_IF_FALSE(GetTimeContainer(),
      "Got sample without being registered with a time container");

  
  
  
  
  
  if (GetTimeContainer()->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN))
    return;

  
  
  
  
  
  
  
  
  
  
  
  if (mElementState == STATE_STARTUP && !aEndOnly)
    return;

  bool finishedSeek = false;
  if (GetTimeContainer()->IsSeeking() && mSeekState == SEEK_NOT_SEEKING) {
    mSeekState = mElementState == STATE_ACTIVE ?
                 SEEK_FORWARD_FROM_ACTIVE :
                 SEEK_FORWARD_FROM_INACTIVE;
  } else if (mSeekState != SEEK_NOT_SEEKING &&
             !GetTimeContainer()->IsSeeking()) {
    finishedSeek = true;
  }

  bool            stateChanged;
  nsSMILTimeValue sampleTime(aContainerTime);

  do {
#ifdef DEBUG
    
    if (mElementState == STATE_STARTUP || mElementState == STATE_POSTACTIVE) {
      NS_ABORT_IF_FALSE(!mCurrentInterval,
          "Shouldn't have current interval in startup or postactive states");
    } else {
      NS_ABORT_IF_FALSE(mCurrentInterval,
          "Should have current interval in waiting and active states");
    }
#endif

    stateChanged = false;

    switch (mElementState)
    {
    case STATE_STARTUP:
      {
        nsSMILInterval firstInterval;
        mElementState = GetNextInterval(nullptr, nullptr, nullptr, firstInterval)
         ? STATE_WAITING
         : STATE_POSTACTIVE;
        stateChanged = true;
        if (mElementState == STATE_WAITING) {
          mCurrentInterval = new nsSMILInterval(firstInterval);
          NotifyNewInterval();
        }
      }
      break;

    case STATE_WAITING:
      {
        if (mCurrentInterval->Begin()->Time() <= sampleTime) {
          mElementState = STATE_ACTIVE;
          mCurrentInterval->FixBegin();
          if (mClient) {
            mClient->Activate(mCurrentInterval->Begin()->Time().GetMillis());
          }
          if (mSeekState == SEEK_NOT_SEEKING) {
            FireTimeEventAsync(NS_SMIL_BEGIN, 0);
          }
          if (HasPlayed()) {
            Reset(); 
            
            
            
            
            
            
            
            UpdateCurrentInterval();
          }
          stateChanged = true;
        }
      }
      break;

    case STATE_ACTIVE:
      {
        
        
        
        bool didApplyEarlyEnd = ApplyEarlyEnd(sampleTime);

        if (mCurrentInterval->End()->Time() <= sampleTime) {
          nsSMILInterval newInterval;
          mElementState =
            GetNextInterval(mCurrentInterval, nullptr, nullptr, newInterval)
            ? STATE_WAITING
            : STATE_POSTACTIVE;
          if (mClient) {
            mClient->Inactivate(mFillMode == FILL_FREEZE);
          }
          mCurrentInterval->FixEnd();
          if (mSeekState == SEEK_NOT_SEEKING) {
            FireTimeEventAsync(NS_SMIL_END, 0);
          }
          mCurrentRepeatIteration = 0;
          mOldIntervals.AppendElement(mCurrentInterval.forget());
          SampleFillValue();
          if (mElementState == STATE_WAITING) {
            mCurrentInterval = new nsSMILInterval(newInterval);
          }
          
          if (didApplyEarlyEnd) {
            NotifyChangedInterval(
                mOldIntervals[mOldIntervals.Length() - 1], false, true);
          }
          if (mElementState == STATE_WAITING) {
            NotifyNewInterval();
          }
          FilterHistory();
          stateChanged = true;
        } else {
          NS_ABORT_IF_FALSE(!didApplyEarlyEnd,
              "We got an early end, but didn't end");
          nsSMILTime beginTime = mCurrentInterval->Begin()->Time().GetMillis();
          NS_ASSERTION(aContainerTime >= beginTime,
                       "Sample time should not precede current interval");
          nsSMILTime activeTime = aContainerTime - beginTime;

          
          
          
          if (GetRepeatDuration() <= nsSMILTimeValue(activeTime)) {
            if (mClient && mClient->IsActive()) {
              mClient->Inactivate(mFillMode == FILL_FREEZE);
            }
            SampleFillValue();
          } else {
            SampleSimpleTime(activeTime);

            
            
            
            
            
            uint32_t prevRepeatIteration = mCurrentRepeatIteration;
            if (
              ActiveTimeToSimpleTime(activeTime, mCurrentRepeatIteration)==0 &&
              mCurrentRepeatIteration != prevRepeatIteration &&
              mCurrentRepeatIteration &&
              mSeekState == SEEK_NOT_SEEKING) {
              FireTimeEventAsync(NS_SMIL_REPEAT,
                            static_cast<int32_t>(mCurrentRepeatIteration));
            }
          }
        }
      }
      break;

    case STATE_POSTACTIVE:
      break;
    }

  
  
  
  
  
  
  } while (stateChanged && (!aEndOnly || (mElementState != STATE_WAITING &&
                                          mElementState != STATE_POSTACTIVE)));

  if (finishedSeek) {
    DoPostSeek();
  }
  RegisterMilestone();
}

void
nsSMILTimedElement::HandleContainerTimeChange()
{
  
  
  
  
  
  if (mElementState == STATE_WAITING || mElementState == STATE_ACTIVE) {
    NotifyChangedInterval(mCurrentInterval, false, false);
  }
}

namespace
{
  bool
  RemoveNonDynamic(nsSMILInstanceTime* aInstanceTime)
  {
    
    
    
    NS_ABORT_IF_FALSE(!aInstanceTime->IsDynamic() ||
         !aInstanceTime->GetCreator(),
        "Dynamic instance time should be unlinked from its creator");
    return !aInstanceTime->IsDynamic() && !aInstanceTime->ShouldPreserve();
  }
}

void
nsSMILTimedElement::Rewind()
{
  NS_ABORT_IF_FALSE(mAnimationElement,
      "Got rewind request before being attached to an animation element");

  
  
  
  
  
  
  
  
  
  
  if (mSeekState == SEEK_NOT_SEEKING) {
    mSeekState = mElementState == STATE_ACTIVE ?
                 SEEK_BACKWARD_FROM_ACTIVE :
                 SEEK_BACKWARD_FROM_INACTIVE;
  }
  NS_ABORT_IF_FALSE(mSeekState == SEEK_BACKWARD_FROM_INACTIVE ||
                    mSeekState == SEEK_BACKWARD_FROM_ACTIVE,
                    "Rewind in the middle of a forwards seek?");

  ClearTimingState(RemoveNonDynamic);
  RebuildTimingState(RemoveNonDynamic);

  NS_ABORT_IF_FALSE(!mCurrentInterval,
                    "Current interval is set at end of rewind");
}

namespace
{
  bool
  RemoveAll(nsSMILInstanceTime* aInstanceTime)
  {
    return true;
  }
}

bool
nsSMILTimedElement::SetIsDisabled(bool aIsDisabled)
{
  if (mIsDisabled == aIsDisabled)
    return false;

  if (aIsDisabled) {
    mIsDisabled = true;
    ClearTimingState(RemoveAll);
  } else {
    RebuildTimingState(RemoveAll);
    mIsDisabled = false;
  }
  return true;
}

namespace
{
  bool
  RemoveNonDOM(nsSMILInstanceTime* aInstanceTime)
  {
    return !aInstanceTime->FromDOM() && !aInstanceTime->ShouldPreserve();
  }
}

bool
nsSMILTimedElement::SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                            nsAttrValue& aResult,
                            Element* aContextNode,
                            nsresult* aParseResult)
{
  bool foundMatch = true;
  nsresult parseResult = NS_OK;

  if (aAttribute == nsGkAtoms::begin) {
    parseResult = SetBeginSpec(aValue, aContextNode, RemoveNonDOM);
  } else if (aAttribute == nsGkAtoms::dur) {
    parseResult = SetSimpleDuration(aValue);
  } else if (aAttribute == nsGkAtoms::end) {
    parseResult = SetEndSpec(aValue, aContextNode, RemoveNonDOM);
  } else if (aAttribute == nsGkAtoms::fill) {
    parseResult = SetFillMode(aValue);
  } else if (aAttribute == nsGkAtoms::max) {
    parseResult = SetMax(aValue);
  } else if (aAttribute == nsGkAtoms::min) {
    parseResult = SetMin(aValue);
  } else if (aAttribute == nsGkAtoms::repeatCount) {
    parseResult = SetRepeatCount(aValue);
  } else if (aAttribute == nsGkAtoms::repeatDur) {
    parseResult = SetRepeatDur(aValue);
  } else if (aAttribute == nsGkAtoms::restart) {
    parseResult = SetRestart(aValue);
  } else {
    foundMatch = false;
  }

  if (foundMatch) {
    aResult.SetTo(aValue);
    if (aParseResult) {
      *aParseResult = parseResult;
    }
  }

  return foundMatch;
}

bool
nsSMILTimedElement::UnsetAttr(nsIAtom* aAttribute)
{
  bool foundMatch = true;

  if (aAttribute == nsGkAtoms::begin) {
    UnsetBeginSpec(RemoveNonDOM);
  } else if (aAttribute == nsGkAtoms::dur) {
    UnsetSimpleDuration();
  } else if (aAttribute == nsGkAtoms::end) {
    UnsetEndSpec(RemoveNonDOM);
  } else if (aAttribute == nsGkAtoms::fill) {
    UnsetFillMode();
  } else if (aAttribute == nsGkAtoms::max) {
    UnsetMax();
  } else if (aAttribute == nsGkAtoms::min) {
    UnsetMin();
  } else if (aAttribute == nsGkAtoms::repeatCount) {
    UnsetRepeatCount();
  } else if (aAttribute == nsGkAtoms::repeatDur) {
    UnsetRepeatDur();
  } else if (aAttribute == nsGkAtoms::restart) {
    UnsetRestart();
  } else {
    foundMatch = false;
  }

  return foundMatch;
}




nsresult
nsSMILTimedElement::SetBeginSpec(const nsAString& aBeginSpec,
                                 Element* aContextNode,
                                 RemovalTestFunction aRemove)
{
  return SetBeginOrEndSpec(aBeginSpec, aContextNode, true ,
                           aRemove);
}

void
nsSMILTimedElement::UnsetBeginSpec(RemovalTestFunction aRemove)
{
  ClearSpecs(mBeginSpecs, mBeginInstances, aRemove);
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetEndSpec(const nsAString& aEndSpec,
                               Element* aContextNode,
                               RemovalTestFunction aRemove)
{
  return SetBeginOrEndSpec(aEndSpec, aContextNode, false ,
                           aRemove);
}

void
nsSMILTimedElement::UnsetEndSpec(RemovalTestFunction aRemove)
{
  ClearSpecs(mEndSpecs, mEndInstances, aRemove);
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetSimpleDuration(const nsAString& aDurSpec)
{
  
  AutoIntervalUpdater updater(*this);

  nsSMILTimeValue duration;
  const nsAString& dur = nsSMILParserUtils::TrimWhitespace(aDurSpec);

  
  
  if (dur.EqualsLiteral("media") || dur.EqualsLiteral("indefinite")) {
    duration.SetIndefinite();
  } else {
    if (!nsSMILParserUtils::ParseClockValue(dur, &duration) ||
        duration.GetMillis() == 0L) {
      mSimpleDur.SetIndefinite();
      return NS_ERROR_FAILURE;
    }
  }
  
  
  NS_ABORT_IF_FALSE(duration.IsResolved(),
    "Setting unresolved simple duration");

  mSimpleDur = duration;

  return NS_OK;
}

void
nsSMILTimedElement::UnsetSimpleDuration()
{
  mSimpleDur.SetIndefinite();
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetMin(const nsAString& aMinSpec)
{
  
  AutoIntervalUpdater updater(*this);

  nsSMILTimeValue duration;
  const nsAString& min = nsSMILParserUtils::TrimWhitespace(aMinSpec);

  if (min.EqualsLiteral("media")) {
    duration.SetMillis(0L);
  } else {
    if (!nsSMILParserUtils::ParseClockValue(min, &duration)) {
      mMin.SetMillis(0L);
      return NS_ERROR_FAILURE;
    }
  }

  NS_ABORT_IF_FALSE(duration.GetMillis() >= 0L, "Invalid duration");

  mMin = duration;

  return NS_OK;
}

void
nsSMILTimedElement::UnsetMin()
{
  mMin.SetMillis(0L);
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetMax(const nsAString& aMaxSpec)
{
  
  AutoIntervalUpdater updater(*this);

  nsSMILTimeValue duration;
  const nsAString& max = nsSMILParserUtils::TrimWhitespace(aMaxSpec);

  if (max.EqualsLiteral("media") || max.EqualsLiteral("indefinite")) {
    duration.SetIndefinite();
  } else {
    if (!nsSMILParserUtils::ParseClockValue(max, &duration) ||
        duration.GetMillis() == 0L) {
      mMax.SetIndefinite();
      return NS_ERROR_FAILURE;
    }
    NS_ABORT_IF_FALSE(duration.GetMillis() > 0L, "Invalid duration");
  }

  mMax = duration;

  return NS_OK;
}

void
nsSMILTimedElement::UnsetMax()
{
  mMax.SetIndefinite();
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetRestart(const nsAString& aRestartSpec)
{
  nsAttrValue temp;
  bool parseResult
    = temp.ParseEnumValue(aRestartSpec, sRestartModeTable, true);
  mRestartMode = parseResult
               ? nsSMILRestartMode(temp.GetEnumValue())
               : RESTART_ALWAYS;
  UpdateCurrentInterval();
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILTimedElement::UnsetRestart()
{
  mRestartMode = RESTART_ALWAYS;
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetRepeatCount(const nsAString& aRepeatCountSpec)
{
  
  AutoIntervalUpdater updater(*this);

  nsSMILRepeatCount newRepeatCount;

  if (nsSMILParserUtils::ParseRepeatCount(aRepeatCountSpec, newRepeatCount)) {
    mRepeatCount = newRepeatCount;
    return NS_OK;
  }
  mRepeatCount.Unset();
  return NS_ERROR_FAILURE;
}

void
nsSMILTimedElement::UnsetRepeatCount()
{
  mRepeatCount.Unset();
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetRepeatDur(const nsAString& aRepeatDurSpec)
{
  
  AutoIntervalUpdater updater(*this);

  nsSMILTimeValue duration;

  const nsAString& repeatDur =
    nsSMILParserUtils::TrimWhitespace(aRepeatDurSpec);

  if (repeatDur.EqualsLiteral("indefinite")) {
    duration.SetIndefinite();
  } else {
    if (!nsSMILParserUtils::ParseClockValue(repeatDur, &duration)) {
      mRepeatDur.SetUnresolved();
      return NS_ERROR_FAILURE;
    }
  }

  mRepeatDur = duration;

  return NS_OK;
}

void
nsSMILTimedElement::UnsetRepeatDur()
{
  mRepeatDur.SetUnresolved();
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetFillMode(const nsAString& aFillModeSpec)
{
  uint16_t previousFillMode = mFillMode;

  nsAttrValue temp;
  bool parseResult =
    temp.ParseEnumValue(aFillModeSpec, sFillModeTable, true);
  mFillMode = parseResult
            ? nsSMILFillMode(temp.GetEnumValue())
            : FILL_REMOVE;

  
  if (mFillMode != previousFillMode && HasClientInFillRange()) {
    mClient->Inactivate(mFillMode == FILL_FREEZE);
    SampleFillValue();
  }

  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILTimedElement::UnsetFillMode()
{
  uint16_t previousFillMode = mFillMode;
  mFillMode = FILL_REMOVE;
  if (previousFillMode == FILL_FREEZE && HasClientInFillRange()) {
    mClient->Inactivate(false);
  }
}

void
nsSMILTimedElement::AddDependent(nsSMILTimeValueSpec& aDependent)
{
  
  
  NS_ABORT_IF_FALSE(!mTimeDependents.GetEntry(&aDependent),
      "nsSMILTimeValueSpec is already registered as a dependency");
  mTimeDependents.PutEntry(&aDependent);

  
  
  
  
  
  
  
  if (mCurrentInterval) {
    aDependent.HandleNewInterval(*mCurrentInterval, GetTimeContainer());
  }
}

void
nsSMILTimedElement::RemoveDependent(nsSMILTimeValueSpec& aDependent)
{
  mTimeDependents.RemoveEntry(&aDependent);
}

bool
nsSMILTimedElement::IsTimeDependent(const nsSMILTimedElement& aOther) const
{
  const nsSMILInstanceTime* thisBegin = GetEffectiveBeginInstance();
  const nsSMILInstanceTime* otherBegin = aOther.GetEffectiveBeginInstance();

  if (!thisBegin || !otherBegin)
    return false;

  return thisBegin->IsDependentOn(*otherBegin);
}

void
nsSMILTimedElement::BindToTree(nsIContent* aContextNode)
{
  
  
  mPrevRegisteredMilestone = sMaxMilestone;

  
  
  if (mElementState != STATE_STARTUP) {
    mSeekState = SEEK_NOT_SEEKING;
    Rewind();
  }

  
  {
    AutoIntervalUpdateBatcher updateBatcher(*this);

    
    uint32_t count = mBeginSpecs.Length();
    for (uint32_t i = 0; i < count; ++i) {
      mBeginSpecs[i]->ResolveReferences(aContextNode);
    }

    count = mEndSpecs.Length();
    for (uint32_t j = 0; j < count; ++j) {
      mEndSpecs[j]->ResolveReferences(aContextNode);
    }
  }

  RegisterMilestone();
}

void
nsSMILTimedElement::HandleTargetElementChange(Element* aNewTarget)
{
  AutoIntervalUpdateBatcher updateBatcher(*this);

  uint32_t count = mBeginSpecs.Length();
  for (uint32_t i = 0; i < count; ++i) {
    mBeginSpecs[i]->HandleTargetElementChange(aNewTarget);
  }

  count = mEndSpecs.Length();
  for (uint32_t j = 0; j < count; ++j) {
    mEndSpecs[j]->HandleTargetElementChange(aNewTarget);
  }
}

void
nsSMILTimedElement::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  uint32_t count = mBeginSpecs.Length();
  for (uint32_t i = 0; i < count; ++i) {
    nsSMILTimeValueSpec* beginSpec = mBeginSpecs[i];
    NS_ABORT_IF_FALSE(beginSpec,
        "null nsSMILTimeValueSpec in list of begin specs");
    beginSpec->Traverse(aCallback);
  }

  count = mEndSpecs.Length();
  for (uint32_t j = 0; j < count; ++j) {
    nsSMILTimeValueSpec* endSpec = mEndSpecs[j];
    NS_ABORT_IF_FALSE(endSpec, "null nsSMILTimeValueSpec in list of end specs");
    endSpec->Traverse(aCallback);
  }
}

void
nsSMILTimedElement::Unlink()
{
  AutoIntervalUpdateBatcher updateBatcher(*this);

  
  uint32_t count = mBeginSpecs.Length();
  for (uint32_t i = 0; i < count; ++i) {
    nsSMILTimeValueSpec* beginSpec = mBeginSpecs[i];
    NS_ABORT_IF_FALSE(beginSpec,
        "null nsSMILTimeValueSpec in list of begin specs");
    beginSpec->Unlink();
  }

  count = mEndSpecs.Length();
  for (uint32_t j = 0; j < count; ++j) {
    nsSMILTimeValueSpec* endSpec = mEndSpecs[j];
    NS_ABORT_IF_FALSE(endSpec, "null nsSMILTimeValueSpec in list of end specs");
    endSpec->Unlink();
  }

  ClearIntervals();

  
  mTimeDependents.Clear();
}




nsresult
nsSMILTimedElement::SetBeginOrEndSpec(const nsAString& aSpec,
                                      Element* aContextNode,
                                      bool aIsBegin,
                                      RemovalTestFunction aRemove)
{
  TimeValueSpecList& timeSpecsList = aIsBegin ? mBeginSpecs : mEndSpecs;
  InstanceTimeList& instances = aIsBegin ? mBeginInstances : mEndInstances;

  ClearSpecs(timeSpecsList, instances, aRemove);

  AutoIntervalUpdateBatcher updateBatcher(*this);

  nsCharSeparatedTokenizer tokenizer(aSpec, ';');
  if (!tokenizer.hasMoreTokens()) { 
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  while (tokenizer.hasMoreTokens() && NS_SUCCEEDED(rv)) {
    nsAutoPtr<nsSMILTimeValueSpec>
      spec(new nsSMILTimeValueSpec(*this, aIsBegin));
    rv = spec->SetSpec(tokenizer.nextToken(), aContextNode);
    if (NS_SUCCEEDED(rv)) {
      timeSpecsList.AppendElement(spec.forget());
    }
  }

  if (NS_FAILED(rv)) {
    ClearSpecs(timeSpecsList, instances, aRemove);
  }

  return rv;
}

namespace
{
  
  
  
  
  class MOZ_STACK_CLASS RemoveByFunction
  {
  public:
    explicit RemoveByFunction(nsSMILTimedElement::RemovalTestFunction aFunction)
      : mFunction(aFunction) { }
    bool operator()(nsSMILInstanceTime* aInstanceTime, uint32_t )
    {
      return mFunction(aInstanceTime);
    }

  private:
    nsSMILTimedElement::RemovalTestFunction mFunction;
  };
}

void
nsSMILTimedElement::ClearSpecs(TimeValueSpecList& aSpecs,
                               InstanceTimeList& aInstances,
                               RemovalTestFunction aRemove)
{
  AutoIntervalUpdateBatcher updateBatcher(*this);

  for (uint32_t i = 0; i < aSpecs.Length(); ++i) {
    aSpecs[i]->Unlink();
  }
  aSpecs.Clear();

  RemoveByFunction removeByFunction(aRemove);
  RemoveInstanceTimes(aInstances, removeByFunction);
}

void
nsSMILTimedElement::ClearIntervals()
{
  if (mElementState != STATE_STARTUP) {
    mElementState = STATE_POSTACTIVE;
  }
  mCurrentRepeatIteration = 0;
  ResetCurrentInterval();

  
  for (int32_t i = mOldIntervals.Length() - 1; i >= 0; --i) {
    mOldIntervals[i]->Unlink();
  }
  mOldIntervals.Clear();
}

bool
nsSMILTimedElement::ApplyEarlyEnd(const nsSMILTimeValue& aSampleTime)
{
  
  NS_ABORT_IF_FALSE(mElementState == STATE_ACTIVE,
      "Unexpected state to try to apply an early end");

  bool updated = false;

  
  if (mCurrentInterval->End()->Time() > aSampleTime) {
    nsSMILInstanceTime* earlyEnd = CheckForEarlyEnd(aSampleTime);
    if (earlyEnd) {
      if (earlyEnd->IsDependent()) {
        
        
        
        nsRefPtr<nsSMILInstanceTime> newEarlyEnd =
          new nsSMILInstanceTime(earlyEnd->Time());
        mCurrentInterval->SetEnd(*newEarlyEnd);
      } else {
        mCurrentInterval->SetEnd(*earlyEnd);
      }
      updated = true;
    }
  }
  return updated;
}

namespace
{
  class MOZ_STACK_CLASS RemoveReset
  {
  public:
    explicit RemoveReset(const nsSMILInstanceTime* aCurrentIntervalBegin)
      : mCurrentIntervalBegin(aCurrentIntervalBegin) { }
    bool operator()(nsSMILInstanceTime* aInstanceTime, uint32_t )
    {
      
      
      
      
      
      
      return aInstanceTime->IsDynamic() &&
             !aInstanceTime->ShouldPreserve() &&
             (!mCurrentIntervalBegin || aInstanceTime != mCurrentIntervalBegin);
    }

  private:
    const nsSMILInstanceTime* mCurrentIntervalBegin;
  };
}

void
nsSMILTimedElement::Reset()
{
  RemoveReset resetBegin(mCurrentInterval ? mCurrentInterval->Begin() : nullptr);
  RemoveInstanceTimes(mBeginInstances, resetBegin);

  RemoveReset resetEnd(nullptr);
  RemoveInstanceTimes(mEndInstances, resetEnd);
}

void
nsSMILTimedElement::ClearTimingState(RemovalTestFunction aRemove)
{
  mElementState = STATE_STARTUP;
  ClearIntervals();

  UnsetBeginSpec(aRemove);
  UnsetEndSpec(aRemove);

  if (mClient) {
    mClient->Inactivate(false);
  }
}

void
nsSMILTimedElement::RebuildTimingState(RemovalTestFunction aRemove)
{
  MOZ_ASSERT(mAnimationElement,
             "Attempting to enable a timed element not attached to an "
             "animation element");
  MOZ_ASSERT(mElementState == STATE_STARTUP,
             "Rebuilding timing state from non-startup state");

  if (mAnimationElement->HasAnimAttr(nsGkAtoms::begin)) {
    nsAutoString attValue;
    mAnimationElement->GetAnimAttr(nsGkAtoms::begin, attValue);
    SetBeginSpec(attValue, mAnimationElement, aRemove);
  }

  if (mAnimationElement->HasAnimAttr(nsGkAtoms::end)) {
    nsAutoString attValue;
    mAnimationElement->GetAnimAttr(nsGkAtoms::end, attValue);
    SetEndSpec(attValue, mAnimationElement, aRemove);
  }

  mPrevRegisteredMilestone = sMaxMilestone;
  RegisterMilestone();
}

void
nsSMILTimedElement::DoPostSeek()
{
  
  if (mSeekState == SEEK_BACKWARD_FROM_INACTIVE ||
      mSeekState == SEEK_BACKWARD_FROM_ACTIVE) {
    
    
    
    
    
    UnpreserveInstanceTimes(mBeginInstances);
    UnpreserveInstanceTimes(mEndInstances);

    
    
    
    
    
    
    Reset();
    UpdateCurrentInterval();
  }

  switch (mSeekState)
  {
  case SEEK_FORWARD_FROM_ACTIVE:
  case SEEK_BACKWARD_FROM_ACTIVE:
    if (mElementState != STATE_ACTIVE) {
      FireTimeEventAsync(NS_SMIL_END, 0);
    }
    break;

  case SEEK_FORWARD_FROM_INACTIVE:
  case SEEK_BACKWARD_FROM_INACTIVE:
    if (mElementState == STATE_ACTIVE) {
      FireTimeEventAsync(NS_SMIL_BEGIN, 0);
    }
    break;

  case SEEK_NOT_SEEKING:
    
    break;
  }

  mSeekState = SEEK_NOT_SEEKING;
}

void
nsSMILTimedElement::UnpreserveInstanceTimes(InstanceTimeList& aList)
{
  const nsSMILInterval* prevInterval = GetPreviousInterval();
  const nsSMILInstanceTime* cutoff = mCurrentInterval ?
      mCurrentInterval->Begin() :
      prevInterval ? prevInterval->Begin() : nullptr;
  uint32_t count = aList.Length();
  for (uint32_t i = 0; i < count; ++i) {
    nsSMILInstanceTime* instance = aList[i].get();
    if (!cutoff || cutoff->Time().CompareTo(instance->Time()) < 0) {
      instance->UnmarkShouldPreserve();
    }
  }
}

void
nsSMILTimedElement::FilterHistory()
{
  
  
  FilterIntervals();
  FilterInstanceTimes(mBeginInstances);
  FilterInstanceTimes(mEndInstances);
}

void
nsSMILTimedElement::FilterIntervals()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  uint32_t threshold = mOldIntervals.Length() > sMaxNumIntervals ?
                       mOldIntervals.Length() - sMaxNumIntervals :
                       0;
  IntervalList filteredList;
  for (uint32_t i = 0; i < mOldIntervals.Length(); ++i)
  {
    nsSMILInterval* interval = mOldIntervals[i].get();
    if (i != 0 && 
        i + 1 < mOldIntervals.Length() && 
        (i < threshold || !interval->IsDependencyChainLink())) {
      interval->Unlink(true );
    } else {
      filteredList.AppendElement(mOldIntervals[i].forget());
    }
  }
  mOldIntervals.Clear();
  mOldIntervals.SwapElements(filteredList);
}

namespace
{
  class MOZ_STACK_CLASS RemoveFiltered
  {
  public:
    explicit RemoveFiltered(nsSMILTimeValue aCutoff) : mCutoff(aCutoff) { }
    bool operator()(nsSMILInstanceTime* aInstanceTime, uint32_t )
    {
      
      
      
      
      
      return aInstanceTime->Time() < mCutoff &&
             aInstanceTime->IsFixedTime() &&
             !aInstanceTime->ShouldPreserve();
    }

  private:
    nsSMILTimeValue mCutoff;
  };

  class MOZ_STACK_CLASS RemoveBelowThreshold
  {
  public:
    RemoveBelowThreshold(uint32_t aThreshold,
                         nsTArray<const nsSMILInstanceTime *>& aTimesToKeep)
      : mThreshold(aThreshold),
        mTimesToKeep(aTimesToKeep) { }
    bool operator()(nsSMILInstanceTime* aInstanceTime, uint32_t aIndex)
    {
      return aIndex < mThreshold && !mTimesToKeep.Contains(aInstanceTime);
    }

  private:
    uint32_t mThreshold;
    nsTArray<const nsSMILInstanceTime *>& mTimesToKeep;
  };
}

void
nsSMILTimedElement::FilterInstanceTimes(InstanceTimeList& aList)
{
  if (GetPreviousInterval()) {
    RemoveFiltered removeFiltered(GetPreviousInterval()->End()->Time());
    RemoveInstanceTimes(aList, removeFiltered);
  }

  
  
  
  
  
  
  if (aList.Length() > sMaxNumInstanceTimes) {
    uint32_t threshold = aList.Length() - sMaxNumInstanceTimes;
    
    
    
    
    nsTArray<const nsSMILInstanceTime *> timesToKeep;
    if (mCurrentInterval) {
      timesToKeep.AppendElement(mCurrentInterval->Begin());
    }
    const nsSMILInterval* prevInterval = GetPreviousInterval();
    if (prevInterval) {
      timesToKeep.AppendElement(prevInterval->End());
    }
    if (!mOldIntervals.IsEmpty()) {
      timesToKeep.AppendElement(mOldIntervals[0]->Begin());
    }
    RemoveBelowThreshold removeBelowThreshold(threshold, timesToKeep);
    RemoveInstanceTimes(aList, removeBelowThreshold);
  }
}







bool
nsSMILTimedElement::GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    const nsSMILInterval* aReplacedInterval,
                                    const nsSMILInstanceTime* aFixedBeginTime,
                                    nsSMILInterval& aResult) const
{
  NS_ABORT_IF_FALSE(!aFixedBeginTime || aFixedBeginTime->Time().IsDefinite(),
      "Unresolved or indefinite begin time specified for interval start");
  static const nsSMILTimeValue zeroTime(0L);

  if (mRestartMode == RESTART_NEVER && aPrevInterval)
    return false;

  
  nsSMILTimeValue beginAfter;
  bool prevIntervalWasZeroDur = false;
  if (aPrevInterval) {
    beginAfter = aPrevInterval->End()->Time();
    prevIntervalWasZeroDur
      = aPrevInterval->End()->Time() == aPrevInterval->Begin()->Time();
  } else {
    beginAfter.SetMillis(INT64_MIN);
  }

  nsRefPtr<nsSMILInstanceTime> tempBegin;
  nsRefPtr<nsSMILInstanceTime> tempEnd;

  while (true) {
    
    if (aFixedBeginTime) {
      if (aFixedBeginTime->Time() < beginAfter) {
        return false;
      }
      
      tempBegin = const_cast<nsSMILInstanceTime*>(aFixedBeginTime);
    } else if ((!mAnimationElement ||
                !mAnimationElement->HasAnimAttr(nsGkAtoms::begin)) &&
               beginAfter <= zeroTime) {
      tempBegin = new nsSMILInstanceTime(nsSMILTimeValue(0));
    } else {
      int32_t beginPos = 0;
      do {
        tempBegin =
          GetNextGreaterOrEqual(mBeginInstances, beginAfter, beginPos);
        if (!tempBegin || !tempBegin->Time().IsDefinite()) {
          return false;
        }
      
      
      
      
      
      } while (aReplacedInterval &&
               tempBegin->GetBaseTime() == aReplacedInterval->Begin());
    }
    NS_ABORT_IF_FALSE(tempBegin && tempBegin->Time().IsDefinite() &&
        tempBegin->Time() >= beginAfter,
        "Got a bad begin time while fetching next interval");

    
    {
      int32_t endPos = 0;
      do {
        tempEnd =
          GetNextGreaterOrEqual(mEndInstances, tempBegin->Time(), endPos);

        
        
        
        
        if (tempEnd && prevIntervalWasZeroDur &&
            tempEnd->Time() == beginAfter) {
          tempEnd = GetNextGreater(mEndInstances, tempBegin->Time(), endPos);
        }
      
      
      
      } while (tempEnd && aReplacedInterval &&
               tempEnd->GetBaseTime() == aReplacedInterval->End());

      if (!tempEnd) {
        
        
        
        
        
        
        
        bool openEndedIntervalOk = mEndSpecs.IsEmpty() ||
                                   mEndInstances.IsEmpty() ||
                                   EndHasEventConditions();

        
        
        
        
        
        
        
        
        
        
        
        openEndedIntervalOk = openEndedIntervalOk ||
                             (aReplacedInterval &&
                              AreEndTimesDependentOn(aReplacedInterval->End()));

        if (!openEndedIntervalOk) {
          return false; 
        }
      }

      nsSMILTimeValue intervalEnd = tempEnd
                                  ? tempEnd->Time() : nsSMILTimeValue();
      nsSMILTimeValue activeEnd = CalcActiveEnd(tempBegin->Time(), intervalEnd);

      if (!tempEnd || intervalEnd != activeEnd) {
        tempEnd = new nsSMILInstanceTime(activeEnd);
      }
    }
    NS_ABORT_IF_FALSE(tempEnd, "Failed to get end point for next interval");

    
    
    
    
    
    
    
    if (prevIntervalWasZeroDur && tempEnd->Time() == beginAfter) {
      if (prevIntervalWasZeroDur) {
        beginAfter.SetMillis(tempBegin->Time().GetMillis() + 1);
        prevIntervalWasZeroDur = false;
        continue;
      }
    }
    prevIntervalWasZeroDur = tempBegin->Time() == tempEnd->Time();

    
    if (tempEnd->Time() > zeroTime ||
       (tempBegin->Time() == zeroTime && tempEnd->Time() == zeroTime)) {
      aResult.Set(*tempBegin, *tempEnd);
      return true;
    }

    if (mRestartMode == RESTART_NEVER) {
      
      return false;
    }

    beginAfter = tempEnd->Time();
  }
  NS_NOTREACHED("Hmm... we really shouldn't be here");

  return false;
}

nsSMILInstanceTime*
nsSMILTimedElement::GetNextGreater(const InstanceTimeList& aList,
                                   const nsSMILTimeValue& aBase,
                                   int32_t& aPosition) const
{
  nsSMILInstanceTime* result = nullptr;
  while ((result = GetNextGreaterOrEqual(aList, aBase, aPosition)) &&
         result->Time() == aBase) { }
  return result;
}

nsSMILInstanceTime*
nsSMILTimedElement::GetNextGreaterOrEqual(const InstanceTimeList& aList,
                                          const nsSMILTimeValue& aBase,
                                          int32_t& aPosition) const
{
  nsSMILInstanceTime* result = nullptr;
  int32_t count = aList.Length();

  for (; aPosition < count && !result; ++aPosition) {
    nsSMILInstanceTime* val = aList[aPosition].get();
    NS_ABORT_IF_FALSE(val, "NULL instance time in list");
    if (val->Time() >= aBase) {
      result = val;
    }
  }

  return result;
}




nsSMILTimeValue
nsSMILTimedElement::CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd) const
{
  nsSMILTimeValue result;

  NS_ABORT_IF_FALSE(mSimpleDur.IsResolved(),
    "Unresolved simple duration in CalcActiveEnd");
  NS_ABORT_IF_FALSE(aBegin.IsDefinite(),
    "Indefinite or unresolved begin time in CalcActiveEnd");

  result = GetRepeatDuration();

  if (aEnd.IsDefinite()) {
    nsSMILTime activeDur = aEnd.GetMillis() - aBegin.GetMillis();

    if (result.IsDefinite()) {
      result.SetMillis(std::min(result.GetMillis(), activeDur));
    } else {
      result.SetMillis(activeDur);
    }
  }

  result = ApplyMinAndMax(result);

  if (result.IsDefinite()) {
    nsSMILTime activeEnd = result.GetMillis() + aBegin.GetMillis();
    result.SetMillis(activeEnd);
  }

  return result;
}

nsSMILTimeValue
nsSMILTimedElement::GetRepeatDuration() const
{
  nsSMILTimeValue multipliedDuration;
  if (mRepeatCount.IsDefinite() && mSimpleDur.IsDefinite()) {
    multipliedDuration.SetMillis(
      nsSMILTime(mRepeatCount * double(mSimpleDur.GetMillis())));
  } else {
    multipliedDuration.SetIndefinite();
  }

  nsSMILTimeValue repeatDuration;

  if (mRepeatDur.IsResolved()) {
    repeatDuration = std::min(multipliedDuration, mRepeatDur);
  } else if (mRepeatCount.IsSet()) {
    repeatDuration = multipliedDuration;
  } else {
    repeatDuration = mSimpleDur;
  }

  return repeatDuration;
}

nsSMILTimeValue
nsSMILTimedElement::ApplyMinAndMax(const nsSMILTimeValue& aDuration) const
{
  if (!aDuration.IsResolved()) {
    return aDuration;
  }

  if (mMax < mMin) {
    return aDuration;
  }

  nsSMILTimeValue result;

  if (aDuration > mMax) {
    result = mMax;
  } else if (aDuration < mMin) {
    result = mMin;
  } else {
    result = aDuration;
  }

  return result;
}

nsSMILTime
nsSMILTimedElement::ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           uint32_t& aRepeatIteration)
{
  nsSMILTime result;

  NS_ABORT_IF_FALSE(mSimpleDur.IsResolved(),
      "Unresolved simple duration in ActiveTimeToSimpleTime");
  NS_ABORT_IF_FALSE(aActiveTime >= 0, "Expecting non-negative active time");
  
  

  if (mSimpleDur.IsIndefinite() || mSimpleDur.GetMillis() == 0L) {
    aRepeatIteration = 0;
    result = aActiveTime;
  } else {
    result = aActiveTime % mSimpleDur.GetMillis();
    aRepeatIteration = (uint32_t)(aActiveTime / mSimpleDur.GetMillis());
  }

  return result;
}













nsSMILInstanceTime*
nsSMILTimedElement::CheckForEarlyEnd(
    const nsSMILTimeValue& aContainerTime) const
{
  NS_ABORT_IF_FALSE(mCurrentInterval,
      "Checking for an early end but the current interval is not set");
  if (mRestartMode != RESTART_ALWAYS)
    return nullptr;

  int32_t position = 0;
  nsSMILInstanceTime* nextBegin =
    GetNextGreater(mBeginInstances, mCurrentInterval->Begin()->Time(),
                   position);

  if (nextBegin &&
      nextBegin->Time() > mCurrentInterval->Begin()->Time() &&
      nextBegin->Time() < mCurrentInterval->End()->Time() &&
      nextBegin->Time() <= aContainerTime) {
    return nextBegin;
  }

  return nullptr;
}

void
nsSMILTimedElement::UpdateCurrentInterval(bool aForceChangeNotice)
{
  
  if (mDeferIntervalUpdates) {
    mDoDeferredUpdate = true;
    return;
  }

  
  
  
  
  
  
  
  if (mElementState == STATE_STARTUP)
    return;

  
  
  
  
  
  
  
  if (mDeleteCount > 1) {
    
    
    
    
    NS_ABORT_IF_FALSE(mElementState == STATE_POSTACTIVE,
      "Expected to be in post-active state after performing double delete");
    return;
  }

  
  
  
  
  
  AutoRestore<uint8_t> depthRestorer(mUpdateIntervalRecursionDepth);
  if (++mUpdateIntervalRecursionDepth > sMaxUpdateIntervalRecursionDepth) {
    NS_ABORT_IF_FALSE(false,
        "Update current interval recursion depth exceeded threshold");
    return;
  }

  
  const nsSMILInstanceTime* beginTime = mElementState == STATE_ACTIVE
                                      ? mCurrentInterval->Begin()
                                      : nullptr;
  nsSMILInterval updatedInterval;
  if (GetNextInterval(GetPreviousInterval(), mCurrentInterval,
                      beginTime, updatedInterval)) {

    if (mElementState == STATE_POSTACTIVE) {

      NS_ABORT_IF_FALSE(!mCurrentInterval,
          "In postactive state but the interval has been set");
      mCurrentInterval = new nsSMILInterval(updatedInterval);
      mElementState = STATE_WAITING;
      NotifyNewInterval();

    } else {

      bool beginChanged = false;
      bool endChanged   = false;

      if (mElementState != STATE_ACTIVE &&
          !updatedInterval.Begin()->SameTimeAndBase(
            *mCurrentInterval->Begin())) {
        mCurrentInterval->SetBegin(*updatedInterval.Begin());
        beginChanged = true;
      }

      if (!updatedInterval.End()->SameTimeAndBase(*mCurrentInterval->End())) {
        mCurrentInterval->SetEnd(*updatedInterval.End());
        endChanged = true;
      }

      if (beginChanged || endChanged || aForceChangeNotice) {
        NotifyChangedInterval(mCurrentInterval, beginChanged, endChanged);
      }
    }

    
    
    RegisterMilestone();
  } else { 
    if (mElementState == STATE_ACTIVE) {
      
      
      if (!mCurrentInterval->End()->SameTimeAndBase(*mCurrentInterval->Begin()))
      {
        mCurrentInterval->SetEnd(*mCurrentInterval->Begin());
        NotifyChangedInterval(mCurrentInterval, false, true);
      }
      
      
      RegisterMilestone();
    } else if (mElementState == STATE_WAITING) {
      AutoRestore<uint8_t> deleteCountRestorer(mDeleteCount);
      ++mDeleteCount;
      mElementState = STATE_POSTACTIVE;
      ResetCurrentInterval();
    }
  }
}

void
nsSMILTimedElement::SampleSimpleTime(nsSMILTime aActiveTime)
{
  if (mClient) {
    uint32_t repeatIteration;
    nsSMILTime simpleTime =
      ActiveTimeToSimpleTime(aActiveTime, repeatIteration);
    mClient->SampleAt(simpleTime, mSimpleDur, repeatIteration);
  }
}

void
nsSMILTimedElement::SampleFillValue()
{
  if (mFillMode != FILL_FREEZE || !mClient)
    return;

  nsSMILTime activeTime;

  if (mElementState == STATE_WAITING || mElementState == STATE_POSTACTIVE) {
    const nsSMILInterval* prevInterval = GetPreviousInterval();
    NS_ABORT_IF_FALSE(prevInterval,
        "Attempting to sample fill value but there is no previous interval");
    NS_ABORT_IF_FALSE(prevInterval->End()->Time().IsDefinite() &&
        prevInterval->End()->IsFixedTime(),
        "Attempting to sample fill value but the endpoint of the previous "
        "interval is not resolved and fixed");

    activeTime = prevInterval->End()->Time().GetMillis() -
                 prevInterval->Begin()->Time().GetMillis();

    
    
    
    nsSMILTimeValue repeatDuration = GetRepeatDuration();
    if (repeatDuration.IsDefinite()) {
      activeTime = std::min(repeatDuration.GetMillis(), activeTime);
    }
  } else {
    MOZ_ASSERT(mElementState == STATE_ACTIVE,
        "Attempting to sample fill value when we're in an unexpected state "
        "(probably STATE_STARTUP)");

    
    
    MOZ_ASSERT(GetRepeatDuration().IsDefinite(),
        "Attempting to sample fill value of an active animation with "
        "an indefinite repeat duration");
    activeTime = GetRepeatDuration().GetMillis();
  }

  uint32_t repeatIteration;
  nsSMILTime simpleTime =
    ActiveTimeToSimpleTime(activeTime, repeatIteration);

  if (simpleTime == 0L && repeatIteration) {
    mClient->SampleLastValue(--repeatIteration);
  } else {
    mClient->SampleAt(simpleTime, mSimpleDur, repeatIteration);
  }
}

nsresult
nsSMILTimedElement::AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
    double aOffsetSeconds, bool aIsBegin)
{
  double offset = aOffsetSeconds * PR_MSEC_PER_SEC;

  
  if (aCurrentTime + NS_round(offset) > INT64_MAX)
    return NS_ERROR_ILLEGAL_VALUE;

  nsSMILTimeValue timeVal(aCurrentTime + int64_t(NS_round(offset)));

  nsRefPtr<nsSMILInstanceTime> instanceTime =
    new nsSMILInstanceTime(timeVal, nsSMILInstanceTime::SOURCE_DOM);

  AddInstanceTime(instanceTime, aIsBegin);

  return NS_OK;
}

void
nsSMILTimedElement::RegisterMilestone()
{
  nsSMILTimeContainer* container = GetTimeContainer();
  if (!container)
    return;
  NS_ABORT_IF_FALSE(mAnimationElement,
      "Got a time container without an owning animation element");

  nsSMILMilestone nextMilestone;
  if (!GetNextMilestone(nextMilestone))
    return;

  
  
  
  
  if (nextMilestone >= mPrevRegisteredMilestone)
    return;

  container->AddMilestone(nextMilestone, *mAnimationElement);
  mPrevRegisteredMilestone = nextMilestone;
}

bool
nsSMILTimedElement::GetNextMilestone(nsSMILMilestone& aNextMilestone) const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  switch (mElementState)
  {
  case STATE_STARTUP:
    
    
    aNextMilestone.mIsEnd = true; 
    aNextMilestone.mTime = 0;
    return true;

  case STATE_WAITING:
    NS_ABORT_IF_FALSE(mCurrentInterval,
        "In waiting state but the current interval has not been set");
    aNextMilestone.mIsEnd = false;
    aNextMilestone.mTime = mCurrentInterval->Begin()->Time().GetMillis();
    return true;

  case STATE_ACTIVE:
    {
      
      nsSMILTimeValue nextRepeat;
      if (mSeekState == SEEK_NOT_SEEKING && mSimpleDur.IsDefinite()) {
        nsSMILTime nextRepeatActiveTime =
          (mCurrentRepeatIteration + 1) * mSimpleDur.GetMillis();
        
        if (nsSMILTimeValue(nextRepeatActiveTime) < GetRepeatDuration()) {
          nextRepeat.SetMillis(mCurrentInterval->Begin()->Time().GetMillis() +
                               nextRepeatActiveTime);
        }
      }
      nsSMILTimeValue nextMilestone =
        std::min(mCurrentInterval->End()->Time(), nextRepeat);

      
      nsSMILInstanceTime* earlyEnd = CheckForEarlyEnd(nextMilestone);
      if (earlyEnd) {
        aNextMilestone.mIsEnd = true;
        aNextMilestone.mTime = earlyEnd->Time().GetMillis();
        return true;
      }

      
      if (nextMilestone.IsDefinite()) {
        aNextMilestone.mIsEnd = nextMilestone != nextRepeat;
        aNextMilestone.mTime = nextMilestone.GetMillis();
        return true;
      }

      return false;
    }

  case STATE_POSTACTIVE:
    return false;
  }
  MOZ_CRASH("Invalid element state");
}

void
nsSMILTimedElement::NotifyNewInterval()
{
  NS_ABORT_IF_FALSE(mCurrentInterval,
      "Attempting to notify dependents of a new interval but the interval "
      "is not set");

  nsSMILTimeContainer* container = GetTimeContainer();
  if (container) {
    container->SyncPauseTime();
  }

  NotifyTimeDependentsParams params = { this, container };
  mTimeDependents.EnumerateEntries(NotifyNewIntervalCallback, &params);
}

void
nsSMILTimedElement::NotifyChangedInterval(nsSMILInterval* aInterval,
                                          bool aBeginObjectChanged,
                                          bool aEndObjectChanged)
{
  NS_ABORT_IF_FALSE(aInterval, "Null interval for change notification");

  nsSMILTimeContainer* container = GetTimeContainer();
  if (container) {
    container->SyncPauseTime();
  }

  
  
  
  InstanceTimeList times;
  aInterval->GetDependentTimes(times);

  for (uint32_t i = 0; i < times.Length(); ++i) {
    times[i]->HandleChangedInterval(container, aBeginObjectChanged,
                                    aEndObjectChanged);
  }
}

void
nsSMILTimedElement::FireTimeEventAsync(uint32_t aMsg, int32_t aDetail)
{
  if (!mAnimationElement)
    return;

  nsCOMPtr<nsIRunnable> event =
    new AsyncTimeEventRunner(mAnimationElement, aMsg, aDetail);
  NS_DispatchToMainThread(event);
}

const nsSMILInstanceTime*
nsSMILTimedElement::GetEffectiveBeginInstance() const
{
  switch (mElementState)
  {
  case STATE_STARTUP:
    return nullptr;

  case STATE_ACTIVE:
    return mCurrentInterval->Begin();

  case STATE_WAITING:
  case STATE_POSTACTIVE:
    {
      const nsSMILInterval* prevInterval = GetPreviousInterval();
      return prevInterval ? prevInterval->Begin() : nullptr;
    }
  }
  MOZ_CRASH("Invalid element state");
}

const nsSMILInterval*
nsSMILTimedElement::GetPreviousInterval() const
{
  return mOldIntervals.IsEmpty()
    ? nullptr
    : mOldIntervals[mOldIntervals.Length()-1].get();
}

bool
nsSMILTimedElement::HasClientInFillRange() const
{
  
  return mClient &&
         ((mElementState != STATE_ACTIVE && HasPlayed()) ||
          (mElementState == STATE_ACTIVE && !mClient->IsActive()));
}

bool
nsSMILTimedElement::EndHasEventConditions() const
{
  for (uint32_t i = 0; i < mEndSpecs.Length(); ++i) {
    if (mEndSpecs[i]->IsEventBased())
      return true;
  }
  return false;
}

bool
nsSMILTimedElement::AreEndTimesDependentOn(
  const nsSMILInstanceTime* aBase) const
{
  if (mEndInstances.IsEmpty())
    return false;

  for (uint32_t i = 0; i < mEndInstances.Length(); ++i) {
    if (mEndInstances[i]->GetBaseTime() != aBase) {
      return false;
    }
  }
  return true;
}




 PLDHashOperator
nsSMILTimedElement::NotifyNewIntervalCallback(TimeValueSpecPtrKey* aKey,
                                              void* aData)
{
  NS_ABORT_IF_FALSE(aKey, "Null hash key for time container hash table");
  NS_ABORT_IF_FALSE(aKey->GetKey(),
                    "null nsSMILTimeValueSpec in set of time dependents");

  NotifyTimeDependentsParams* params =
    static_cast<NotifyTimeDependentsParams*>(aData);
  NS_ABORT_IF_FALSE(params, "null data ptr while enumerating hashtable");
  nsSMILInterval* interval = params->mTimedElement->mCurrentInterval;
  
  
  
  if (!interval)
    return PL_DHASH_STOP;

  nsSMILTimeValueSpec* spec = aKey->GetKey();
  spec->HandleNewInterval(*interval, params->mTimeContainer);
  return PL_DHASH_NEXT;
}
