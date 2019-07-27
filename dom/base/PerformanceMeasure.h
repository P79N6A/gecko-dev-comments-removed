




#ifndef mozilla_dom_performancemeasure_h___
#define mozilla_dom_performancemeasure_h___

#include "mozilla/dom/PerformanceEntry.h"

namespace mozilla {
namespace dom {


class PerformanceMeasure MOZ_FINAL : public PerformanceEntry
{
public:
  PerformanceMeasure(nsPerformance* aPerformance,
                     const nsAString& aName,
                     DOMHighResTimeStamp aStartTime,
                     DOMHighResTimeStamp aEndTime);

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  virtual DOMHighResTimeStamp StartTime() const MOZ_OVERRIDE
  {
    return mStartTime;
  }

  virtual DOMHighResTimeStamp Duration() const MOZ_OVERRIDE
  {
    return mDuration;
  }

protected:
  virtual ~PerformanceMeasure();
  DOMHighResTimeStamp mStartTime;
  DOMHighResTimeStamp mDuration;
};

} 
} 

#endif 
