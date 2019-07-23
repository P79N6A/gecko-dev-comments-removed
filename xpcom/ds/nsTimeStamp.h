





































#ifndef nsTimeStamp_h_
#define nsTimeStamp_h_

#include "prinrval.h"
#include "nsDebug.h"

namespace mozilla {

class TimeStamp;










class TimeDuration {
public:
  
  TimeDuration() : mValue(0) {}
  

  double ToSeconds() const { return double(mValue)/PR_TicksPerSecond(); }

  static TimeDuration FromSeconds(PRInt32 aSeconds) {
    
    return TimeDuration(PRInt64(aSeconds)*PR_TicksPerSecond());
  }
  static TimeDuration FromMilliseconds(PRInt32 aMilliseconds) {
    
    return TimeDuration(PRInt64(aMilliseconds)*PR_TicksPerSecond()/1000);
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

  
  
  
  
  
  

private:
  friend class TimeStamp;

  TimeDuration(PRInt64 aTicks) : mValue(aTicks) {}

  
  PRInt64 mValue;
};





















class NS_COM TimeStamp {
public:
  


  TimeStamp() : mValue(0) {}
  

  


  PRBool IsNull() const { return mValue == 0; }
  




  static TimeStamp Now();
  


  TimeDuration operator-(const TimeStamp& aOther) const {
    NS_ASSERTION(!IsNull(), "Cannot compute with a null value");
    NS_ASSERTION(!aOther.IsNull(), "Cannot compute with aOther null value");
    return TimeDuration(mValue - aOther.mValue);
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

  
  
  
  
  
  

  static NS_HIDDEN_(nsresult) Startup();
  static NS_HIDDEN_(void) Shutdown();

private:
  TimeStamp(PRUint64 aValue) : mValue(aValue) {}

  










  PRUint64 mValue;
};

}

#endif 
