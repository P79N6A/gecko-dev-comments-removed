



#include "chrome/common/process_watcher.h"

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "base/eintr_wrapper.h"
#include "base/platform_thread.h"



static bool IsChildDead(pid_t child) {
  const int result = HANDLE_EINTR(waitpid(child, NULL, WNOHANG));
  if (result == -1) {
    NOTREACHED();
  } else if (result > 0) {
    
    return true;
  }

  return false;
}



class BackgroundReaper : public PlatformThread::Delegate {
 public:
  explicit BackgroundReaper(pid_t child)
      : child_(child) {
  }

  void ThreadMain() {
    WaitForChildToDie();
    delete this;
  }

  void WaitForChildToDie() {
    
    

    
    for (unsigned i = 0; i < 4; ++i) {
      PlatformThread::Sleep(500);  
      if (IsChildDead(child_))
        return;
    }

    if (kill(child_, SIGKILL) == 0) {
      
      
      HANDLE_EINTR(waitpid(child_, NULL, 0));
    } else {
      LOG(ERROR) << "While waiting for " << child_ << " to terminate we"
                 << " failed to deliver a SIGKILL signal (" << errno << ").";
    }
  }

 private:
  const pid_t child_;

  DISALLOW_COPY_AND_ASSIGN(BackgroundReaper);
};


void ProcessWatcher::EnsureProcessTerminated(base::ProcessHandle process) {
  
  if (IsChildDead(process))
    return;

  BackgroundReaper* reaper = new BackgroundReaper(process);
  PlatformThread::CreateNonJoinable(0, reaper);
}
