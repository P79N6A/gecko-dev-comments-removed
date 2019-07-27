









#ifndef WEBRTC_BASE_TIMEUTILS_H_
#define WEBRTC_BASE_TIMEUTILS_H_

#include <time.h>

#include "webrtc/base/basictypes.h"

namespace rtc {

static const int64 kNumMillisecsPerSec = INT64_C(1000);
static const int64 kNumMicrosecsPerSec = INT64_C(1000000);
static const int64 kNumNanosecsPerSec = INT64_C(1000000000);

static const int64 kNumMicrosecsPerMillisec = kNumMicrosecsPerSec /
    kNumMillisecsPerSec;
static const int64 kNumNanosecsPerMillisec =  kNumNanosecsPerSec /
    kNumMillisecsPerSec;
static const int64 kNumNanosecsPerMicrosec =  kNumNanosecsPerSec /
    kNumMicrosecsPerSec;


static const int64 kJan1970AsNtpMillisecs = INT64_C(2208988800000);

typedef uint32 TimeStamp;


uint32 Time();

uint64 TimeMicros();

uint64 TimeNanos();


void CurrentTmTime(struct tm *tm, int *microseconds);


uint32 TimeAfter(int32 elapsed);


bool TimeIsBetween(uint32 earlier, uint32 middle, uint32 later);  
bool TimeIsLaterOrEqual(uint32 earlier, uint32 later);  
bool TimeIsLater(uint32 earlier, uint32 later);  


inline uint32 TimeMax(uint32 ts1, uint32 ts2) {
  return TimeIsLaterOrEqual(ts1, ts2) ? ts2 : ts1;
}


inline uint32 TimeMin(uint32 ts1, uint32 ts2) {
  return TimeIsLaterOrEqual(ts1, ts2) ? ts1 : ts2;
}



int32 TimeDiff(uint32 later, uint32 earlier);


inline int32 TimeSince(uint32 earlier) {
  return TimeDiff(Time(), earlier);
}


inline int32 TimeUntil(uint32 later) {
  return TimeDiff(later, Time());
}


inline int64 UnixTimestampNanosecsToNtpMillisecs(int64 unix_ts_ns) {
  return unix_ts_ns / kNumNanosecsPerMillisec + kJan1970AsNtpMillisecs;
}

class TimestampWrapAroundHandler {
 public:
  TimestampWrapAroundHandler();

  int64 Unwrap(uint32 ts);

 private:
  uint32 last_ts_;
  int64 num_wrap_;
};

}  

#endif  
