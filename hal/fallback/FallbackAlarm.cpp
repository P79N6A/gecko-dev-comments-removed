



#include "Hal.h"

#include <algorithm>

#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace hal_impl {

static void
TimerCallbackFunc(nsITimer *aTimer, void *aClosure)
{
  hal::NotifyAlarmFired();
}

static StaticRefPtr<nsITimer> sTimer;

bool
EnableAlarm()
{
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    ClearOnShutdown(&sTimer);
  }

  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
  sTimer = timer;
  MOZ_ASSERT(sTimer);
  return true;
}

void
DisableAlarm()
{
  



  if (sTimer) {
    sTimer->Cancel();
  }
}

bool
SetAlarm(int32_t aSeconds, int32_t aNanoseconds)
{
  if (!sTimer) {
    HAL_LOG(("We should have enabled the alarm"));
    MOZ_ASSERT(false);
    return false;
  }

  
  
  int64_t milliseconds = static_cast<int64_t>(aSeconds) * 1000 +
                         static_cast<int64_t>(aNanoseconds) / 1000000;

  
  int64_t relMilliseconds = milliseconds - PR_Now() / 1000;

  
  
  
  sTimer->InitWithFuncCallback(TimerCallbackFunc, nullptr,
                               clamped<int64_t>(relMilliseconds, 0, INT32_MAX),
                               nsITimer::TYPE_ONE_SHOT);
  return true;
}

} 
} 
