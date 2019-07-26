





#ifndef mozilla_dom_TaskThrottler_h
#define mozilla_dom_TaskThrottler_h

#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "mozilla/TimeStamp.h"

class CancelableTask;
namespace tracked_objects {
  class Location;
}

namespace mozilla {
namespace layers {














class TaskThrottler {
public:
  TaskThrottler();

  






  void PostTask(const tracked_objects::Location& aLocation,
                CancelableTask* aTask, const TimeStamp& aTimeStamp);
  


  void TaskComplete(const TimeStamp& aTimeStamp);

  



  TimeDuration AverageDuration();

  


  bool IsOutstanding() { return mOutstanding; }

  


  TimeDuration TimeSinceLastRequest(const TimeStamp& aTimeStamp =
                                    TimeStamp::Now());

  


  void ClearHistory() { mDurations.Clear(); }

  



  void SetMaxDurations(uint32_t aMaxDurations)
  {
    mMaxDurations = aMaxDurations;
  }

private:
  bool mOutstanding;
  nsAutoPtr<CancelableTask> mQueuedTask;
  TimeStamp mStartTime;

  
  nsTArray<TimeDuration> mDurations;
  uint32_t mMaxDurations;
};

}
}

#endif 
