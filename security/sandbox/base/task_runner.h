



#ifndef BASE_TASK_RUNNER_H_
#define BASE_TASK_RUNNER_H_

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"

namespace tracked_objects {
class Location;
} 

namespace base {

struct TaskRunnerTraits;





































class BASE_EXPORT TaskRunner
    : public RefCountedThreadSafe<TaskRunner, TaskRunnerTraits> {
 public:
  
  
  
  
  
  bool PostTask(const tracked_objects::Location& from_here,
                const Closure& task);

  
  
  
  
  
  virtual bool PostDelayedTask(const tracked_objects::Location& from_here,
                               const Closure& task,
                               base::TimeDelta delay) = 0;

  
  
  
  
  
  
  virtual bool RunsTasksOnCurrentThread() const = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool PostTaskAndReply(const tracked_objects::Location& from_here,
                        const Closure& task,
                        const Closure& reply);

 protected:
  friend struct TaskRunnerTraits;

  
  
  friend class RefCountedThreadSafe<TaskRunner, TaskRunnerTraits>;

  TaskRunner();
  virtual ~TaskRunner();

  
  
  
  virtual void OnDestruct() const;
};

struct BASE_EXPORT TaskRunnerTraits {
  static void Destruct(const TaskRunner* task_runner);
};

}  

#endif  
