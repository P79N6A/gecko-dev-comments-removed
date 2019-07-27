




#ifndef mozilla_widget_VsyncDispatcher_h
#define mozilla_widget_VsyncDispatcher_h

#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"
#include "nsRefPtr.h"

namespace mozilla {

class VsyncObserver
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncObserver)

public:
  
  
  
  
  
  virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) = 0;

protected:
  VsyncObserver() {}
  virtual ~VsyncObserver() {}
}; 


class CompositorVsyncDispatcher MOZ_FINAL
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositorVsyncDispatcher)

public:
  CompositorVsyncDispatcher();

  
  void NotifyVsync(TimeStamp aVsyncTimestamp);

  
  void SetCompositorVsyncObserver(VsyncObserver* aVsyncObserver);
  void Shutdown();

private:
  virtual ~CompositorVsyncDispatcher();
  void ObserveVsync(bool aEnable);

  Mutex mCompositorObserverLock;
  nsRefPtr<VsyncObserver> mCompositorVsyncObserver;
  bool mDidShutdown;
};


class RefreshTimerVsyncDispatcher MOZ_FINAL
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefreshTimerVsyncDispatcher)

public:
  RefreshTimerVsyncDispatcher();

  
  void NotifyVsync(TimeStamp aVsyncTimestamp);

  
  
  void SetParentRefreshTimer(VsyncObserver* aVsyncObserver);

  
  
  
  
  void AddChildRefreshTimer(VsyncObserver* aVsyncObserver);
  void RemoveChildRefreshTimer(VsyncObserver* aVsyncObserver);

private:
  virtual ~RefreshTimerVsyncDispatcher();
  void UpdateVsyncStatus();
  bool NeedsVsync();

  Mutex mRefreshTimersLock;
  nsRefPtr<VsyncObserver> mParentRefreshTimer;
  nsTArray<nsRefPtr<VsyncObserver>> mChildRefreshTimers;
};

} 

#endif 
