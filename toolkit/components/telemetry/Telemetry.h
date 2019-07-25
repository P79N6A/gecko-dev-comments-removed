




#ifndef Telemetry_h__
#define Telemetry_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/StartupTimeline.h"
#include "nsTArray.h"
#include "nsStringGlue.h"
#if defined(MOZ_ENABLE_PROFILER_SPS)
#include "shared-libraries.h"
#endif

namespace base {
  class Histogram;
}

namespace mozilla {
namespace Telemetry {

enum ID {
#define HISTOGRAM(name, a, b, c, d, e) name,

#include "TelemetryHistograms.h"

#undef HISTOGRAM
HistogramCount
};




void Init();







void Accumulate(ID id, PRUint32 sample);








void AccumulateTimeDelta(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now());




base::Histogram* GetHistogramById(ID id);

template<ID id>
class AutoTimer {
public:
  AutoTimer(TimeStamp aStart = TimeStamp::Now() MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
     : start(aStart)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoTimer() {
    AccumulateTimeDelta(id, start);
  }

private:
  const TimeStamp start;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<ID id>
class AutoCounter {
public:
  AutoCounter(PRUint32 counterStart = 0 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : counter(counterStart)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoCounter() {
    Accumulate(id, counter);
  }

  
  void operator++() {
    ++counter;
  }

  
  void operator+=(int increment) {
    counter += increment;
  }

private:
  PRUint32 counter;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};






bool CanRecord();








void RecordSlowSQLStatement(const nsACString &statement,
                            const nsACString &dbName,
                            PRUint32 delay);




const PRUint32 kSlowStatementThreshold = 100;

class ProcessedStack;








#if defined(MOZ_ENABLE_PROFILER_SPS)
void RecordChromeHang(PRUint32 duration,
                      ProcessedStack &aStack);
#endif

} 
} 
#endif
