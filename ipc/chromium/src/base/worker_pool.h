



#ifndef BASE_WORKER_POOL_H_
#define BASE_WORKER_POOL_H_

#include "base/tracked.h"

class Task;



class WorkerPool {
 public:
  
  
  
  
  static bool PostTask(const tracked_objects::Location& from_here,
                       Task* task, bool task_is_slow);
};

#endif  
