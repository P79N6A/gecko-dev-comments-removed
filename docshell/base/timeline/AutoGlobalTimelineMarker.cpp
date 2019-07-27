





#include "mozilla/AutoGlobalTimelineMarker.h"

#include "mozilla/TimelineConsumers.h"
#include "MainThreadUtils.h"
#include "nsDocShell.h"

namespace mozilla {

AutoGlobalTimelineMarker::AutoGlobalTimelineMarker(const char* aName
                                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mName(aName)
  , mDocShells()
  , mDocShellsRetrieved(false)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  mDocShellsRetrieved = TimelineConsumers::GetKnownDocShells(mDocShells);
  if (!mDocShellsRetrieved) {
    
    
    return;
  }

  for (Vector<nsRefPtr<nsDocShell>>::Range range = mDocShells.all();
       !range.empty();
       range.popFront()) {
    range.front()->AddProfileTimelineMarker(mName, TRACING_INTERVAL_START);
  }
}

AutoGlobalTimelineMarker::~AutoGlobalTimelineMarker()
{
  if (!mDocShellsRetrieved) {
    return;
  }

  for (Vector<nsRefPtr<nsDocShell>>::Range range = mDocShells.all();
       !range.empty();
       range.popFront()) {
    range.front()->AddProfileTimelineMarker(mName, TRACING_INTERVAL_END);
  }
}

} 
