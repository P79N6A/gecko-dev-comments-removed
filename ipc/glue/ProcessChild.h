






#ifndef mozilla_ipc_ProcessChild_h
#define mozilla_ipc_ProcessChild_h

#include "base/message_loop.h"
#include "base/process.h"

#include "chrome/common/child_process.h"





namespace mozilla {
namespace ipc {

class ProcessChild : public ChildProcess {
protected:
  typedef base::ProcessHandle ProcessHandle;

public:
  explicit ProcessChild(ProcessHandle parentHandle);
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

  ProcessHandle ParentHandle() {
    return mParentHandle;
  }

private:
  static ProcessChild* gProcessChild;

  MessageLoop* mUILoop;
  ProcessHandle mParentHandle;

  DISALLOW_EVIL_CONSTRUCTORS(ProcessChild);
};

} 
} 


#endif 
