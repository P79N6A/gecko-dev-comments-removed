




#include "VsyncSource.h"
#include "gfxPlatform.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/VsyncDispatcher.h"

using namespace mozilla;
using namespace mozilla::gfx;

void
VsyncSource::AddVsyncDispatcher(VsyncDispatcher* aVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetGlobalDisplay().AddVsyncDispatcher(aVsyncDispatcher);
}

void
VsyncSource::RemoveVsyncDispatcher(VsyncDispatcher* aVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetGlobalDisplay().RemoveVsyncDispatcher(aVsyncDispatcher);
}

VsyncSource::Display&
VsyncSource::FindDisplay(VsyncDispatcher* aVsyncDispatcher)
{
  return GetGlobalDisplay();
}

void
VsyncSource::Display::NotifyVsync(TimeStamp aVsyncTimestamp)
{
  
  for (size_t i = 0; i < mVsyncDispatchers.Length(); i++) {
    mVsyncDispatchers[i]->NotifyVsync(aVsyncTimestamp);
  }
}

VsyncSource::Display::Display()
{
  MOZ_ASSERT(NS_IsMainThread());
}

VsyncSource::Display::~Display()
{
  MOZ_ASSERT(NS_IsMainThread());
  mVsyncDispatchers.Clear();
}

void
VsyncSource::Display::AddVsyncDispatcher(VsyncDispatcher* aVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  mVsyncDispatchers.AppendElement(aVsyncDispatcher);
}

void
VsyncSource::Display::RemoveVsyncDispatcher(VsyncDispatcher* aVsyncDispatcher)
{
  MOZ_ASSERT(NS_IsMainThread());
  mVsyncDispatchers.RemoveElement(aVsyncDispatcher);
}
