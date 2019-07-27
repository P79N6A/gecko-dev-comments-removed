




#include "MainThreadUtils.h"
#include "VsyncDispatcher.h"
#include "VsyncSource.h"
#include "gfxPlatform.h"
#include "mozilla/layers/Compositor.h"
#include "mozilla/layers/CompositorParent.h"

#ifdef MOZ_ENABLE_PROFILER_SPS
#include "GeckoProfiler.h"
#include "ProfilerMarkers.h"
#endif

namespace mozilla {
static bool sThreadAssertionsEnabled = true;

void CompositorVsyncDispatcher::SetThreadAssertionsEnabled(bool aEnable)
{
  
  MOZ_ASSERT(NS_IsMainThread());
  sThreadAssertionsEnabled = aEnable;
}

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
  
}

void
CompositorVsyncDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
#ifdef MOZ_ENABLE_PROFILER_SPS
  layers::CompositorParent::PostInsertVsyncProfilerMarker(aVsyncTimestamp);
#endif

  MutexAutoLock lock(mCompositorObserverLock);
  if (mCompositorVsyncObserver) {
    mCompositorVsyncObserver->NotifyVsync(aVsyncTimestamp);
  }
}

void
CompositorVsyncDispatcher::AssertOnCompositorThread()
{
  if (!sThreadAssertionsEnabled) {
    return;
  }

  Compositor::AssertOnCompositorThread();
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
  AssertOnCompositorThread();
  { 
    MutexAutoLock lock(mCompositorObserverLock);
    mCompositorVsyncObserver = aVsyncObserver;
  }

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
  MOZ_ASSERT(NS_IsMainThread());
  { 
    MutexAutoLock lock(mRefreshTimersLock);
    mParentRefreshTimer = aVsyncObserver;
  }

  UpdateVsyncStatus();
}

void
RefreshTimerVsyncDispatcher::AddChildRefreshTimer(VsyncObserver* aVsyncObserver)
{
  { 
    MutexAutoLock lock(mRefreshTimersLock);
    MOZ_ASSERT(aVsyncObserver);
    if (!mChildRefreshTimers.Contains(aVsyncObserver)) {
      mChildRefreshTimers.AppendElement(aVsyncObserver);
    }
  }

  UpdateVsyncStatus();
}

void
RefreshTimerVsyncDispatcher::RemoveChildRefreshTimer(VsyncObserver* aVsyncObserver)
{
  { 
    MutexAutoLock lock(mRefreshTimersLock);
    MOZ_ASSERT(aVsyncObserver);
    mChildRefreshTimers.RemoveElement(aVsyncObserver);
  }

  UpdateVsyncStatus();
}

void
RefreshTimerVsyncDispatcher::UpdateVsyncStatus()
{
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> vsyncControl = NS_NewRunnableMethod(this,
                                           &RefreshTimerVsyncDispatcher::UpdateVsyncStatus);
    NS_DispatchToMainThread(vsyncControl);
    return;
  }

  gfx::VsyncSource::Display& display = gfxPlatform::GetPlatform()->GetHardwareVsync()->GetGlobalDisplay();
  display.NotifyRefreshTimerVsyncStatus(NeedsVsync());
}

bool
RefreshTimerVsyncDispatcher::NeedsVsync()
{
  MOZ_ASSERT(NS_IsMainThread());
  MutexAutoLock lock(mRefreshTimersLock);
  return (mParentRefreshTimer != nullptr) || !mChildRefreshTimers.IsEmpty();
}

} 
