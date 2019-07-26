





#ifndef mozilla_TimeStamp_h
#define mozilla_TimeStamp_h

#include "mozilla/Assertions.h"

#include "prinrval.h"
#include "nsDebug.h"
#include "prlong.h"

namespace IPC {
template <typename T> struct ParamTraits;
}

namespace mozilla {

class TimeStamp;











class TimeDuration
{
public:
  
  TimeDuration() : mValue(0) {}
  
  
  struct _SomethingVeryRandomHere;
  TimeDuration(_SomethingVeryRandomHere* aZero) : mValue(0) {
    MOZ_ASSERT(!aZero, "Who's playing funny games here?");
  }
  

  double ToSeconds() const;
  
  
  
  double ToSecondsSigDigits() const;
  double ToMilliseconds() const {
    return ToSeconds() * 1000.0;
  }
  double ToMicroseconds() const {
    return ToMilliseconds() * 1000.0;
  }

  
  
  
  
  
  static inline TimeDuration FromSeconds(double aSeconds) {
    return FromMilliseconds(aSeconds * 1000.0);
  }
  static TimeDuration FromMilliseconds(double aMilliseconds);
  static inline TimeDuration FromMicroseconds(double aMicroseconds) {
    return FromMilliseconds(aMicroseconds / 1000.0);
  }

  TimeDuration operator+(const TimeDuration& aOther) const {
    return TimeDuration::FromTicks(mValue + aOther.mValue);
  }
  TimeDuration operator-(const TimeDuration& aOther) const {
    return TimeDuration::FromTicks(mValue - aOther.mValue);
  }
  TimeDuration& operator+=(const TimeDuration& aOther) {
    mValue += aOther.mValue;
    return *this;
  }
  TimeDuration& operator-=(const TimeDuration& aOther) {
    mValue -= aOther.mValue;
    return *this;
  }
  double operator/(const TimeDuration& aOther) {
    return static_cast<double>(mValue) / aOther.mValue;
  }

  bool operator<(const TimeDuration& aOther) const {
    return mValue < aOther.mValue;
  }
  bool operator<=(const TimeDuration& aOther) const {
    return mValue <= aOther.mValue;
  }
  bool operator>=(const TimeDuration& aOther) const {
    return mValue >= aOther.mValue;
  }
  bool operator>(const TimeDuration& aOther) const {
    return mValue > aOther.mValue;
  }
  bool operator==(const TimeDuration& aOther) const {
    return mValue == aOther.mValue;
  }

  
  
  
  
  static TimeDuration Resolution();

  
  
  
  
  
  

private:
  friend class TimeStamp;
  friend struct IPC::ParamTraits<mozilla::TimeDuration>;

  static TimeDuration FromTicks(int64_t aTicks) {
    TimeDuration t;
    t.mValue = aTicks;
    return t;
  }

  static TimeDuration FromTicks(double aTicks) {
    
    
    if (aTicks >= double(LL_MAXINT))
      return TimeDuration::FromTicks(LL_MAXINT);

    
    if (aTicks <= double(LL_MININT))
      return TimeDuration::FromTicks(LL_MININT);

    return TimeDuration::FromTicks(int64_t(aTicks));
  }

  
  int64_t mValue;
};































class TimeStamp
{
public:
  


  TimeStamp() : mValue(0) {}
  

  


  bool IsNull() const { return mValue == 0; }
  




  static TimeStamp Now();
  


  TimeDuration operator-(const TimeStamp& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    PR_STATIC_ASSERT(-LL_MAXINT > LL_MININT);
    int64_t ticks = int64_t(mValue - aOther.mValue);
    
    if (mValue > aOther.mValue) {
      if (ticks < 0) {
        ticks = LL_MAXINT;
      }
    } else {
      if (ticks > 0) {
        ticks = LL_MININT;
      }
    }
    return TimeDuration::FromTicks(ticks);
  }

  TimeStamp operator+(const TimeDuration& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue + aOther.mValue);
  }
  TimeStamp operator-(const TimeDuration& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue - aOther.mValue);
  }
  TimeStamp& operator+=(const TimeDuration& aOther) {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    mValue += aOther.mValue;
    return *this;
  }
  TimeStamp& operator-=(const TimeDuration& aOther) {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    mValue -= aOther.mValue;
    return *this;
  }

  bool operator<(const TimeStamp& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue < aOther.mValue;
  }
  bool operator<=(const TimeStamp& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue <= aOther.mValue;
  }
  bool operator>=(const TimeStamp& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue >= aOther.mValue;
  }
  bool operator>(const TimeStamp& aOther) const {
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue > aOther.mValue;
  }
  bool operator==(const TimeStamp& aOther) const {
    
    MOZ_ASSERT(!IsNull() && "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue == aOther.mValue;
  }
  bool operator!=(const TimeStamp& aOther) const {
    
    MOZ_ASSERT(!IsNull(), "Cannot compute with a null value");
    MOZ_ASSERT(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue != aOther.mValue;
  }

  
  
  

  static NS_HIDDEN_(nsresult) Startup();
  static NS_HIDDEN_(void) Shutdown();

private:
  friend struct IPC::ParamTraits<mozilla::TimeStamp>;

  TimeStamp(uint64_t aValue) : mValue(aValue) {}

  












  uint64_t mValue;
};

}

#endif 
