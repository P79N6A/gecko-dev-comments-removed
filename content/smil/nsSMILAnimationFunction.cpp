






































#include "nsSMILAnimationFunction.h"
#include "nsISMILAttr.h"
#include "nsSMILParserUtils.h"
#include "nsSMILNullType.h"
#include "nsISMILAnimationElement.h"
#include "nsSMILTimedElement.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include <math.h>




nsAttrValue::EnumTable nsSMILAnimationFunction::sAccumulateTable[] = {
      {"none", PR_FALSE},
      {"sum", PR_TRUE},
      {nsnull, 0}
};

nsAttrValue::EnumTable nsSMILAnimationFunction::sAdditiveTable[] = {
      {"replace", PR_FALSE},
      {"sum", PR_TRUE},
      {nsnull, 0}
};

nsAttrValue::EnumTable nsSMILAnimationFunction::sCalcModeTable[] = {
      {"linear", CALC_LINEAR},
      {"discrete", CALC_DISCRETE},
      {"paced", CALC_PACED},
      {"spline", CALC_SPLINE},
      {nsnull, 0}
};



#define COMPUTE_DISTANCE_ERROR (-1)




nsSMILAnimationFunction::nsSMILAnimationFunction()
  : mSampleTime(-1),
    mRepeatIteration(0),
    mBeginTime(LL_MININT),
    mAnimationElement(nsnull),
    mErrorFlags(0),
    mIsActive(PR_FALSE),
    mIsFrozen(PR_FALSE),
    mLastValue(PR_FALSE),
    mHasChanged(PR_TRUE),
    mValueNeedsReparsingEverySample(PR_FALSE),
    mPrevSampleWasSingleValueAnimation(PR_FALSE)
{
}

void
nsSMILAnimationFunction::SetAnimationElement(
    nsISMILAnimationElement* aAnimationElement)
{
  mAnimationElement = aAnimationElement;
}

PRBool
nsSMILAnimationFunction::SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                                 nsAttrValue& aResult, nsresult* aParseResult)
{
  PRBool foundMatch = PR_TRUE;
  nsresult parseResult = NS_OK;

  
  
  
  if (aAttribute == nsGkAtoms::by ||
      aAttribute == nsGkAtoms::from ||
      aAttribute == nsGkAtoms::to ||
      aAttribute == nsGkAtoms::values) {
    
    
    
    mHasChanged = PR_TRUE;
    aResult.SetTo(aValue);
  } else if (aAttribute == nsGkAtoms::accumulate) {
    parseResult = SetAccumulate(aValue, aResult);
  } else if (aAttribute == nsGkAtoms::additive) {
    parseResult = SetAdditive(aValue, aResult);
  } else if (aAttribute == nsGkAtoms::calcMode) {
    parseResult = SetCalcMode(aValue, aResult);
  } else if (aAttribute == nsGkAtoms::keyTimes) {
    parseResult = SetKeyTimes(aValue, aResult);
  } else if (aAttribute == nsGkAtoms::keySplines) {
    parseResult = SetKeySplines(aValue, aResult);
  } else {
    foundMatch = PR_FALSE;
  }

  if (foundMatch && aParseResult) {
    *aParseResult = parseResult;
  }

  return foundMatch;
}

PRBool
nsSMILAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  PRBool foundMatch = PR_TRUE;

  if (aAttribute == nsGkAtoms::by ||
      aAttribute == nsGkAtoms::from ||
      aAttribute == nsGkAtoms::to ||
      aAttribute == nsGkAtoms::values) {
    mHasChanged = PR_TRUE;
  } else if (aAttribute == nsGkAtoms::accumulate) {
    UnsetAccumulate();
  } else if (aAttribute == nsGkAtoms::additive) {
    UnsetAdditive();
  } else if (aAttribute == nsGkAtoms::calcMode) {
    UnsetCalcMode();
  } else if (aAttribute == nsGkAtoms::keyTimes) {
    UnsetKeyTimes();
  } else if (aAttribute == nsGkAtoms::keySplines) {
    UnsetKeySplines();
  } else {
    foundMatch = PR_FALSE;
  }

  return foundMatch;
}

