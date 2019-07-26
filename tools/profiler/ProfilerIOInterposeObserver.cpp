



#include "GeckoProfiler.h"
#include "ProfilerIOInterposeObserver.h"

using namespace mozilla;

void ProfilerIOInterposeObserver::Observe(Observation& aObservation)
{
  
  
  if (NS_IsMainThread()) {
    const char* ops[] = {"none", "read", "write", "invalid", "fsync"};
    PROFILER_MARKER(ops[aObservation.ObservedOperation()]);
  }
}
