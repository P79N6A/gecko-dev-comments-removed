





#ifndef AudioEventTimeline_h_
#define AudioEventTimeline_h_

#include <algorithm>
#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/PodOperations.h"

#include "nsTArray.h"
#include "math.h"
#include "WebAudioUtils.h"

namespace mozilla {

namespace dom {


struct AudioTimelineEvent {
  enum Type MOZ_ENUM_TYPE(uint32_t) {
    SetValue,
    LinearRamp,
    ExponentialRamp,
    SetTarget,
    SetValueCurve
  };

  AudioTimelineEvent(Type aType, double aTime, float aValue, double aTimeConstant = 0.0,
                     float aDuration = 0.0, const float* aCurve = nullptr, uint32_t aCurveLength = 0)
    : mType(aType)
    , mTimeConstant(aTimeConstant)
    , mDuration(aDuration)
#ifdef DEBUG
    , mTimeIsInTicks(false)
#endif
  {
    mTime = aTime;
    if (aType == AudioTimelineEvent::SetValueCurve) {
      SetCurveParams(aCurve, aCurveLength);
    } else {
      mValue = aValue;
    }
  }

  AudioTimelineEvent(const AudioTimelineEvent& rhs)
  {
    PodCopy(this, &rhs, 1);
    if (rhs.mType == AudioTimelineEvent::SetValueCurve) {
      SetCurveParams(rhs.mCurve, rhs.mCurveLength);
    }
  }

  ~AudioTimelineEvent()
  {
    if (mType == AudioTimelineEvent::SetValueCurve) {
      delete[] mCurve;
    }
  }

  bool IsValid() const
  {
    if (mType == AudioTimelineEvent::SetValueCurve) {
      if (!mCurve || !mCurveLength) {
        return false;
      }
      for (uint32_t i = 0; i < mCurveLength; ++i) {
        if (!IsValid(mCurve[i])) {
          return false;
        }
      }
    }

    if (mType == AudioTimelineEvent::SetTarget &&
        WebAudioUtils::FuzzyEqual(mTimeConstant, 0.0)) {
      return false;
    }

    return IsValid(mTime) &&
           IsValid(mValue) &&
           IsValid(mTimeConstant) &&
           IsValid(mDuration);
  }

  template <class TimeType>
  TimeType Time() const;

  void SetTimeInTicks(int64_t aTimeInTicks)
  {
    mTimeInTicks = aTimeInTicks;
#ifdef DEBUG
    mTimeIsInTicks = true;
#endif
  }

  void SetCurveParams(const float* aCurve, uint32_t aCurveLength) {
    mCurveLength = aCurveLength;
    if (aCurveLength) {
      mCurve = new float[aCurveLength];
      PodCopy(mCurve, aCurve, aCurveLength);
    } else {
      mCurve = nullptr;
    }
  }

  Type mType;
  union {
    float mValue;
    uint32_t mCurveLength;
  };
  
  
  
  
  
  union {
    double mTime;
    int64_t mTimeInTicks;
  };
  
  
  
  
  
  float* mCurve;
  double mTimeConstant;
  double mDuration;
#ifdef DEBUG
  bool mTimeIsInTicks;
#endif

private:
  static bool IsValid(double value)
  {
    return mozilla::IsFinite(value);
  }
};

template <>
inline double AudioTimelineEvent::Time<double>() const
{
  MOZ_ASSERT(!mTimeIsInTicks);
  return mTime;
}

template <>
inline int64_t AudioTimelineEvent::Time<int64_t>() const
{
  MOZ_ASSERT(mTimeIsInTicks);
  return mTimeInTicks;
}








template <class ErrorResult>
class AudioEventTimeline
{
public:
  explicit AudioEventTimeline(float aDefaultValue)
    : mValue(aDefaultValue),
      mComputedValue(aDefaultValue),
      mLastComputedValue(aDefaultValue)
  {
  }

  bool HasSimpleValue() const
  {
    return mEvents.IsEmpty();
  }

