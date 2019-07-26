



#ifndef PENDING_TASK_H_
#define PENDING_TASK_H_

#include <queue>

#include "base/base_export.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/time/time.h"
#include "base/tracking_info.h"

namespace base {



struct BASE_EXPORT PendingTask : public TrackingInfo {
#if _MSC_VER >= 1700
  PendingTask();
#endif
  PendingTask(const tracked_objects::Location& posted_from,
              const Closure& task);
  PendingTask(const tracked_objects::Location& posted_from,
              const Closure& task,
              TimeTicks delayed_run_time,
              bool nestable);
  ~PendingTask();

  
  bool operator<(const PendingTask& other) const;

  
  Closure task;

  
  tracked_objects::Location posted_from;

  
  int sequence_num;

  
  bool nestable;
};



class BASE_EXPORT TaskQueue : public std::queue<PendingTask> {
 public:
  void Swap(TaskQueue* queue);
};


typedef std::priority_queue<base::PendingTask> DelayedTaskQueue;

}  

#endif  
