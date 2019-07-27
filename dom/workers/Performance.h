




#ifndef mozilla_dom_workers_performance_h__
#define mozilla_dom_workers_performance_h__

#include "nsWrapperCache.h"
#include "js/TypeDecls.h"
#include "Workers.h"
#include "nsISupportsImpl.h"
#include "nsCycleCollectionParticipant.h"

BEGIN_WORKERS_NAMESPACE

class WorkerPrivate;

class Performance MOZ_FINAL : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Performance)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(Performance)

  explicit Performance(WorkerPrivate* aWorkerPrivate);

private:
  ~Performance();

  WorkerPrivate* mWorkerPrivate;

public:
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  nsISupports*
  GetParentObject() const
  {
    
    return nullptr;
  }

  
  double Now() const;
};

END_WORKERS_NAMESPACE

#endif 