void
nsSMILAnimationFunction::SampleAt(nsSMILTime aSampleTime,
                                  const nsSMILTimeValue& aSimpleDuration,
                                  PRUint32 aRepeatIteration)
{
  
  
  mHasChanged |= mLastValue;

  
  mHasChanged |=
    (mSampleTime != aSampleTime || mSimpleDuration != aSimpleDuration) &&
    !IsValueFixedForSimpleDuration();

  
  mHasChanged |= (mRepeatIteration != aRepeatIteration) && GetAccumulate();

  mSampleTime       = aSampleTime;
  mSimpleDuration   = aSimpleDuration;
  mRepeatIteration  = aRepeatIteration;
  mLastValue        = PR_FALSE;
}

void
nsSMILAnimationFunction::SampleLastValue(PRUint32 aRepeatIteration)
{
  if (mHasChanged || !mLastValue || mRepeatIteration != aRepeatIteration) {
    mHasChanged = PR_TRUE;
  }

  mRepeatIteration  = aRepeatIteration;
  mLastValue        = PR_TRUE;
}

void
nsSMILAnimationFunction::Activate(nsSMILTime aBeginTime)
{
  mBeginTime = aBeginTime;
  mIsActive = PR_TRUE;
  mIsFrozen = PR_FALSE;
  mFrozenValue = nsSMILValue();
  mHasChanged = PR_TRUE;
}

void
nsSMILAnimationFunction::Inactivate(PRBool aIsFrozen)
{
  mIsActive = PR_FALSE;
  mIsFrozen = aIsFrozen;
  mFrozenValue = nsSMILValue();
  mHasChanged = PR_TRUE;
}

void
nsSMILAnimationFunction::ComposeResult(const nsISMILAttr& aSMILAttr,
                                       nsSMILValue& aResult)
{
  mHasChanged = PR_FALSE;
  mPrevSampleWasSingleValueAnimation = PR_FALSE;

  
  if (!IsActiveOrFrozen() || mErrorFlags != 0)
    return;

  
  nsSMILValueArray values;
  nsresult rv = GetValues(aSMILAttr, values);
  if (NS_FAILED(rv))
    return;

  
  CheckValueListDependentAttrs(values.Length());
  if (mErrorFlags != 0)
    return;

  
  NS_ABORT_IF_FALSE(mSampleTime >= 0 || !mIsActive,
      "Negative sample time for active animation");
  NS_ABORT_IF_FALSE(mSimpleDuration.IsResolved() ||
      mSimpleDuration.IsIndefinite() || mLastValue,
      "Unresolved simple duration for active or frozen animation");

  
  
  
  
  PRBool isAdditive = IsAdditive();
  if (isAdditive && aResult.IsNull())
    return;

  nsSMILValue result;

  if (values.Length() == 1 && !IsToAnimation()) {

    
    result = values[0];
    mPrevSampleWasSingleValueAnimation = PR_TRUE;

  } else if (mLastValue) {

    
    const nsSMILValue& last = values[values.Length() - 1];
    result = last;

    
    if (!IsToAnimation() && GetAccumulate() && mRepeatIteration) {
      
      
      result.Add(last, mRepeatIteration);
    }

  } else if (!mFrozenValue.IsNull() && !mHasChanged) {

    
    result = mFrozenValue;

  } else {

    
    if (NS_FAILED(InterpolateResult(values, result, aResult)))
      return;

    if (NS_FAILED(AccumulateResult(values, result)))
      return;

    if (IsToAnimation() && mIsFrozen) {
      mFrozenValue = result;
    }
  }

  
  if (!isAdditive || NS_FAILED(aResult.SandwichAdd(result))) {
    aResult.Swap(result);
    
    
  }
}

