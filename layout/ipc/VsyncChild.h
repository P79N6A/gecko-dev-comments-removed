




#ifndef mozilla_layout_ipc_VsyncChild_h
#define mozilla_layout_ipc_VsyncChild_h

#include "mozilla/layout/PVsyncChild.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"

namespace mozilla {

class VsyncObserver;

namespace ipc {
class BackgroundChildImpl;
} 

namespace layout {





class VsyncChild final : public PVsyncChild
{
  NS_INLINE_DECL_REFCOUNTING(VsyncChild)

  friend class mozilla::ipc::BackgroundChildImpl;

public:
  
  
  bool SendObserve();
  bool SendUnobserve();

  
  void SetVsyncObserver(VsyncObserver* aVsyncObserver);

private:
  VsyncChild();
  virtual ~VsyncChild();

  virtual bool RecvNotify(const TimeStamp& aVsyncTimestamp) override;
  virtual void ActorDestroy(ActorDestroyReason aActorDestroyReason) override;

  bool mObservingVsync;
  bool mIsShutdown;

  
  nsRefPtr<VsyncObserver> mObserver;
};

} 
} 

#endif  
