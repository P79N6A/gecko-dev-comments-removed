




































#include "nsSMILTimedElement.h"
#include "nsSMILAnimationFunction.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsSMILTimeContainer.h"
#include "nsGkAtoms.h"
#include "nsReadableUtils.h"
#include "nsMathUtils.h"
#include "prdtoa.h"
#include "plstr.h"
#include "prtime.h"
#include "nsString.h"












PRBool
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

PRBool
nsSMILTimedElement::InstanceTimeComparator::LessThan(
    const nsSMILInstanceTime* aElem1,
    const nsSMILInstanceTime* aElem2) const
{
  NS_ABORT_IF_FALSE(aElem1 && aElem2,
      "Trying to compare null instance time pointers");
  NS_ABORT_IF_FALSE(aElem1->Serial() && aElem2->Serial(),
      "Instance times have not been assigned serial numbers");

  PRInt8 cmp = aElem1->Time().CompareTo(aElem2->Time());
  return cmp == 0 ? aElem1->Serial() < aElem2->Serial() : cmp < 0;
}






template <class TestFunctor>
void
nsSMILTimedElement::RemoveInstanceTimes(InstanceTimeList& aArray,
                                        TestFunctor& aTest)
{
  InstanceTimeList newArray;
  for (PRUint32 i = 0; i < aArray.Length(); ++i) {
    nsSMILInstanceTime* item = aArray[i].get();
    if (aTest(item, i)) {
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
      {nsnull, 0}
};

nsAttrValue::EnumTable nsSMILTimedElement::sRestartModeTable[] = {
      {"always", RESTART_ALWAYS},
      {"whenNotActive", RESTART_WHENNOTACTIVE},
      {"never", RESTART_NEVER},
      {nsnull, 0}
};

const nsSMILMilestone nsSMILTimedElement::sMaxMilestone(LL_MAXINT, PR_FALSE);




const PRUint8 nsSMILTimedElement::sMaxNumIntervals = 20;
const PRUint8 nsSMILTimedElement::sMaxNumInstanceTimes = 100;




nsSMILTimedElement::nsSMILTimedElement()
:
  mAnimationElement(nsnull),
  mFillMode(FILL_REMOVE),
  mRestartMode(RESTART_ALWAYS),
  mBeginSpecSet(PR_FALSE),
  mEndHasEventConditions(PR_FALSE),
  mInstanceSerialIndex(0),
  mClient(nsnull),
  mCurrentInterval(nsnull),
  mPrevRegisteredMilestone(sMaxMilestone),
  mElementState(STATE_STARTUP)
{
  mSimpleDur.SetIndefinite();
  mMin.SetMillis(0L);
  mMax.SetIndefinite();
  mTimeDependents.Init();
}

nsSMILTimedElement::~nsSMILTimedElement()
{
  
  mElementState = STATE_POSTACTIVE;

  
  for (PRUint32 i = 0; i < mBeginInstances.Length(); ++i) {
    mBeginInstances[i]->Unlink();
  }
  mBeginInstances.Clear();
  for (PRUint32 i = 0; i < mEndInstances.Length(); ++i) {
    mEndInstances[i]->Unlink();
  }
  mEndInstances.Clear();

  
  
  
  if (mCurrentInterval) {
    mCurrentInterval->Unlink();
    mCurrentInterval = nsnull;
  }

  for (PRInt32 i = mOldIntervals.Length() - 1; i >= 0; --i) {
    mOldIntervals[i]->Unlink();
  }
  mOldIntervals.Clear();
}

void
nsSMILTimedElement::SetAnimationElement(nsISMILAnimationElement* aElement)
{
  NS_ABORT_IF_FALSE(aElement, "NULL owner element");
  NS_ABORT_IF_FALSE(!mAnimationElement, "Re-setting owner");
  mAnimationElement = aElement;
}

nsSMILTimeContainer*
nsSMILTimedElement::GetTimeContainer()
{
  return mAnimationElement ? mAnimationElement->GetTimeContainer() : nsnull;
}

















nsresult
nsSMILTimedElement::BeginElementAt(double aOffsetSeconds)
{
  nsSMILTimeContainer* container = GetTimeContainer();
  if (!container)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = container->GetCurrentTime();
  AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, PR_TRUE);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  if (mElementState != STATE_STARTUP) {
    DoSampleAt(currentTime, PR_FALSE); 
  }

  return NS_OK;
}

nsresult
nsSMILTimedElement::EndElementAt(double aOffsetSeconds)
{
  nsSMILTimeContainer* container = GetTimeContainer();
  if (!container)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = container->GetCurrentTime();
  AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, PR_FALSE);
  if (mElementState != STATE_STARTUP) {
    DoSampleAt(currentTime, PR_FALSE); 
  }

  return NS_OK;
}




nsSMILTimeValue
nsSMILTimedElement::GetStartTime() const
{
  return mElementState == STATE_WAITING || mElementState == STATE_ACTIVE
         ? mCurrentInterval->Begin()->Time()
         : nsSMILTimeValue();
}




