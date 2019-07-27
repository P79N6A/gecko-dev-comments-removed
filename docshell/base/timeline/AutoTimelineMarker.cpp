





#include "mozilla/AutoTimelineMarker.h"

#include "MainThreadUtils.h"
#include "nsDocShell.h"

namespace mozilla {

bool
AutoTimelineMarker::DocShellIsRecording(nsDocShell& aDocShell)
{
  bool isRecording = false;
  if (!TimelineConsumers::IsEmpty()) {
    aDocShell.GetRecordProfileTimelineMarkers(&isRecording);
  }
  return isRecording;
}

AutoTimelineMarker::AutoTimelineMarker(nsIDocShell* aDocShell, const char* aName
                                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mDocShell(nullptr)
  , mName(aName)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());

  nsDocShell* docShell = static_cast<nsDocShell*>(aDocShell);
  if (docShell && DocShellIsRecording(*docShell)) {
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
