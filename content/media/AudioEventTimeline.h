





#ifndef AudioEventTimeline_h_
#define AudioEventTimeline_h_

#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypedEnum.h"

#include "nsTArray.h"
#include "math.h"

namespace mozilla {

namespace dom {








template <class ErrorResult>
class AudioEventTimeline
{
private:
  struct Event {
    enum Type MOZ_ENUM_TYPE(uint32_t) {
      SetValue,
      LinearRamp,
      ExponentialRamp,
      SetTarget,
      SetValueCurve
    };

    Event(Type aType, double aTime, float aValue, double aTimeConstant = 0.0,
          float aDuration = 0.0, float* aCurve = nullptr, uint32_t aCurveLength = 0)
      : mType(aType)
      , mTimeConstant(aTimeConstant)
      , mDuration(aDuration)
    {
      if (aType == Event::SetValueCurve) {
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

    Type mType;
    union {
      float mValue;
      uint32_t mCurveLength;
    };
    union {
      double mTime;
      float* mCurve;
    };
    double mTimeConstant;
    double mDuration;

  private:
    static bool IsValid(double value)
    {
      return MOZ_DOUBLE_IS_FINITE(value);
    }
  };

public:
  explicit AudioEventTimeline(float aDefaultValue)
    : mValue(aDefaultValue)
  {
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
    InsertEvent(Event(Event::SetValue, aStartTime, aValue), aRv);
  }

  void LinearRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::LinearRamp, aEndTime, aValue), aRv);
  }

  void ExponentialRampToValueAtTime(float aValue, double aEndTime, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::ExponentialRamp, aEndTime, aValue), aRv);
  }

  void SetTargetAtTime(float aTarget, double aStartTime, double aTimeConstant, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::SetTarget, aStartTime, aTarget, aTimeConstant), aRv);
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

  
  float GetValueAtTime(double aTime) const
  {
    const Event* previous = nullptr;
    const Event* next = nullptr;

    bool bailOut = false;
    for (unsigned i = 0; !bailOut && i < mEvents.Length(); ++i) {
      switch (mEvents[i].mType) {
      case Event::SetValue:
      case Event::SetTarget:
      case Event::LinearRamp:
      case Event::ExponentialRamp:
        if (aTime == mEvents[i].mTime) {
          
          do {
            ++i;
          } while (i < mEvents.Length() &&
                   aTime == mEvents[i].mTime);
          return mEvents[i - 1].mValue;
        }
        previous = next;
        next = &mEvents[i];
        if (aTime < mEvents[i].mTime) {
          bailOut = true;
        }
        break;
      case Event::SetValueCurve:
        
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
      case Event::SetValue:
      case Event::SetTarget:
        
        return mValue;
      case Event::LinearRamp:
        
        return LinearInterpolate(0.0, mValue, next->mTime, next->mValue, aTime);
      case Event::ExponentialRamp:
        
        return ExponentialInterpolate(0.0, mValue, next->mTime, next->mValue, aTime);
      case Event::SetValueCurve:
        
        return 0.0f;
      }
      MOZ_ASSERT(false, "unreached");
    }

    
    if (previous->mType == Event::SetTarget) {
      
      return ExponentialApproach(previous->mTime, mValue, previous->mValue,
                                 previous->mTimeConstant, aTime);
    }

    
    if (!next) {
      switch (previous->mType) {
      case Event::SetValue:
      case Event::LinearRamp:
      case Event::ExponentialRamp:
        
        return previous->mValue;
      case Event::SetValueCurve:
        
        return 0.0f;
      case Event::SetTarget:
        MOZ_ASSERT(false, "unreached");
      }
      MOZ_ASSERT(false, "unreached");
    }

    

    
    switch (next->mType) {
    case Event::LinearRamp:
      return LinearInterpolate(previous->mTime, previous->mValue, next->mTime, next->mValue, aTime);
    case Event::ExponentialRamp:
      return ExponentialInterpolate(previous->mTime, previous->mValue, next->mTime, next->mValue, aTime);
    case Event::SetValue:
    case Event::SetTarget:
    case Event::SetValueCurve:
      break;
    }

    
    switch (previous->mType) {
    case Event::SetValue:
    case Event::LinearRamp:
    case Event::ExponentialRamp:
      
      
      return previous->mValue;
    case Event::SetValueCurve:
      
      return 0.0f;
    case Event::SetTarget:
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

private:
  const Event* GetPreviousEvent(double aTime) const
  {
    const Event* previous = nullptr;
    const Event* next = nullptr;

    bool bailOut = false;
    for (unsigned i = 0; !bailOut && i < mEvents.Length(); ++i) {
      switch (mEvents[i].mType) {
      case Event::SetValue:
      case Event::SetTarget:
      case Event::LinearRamp:
      case Event::ExponentialRamp:
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
      case Event::SetValueCurve:
        
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

  void InsertEvent(const Event& aEvent, ErrorResult& aRv)
  {
    if (!aEvent.IsValid()) {
      aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
      return;
    }

    
    
    for (unsigned i = 0; i < mEvents.Length(); ++i) {
      if (mEvents[i].mType == Event::SetValueCurve &&
          mEvents[i].mTime <= aEvent.mTime &&
          (mEvents[i].mTime + mEvents[i].mDuration) >= aEvent.mTime) {
        aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
        return;
      }
    }

    
    
    if (aEvent.mType == Event::SetValueCurve) {
      for (unsigned i = 0; i < mEvents.Length(); ++i) {
        if (mEvents[i].mTime >= aEvent.mTime &&
            mEvents[i].mTime <= (aEvent.mTime + aEvent.mDuration)) {
          aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
          return;
        }
      }
    }

    
    if (aEvent.mType == Event::ExponentialRamp) {
      if (aEvent.mValue <= 0.f) {
        aRv.Throw(NS_ERROR_DOM_SYNTAX_ERR);
        return;
      }
      const Event* previousEvent = GetPreviousEvent(aEvent.mTime);
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
  
  
  
  
  
  nsTArray<Event> mEvents;
  float mValue;
};

}
}

#endif

