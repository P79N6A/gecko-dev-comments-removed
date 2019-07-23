






















#ifndef BASE_WORKER_POOL_LINUX_H_
#define BASE_WORKER_POOL_LINUX_H_

#include <queue>
#include <string>

#include "base/basictypes.h"
#include "base/condition_variable.h"
#include "base/lock.h"
#include "base/platform_thread.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"

class Task;

namespace base {

class LinuxDynamicThreadPool
    : public RefCountedThreadSafe<LinuxDynamicThreadPool> {
 public:
  class LinuxDynamicThreadPoolPeer;

  
  
  LinuxDynamicThreadPool(const std::string& name_prefix,
                         int idle_seconds_before_exit);
  ~LinuxDynamicThreadPool();

  
  
  void Terminate();

  
  
  void PostTask(Task* task);

  
  
  Task* WaitForTask();

 private:
  friend class LinuxDynamicThreadPoolPeer;

  const std::string name_prefix_;
  const int idle_seconds_before_exit_;

  Lock lock_;  

  
  
  
  ConditionVariable tasks_available_cv_;
  int num_idle_threads_;
  std::queue<Task*> tasks_;
  bool terminated_;
  
  
  scoped_ptr<ConditionVariable> num_idle_threads_cv_;

  DISALLOW_COPY_AND_ASSIGN(LinuxDynamicThreadPool);
};

}  

#endif  
