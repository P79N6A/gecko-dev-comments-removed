



#ifndef mozilla_dom_PromiseWorkerProxy_h
#define mozilla_dom_PromiseWorkerProxy_h


#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/workers/bindings/WorkerFeature.h"
#include "nsProxyRelease.h"

#include "WorkerRunnable.h"

namespace mozilla {
namespace dom {

class Promise;

namespace workers {
class WorkerPrivate;
}









































class PromiseWorkerProxy : public PromiseNativeHandler,
                           public workers::WorkerFeature
{
  friend class PromiseWorkerProxyRunnable;

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PromiseWorkerProxy, override)

public:
  static already_AddRefed<PromiseWorkerProxy>
  Create(workers::WorkerPrivate* aWorkerPrivate,
         Promise* aWorkerPromise,
         const JSStructuredCloneCallbacks* aCallbacks = nullptr);

  workers::WorkerPrivate* GetWorkerPrivate() const;

  Promise* GetWorkerPromise() const;

  void StoreISupports(nsISupports* aSupports);

  void CleanUp(JSContext* aCx);

  Mutex& GetCleanUpLock()
  {
    return mCleanUpLock;
  }

  bool IsClean() const
  {
    mCleanUpLock.AssertCurrentThreadOwns();
    return mCleanedUp;
  }

protected:
  virtual void ResolvedCallback(JSContext* aCx,
                                JS::Handle<JS::Value> aValue) override;

  virtual void RejectedCallback(JSContext* aCx,
                                JS::Handle<JS::Value> aValue) override;

  virtual bool Notify(JSContext* aCx, workers::Status aStatus) override;

private:
  PromiseWorkerProxy(workers::WorkerPrivate* aWorkerPrivate,
                     Promise* aWorkerPromise,
                     const JSStructuredCloneCallbacks* aCallbacks = nullptr);

  virtual ~PromiseWorkerProxy();

  
  typedef void (Promise::*RunCallbackFunc)(JSContext*,
                                           JS::Handle<JS::Value>);

  void RunCallback(JSContext* aCx,
                   JS::Handle<JS::Value> aValue,
                   RunCallbackFunc aFunc);

  workers::WorkerPrivate* mWorkerPrivate;

  
  nsRefPtr<Promise> mWorkerPromise;

  bool mCleanedUp; 

  const JSStructuredCloneCallbacks* mCallbacks;

  
  
  nsTArray<nsMainThreadPtrHandle<nsISupports>> mSupportsArray;

  
  Mutex mCleanUpLock;
};




class PromiseWorkerProxyControlRunnable final : public workers::WorkerControlRunnable
{
  nsRefPtr<PromiseWorkerProxy> mProxy;

public:
  PromiseWorkerProxyControlRunnable(workers::WorkerPrivate* aWorkerPrivate,
                                    PromiseWorkerProxy* aProxy)
    : WorkerControlRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount)
    , mProxy(aProxy)
  {
    MOZ_ASSERT(aProxy);
  }

  virtual bool
  WorkerRun(JSContext* aCx, workers::WorkerPrivate* aWorkerPrivate) override;

private:
  ~PromiseWorkerProxyControlRunnable()
  {}
};

} 
} 

#endif 