void
nsSMILTimedElement::AddInstanceTime(nsSMILInstanceTime* aInstanceTime,
                                    PRBool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to add null instance time");

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
                                       PRBool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to update null instance time");

  
  
  
  
  aInstanceTime->DependentUpdate(aUpdatedTime);
  InstanceTimeList& instanceList = aIsBegin ? mBeginInstances : mEndInstances;
  instanceList.Sort(InstanceTimeComparator());

  
  
  
  
  
  
  
  
  
  
  
  PRBool changedCurrentInterval = mCurrentInterval &&
    (mCurrentInterval->Begin() == aInstanceTime ||
     mCurrentInterval->End() == aInstanceTime);

  UpdateCurrentInterval(changedCurrentInterval);
}

void
nsSMILTimedElement::RemoveInstanceTime(nsSMILInstanceTime* aInstanceTime,
                                       PRBool aIsBegin)
{
  NS_ABORT_IF_FALSE(aInstanceTime, "Attempting to remove null instance time");

  InstanceTimeList& instanceList = aIsBegin ? mBeginInstances : mEndInstances;
#ifdef DEBUG
  PRBool found =
#endif
    instanceList.RemoveElementSorted(aInstanceTime, InstanceTimeComparator());
  NS_ABORT_IF_FALSE(found, "Couldn't find instance time to delete");

  UpdateCurrentInterval();
}

namespace
{
  class RemoveByCreator
  {
  public:
    RemoveByCreator(const nsSMILTimeValueSpec* aCreator) : mCreator(aCreator)
    { }

    PRBool operator()(nsSMILInstanceTime* aInstanceTime, PRUint32 )
    {
      return aInstanceTime->GetCreator() == mCreator;
    }

  private:
    const nsSMILTimeValueSpec* mCreator;
  };
}

void
nsSMILTimedElement::RemoveInstanceTimesForCreator(
    const nsSMILTimeValueSpec* aCreator, PRBool aIsBegin)
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
  
  mPrevRegisteredMilestone = sMaxMilestone;

  DoSampleAt(aContainerTime, PR_FALSE);
}

void
nsSMILTimedElement::SampleEndAt(nsSMILTime aContainerTime)
{
  
  mPrevRegisteredMilestone = sMaxMilestone;

  
  
  
  
  
  
  
  
  
  if (mElementState == STATE_ACTIVE || mElementState == STATE_STARTUP) {
    DoSampleAt(aContainerTime, PR_TRUE); 
  } else {
    
    
    RegisterMilestone();
  }
}

void
nsSMILTimedElement::DoSampleAt(nsSMILTime aContainerTime, PRBool aEndOnly)
{
  NS_ABORT_IF_FALSE(mAnimationElement,
      "Got sample before being registered with an animation element");
  NS_ABORT_IF_FALSE(GetTimeContainer(),
      "Got sample without being registered with a time container");

  
  
  
  
  
  if (GetTimeContainer()->IsPausedByType(nsSMILTimeContainer::PAUSE_BEGIN))
    return;

  NS_ABORT_IF_FALSE(mElementState != STATE_STARTUP || aEndOnly,
      "Got a regular sample during startup state, expected an end sample"
      " instead");

  PRBool          stateChanged;
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

    stateChanged = PR_FALSE;

    switch (mElementState)
    {
    case STATE_STARTUP:
      {
        nsSMILInterval firstInterval;
        mElementState =
         NS_SUCCEEDED(GetNextInterval(nsnull, nsnull, firstInterval))
         ? STATE_WAITING
         : STATE_POSTACTIVE;
        stateChanged = PR_TRUE;
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
          if (HasPlayed()) {
            Reset(); 
          }
          if (mClient) {
            mClient->Activate(mCurrentInterval->Begin()->Time().GetMillis());
          }
          if (HasPlayed()) {
            
            
            
            
            
            
            
            UpdateCurrentInterval();
          }
          stateChanged = PR_TRUE;
        }
      }
      break;

    case STATE_ACTIVE:
      {
        ApplyEarlyEnd(sampleTime);

        if (mCurrentInterval->End()->Time() <= sampleTime) {
          nsSMILInterval newInterval;
          mElementState =
            NS_SUCCEEDED(GetNextInterval(mCurrentInterval, nsnull, newInterval))
            ? STATE_WAITING
            : STATE_POSTACTIVE;
          if (mClient) {
            mClient->Inactivate(mFillMode == FILL_FREEZE);
          }
          mCurrentInterval->FixEnd();
          mOldIntervals.AppendElement(mCurrentInterval.forget());
          
          SampleFillValue();
          if (mElementState == STATE_WAITING) {
            mCurrentInterval = new nsSMILInterval(newInterval);
            NotifyNewInterval();
          }
          FilterHistory();
          stateChanged = PR_TRUE;
        } else {
          nsSMILTime beginTime = mCurrentInterval->Begin()->Time().GetMillis();
          nsSMILTime activeTime = aContainerTime - beginTime;
          SampleSimpleTime(activeTime);
        }
      }
      break;

    case STATE_POSTACTIVE:
      break;
    }

  
  
  
  
  
  
  } while (stateChanged && (!aEndOnly || (mElementState != STATE_WAITING &&
                                          mElementState != STATE_POSTACTIVE)));

  RegisterMilestone();
}

