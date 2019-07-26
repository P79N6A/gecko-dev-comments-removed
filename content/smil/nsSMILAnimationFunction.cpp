




#include "mozilla/dom/SVGAnimationElement.h"
#include "nsSMILAnimationFunction.h"
#include "nsISMILAttr.h"
#include "nsSMILParserUtils.h"
#include "nsSMILNullType.h"
#include "nsSMILTimedElement.h"
#include "nsAttrValueInlines.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include <math.h>
#include <algorithm>

using namespace mozilla::dom;




nsAttrValue::EnumTable nsSMILAnimationFunction::sAccumulateTable[] = {
      {"none", false},
      {"sum", true},
      {nullptr, 0}
};

nsAttrValue::EnumTable nsSMILAnimationFunction::sAdditiveTable[] = {
      {"replace", false},
      {"sum", true},
      {nullptr, 0}
};

nsAttrValue::EnumTable nsSMILAnimationFunction::sCalcModeTable[] = {
      {"linear", CALC_LINEAR},
      {"discrete", CALC_DISCRETE},
      {"paced", CALC_PACED},
      {"spline", CALC_SPLINE},
      {nullptr, 0}
};



#define COMPUTE_DISTANCE_ERROR (-1)




nsSMILAnimationFunction::nsSMILAnimationFunction()
  : mSampleTime(-1),
    mRepeatIteration(0),
    mBeginTime(INT64_MIN),
    mAnimationElement(nullptr),
    mErrorFlags(0),
    mIsActive(false),
    mIsFrozen(false),
    mLastValue(false),
    mHasChanged(true),
    mValueNeedsReparsingEverySample(false),
    mPrevSampleWasSingleValueAnimation(false),
    mWasSkippedInPrevSample(false)
{
}

void
nsSMILAnimationFunction::SetAnimationElement(
    SVGAnimationElement* aAnimationElement)
{
  mAnimationElement = aAnimationElement;
}

bool
nsSMILAnimationFunction::SetAttr(nsIAtom* aAttribute, const nsAString& aValue,
                                 nsAttrValue& aResult, nsresult* aParseResult)
{
  bool foundMatch = true;
  nsresult parseResult = NS_OK;

  
  
  
  if (aAttribute == nsGkAtoms::by ||
      aAttribute == nsGkAtoms::from ||
      aAttribute == nsGkAtoms::to ||
      aAttribute == nsGkAtoms::values) {
    
    
    
    mHasChanged = true;
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
    foundMatch = false;
  }

  if (foundMatch && aParseResult) {
    *aParseResult = parseResult;
  }

  return foundMatch;
}

bool
nsSMILAnimationFunction::UnsetAttr(nsIAtom* aAttribute)
{
  bool foundMatch = true;

  if (aAttribute == nsGkAtoms::by ||
      aAttribute == nsGkAtoms::from ||
      aAttribute == nsGkAtoms::to ||
      aAttribute == nsGkAtoms::values) {
    mHasChanged = true;
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
    foundMatch = false;
  }

  return foundMatch;
}

void
nsSMILAnimationFunction::SampleAt(nsSMILTime aSampleTime,
                                  const nsSMILTimeValue& aSimpleDuration,
                                  uint32_t aRepeatIteration)
{
  
  
  mHasChanged |= mLastValue;

  
  mHasChanged |=
    (mSampleTime != aSampleTime || mSimpleDuration != aSimpleDuration) &&
    !IsValueFixedForSimpleDuration();

  
  if (!mErrorFlags) { 
    mHasChanged |= (mRepeatIteration != aRepeatIteration) && GetAccumulate();
  }

  mSampleTime       = aSampleTime;
  mSimpleDuration   = aSimpleDuration;
  mRepeatIteration  = aRepeatIteration;
  mLastValue        = false;
}

void
nsSMILAnimationFunction::SampleLastValue(uint32_t aRepeatIteration)
{
  if (mHasChanged || !mLastValue || mRepeatIteration != aRepeatIteration) {
    mHasChanged = true;
  }

  mRepeatIteration  = aRepeatIteration;
  mLastValue        = true;
}

void
nsSMILAnimationFunction::Activate(nsSMILTime aBeginTime)
{
  mBeginTime = aBeginTime;
  mIsActive = true;
  mIsFrozen = false;
  mHasChanged = true;
}

