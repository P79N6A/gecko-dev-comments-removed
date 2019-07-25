





































#ifndef Telemetry_h__
#define Telemetry_h__

#include "mozilla/TimeStamp.h"
#include "mozilla/AutoRestore.h"

namespace mozilla {
namespace Telemetry {

enum ID {
#define HISTOGRAM(name, a, b, c, d, e) name,

#include "TelemetryHistograms.h"

#undef HISTOGRAM
HistogramCount
};







void Accumulate(ID id, PRUint32 sample);

template<ID id>
class AutoTimer {
public:
  AutoTimer(MOZILLA_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    : start(TimeStamp::Now())
  {
    MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoTimer() {
    Accumulate(id, (TimeStamp::Now() - start).ToMilliseconds());
  }

private:
  const TimeStamp start;
  MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
};
} 
} 
#endif
