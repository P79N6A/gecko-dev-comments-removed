




#include "VsyncSource.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "mozilla/VsyncDispatcher.h"
#include "MainThreadUtils.h"

namespace mozilla {
namespace gfx {

void
VsyncSource::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  
  
  GetGlobalDisplay().AddCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

void
VsyncSource::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  
  GetGlobalDisplay().RemoveCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

nsRefPtr<RefreshTimerVsyncDispatcher>
VsyncSource::GetRefreshTimerVsyncDispatcher()
{
  MOZ_ASSERT(XRE_IsParentProcess());
  
  return GetGlobalDisplay().GetRefreshTimerVsyncDispatcher();
}

VsyncSource::Display::Display()
  : mDispatcherLock("display dispatcher lock")
  , mRefreshTimerNeedsVsync(false)
{
  MOZ_ASSERT(NS_IsMainThread());
  mRefreshTimerVsyncDispatcher = new RefreshTimerVsyncDispatcher();
}

VsyncSource::Display::~Display()
{
  MOZ_ASSERT(NS_IsMainThread());
  MutexAutoLock lock(mDispatcherLock);
  mRefreshTimerVsyncDispatcher = nullptr;
  mCompositorVsyncDispatchers.Clear();
}

void
VsyncSource::Display::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
  MutexAutoLock lock(mDispatcherLock);

  for (size_t i = 0; i < mCompositorVsyncDispatchers.Length(); i++) {
    mCompositorVsyncDispatchers[i]->NotifyVsync(aVsyncTimestamp);
  }

  mRefreshTimerVsyncDispatcher->NotifyVsync(aVsyncTimestamp);
}

void
VsyncSource::Display::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aCompositorVsyncDispatcher);
  { 
    MutexAutoLock lock(mDispatcherLock);
    if (!mCompositorVsyncDispatchers.Contains(aCompositorVsyncDispatcher)) {
      mCompositorVsyncDispatchers.AppendElement(aCompositorVsyncDispatcher);
    }
  }
  UpdateVsyncStatus();
}

void
VsyncSource::Display::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aCompositorVsyncDispatcher);
  { 
    MutexAutoLock lock(mDispatcherLock);
    if (mCompositorVsyncDispatchers.Contains(aCompositorVsyncDispatcher)) {
      mCompositorVsyncDispatchers.RemoveElement(aCompositorVsyncDispatcher);
    }
  }
  UpdateVsyncStatus();
}

void
VsyncSource::Display::NotifyRefreshTimerVsyncStatus(bool aEnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  mRefreshTimerNeedsVsync = aEnable;
  UpdateVsyncStatus();
}

void
VsyncSource::Display::UpdateVsyncStatus()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  
  
  bool enableVsync = false;
  { 
    MutexAutoLock lock(mDispatcherLock);
    enableVsync = !mCompositorVsyncDispatchers.IsEmpty() || mRefreshTimerNeedsVsync;
  }

  if (enableVsync) {
    EnableVsync();
  } else {
    DisableVsync();
  }

  if (IsVsyncEnabled() != enableVsync) {
    NS_WARNING("Vsync status did not change.");
  }
}

nsRefPtr<RefreshTimerVsyncDispatcher>
VsyncSource::Display::GetRefreshTimerVsyncDispatcher()
{
  return mRefreshTimerVsyncDispatcher;
}

} 
} 
