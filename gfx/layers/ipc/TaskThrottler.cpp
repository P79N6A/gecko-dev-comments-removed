





#include "base/basictypes.h"
#include "base/message_loop.h"
#include "TaskThrottler.h"

namespace mozilla {
namespace layers {

TaskThrottler::TaskThrottler()
  : mOutstanding(false)
  , mQueuedTask(nullptr)
{ }

void
TaskThrottler::PostTask(const tracked_objects::Location& aLocation,
                        CancelableTask* aTask, const TimeStamp& aTimeStamp)
{
  aTask->SetBirthPlace(aLocation);

  if (mOutstanding) {
    if (mQueuedTask) {
      mQueuedTask->Cancel();
    }
    mQueuedTask = aTask;
  } else {
    mStartTime = aTimeStamp;
    aTask->Run();
    delete aTask;
    mOutstanding = true;
  }
}

void
TaskThrottler::TaskComplete(const TimeStamp& aTimeStamp)
{
  if (!mOutstanding) {
    return;
  }

  
  
  if (mDurations.Length() >= mMaxDurations) {
    mDurations.RemoveElementAt(0);
  }
  if (mMaxDurations) {
    mDurations.AppendElement(aTimeStamp - mStartTime);
  }

  if (mQueuedTask) {
    mStartTime = aTimeStamp;
    mQueuedTask->Run();
    mQueuedTask = nullptr;
  } else {
    mOutstanding = false;
  }
}

TimeDuration
TaskThrottler::AverageDuration()
{
  TimeDuration durationSum;
  for (uint32_t i = 0; i < mDurations.Length(); i++) {
    durationSum += mDurations[i];
  }

  return durationSum / mDurations.Length();
}

TimeDuration
TaskThrottler::TimeSinceLastRequest(const TimeStamp& aTimeStamp)
{
  return aTimeStamp - mStartTime;
}

}
}
