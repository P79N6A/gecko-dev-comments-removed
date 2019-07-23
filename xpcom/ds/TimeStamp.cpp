





































#include "mozilla/TimeStamp.h"
#include "prlock.h"

namespace mozilla {

static PRLock* gTimeStampLock;
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
TimeDuration::FromSeconds(PRInt32 aSeconds)
{
  
  return TimeDuration::FromTicks(PRInt64(aSeconds)*PR_TicksPerSecond());
}

TimeDuration
TimeDuration::FromMilliseconds(PRInt32 aMilliseconds)
{
  
  return TimeDuration::FromTicks(PRInt64(aMilliseconds)*PR_TicksPerSecond()/1000);
}

TimeDuration
TimeDuration::Resolution()
{
  
  
  return TimeDuration::FromTicks(1);
}

nsresult
TimeStamp::Startup()
{
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
