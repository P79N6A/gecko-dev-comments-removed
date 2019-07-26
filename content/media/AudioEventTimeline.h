





#ifndef AudioEventTimeline_h_
#define AudioEventTimeline_h_

#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypedEnum.h"

#include "nsTArray.h"
#include "math.h"

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
                     float aDuration = 0.0, float* aCurve = nullptr, uint32_t aCurveLength = 0)
    : mType(aType)
    , mTimeConstant(aTimeConstant)
    , mDuration(aDuration)
#ifdef DEBUG
    , mTimeIsInTicks(false)
#endif
  {
    if (aType == AudioTimelineEvent::SetValueCurve) {
      mCurve = aCurve;
      mCurveLength = aCurveLength;
    } else {
      mValue = aValue;
      mTime = aTime;
    }
  }

  bool IsValid() const
  {
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

  Type mType;
  union {
    float mValue;
    uint32_t mCurveLength;
  };
  union {
    
    
    
    
    
    union {
      double mTime;
      int64_t mTimeInTicks;
    };
    float* mCurve;
  };
  double mTimeConstant;
  double mDuration;
#ifdef DEBUG
  bool mTimeIsInTicks;
#endif

private:
  static bool IsValid(double value)
  {
    return MOZ_DOUBLE_IS_FINITE(value);
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
    : mValue(aDefaultValue)
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
      mValue = aValue;
    }
  }

  float ComputedValue() const
  {
    
    return 0;
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

  
  template<class TimeType>
  float GetValueAtTime(TimeType aTime) const
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
        if (aTime == mEvents[i].template Time<TimeType>()) {
          
          do {
            ++i;
          } while (i < mEvents.Length() &&
                   aTime == mEvents[i].template Time<TimeType>());
          return mEvents[i - 1].mValue;
        }
        previous = next;
        next = &mEvents[i];
        if (aTime < mEvents[i].template Time<TimeType>()) {
          bailOut = true;
        }
        break;
      case AudioTimelineEvent::SetValueCurve:
        
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
      switch (next->mType) {
      case AudioTimelineEvent::SetValue:
      case AudioTimelineEvent::SetTarget:
        
        return mValue;
      case AudioTimelineEvent::LinearRamp:
        
        return LinearInterpolate(0.0, mValue, next->template Time<TimeType>(), next->mValue, aTime);
      case AudioTimelineEvent::ExponentialRamp:
        
        return ExponentialInterpolate(0.0, mValue, next->template Time<TimeType>(), next->mValue, aTime);
      case AudioTimelineEvent::SetValueCurve:
        
        return 0.0f;
      }
      MOZ_ASSERT(false, "unreached");
    }

    
    if (previous->mType == AudioTimelineEvent::SetTarget) {
      
      return ExponentialApproach(previous->template Time<TimeType>(), mValue, previous->mValue,
                                 previous->mTimeConstant, aTime);
    }

    
    if (!next) {
      switch (previous->mType) {
      case AudioTimelineEvent::SetValue:
      case AudioTimelineEvent::LinearRamp:
      case AudioTimelineEvent::ExponentialRamp:
        
        return previous->mValue;
      case AudioTimelineEvent::SetValueCurve:
        
        return 0.0f;
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
      
      return 0.0f;
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

  void ConvertEventTimesToTicks(int64_t (*aConvertor)(double aTime, void* aClosure), void* aClosure)
  {
    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      mEvents[i].SetTimeInTicks(aConvertor(mEvents[i].template Time<double>(), aClosure));
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
      case AudioTimelineEvent::SetValueCurve:
        
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
        if (mEvents[i].mTime >= aEvent.mTime &&
            mEvents[i].mTime <= (aEvent.mTime + aEvent.mDuration)) {
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
};

}
}

#endif

