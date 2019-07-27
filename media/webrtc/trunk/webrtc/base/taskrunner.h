









#ifndef WEBRTC_BASE_TASKRUNNER_H__
#define WEBRTC_BASE_TASKRUNNER_H__

#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/taskparent.h"

namespace rtc {
class Task;

const int64 kSecToMsec = 1000;
const int64 kMsecTo100ns = 10000;
const int64 kSecTo100ns = kSecToMsec * kMsecTo100ns;

class TaskRunner : public TaskParent, public sigslot::has_slots<> {
 public:
  TaskRunner();
  virtual ~TaskRunner();

  virtual void WakeTasks() = 0;

  
  
  
  
  
  virtual int64 CurrentTime() = 0 ;

  void StartTask(Task *task);
  void RunTasks();
  void PollTasks();

  void UpdateTaskTimeout(Task *task, int64 previous_task_timeout_time);

#ifdef _DEBUG
  bool is_ok_to_delete(Task* task) {
    return task == deleting_task_;
  }

  void IncrementAbortCount() {
    ++abort_count_;
  }

  void DecrementAbortCount() {
    --abort_count_;
  }
#endif

  
  
  int64 next_task_timeout() const;

 protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void OnTimeoutChange() {
    
  }

 private:
  void InternalRunTasks(bool in_destructor);
  void CheckForTimeoutChange(int64 previous_timeout_time);

  std::vector<Task *> tasks_;
  Task *next_timeout_task_;
  bool tasks_running_;
#ifdef _DEBUG
  int abort_count_;
  Task* deleting_task_;
#endif

  void RecalcNextTimeout(Task *exclude_task);
};

} 

#endif  
