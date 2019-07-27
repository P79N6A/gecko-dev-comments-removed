




#ifndef mozilla_layout_ipc_VsyncParent_h
#define mozilla_layout_ipc_VsyncParent_h

#include "mozilla/layout/PVsyncParent.h"
#include "mozilla/VsyncDispatcher.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"

class nsIThread;

namespace mozilla {

namespace ipc {
class BackgroundParentImpl;
} 

namespace layout {




class VsyncParent final : public PVsyncParent,
                          public VsyncObserver
{
  friend class mozilla::ipc::BackgroundParentImpl;

private:
  static already_AddRefed<VsyncParent> Create();

  VsyncParent();
  virtual ~VsyncParent();

  virtual bool NotifyVsync(TimeStamp aTimeStamp) override;

  virtual bool RecvObserve() override;
  virtual bool RecvUnobserve() override;
  virtual void ActorDestroy(ActorDestroyReason aActorDestroyReason) override;

  void DispatchVsyncEvent(TimeStamp aTimeStamp);

  bool mObservingVsync;
  bool mDestroyed;
  nsCOMPtr<nsIThread> mBackgroundThread;
  nsRefPtr<RefreshTimerVsyncDispatcher> mVsyncDispatcher;
};

} 
} 

#endif  