PRInt8
nsSMILAnimationFunction::CompareTo(const nsSMILAnimationFunction* aOther) const
{
  NS_ENSURE_TRUE(aOther, 0);

  NS_ASSERTION(aOther != this, "Trying to compare to self");

  
  if (!IsActiveOrFrozen() && aOther->IsActiveOrFrozen())
    return -1;

  if (IsActiveOrFrozen() && !aOther->IsActiveOrFrozen())
    return 1;

  
  if (mBeginTime != aOther->GetBeginTime())
    return mBeginTime > aOther->GetBeginTime() ? 1 : -1;

  
  
  const nsSMILTimedElement& thisTimedElement =
    mAnimationElement->TimedElement();
  const nsSMILTimedElement& otherTimedElement =
    aOther->mAnimationElement->TimedElement();
  if (thisTimedElement.IsTimeDependent(otherTimedElement))
    return 1;
  if (otherTimedElement.IsTimeDependent(thisTimedElement))
    return -1;

  
  
  nsIContent& thisContent = mAnimationElement->AsElement();
  nsIContent& otherContent = aOther->mAnimationElement->AsElement();

  NS_ABORT_IF_FALSE(&thisContent != &otherContent,
      "Two animations cannot have the same animation content element!");

  return (nsContentUtils::PositionIsBefore(&thisContent, &otherContent))
          ? -1 : 1;
}

PRBool
nsSMILAnimationFunction::WillReplace() const
{
  





  return !mErrorFlags && (!(IsAdditive() || IsToAnimation()) ||
                          (IsToAnimation() && mIsFrozen && !mHasChanged));
}

PRBool
nsSMILAnimationFunction::HasChanged() const
{
  return mHasChanged || mValueNeedsReparsingEverySample;
}

PRBool
nsSMILAnimationFunction::UpdateCachedTarget(const nsSMILTargetIdentifier& aNewTarget)
{
  if (!mLastTarget.Equals(aNewTarget)) {
    mLastTarget = aNewTarget;
    return PR_TRUE;
  }
  return PR_FALSE;
}




nsresult
nsSMILAnimationFunction::InterpolateResult(const nsSMILValueArray& aValues,
                                           nsSMILValue& aResult,
                                           nsSMILValue& aBaseValue)
{
  
  if ((!IsToAnimation() && aValues.Length() < 2) ||
      (IsToAnimation()  && aValues.Length() != 1)) {
    NS_ERROR("Unexpected number of values");
    return NS_ERROR_FAILURE;
  }

  if (IsToAnimation() && aBaseValue.IsNull()) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  
  double simpleProgress = 0.0;

  if (mSimpleDuration.IsResolved()) {
    nsSMILTime dur = mSimpleDuration.GetMillis();

    NS_ABORT_IF_FALSE(dur >= 0, "Simple duration should not be negative");
    NS_ABORT_IF_FALSE(mSampleTime >= 0, "Sample time should not be negative");

    if (mSampleTime >= dur || mSampleTime < 0) {
      NS_ERROR("Animation sampled outside interval");
      return NS_ERROR_FAILURE;
    }

    if (dur > 0) {
      simpleProgress = (double)mSampleTime / dur;
    } 
  }

  nsresult rv = NS_OK;
  nsSMILCalcMode calcMode = GetCalcMode();
  if (calcMode != CALC_DISCRETE) {
    
    const nsSMILValue* from = nsnull;
    const nsSMILValue* to = nsnull;
    
    
    double intervalProgress = -1.f;
    if (IsToAnimation()) {
      from = &aBaseValue;
      to = &aValues[0];
      if (calcMode == CALC_PACED) {
        
        intervalProgress = simpleProgress;
      } else {
        double scaledSimpleProgress =
          ScaleSimpleProgress(simpleProgress, calcMode);
        intervalProgress = ScaleIntervalProgress(scaledSimpleProgress, 0);
      }
    } else if (calcMode == CALC_PACED) {
      rv = ComputePacedPosition(aValues, simpleProgress,
                                intervalProgress, from, to);
      
      
      
      
    } else { 
      double scaledSimpleProgress =
        ScaleSimpleProgress(simpleProgress, calcMode);
      PRUint32 index = (PRUint32)floor(scaledSimpleProgress *
                                       (aValues.Length() - 1));
      from = &aValues[index];
      to = &aValues[index + 1];
      intervalProgress =
        scaledSimpleProgress * (aValues.Length() - 1) - index;
      intervalProgress = ScaleIntervalProgress(intervalProgress, index);
    }

    if (NS_SUCCEEDED(rv)) {
      NS_ABORT_IF_FALSE(from, "NULL from-value during interpolation");
      NS_ABORT_IF_FALSE(to, "NULL to-value during interpolation");
      NS_ABORT_IF_FALSE(0.0f <= intervalProgress && intervalProgress < 1.0f,
                      "Interval progress should be in the range [0, 1)");
      rv = from->Interpolate(*to, intervalProgress, aResult);
    }
  }

  
  
  
  if (calcMode == CALC_DISCRETE || NS_FAILED(rv)) {
    double scaledSimpleProgress =
      ScaleSimpleProgress(simpleProgress, CALC_DISCRETE);
    if (IsToAnimation()) {
      
      
      
      
      PRUint32 index = (PRUint32)floor(scaledSimpleProgress * 2);
      aResult = index == 0 ? aBaseValue : aValues[0];
    } else {
      PRUint32 index = (PRUint32)floor(scaledSimpleProgress * aValues.Length());
      aResult = aValues[index];
    }
    rv = NS_OK;
  }
  return rv;
}

