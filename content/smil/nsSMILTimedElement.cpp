




































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




nsSMILTimedElement::nsSMILTimedElement()
:
  mBeginSpecs(),
  mEndSpecs(),
  mFillMode(FILL_REMOVE),
  mRestartMode(RESTART_ALWAYS),
  mBeginSpecSet(PR_FALSE),
  mEndHasEventConditions(PR_FALSE),
  mClient(nsnull),
  mCurrentInterval(),
  mElementState(STATE_STARTUP)
{
  mSimpleDur.SetIndefinite();
  mMin.SetMillis(0L);
  mMax.SetIndefinite();
}

















nsresult
nsSMILTimedElement::BeginElementAt(double aOffsetSeconds,
                                   const nsSMILTimeContainer* aContainer)
{
  if (!aContainer)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = aContainer->GetCurrentTime();

  AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, PR_TRUE);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  SampleAt(currentTime);

  return NS_OK;
}

nsresult
nsSMILTimedElement::EndElementAt(double aOffsetSeconds,
                                 const nsSMILTimeContainer* aContainer)
{
  if (!aContainer)
    return NS_ERROR_FAILURE;

  nsSMILTime currentTime = aContainer->GetCurrentTime();
  AddInstanceTimeFromCurrentTime(currentTime, aOffsetSeconds, PR_FALSE);
  SampleAt(currentTime);

  return NS_OK;
}




nsSMILTimeValue
nsSMILTimedElement::GetStartTime() const
{
  nsSMILTimeValue startTime;

  switch (mElementState)
  {
  case STATE_STARTUP:
  case STATE_ACTIVE:
    startTime = mCurrentInterval.mBegin;
    break;

  case STATE_WAITING:
  case STATE_POSTACTIVE:
    if (!mOldIntervals.IsEmpty()) {
      startTime = mOldIntervals[mOldIntervals.Length() - 1].mBegin;
    } else {
      startTime = mCurrentInterval.mBegin;
    }
  }

  if (!startTime.IsResolved()) {
    startTime.SetIndefinite();
  }

  return startTime;
}




void
nsSMILTimedElement::AddInstanceTime(const nsSMILInstanceTime& aInstanceTime,
                                    PRBool aIsBegin)
{
  if (aIsBegin) {
    mBeginInstances.AppendElement(aInstanceTime);
  } else {
    mEndInstances.AppendElement(aInstanceTime);
  }

  UpdateCurrentInterval();
}

void
nsSMILTimedElement::SetTimeClient(nsSMILAnimationFunction* aClient)
{
  
  
  
  

  mClient = aClient;
}

void
nsSMILTimedElement::SampleAt(nsSMILTime aDocumentTime)
{
  PRBool          stateChanged;
  nsSMILTimeValue docTime;
  docTime.SetMillis(aDocumentTime);

  
  
  

  do {
    stateChanged = PR_FALSE;

    switch (mElementState)
    {
    case STATE_STARTUP:
      {
        mElementState =
         (NS_SUCCEEDED(GetNextInterval(nsnull, mCurrentInterval)))
         ? STATE_WAITING
         : STATE_POSTACTIVE;
        stateChanged = PR_TRUE;
      }
      break;

    case STATE_WAITING:
      {
        if (mCurrentInterval.mBegin.CompareTo(docTime) <= 0) {
          mElementState = STATE_ACTIVE;
          if (mClient) {
            mClient->Activate(mCurrentInterval.mBegin.GetMillis());
          }
          stateChanged = PR_TRUE;
        }
      }
      break;

    case STATE_ACTIVE:
      {
        CheckForEarlyEnd(docTime);
        if (mCurrentInterval.mEnd.CompareTo(docTime) <= 0) {
          nsSMILInterval newInterval;
          mElementState =
            (NS_SUCCEEDED(GetNextInterval(&mCurrentInterval, newInterval)))
            ? STATE_WAITING
            : STATE_POSTACTIVE;
          if (mClient) {
            mClient->Inactivate(mFillMode == FILL_FREEZE);
          }
          SampleFillValue();
          mOldIntervals.AppendElement(mCurrentInterval);
          mCurrentInterval = newInterval;
          Reset();
          stateChanged = PR_TRUE;
        } else {
          nsSMILTime beginTime = mCurrentInterval.mBegin.GetMillis();
          nsSMILTime activeTime = aDocumentTime - beginTime;
          SampleSimpleTime(activeTime);
        }
      }
      break;

    case STATE_POSTACTIVE:
      break;
    }
  } while (stateChanged);
}

