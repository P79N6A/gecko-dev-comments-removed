






































#include "nsSMILAnimationFunction.h"
#include "nsISMILAttr.h"
#include "nsSMILParserUtils.h"
#include "nsSMILNullType.h"
#include "nsISMILAnimationElement.h"
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


#define BF_ACCUMULATE  0
#define BF_ADDITIVE    1
#define BF_CALC_MODE   2
#define BF_KEY_TIMES   3
#define BF_KEY_SPLINES 4



#define COMPUTE_DISTANCE_ERROR (-1)


#define GET_FLAG(bitfield, field) (((bitfield) & (0x01 << (field))) \
                                     ? PR_TRUE : PR_FALSE)
#define SET_FLAG(bitfield, field, b) ((b) \
                                     ? ((bitfield) |=  (0x01 << (field))) \
                                     : ((bitfield) &= ~(0x01 << (field))))




nsSMILAnimationFunction::nsSMILAnimationFunction()
  : mIsActive(PR_FALSE),
    mIsFrozen(PR_FALSE),
    mSampleTime(-1),
    mRepeatIteration(0),
    mLastValue(PR_FALSE),
    mHasChanged(PR_TRUE),
    mBeginTime(LL_MININT),
    mAnimationElement(nsnull),
    mErrorFlags(0)
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
  if (mHasChanged || mLastValue || mSampleTime != aSampleTime ||
      mSimpleDuration.CompareTo(aSimpleDuration) ||
      mRepeatIteration != aRepeatIteration) {
    mHasChanged = PR_TRUE;
  }

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

  
  if (!IsActiveOrFrozen() || mErrorFlags != 0)
    return;

  
  nsSMILValueArray values;
  nsresult rv = GetValues(aSMILAttr, values);
  if (NS_FAILED(rv))
    return;

  
  if (mErrorFlags != 0)
    return;

  
  
  
  if (mIsActive) {
    NS_ENSURE_TRUE(mSampleTime >= 0,);
    NS_ENSURE_TRUE(mSimpleDuration.IsResolved() ||
                   mSimpleDuration.IsIndefinite(),);
  }

  nsSMILValue result(aResult.mType);

  if (mSimpleDuration.IsIndefinite() ||
      (HasAttr(nsGkAtoms::values) && values.Length() == 1)) {

    
    result = values[0];

  } else if (mLastValue) {

    
    nsSMILValue last(values[values.Length() - 1]);
    result = last;

    
    if (!IsToAnimation() && GetAccumulate() && mRepeatIteration) {
      
      
      result.Add(last, mRepeatIteration);
    }

  } else if (!mFrozenValue.IsNull() && !mHasChanged) {

    
    result = mFrozenValue;

  } else {

    
    NS_ENSURE_SUCCESS(InterpolateResult(values, result, aResult),);
    NS_ENSURE_SUCCESS(AccumulateResult(values, result),);

    if (IsToAnimation() && mIsFrozen) {
      mFrozenValue = result;
    }
  }

  
  if (!IsAdditive() || NS_FAILED(aResult.SandwichAdd(result))) {
    aResult = result;
  }
}

PRInt8
nsSMILAnimationFunction::CompareTo(const nsSMILAnimationFunction* aOther) const
{
  NS_ENSURE_TRUE(aOther, 0);

  NS_ASSERTION(aOther != this, "Trying to compare to self.");

  
  if (!IsActiveOrFrozen() && aOther->IsActiveOrFrozen())
    return -1;

  if (IsActiveOrFrozen() && !aOther->IsActiveOrFrozen())
    return 1;

  
  if (mBeginTime != aOther->GetBeginTime())
    return mBeginTime > aOther->GetBeginTime() ? 1 : -1;

  
  

  
  
  nsIContent &thisElement = mAnimationElement->Content();
  nsIContent &otherElement = aOther->mAnimationElement->Content();

  NS_ASSERTION(&thisElement != &otherElement,
             "Two animations cannot have the same animation content element!");

  return (nsContentUtils::PositionIsBefore(&thisElement, &otherElement))
          ? -1 : 1;
}

