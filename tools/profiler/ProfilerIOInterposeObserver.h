



#ifndef PROFILERIOINTERPOSEOBSERVER_H
#define PROFILERIOINTERPOSEOBSERVER_H

#ifdef MOZ_ENABLE_PROFILER_SPS

#include "mozilla/IOInterposer.h"

namespace mozilla {





class ProfilerIOInterposeObserver MOZ_FINAL : public IOInterposeObserver
{
public:
  virtual void Observe(Observation& aObservation);
};

} 

#endif 

#endif 