  float GetValue() const
  {
    
    MOZ_ASSERT(HasSimpleValue());
    return mValue;
  }

  float Value() const
  {
    
    return mValue;
  }

  void SetValue(float aValue)
  {
    
    if (mEvents.IsEmpty()) {
      mLastComputedValue = mComputedValue = mValue = aValue;
    }
  }

  void SetValueAtTime(float aValue, double aStartTime, ErrorResult& aRv)
  {
    InsertEvent(AudioTimelineEvent(AudioTimelineEvent::SetValue, aStartTime, aValue), aRv);
  }

  void LinearRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    InsertEvent(AudioTimelineEvent(AudioTimelineEvent::LinearRamp, aEndTime, aValue), aRv);
  }

  void ExponentialRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    InsertEvent(AudioTimelineEvent(AudioTimelineEvent::ExponentialRamp, aEndTime, aValue), aRv);
  }

  void SetTargetAtTime(float aTarget, double aStartTime, double aTimeConstant, ErrorResult& aRv)
  {
    InsertEvent(AudioTimelineEvent(AudioTimelineEvent::SetTarget, aStartTime, aTarget, aTimeConstant), aRv);
  }

  void SetValueCurveAtTime(const float* aValues, uint32_t aValuesLength, double aStartTime, double aDuration, ErrorResult& aRv)
  {
    InsertEvent(AudioTimelineEvent(AudioTimelineEvent::SetValueCurve, aStartTime, 0.0f, 0.0f, aDuration, aValues, aValuesLength), aRv);
  }

