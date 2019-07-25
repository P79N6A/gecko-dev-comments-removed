





































#include "mozilla/TimeStamp.h"
#include "prlock.h"

namespace mozilla {

struct TimeStampInitialization
{
  TimeStampInitialization() {
    TimeStamp::Startup();
  }
  ~TimeStampInitialization() {
    TimeStamp::Shutdown();
  }
};

static TimeStampInitialization initOnce;

static PRLock* gTimeStampLock = 0;
static PRUint32 gRolloverCount;
static PRIntervalTime gLastNow;

double
TimeDuration::ToSeconds() const
{
 return double(mValue)/PR_TicksPerSecond();
}

double
TimeDuration::ToSecondsSigDigits() const
{
  return ToSeconds();
}

TimeDuration
TimeDuration::FromMilliseconds(double aMilliseconds)
{
  static double kTicksPerMs = double(PR_TicksPerSecond()) / 1000.0;
  return TimeDuration::FromTicks(aMilliseconds * kTicksPerMs);
}

TimeDuration
TimeDuration::Resolution()
{
  
  
  return TimeDuration::FromTicks(PRInt64(1));
}

nsresult
TimeStamp::Startup()
{
  if (gTimeStampLock)
    return NS_OK;

  gTimeStampLock = PR_NewLock();
  gRolloverCount = 1;
  gLastNow = 0;
  return gTimeStampLock ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
TimeStamp::Shutdown()
{
  if (gTimeStampLock) {
    PR_DestroyLock(gTimeStampLock);
    gTimeStampLock = nsnull;
  }
}

TimeStamp
TimeStamp::Now()
{
  
  
  PR_Lock(gTimeStampLock);

  PRIntervalTime now = PR_IntervalNow();
  if (now < gLastNow) {
    ++gRolloverCount;
    
    NS_ASSERTION(gRolloverCount > 0, "Rollover in rollover count???");
  }

  gLastNow = now;
  TimeStamp result((PRUint64(gRolloverCount) << 32) + now);

  PR_Unlock(gTimeStampLock);
  return result;
}

}
