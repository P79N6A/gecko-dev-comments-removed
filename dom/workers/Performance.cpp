





#include "Performance.h"
#include "mozilla/dom/PerformanceBinding.h"

#include "WorkerPrivate.h"

BEGIN_WORKERS_NAMESPACE

Performance::Performance(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
}

Performance::~Performance()
{
  mWorkerPrivate->AssertIsOnWorkerThread();
}

JSObject*
Performance::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceBinding_workers::Wrap(aCx, this, aGivenProto);
}

DOMHighResTimeStamp
Performance::Now() const
{
  TimeDuration duration =
    TimeStamp::Now() - mWorkerPrivate->NowBaseTimeStamp();
  return duration.ToMilliseconds();
}


bool
Performance::IsPerformanceTimingAttribute(const nsAString& aName)
{
  
  return aName.EqualsASCII("navigationStart");
}

DOMHighResTimeStamp
Performance::GetPerformanceTimingFromString(const nsAString& aProperty)
{
  if (!IsPerformanceTimingAttribute(aProperty)) {
    return 0;
  }

  if (aProperty.EqualsLiteral("navigationStart")) {
    return mWorkerPrivate->NowBaseTimeHighRes();
  }

  MOZ_CRASH("IsPerformanceTimingAttribute and GetPerformanceTimingFromString are out of sync");
  return 0;
}

DOMHighResTimeStamp
Performance::DeltaFromNavigationStart(DOMHighResTimeStamp aTime)
{
  if (aTime == 0) {
    return 0;
  }

  return aTime - mWorkerPrivate->NowBaseTimeHighRes();
}

void
Performance::DispatchBufferFullEvent()
{
  
  
  MOZ_CRASH("This should not be called.");
}

END_WORKERS_NAMESPACE
