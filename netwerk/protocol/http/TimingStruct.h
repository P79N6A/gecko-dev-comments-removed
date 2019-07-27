




#ifndef TimingStruct_h_
#define TimingStruct_h_

#include "mozilla/TimeStamp.h"

namespace mozilla { namespace net {

struct TimingStruct {
  TimeStamp domainLookupStart;
  TimeStamp domainLookupEnd;
  TimeStamp connectStart;
  TimeStamp connectEnd;
  TimeStamp requestStart;
  TimeStamp responseStart;
  TimeStamp responseEnd;
};

struct ResourceTimingStruct : TimingStruct {
  TimeStamp fetchStart;
  TimeStamp redirectStart;
  TimeStamp redirectEnd;
};

} 
} 

#endif
