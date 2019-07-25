





































#ifndef TimingStruct_h_
#define TimingStruct_h_

#include "mozilla/TimeStamp.h"

struct TimingStruct {
  mozilla::TimeStamp domainLookupStart;
  mozilla::TimeStamp domainLookupEnd;
  mozilla::TimeStamp connectStart;
  mozilla::TimeStamp connectEnd;
  mozilla::TimeStamp requestStart;
  mozilla::TimeStamp responseStart;
  mozilla::TimeStamp responseEnd;
};

#endif
