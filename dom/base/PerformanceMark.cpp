




#include "PerformanceMark.h"
#include "mozilla/dom/PerformanceMarkBinding.h"

using namespace mozilla::dom;

PerformanceMark::PerformanceMark(nsPerformance* aPerformance,
                                 const nsAString& aName)
: PerformanceEntry(aPerformance, aName, NS_LITERAL_STRING("mark"))
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
  mStartTime = aPerformance->GetDOMTiming()->TimeStampToDOMHighRes(mozilla::TimeStamp::Now());
}

PerformanceMark::~PerformanceMark()
{
}

JSObject*
PerformanceMark::WrapObject(JSContext* aCx)
{
  return PerformanceMarkBinding::Wrap(aCx, this);
}
