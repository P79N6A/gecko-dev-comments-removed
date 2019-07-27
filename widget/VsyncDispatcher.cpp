




#include "MainThreadUtils.h"
#include "VsyncDispatcher.h"
#include "VsyncSource.h"
#include "gfxPlatform.h"
#include "mozilla/layers/CompositorParent.h"

#ifdef MOZ_ENABLE_PROFILER_SPS
#include "GeckoProfiler.h"
#include "ProfilerMarkers.h"
#endif

namespace mozilla {

CompositorVsyncDispatcher::CompositorVsyncDispatcher()
  : mCompositorObserverLock("CompositorObserverLock")
  , mDidShutdown(false)
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
}

CompositorVsyncDispatcher::~CompositorVsyncDispatcher()
{
  MOZ_ASSERT(XRE_IsParentProcess());
  
  MOZ_ASSERT(NS_IsMainThread());
}

void
CompositorVsyncDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
#ifdef MOZ_ENABLE_PROFILER_SPS
    if (profiler_is_active()) {
        layers::CompositorParent::PostInsertVsyncProfilerMarker(aVsyncTimestamp);
    }
#endif

  MutexAutoLock lock(mCompositorObserverLock);
  if (mCompositorVsyncObserver) {
    mCompositorVsyncObserver->NotifyVsync(aVsyncTimestamp);
  }
}

void
CompositorVsyncDispatcher::ObserveVsync(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_IsParentProcess());
  if (mDidShutdown) {
    return;
  }

  if (aEnable) {
    gfxPlatform::GetPlatform()->GetHardwareVsync()->AddCompositorVsyncDispatcher(this);
  } else {
    gfxPlatform::GetPlatform()->GetHardwareVsync()->RemoveCompositorVsyncDispatcher(this);
  }
}

void
CompositorVsyncDispatcher::SetCompositorVsyncObserver(VsyncObserver* aVsyncObserver)
{
  MOZ_ASSERT(layers::CompositorParent::IsInCompositorThread());
  MutexAutoLock lock(mCompositorObserverLock);
  mCompositorVsyncObserver = aVsyncObserver;

  bool observeVsync = aVsyncObserver != nullptr;
  nsCOMPtr<nsIRunnable> vsyncControl = NS_NewRunnableMethodWithArg<bool>(this,
                                        &CompositorVsyncDispatcher::ObserveVsync,
                                        observeVsync);
  NS_DispatchToMainThread(vsyncControl);
}

void
CompositorVsyncDispatcher::Shutdown()
{
  
  
  
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  ObserveVsync(false);
  mDidShutdown = true;
  { 
    MutexAutoLock lock(mCompositorObserverLock);
    mCompositorVsyncObserver = nullptr;
  }
}

RefreshTimerVsyncDispatcher::RefreshTimerVsyncDispatcher()
  : mRefreshTimersLock("RefreshTimers lock")
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
}

RefreshTimerVsyncDispatcher::~RefreshTimerVsyncDispatcher()
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
}

void
RefreshTimerVsyncDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  MutexAutoLock lock(mRefreshTimersLock);

  for (size_t i = 0; i < mChildRefreshTimers.Length(); i++) {
    mChildRefreshTimers[i]->NotifyVsync(aVsyncTimestamp);
  }

  if (mParentRefreshTimer) {
    mParentRefreshTimer->NotifyVsync(aVsyncTimestamp);
  }
}

void
RefreshTimerVsyncDispatcher::SetParentRefreshTimer(VsyncObserver* aVsyncObserver)
{
  MutexAutoLock lock(mRefreshTimersLock);
  mParentRefreshTimer = aVsyncObserver;
}

void
RefreshTimerVsyncDispatcher::AddChildRefreshTimer(VsyncObserver* aVsyncObserver)
{
  MutexAutoLock lock(mRefreshTimersLock);
  MOZ_ASSERT(aVsyncObserver);
  if (!mChildRefreshTimers.Contains(aVsyncObserver)) {
    mChildRefreshTimers.AppendElement(aVsyncObserver);
  }
}

void
RefreshTimerVsyncDispatcher::RemoveChildRefreshTimer(VsyncObserver* aVsyncObserver)
{
  MutexAutoLock lock(mRefreshTimersLock);
  MOZ_ASSERT(aVsyncObserver);
  mChildRefreshTimers.RemoveElement(aVsyncObserver);
}

} 