void
nsSMILTimedElement::HandleContainerTimeChange()
{
  
  
  
  
  
  if (mElementState == STATE_WAITING || mElementState == STATE_ACTIVE) {
    NotifyChangedInterval();
  }
}

PRBool
nsSMILTimedElement::SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                            nsAttrValue& aResult, nsIContent* aContextNode,
                            nsresult* aParseResult)
{
  PRBool foundMatch = PR_TRUE;
  nsresult parseResult = NS_OK;

  if (aAttribute == nsGkAtoms::begin) {
    parseResult = SetBeginSpec(aValue, aContextNode);
  } else if (aAttribute == nsGkAtoms::dur) {
    parseResult = SetSimpleDuration(aValue);
  } else if (aAttribute == nsGkAtoms::end) {
    parseResult = SetEndSpec(aValue, aContextNode);
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
    foundMatch = PR_FALSE;
  }

  if (foundMatch) {
    aResult.SetTo(aValue);
    if (aParseResult) {
      *aParseResult = parseResult;
    }
  }

  return foundMatch;
}

PRBool
nsSMILTimedElement::UnsetAttr(nsIAtom* aAttribute)
{
  PRBool foundMatch = PR_TRUE;

  if (aAttribute == nsGkAtoms::begin) {
    UnsetBeginSpec();
  } else if (aAttribute == nsGkAtoms::dur) {
    UnsetSimpleDuration();
  } else if (aAttribute == nsGkAtoms::end) {
    UnsetEndSpec();
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
    foundMatch = PR_FALSE;
  }

  return foundMatch;
}




nsresult
nsSMILTimedElement::SetBeginSpec(const nsAString& aBeginSpec,
                                 nsIContent* aContextNode)
{
  mBeginSpecSet = PR_TRUE;
  return SetBeginOrEndSpec(aBeginSpec, aContextNode, PR_TRUE);
}

