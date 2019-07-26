



#include "base/basictypes.h"
#include "jsapi.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Hal.h"
#include "TimeService.h"

namespace mozilla {
namespace dom {
namespace time {

NS_IMPL_ISUPPORTS1(TimeService, nsITimeService)

 StaticRefPtr<TimeService> TimeService::sSingleton;

 already_AddRefed<TimeService>
TimeService::GetInstance()
{
  if (!sSingleton) {
    sSingleton = new TimeService();
    ClearOnShutdown(&sSingleton);
  }
  nsRefPtr<TimeService> service = sSingleton.get();
  return service.forget();
}

NS_IMETHODIMP
TimeService::Set(int64_t aTimeInMS) {
  hal::AdjustSystemClock(aTimeInMS - (JS_Now() / PR_USEC_PER_MSEC));
  return NS_OK;
}

} 
} 
} 
