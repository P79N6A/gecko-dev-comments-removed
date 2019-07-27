





#ifndef mozilla_TimeStamp_h
#define mozilla_TimeStamp_h

#include <stdint.h>
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypeTraits.h"
#include "nscore.h"
#include "nsDebug.h"

namespace IPC {
template<typename T> struct ParamTraits;
}

#ifdef XP_WIN


#include "TimeStamp_windows.h"
#endif

namespace mozilla {

#ifndef XP_WIN
typedef uint64_t TimeStampValue;
#endif

class TimeStamp;




class BaseTimeDurationPlatformUtils
{
public:
  static double ToSeconds(int64_t aTicks);
  static double ToSecondsSigDigits(int64_t aTicks);
  static int64_t TicksFromMilliseconds(double aMilliseconds);
  static int64_t ResolutionInTicks();
};














template <typename ValueCalculator>
class BaseTimeDuration
{
public:
  
  MOZ_CONSTEXPR BaseTimeDuration() : mValue(0) {}
  
  
  struct _SomethingVeryRandomHere;
  MOZ_IMPLICIT BaseTimeDuration(_SomethingVeryRandomHere* aZero) : mValue(0)
  {
    MOZ_ASSERT(!aZero, "Who's playing funny games here?");
  }
  

  
  template <typename E>
  explicit BaseTimeDuration(const BaseTimeDuration<E>& aOther)
    : mValue(aOther.mValue)
  { }

  template <typename E>
  BaseTimeDuration& operator=(const BaseTimeDuration<E>& aOther)
  {
    mValue = aOther.mValue;
    return *this;
  }

  double ToSeconds() const
  {
    if (mValue == INT64_MAX) {
      return PositiveInfinity<double>();
    }
    if (mValue == INT64_MIN) {
      return NegativeInfinity<double>();
    }
    return BaseTimeDurationPlatformUtils::ToSeconds(mValue);
  }
  
  
  
  double ToSecondsSigDigits() const
  {
    if (mValue == INT64_MAX) {
      return PositiveInfinity<double>();
    }
    if (mValue == INT64_MIN) {
      return NegativeInfinity<double>();
    }
    return BaseTimeDurationPlatformUtils::ToSecondsSigDigits(mValue);
  }
  double ToMilliseconds() const { return ToSeconds() * 1000.0; }
  double ToMicroseconds() const { return ToMilliseconds() * 1000.0; }

  
  
  
  
  
  static inline BaseTimeDuration FromSeconds(double aSeconds)
  {
    return FromMilliseconds(aSeconds * 1000.0);
  }
  static BaseTimeDuration FromMilliseconds(double aMilliseconds)
  {
    if (aMilliseconds == PositiveInfinity<double>()) {
      return Forever();
    }
    if (aMilliseconds == NegativeInfinity<double>()) {
      return FromTicks(INT64_MIN);
    }
    return FromTicks(
      BaseTimeDurationPlatformUtils::TicksFromMilliseconds(aMilliseconds));
  }
  static inline BaseTimeDuration FromMicroseconds(double aMicroseconds)
  {
    return FromMilliseconds(aMicroseconds / 1000.0);
  }

  static BaseTimeDuration Forever()
  {
    return FromTicks(INT64_MAX);
  }

