




#include "mozilla/dom/KeyframeEffect.h"
#include "mozilla/dom/KeyframeEffectBinding.h"
#include "mozilla/FloatingPoint.h"
#include "AnimationCommon.h"
#include "nsCSSPropertySet.h"

namespace mozilla {

void
ComputedTimingFunction::Init(const nsTimingFunction &aFunction)
{
  mType = aFunction.mType;
  if (mType == nsTimingFunction::Function) {
    mTimingFunction.Init(aFunction.mFunc.mX1, aFunction.mFunc.mY1,
                         aFunction.mFunc.mX2, aFunction.mFunc.mY2);
  } else {
    mSteps = aFunction.mSteps;
  }
}

static inline double
StepEnd(uint32_t aSteps, double aPortion)
{
  MOZ_ASSERT(0.0 <= aPortion && aPortion <= 1.0, "out of range");
  uint32_t step = uint32_t(aPortion * aSteps); 
  return double(step) / double(aSteps);
}

double
ComputedTimingFunction::GetValue(double aPortion) const
{
  switch (mType) {
    case nsTimingFunction::Function:
      return mTimingFunction.GetSplineValue(aPortion);
    case nsTimingFunction::StepStart:
      
      
      
      
      
      
      return 1.0 - StepEnd(mSteps, 1.0 - aPortion);
    default:
      MOZ_ASSERT(false, "bad type");
      
    case nsTimingFunction::StepEnd:
      return StepEnd(mSteps, aPortion);
  }
}



const double ComputedTiming::kNullTimeFraction = PositiveInfinity<double>();

namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(KeyframeEffectReadonly,
                                   AnimationEffectReadonly,
                                   mTarget)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(KeyframeEffectReadonly,
                                               AnimationEffectReadonly)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(KeyframeEffectReadonly)
NS_INTERFACE_MAP_END_INHERITING(AnimationEffectReadonly)

NS_IMPL_ADDREF_INHERITED(KeyframeEffectReadonly, AnimationEffectReadonly)
NS_IMPL_RELEASE_INHERITED(KeyframeEffectReadonly, AnimationEffectReadonly)

JSObject*
KeyframeEffectReadonly::WrapObject(JSContext* aCx,
                                   JS::Handle<JSObject*> aGivenProto)
{
  return KeyframeEffectReadonlyBinding::Wrap(aCx, this, aGivenProto);
}

void
KeyframeEffectReadonly::SetParentTime(Nullable<TimeDuration> aParentTime)
{
  mParentTime = aParentTime;
}

ComputedTiming
KeyframeEffectReadonly::GetComputedTimingAt(
                          const Nullable<TimeDuration>& aLocalTime,
                          const AnimationTiming& aTiming)
{
  const TimeDuration zeroDuration;

  
  
  
  
  
  MOZ_ASSERT(aTiming.mIterationDuration >= zeroDuration,
             "Expecting iteration duration >= 0");

  
  ComputedTiming result;

  result.mActiveDuration = ActiveDuration(aTiming);

  
  
  if (aLocalTime.IsNull()) {
    return result;
  }
  const TimeDuration& localTime = aLocalTime.Value();

  
  
  
  bool isEndOfFinalIteration = false;

  
  StickyTimeDuration activeTime;
  if (localTime >= aTiming.mDelay + result.mActiveDuration) {
    result.mPhase = ComputedTiming::AnimationPhase_After;
    if (!aTiming.FillsForwards()) {
      
      result.mTimeFraction = ComputedTiming::kNullTimeFraction;
      return result;
    }
    activeTime = result.mActiveDuration;
    
    
    isEndOfFinalIteration =
      aTiming.mIterationCount != 0.0 &&
      aTiming.mIterationCount == floor(aTiming.mIterationCount);
  } else if (localTime < aTiming.mDelay) {
    result.mPhase = ComputedTiming::AnimationPhase_Before;
    if (!aTiming.FillsBackwards()) {
      
      result.mTimeFraction = ComputedTiming::kNullTimeFraction;
      return result;
    }
    
  } else {
    MOZ_ASSERT(result.mActiveDuration != zeroDuration,
               "How can we be in the middle of a zero-duration interval?");
    result.mPhase = ComputedTiming::AnimationPhase_Active;
    activeTime = localTime - aTiming.mDelay;
  }

  
  StickyTimeDuration iterationTime;
  if (aTiming.mIterationDuration != zeroDuration) {
    iterationTime = isEndOfFinalIteration
                    ? StickyTimeDuration(aTiming.mIterationDuration)
                    : activeTime % aTiming.mIterationDuration;
  } 

  
  if (isEndOfFinalIteration) {
    result.mCurrentIteration =
      aTiming.mIterationCount == NS_IEEEPositiveInfinity()
      ? UINT64_MAX 
                   
      : static_cast<uint64_t>(aTiming.mIterationCount) - 1;
  } else if (activeTime == zeroDuration) {
    
    
    
    
    result.mCurrentIteration =
      result.mPhase == ComputedTiming::AnimationPhase_After
      ? static_cast<uint64_t>(aTiming.mIterationCount) 
      : 0;
  } else {
    result.mCurrentIteration =
      static_cast<uint64_t>(activeTime / aTiming.mIterationDuration); 
  }

  
  if (result.mPhase == ComputedTiming::AnimationPhase_Before) {
    result.mTimeFraction = 0.0;
  } else if (result.mPhase == ComputedTiming::AnimationPhase_After) {
    result.mTimeFraction = isEndOfFinalIteration
                         ? 1.0
                         : fmod(aTiming.mIterationCount, 1.0f);
  } else {
    
    MOZ_ASSERT(aTiming.mIterationDuration != zeroDuration,
               "In the active phase of a zero-duration animation?");
    result.mTimeFraction =
      aTiming.mIterationDuration == TimeDuration::Forever()
      ? 0.0
      : iterationTime / aTiming.mIterationDuration;
  }

  bool thisIterationReverse = false;
  switch (aTiming.mDirection) {
    case NS_STYLE_ANIMATION_DIRECTION_NORMAL:
      thisIterationReverse = false;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_REVERSE:
      thisIterationReverse = true;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE:
      thisIterationReverse = (result.mCurrentIteration & 1) == 1;
      break;
    case NS_STYLE_ANIMATION_DIRECTION_ALTERNATE_REVERSE:
      thisIterationReverse = (result.mCurrentIteration & 1) == 0;
      break;
  }
  if (thisIterationReverse) {
    result.mTimeFraction = 1.0 - result.mTimeFraction;
  }

  return result;
}

StickyTimeDuration
KeyframeEffectReadonly::ActiveDuration(const AnimationTiming& aTiming)
{
  if (aTiming.mIterationCount == mozilla::PositiveInfinity<float>()) {
    
    
    
    const StickyTimeDuration zeroDuration;
    return aTiming.mIterationDuration == zeroDuration
           ? zeroDuration
           : StickyTimeDuration::Forever();
  }
  return StickyTimeDuration(
    aTiming.mIterationDuration.MultDouble(aTiming.mIterationCount));
}


bool
KeyframeEffectReadonly::IsInPlay(const Animation& aPlayer) const
{
  if (IsFinishedTransition() ||
      aPlayer.PlayState() == AnimationPlayState::Finished) {
    return false;
  }

  return GetComputedTiming().mPhase == ComputedTiming::AnimationPhase_Active;
}


bool
KeyframeEffectReadonly::IsCurrent(const Animation& aPlayer) const
{
  if (IsFinishedTransition() ||
      aPlayer.PlayState() == AnimationPlayState::Finished) {
    return false;
  }

  ComputedTiming computedTiming = GetComputedTiming();
  return computedTiming.mPhase == ComputedTiming::AnimationPhase_Before ||
         computedTiming.mPhase == ComputedTiming::AnimationPhase_Active;
}

bool
KeyframeEffectReadonly::IsInEffect() const
{
  if (IsFinishedTransition()) {
    return false;
  }

  ComputedTiming computedTiming = GetComputedTiming();
  return computedTiming.mTimeFraction != ComputedTiming::kNullTimeFraction;
}

const AnimationProperty*
KeyframeEffectReadonly::GetAnimationOfProperty(nsCSSProperty aProperty) const
{
  for (size_t propIdx = 0, propEnd = mProperties.Length();
       propIdx != propEnd; ++propIdx) {
    if (aProperty == mProperties[propIdx].mProperty) {
      const AnimationProperty* result = &mProperties[propIdx];
      if (!result->mWinsInCascade) {
        result = nullptr;
      }
      return result;
    }
  }
  return nullptr;
}

bool
KeyframeEffectReadonly::HasAnimationOfProperties(
                          const nsCSSProperty* aProperties,
                          size_t aPropertyCount) const
{
  for (size_t i = 0; i < aPropertyCount; i++) {
    if (HasAnimationOfProperty(aProperties[i])) {
      return true;
    }
  }
  return false;
}

void
KeyframeEffectReadonly::ComposeStyle(
                          nsRefPtr<css::AnimValuesStyleRule>& aStyleRule,
                          nsCSSPropertySet& aSetProperties)
{
  ComputedTiming computedTiming = GetComputedTiming();

  
  
  if (computedTiming.mTimeFraction == ComputedTiming::kNullTimeFraction) {
    return;
  }

  MOZ_ASSERT(0.0 <= computedTiming.mTimeFraction &&
             computedTiming.mTimeFraction <= 1.0,
             "timing fraction should be in [0-1]");

  for (size_t propIdx = 0, propEnd = mProperties.Length();
       propIdx != propEnd; ++propIdx)
  {
    const AnimationProperty& prop = mProperties[propIdx];

    MOZ_ASSERT(prop.mSegments[0].mFromKey == 0.0, "incorrect first from key");
    MOZ_ASSERT(prop.mSegments[prop.mSegments.Length() - 1].mToKey == 1.0,
               "incorrect last to key");

    if (aSetProperties.HasProperty(prop.mProperty)) {
      
      
      
      
      continue;
    }

    if (!prop.mWinsInCascade) {
      
      
      
      
      
      
      continue;
    }

    aSetProperties.AddProperty(prop.mProperty);

    MOZ_ASSERT(prop.mSegments.Length() > 0,
               "property should not be in animations if it has no segments");

    
    const AnimationPropertySegment *segment = prop.mSegments.Elements(),
                                *segmentEnd = segment + prop.mSegments.Length();
    while (segment->mToKey < computedTiming.mTimeFraction) {
      MOZ_ASSERT(segment->mFromKey < segment->mToKey, "incorrect keys");
      ++segment;
      if (segment == segmentEnd) {
        MOZ_ASSERT_UNREACHABLE("incorrect time fraction");
        break; 
      }
      MOZ_ASSERT(segment->mFromKey == (segment-1)->mToKey, "incorrect keys");
    }
    if (segment == segmentEnd) {
      continue;
    }
    MOZ_ASSERT(segment->mFromKey < segment->mToKey, "incorrect keys");
    MOZ_ASSERT(segment >= prop.mSegments.Elements() &&
               size_t(segment - prop.mSegments.Elements()) <
                 prop.mSegments.Length(),
               "out of array bounds");

    if (!aStyleRule) {
      
      aStyleRule = new css::AnimValuesStyleRule();
    }

    double positionInSegment =
      (computedTiming.mTimeFraction - segment->mFromKey) /
      (segment->mToKey - segment->mFromKey);
    double valuePosition =
      segment->mTimingFunction.GetValue(positionInSegment);

    StyleAnimationValue *val = aStyleRule->AddEmptyValue(prop.mProperty);

#ifdef DEBUG
    bool result =
#endif
      StyleAnimationValue::Interpolate(prop.mProperty,
                                       segment->mFromValue,
                                       segment->mToValue,
                                       valuePosition, *val);
    MOZ_ASSERT(result, "interpolate must succeed now");
  }
}

} 
} 
