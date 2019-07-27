




#ifndef mozilla_widget_VsyncDispatcher_h
#define mozilla_widget_VsyncDispatcher_h

#include "base/message_loop.h"
#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "ThreadSafeRefcountingWithMainThreadDestruction.h"

class MessageLoop;

namespace mozilla {
class TimeStamp;

namespace layers {
class CompositorVsyncObserver;
}


class VsyncSource
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)
  virtual void EnableVsync() = 0;
  virtual void DisableVsync() = 0;
  virtual bool IsVsyncEnabled() = 0;

protected:
  virtual ~VsyncSource() {}
}; 

class VsyncObserver
{
  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING_WITH_MAIN_THREAD_DESTRUCTION(VsyncObserver)

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
  static VsyncDispatcher* GetInstance();
  
  
  
  
  
  
  void NotifyVsync(TimeStamp aVsyncTimestamp);
  void SetVsyncSource(VsyncSource* aVsyncSource);

  
  void AddCompositorVsyncObserver(VsyncObserver* aVsyncObserver);
  void RemoveCompositorVsyncObserver(VsyncObserver* aVsyncObserver);

private:
  VsyncDispatcher();
  virtual ~VsyncDispatcher();
  void DispatchTouchEvents(bool aNotifiedCompositors, TimeStamp aVsyncTime);

  
  bool NotifyVsyncObservers(TimeStamp aVsyncTimestamp, nsTArray<nsRefPtr<VsyncObserver>>& aObservers);

  
  Mutex mCompositorObserverLock;
  nsTArray<nsRefPtr<VsyncObserver>> mCompositorObservers;
  nsRefPtr<VsyncSource> mVsyncSource;
}; 

} 

#endif 
