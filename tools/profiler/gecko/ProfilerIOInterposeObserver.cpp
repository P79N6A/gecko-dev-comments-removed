



#include "GeckoProfiler.h"
#include "ProfilerIOInterposeObserver.h"
#include "ProfilerMarkers.h"

using namespace mozilla;

void ProfilerIOInterposeObserver::Observe(Observation& aObservation)
{
  if (!IsMainThread()) {
    return;
  }

  ProfilerBacktrace* stack = profiler_get_backtrace();

  nsCString filename;
  if (aObservation.Filename()) {
    filename = NS_ConvertUTF16toUTF8(aObservation.Filename());
  }

  IOMarkerPayload* markerPayload = new IOMarkerPayload(aObservation.Reference(),
                                                       filename.get(),
                                                       aObservation.Start(),
                                                       aObservation.End(),
                                                       stack);
  PROFILER_MARKER_PAYLOAD(aObservation.ObservedOperationString(), markerPayload);
}
