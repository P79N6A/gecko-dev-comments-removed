






#ifndef mozilla_ipc_ProcessChild_h
#define mozilla_ipc_ProcessChild_h

#include "base/message_loop.h"
#include "base/process.h"

#include "chrome/common/child_process.h"





namespace mozilla {
namespace ipc {

class ProcessChild : public ChildProcess {
protected:
  typedef base::ProcessId ProcessId;

public:
  explicit ProcessChild(ProcessId aParentPid);
  virtual ~ProcessChild();

  virtual bool Init() = 0;
  virtual void CleanUp()
  { }

  static MessageLoop* message_loop() {
    return gProcessChild->mUILoop;
  }

protected:
  static ProcessChild* current() {
    return gProcessChild;
  }

  ProcessId ParentPid() {
    return mParentPid;
  }

private:
  static ProcessChild* gProcessChild;

  MessageLoop* mUILoop;
  ProcessId mParentPid;

  DISALLOW_EVIL_CONSTRUCTORS(ProcessChild);
};

} 
} 


#endif 
