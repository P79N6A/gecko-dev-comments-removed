




#ifndef mozilla_dom_performancemark_h___
#define mozilla_dom_performancemark_h___

#include "mozilla/dom/PerformanceEntry.h"

namespace mozilla {
namespace dom {


class PerformanceMark MOZ_FINAL : public PerformanceEntry
{
public:
  PerformanceMark(nsPerformance* aPerformance,
                  const nsAString& aName);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual DOMHighResTimeStamp StartTime() const MOZ_OVERRIDE
  {
    return mStartTime;
  }

protected:
  virtual ~PerformanceMark();
  DOMHighResTimeStamp mStartTime;
};

} 
} 

#endif 