void
nsSMILTimedElement::Reset()
{
  PRInt32 count = mBeginInstances.Length();

  for (PRInt32 i = count - 1; i >= 0; --i) {
    nsSMILInstanceTime &instance = mBeginInstances[i];
    
    
    
    if (instance.ClearOnReset()) {
      mBeginInstances.RemoveElementAt(i);
    }
  }

  count = mEndInstances.Length();

  for (PRInt32 j = count - 1; j >= 0; --j) {
    nsSMILInstanceTime &instance = mEndInstances[j];
    if (instance.ClearOnReset()) {
      mEndInstances.RemoveElementAt(j);
    }
  }
}

void
nsSMILTimedElement::HardReset()
{
  Reset();
  mCurrentInterval = nsSMILInterval();
  mElementState    = STATE_STARTUP;
  mOldIntervals.Clear();

  
  if (mClient) {
    mClient->Inactivate(PR_FALSE);
  }
}

PRBool
nsSMILTimedElement::SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                            nsAttrValue& aResult, nsresult* aParseResult)
{
  PRBool foundMatch = PR_TRUE;
  nsresult parseResult = NS_OK;

  if (aAttribute == nsGkAtoms::begin) {
    parseResult = SetBeginSpec(aValue);
  } else if (aAttribute == nsGkAtoms::dur) {
    parseResult = SetSimpleDuration(aValue);
  } else if (aAttribute == nsGkAtoms::end) {
    parseResult = SetEndSpec(aValue);
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
nsSMILTimedElement::SetBeginSpec(const nsAString& aBeginSpec)
{
  mBeginSpecSet = PR_TRUE;
  return SetBeginOrEndSpec(aBeginSpec, PR_TRUE);
}

void
nsSMILTimedElement::UnsetBeginSpec()
{
  mBeginSpecs.Clear();
  mBeginInstances.Clear();
  mBeginSpecSet = PR_FALSE;
  UpdateCurrentInterval();
}

nsresult
nsSMILTimedElement::SetEndSpec(const nsAString& aEndSpec)
{
  
  
  
  
  
  return SetBeginOrEndSpec(aEndSpec, PR_FALSE);
}

void
nsSMILTimedElement::UnsetEndSpec()
{
  mEndSpecs.Clear();
  mEndInstances.Clear();
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

  if (mFillMode != previousFillMode &&
      (mElementState == STATE_WAITING || mElementState == STATE_POSTACTIVE) &&
      mClient)
      mClient->Inactivate(mFillMode == FILL_FREEZE);

  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILTimedElement::UnsetFillMode()
{
  PRUint16 previousFillMode = mFillMode;
  mFillMode = FILL_REMOVE;
  if ((mElementState == STATE_WAITING || mElementState == STATE_POSTACTIVE) &&
      previousFillMode == FILL_FREEZE &&
      mClient)
    mClient->Inactivate(PR_FALSE);
}




nsresult
nsSMILTimedElement::SetBeginOrEndSpec(const nsAString& aSpec,
                                      PRBool aIsBegin)
{
  nsRefPtr<nsSMILTimeValueSpec> spec;
  SMILTimeValueSpecList& timeSpecsList = aIsBegin ? mBeginSpecs : mEndSpecs;
  nsTArray<nsSMILInstanceTime>& instancesList
    = aIsBegin ? mBeginInstances : mEndInstances;

  timeSpecsList.Clear();
  instancesList.Clear();
  HardReset(); 

  PRInt32 start;
  PRInt32 end = -1;
  PRInt32 length;

  do {
    start = end + 1;
    end = aSpec.FindChar(';', start);
    length = (end == -1) ? -1 : end - start;
    spec = NS_NewSMILTimeValueSpec(this, aIsBegin,
                                   Substring(aSpec, start, length));

    if (spec)
      timeSpecsList.AppendElement(spec);
  } while (end != -1 && spec);

  if (!spec) {
    timeSpecsList.Clear();
    instancesList.Clear();
    HardReset();
    return NS_ERROR_FAILURE;
  }

  UpdateCurrentInterval();

  return NS_OK;
}







nsresult
nsSMILTimedElement::GetNextInterval(const nsSMILInterval* aPrevInterval,
                                    nsSMILInterval& aResult)
{
  static nsSMILTimeValue zeroTime;
  zeroTime.SetMillis(0L);

  if (mRestartMode == RESTART_NEVER && aPrevInterval)
    return NS_ERROR_FAILURE;

  
  nsSMILTimeValue beginAfter;
  PRBool prevIntervalWasZeroDur = PR_FALSE;
  if (aPrevInterval) {
    beginAfter = aPrevInterval->mEnd;
    prevIntervalWasZeroDur
      = (aPrevInterval->mEnd.CompareTo(aPrevInterval->mBegin) == 0);
  } else {
    beginAfter.SetMillis(LL_MININT);
  }

  nsSMILTimeValue tempBegin;
  nsSMILTimeValue tempEnd;

  nsSMILInstanceTime::Comparator comparator;
  mBeginInstances.Sort(comparator);
  mEndInstances.Sort(comparator);

  while (PR_TRUE) {
    if (!mBeginSpecSet && beginAfter.CompareTo(zeroTime) <= 0) {
      tempBegin.SetMillis(0);
    } else {
      PRInt32 beginPos = 0;
      PRBool beginFound = GetNextGreaterOrEqual(mBeginInstances, beginAfter,
                                                beginPos, tempBegin);
      if (!beginFound)
        return NS_ERROR_FAILURE;
    }

    if (mEndInstances.Length() == 0) {
      nsSMILTimeValue indefiniteEnd;
      indefiniteEnd.SetIndefinite();

      tempEnd = CalcActiveEnd(tempBegin, indefiniteEnd);
    } else {
      PRInt32 endPos = 0;
      PRBool endFound = GetNextGreaterOrEqual(mEndInstances, tempBegin,
                                              endPos, tempEnd);

      
      
      if (tempEnd.CompareTo(tempBegin) == 0 && prevIntervalWasZeroDur) {
        endFound = GetNextGreater(mEndInstances, tempBegin, endPos, tempEnd);
      }

      if (!endFound) {
        if (mEndHasEventConditions || mEndInstances.Length() == 0) {
          tempEnd.SetUnresolved();
        } else {
          
          
          
          
          
          
          return NS_ERROR_FAILURE;
        }
      }

      tempEnd = CalcActiveEnd(tempBegin, tempEnd);
    }

    
    
    
    if (tempEnd.IsResolved() && tempBegin.CompareTo(tempEnd) == 0) {
      if (prevIntervalWasZeroDur) {
        beginAfter.SetMillis(tempEnd.GetMillis()+1);
        prevIntervalWasZeroDur = PR_FALSE;
        continue;
      }
      prevIntervalWasZeroDur = PR_TRUE;
    }

    if (tempEnd.CompareTo(zeroTime) > 0 ||
     (tempBegin.CompareTo(zeroTime) == 0 && tempEnd.CompareTo(zeroTime) == 0)) {
      aResult.mBegin = tempBegin;
      aResult.mEnd = tempEnd;
      return NS_OK;
    } else if (mRestartMode == RESTART_NEVER) {
      
      return NS_ERROR_FAILURE;
    } else {
      beginAfter = tempEnd;
    }
  }
  NS_NOTREACHED("Hmm... we really shouldn't be here");

  return NS_ERROR_FAILURE;
}

PRBool
nsSMILTimedElement::GetNextGreater(
    const nsTArray<nsSMILInstanceTime>& aList,
    const nsSMILTimeValue& aBase,
    PRInt32 &aPosition,
    nsSMILTimeValue& aResult)
{
  PRBool found;
  while ((found = GetNextGreaterOrEqual(aList, aBase, aPosition, aResult))
         && aResult.CompareTo(aBase) == 0);
  return found;
}

PRBool
nsSMILTimedElement::GetNextGreaterOrEqual(
    const nsTArray<nsSMILInstanceTime>& aList,
    const nsSMILTimeValue& aBase,
    PRInt32 &aPosition,
    nsSMILTimeValue& aResult)
{
  PRBool found = PR_FALSE;
  PRInt32 count = aList.Length();

  for (; aPosition < count && !found; ++aPosition) {
    const nsSMILInstanceTime &val = aList[aPosition];
    if (val.Time().CompareTo(aBase) >= 0) {
      aResult = val.Time();
      found = PR_TRUE;
    }
  }

  return found;
}




nsSMILTimeValue
nsSMILTimedElement::CalcActiveEnd(const nsSMILTimeValue& aBegin,
                                  const nsSMILTimeValue& aEnd)
{
  nsSMILTimeValue result;

  NS_ASSERTION(mSimpleDur.IsResolved() || mSimpleDur.IsIndefinite(),
    "Unresolved simple duration in CalcActiveEnd.");

  if (!aBegin.IsResolved() && !aBegin.IsIndefinite()) {
    NS_ERROR("Unresolved begin time passed to CalcActiveEnd.");
    result.SetIndefinite();
    return result;
  }

  if (mRepeatDur.IsIndefinite() || aBegin.IsIndefinite()) {
    result.SetIndefinite();
  } else {
    result = GetRepeatDuration();
  }

  if (aEnd.IsResolved() && aBegin.IsResolved()) {
    nsSMILTime activeDur = aEnd.GetMillis() - aBegin.GetMillis();

    if (result.IsResolved()) {
      result.SetMillis(PR_MIN(result.GetMillis(), activeDur));
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
nsSMILTimedElement::GetRepeatDuration()
{
  nsSMILTimeValue result;

  if (mRepeatCount.IsDefinite() && mRepeatDur.IsResolved()) {
    if (mSimpleDur.IsResolved()) {
      nsSMILTime activeDur = mRepeatCount * double(mSimpleDur.GetMillis());
      result.SetMillis(PR_MIN(activeDur, mRepeatDur.GetMillis()));
    } else {
      result = mRepeatDur;
    }
  } else if (mRepeatCount.IsDefinite() && mSimpleDur.IsResolved()) {
    nsSMILTime activeDur = mRepeatCount * double(mSimpleDur.GetMillis());
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
nsSMILTimedElement::ApplyMinAndMax(const nsSMILTimeValue& aDuration)
{
  if (!aDuration.IsResolved() && !aDuration.IsIndefinite()) {
    return aDuration;
  }

  if (mMax.CompareTo(mMin) < 0) {
    return aDuration;
  }

  nsSMILTimeValue result;

  if (aDuration.CompareTo(mMax) > 0) {
    result = mMax;
  } else if (aDuration.CompareTo(mMin) < 0) {
    nsSMILTimeValue repeatDur = GetRepeatDuration();
    result = (mMin.CompareTo(repeatDur) > 0) ? repeatDur : mMin;
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













void
nsSMILTimedElement::CheckForEarlyEnd(const nsSMILTimeValue& aDocumentTime)
{
  if (mRestartMode != RESTART_ALWAYS)
    return;

  nsSMILTimeValue nextBegin;
  PRInt32 position = 0;

  GetNextGreater(mBeginInstances, mCurrentInterval.mBegin, position, nextBegin);

  if (nextBegin.IsResolved() &&
      nextBegin.CompareTo(mCurrentInterval.mBegin) > 0 &&
      nextBegin.CompareTo(mCurrentInterval.mEnd) < 0 &&
      nextBegin.CompareTo(aDocumentTime) <= 0) {
    mCurrentInterval.mEnd = nextBegin;
  }
}

void
nsSMILTimedElement::UpdateCurrentInterval()
{
  nsSMILInterval updatedInterval;
  nsSMILInterval* prevInterval = mOldIntervals.IsEmpty()
                               ? nsnull
                               : &mOldIntervals[mOldIntervals.Length() - 1];
  nsresult rv = GetNextInterval(prevInterval, updatedInterval);

  if (NS_SUCCEEDED(rv)) {

    if (mElementState != STATE_ACTIVE &&
        updatedInterval.mBegin.CompareTo(mCurrentInterval.mBegin)) {
      mCurrentInterval.mBegin = updatedInterval.mBegin;
    }

    if (updatedInterval.mEnd.CompareTo(mCurrentInterval.mEnd)) {
      mCurrentInterval.mEnd = updatedInterval.mEnd;
    }

    if (mElementState == STATE_POSTACTIVE) {
      
      mElementState = STATE_WAITING;
    }
  } else {

    nsSMILTimeValue unresolvedTime;
    mCurrentInterval.mEnd = unresolvedTime;
    if (mElementState != STATE_ACTIVE) {
      mCurrentInterval.mBegin = unresolvedTime;
    }

    if (mElementState == STATE_WAITING) {
      
      mElementState = STATE_POSTACTIVE;
    }

    if (mElementState == STATE_ACTIVE) {
      
      mElementState = STATE_POSTACTIVE;
      if (mClient) {
        mClient->Inactivate(PR_FALSE);
      }
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
  if (mFillMode != FILL_FREEZE)
    return;

  if (!mClient)
    return;

  PRUint32 repeatIteration;
  nsSMILTime activeTime =
    mCurrentInterval.mEnd.GetMillis() - mCurrentInterval.mBegin.GetMillis();

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

  nsSMILTimeValue timeVal;
  timeVal.SetMillis(timeWithOffset);

  nsSMILInstanceTime instanceTime(timeVal, nsnull, PR_TRUE);
  AddInstanceTime(instanceTime, aIsBegin);
}
