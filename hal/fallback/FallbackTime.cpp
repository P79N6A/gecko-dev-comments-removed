






#include "Hal.h"

namespace mozilla {
namespace hal_impl {

void 
AdjustSystemClock(int64_t aDeltaMilliseconds)
{}

void
SetTimezone(const nsCString& aTimezoneSpec)
{}

nsCString
GetTimezone()
{
  return EmptyCString();
}

int32_t
GetTimezoneOffset()
{
  return 0;
}

void
EnableSystemClockChangeNotifications()
{
}

void
DisableSystemClockChangeNotifications()
{
}

void
EnableSystemTimezoneChangeNotifications()
{
}

void
DisableSystemTimezoneChangeNotifications()
{
}

} 
} 
