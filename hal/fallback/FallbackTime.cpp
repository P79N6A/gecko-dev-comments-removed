






#include "Hal.h"

namespace mozilla {
namespace hal_impl {

void 
AdjustSystemClock(int32_t aDeltaMilliseconds)
{}

void
SetTimezone(const nsCString& aTimezoneSpec)
{}

nsCString
GetTimezone()
{
  return EmptyCString();
}

} 
} 
