









#include "mozilla/TimeStamp.h"
#include "prenv.h"

namespace mozilla {

TimeStamp TimeStamp::sFirstTimeStamp;
TimeStamp TimeStamp::sProcessCreation;

TimeStamp
TimeStamp::ProcessCreation(bool& aIsInconsistent)
{
  aIsInconsistent = false;

  if (sProcessCreation.IsNull()) {
    char *mozAppRestart = PR_GetEnv("MOZ_APP_RESTART");
    TimeStamp ts;

    


    if (mozAppRestart && (strcmp(mozAppRestart, "") != 0)) {
      

      ts = sFirstTimeStamp;
      PR_SetEnv("MOZ_APP_RESTART=");
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
  PR_SetEnv("MOZ_APP_RESTART=1");
  sProcessCreation = TimeStamp();
}

} 
