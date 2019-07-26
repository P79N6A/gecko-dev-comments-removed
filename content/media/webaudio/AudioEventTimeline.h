





#ifndef AudioEventTimeline_h_
#define AudioEventTimeline_h_

#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "nsTArray.h"
#include "math.h"

namespace mozilla {

namespace dom {













template <class FloatArrayWrapper, class ErrorResult>
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

    Event(Type aType, float aTime, float aValue, float aTimeConstant = 0.0,
          float aDuration = 0.0, FloatArrayWrapper aCurve = FloatArrayWrapper())
      : mType(aType)
      , mTime(aTime)
      , mValue(aValue)
      , mTimeConstant(aTimeConstant)
      , mDuration(aDuration)
    {
      if (aCurve.inited()) {
        mCurve = aCurve;
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
    float mTime;
    float mValue;
    float mTimeConstant;
    float mDuration;
    FloatArrayWrapper mCurve;

  private:
    static bool IsValid(float value)
    {
      return MOZ_DOUBLE_IS_FINITE(value);
    }
  };

public:
  AudioEventTimeline(float aDefaultValue,
                     float aMinValue,
                     float aMaxValue)
    : mValue(aDefaultValue)
    , mDefaultValue(aDefaultValue)
    , mMinValue(aMinValue)
    , mMaxValue(aMaxValue)
  {
    MOZ_ASSERT(aDefaultValue >= aMinValue);
    MOZ_ASSERT(aDefaultValue <= aMaxValue);
    MOZ_ASSERT(aMinValue < aMaxValue);
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

  float MinValue() const
  {
    return mMinValue;
  }

  float MaxValue() const
  {
    return mMaxValue;
  }

  float DefaultValue() const
  {
    return mDefaultValue;
  }

  void SetValueAtTime(float aValue, float aStartTime, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::SetValue, aStartTime, aValue), aRv);
  }

  void LinearRampToValueAtTime(float aValue, float aEndTime, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::LinearRamp, aEndTime, aValue), aRv);
  }

  void ExponentialRampToValueAtTime(float aValue, float aEndTime, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::ExponentialRamp, aEndTime, aValue), aRv);
  }

  void SetTargetAtTime(float aTarget, float aStartTime, float aTimeConstant, ErrorResult& aRv)
  {
    InsertEvent(Event(Event::SetTarget, aStartTime, aTarget, aTimeConstant), aRv);
  }

  void SetValueCurveAtTime(const FloatArrayWrapper& aValues, float aStartTime, float aDuration, ErrorResult& aRv)
  {
    
    
  }

  void CancelScheduledValues(float aStartTime)
  {
    
  }

  
  float GetValueAtTime(float aTime) const
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
        
        return LinearInterpolate(0.0f, mValue, next->mTime, next->mValue, aTime);
      case Event::ExponentialRamp:
        
        return ExponentialInterpolate(0.0f, mValue, next->mTime, next->mValue, aTime);
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

  static float LinearInterpolate(float t0, float v0, float t1, float v1, float t)
  {
    return v0 + (v1 - v0) * ((t - t0) / (t1 - t0));
  }

  static float ExponentialInterpolate(float t0, float v0, float t1, float v1, float t)
  {
    return v0 * powf(v1 / v0, (t - t0) / (t1 - t0));
  }

  static float ExponentialApproach(float t0, float v0, float v1, float timeConstant, float t)
  {
    return v1 + (v0 - v1) * expf(-(t - t0) / timeConstant);
  }

private:
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
  const float mDefaultValue;
  const float mMinValue;
  const float mMaxValue;
};

}
}

#endif

