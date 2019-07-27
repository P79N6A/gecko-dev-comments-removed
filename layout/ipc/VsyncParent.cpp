




#include "VsyncParent.h"

#include "BackgroundParent.h"
#include "BackgroundParentImpl.h"
#include "gfxPlatform.h"
#include "mozilla/unused.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "VsyncSource.h"

namespace mozilla {

using namespace ipc;

namespace layout {

 already_AddRefed<VsyncParent>
VsyncParent::Create()
{
  AssertIsOnBackgroundThread();
  nsRefPtr<gfx::VsyncSource> vsyncSource = gfxPlatform::GetPlatform()->GetHardwareVsync();
  nsRefPtr<VsyncParent> vsyncParent = new VsyncParent();
  vsyncParent->mVsyncDispatcher = vsyncSource->GetRefreshTimerVsyncDispatcher();
  return vsyncParent.forget();
}

VsyncParent::VsyncParent()
  : mObservingVsync(false)
  , mDestroyed(false)
  , mBackgroundThread(NS_GetCurrentThread())
{
  MOZ_ASSERT(mBackgroundThread);
  AssertIsOnBackgroundThread();
}

VsyncParent::~VsyncParent()
{
  
  
}

bool
VsyncParent::NotifyVsync(TimeStamp aTimeStamp)
{
  
  MOZ_ASSERT(!IsOnBackgroundThread());
  nsCOMPtr<nsIRunnable> vsyncEvent =
    NS_NewRunnableMethodWithArg<TimeStamp>(this,
                                           &VsyncParent::DispatchVsyncEvent,
                                           aTimeStamp);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mBackgroundThread->Dispatch(vsyncEvent, NS_DISPATCH_NORMAL)));
  return true;
}

void
VsyncParent::DispatchVsyncEvent(TimeStamp aTimeStamp)
{
  AssertIsOnBackgroundThread();

  
  
  
  
  
  if (mObservingVsync && !mDestroyed) {
    unused << SendNotify(aTimeStamp);
  }
}

bool
VsyncParent::RecvObserve()
{
  AssertIsOnBackgroundThread();
  if (!mObservingVsync) {
    mVsyncDispatcher->AddChildRefreshTimer(this);
    mObservingVsync = true;
    return true;
  }
  return false;
}

bool
VsyncParent::RecvUnobserve()
{
  AssertIsOnBackgroundThread();
  if (mObservingVsync) {
    mVsyncDispatcher->RemoveChildRefreshTimer(this);
    mObservingVsync = false;
    return true;
  }
  return false;
}

void
VsyncParent::ActorDestroy(ActorDestroyReason aReason)
{
  MOZ_ASSERT(!mDestroyed);
  AssertIsOnBackgroundThread();
  if (mObservingVsync) {
    mVsyncDispatcher->RemoveChildRefreshTimer(this);
  }
  mVsyncDispatcher = nullptr;
  mDestroyed = true;
}

} 
} 
