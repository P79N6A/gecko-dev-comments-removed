


















































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


class ChildReaper : public base::MessagePumpLibevent::SignalEvent,
                    public base::MessagePumpLibevent::SignalWatcher
{
public:
  explicit ChildReaper(pid_t process) : process_(process)
  {
  } 

  virtual ~ChildReaper()
  {
    
    DCHECK(!process_);

    
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

protected:
  void WaitForChildExit()
  {
    DCHECK(process_);
    HANDLE_EINTR(waitpid(process_, NULL, 0));
  }

  pid_t process_;

private:
  DISALLOW_EVIL_CONSTRUCTORS(ChildReaper);
};



class ChildGrimReaper : public ChildReaper,
                        public Task
{
public:
  explicit ChildGrimReaper(pid_t process) : ChildReaper(process)
  {
  } 

  virtual ~ChildGrimReaper()
  {
    if (process_)
      KillProcess();
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
      
      
      
      WaitForChildExit();
    }
    else {
      LOG(ERROR) << "Failed to deliver SIGKILL to " << process_ << "!"
                 << "("<< errno << ").";
    }
    process_ = 0;
  }

  DISALLOW_EVIL_CONSTRUCTORS(ChildGrimReaper);
};


class ChildLaxReaper : public ChildReaper,
                       public MessageLoop::DestructionObserver
{
public:
  explicit ChildLaxReaper(pid_t process) : ChildReaper(process)
  {
  } 

  virtual ~ChildLaxReaper()
  {
    
    DCHECK(!process_);
  }

  
  virtual void OnSignal(int sig)
  {
    ChildReaper::OnSignal(sig);

    if (!process_) {
      MessageLoop::current()->RemoveDestructionObserver(this);
      delete this;
    }
  }

  
  virtual void WillDestroyCurrentMessageLoop()
  {
    DCHECK(process_);

    WaitForChildExit();
    process_ = 0;

    
    
    MessageLoop::current()->RemoveDestructionObserver(this);
    delete this;
  }

private:
  DISALLOW_EVIL_CONSTRUCTORS(ChildLaxReaper);
};

}  



















void
ProcessWatcher::EnsureProcessTerminated(base::ProcessHandle process,
                                        bool force)
{
  DCHECK(process != base::GetCurrentProcId());
  DCHECK(process > 0);

  if (IsProcessDead(process))
    return;

  MessageLoopForIO* loop = MessageLoopForIO::current();
  if (force) {
    ChildGrimReaper* reaper = new ChildGrimReaper(process);

    loop->CatchSignal(SIGCHLD, reaper, reaper);
    
    loop->PostDelayedTask(FROM_HERE, reaper, kMaxWaitMs);
  } else {
    ChildLaxReaper* reaper = new ChildLaxReaper(process);

    loop->CatchSignal(SIGCHLD, reaper, reaper);
    
    loop->AddDestructionObserver(reaper);
  }
}
