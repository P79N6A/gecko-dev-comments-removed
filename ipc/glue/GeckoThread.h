



































#ifndef __IPC_GLUE_GECKOTHREAD_H__
#define __IPC_GLUE_GECKOTHREAD_H__

#include "base/thread.h"
#include "base/lock.h"
#include "base/process.h"

#include "chrome/common/child_thread.h"

#include "mozilla/ipc/ScopedXREEmbed.h"

class NotificationService;

namespace mozilla {
namespace ipc {

class GeckoThread : public ChildThread
{
public:
  typedef base::ProcessHandle ProcessHandle;

  GeckoThread(ProcessHandle aParentProcessHandle)
  : ChildThread(base::Thread::Options(
                    MessageLoop::TYPE_MOZILLA_CHILD, 
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

  ScopedXREEmbed mXREEmbed;

private:
  ProcessHandle mParentProcessHandle;

  DISALLOW_EVIL_CONSTRUCTORS(GeckoThread);
};


class BrowserProcessSubThread : public base::Thread
{
public:
  
  enum ID {
      IO,
      
      
      
#if defined(OS_LINUX)
      
      
      
      BACKGROUND_X11,
#endif

      
      
      
      ID_COUNT
  };

  explicit BrowserProcessSubThread(ID aId);
  ~BrowserProcessSubThread();

  static MessageLoop* GetMessageLoop(ID identifier);

protected:
  virtual void Init();
  virtual void CleanUp();

private:
  
  
  ID mIdentifier;

  NotificationService* mNotificationService;

  
  

  
  
  static Lock sLock;

  
  
  
  
  static BrowserProcessSubThread* sBrowserThreads[ID_COUNT];
};

} 
} 

#endif
