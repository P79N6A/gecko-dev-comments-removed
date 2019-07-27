




#include "PerformanceMeasure.h"
#include "mozilla/dom/PerformanceMeasureBinding.h"

using namespace mozilla::dom;

PerformanceMeasure::PerformanceMeasure(nsPerformance* aPerformance,
                                       const nsAString& aName,
                                       DOMHighResTimeStamp aStartTime,
                                       DOMHighResTimeStamp aEndTime)
: PerformanceEntry(aPerformance, aName, NS_LITERAL_STRING("measure")),
  mStartTime(aStartTime),
  mDuration(aEndTime - aStartTime)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
}

PerformanceMeasure::~PerformanceMeasure()
{
}

JSObject*
PerformanceMeasure::WrapObject(JSContext* aCx)
{
  return PerformanceMeasureBinding::Wrap(aCx, this);
}
