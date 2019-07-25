





































#ifndef mozilla_TimeStamp_h
#define mozilla_TimeStamp_h

#include "prinrval.h"
#include "nsDebug.h"
#include "prlong.h"

namespace mozilla {

class TimeStamp;











class NS_COM TimeDuration
{
public:
  
  TimeDuration() : mValue(0) {}
  
  
  struct _SomethingVeryRandomHere;
  TimeDuration(_SomethingVeryRandomHere* aZero) : mValue(0) {
    NS_ASSERTION(!aZero, "Who's playing funny games here?");
  }
  

  double ToSeconds() const;
  
  
  
  double ToSecondsSigDigits() const;
  double ToMilliseconds() const {
    return ToSeconds() * 1000.0;
  }

  
  
  
  
  
  static inline TimeDuration FromSeconds(double aSeconds) {
    return FromMilliseconds(aSeconds * 1000.0);
  }
  static TimeDuration FromMilliseconds(double aMilliseconds);

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

  PRBool operator<(const TimeDuration& aOther) const {
    return mValue < aOther.mValue;
  }
  PRBool operator<=(const TimeDuration& aOther) const {
    return mValue <= aOther.mValue;
  }
  PRBool operator>=(const TimeDuration& aOther) const {
    return mValue >= aOther.mValue;
  }
  PRBool operator>(const TimeDuration& aOther) const {
    return mValue > aOther.mValue;
  }

  
  
  
  
  static TimeDuration Resolution();

  
  
  
  
  
  

private:
  friend class TimeStamp;

  static TimeDuration FromTicks(PRInt64 aTicks) {
    TimeDuration t;
    t.mValue = aTicks;
    return t;
  }

  static TimeDuration FromTicks(double aTicks) {
    
    
    if (aTicks >= double(LL_MAXINT))
      return TimeDuration::FromTicks(LL_MAXINT);

    
    if (aTicks <= double(LL_MININT))
      return TimeDuration::FromTicks(LL_MININT);

    return TimeDuration::FromTicks(PRInt64(aTicks));
  }

  
  PRInt64 mValue;
};

























class NS_COM TimeStamp
{
public:
  


  TimeStamp() : mValue(0) {}
  

  


  PRBool IsNull() const { return mValue == 0; }
  




  static TimeStamp Now();
  


  TimeDuration operator-(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    PR_STATIC_ASSERT(-LL_MAXINT > LL_MININT);
    PRInt64 ticks = PRInt64(mValue - aOther.mValue);
    
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
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue + aOther.mValue);
  }
  TimeStamp operator-(const TimeDuration& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    return TimeStamp(mValue - aOther.mValue);
  }
  TimeStamp& operator+=(const TimeDuration& aOther) {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    mValue += aOther.mValue;
    return *this;
  }
  TimeStamp& operator-=(const TimeDuration& aOther) {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    mValue -= aOther.mValue;
    return *this;
  }

  PRBool operator<(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue < aOther.mValue;
  }
  PRBool operator<=(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue <= aOther.mValue;
  }
  PRBool operator>=(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue >= aOther.mValue;
  }
  PRBool operator>(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue > aOther.mValue;
  }
  PRBool operator==(const TimeStamp& aOther) const {
    
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue == aOther.mValue;
  }
  PRBool operator!=(const TimeStamp& aOther) const {
    
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return mValue != aOther.mValue;
  }

  
  
  

  static NS_HIDDEN_(nsresult) Startup();
  static NS_HIDDEN_(void) Shutdown();

private:
  TimeStamp(PRUint64 aValue) : mValue(aValue) {}

  












  PRUint64 mValue;
};

}

#endif 
