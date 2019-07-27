




#include "VsyncDispatcher.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/layers/CompositorParent.h"
#include "gfxPrefs.h"

#ifdef MOZ_WIDGET_GONK
#include "GeckoTouchDispatcher.h"
#endif

using namespace mozilla::layers;

namespace mozilla {

StaticRefPtr<VsyncDispatcher> sVsyncDispatcher;

 VsyncDispatcher*
VsyncDispatcher::GetInstance()
{
  if (!sVsyncDispatcher) {
    sVsyncDispatcher = new VsyncDispatcher();
    ClearOnShutdown(&sVsyncDispatcher);
  }

  return sVsyncDispatcher;
}

VsyncDispatcher::VsyncDispatcher()
  : mCompositorObserverLock("CompositorObserverLock")
{

}

VsyncDispatcher::~VsyncDispatcher()
{
  MutexAutoLock lock(mCompositorObserverLock);
  mCompositorObservers.Clear();
}

void
VsyncDispatcher::DispatchTouchEvents(bool aNotifiedCompositors, nsecs_t aAndroidVsyncTime)
{
  
  
#ifdef MOZ_WIDGET_GONK
  if (!aNotifiedCompositors && gfxPrefs::TouchResampling()) {
    GeckoTouchDispatcher::NotifyVsync(aAndroidVsyncTime);
  }
#endif
}

void
VsyncDispatcher::NotifyVsync(TimeStamp aVsyncTimestamp, nsecs_t aAndroidVsyncTime)
{
  bool notifiedCompositors = false;
  if (gfxPrefs::VsyncAlignedCompositor()) {
    MutexAutoLock lock(mCompositorObserverLock);
    notifiedCompositors = NotifyVsyncObservers(aVsyncTimestamp, mCompositorObservers);
  }

  DispatchTouchEvents(notifiedCompositors, aAndroidVsyncTime);
}

bool
VsyncDispatcher::NotifyVsyncObservers(TimeStamp aVsyncTimestamp, nsTArray<nsRefPtr<VsyncObserver>>& aObservers)
{
  
  for (size_t i = 0; i < aObservers.Length(); i++) {
    aObservers[i]->NotifyVsync(aVsyncTimestamp);
 }
 return aObservers.IsEmpty();
}

void
VsyncDispatcher::AddCompositorVsyncObserver(VsyncObserver* aVsyncObserver)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MutexAutoLock lock(mCompositorObserverLock);
  if (!mCompositorObservers.Contains(aVsyncObserver)) {
    mCompositorObservers.AppendElement(aVsyncObserver);
  }
}

void
VsyncDispatcher::RemoveCompositorVsyncObserver(VsyncObserver* aVsyncObserver)
{
  MOZ_ASSERT(CompositorParent::IsInCompositorThread());
  MutexAutoLock lock(mCompositorObserverLock);
  if (mCompositorObservers.Contains(aVsyncObserver)) {
    mCompositorObservers.RemoveElement(aVsyncObserver);
  } else {
    NS_WARNING("Could not delete a compositor vsync observer\n");
  }
}

} 
