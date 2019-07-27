





#ifndef MOZILLA_MEDIASOURCEUTILS_H_
#define MOZILLA_MEDIASOURCEUTILS_H_

#include "nsString.h"

namespace mozilla {

namespace dom {
  class TimeRanges;
} 

nsCString DumpTimeRanges(dom::TimeRanges* aRanges);

} 

#endif 
