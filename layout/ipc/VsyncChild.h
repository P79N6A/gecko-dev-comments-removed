




#ifndef mozilla_layout_ipc_VsyncChild_h
#define mozilla_layout_ipc_VsyncChild_h

#include "mozilla/layout/PVsyncChild.h"
#include "nsRefPtr.h"

namespace mozilla {

class VsyncObserver;

namespace ipc {
class BackgroundChildImpl;
}

namespace layout {





class VsyncChild MOZ_FINAL : public PVsyncChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  
  
  bool SendObserve();
  bool SendUnobserve();

  
  void SetVsyncObserver(VsyncObserver* aVsyncObserver);

private:
  VsyncChild();
  virtual ~VsyncChild();

  virtual bool RecvNotify(const TimeStamp& aVsyncTimestamp) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aActorDestroyReason) MOZ_OVERRIDE;

  bool mObservingVsync;

  
  nsRefPtr<VsyncObserver> mObserver;
};

} 
} 

#endif  