PRBool
nsSMILAnimationFunction::WillReplace() const
{
  





  return !(IsAdditive() || IsToAnimation()) ||
    (IsToAnimation() && mIsFrozen && !mHasChanged);
}

PRBool
nsSMILAnimationFunction::HasChanged() const
{
  return mHasChanged;
}




nsresult
nsSMILAnimationFunction::InterpolateResult(const nsSMILValueArray& aValues,
                                           nsSMILValue& aResult,
                                           nsSMILValue& aBaseValue)
{
  nsresult rv = NS_OK;
  const nsSMILTime& dur = mSimpleDuration.GetMillis();

  
  NS_ABORT_IF_FALSE(mSampleTime >= 0.0f, "Sample time should not be negative");
  NS_ABORT_IF_FALSE(dur >= 0.0f, "Simple duration should not be negative");

  if (mSampleTime >= dur || mSampleTime < 0.0f) {
    NS_ERROR("Animation sampled outside interval.");
    return NS_ERROR_FAILURE;
  }

  if ((!IsToAnimation() && aValues.Length() < 2) ||
      (IsToAnimation()  && aValues.Length() != 1)) {
    NS_ERROR("Unexpected number of values.");
    return NS_ERROR_FAILURE;
  }
  

  double fTime = double(mSampleTime);
  double fDur = double(dur);

  
  double simpleProgress = (fDur > 0.0) ? fTime / fDur : 0.0;

  
  
  if (HasAttr(nsGkAtoms::keyTimes)) {
    double first = mKeyTimes[0];
    if (first > 0.0 && simpleProgress < first) {
      if (!IsToAnimation())
        aResult = aValues[0];
      return rv;
    }
    double last = mKeyTimes[mKeyTimes.Length() - 1];
    if (last < 1.0 && simpleProgress >= last) {
      if (IsToAnimation())
        aResult = aValues[0];
      else
        aResult = aValues[aValues.Length() - 1];
      return rv;
    }
  }

  ScaleSimpleProgress(simpleProgress);

  if (GetCalcMode() != CALC_DISCRETE) {
    
    const nsSMILValue* from = nsnull;
    const nsSMILValue* to = nsnull;
    double intervalProgress;
    if (IsToAnimation()) {
      
      
      from = &aBaseValue;
      to = &aValues[0];
      intervalProgress = simpleProgress;
      ScaleIntervalProgress(intervalProgress, 0, 1);
    } else {
      if (GetCalcMode() == CALC_PACED) {
        rv = ComputePacedPosition(aValues, simpleProgress,
                                  intervalProgress, from, to);
        
        
        
        
      } else { 
        PRUint32 index = (PRUint32)floor(simpleProgress *
                                         (aValues.Length() - 1));
        from = &aValues[index];
        to = &aValues[index + 1];
        intervalProgress = simpleProgress * (aValues.Length() - 1) - index;
        ScaleIntervalProgress(intervalProgress, index, aValues.Length() - 1);
      }
    }
    if (NS_SUCCEEDED(rv)) {
      NS_ABORT_IF_FALSE(from, "NULL from-value during interpolation.");
      NS_ABORT_IF_FALSE(to, "NULL to-value during interpolation.");
      NS_ABORT_IF_FALSE(0.0f <= intervalProgress && intervalProgress < 1.0f,
                      "Interval progress should be in the range [0, 1)");
      rv = from->Interpolate(*to, intervalProgress, aResult);
    }
  }

  
  
  
  if (GetCalcMode() == CALC_DISCRETE || NS_FAILED(rv)) {
    if (IsToAnimation()) {
      
      aResult = (simpleProgress < 0.5f) ? aBaseValue : aValues[0];
    } else {
      PRUint32 index = (PRUint32) floor(simpleProgress * (aValues.Length()));
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
    nsSMILValue lastValue = aValues[aValues.Length() - 1];

    
    
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
               "aSimpleProgress is out of bounds.");
  NS_ASSERTION(GetCalcMode() == CALC_PACED,
               "Calling paced-specific function, but not in paced mode");

  double totalDistance = ComputePacedTotalDistance(aValues);
  if (totalDistance == COMPUTE_DISTANCE_ERROR)
    return NS_ERROR_FAILURE;

  
  
  double remainingDist = aSimpleProgress * totalDistance;

  
  
  NS_ASSERTION(remainingDist >= 0, "distance values must be non-negative");

  
  
  
  for (PRUint32 i = 0; i < aValues.Length() - 1; i++) {
    
    
    
    NS_ASSERTION(remainingDist >= 0, "distance values must be non-negative");

    double curIntervalDist;
    nsresult rv = aValues[i].ComputeDistance(aValues[i+1], curIntervalDist);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv),
                      "If we got through ComputePacedTotalDistance, we should "
                      "be able to recompute each sub-distance without errors");

    NS_ASSERTION(curIntervalDist >= 0, "distance values must be non-negative");
    
    curIntervalDist = PR_MAX(curIntervalDist, 0.0f);

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
                "then aSimpleProgress was probably out of bounds.");
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
    tmpDist = PR_MAX(tmpDist, 0.0f);

    totalDistance += tmpDist;
  }

  return totalDistance;
}




