



#include "ProfilerIOInterposeObserver.h"

#include "GeckoProfiler.h"

using namespace mozilla;

ProfilerIOInterposeObserver::ProfilerIOInterposeObserver()
{
  IOInterposer* interposer = IOInterposer::GetInstance();
  if (interposer) {
    interposer->Register(IOInterposeObserver::OpAll, this);
  }
}

ProfilerIOInterposeObserver::~ProfilerIOInterposeObserver()
{
  IOInterposer* interposer = IOInterposer::GetInstance();
  if (interposer) {
    interposer->Deregister(IOInterposeObserver::OpAll, this);
  }
}

void ProfilerIOInterposeObserver::Observe(IOInterposeObserver::Operation aOp,
                                          double& aDuration,
                                          const char* aModuleInfo)
{
  MOZ_ASSERT(NS_IsMainThread());
  const char* ops[] = {"none", "read", "write", "invalid", "fsync"};
  PROFILER_MARKER(ops[aOp]);
}