void
nsSMILTimedElement::UnsetBeginSpec()
{
  ClearBeginOrEndSpecs(PR_TRUE);
  mBeginSpecSet = PR_FALSE;
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetEndSpec(const nsAString& aEndSpec,
                               nsIContent* aContextNode)
{
  
  
  
  return SetBeginOrEndSpec(aEndSpec, aContextNode, PR_FALSE);
}

void
nsSMILTimedElement::UnsetEndSpec()
{
  ClearBeginOrEndSpecs(PR_FALSE);
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetSimpleDuration(const nsAString& aDurSpec)
{
  nsSMILTimeValue duration;
  PRBool isMedia;
  nsresult rv;

  rv = nsSMILParserUtils::ParseClockValue(aDurSpec, &duration,
          nsSMILParserUtils::kClockValueAllowIndefinite, &isMedia);

  if (NS_FAILED(rv)) {
    mSimpleDur.SetIndefinite();
    return NS_ERROR_FAILURE;
  }

  if (duration.IsResolved() && duration.GetMillis() == 0L) {
    mSimpleDur.SetIndefinite();
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  if (isMedia)
    duration.SetIndefinite();

  
  
  NS_ASSERTION(duration.IsResolved() || duration.IsIndefinite(),
    "Setting unresolved simple duration");

  mSimpleDur = duration;
  UpdateCurrentInterval();

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
  nsSMILTimeValue duration;
  PRBool isMedia;
  nsresult rv;

  rv = nsSMILParserUtils::ParseClockValue(aMinSpec, &duration, 0, &isMedia);

  if (isMedia) {
    duration.SetMillis(0L);
  }

  if (NS_FAILED(rv) || !duration.IsResolved()) {
    mMin.SetMillis(0L);
    return NS_ERROR_FAILURE;
  }

  if (duration.GetMillis() < 0L) {
    mMin.SetMillis(0L);
    return NS_ERROR_FAILURE;
  }

  mMin = duration;
  UpdateCurrentInterval();

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
  nsSMILTimeValue duration;
  PRBool isMedia;
  nsresult rv;

  rv = nsSMILParserUtils::ParseClockValue(aMaxSpec, &duration,
          nsSMILParserUtils::kClockValueAllowIndefinite, &isMedia);

  if (isMedia)
    duration.SetIndefinite();

  if (NS_FAILED(rv) || (!duration.IsResolved() && !duration.IsIndefinite())) {
    mMax.SetIndefinite();
    return NS_ERROR_FAILURE;
  }

  if (duration.IsResolved() && duration.GetMillis() <= 0L) {
    mMax.SetIndefinite();
    return NS_ERROR_FAILURE;
  }

  mMax = duration;
  UpdateCurrentInterval();

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
  PRBool parseResult
    = temp.ParseEnumValue(aRestartSpec, sRestartModeTable, PR_TRUE);
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
  nsSMILRepeatCount newRepeatCount;
  nsresult rv =
    nsSMILParserUtils::ParseRepeatCount(aRepeatCountSpec, newRepeatCount);

  if (NS_SUCCEEDED(rv)) {
    mRepeatCount = newRepeatCount;
  } else {
    mRepeatCount.Unset();
  }

  UpdateCurrentInterval();

  return rv;
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
  nsresult rv;
  nsSMILTimeValue duration;

  rv = nsSMILParserUtils::ParseClockValue(aRepeatDurSpec, &duration,
          nsSMILParserUtils::kClockValueAllowIndefinite);

  if (NS_FAILED(rv) || (!duration.IsResolved() && !duration.IsIndefinite())) {
    mRepeatDur.SetUnresolved();
    return NS_ERROR_FAILURE;
  }

  mRepeatDur = duration;
  UpdateCurrentInterval();

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
  PRUint16 previousFillMode = mFillMode;

  nsAttrValue temp;
  PRBool parseResult =
    temp.ParseEnumValue(aFillModeSpec, sFillModeTable, PR_TRUE);
  mFillMode = parseResult
            ? nsSMILFillMode(temp.GetEnumValue())
            : FILL_REMOVE;

  
  
  PRBool isFillable = HasPlayed() &&
    (mElementState == STATE_WAITING || mElementState == STATE_POSTACTIVE);

  if (mClient && mFillMode != previousFillMode && isFillable) {
    mClient->Inactivate(mFillMode == FILL_FREEZE);
    SampleFillValue();
  }

  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILTimedElement::UnsetFillMode()
{
  PRUint16 previousFillMode = mFillMode;
  mFillMode = FILL_REMOVE;
  if ((mElementState == STATE_WAITING || mElementState == STATE_POSTACTIVE) &&
      previousFillMode == FILL_FREEZE && mClient && HasPlayed())
    mClient->Inactivate(PR_FALSE);
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

PRBool
nsSMILTimedElement::IsTimeDependent(const nsSMILTimedElement& aOther) const
{
  const nsSMILInstanceTime* thisBegin = GetEffectiveBeginInstance();
  const nsSMILInstanceTime* otherBegin = aOther.GetEffectiveBeginInstance();

  if (!thisBegin || !otherBegin)
    return PR_FALSE;

  return thisBegin->IsDependentOn(*otherBegin);
}

void
nsSMILTimedElement::BindToTree(nsIContent* aContextNode)
{
  
  PRUint32 count = mBeginSpecs.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsSMILTimeValueSpec* beginSpec = mBeginSpecs[i];
    NS_ABORT_IF_FALSE(beginSpec,
        "null nsSMILTimeValueSpec in list of begin specs");
    beginSpec->ResolveReferences(aContextNode);
  }

  count = mEndSpecs.Length();
  for (PRUint32 j = 0; j < count; ++j) {
    nsSMILTimeValueSpec* endSpec = mEndSpecs[j];
    NS_ABORT_IF_FALSE(endSpec, "null nsSMILTimeValueSpec in list of end specs");
    endSpec->ResolveReferences(aContextNode);
  }

  
  
  mPrevRegisteredMilestone = sMaxMilestone;

  RegisterMilestone();
}

void
nsSMILTimedElement::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  PRUint32 count = mBeginSpecs.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsSMILTimeValueSpec* beginSpec = mBeginSpecs[i];
    NS_ABORT_IF_FALSE(beginSpec,
        "null nsSMILTimeValueSpec in list of begin specs");
    beginSpec->Traverse(aCallback);
  }

  count = mEndSpecs.Length();
  for (PRUint32 j = 0; j < count; ++j) {
    nsSMILTimeValueSpec* endSpec = mEndSpecs[j];
    NS_ABORT_IF_FALSE(endSpec, "null nsSMILTimeValueSpec in list of end specs");
    endSpec->Traverse(aCallback);
  }
}

void
nsSMILTimedElement::Unlink()
{
  PRUint32 count = mBeginSpecs.Length();
  for (PRUint32 i = 0; i < count; ++i) {
    nsSMILTimeValueSpec* beginSpec = mBeginSpecs[i];
    NS_ABORT_IF_FALSE(beginSpec,
        "null nsSMILTimeValueSpec in list of begin specs");
    beginSpec->Unlink();
  }

  count = mEndSpecs.Length();
  for (PRUint32 j = 0; j < count; ++j) {
    nsSMILTimeValueSpec* endSpec = mEndSpecs[j];
    NS_ABORT_IF_FALSE(endSpec, "null nsSMILTimeValueSpec in list of end specs");
    endSpec->Unlink();
  }
}




nsresult
nsSMILTimedElement::SetBeginOrEndSpec(const nsAString& aSpec,
                                      nsIContent* aContextNode,
                                      PRBool aIsBegin)
{
  ClearBeginOrEndSpecs(aIsBegin);

  PRInt32 start;
  PRInt32 end = -1;
  PRInt32 length;
  nsresult rv = NS_OK;
  TimeValueSpecList& timeSpecsList = aIsBegin ? mBeginSpecs : mEndSpecs;

  do {
    start = end + 1;
    end = aSpec.FindChar(';', start);
    length = (end == -1) ? -1 : end - start;
    nsAutoPtr<nsSMILTimeValueSpec>
      spec(new nsSMILTimeValueSpec(*this, aIsBegin));
    rv = spec->SetSpec(Substring(aSpec, start, length), aContextNode);
    if (NS_SUCCEEDED(rv)) {
      timeSpecsList.AppendElement(spec.forget());
    }
  } while (end != -1 && NS_SUCCEEDED(rv));

  if (NS_FAILED(rv)) {
    ClearBeginOrEndSpecs(aIsBegin);
  }

  UpdateCurrentInterval();

  return rv;
}

namespace
{
  class RemoveNonDOM
  {
  public:
    PRBool operator()(nsSMILInstanceTime* aInstanceTime, PRUint32 )
    {
      return !aInstanceTime->FromDOM();
    }
  };
}

void
nsSMILTimedElement::ClearBeginOrEndSpecs(PRBool aIsBegin)
{
  TimeValueSpecList& specs = aIsBegin ? mBeginSpecs : mEndSpecs;
  specs.Clear();

  
  
  InstanceTimeList& instances = aIsBegin ? mBeginInstances : mEndInstances;
  RemoveNonDOM removeNonDOM;
  RemoveInstanceTimes(instances, removeNonDOM);
}

void
nsSMILTimedElement::ApplyEarlyEnd(const nsSMILTimeValue& aSampleTime)
{
  
  NS_ABORT_IF_FALSE(mElementState == STATE_ACTIVE,
      "Unexpected state to try to apply an early end");

  
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
      NotifyChangedInterval();
    }
  }
}

