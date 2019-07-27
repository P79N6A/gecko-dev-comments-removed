





#include "mozilla/AutoGlobalTimelineMarker.h"

#include "mozilla/TimelineConsumers.h"
#include "MainThreadUtils.h"
#include "nsDocShell.h"

namespace mozilla {

void
AutoGlobalTimelineMarker::PopulateDocShells()
{
  const LinkedList<nsDocShell::ObservedDocShell>& docShells =
    nsDocShell::GetObservedDocShells();
  MOZ_ASSERT(!docShells.isEmpty());

  for (const nsDocShell::ObservedDocShell* ds = docShells.getFirst();
       ds;
       ds = ds->getNext()) {
    mOk = mDocShells.append(**ds);
    if (!mOk) {
      return;
    }
  }
}

AutoGlobalTimelineMarker::AutoGlobalTimelineMarker(const char* aName
                                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mOk(true)
  , mDocShells()
  , mName(aName)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  PopulateDocShells();
  if (!mOk) {
    
    
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
  if (!mOk) {
    return;
  }

  for (Vector<nsRefPtr<nsDocShell>>::Range range = mDocShells.all();
       !range.empty();
       range.popFront()) {
    range.front()->AddProfileTimelineMarker(mName, TRACING_INTERVAL_END);
  }
}

} 
