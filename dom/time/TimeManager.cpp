



#include "jsapi.h"
#include "nsIDOMClassInfo.h"
#include "nsITimeService.h"
#include "TimeManager.h"

DOMCI_DATA(MozTimeManager, mozilla::dom::time::TimeManager)

namespace mozilla {
namespace dom {
namespace time {

NS_INTERFACE_MAP_BEGIN(TimeManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozTimeManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozTimeManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(TimeManager)
NS_IMPL_RELEASE(TimeManager)

nsresult
TimeManager::Set(const JS::Value& date, JSContext* ctx) {
  double dateMSec;

  if (date.isObject()) {
    JSObject* dateObj = JSVAL_TO_OBJECT(date);

    if (JS_ObjectIsDate(ctx, dateObj) && js_DateIsValid(dateObj)) {
      dateMSec = js_DateGetMsecSinceEpoch(dateObj);
    }
    else {
      NS_WARN_IF_FALSE(JS_ObjectIsDate(ctx, dateObj), "This is not a Date object");
      NS_WARN_IF_FALSE(js_DateIsValid(dateObj), "Date is not valid");
      return NS_ERROR_INVALID_ARG;
    }
  } else if (date.isNumber()) {
    dateMSec = date.toNumber();
  } else {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsITimeService> timeService = do_GetService(TIMESERVICE_CONTRACTID);
  if (timeService) {
    return timeService->Set(dateMSec);
  }
  return NS_OK;
}

} 
} 
} 