namespace
{
  class RemoveReset
  {
  public:
    RemoveReset(const nsSMILInstanceTime* aCurrentIntervalBegin)
      : mCurrentIntervalBegin(aCurrentIntervalBegin) { }
    PRBool operator()(nsSMILInstanceTime* aInstanceTime, PRUint32 )
    {
      
      
      
      
      
      
      return aInstanceTime->IsDynamic() &&
             !aInstanceTime->IsUsedAsFixedEndpoint() &&
             (!mCurrentIntervalBegin || aInstanceTime != mCurrentIntervalBegin);
    }

  private:
    const nsSMILInstanceTime* mCurrentIntervalBegin;
  };
}

void
nsSMILTimedElement::Reset()
{
  RemoveReset resetBegin(mCurrentInterval ? mCurrentInterval->Begin() : nsnull);
  RemoveInstanceTimes(mBeginInstances, resetBegin);

  RemoveReset resetEnd(nsnull);
  RemoveInstanceTimes(mEndInstances, resetEnd);
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
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PRUint32 threshold = mOldIntervals.Length() > sMaxNumIntervals ?
                       mOldIntervals.Length() - sMaxNumIntervals :
                       0;
  IntervalList filteredList;
  for (PRUint32 i = 0; i < mOldIntervals.Length(); ++i)
  {
    nsSMILInterval* interval = mOldIntervals[i].get();
    if (i + 1 < mOldIntervals.Length()  &&
        (i < threshold || !interval->IsDependencyChainLink())) {
      interval->Unlink(PR_TRUE );
    } else {
      filteredList.AppendElement(mOldIntervals[i].forget());
    }
  }
  mOldIntervals.Clear();
  mOldIntervals.SwapElements(filteredList);
}

namespace
{
  class RemoveFiltered
  {
  public:
    RemoveFiltered(nsSMILTimeValue aCutoff) : mCutoff(aCutoff) { }
    PRBool operator()(nsSMILInstanceTime* aInstanceTime, PRUint32 )
    {
      
      
      
      
      
      return aInstanceTime->Time() < mCutoff &&
             aInstanceTime->IsFixedTime() &&
             !aInstanceTime->IsUsedAsFixedEndpoint();
    }

  private:
    nsSMILTimeValue mCutoff;
  };

  class RemoveBelowThreshold
  {
  public:
    RemoveBelowThreshold(PRUint32 aThreshold,
                         const nsSMILInstanceTime* aCurrentIntervalBegin)
      : mThreshold(aThreshold),
        mCurrentIntervalBegin(aCurrentIntervalBegin) { }
    PRBool operator()(nsSMILInstanceTime* aInstanceTime, PRUint32 aIndex)
    {
      return aInstanceTime != mCurrentIntervalBegin && aIndex < mThreshold;
    }

  private:
    PRUint32 mThreshold;
    const nsSMILInstanceTime* mCurrentIntervalBegin;
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
    PRUint32 threshold = aList.Length() - sMaxNumInstanceTimes;
    
    const nsSMILInstanceTime* currentIntervalBegin = mCurrentInterval ?
      mCurrentInterval->Begin() : nsnull;
    RemoveBelowThreshold removeBelowThreshold(threshold, currentIntervalBegin);
    RemoveInstanceTimes(aList, removeBelowThreshold);
  }
}