  BaseTimeDuration operator+(const BaseTimeDuration& aOther) const
  {
    return FromTicks(ValueCalculator::Add(mValue, aOther.mValue));
  }
  BaseTimeDuration operator-(const BaseTimeDuration& aOther) const
  {
    return FromTicks(ValueCalculator::Subtract(mValue, aOther.mValue));
  }
  BaseTimeDuration& operator+=(const BaseTimeDuration& aOther)
  {
    mValue = ValueCalculator::Add(mValue, aOther.mValue);
    return *this;
  }
  BaseTimeDuration& operator-=(const BaseTimeDuration& aOther)
  {
    mValue = ValueCalculator::Subtract(mValue, aOther.mValue);
    return *this;
  }
  BaseTimeDuration operator-() const
  {
    
    
    int64_t ticks;
    if (mValue == INT64_MAX) {
      ticks = INT64_MIN;
    } else if (mValue == INT64_MIN) {
      ticks = INT64_MAX;
    } else {
      ticks = -mValue;
    }

    return FromTicks(ticks);
  }

private:
  
  
  BaseTimeDuration operator*(const double aMultiplier) const = delete;

  
  
  
  BaseTimeDuration operator/(const double aDivisor) const = delete;

public:
  BaseTimeDuration MultDouble(double aMultiplier) const
  {
    return FromTicks(ValueCalculator::Multiply(mValue, aMultiplier));
  }
  BaseTimeDuration operator*(const int32_t aMultiplier) const
  {
    return FromTicks(ValueCalculator::Multiply(mValue, aMultiplier));
  }
  BaseTimeDuration operator*(const uint32_t aMultiplier) const
  {
    return FromTicks(ValueCalculator::Multiply(mValue, aMultiplier));
  }
  BaseTimeDuration operator*(const int64_t aMultiplier) const
  {
    return FromTicks(ValueCalculator::Multiply(mValue, aMultiplier));
  }
  BaseTimeDuration operator*(const uint64_t aMultiplier) const
  {
    if (aMultiplier > INT64_MAX) {
      NS_WARNING("Out-of-range multiplier when multiplying BaseTimeDuration");
      return Forever();
    }
    return FromTicks(ValueCalculator::Multiply(mValue, aMultiplier));
  }
  BaseTimeDuration operator/(const int64_t aDivisor) const
  {
    MOZ_ASSERT(aDivisor != 0, "Division by zero");
    return FromTicks(ValueCalculator::Divide(mValue, aDivisor));
  }
  double operator/(const BaseTimeDuration& aOther) const
  {
#ifndef MOZ_B2G
    
    MOZ_ASSERT(aOther.mValue != 0, "Division by zero");
#endif
    return ValueCalculator::DivideDouble(mValue, aOther.mValue);
  }
  BaseTimeDuration operator%(const BaseTimeDuration& aOther) const
  {
    MOZ_ASSERT(aOther.mValue != 0, "Division by zero");
    return FromTicks(ValueCalculator::Modulo(mValue, aOther.mValue));
  }

  template<typename E>
  bool operator<(const BaseTimeDuration<E>& aOther) const
  {
    return mValue < aOther.mValue;
  }
  template<typename E>
  bool operator<=(const BaseTimeDuration<E>& aOther) const
  {
    return mValue <= aOther.mValue;
  }
  template<typename E>
  bool operator>=(const BaseTimeDuration<E>& aOther) const
  {
    return mValue >= aOther.mValue;
  }
  template<typename E>
  bool operator>(const BaseTimeDuration<E>& aOther) const
  {
    return mValue > aOther.mValue;
  }
  template<typename E>
  bool operator==(const BaseTimeDuration<E>& aOther) const
  {
    return mValue == aOther.mValue;
  }
  template<typename E>
  bool operator!=(const BaseTimeDuration<E>& aOther) const
  {
    return mValue != aOther.mValue;
  }

  
  
  
  
  static BaseTimeDuration Resolution() {
    return FromTicks(BaseTimeDurationPlatformUtils::ResolutionInTicks());
  }

  
  
  
  
  
  

private:
  friend class TimeStamp;
  friend struct IPC::ParamTraits<mozilla::BaseTimeDuration<ValueCalculator>>;
  template <typename>
  friend class BaseTimeDuration;

  static BaseTimeDuration FromTicks(int64_t aTicks)
  {
    BaseTimeDuration t;
    t.mValue = aTicks;
    return t;
  }