nsresult
nsSMILAnimationFunction::AccumulateResult(const nsSMILValueArray& aValues,
                                          nsSMILValue& aResult)
{
  if (!IsToAnimation() && GetAccumulate() && mRepeatIteration)
  {
    const nsSMILValue& lastValue = aValues[aValues.Length() - 1];

    
    
    aResult.Add(lastValue, mRepeatIteration);
  }

  return NS_OK;
}











nsresult
nsSMILAnimationFunction::ComputePacedPosition(const nsSMILValueArray& aValues,
                                              double aSimpleProgress,
                                              double& aIntervalProgress,
                                              const nsSMILValue*& aFrom,
                                              const nsSMILValue*& aTo)
{
  NS_ASSERTION(0.0f <= aSimpleProgress && aSimpleProgress < 1.0f,
               "aSimpleProgress is out of bounds");
  NS_ASSERTION(GetCalcMode() == CALC_PACED,
               "Calling paced-specific function, but not in paced mode");
  NS_ABORT_IF_FALSE(aValues.Length() >= 2, "Unexpected number of values");

  
  
  
  if (aValues.Length() == 2) {
    aIntervalProgress = aSimpleProgress;
    aFrom = &aValues[0];
    aTo = &aValues[1];
    return NS_OK;
  }

  double totalDistance = ComputePacedTotalDistance(aValues);
  if (totalDistance == COMPUTE_DISTANCE_ERROR)
    return NS_ERROR_FAILURE;

  
  
  double remainingDist = aSimpleProgress * totalDistance;

  
  
  NS_ASSERTION(remainingDist >= 0, "distance values must be non-negative");

  
  
  
  for (PRUint32 i = 0; i < aValues.Length() - 1; i++) {
    
    
    
    NS_ASSERTION(remainingDist >= 0, "distance values must be non-negative");

    double curIntervalDist;

#ifdef DEBUG
    nsresult rv =
#endif
      aValues[i].ComputeDistance(aValues[i+1], curIntervalDist);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv),
                      "If we got through ComputePacedTotalDistance, we should "
                      "be able to recompute each sub-distance without errors");

    NS_ASSERTION(curIntervalDist >= 0, "distance values must be non-negative");
    
    curIntervalDist = NS_MAX(curIntervalDist, 0.0);

    if (remainingDist >= curIntervalDist) {
      remainingDist -= curIntervalDist;
    } else {
      
      
      
      
      
      NS_ASSERTION(curIntervalDist != 0,
                   "We should never get here with this set to 0...");

      
      
      aFrom = &aValues[i];
      aTo = &aValues[i+1];
      aIntervalProgress = remainingDist / curIntervalDist;
      return NS_OK;
    }
  }

  NS_NOTREACHED("shouldn't complete loop & get here -- if we do, "
                "then aSimpleProgress was probably out of bounds");
  return NS_ERROR_FAILURE;
}