void
nsSMILAnimationFunction::ScaleSimpleProgress(double& aProgress)
{
  if (!HasAttr(nsGkAtoms::keyTimes))
    return;

  PRUint32 numTimes = mKeyTimes.Length();

  if (numTimes < 2)
    return;

  PRUint32 i = 0;
  for (; i < numTimes - 2 && aProgress >= mKeyTimes[i+1]; ++i);

  double& intervalStart = mKeyTimes[i];
  double& intervalEnd   = mKeyTimes[i+1];

  double intervalLength = intervalEnd - intervalStart;
  if (intervalLength <= 0.0) {
    aProgress = intervalStart;
    return;
  }

  aProgress = (i + (aProgress - intervalStart) / intervalLength) *
         1.0 / double(numTimes - 1);
}





void
nsSMILAnimationFunction::ScaleIntervalProgress(double& aProgress,
                                               PRUint32   aIntervalIndex,
                                               PRUint32   aNumIntervals)
{
  if (GetCalcMode() != CALC_SPLINE)
    return;

  if (!HasAttr(nsGkAtoms::keySplines))
    return;

  NS_ASSERTION(aIntervalIndex < (PRUint32)mKeySplines.Length(),
               "Invalid interval index.");
  NS_ASSERTION(aNumIntervals >= 1, "Invalid number of intervals.");

  if (aIntervalIndex >= (PRUint32)mKeySplines.Length() ||
      aNumIntervals < 1)
    return;

  nsSMILKeySpline const &spline = mKeySplines[aIntervalIndex];
  aProgress = spline.GetSplineValue(aProgress);
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
                                   nsSMILValue& aResult) const
{
  nsAutoString attValue;
  if (GetAttr(aAttName, attValue)) {
    nsresult rv =
      aSMILAttr.ValueFromString(attValue, mAnimationElement, aResult);
    if (NS_FAILED(rv))
      return PR_FALSE;
  }
  return PR_TRUE;
}















