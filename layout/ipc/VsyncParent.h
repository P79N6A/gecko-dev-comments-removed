




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




class VsyncParent MOZ_FINAL : public PVsyncParent,
                              public VsyncObserver
{
  friend class mozilla::ipc::BackgroundParentImpl;

private:
  static already_AddRefed<VsyncParent> Create();

  VsyncParent();
  virtual ~VsyncParent();

  virtual bool NotifyVsync(TimeStamp aTimeStamp) MOZ_OVERRIDE;

  virtual bool RecvObserve() MOZ_OVERRIDE;
  virtual bool RecvUnobserve() MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aActorDestroyReason) MOZ_OVERRIDE;

  void DispatchVsyncEvent(TimeStamp aTimeStamp);

  bool mObservingVsync;
  bool mDestroyed;
  nsCOMPtr<nsIThread> mBackgroundThread;
  nsRefPtr<RefreshTimerVsyncDispatcher> mVsyncDispatcher;
};

} 
} 

#endif  
