




#include "mozilla/Hal.h"
#include "mozilla/ClearOnShutdown.h"
#include "DateCacheCleaner.h"

#include "nsContentUtils.h"
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
    JSAutoRequest ar(cx);
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
