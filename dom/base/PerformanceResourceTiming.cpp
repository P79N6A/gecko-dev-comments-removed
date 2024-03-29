





#include "PerformanceResourceTiming.h"
#include "mozilla/dom/PerformanceResourceTimingBinding.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_INHERITED(PerformanceResourceTiming,
                                   PerformanceEntry,
                                   mTiming)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(PerformanceResourceTiming,
                                               PerformanceEntry)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(PerformanceResourceTiming)
NS_INTERFACE_MAP_END_INHERITING(PerformanceEntry)

NS_IMPL_ADDREF_INHERITED(PerformanceResourceTiming, PerformanceEntry)
NS_IMPL_RELEASE_INHERITED(PerformanceResourceTiming, PerformanceEntry)

PerformanceResourceTiming::PerformanceResourceTiming(nsPerformanceTiming* aPerformanceTiming,
                                                     nsPerformance* aPerformance,
                                                     const nsAString& aName)
: PerformanceEntry(aPerformance, aName, NS_LITERAL_STRING("resource")),
  mTiming(aPerformanceTiming)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
}

PerformanceResourceTiming::~PerformanceResourceTiming()
{
}

DOMHighResTimeStamp
PerformanceResourceTiming::StartTime() const
{
  DOMHighResTimeStamp startTime = mTiming->RedirectStartHighRes();
  return startTime ? startTime : mTiming->FetchStartHighRes();
}

JSObject*
PerformanceResourceTiming::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceResourceTimingBinding::Wrap(aCx, this, aGivenProto);
}