  static BaseTimeDuration FromTicks(double aTicks)
  {
    
    
    if (aTicks >= double(INT64_MAX)) {
      return FromTicks(INT64_MAX);
    }

    
    if (aTicks <= double(INT64_MIN)) {
      return FromTicks(INT64_MIN);
    }

    return FromTicks(int64_t(aTicks));
  }

  
  int64_t mValue;
};





class TimeDurationValueCalculator
{
public:
  static int64_t Add(int64_t aA, int64_t aB) { return aA + aB; }
  static int64_t Subtract(int64_t aA, int64_t aB) { return aA - aB; }

  template <typename T>
  static int64_t Multiply(int64_t aA, T aB)
  {
    static_assert(IsIntegral<T>::value,
                  "Using integer multiplication routine with non-integer type."
                  " Further specialization required");
    return aA * static_cast<int64_t>(aB);
  }

  static int64_t Divide(int64_t aA, int64_t aB) { return aA / aB; }
  static double DivideDouble(int64_t aA, int64_t aB)
  {
    return static_cast<double>(aA) / aB;
  }
  static int64_t Modulo(int64_t aA, int64_t aB) { return aA % aB; }
};

template <>
inline int64_t
TimeDurationValueCalculator::Multiply<double>(int64_t aA, double aB)
{
  return static_cast<int64_t>(aA * aB);
}









typedef BaseTimeDuration<TimeDurationValueCalculator> TimeDuration;































class TimeStamp
{
public:
  


  MOZ_CONSTEXPR TimeStamp() : mValue(0) {}
  

  








#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_COCOA)
  static TimeStamp FromSystemTime(int64_t aSystemTime)
  {
    static_assert(sizeof(aSystemTime) == sizeof(TimeStampValue),
                  "System timestamp should be same units as TimeStampValue");
    return TimeStamp(aSystemTime);
  }
#endif

  


  bool IsNull() const { return mValue == 0; }

  












  static TimeStamp Now() { return Now(true); }
  static TimeStamp NowLoRes() { return Now(false); }

  











  static TimeStamp ProcessCreation(bool& aIsInconsistent);

  




  static void RecordProcessRestart();

  


  TimeDuration operator-(const TimeStamp& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    static_assert(-INT64_MAX > INT64_MIN, "int64_t sanity check");
    int64_t ticks = int64_t(mValue - aOther.mValue);
    
    if (mValue > aOther.mValue) {
      if (ticks < 0) {
        ticks = INT64_MAX;
      }
    } else {
      if (ticks > 0) {
        ticks = INT64_MIN;
      }
    }
    return TimeDuration::FromTicks(ticks);
  }

  TimeStamp operator+(const TimeDuration& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue + aOther.mValue);
  }
  TimeStamp operator-(const TimeDuration& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue - aOther.mValue);
  }
  TimeStamp& operator+=(const TimeDuration& aOther)
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    mValue += aOther.mValue;
    return *this;
  }
  TimeStamp& operator-=(const TimeDuration& aOther)
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    mValue -= aOther.mValue;
    return *this;
  }

  bool operator<(const TimeStamp& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue < aOther.mValue;
  }
  bool operator<=(const TimeStamp& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue <= aOther.mValue;
  }
  bool operator>=(const TimeStamp& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue >= aOther.mValue;
  }
  bool operator>(const TimeStamp& aOther) const
  {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue > aOther.mValue;
  }
  bool operator==(const TimeStamp& aOther) const
  {
    return IsNull()
           ? aOther.IsNull()
           : !aOther.IsNull() && mValue == aOther.mValue;
  }
  bool operator!=(const TimeStamp& aOther) const
  {
    return !(*this == aOther);
  }

  
  
  

  static nsresult Startup();
  static void Shutdown();

private:
  friend struct IPC::ParamTraits<mozilla::TimeStamp>;
  friend void StartupTimelineRecordExternal(int, uint64_t);

  MOZ_IMPLICIT TimeStamp(TimeStampValue aValue) : mValue(aValue) {}

  static TimeStamp Now(bool aHighResolution);

  







  static uint64_t ComputeProcessUptime();

  












  TimeStampValue mValue;
};

}

#endif 
