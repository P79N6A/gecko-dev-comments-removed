



#include "Performance.h"
#include "mozilla/dom/PerformanceBinding.h"

#include "WorkerPrivate.h"

BEGIN_WORKERS_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(Performance, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(Performance, Release)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(Performance)

Performance::Performance(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
{
  mWorkerPrivate->AssertIsOnWorkerThread();
  SetIsDOMBinding();
}

Performance::~Performance()
{
  mWorkerPrivate->AssertIsOnWorkerThread();
}

JSObject*
Performance::WrapObject(JSContext* aCx)
{
  return PerformanceBinding_workers::Wrap(aCx, this);
}

double
Performance::Now() const
{
  TimeDuration duration =
    TimeStamp::Now() - mWorkerPrivate->NowBaseTimeStamp();
  return duration.ToMilliseconds();
}

END_WORKERS_NAMESPACE
