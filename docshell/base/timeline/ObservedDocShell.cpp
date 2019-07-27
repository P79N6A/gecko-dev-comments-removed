





#include "ObservedDocShell.h"

#include "TimelineMarker.h"
#include "mozilla/Move.h"

namespace mozilla {

ObservedDocShell::ObservedDocShell(nsDocShell* aDocShell)
  : mDocShell(aDocShell)
{}

void
ObservedDocShell::AddMarker(const char* aName, TracingMetadata aMetaData)
{
  TimelineMarker* marker = new TimelineMarker(mDocShell, aName, aMetaData);
  mTimelineMarkers.AppendElement(marker);
}

void
ObservedDocShell::AddMarker(UniquePtr<TimelineMarker>&& aMarker)
{
  mTimelineMarkers.AppendElement(Move(aMarker));
}

void
ObservedDocShell::ClearMarkers()
{
  mTimelineMarkers.Clear();
}

} 
