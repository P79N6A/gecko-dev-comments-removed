




#ifndef GFX_VSYNCSOURCE_H
#define GFX_VSYNCSOURCE_H

#include "nsTArray.h"
#include "nsRefPtr.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsISupportsImpl.h"

namespace mozilla {
class RefreshTimerVsyncDispatcher;
class CompositorVsyncDispatcher;

namespace gfx {



class VsyncSource
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(VsyncSource)

  typedef mozilla::RefreshTimerVsyncDispatcher RefreshTimerVsyncDispatcher;
  typedef mozilla::CompositorVsyncDispatcher CompositorVsyncDispatcher;

public:
  
  class Display {
    public:
      Display();
      virtual ~Display();

      
      
      
      
      
      
      
      
      
      virtual void NotifyVsync(TimeStamp aVsyncTimestamp);

      nsRefPtr<RefreshTimerVsyncDispatcher> GetRefreshTimerVsyncDispatcher();

      void AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
      void NotifyRefreshTimerVsyncStatus(bool aEnable);

      
      virtual void EnableVsync() = 0;
      virtual void DisableVsync() = 0;
      virtual bool IsVsyncEnabled() = 0;

    private:
      void UpdateVsyncStatus();

      Mutex mDispatcherLock;
      bool mRefreshTimerNeedsVsync;
      nsTArray<nsRefPtr<CompositorVsyncDispatcher>> mCompositorVsyncDispatchers;
      nsRefPtr<RefreshTimerVsyncDispatcher> mRefreshTimerVsyncDispatcher;
  };

  void AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);
  void RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher);

  nsRefPtr<RefreshTimerVsyncDispatcher> GetRefreshTimerVsyncDispatcher();
  virtual Display& GetGlobalDisplay() = 0; 

protected:
  virtual ~VsyncSource() {}
};

} 
} 

#endif 