nsresult
nsSMILAnimationFunction::GetValues(const nsISMILAttr& aSMILAttr,
                                   nsSMILValueArray& aResult)
{
  if (!mAnimationElement)
    return NS_ERROR_FAILURE;

  nsSMILValueArray result;

  
  if (HasAttr(nsGkAtoms::values)) {
    nsAutoString attValue;
    GetAttr(nsGkAtoms::values, attValue);
    nsresult rv = nsSMILParserUtils::ParseValues(attValue, mAnimationElement,
                                                 aSMILAttr, result);
    if (NS_FAILED(rv))
      return rv;

  
  } else {

    PRBool parseOk = PR_TRUE;
    nsSMILValue to, from, by;
    parseOk &= ParseAttr(nsGkAtoms::to,   aSMILAttr, to);
    parseOk &= ParseAttr(nsGkAtoms::from, aSMILAttr, from);
    parseOk &= ParseAttr(nsGkAtoms::by,   aSMILAttr, by);

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

  
  CheckKeyTimes(result.Length());
  CheckKeySplines(result.Length());

  result.SwapElements(aResult);

  return NS_OK;
}






void
nsSMILAnimationFunction::CheckKeyTimes(PRUint32 aNumValues)
{
  if (!HasAttr(nsGkAtoms::keyTimes))
    return;

  
  if (GetCalcMode() == CALC_PACED) {
    SET_FLAG(mErrorFlags, BF_KEY_TIMES, PR_FALSE);
    return;
  }

  if (mKeyTimes.Length() < 1) {
    
    SET_FLAG(mErrorFlags, BF_KEY_TIMES, PR_TRUE);
    return;
  }

  
  if ((mKeyTimes.Length() != aNumValues && !IsToAnimation()) ||
      (IsToAnimation() && mKeyTimes.Length() != 2)) {
    SET_FLAG(mErrorFlags, BF_KEY_TIMES, PR_TRUE);
    return;
  }

  
  
  if (mKeyTimes.Length() == 1) {
    double time = mKeyTimes[0];
    SET_FLAG(mErrorFlags, BF_KEY_TIMES, !(time == 0.0 || time == 1.0));
    return;
  }

  
  
  
  

  SET_FLAG(mErrorFlags, BF_KEY_TIMES, PR_FALSE);
}

void
nsSMILAnimationFunction::CheckKeySplines(PRUint32 aNumValues)
{
  
  if (GetCalcMode() != CALC_SPLINE) {
    SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_FALSE);
    return;
  }

  
  if (!HasAttr(nsGkAtoms::keySplines)) {
    SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_FALSE);
    return;
  }

  if (mKeySplines.Length() < 1) {
    
    SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_TRUE);
    return;
  }

  
  if (aNumValues == 1 && !IsToAnimation()) {
    SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_FALSE);
    return;
  }

  
  PRUint32 splineSpecs = mKeySplines.Length();
  if ((splineSpecs != aNumValues - 1 && !IsToAnimation()) ||
      (IsToAnimation() && splineSpecs != 1)) {
    SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_TRUE);
    return;
  }

  SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_FALSE);
}




PRBool
nsSMILAnimationFunction::GetAccumulate() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::accumulate);
  if (!value)
    return PR_FALSE;

  return (value->GetEnumValue() == PR_TRUE);
}

PRBool
nsSMILAnimationFunction::GetAdditive() const
{
  const nsAttrValue* value = GetAttr(nsGkAtoms::additive);
  if (!value)
    return PR_FALSE;

  return (value->GetEnumValue() == PR_TRUE);
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
  SET_FLAG(mErrorFlags, BF_ACCUMULATE, !parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAccumulate()
{
  SET_FLAG(mErrorFlags, BF_ACCUMULATE, PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetAdditive(const nsAString& aAdditive,
                                     nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;
  PRBool parseResult
    = aResult.ParseEnumValue(aAdditive, sAdditiveTable, PR_TRUE);
  SET_FLAG(mErrorFlags, BF_ADDITIVE, !parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetAdditive()
{
  SET_FLAG(mErrorFlags, BF_ADDITIVE, PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetCalcMode(const nsAString& aCalcMode,
                                     nsAttrValue& aResult)
{
  mHasChanged = PR_TRUE;
  PRBool parseResult
    = aResult.ParseEnumValue(aCalcMode, sCalcModeTable, PR_TRUE);
  SET_FLAG(mErrorFlags, BF_CALC_MODE, !parseResult);
  return parseResult ? NS_OK : NS_ERROR_FAILURE;
}

void
nsSMILAnimationFunction::UnsetCalcMode()
{
  SET_FLAG(mErrorFlags, BF_CALC_MODE, PR_FALSE);
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
  SET_FLAG(mErrorFlags, BF_KEY_SPLINES, PR_FALSE);
  mHasChanged = PR_TRUE;
}

nsresult
nsSMILAnimationFunction::SetKeyTimes(const nsAString& aKeyTimes,
                                     nsAttrValue& aResult)
{
  mKeyTimes.Clear();
  aResult.SetTo(aKeyTimes);

  nsresult rv = nsSMILParserUtils::ParseKeyTimes(aKeyTimes, mKeyTimes);

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
  SET_FLAG(mErrorFlags, BF_KEY_TIMES, PR_FALSE);
  mHasChanged = PR_TRUE;
}