  void CancelScheduledValues(double aStartTime)
  {
    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      if (mEvents[i].mTime >= aStartTime) {
#ifdef DEBUG
        
        
        for (unsigned j = i + 1; j < mEvents.Length(); ++j) {
          MOZ_ASSERT(mEvents[j].mTime >= aStartTime);
        }
#endif
        mEvents.TruncateLength(i);
        break;
      }
    }
  }

  void CancelAllEvents()
  {
    mEvents.Clear();
  }

  static bool TimesEqual(int64_t aLhs, int64_t aRhs)
  {
    return aLhs == aRhs;
  }

  
  
  static bool TimesEqual(double aLhs, double aRhs)
  {
    const float kEpsilon = 0.0000000001f;
    return fabs(aLhs - aRhs) < kEpsilon;
  }

  template<class TimeType>
  float GetValueAtTime(TimeType aTime)
  {
    mComputedValue = GetValueAtTimeHelper(aTime);
    return mComputedValue;
  }

  
  template<class TimeType>
  float GetValueAtTimeHelper(TimeType aTime)
  {
    const AudioTimelineEvent* previous = nullptr;
    const AudioTimelineEvent* next = nullptr;

    bool bailOut = false;
    for (unsigned i = 0; !bailOut && i < mEvents.Length(); ++i) {
      switch (mEvents[i].mType) {
      case AudioTimelineEvent::SetValue:
      case AudioTimelineEvent::SetTarget:
      case AudioTimelineEvent::LinearRamp:
      case AudioTimelineEvent::ExponentialRamp:
      case AudioTimelineEvent::SetValueCurve:
        if (TimesEqual(aTime, mEvents[i].template Time<TimeType>())) {
          mLastComputedValue = mComputedValue;
          
          do {
            ++i;
          } while (i < mEvents.Length() &&
                   aTime == mEvents[i].template Time<TimeType>());

          
          if (mEvents[i - 1].mType == AudioTimelineEvent::SetTarget) {
            
            
            return ExponentialApproach(mEvents[i - 1].template Time<TimeType>(),
                                       mLastComputedValue, mEvents[i - 1].mValue,
                                       mEvents[i - 1].mTimeConstant, aTime);
          }

          
          if (mEvents[i - 1].mType == AudioTimelineEvent::SetValueCurve) {
            return ExtractValueFromCurve(mEvents[i - 1].template Time<TimeType>(),
                                         mEvents[i - 1].mCurve,
                                         mEvents[i - 1].mCurveLength,
                                         mEvents[i - 1].mDuration, aTime);
          }

          
          return mEvents[i - 1].mValue;
        }
        previous = next;
        next = &mEvents[i];
        if (aTime < mEvents[i].template Time<TimeType>()) {
          bailOut = true;
        }
        break;
      default:
        MOZ_ASSERT(false, "unreached");
      }
    }
    
    if (!bailOut) {
      previous = next;
      next = nullptr;
    }

    
    if (!previous && !next) {
      return mValue;
    }

    
    if (!previous) {
      return mValue;
    }

    
    if (previous->mType == AudioTimelineEvent::SetTarget) {
      return ExponentialApproach(previous->template Time<TimeType>(),
                                 mLastComputedValue, previous->mValue,
                                 previous->mTimeConstant, aTime);
    }

    
    if (previous->mType == AudioTimelineEvent::SetValueCurve) {
      return ExtractValueFromCurve(previous->template Time<TimeType>(),
                                   previous->mCurve, previous->mCurveLength,
                                   previous->mDuration, aTime);
    }

    
    if (!next) {
      switch (previous->mType) {
      case AudioTimelineEvent::SetValue:
      case AudioTimelineEvent::LinearRamp:
      case AudioTimelineEvent::ExponentialRamp:
        
        return previous->mValue;
      case AudioTimelineEvent::SetValueCurve:
        return ExtractValueFromCurve(previous->template Time<TimeType>(),
                                     previous->mCurve, previous->mCurveLength,
                                     previous->mDuration, aTime);
      case AudioTimelineEvent::SetTarget:
        MOZ_ASSERT(false, "unreached");
      }
      MOZ_ASSERT(false, "unreached");
    }

    

    
    switch (next->mType) {
    case AudioTimelineEvent::LinearRamp:
      return LinearInterpolate(previous->template Time<TimeType>(), previous->mValue, next->template Time<TimeType>(), next->mValue, aTime);
    case AudioTimelineEvent::ExponentialRamp:
      return ExponentialInterpolate(previous->template Time<TimeType>(), previous->mValue, next->template Time<TimeType>(), next->mValue, aTime);
    case AudioTimelineEvent::SetValue:
    case AudioTimelineEvent::SetTarget:
    case AudioTimelineEvent::SetValueCurve:
      break;
    }

    
    switch (previous->mType) {
    case AudioTimelineEvent::SetValue:
    case AudioTimelineEvent::LinearRamp:
    case AudioTimelineEvent::ExponentialRamp:
      
      
      return previous->mValue;
    case AudioTimelineEvent::SetValueCurve:
      return ExtractValueFromCurve(previous->template Time<TimeType>(),
                                   previous->mCurve, previous->mCurveLength,
                                   previous->mDuration, aTime);
    case AudioTimelineEvent::SetTarget:
      MOZ_ASSERT(false, "unreached");
    }

    MOZ_ASSERT(false, "unreached");
    return 0.0f;
  }

  
  uint32_t GetEventCount() const
  {
    return mEvents.Length();
  }

  static float LinearInterpolate(double t0, float v0, double t1, float v1, double t)
  {
    return v0 + (v1 - v0) * ((t - t0) / (t1 - t0));
  }

  static float ExponentialInterpolate(double t0, float v0, double t1, float v1, double t)
  {
    return v0 * powf(v1 / v0, (t - t0) / (t1 - t0));
  }

  static float ExponentialApproach(double t0, double v0, float v1, double timeConstant, double t)
  {
    return v1 + (v0 - v1) * expf(-(t - t0) / timeConstant);
  }

  static float ExtractValueFromCurve(double startTime, float* aCurve, uint32_t aCurveLength, double duration, double t)
  {
    if (t >= startTime + duration) {
      
      return aCurve[aCurveLength - 1];
    }
    double ratio = std::max((t - startTime) / duration, 0.0);
    if (ratio >= 1.0) {
      return aCurve[aCurveLength - 1];
    }
    return aCurve[uint32_t(aCurveLength * ratio)];
  }

  void ConvertEventTimesToTicks(int64_t (*aConvertor)(double aTime, void* aClosure), void* aClosure,
                                int32_t aSampleRate)
  {
    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      mEvents[i].SetTimeInTicks(aConvertor(mEvents[i].template Time<double>(), aClosure));
      mEvents[i].mTimeConstant *= aSampleRate;
      mEvents[i].mDuration *= aSampleRate;
    }
  }

