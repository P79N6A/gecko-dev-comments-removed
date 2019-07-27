



#ifndef BASE_SEQUENCED_TASKRUNNER_H_
#define BASE_SEQUENCED_TASKRUNNER_H_

#include "base/base_export.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/task_runner.h"

namespace base {



















































































class BASE_EXPORT SequencedTaskRunner : public TaskRunner {
 public:
  
  
  
  
  
  
  
  
  

  
  

  bool PostNonNestableTask(const tracked_objects::Location& from_here,
                           const Closure& task);

  virtual bool PostNonNestableDelayedTask(
      const tracked_objects::Location& from_here,
      const Closure& task,
      base::TimeDelta delay) = 0;

  
  
  
  template <class T>
  bool DeleteSoon(const tracked_objects::Location& from_here,
                  const T* object) {
    return
        subtle::DeleteHelperInternal<T, bool>::DeleteViaSequencedTaskRunner(
            this, from_here, object);
  }

  
  
  
  template <class T>
  bool ReleaseSoon(const tracked_objects::Location& from_here,
                   T* object) {
    return
        subtle::ReleaseHelperInternal<T, bool>::ReleaseViaSequencedTaskRunner(
            this, from_here, object);
  }

 protected:
  ~SequencedTaskRunner() override {}

 private:
  template <class T, class R> friend class subtle::DeleteHelperInternal;
  template <class T, class R> friend class subtle::ReleaseHelperInternal;

  bool DeleteSoonInternal(const tracked_objects::Location& from_here,
                          void(*deleter)(const void*),
                          const void* object);

  bool ReleaseSoonInternal(const tracked_objects::Location& from_here,
                           void(*releaser)(const void*),
                           const void* object);
};

}  

#endif  