double
nsSMILAnimationFunction::ComputePacedTotalDistance(
    const nsSMILValueArray& aValues) const
{
  NS_ASSERTION(GetCalcMode() == CALC_PACED,
               "Calling paced-specific function, but not in paced mode");

  double totalDistance = 0.0;
  for (PRUint32 i = 0; i < aValues.Length() - 1; i++) {
    double tmpDist;
    nsresult rv = aValues[i].ComputeDistance(aValues[i+1], tmpDist);
    if (NS_FAILED(rv)) {
      return COMPUTE_DISTANCE_ERROR;
    }

    
    
    NS_ABORT_IF_FALSE(tmpDist >= 0.0f, "distance values must be non-negative");
    tmpDist = NS_MAX(tmpDist, 0.0);

    totalDistance += tmpDist;
  }

  return totalDistance;
}

double
nsSMILAnimationFunction::ScaleSimpleProgress(double aProgress,
                                             nsSMILCalcMode aCalcMode)
{
  if (!HasAttr(nsGkAtoms::keyTimes))
    return aProgress;

  PRUint32 numTimes = mKeyTimes.Length();

  if (numTimes < 2)
    return aProgress;

  PRUint32 i = 0;
  for (; i < numTimes - 2 && aProgress >= mKeyTimes[i+1]; ++i);

  if (aCalcMode == CALC_DISCRETE) {
    
    
    
    
    if (aProgress >= mKeyTimes[i+1]) {
      NS_ABORT_IF_FALSE(i == numTimes - 2,
          "aProgress is not in range of the current interval, yet the current"
          " interval is not the last bounded interval either.");
      ++i;
    }
    return (double)i / numTimes;
  }

  double& intervalStart = mKeyTimes[i];
  double& intervalEnd   = mKeyTimes[i+1];

  double intervalLength = intervalEnd - intervalStart;
  if (intervalLength <= 0.0)
    return intervalStart;

  return (i + (aProgress - intervalStart) / intervalLength) /
         double(numTimes - 1);
}

double
nsSMILAnimationFunction::ScaleIntervalProgress(double aProgress,
                                               PRUint32 aIntervalIndex)
{
  if (GetCalcMode() != CALC_SPLINE)
    return aProgress;

  if (!HasAttr(nsGkAtoms::keySplines))
    return aProgress;

  NS_ABORT_IF_FALSE(aIntervalIndex < mKeySplines.Length(),
                    "Invalid interval index");

  nsSMILKeySpline const &spline = mKeySplines[aIntervalIndex];
  return spline.GetSplineValue(aProgress);
}

PRBool
nsSMILAnimationFunction::HasAttr(nsIAtom* aAttName) const
{
  return mAnimationElement->HasAnimAttr(aAttName);
}

const nsAttrValue*
nsSMILAnimationFunction::GetAttr(nsIAtom* aAttName) const
{
  return mAnimationElement->GetAnimAttr(aAttName);
}

PRBool
nsSMILAnimationFunction::GetAttr(nsIAtom* aAttName, nsAString& aResult) const
{
  return mAnimationElement->GetAnimAttr(aAttName, aResult);
}

















PRBool
nsSMILAnimationFunction::ParseAttr(nsIAtom* aAttName,
                                   const nsISMILAttr& aSMILAttr,
                                   nsSMILValue& aResult,
                                   PRBool& aPreventCachingOfSandwich) const
{
  nsAutoString attValue;
  if (GetAttr(aAttName, attValue)) {
    PRBool preventCachingOfSandwich;
    nsresult rv = aSMILAttr.ValueFromString(attValue, mAnimationElement,
                                            aResult, preventCachingOfSandwich);
    if (NS_FAILED(rv))
      return PR_FALSE;

    if (preventCachingOfSandwich) {
      aPreventCachingOfSandwich = PR_TRUE;
    }
  }
  return PR_TRUE;
}















