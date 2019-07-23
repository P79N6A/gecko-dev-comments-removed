



#include "base/worker_pool.h"

#include "base/logging.h"
#include "base/task.h"

namespace {

DWORD CALLBACK WorkItemCallback(void* param) {
  Task* task = static_cast<Task*>(param);
  task->Run();
  delete task;
  return 0;
}

}  

bool WorkerPool::PostTask(const tracked_objects::Location& from_here,
                          Task* task, bool task_is_slow) {
  task->SetBirthPlace(from_here);

  ULONG flags = 0;
  if (task_is_slow)
    flags |= WT_EXECUTELONGFUNCTION;

  if (!QueueUserWorkItem(WorkItemCallback, task, flags)) {
    DLOG(ERROR) << "QueueUserWorkItem failed: " << GetLastError();
    delete task;
    return false;
  }

  return true;
}
