





#include "mozilla/AutoGlobalTimelineMarker.h"

#include "mozilla/TimelineConsumers.h"
#include "MainThreadUtils.h"

namespace mozilla {

AutoGlobalTimelineMarker::AutoGlobalTimelineMarker(const char* aName
                                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mName(aName)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  TimelineConsumers::AddMarkerToAllObservedDocShells(mName, TRACING_INTERVAL_START);
}

AutoGlobalTimelineMarker::~AutoGlobalTimelineMarker()
{
  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  TimelineConsumers::AddMarkerToAllObservedDocShells(mName, TRACING_INTERVAL_END);
}

} 
