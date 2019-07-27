





#ifndef AutoTimelineMarker_h__
#define AutoTimelineMarker_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/Move.h"
#include "nsDocShell.h"
#include "nsRefPtr.h"

namespace mozilla {















class MOZ_STACK_CLASS AutoTimelineMarker
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;

  nsRefPtr<nsDocShell> mDocShell;
  const char* mName;

  bool
  DocShellIsRecording(nsDocShell& aDocShell)
  {
    bool isRecording = false;
    if (nsDocShell::gProfileTimelineRecordingsCount > 0) {
      aDocShell.GetRecordProfileTimelineMarkers(&isRecording);
    }
    return isRecording;
  }

public:
  explicit AutoTimelineMarker(nsIDocShell* aDocShell, const char* aName
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mDocShell(nullptr)
    , mName(aName)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    nsDocShell* docShell = static_cast<nsDocShell*>(aDocShell);
    if (docShell && DocShellIsRecording(*docShell)) {
      mDocShell = docShell;
      mDocShell->AddProfileTimelineMarker(mName, TRACING_INTERVAL_START);
    }
  }

  ~AutoTimelineMarker()
  {
    if (mDocShell) {
      mDocShell->AddProfileTimelineMarker(mName, TRACING_INTERVAL_END);
    }
  }
};

} 

#endif 
