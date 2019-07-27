





#include "nsDocShell.h"
#include "TimelineMarker.h"

TimelineMarker::TimelineMarker(nsDocShell* aDocShell, const char* aName,
                               TracingMetadata aMetaData)
  : mName(aName)
  , mMetaData(aMetaData)
{
  MOZ_COUNT_CTOR(TimelineMarker);
  MOZ_ASSERT(aName);
  aDocShell->Now(&mTime);
  if (aMetaData == TRACING_INTERVAL_START || aMetaData == TRACING_TIMESTAMP) {
    CaptureStack();
  }
}

TimelineMarker::TimelineMarker(nsDocShell* aDocShell, const char* aName,
                               TracingMetadata aMetaData,
                               const nsAString& aCause,
                               TimelineStackRequest aStackRequest)
  : mName(aName)
  , mMetaData(aMetaData)
  , mCause(aCause)
{
  MOZ_COUNT_CTOR(TimelineMarker);
  MOZ_ASSERT(aName);
  aDocShell->Now(&mTime);
  if ((aMetaData == TRACING_INTERVAL_START ||
      aMetaData == TRACING_TIMESTAMP) &&
      aStackRequest != NO_STACK) {
    CaptureStack();
  }
}

TimelineMarker::~TimelineMarker()
{
  MOZ_COUNT_DTOR(TimelineMarker);
}
