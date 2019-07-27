




#include "VsyncDispatcher.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/layers/CompositorParent.h"
#include "gfxPrefs.h"
#include "gfxPlatform.h"
#include "VsyncSource.h"

#ifdef MOZ_ENABLE_PROFILER_SPS
#include "GeckoProfiler.h"
#include "ProfilerMarkers.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "GeckoTouchDispatcher.h"
#endif

using namespace mozilla::layers;

namespace mozilla {

CompositorVsyncDispatcher::CompositorVsyncDispatcher()
  : mCompositorObserverLock("CompositorObserverLock")
{
  MOZ_ASSERT(XRE_IsParentProcess());
  gfxPlatform::GetPlatform()->GetHardwareVsync()->AddCompositorVsyncDispatcher(this);
}

CompositorVsyncDispatcher::~CompositorVsyncDispatcher()
{
  
  MOZ_ASSERT(NS_IsMainThread());
}

void
CompositorVsyncDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
#ifdef MOZ_ENABLE_PROFILER_SPS
    if (profiler_is_active()) {
        CompositorParent::PostInsertVsyncProfilerMarker(aVsyncTimestamp);
    }
#endif

  MutexAutoLock lock(mCompositorObserverLock);
  if (gfxPrefs::VsyncAlignedCompositor() && mCompositorVsyncObserver) {
    mCompositorVsyncObserver->NotifyVsync(aVsyncTimestamp);
  }
}

void
CompositorVsyncDispatcher::SetCompositorVsyncObserver(VsyncObserver* aVsyncObserver)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MutexAutoLock lock(mCompositorObserverLock);
  mCompositorVsyncObserver = aVsyncObserver;
}

void
CompositorVsyncDispatcher::Shutdown()
{
  
  
  
  MOZ_ASSERT(XRE_IsParentProcess());
  MOZ_ASSERT(NS_IsMainThread());
  gfxPlatform::GetPlatform()->GetHardwareVsync()->RemoveCompositorVsyncDispatcher(this);
}
} 
