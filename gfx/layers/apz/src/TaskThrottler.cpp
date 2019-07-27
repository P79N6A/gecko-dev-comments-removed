





#include "TaskThrottler.h"

namespace mozilla {
namespace layers {

TaskThrottler::TaskThrottler(const TimeStamp& aTimeStamp)
  : mOutstanding(false)
  , mQueuedTask(nullptr)
  , mStartTime(aTimeStamp)
  , mMean(1)
{ }

void
TaskThrottler::PostTask(const tracked_objects::Location& aLocation,
                        UniquePtr<CancelableTask> aTask, const TimeStamp& aTimeStamp)
{
  aTask->SetBirthPlace(aLocation);

  if (mOutstanding) {
    if (mQueuedTask) {
      mQueuedTask->Cancel();
    }
    mQueuedTask = Move(aTask);
  } else {
    mStartTime = aTimeStamp;
    aTask->Run();
    mOutstanding = true;
  }
}

void
TaskThrottler::TaskComplete(const TimeStamp& aTimeStamp)
{
  if (!mOutstanding) {
    return;
  }

  mMean.insert(aTimeStamp - mStartTime);

  if (mQueuedTask) {
    mStartTime = aTimeStamp;
    mQueuedTask->Run();
    mQueuedTask = nullptr;
  } else {
    mOutstanding = false;
  }
}

TimeDuration
TaskThrottler::TimeSinceLastRequest(const TimeStamp& aTimeStamp)
{
  return aTimeStamp - mStartTime;
}

}
}
