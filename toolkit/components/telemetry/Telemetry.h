





































#ifndef Telemetry_h__
#define Telemetry_h__

#include "mozilla/TimeStamp.h"

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
class Timer {
public:
  Timer():
    start(TimeStamp::Now())
  {
  }

  ~Timer() {
    Accumulate(id, (TimeStamp::Now() - start).ToMilliseconds());
  }

private:
  const TimeStamp start;
};
} 
} 
#endif 
