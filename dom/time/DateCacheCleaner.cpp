




#include "mozilla/Hal.h"
#include "mozilla/ClearOnShutdown.h"
#include "DateCacheCleaner.h"

#include "nsIJSContextStack.h"
#include "mozilla/StaticPtr.h"

using namespace mozilla::hal;

namespace mozilla {
namespace dom {
namespace time {

class DateCacheCleaner : public SystemTimezoneChangeObserver
{
public:
  DateCacheCleaner()
  {
    RegisterSystemTimezoneChangeObserver(this);
  }

  ~DateCacheCleaner()
  {
    UnregisterSystemTimezoneChangeObserver(this);
  }
  void Notify(const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo)
  {
    nsCOMPtr<nsIThreadJSContextStack> stack =
      do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    if (!stack) {
      NS_WARNING("Failed to get JSContextStack");
    }
    JSContext *cx = stack->GetSafeJSContext();
    if (!cx) {
      NS_WARNING("Failed to GetSafeJSContext");
    }
    JS_ClearDateCaches(cx);
  }

};

StaticAutoPtr<DateCacheCleaner> sDateCacheCleaner;

void
InitializeDateCacheCleaner()
{
  if (!sDateCacheCleaner) {
    sDateCacheCleaner = new DateCacheCleaner();
    ClearOnShutdown(&sDateCacheCleaner);
  }
}

} 
} 
} 
