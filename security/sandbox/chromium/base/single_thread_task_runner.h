



#ifndef BASE_SINGLE_THREAD_TASK_RUNNER_H_
#define BASE_SINGLE_THREAD_TASK_RUNNER_H_

#include "base/base_export.h"
#include "base/sequenced_task_runner.h"

namespace base {












class BASE_EXPORT SingleThreadTaskRunner : public SequencedTaskRunner {
 public:
  
  bool BelongsToCurrentThread() const {
    return RunsTasksOnCurrentThread();
  }

 protected:
  ~SingleThreadTaskRunner() override {}
};

}  

#endif  
