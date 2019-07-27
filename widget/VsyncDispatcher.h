




#ifndef mozilla_widget_VsyncDispatcher_h
#define mozilla_widget_VsyncDispatcher_h

#include "base/message_loop.h"
#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"

namespace mozilla {
class TimeStamp;

namespace layers {
class CompositorVsyncObserver;
}

class VsyncObserver
{
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncObserver)

public:
  
  
  virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) = 0;

protected:
  VsyncObserver() {}
  virtual ~VsyncObserver() {}
}; 


class VsyncDispatcher
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncDispatcher)

public:
  VsyncDispatcher();

  
  
  
  
  
  
  void NotifyVsync(TimeStamp aVsyncTimestamp);

  
  void SetCompositorVsyncObserver(VsyncObserver* aVsyncObserver);
  void Shutdown();

private:
  virtual ~VsyncDispatcher();
  Mutex mCompositorObserverLock;
  nsRefPtr<VsyncObserver> mCompositorVsyncObserver;
}; 

} 

#endif 