void
nsSMILAnimationFunction::Inactivate(bool aIsFrozen)
{
  mIsActive = false;
  mIsFrozen = aIsFrozen;
  mHasChanged = true;
}

void
nsSMILAnimationFunction::ComposeResult(const nsISMILAttr& aSMILAttr,
                                       nsSMILValue& aResult)
{
  mHasChanged = false;
  mPrevSampleWasSingleValueAnimation = false;
  mWasSkippedInPrevSample = false;

  
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
  NS_ABORT_IF_FALSE(mSimpleDuration.IsResolved() || mLastValue,
      "Unresolved simple duration for active or frozen animation");

  
  
  
  
  bool isAdditive = IsAdditive();
  if (isAdditive && aResult.IsNull())
    return;

  nsSMILValue result;

  if (values.Length() == 1 && !IsToAnimation()) {

    
    result = values[0];
    mPrevSampleWasSingleValueAnimation = true;

  } else if (mLastValue) {

    
    const nsSMILValue& last = values[values.Length() - 1];
    result = last;

    
    if (!IsToAnimation() && GetAccumulate() && mRepeatIteration) {
      
      
      result.Add(last, mRepeatIteration);
    }

  } else {

    
    if (NS_FAILED(InterpolateResult(values, result, aResult)))
      return;

    if (NS_FAILED(AccumulateResult(values, result)))
      return;
  }

  
  if (!isAdditive || NS_FAILED(aResult.SandwichAdd(result))) {
    aResult.Swap(result);
    
    
  }
}

int8_t
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

  
  
  NS_ABORT_IF_FALSE(mAnimationElement != aOther->mAnimationElement,
      "Two animations cannot have the same animation content element!");

  return (nsContentUtils::PositionIsBefore(mAnimationElement, aOther->mAnimationElement))
          ? -1 : 1;
}

bool
nsSMILAnimationFunction::WillReplace() const
{
  





  return !mErrorFlags && !(IsAdditive() || IsToAnimation());
}

bool
nsSMILAnimationFunction::HasChanged() const
{
  return mHasChanged || mValueNeedsReparsingEverySample;
}

