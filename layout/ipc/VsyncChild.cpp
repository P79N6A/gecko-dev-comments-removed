




#include "VsyncChild.h"

#include "mozilla/VsyncDispatcher.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace layout {

VsyncChild::VsyncChild()
  : mObservingVsync(false)
{
  MOZ_ASSERT(NS_IsMainThread());
}

VsyncChild::~VsyncChild()
{
  MOZ_ASSERT(NS_IsMainThread());
}

bool
VsyncChild::SendObserve()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mObservingVsync) {
    PVsyncChild::SendObserve();
    mObservingVsync = true;
  }
  return true;
}

bool
VsyncChild::SendUnobserve()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mObservingVsync) {
    PVsyncChild::SendUnobserve();
    mObservingVsync = false;
  }
  return true;
}

void
VsyncChild::ActorDestroy(ActorDestroyReason aActorDestroyReason)
{
  MOZ_ASSERT(NS_IsMainThread());
  mObserver = nullptr;
}

bool
VsyncChild::RecvNotify(const TimeStamp& aVsyncTimestamp)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mObservingVsync && mObserver) {
    mObserver->NotifyVsync(aVsyncTimestamp);
  }
  return true;
}

void
VsyncChild::SetVsyncObserver(VsyncObserver* aVsyncObserver)
{
  MOZ_ASSERT(NS_IsMainThread());
  mObserver = aVsyncObserver;
}

} 
} 
