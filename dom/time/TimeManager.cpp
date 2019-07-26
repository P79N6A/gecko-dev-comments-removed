



#include "TimeManager.h"

#include "mozilla/dom/Date.h"
#include "mozilla/dom/MozTimeManagerBinding.h"
#include "nsITimeService.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {
namespace time {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(TimeManager)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(TimeManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TimeManager)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(TimeManager, mWindow)

JSObject*
TimeManager::WrapObject(JSContext* aCx)
{
  return MozTimeManagerBinding::Wrap(aCx, this);
}

void
TimeManager::Set(Date& aDate)
{
  Set(aDate.TimeStamp());
}

void
TimeManager::Set(double aTime)
{
  nsCOMPtr<nsITimeService> timeService = do_GetService(TIMESERVICE_CONTRACTID);
  if (timeService) {
    timeService->Set(aTime);
  }
}

} 
} 
} 