bool
nsSMILAnimationFunction::UpdateCachedTarget(
  const nsSMILTargetIdentifier& aNewTarget)
{
  if (!mLastTarget.Equals(aNewTarget)) {
    mLastTarget = aNewTarget;
    return true;
  }
  return false;
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

  if (mSimpleDuration.IsDefinite()) {
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
    
    const nsSMILValue* from = nullptr;
    const nsSMILValue* to = nullptr;
    
    
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
      uint32_t index = (uint32_t)floor(scaledSimpleProgress *
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

    
    
    
    
    
    
    
    
    
    static const double kFloatingPointFudgeFactor = 1.0e-16;
    if (scaledSimpleProgress + kFloatingPointFudgeFactor <= 1.0) {
      scaledSimpleProgress += kFloatingPointFudgeFactor;
    }

    if (IsToAnimation()) {
      
      
      
      
      uint32_t index = (uint32_t)floor(scaledSimpleProgress * 2);
      aResult = index == 0 ? aBaseValue : aValues[0];
    } else {
      uint32_t index = (uint32_t)floor(scaledSimpleProgress * aValues.Length());
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
  if (!IsToAnimation() && GetAccumulate() && mRepeatIteration) {
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

  
  
  
  if (totalDistance == 0.0) {
    return NS_ERROR_FAILURE;
  }

  
  
  double remainingDist = aSimpleProgress * totalDistance;

  
  
  NS_ASSERTION(remainingDist >= 0, "distance values must be non-negative");

  
  
  
  for (uint32_t i = 0; i < aValues.Length() - 1; i++) {
    
    
    
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
    
    curIntervalDist = std::max(curIntervalDist, 0.0);

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
  for (uint32_t i = 0; i < aValues.Length() - 1; i++) {
    double tmpDist;
    nsresult rv = aValues[i].ComputeDistance(aValues[i+1], tmpDist);
    if (NS_FAILED(rv)) {
      return COMPUTE_DISTANCE_ERROR;
    }

    
    
    NS_ABORT_IF_FALSE(tmpDist >= 0.0f, "distance values must be non-negative");
    tmpDist = std::max(tmpDist, 0.0);

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

  uint32_t numTimes = mKeyTimes.Length();

  if (numTimes < 2)
    return aProgress;

  uint32_t i = 0;
  for (; i < numTimes - 2 && aProgress >= mKeyTimes[i+1]; ++i) { }

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
                                               uint32_t aIntervalIndex)
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

bool
nsSMILAnimationFunction::HasAttr(nsIAtom* aAttName) const
{
  return mAnimationElement->HasAnimAttr(aAttName);
}

const nsAttrValue*
nsSMILAnimationFunction::GetAttr(nsIAtom* aAttName) const
{
  return mAnimationElement->GetAnimAttr(aAttName);
}

bool
nsSMILAnimationFunction::GetAttr(nsIAtom* aAttName, nsAString& aResult) const
{
  return mAnimationElement->GetAnimAttr(aAttName, aResult);
}

















bool
nsSMILAnimationFunction::ParseAttr(nsIAtom* aAttName,
                                   const nsISMILAttr& aSMILAttr,
                                   nsSMILValue& aResult,
                                   bool& aPreventCachingOfSandwich) const
{
  nsAutoString attValue;
  if (GetAttr(aAttName, attValue)) {
    bool preventCachingOfSandwich = false;
    nsresult rv = aSMILAttr.ValueFromString(attValue, mAnimationElement,
                                            aResult, preventCachingOfSandwich);
    if (NS_FAILED(rv))
      return false;

    if (preventCachingOfSandwich) {
      aPreventCachingOfSandwich = true;
    }
  }
  return true;
}















nsresult
nsSMILAnimationFunction::GetValues(const nsISMILAttr& aSMILAttr,
                                   nsSMILValueArray& aResult)
{
  if (!mAnimationElement)
    return NS_ERROR_FAILURE;

  mValueNeedsReparsingEverySample = false;
  nsSMILValueArray result;

  
  if (HasAttr(nsGkAtoms::values)) {
    nsAutoString attValue;
    GetAttr(nsGkAtoms::values, attValue);
    bool preventCachingOfSandwich = false;
    nsresult rv = nsSMILParserUtils::ParseValues(attValue, mAnimationElement,
                                                 aSMILAttr, result,
                                                 preventCachingOfSandwich);
    if (NS_FAILED(rv))
      return rv;

    if (preventCachingOfSandwich) {
      mValueNeedsReparsingEverySample = true;
    }
  
  } else {
    bool preventCachingOfSandwich = false;
    bool parseOk = true;
    nsSMILValue to, from, by;
    parseOk &= ParseAttr(nsGkAtoms::to,   aSMILAttr, to,
                         preventCachingOfSandwich);
    parseOk &= ParseAttr(nsGkAtoms::from, aSMILAttr, from,
                         preventCachingOfSandwich);
    parseOk &= ParseAttr(nsGkAtoms::by,   aSMILAttr, by,
                         preventCachingOfSandwich);

    if (preventCachingOfSandwich) {
      mValueNeedsReparsingEverySample = true;
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
nsSMILAnimationFunction::CheckValueListDependentAttrs(uint32_t aNumValues)
{
  CheckKeyTimes(aNumValues);
  CheckKeySplines(aNumValues);
}






void
nsSMILAnimationFunction::CheckKeyTimes(uint32_t aNumValues)
{
  if (!HasAttr(nsGkAtoms::keyTimes))
    return;

  nsSMILCalcMode calcMode = GetCalcMode();

  
  if (calcMode == CALC_PACED) {
    SetKeyTimesErrorFlag(false);
    return;
  }

  uint32_t numKeyTimes = mKeyTimes.Length();
  if (numKeyTimes < 1) {
    
    SetKeyTimesErrorFlag(true);
    return;
  }

  
  
  bool matchingNumOfValues =
    numKeyTimes == (IsToAnimation() ? 2 : aNumValues);
  if (!matchingNumOfValues) {
    SetKeyTimesErrorFlag(true);
    return;
  }

  
  if (mKeyTimes[0] != 0.0) {
    SetKeyTimesErrorFlag(true);
    return;
  }

  
  if (calcMode != CALC_DISCRETE && numKeyTimes > 1 &&
      mKeyTimes[numKeyTimes - 1] != 1.0) {
    SetKeyTimesErrorFlag(true);
    return;
  }

  SetKeyTimesErrorFlag(false);
}

void
nsSMILAnimationFunction::CheckKeySplines(uint32_t aNumValues)
{
  
  if (GetCalcMode() != CALC_SPLINE) {
    SetKeySplinesErrorFlag(false);
    return;
  }

  
  if (!HasAttr(nsGkAtoms::keySplines)) {
    SetKeySplinesErrorFlag(false);
    return;
  }

  if (mKeySplines.Length() < 1) {
    
    SetKeySplinesErrorFlag(true);
    return;
  }

  
  if (aNumValues == 1 && !IsToAnimation()) {
    SetKeySplinesErrorFlag(false);
    return;
  }

  
  uint32_t splineSpecs = mKeySplines.Length();
  if ((splineSpecs != aNumValues - 1 && !IsToAnimation()) ||
      (IsToAnimation() && splineSpecs != 1)) {
    SetKeySplinesErrorFlag(true);
    return;
  }

  SetKeySplinesErrorFlag(false);
}

bool
nsSMILAnimationFunction::IsValueFixedForSimpleDuration() const
{
  return mSimpleDuration.IsIndefinite() ||
    (!mHasChanged && mPrevSampleWasSingleValueAnimation);
}




bool
nsSMILAnimationFunction::GetAccumulate() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::accumulate);
  if (!value)
    return false;

  return value->GetEnumValue();
}

bool
nsSMILAnimationFunction::GetAdditive() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::additive);
  if (!value)
    return false;

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
  mHasChanged = true;
  bool parseResult =
    aResult.ParseEnumValue(aAccumulate, sAccumulateTable, true);
  SetAccumulateErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAccumulate()
{
  SetAccumulateErrorFlag(false);
  mHasChanged = true;
}

nsresult
nsSMILAnimationFunction::SetAdditive(const nsAString& aAdditive,
                                     nsAttrValue& aResult)
{
  mHasChanged = true;
  bool parseResult
    = aResult.ParseEnumValue(aAdditive, sAdditiveTable, true);
  SetAdditiveErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAdditive()
{
  SetAdditiveErrorFlag(false);
  mHasChanged = true;
}

nsresult
nsSMILAnimationFunction::SetCalcMode(const nsAString& aCalcMode,
                                     nsAttrValue& aResult)
{
  mHasChanged = true;
  bool parseResult
    = aResult.ParseEnumValue(aCalcMode, sCalcModeTable, true);
  SetCalcModeErrorFlag(!parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetCalcMode()
{
  SetCalcModeErrorFlag(false);
  mHasChanged = true;
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
    for (uint32_t i = 0; i < keySplines.Length() && NS_SUCCEEDED(rv); i += 4)
    {
      if (!mKeySplines.AppendElement(nsSMILKeySpline(keySplines[i],
                                                     keySplines[i+1],
                                                     keySplines[i+2],
                                                     keySplines[i+3]))) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  mHasChanged = true;

  return rv;
}

void
nsSMILAnimationFunction::UnsetKeySplines()
{
  mKeySplines.Clear();
  SetKeySplinesErrorFlag(false);
  mHasChanged = true;
}

nsresult
nsSMILAnimationFunction::SetKeyTimes(const nsAString& aKeyTimes,
                                     nsAttrValue& aResult)
{
  mKeyTimes.Clear();
  aResult.SetTo(aKeyTimes);

  nsresult rv =
    nsSMILParserUtils::ParseSemicolonDelimitedProgressList(aKeyTimes, true,
                                                           mKeyTimes);

  if (NS_SUCCEEDED(rv) && mKeyTimes.Length() < 1)
    rv = NS_ERROR_FAILURE;

  if (NS_FAILED(rv))
    mKeyTimes.Clear();

  mHasChanged = true;

  return NS_OK;
}

void
nsSMILAnimationFunction::UnsetKeyTimes()
{
  mKeyTimes.Clear();
  SetKeyTimesErrorFlag(false);
  mHasChanged = true;
}
