









#ifndef WEBRTC_BASE_TASKPARENT_H__
#define WEBRTC_BASE_TASKPARENT_H__

#include <set>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {

class Task;
class TaskRunner;

class TaskParent {
 public:
  TaskParent(Task *derived_instance, TaskParent *parent);
  explicit TaskParent(TaskRunner *derived_instance);
  virtual ~TaskParent() { }

  TaskParent *GetParent() { return parent_; }
  TaskRunner *GetRunner() { return runner_; }

  bool AllChildrenDone();
  bool AnyChildError();
#ifdef _DEBUG
  bool IsChildTask(Task *task);
#endif

 protected:
  void OnStopped(Task *task);
  void AbortAllChildren();
  TaskParent *parent() {
    return parent_;
  }

 private:
  void Initialize();
  void OnChildStopped(Task *child);
  void AddChild(Task *child);

  TaskParent *parent_;
  TaskRunner *runner_;
  bool child_error_;
  typedef std::set<Task *> ChildSet;
  scoped_ptr<ChildSet> children_;
  DISALLOW_EVIL_CONSTRUCTORS(TaskParent);
};


} 

#endif  
