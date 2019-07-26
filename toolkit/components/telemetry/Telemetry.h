




#ifndef Telemetry_h__
#define Telemetry_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/StartupTimeline.h"
#include "nsTArray.h"
#include "nsStringGlue.h"

namespace base {
  class Histogram;
}

namespace mozilla {
namespace Telemetry {

#include "TelemetryHistogramEnums.h"

enum TimerResolution {
  Millisecond,
  Microsecond
};




void Init();







void Accumulate(ID id, uint32_t sample);










void Accumulate(const char* name, uint32_t sample);








void AccumulateTimeDelta(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now());




base::Histogram* GetHistogramById(ID id);





template<TimerResolution res>
struct AccumulateDelta_impl
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now());
};

template<>
struct AccumulateDelta_impl<Millisecond>
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, static_cast<uint32_t>((end - start).ToMilliseconds()));
  }
};

template<>
struct AccumulateDelta_impl<Microsecond>
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, static_cast<uint32_t>((end - start).ToMicroseconds()));
  }
};


template<ID id, TimerResolution res = Millisecond>
class AutoTimer {
public:
  AutoTimer(TimeStamp aStart = TimeStamp::Now() MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
     : start(aStart)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoTimer() {
    AccumulateDelta_impl<res>::compute(id, start);
  }

private:
  const TimeStamp start;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<ID id>
class AutoCounter {
public:
  AutoCounter(uint32_t counterStart = 0 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
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
  uint32_t counter;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};






bool CanRecord();








void RecordSlowSQLStatement(const nsACString &statement,
                            const nsACString &dbName,
                            uint32_t delay);




const uint32_t kSlowSQLThresholdForMainThread = 50;
const uint32_t kSlowSQLThresholdForHelperThreads = 100;

class ProcessedStack;









#if defined(MOZ_ENABLE_PROFILER_SPS)
void RecordChromeHang(uint32_t aDuration,
                      ProcessedStack &aStack,
                      int32_t aSystemUptime,
                      int32_t aFirefoxUptime);
#endif

class ThreadHangStats;











void RecordThreadHangStats(ThreadHangStats& aStats);






void WriteFailedProfileLock(nsIFile* aProfileDir);

} 
} 
#endif 