nsresult
nsSMILAnimationFunction::GetValues(const nsISMILAttr& aSMILAttr,
                                   nsSMILValueArray& aResult)
{
  if (!mAnimationElement)
    return NS_ERROR_FAILURE;

  mValueNeedsReparsingEverySample = PR_FALSE;
  nsSMILValueArray result;

  
  if (HasAttr(nsGkAtoms::values)) {
    nsAutoString attValue;
    GetAttr(nsGkAtoms::values, attValue);
    PRBool preventCachingOfSandwich;
    nsresult rv = nsSMILParserUtils::ParseValues(attValue, mAnimationElement,
                                                 aSMILAttr, result,
                                                 preventCachingOfSandwich);
    if (NS_FAILED(rv))
      return rv;

    if (preventCachingOfSandwich) {
      mValueNeedsReparsingEverySample = PR_TRUE;
    }
  
  } else {
    PRBool preventCachingOfSandwich = PR_FALSE;
    PRBool parseOk = PR_TRUE;
    nsSMILValue to, from, by;
    parseOk &= ParseAttr(nsGkAtoms::to,   aSMILAttr, to,
                         preventCachingOfSandwich);
    parseOk &= ParseAttr(nsGkAtoms::from, aSMILAttr, from,
                         preventCachingOfSandwich);
    parseOk &= ParseAttr(nsGkAtoms::by,   aSMILAttr, by,
                         preventCachingOfSandwich);
    
    if (preventCachingOfSandwich) {
      mValueNeedsReparsingEverySample = PR_TRUE;
    }

    if (!parseOk)
      return NS_ERROR_FAILURE;

    result.SetCapacity(2);
    if (!to.IsNull()) {
      if (!from.IsNull()) {
        result.AppendElement(from);
        result.AppendElement(to);
      } else {
        result.AppendElement(to);
      }
    } else if (!by.IsNull()) {
      nsSMILValue effectiveFrom(by.mType);
      if (!from.IsNull())
        effectiveFrom = from;
      
      result.AppendElement(effectiveFrom);
      nsSMILValue effectiveTo(effectiveFrom);
      if (!effectiveTo.IsNull() && NS_SUCCEEDED(effectiveTo.Add(by))) {
        result.AppendElement(effectiveTo);
      } else {
        
        return NS_ERROR_FAILURE;
      }
    } else {
      
      return NS_ERROR_FAILURE;
    }
  }

  result.SwapElements(aResult);

  return NS_OK;
}

void
nsSMILAnimationFunction::CheckValueListDependentAttrs(PRUint32 aNumValues)
{
  CheckKeyTimes(aNumValues);
  CheckKeySplines(aNumValues);
}






void
nsSMILAnimationFunction::CheckKeyTimes(PRUint32 aNumValues)
{
  if (!HasAttr(nsGkAtoms::keyTimes))
    return;

  nsSMILCalcMode calcMode = GetCalcMode();

  
  if (calcMode == CALC_PACED) {
    SetKeyTimesErrorFlag(PR_FALSE);
    return;
  }

  PRUint32 numKeyTimes = mKeyTimes.Length();
  if (numKeyTimes < 1) {
    
    SetKeyTimesErrorFlag(PR_TRUE);
    return;
  }

  
  
  PRBool matchingNumOfValues =
    numKeyTimes == (IsToAnimation() ? 2 : aNumValues);
  if (!matchingNumOfValues) {
    SetKeyTimesErrorFlag(PR_TRUE);
    return;
  }

  
  if (mKeyTimes[0] != 0.0) {
    SetKeyTimesErrorFlag(PR_TRUE);
    return;
  }

  
  if (calcMode != CALC_DISCRETE && numKeyTimes > 1 &&
      mKeyTimes[numKeyTimes - 1] != 1.0) {
    SetKeyTimesErrorFlag(PR_TRUE);
    return;
  }

  SetKeyTimesErrorFlag(PR_FALSE);
}

void
nsSMILAnimationFunction::CheckKeySplines(PRUint32 aNumValues)
{
  
  if (GetCalcMode() != CALC_SPLINE) {
    SetKeySplinesErrorFlag(PR_FALSE);
    return;
  }

  
  if (!HasAttr(nsGkAtoms::keySplines)) {
    SetKeySplinesErrorFlag(PR_FALSE);
    return;
  }

  if (mKeySplines.Length() < 1) {
    
    SetKeySplinesErrorFlag(PR_TRUE);
    return;
  }

  
  if (aNumValues == 1 && !IsToAnimation()) {
    SetKeySplinesErrorFlag(PR_FALSE);
    return;
  }

  
  PRUint32 splineSpecs = mKeySplines.Length();
  if ((splineSpecs != aNumValues - 1 && !IsToAnimation()) ||
      (IsToAnimation() && splineSpecs != 1)) {
    SetKeySplinesErrorFlag(PR_TRUE);
    return;
  }

  SetKeySplinesErrorFlag(PR_FALSE);
}

