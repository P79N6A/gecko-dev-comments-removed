





#include "mozilla/AutoTimelineMarker.h"

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

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  bool isRecordingEnabledForDocShell = false;
  nsDocShell* docShell = static_cast<nsDocShell*>(aDocShell);
  aDocShell->GetRecordProfileTimelineMarkers(&isRecordingEnabledForDocShell);

  if (isRecordingEnabledForDocShell) {
    mDocShell = docShell;
    mDocShell->AddProfileTimelineMarker(mName, TRACING_INTERVAL_START);
  }
}

AutoTimelineMarker::~AutoTimelineMarker()
{
  if (mDocShell) {
    mDocShell->AddProfileTimelineMarker(mName, TRACING_INTERVAL_END);
  }
}

} 
