





#ifndef mozilla_dom_TaskThrottler_h
#define mozilla_dom_TaskThrottler_h

#include "nsAutoPtr.h"

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
                CancelableTask* aTask);
  


  bool TaskComplete();

private:
  bool mOutstanding;
  nsAutoPtr<CancelableTask> mQueuedTask;
};

}
}

#endif 
