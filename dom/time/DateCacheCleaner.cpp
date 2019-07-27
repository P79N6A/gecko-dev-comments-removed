




#include "DateCacheCleaner.h"

#include "jsapi.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Hal.h"
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
    mozilla::AutoSafeJSContext cx;
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
