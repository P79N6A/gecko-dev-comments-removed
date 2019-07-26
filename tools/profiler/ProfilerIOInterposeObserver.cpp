



#include "GeckoProfiler.h"
#include "ProfilerIOInterposeObserver.h"
#include "ProfilerMarkers.h"

using namespace mozilla;

void ProfilerIOInterposeObserver::Observe(Observation& aObservation)
{
  const char* str = nullptr;

  switch (aObservation.ObservedOperation()) {
    case IOInterposeObserver::OpCreateOrOpen:
      str = "create/open";
      break;
    case IOInterposeObserver::OpRead:
      str = "read";
      break;
    case IOInterposeObserver::OpWrite:
      str = "write";
      break;
    case IOInterposeObserver::OpFSync:
      str = "fsync";
      break;
    case IOInterposeObserver::OpStat:
      str = "stat";
      break;
    case IOInterposeObserver::OpClose:
      str = "close";
      break;
    default:
      return;
  }
  ProfilerBacktrace* stack = profiler_get_backtrace();
  IOMarkerPayload* markerPayload = new IOMarkerPayload(aObservation.Reference(),
                                                       aObservation.Start(),
                                                       aObservation.End(),
                                                       stack);
  PROFILER_MARKER_PAYLOAD(str, markerPayload);
}
