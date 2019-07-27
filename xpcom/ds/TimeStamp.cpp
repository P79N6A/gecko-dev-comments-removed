









#include "mozilla/TimeStamp.h"
#include "prenv.h"

namespace mozilla {




struct TimeStampInitialization
{
  



  TimeStamp mFirstTimeStamp;

  




  TimeStamp mProcessCreation;

  TimeStampInitialization()
  {
    TimeStamp::Startup();
    mFirstTimeStamp = TimeStamp::Now();
  };

  ~TimeStampInitialization()
  {
    TimeStamp::Shutdown();
  };
};

static TimeStampInitialization sInitOnce;

TimeStamp
TimeStamp::ProcessCreation(bool& aIsInconsistent)
{
  aIsInconsistent = false;

  if (sInitOnce.mProcessCreation.IsNull()) {
    char* mozAppRestart = PR_GetEnv("MOZ_APP_RESTART");
    TimeStamp ts;

    


    if (mozAppRestart && (strcmp(mozAppRestart, "") != 0)) {
      

      ts = sInitOnce.mFirstTimeStamp;
    } else {
      TimeStamp now = Now();
      uint64_t uptime = ComputeProcessUptime();

      ts = now - TimeDuration::FromMicroseconds(uptime);

      if ((ts > sInitOnce.mFirstTimeStamp) || (uptime == 0)) {
        


        aIsInconsistent = true;
        ts = sInitOnce.mFirstTimeStamp;
      }
    }

    sInitOnce.mProcessCreation = ts;
  }

  return sInitOnce.mProcessCreation;
}

void
TimeStamp::RecordProcessRestart()
{
  sInitOnce.mProcessCreation = TimeStamp();
}

} 
