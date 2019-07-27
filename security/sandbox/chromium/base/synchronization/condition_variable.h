































































#ifndef BASE_SYNCHRONIZATION_CONDITION_VARIABLE_H_
#define BASE_SYNCHRONIZATION_CONDITION_VARIABLE_H_

#include "build/build_config.h"

#if defined(OS_POSIX)
#include <pthread.h>
#endif

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/synchronization/lock.h"

namespace base {

class ConditionVarImpl;
class TimeDelta;

class BASE_EXPORT ConditionVariable {
 public:
  
  explicit ConditionVariable(Lock* user_lock);

  ~ConditionVariable();

  
  
  void Wait();
  void TimedWait(const TimeDelta& max_time);

  
  void Broadcast();
  
  void Signal();

 private:

#if defined(OS_WIN)
  ConditionVarImpl* impl_;
#elif defined(OS_POSIX)
  pthread_cond_t condition_;
  pthread_mutex_t* user_mutex_;
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)
  base::Lock* user_lock_;     
#endif

#endif

  DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

}  

#endif  
