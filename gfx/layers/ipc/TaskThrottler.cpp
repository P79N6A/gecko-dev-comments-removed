





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
                        CancelableTask* aTask)
{
  aTask->SetBirthPlace(aLocation);

  if (mOutstanding) {
    if (mQueuedTask) {
      mQueuedTask->Cancel();
    }
    mQueuedTask = aTask;
  } else {
    aTask->Run();
    delete aTask;
    mOutstanding = true;
  }
}

bool
TaskThrottler::TaskComplete()
{
  if (mQueuedTask) {
    mQueuedTask->Run();
    mQueuedTask = nullptr;
    return true;
  }
  mOutstanding = false;
  return false;
}

}
}
