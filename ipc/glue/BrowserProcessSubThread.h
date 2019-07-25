






#ifndef mozilla_ipc_BrowserProcessSubThread_h
#define mozilla_ipc_BrowserProcessSubThread_h

#include "base/thread.h"
#include "base/lock.h"

#include "nsDebug.h"

class NotificationService;

namespace mozilla {
namespace ipc {


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

inline void AssertIOThread()
{
  NS_ASSERTION(MessageLoop::TYPE_IO == MessageLoop::current()->type(),
	       "should be on the IO thread!");
}

} 
} 

#endif
