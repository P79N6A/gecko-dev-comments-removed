


















































#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"
#include "base/process_util.h"

#include "chrome/common/process_watcher.h"



static const int kMaxWaitMs = 2000;

namespace {

bool
IsProcessDead(pid_t process)
{
  bool exited = false;
  
  base::DidProcessCrash(&exited, process);
  return exited;
}


class ChildReaper : public Task,
                    public base::MessagePumpLibevent::SignalEvent,
                    public base::MessagePumpLibevent::SignalWatcher
{
public:
  explicit ChildReaper(pid_t process) : process_(process)
  {
  } 

  virtual ~ChildReaper()
  {
    if (process_)
      KillProcess();
    
  }

  
  virtual void OnSignal(int sig)
  {
    DCHECK(SIGCHLD == sig);
    DCHECK(process_);

    
    if (IsProcessDead(process_)) {
      process_ = 0;
      StopCatching();
    }
  }

  
  virtual void Run()
  {
    
    if (process_)
      KillProcess();
  }

private:
  void KillProcess()
  {
    DCHECK(process_);

    if (IsProcessDead(process_)) {
      process_ = 0;
      return;
    }

    if (0 == kill(process_, SIGKILL)) {
      
      
      
      HANDLE_EINTR(waitpid(process_, NULL, 0));
    }
    else {
      LOG(ERROR) << "Failed to deliver SIGKILL to " << process_ << "!"
                 << "("<< errno << ").";
    }
    process_ = 0;
  }

  pid_t process_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildReaper);
};

}  


void
ProcessWatcher::EnsureProcessTerminated(base::ProcessHandle process)
{
  DCHECK(process != base::GetCurrentProcId());
  DCHECK(process > 0);

  if (IsProcessDead(process))
    return;

  MessageLoopForIO* loop = MessageLoopForIO::current();
  ChildReaper* reaper = new ChildReaper(process);

  
  
  
  
  loop->CatchSignal(SIGCHLD, reaper, reaper);
  
  loop->PostDelayedTask(FROM_HERE, reaper, kMaxWaitMs);
}