PRBool
nsSMILAnimationFunction::IsValueFixedForSimpleDuration() const
{
  return mSimpleDuration.IsIndefinite() ||
    (!mHasChanged && mPrevSampleWasSingleValueAnimation);
}




PRBool
nsSMILAnimationFunction::GetAccumulate() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::accumulate);
  if (!value)
    return PR_FALSE;

  return value->GetEnumValue();
}

PRBool
nsSMILAnimationFunction::GetAdditive() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::additive);
  if (!value)
    return PR_FALSE;

  return value->GetEnumValue();
}

nsSMILAnimationFunction::nsSMILCalcMode
nsSMILAnimationFunction::GetCalcMode() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::calcMode);
  if (!value)
    return CALC_LINEAR;

  return nsSMILCalcMode(value->GetEnumValue());
}




nsresult
nsSMILAnimationFunction::SetAccumulate(const nsAString& aAccumulate,
                                       nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;
  PRBool parseResult =
    aResult.ParseEnumValue(aAccumulate, sAccumulateTable, PR_TRUE);
  SetAccumulateErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAccumulate()
{
  SetAccumulateErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetAdditive(const nsAString& aAdditive,
                                     nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;
  PRBool parseResult
    = aResult.ParseEnumValue(aAdditive, sAdditiveTable, PR_TRUE);
  SetAdditiveErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAdditive()
{
  SetAdditiveErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetCalcMode(const nsAString& aCalcMode,
                                     nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;
  PRBool parseResult
    = aResult.ParseEnumValue(aCalcMode, sCalcModeTable, PR_TRUE);
  SetCalcModeErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetCalcMode()
{
  SetCalcModeErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetKeySplines(const nsAString& aKeySplines,
                                       nsAttrValue& aResult)
{
  mKeySplines.Clear();
  aResult.SetTo(aKeySplines);

  nsTArray<double> keySplines;
  nsresult rv = nsSMILParserUtils::ParseKeySplines(aKeySplines, keySplines);

  if (keySplines.Length() < 1 || keySplines.Length() % 4)
    rv = NS_ERROR_FAILURE;

  if (NS_SUCCEEDED(rv))
  {
    mKeySplines.SetCapacity(keySplines.Length() % 4);
    for (PRUint32 i = 0; i < keySplines.Length() && NS_SUCCEEDED(rv); i += 4)
    {
      if (!mKeySplines.AppendElement(nsSMILKeySpline(keySplines[i],
                                                     keySplines[i+1],
                                                     keySplines[i+2],
                                                     keySplines[i+3]))) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  mHasChanged = PR_TRUE;

  return rv;
}

void
nsSMILAnimationFunction::UnsetKeySplines()
{
  mKeySplines.Clear();
  SetKeySplinesErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetKeyTimes(const nsAString& aKeyTimes,
                                     nsAttrValue& aResult)
{
  mKeyTimes.Clear();
  aResult.SetTo(aKeyTimes);

  nsresult rv =
    nsSMILParserUtils::ParseSemicolonDelimitedProgressList(aKeyTimes, PR_TRUE,
                                                           mKeyTimes);

  if (NS_SUCCEEDED(rv) && mKeyTimes.Length() < 1)
    rv = NS_ERROR_FAILURE;

  if (NS_FAILED(rv))
    mKeyTimes.Clear();

  mHasChanged = PR_TRUE;

  return NS_OK;
}

void
nsSMILAnimationFunction::UnsetKeyTimes()
{
  mKeyTimes.Clear();
  SetKeyTimesErrorFlag(PR_FALSE);
  mHasChanged = PR_TRUE;
}
