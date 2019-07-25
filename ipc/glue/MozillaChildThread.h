






































#ifndef mozilla_ipc_MozillaChildThread_h
#define mozilla_ipc_MozillaChildThread_h

#include "base/thread.h"
#include "base/process.h"

#include "chrome/common/child_thread.h"

namespace mozilla {
namespace ipc {






class MozillaChildThread : public ChildThread
{
public:
  typedef base::ProcessHandle ProcessHandle;

  MozillaChildThread(ProcessHandle aParentProcessHandle,
		     MessageLoop::Type type=MessageLoop::TYPE_UI)
  : ChildThread(base::Thread::Options(type,    
				      0,       
				      false)), 
    mParentProcessHandle(aParentProcessHandle)
  { }

protected:
  virtual void OnControlMessageReceived(const IPC::Message& aMessage);

  ProcessHandle GetParentProcessHandle() {
    return mParentProcessHandle;
  }

  
  virtual void Init();
  virtual void CleanUp();

private:
  ProcessHandle mParentProcessHandle;

  DISALLOW_EVIL_CONSTRUCTORS(MozillaChildThread);
};

} 
} 

#endif 
