





#include "PerformanceMeasure.h"
#include "MainThreadUtils.h"
#include "mozilla/dom/PerformanceMeasureBinding.h"

using namespace mozilla::dom;

PerformanceMeasure::PerformanceMeasure(nsISupports* aParent,
                                       const nsAString& aName,
                                       DOMHighResTimeStamp aStartTime,
                                       DOMHighResTimeStamp aEndTime)
: PerformanceEntry(aParent, aName, NS_LITERAL_STRING("measure")),
  mStartTime(aStartTime),
  mDuration(aEndTime - aStartTime)
{
  
  MOZ_ASSERT(mParent || !NS_IsMainThread());
}

PerformanceMeasure::~PerformanceMeasure()
{
}

JSObject*
PerformanceMeasure::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceMeasureBinding::Wrap(aCx, this, aGivenProto);
}
