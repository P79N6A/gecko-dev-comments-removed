





#include "mozilla/AutoTimelineMarker.h"

#include "mozilla/TimelineConsumers.h"
#include "MainThreadUtils.h"
#include "nsDocShell.h"

namespace mozilla {

AutoTimelineMarker::AutoTimelineMarker(nsIDocShell* aDocShell, const char* aName
                                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mName(aName)
  , mDocShell(nullptr)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aDocShell);

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  mDocShell = static_cast<nsDocShell*>(aDocShell);
  TimelineConsumers::AddMarkerForDocShell(mDocShell, mName, TRACING_INTERVAL_START);
}

AutoTimelineMarker::~AutoTimelineMarker()
{
  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  TimelineConsumers::AddMarkerForDocShell(mDocShell, mName, TRACING_INTERVAL_END);
}

} 
