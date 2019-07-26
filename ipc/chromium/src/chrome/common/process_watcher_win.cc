



#include "chrome/common/process_watcher.h"

#include "base/message_loop.h"
#include "base/object_watcher.h"
#include "base/sys_info.h"
#include "chrome/common/env_vars.h"
#include "chrome/common/result_codes.h"


static const int kWaitInterval = 2000;

namespace {

class TimerExpiredTask : public Task, public base::ObjectWatcher::Delegate {
 public:
  explicit TimerExpiredTask(base::ProcessHandle process) : process_(process) {
    watcher_.StartWatching(process_, this);
  }

  virtual ~TimerExpiredTask() {
    if (process_) {
      KillProcess();
      DCHECK(!process_) << "Make sure to close the handle.";
    }
  }

  

  virtual void Run() {
    if (process_)
      KillProcess();
  }

  

  virtual void OnObjectSignaled(HANDLE object) {
    
    
    watcher_.StopWatching();

    base::CloseProcessHandle(process_);
    process_ = NULL;
  }

 private:
  void KillProcess() {
    if (base::SysInfo::HasEnvVar(env_vars::kHeadless)) {
     
     
     if (WaitForSingleObject(process_, kWaitInterval) == WAIT_OBJECT_0) {
       OnObjectSignaled(process_);
       return;
     }
    }

    
    
    
    
    TerminateProcess(process_, ResultCodes::HUNG);

    
    OnObjectSignaled(process_);
  }

  
  base::ProcessHandle process_;

  base::ObjectWatcher watcher_;

  DISALLOW_EVIL_CONSTRUCTORS(TimerExpiredTask);
};

}  


void ProcessWatcher::EnsureProcessTerminated(base::ProcessHandle process
					     , bool force
) {
  DCHECK(process != GetCurrentProcess());

  if (!force) {
    WaitForSingleObject(process, INFINITE);
    base::CloseProcessHandle(process);
    return;
  }

  
  if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
    base::CloseProcessHandle(process);
    return;
  }

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          new TimerExpiredTask(process),
                                          kWaitInterval);
}
