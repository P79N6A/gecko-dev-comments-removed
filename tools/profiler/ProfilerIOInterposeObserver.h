



#ifndef PROFILERIOINTERPOSEOBSERVER_H
#define PROFILERIOINTERPOSEOBSERVER_H

#ifdef MOZ_ENABLE_PROFILER_SPS

#include "mozilla/IOInterposer.h"

namespace mozilla {





class ProfilerIOInterposeObserver MOZ_FINAL : public IOInterposeObserver
{
public:
  ProfilerIOInterposeObserver();
  ~ProfilerIOInterposeObserver();

  virtual void Observe(Operation aOp, double& aDuration,
                       const char* aModuleInfo);
};

} 

#endif 

#endif 

