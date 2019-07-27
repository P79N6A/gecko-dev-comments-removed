





#ifndef MOZILLA_MEDIASOURCEUTILS_H_
#define MOZILLA_MEDIASOURCEUTILS_H_

#include "nsString.h"
#include "TimeUnits.h"

namespace mozilla {

nsCString DumpTimeRanges(const media::TimeIntervals& aRanges);

} 

#endif 
