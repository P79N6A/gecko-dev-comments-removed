









#include <algorithm>

#include "webrtc/base/taskrunner.h"

#include "webrtc/base/common.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/task.h"
#include "webrtc/base/logging.h"

namespace rtc {

TaskRunner::TaskRunner()
  : TaskParent(this),
    next_timeout_task_(NULL),
    tasks_running_(false)
#ifdef _DEBUG
    , abort_count_(0),
    deleting_task_(NULL)
#endif
{
}

TaskRunner::~TaskRunner() {
  
  AbortAllChildren();
  InternalRunTasks(true);
}

void TaskRunner::StartTask(Task * task) {
  tasks_.push_back(task);

  
  
  UpdateTaskTimeout(task, 0);

  WakeTasks();
}

void TaskRunner::RunTasks() {
  InternalRunTasks(false);
}

void TaskRunner::InternalRunTasks(bool in_destructor) {
  
  
  
  
  
  ASSERT(!abort_count_);
  
  if (tasks_running_) {
    return;  
  }

  tasks_running_ = true;

  int64 previous_timeout_time = next_task_timeout();

  int did_run = true;
  while (did_run) {
    did_run = false;
    
    for (size_t i = 0; i < tasks_.size(); ++i) {
      while (!tasks_[i]->Blocked()) {
        tasks_[i]->Step();
        did_run = true;
      }
    }
  }
  
  bool need_timeout_recalc = false;
  for (size_t i = 0; i < tasks_.size(); ++i) {
    if (tasks_[i]->IsDone()) {
      Task* task = tasks_[i];
      if (next_timeout_task_ &&
          task->unique_id() == next_timeout_task_->unique_id()) {
        next_timeout_task_ = NULL;
        need_timeout_recalc = true;
      }

#ifdef _DEBUG
      deleting_task_ = task;
#endif
      delete task;
#ifdef _DEBUG
      deleting_task_ = NULL;
#endif
      tasks_[i] = NULL;
    }
  }
  
  std::vector<Task *>::iterator it;
  it = std::remove(tasks_.begin(),
                   tasks_.end(),
                   reinterpret_cast<Task *>(NULL));

  tasks_.erase(it, tasks_.end());

  if (need_timeout_recalc)
    RecalcNextTimeout(NULL);

  
  
  
  if (!in_destructor)
    CheckForTimeoutChange(previous_timeout_time);

  tasks_running_ = false;
}

void TaskRunner::PollTasks() {
  
  
  
  
  
  
  Task* old_timeout_task = NULL;
  while (next_timeout_task_ &&
      old_timeout_task != next_timeout_task_ &&
      next_timeout_task_->TimedOut()) {
    old_timeout_task = next_timeout_task_;
    next_timeout_task_->Wake();
    WakeTasks();
  }
}

int64 TaskRunner::next_task_timeout() const {
  if (next_timeout_task_) {
    return next_timeout_task_->timeout_time();
  }
  return 0;
}








void TaskRunner::UpdateTaskTimeout(Task* task,
                                   int64 previous_task_timeout_time) {
  ASSERT(task != NULL);
  int64 previous_timeout_time = next_task_timeout();
  bool task_is_timeout_task = next_timeout_task_ != NULL &&
      task->unique_id() == next_timeout_task_->unique_id();
  if (task_is_timeout_task) {
    previous_timeout_time = previous_task_timeout_time;
  }

  
  
  
  if (task->timeout_time()) {
    if (next_timeout_task_ == NULL ||
        (task->timeout_time() <= next_timeout_task_->timeout_time())) {
      next_timeout_task_ = task;
    }
  } else if (task_is_timeout_task) {
    
    
    
    
    RecalcNextTimeout(task);
  }

  
  
  
  if (!tasks_running_) {
    CheckForTimeoutChange(previous_timeout_time);
  }
}

void TaskRunner::RecalcNextTimeout(Task *exclude_task) {
  
  
  
  
  

  int64 next_timeout_time = 0;
  next_timeout_task_ = NULL;

  for (size_t i = 0; i < tasks_.size(); ++i) {
    Task *task = tasks_[i];
    
    if (!task->IsDone() && (task->timeout_time() > 0))
      
      if (exclude_task == NULL ||
          exclude_task->unique_id() != task->unique_id())
        
        if (next_timeout_time == 0 ||
            task->timeout_time() <= next_timeout_time) {
          
          next_timeout_time = task->timeout_time();
          next_timeout_task_ = task;
        }
  }
}

void TaskRunner::CheckForTimeoutChange(int64 previous_timeout_time) {
  int64 next_timeout = next_task_timeout();
  bool timeout_change = (previous_timeout_time == 0 && next_timeout != 0) ||
      next_timeout < previous_timeout_time ||
      (previous_timeout_time <= CurrentTime() &&
       previous_timeout_time != next_timeout);
  if (timeout_change) {
    OnTimeoutChange();
  }
}

} 
