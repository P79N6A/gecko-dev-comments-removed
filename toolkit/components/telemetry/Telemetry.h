




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
namespace HangMonitor {
  class HangAnnotations;
}
namespace Telemetry {

#include "TelemetryHistogramEnums.h"

enum TimerResolution {
  Millisecond,
  Microsecond
};




void Init();







void Accumulate(ID id, uint32_t sample);








void Accumulate(ID id, const nsCString& key, uint32_t sample = 1);










void Accumulate(const char* name, uint32_t sample);








void AccumulateTimeDelta(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now());




base::Histogram* GetHistogramById(ID id);




base::Histogram* GetKeyedHistogramById(ID id, const nsAString&);





template<TimerResolution res>
struct AccumulateDelta_impl
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now());
  static void compute(ID id, const nsCString& key, TimeStamp start, TimeStamp end = TimeStamp::Now());
};

template<>
struct AccumulateDelta_impl<Millisecond>
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, static_cast<uint32_t>((end - start).ToMilliseconds()));
  }
  static void compute(ID id, const nsCString& key, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, key, static_cast<uint32_t>((end - start).ToMilliseconds()));
  }
};

template<>
struct AccumulateDelta_impl<Microsecond>
{
  static void compute(ID id, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, static_cast<uint32_t>((end - start).ToMicroseconds()));
  }
  static void compute(ID id, const nsCString& key, TimeStamp start, TimeStamp end = TimeStamp::Now()) {
    Accumulate(id, key, static_cast<uint32_t>((end - start).ToMicroseconds()));
  }
};


template<ID id, TimerResolution res = Millisecond>
class AutoTimer {
public:
  explicit AutoTimer(TimeStamp aStart = TimeStamp::Now() MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
     : start(aStart)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  explicit AutoTimer(const nsCString& aKey, TimeStamp aStart = TimeStamp::Now() MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : start(aStart)
    , key(aKey)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoTimer() {
    if (key.IsEmpty()) {
      AccumulateDelta_impl<res>::compute(id, start);
    } else {
      AccumulateDelta_impl<res>::compute(id, key, start);
    }
  }

private:
  const TimeStamp start;
  const nsCString key;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template<ID id>
class AutoCounter {
public:
  explicit AutoCounter(uint32_t counterStart = 0 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
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




bool CanRecordBase();





bool CanRecordExtended();








void RecordSlowSQLStatement(const nsACString &statement,
                            const nsACString &dbName,
                            uint32_t delay);







void InitIOReporting(nsIFile* aXreDir);






void SetProfileDir(nsIFile* aProfD);




void LeavingStartupStage();




void EnteringShutdownStage();




const uint32_t kSlowSQLThresholdForMainThread = 50;
const uint32_t kSlowSQLThresholdForHelperThreads = 100;

class ProcessedStack;










#if defined(MOZ_ENABLE_PROFILER_SPS) && !defined(MOZILLA_XPCOMRT_API)
void RecordChromeHang(uint32_t aDuration,
                      ProcessedStack &aStack,
                      int32_t aSystemUptime,
                      int32_t aFirefoxUptime,
                      mozilla::UniquePtr<mozilla::HangMonitor::HangAnnotations>
                              aAnnotations);
#endif

class ThreadHangStats;











void RecordThreadHangStats(ThreadHangStats& aStats);






void WriteFailedProfileLock(nsIFile* aProfileDir);

} 
} 
#endif 
