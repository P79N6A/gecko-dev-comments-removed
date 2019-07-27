





#ifndef mozilla_dom_TaskThrottler_h
#define mozilla_dom_TaskThrottler_h

#include <stdint.h>                     
#include "base/task.h"                  
#include "mozilla/TimeStamp.h"          
#include "mozilla/RollingMean.h"        
#include "mozilla/mozalloc.h"           
#include "mozilla/UniquePtr.h"          
#include "nsTArray.h"                   

namespace tracked_objects {
class Location;
}

namespace mozilla {
namespace layers {




















class TaskThrottler {
public:
  TaskThrottler(const TimeStamp& aTimeStamp, const TimeDuration& aMaxWait);

  








  void PostTask(const tracked_objects::Location& aLocation,
                UniquePtr<CancelableTask> aTask, const TimeStamp& aTimeStamp);
  


  void TaskComplete(const TimeStamp& aTimeStamp);

  



  TimeDuration AverageDuration()
  {
    return mMean.empty() ? TimeDuration() : mMean.mean();
  }

  


  bool IsOutstanding() { return mOutstanding; }

  


  TimeDuration TimeSinceLastRequest(const TimeStamp& aTimeStamp);

  


  void ClearHistory() { mMean.clear(); }

  



  void SetMaxDurations(uint32_t aMaxDurations)
  {
    if (aMaxDurations != mMean.maxValues()) {
      mMean = RollingMean<TimeDuration, TimeDuration>(aMaxDurations);
    }
  }

private:
  bool mOutstanding;
  UniquePtr<CancelableTask> mQueuedTask;
  TimeStamp mStartTime;
  TimeDuration mMaxWait;
  RollingMean<TimeDuration, TimeDuration> mMean;
};

}
}

#endif 
