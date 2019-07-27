




#include "VsyncSource.h"
#include "gfxPlatform.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/VsyncDispatcher.h"

using namespace mozilla;
using namespace mozilla::gfx;

void
VsyncSource::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetGlobalDisplay().AddCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

void
VsyncSource::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetGlobalDisplay().RemoveCompositorVsyncDispatcher(aCompositorVsyncDispatcher);
}

VsyncSource::Display&
VsyncSource::FindDisplay(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  return GetGlobalDisplay();
}

void
VsyncSource::Display::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
  for (size_t i = 0; i < mCompositorVsyncDispatchers.Length(); i++) {
    mCompositorVsyncDispatchers[i]->NotifyVsync(aVsyncTimestamp);
  }
}

VsyncSource::Display::Display()
{
  MOZ_ASSERT(NS_IsMainThread());
}

VsyncSource::Display::~Display()
{
  MOZ_ASSERT(NS_IsMainThread());
  mCompositorVsyncDispatchers.Clear();
}

void
VsyncSource::Display::AddCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  mCompositorVsyncDispatchers.AppendElement(aCompositorVsyncDispatcher);
}

void
VsyncSource::Display::RemoveCompositorVsyncDispatcher(CompositorVsyncDispatcher* aCompositorVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  mCompositorVsyncDispatchers.RemoveElement(aCompositorVsyncDispatcher);
}
