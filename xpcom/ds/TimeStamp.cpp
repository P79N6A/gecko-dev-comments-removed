









#include "mozilla/TimeStamp.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIAppStartup.h"

namespace mozilla {

TimeStamp TimeStamp::sFirstTimeStamp;
TimeStamp TimeStamp::sProcessCreation;

TimeStamp
TimeStamp::ProcessCreation(bool& aIsInconsistent)
{
  aIsInconsistent = false;

  if (sProcessCreation.IsNull()) {
    TimeStamp ts;

    
    nsCOMPtr<nsIAppStartup> appService =
      do_GetService("@mozilla.org/toolkit/app-startup;1");
    bool wasRestarted;
    appService->GetWasRestarted(&wasRestarted);

    if (wasRestarted) {
      

      ts = sFirstTimeStamp;
    } else {
      TimeStamp now = Now();
      uint64_t uptime = ComputeProcessUptime();

      ts = now - TimeDuration::FromMicroseconds(uptime);

      if ((ts > sFirstTimeStamp) || (uptime == 0)) {
        


        aIsInconsistent = true;
        ts = sFirstTimeStamp;
      }
    }

    sProcessCreation = ts;
  }

  return sProcessCreation;
}

void
TimeStamp::RecordProcessRestart()
{
  sProcessCreation = TimeStamp();
}

} 