nsresult
nsSMILTimedElement::GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    const nsSMILInstanceTime* aFixedBeginTime,
                                    nsSMILInterval& aResult) const
{
  NS_ABORT_IF_FALSE(!aFixedBeginTime || aFixedBeginTime->Time().IsResolved(),
      "Unresolved begin time specified for interval start");
  static nsSMILTimeValue zeroTime(0L);

  if (mRestartMode == RESTART_NEVER && aPrevInterval)
    return NS_ERROR_FAILURE;

  
  nsSMILTimeValue beginAfter;
  PRBool prevIntervalWasZeroDur = PR_FALSE;
  if (aPrevInterval) {
    beginAfter = aPrevInterval->End()->Time();
    prevIntervalWasZeroDur
      = aPrevInterval->End()->Time() == aPrevInterval->Begin()->Time();
    if (aFixedBeginTime) {
      prevIntervalWasZeroDur &=
        aPrevInterval->Begin()->Time() == aFixedBeginTime->Time();
    }
  } else {
    beginAfter.SetMillis(LL_MININT);
  }

  nsRefPtr<nsSMILInstanceTime> tempBegin;
  nsRefPtr<nsSMILInstanceTime> tempEnd;

  while (PR_TRUE) {
    
    if (aFixedBeginTime) {
      if (aFixedBeginTime->Time() < beginAfter)
        return NS_ERROR_FAILURE;
      
      tempBegin = const_cast<nsSMILInstanceTime*>(aFixedBeginTime);
    } else if (!mBeginSpecSet && beginAfter <= zeroTime) {
      tempBegin = new nsSMILInstanceTime(nsSMILTimeValue(0));
    } else {
      PRInt32 beginPos = 0;
      tempBegin = GetNextGreaterOrEqual(mBeginInstances, beginAfter, beginPos);
      if (!tempBegin || !tempBegin->Time().IsResolved())
        return NS_ERROR_FAILURE;
    }
    NS_ABORT_IF_FALSE(tempBegin && tempBegin->Time().IsResolved() && 
        tempBegin->Time() >= beginAfter,
        "Got a bad begin time while fetching next interval");

    
    {
      PRInt32 endPos = 0;
      tempEnd = GetNextGreaterOrEqual(mEndInstances, tempBegin->Time(), endPos);

      
      
      if (tempEnd && tempEnd->Time() == tempBegin->Time() &&
          prevIntervalWasZeroDur) {
        tempEnd = GetNextGreater(mEndInstances, tempBegin->Time(), endPos);
      }

      
      
      
      
      
      
      
      
      
      
      PRBool openEndedIntervalOk = mEndHasEventConditions ||
          mEndSpecs.IsEmpty() ||
          mEndInstances.IsEmpty();
      if (!tempEnd && !openEndedIntervalOk)
        return NS_ERROR_FAILURE; 

      nsSMILTimeValue intervalEnd = tempEnd
                                  ? tempEnd->Time() : nsSMILTimeValue();
      nsSMILTimeValue activeEnd = CalcActiveEnd(tempBegin->Time(), intervalEnd);

      if (!tempEnd || intervalEnd != activeEnd) {
        tempEnd = new nsSMILInstanceTime(activeEnd);
      }
    }
    NS_ABORT_IF_FALSE(tempEnd, "Failed to get end point for next interval");

    
    
    
    if (tempEnd->Time().IsResolved() && tempBegin->Time() == tempEnd->Time()) {
      if (prevIntervalWasZeroDur) {
        beginAfter.SetMillis(tempEnd->Time().GetMillis() + 1);
        prevIntervalWasZeroDur = PR_FALSE;
        continue;
      }
      prevIntervalWasZeroDur = PR_TRUE;
    }

    
    if (tempEnd->Time() > zeroTime ||
       (tempBegin->Time() == zeroTime && tempEnd->Time() == zeroTime)) {
      aResult.Set(*tempBegin, *tempEnd);
      return NS_OK;
    }

    if (mRestartMode == RESTART_NEVER) {
      
      return NS_ERROR_FAILURE;
    }

    beginAfter = tempEnd->Time();
  }
  NS_NOTREACHED("Hmm... we really shouldn't be here");

  return NS_ERROR_FAILURE;
}

nsSMILInstanceTime*
nsSMILTimedElement::GetNextGreater(const InstanceTimeList& aList,
                                   const nsSMILTimeValue& aBase,
                                   PRInt32& aPosition) const
{
  nsSMILInstanceTime* result = nsnull;
  while ((result = GetNextGreaterOrEqual(aList, aBase, aPosition)) &&
         result->Time() == aBase);
  return result;
}

nsSMILInstanceTime*
nsSMILTimedElement::GetNextGreaterOrEqual(const InstanceTimeList& aList,
                                          const nsSMILTimeValue& aBase,
                                          PRInt32& aPosition) const
{
  nsSMILInstanceTime* result = nsnull;
  PRInt32 count = aList.Length();

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

  NS_ABORT_IF_FALSE(mSimpleDur.IsResolved() || mSimpleDur.IsIndefinite(),
    "Unresolved simple duration in CalcActiveEnd");
  NS_ABORT_IF_FALSE(aBegin.IsResolved(),
    "Unresolved begin time in CalcActiveEnd");

  if (mRepeatDur.IsIndefinite()) {
    result.SetIndefinite();
  } else {
    result = GetRepeatDuration();
  }

  if (aEnd.IsResolved()) {
    nsSMILTime activeDur = aEnd.GetMillis() - aBegin.GetMillis();

    if (result.IsResolved()) {
      result.SetMillis(NS_MIN(result.GetMillis(), activeDur));
    } else {
      result.SetMillis(activeDur);
    }
  }

  result = ApplyMinAndMax(result);

  if (result.IsResolved()) {
    nsSMILTime activeEnd = result.GetMillis() + aBegin.GetMillis();
    result.SetMillis(activeEnd);
  }

  return result;
}