private:
  const AudioTimelineEvent* GetPreviousEvent(double aTime) const
  {
    const AudioTimelineEvent* previous = nullptr;
    const AudioTimelineEvent* next = nullptr;

    bool bailOut = false;
    for (unsigned i = 0; !bailOut && i < mEvents.Length(); ++i) {
      switch (mEvents[i].mType) {
      case AudioTimelineEvent::SetValue:
      case AudioTimelineEvent::SetTarget:
      case AudioTimelineEvent::LinearRamp:
      case AudioTimelineEvent::ExponentialRamp:
      case AudioTimelineEvent::SetValueCurve:
        if (aTime == mEvents[i].mTime) {
          
          do {
            ++i;
          } while (i < mEvents.Length() &&
                   aTime == mEvents[i].mTime);
          return &mEvents[i - 1];
        }
        previous = next;
        next = &mEvents[i];
        if (aTime < mEvents[i].mTime) {
          bailOut = true;
        }
        break;
      default:
        MOZ_ASSERT(false, "unreached");
      }
    }
    
    if (!bailOut) {
      previous = next;
    }

    return previous;
  }

  void InsertEvent(const AudioTimelineEvent& aEvent, ErrorResult& aRv)
  {
    if (!aEvent.IsValid()) {
      aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
      return;
    }

    
    
    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      if (mEvents[i].mType == AudioTimelineEvent::SetValueCurve &&
          mEvents[i].mTime <= aEvent.mTime &&
          (mEvents[i].mTime + mEvents[i].mDuration) >= aEvent.mTime) {
        aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
        return;
      }
    }

    
    
    if (aEvent.mType == AudioTimelineEvent::SetValueCurve) {
      for (unsigned i = 0; i < mEvents.Length(); ++i) {
        if (mEvents[i].mTime > aEvent.mTime &&
            mEvents[i].mTime < (aEvent.mTime + aEvent.mDuration)) {
          aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
          return;
        }
      }
    }

    
    if (aEvent.mType == AudioTimelineEvent::ExponentialRamp) {
      if (aEvent.mValue <= 0.f) {
        aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
        return;
      }
      const AudioTimelineEvent* previousEvent = GetPreviousEvent(aEvent.mTime);
      if (previousEvent) {
        if (previousEvent->mValue <= 0.f) {
          aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
          return;
        }
      } else {
        if (mValue <= 0.f) {
          aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
          return;
        }
      }
    }

    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      if (aEvent.mTime == mEvents[i].mTime) {
        if (aEvent.mType == mEvents[i].mType) {
          
          mEvents.ReplaceElementAt(i, aEvent);
        } else {
          
          do {
            ++i;
          } while (i < mEvents.Length() &&
                   aEvent.mType != mEvents[i].mType &&
                   aEvent.mTime == mEvents[i].mTime);
          mEvents.InsertElementAt(i, aEvent);
        }
        return;
      }
      
      if (aEvent.mTime < mEvents[i].mTime) {
        mEvents.InsertElementAt(i, aEvent);
        return;
      }
    }

    
    mEvents.AppendElement(aEvent);
  }

private:
  
  
  
  
  
  nsTArray<AudioTimelineEvent> mEvents;
  float mValue;
  
  float mComputedValue;
  
  float mLastComputedValue;
};

}
}

#endif

