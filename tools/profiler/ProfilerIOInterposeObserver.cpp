



#include "GeckoProfiler.h"
#include "ProfilerIOInterposeObserver.h"
#include "ProfilerMarkers.h"

using namespace mozilla;

void ProfilerIOInterposeObserver::Observe(Observation& aObservation)
{
  
  
  if (NS_IsMainThread()) {
    const char* ops[] = {"none", "read", "write", "invalid", "fsync"};
    ProfilerBacktrace* stack = profiler_get_backtrace();
    IOMarkerPayload* markerPayload = new IOMarkerPayload(aObservation.Reference(),
                                                         aObservation.Start(),
                                                         aObservation.End(),
                                                         stack);
    PROFILER_MARKER_PAYLOAD(ops[aObservation.ObservedOperation()], markerPayload);
  }
}