nsSMILTimeValue
nsSMILTimedElement::GetRepeatDuration() const
{
  nsSMILTimeValue result;

  if (mRepeatCount.IsDefinite() && mRepeatDur.IsResolved()) {
    if (mSimpleDur.IsResolved()) {
      nsSMILTime activeDur =
        nsSMILTime(mRepeatCount * double(mSimpleDur.GetMillis()));
      result.SetMillis(NS_MIN(activeDur, mRepeatDur.GetMillis()));
    } else {
      result = mRepeatDur;
    }
  } else if (mRepeatCount.IsDefinite() && mSimpleDur.IsResolved()) {
    nsSMILTime activeDur =
      nsSMILTime(mRepeatCount * double(mSimpleDur.GetMillis()));
    result.SetMillis(activeDur);
  } else if (mRepeatDur.IsResolved()) {
    result = mRepeatDur;
  } else if (mRepeatCount.IsIndefinite()) {
    result.SetIndefinite();
  } else {
    result = mSimpleDur;
  }

  return result;
}

nsSMILTimeValue
nsSMILTimedElement::ApplyMinAndMax(const nsSMILTimeValue& aDuration) const
{
  if (!aDuration.IsResolved() && !aDuration.IsIndefinite()) {
    return aDuration;
  }

  if (mMax < mMin) {
    return aDuration;
  }

  nsSMILTimeValue result;

  if (aDuration > mMax) {
    result = mMax;
  } else if (aDuration < mMin) {
    nsSMILTimeValue repeatDur = GetRepeatDuration();
    result = mMin > repeatDur ? repeatDur : mMin;
  } else {
    result = aDuration;
  }

  return result;
}

