





#ifndef mozilla_dom_workers_performance_h__
#define mozilla_dom_workers_performance_h__

#include "nsWrapperCache.h"
#include "js/TypeDecls.h"
#include "Workers.h"
#include "nsISupportsImpl.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPerformance.h"

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;

class Performance final : public PerformanceBase
{
public:
  explicit Performance(WorkerPrivate* aWorkerPrivate);

private:
  ~Performance();

  WorkerPrivate* mWorkerPrivate;

public:
  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  DOMHighResTimeStamp Now() const override;

  using PerformanceBase::Mark;
  using PerformanceBase::ClearMarks;
  using PerformanceBase::Measure;
  using PerformanceBase::ClearMeasures;

private:
  nsISupports* GetAsISupports() override
  {
    return nullptr;
  }

  void DispatchBufferFullEvent() override;

  bool IsPerformanceTimingAttribute(const nsAString& aName) override;

  DOMHighResTimeStamp
  GetPerformanceTimingFromString(const nsAString& aTimingName) override;

  DOMHighResTimeStamp
  DeltaFromNavigationStart(DOMHighResTimeStamp aTime) override;
};

END_WORKERS_NAMESPACE

#endif 