nsSMILTime
nsSMILTimedElement::ActiveTimeToSimpleTime(nsSMILTime aActiveTime,
                                           PRUint32& aRepeatIteration)
{
  nsSMILTime result;

  NS_ASSERTION(mSimpleDur.IsResolved() || mSimpleDur.IsIndefinite(),
      "Unresolved simple duration in ActiveTimeToSimpleTime");

  if (mSimpleDur.IsIndefinite() || mSimpleDur.GetMillis() == 0L) {
    aRepeatIteration = 0;
    result = aActiveTime;
  } else {
    result = aActiveTime % mSimpleDur.GetMillis();
    aRepeatIteration = (PRUint32)(aActiveTime / mSimpleDur.GetMillis());
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
    return nsnull;

  PRInt32 position = 0;
  nsSMILInstanceTime* nextBegin =
    GetNextGreater(mBeginInstances, mCurrentInterval->Begin()->Time(),
                   position);

  if (nextBegin &&
      nextBegin->Time() > mCurrentInterval->Begin()->Time() &&
      nextBegin->Time() < mCurrentInterval->End()->Time() &&
      nextBegin->Time() <= aContainerTime) {
    return nextBegin;
  }

  return nsnull;
}

void
nsSMILTimedElement::UpdateCurrentInterval(PRBool aForceChangeNotice)
{
  
  
  
  
  
  
  
  if (mElementState == STATE_STARTUP)
    return;

  
  const nsSMILInstanceTime* beginTime = mElementState == STATE_ACTIVE
                                      ? mCurrentInterval->Begin()
                                      : nsnull;
  nsSMILInterval updatedInterval;
  nsresult rv =
    GetNextInterval(GetPreviousInterval(), beginTime, updatedInterval);

  if (NS_SUCCEEDED(rv)) {

    if (mElementState == STATE_POSTACTIVE) {

      NS_ABORT_IF_FALSE(!mCurrentInterval,
          "In postactive state but the interval has been set");
      mCurrentInterval = new nsSMILInterval(updatedInterval);
      mElementState = STATE_WAITING;
      NotifyNewInterval();

    } else {

      PRBool changed = PR_FALSE;

      if (mElementState != STATE_ACTIVE &&
          !updatedInterval.Begin()->SameTimeAndBase(
            *mCurrentInterval->Begin())) {
        mCurrentInterval->SetBegin(*updatedInterval.Begin());
        changed = PR_TRUE;
      }

      if (!updatedInterval.End()->SameTimeAndBase(*mCurrentInterval->End())) {
        mCurrentInterval->SetEnd(*updatedInterval.End());
        changed = PR_TRUE;
      }

      if (changed || aForceChangeNotice) {
        NotifyChangedInterval();
      }
   }

    
    
    RegisterMilestone();
  } else {
    if (mElementState == STATE_ACTIVE && mClient) {
      
      
      PRBool applyFill = HasPlayed() && mFillMode == FILL_FREEZE;
      mClient->Inactivate(applyFill);
    }

    if (mElementState == STATE_ACTIVE || mElementState == STATE_WAITING) {
      mElementState = STATE_POSTACTIVE;
      mCurrentInterval->Unlink();
      mCurrentInterval = nsnull;
    }
  }
}

void
nsSMILTimedElement::SampleSimpleTime(nsSMILTime aActiveTime)
{
  if (mClient) {
    PRUint32 repeatIteration;
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

  const nsSMILInterval* prevInterval = GetPreviousInterval();
  NS_ABORT_IF_FALSE(prevInterval,
      "Attempting to sample fill value but there is no previous interval");
  NS_ABORT_IF_FALSE(prevInterval->End()->Time().IsResolved() &&
      prevInterval->End()->IsFixedTime(),
      "Attempting to sample fill value but the endpoint of the previous "
      "interval is not resolved and fixed");

  nsSMILTime activeTime = prevInterval->End()->Time().GetMillis() -
                          prevInterval->Begin()->Time().GetMillis();

  PRUint32 repeatIteration;
  nsSMILTime simpleTime =
    ActiveTimeToSimpleTime(activeTime, repeatIteration);

  if (simpleTime == 0L && repeatIteration) {
    mClient->SampleLastValue(--repeatIteration);
  } else {
    mClient->SampleAt(simpleTime, mSimpleDur, repeatIteration);
  }
}

void
nsSMILTimedElement::AddInstanceTimeFromCurrentTime(nsSMILTime aCurrentTime,
    double aOffsetSeconds, PRBool aIsBegin)
{
  double offset = aOffsetSeconds * PR_MSEC_PER_SEC;
  nsSMILTime timeWithOffset = aCurrentTime + PRInt64(NS_round(offset));

  nsSMILTimeValue timeVal(timeWithOffset);

  
  
  nsRefPtr<nsSMILInstanceTime> instanceTime =
    new nsSMILInstanceTime(timeVal, nsSMILInstanceTime::SOURCE_DOM);

  AddInstanceTime(instanceTime, aIsBegin);
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

PRBool
nsSMILTimedElement::GetNextMilestone(nsSMILMilestone& aNextMilestone) const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  switch (mElementState)
  {
  case STATE_STARTUP:
    
    
    aNextMilestone.mIsEnd = PR_TRUE; 
    aNextMilestone.mTime = 0;
    return PR_TRUE;

  case STATE_WAITING:
    NS_ABORT_IF_FALSE(mCurrentInterval,
        "In waiting state but the current interval has not been set");
    aNextMilestone.mIsEnd = PR_FALSE;
    aNextMilestone.mTime = mCurrentInterval->Begin()->Time().GetMillis();
    return PR_TRUE;

  case STATE_ACTIVE:
    {
      
      

      
      nsSMILInstanceTime* earlyEnd =
        CheckForEarlyEnd(mCurrentInterval->End()->Time());
      if (earlyEnd) {
        aNextMilestone.mIsEnd = PR_TRUE;
        aNextMilestone.mTime = earlyEnd->Time().GetMillis();
        return PR_TRUE;
      }

      
      if (mCurrentInterval->End()->Time().IsResolved()) {
        aNextMilestone.mIsEnd = PR_TRUE;
        aNextMilestone.mTime = mCurrentInterval->End()->Time().GetMillis();
        return PR_TRUE;
      }

      return PR_FALSE;
    }

  case STATE_POSTACTIVE:
    return PR_FALSE;

  default:
    NS_ABORT_IF_FALSE(PR_FALSE, "Invalid element state");
    return PR_FALSE;
  }
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

  NotifyTimeDependentsParams params = { mCurrentInterval, container };
  mTimeDependents.EnumerateEntries(NotifyNewIntervalCallback, &params);
}

void
nsSMILTimedElement::NotifyChangedInterval()
{
  NS_ABORT_IF_FALSE(mCurrentInterval,
      "Attempting to notify dependents of a changed interval but the interval "
      "is not set--perhaps we should be deleting the interval instead?");

  nsSMILTimeContainer* container = GetTimeContainer();
  if (container) {
    container->SyncPauseTime();
  }

  mCurrentInterval->NotifyChanged(container);
}

const nsSMILInstanceTime*
nsSMILTimedElement::GetEffectiveBeginInstance() const
{
  switch (mElementState)
  {
  case STATE_STARTUP:
    return nsnull;

  case STATE_ACTIVE:
    return mCurrentInterval->Begin();

  case STATE_WAITING:
  case STATE_POSTACTIVE:
    {
      const nsSMILInterval* prevInterval = GetPreviousInterval();
      return prevInterval ? prevInterval->Begin() : nsnull;
    }

  default:
    NS_NOTREACHED("Invalid element state");
    return nsnull;
  }
}

const nsSMILInterval*
nsSMILTimedElement::GetPreviousInterval() const
{
  return mOldIntervals.IsEmpty()
    ? nsnull
    : mOldIntervals[mOldIntervals.Length()-1].get();
}




 PR_CALLBACK PLDHashOperator
nsSMILTimedElement::NotifyNewIntervalCallback(TimeValueSpecPtrKey* aKey,
                                              void* aData)
{
  NS_ABORT_IF_FALSE(aKey, "Null hash key for time container hash table");
  NS_ABORT_IF_FALSE(aKey->GetKey(),
                    "null nsSMILTimeValueSpec in set of time dependents");

  NotifyTimeDependentsParams* params =
    static_cast<NotifyTimeDependentsParams*>(aData);
  NS_ABORT_IF_FALSE(params, "null data ptr while enumerating hashtable");
  NS_ABORT_IF_FALSE(params->mCurrentInterval, "null current-interval ptr");

  nsSMILTimeValueSpec* spec = aKey->GetKey();
  spec->HandleNewInterval(*params->mCurrentInterval, params->mTimeContainer);
  return PL_DHASH_NEXT;
}
